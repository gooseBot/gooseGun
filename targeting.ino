const float _nozzelVelocity = 35.0;        //feet/sec - Seems accurate for yard
//const float _nozzelVelocity = 20.0;        //feet/sec - indoor testing
const float _nozzelAboveGroundDistance = -7;
const byte _tiltServoNeutralAngle = 125;
const float _gravity = 32.0;               //feet/sec/sec
const byte _valveMosfetPin = 7;
float _maxRange = 0;
float _angle = 0;
byte _distance = 0;
long _totDifferences = 0;
float _arcLength = 0;
//const byte _laserPin = 2;

const int _numScanReturns = 360;
static byte _baseScan[_numScanReturns];
static byte _scan[_numScanReturns];
static Servo _bottomServo;            
static Servo _topServo;               

void initializeTargeting(){
  _maxRange = sqrt(((pow(_nozzelVelocity, 4) / _gravity) - 2 * _nozzelAboveGroundDistance*pow(_nozzelVelocity, 2)) / _gravity);
  pinMode(_valveMosfetPin, OUTPUT);
  //pinMode(_laserPin, OUTPUT);
  closeValve();
}

int getNumScanReturns(){
  return _numScanReturns;
}

float getMaxRange(){
  return _maxRange;
}

float getAngle(){
  return _angle;
}

float setAngle(float angle){
  _angle = angle;
}

byte getBaseScanByte(int index){
  return _baseScan[index];
}

void setBaseScanByte(int index, byte value){
  _baseScan[index] = value;
}

byte getScanByte(int index){
  return _scan[index];
}

void setScanByte(int index, byte value){
  _scan[index] = value;
}

void clearScanArray(){
  for (int i = 0; i < _numScanReturns; i++) { _scan[i] = 0; }
}

byte getDistance(){
  return _distance;
}

byte setDistance(byte distance){
  _distance = distance;;
}

long getTotDifferences(){
  return _totDifferences;
}

float getArcLength(){
  return _arcLength;
}

void controlNozzelServos(boolean turnOn) {
  const byte bottomServoPin = 9;
  const byte topServoPin = 14;  //analog 14
  if (turnOn) {
    _bottomServo.attach(bottomServoPin, 544, 2400);
    _topServo.attach(topServoPin, 1050, 2400);
    _bottomServo.write(90);                         //straight ahead
    _topServo.write(_tiltServoNeutralAngle);       // put tilt servo at "level" position 
  }
  else {
    _topServo.detach();
    _bottomServo.detach();
  }
  delay(1000);       //wait for nozzels to stop moving or motion is caught
}

void closeValve(){
  digitalWrite(_valveMosfetPin, LOW);
  //digitalWrite(_laserPin, LOW);
}

void openValve(){
  digitalWrite(_valveMosfetPin, HIGH);
  //digitalWrite(_laserPin, HIGH);
}

void processScanData() {
  int maxConsecutivei = 0;
  int measure1ft = 0;
  int measure2ft = 0;
  int i = 0;
  int consecutivei = 0;
  int nexti = 0;
  unsigned int sumi = 0;
  unsigned int sumMeasures = 0;
  int difference = 0;
  int sumDifferences = 0;
  int lasti = 0;
  int avgi = 0;
  float arcLength = 0.0;
  const byte minDistanceToScanner = 3;
  byte avgMeasure = 0;  
  boolean readingAgroup = false;

  // zero out globals used to pass out results of this method
  _angle = 0;
  _distance = 0;
  _arcLength = 0;
  _totDifferences = 0;
  
  // process all points in this scan  
  for (i=0; i < (_numScanReturns); i++) {    
    measure1ft = _baseScan[i];     
    measure2ft = _scan[i];
    
    //compute difference between base scan and current scan angle by angle
    difference = measure1ft - measure2ft;
    //assume the end of a cluster of consecutive points (target) that differ from base scan
    readingAgroup = false;  
    // we are looking for consecutive differences that are more than 1 foot in size
    //   this may be a problem as a goose may be only one shot wide at a large enough distance
    //   the pulse of light expands with distance. It can be several inches wide.
    if (abs(difference) > 1) {
      //  need at least two consecutive differences to make a group
      //  count them and average the angle and distance
      nexti = lasti + 1;   
      if (i == nexti) {          
        consecutivei++;
        sumi += i;
        sumMeasures += measure2ft;
        sumDifferences += difference;
        readingAgroup = true;        //now have a group of at least 2 points
      }  
      lasti = i;      //reset variable used to see if we are still reading a target
    }
    // if we were processing a group (target) and it has ended, then process it
    if (consecutivei > 0 && !readingAgroup) {
      avgi = sumi / consecutivei;
      avgMeasure = float(sumMeasures/consecutivei) + 0.5;         //add .5 then truncate, this rounds the value
      arcLength = avgMeasure*(((consecutivei*0.5) * 71) / 4068);  //width of return
      // don't waste time on targets that are out of range or are moving away, or too close to scanner
	    //   water drips from the saucer and can cause false targets during rain
      //   ignore small width returns, probably will be rain drops
      if (avgMeasure <= _maxRange && sumDifferences > 0 && avgMeasure >= minDistanceToScanner && arcLength > .05) {
        if ((maxConsecutivei == 0) || (consecutivei > maxConsecutivei)) {          
          // a larger valid target (or the first one) has been found, compute needed information
          _arcLength = arcLength;
          maxConsecutivei = consecutivei;
          _distance = avgMeasure;
          _angle = avgi*0.5;
          _totDifferences = sumDifferences;  
        }                    
      }
      // reset for next target found in this scan
      consecutivei = 0;
      sumMeasures = 0;
      sumi = 0;  
      avgi = 0;
      avgMeasure = 0;
    }
  }
}

void moveServosAndShootTarget()
{ 
  const byte wiggleAmount = 7;
  //calc the theoretical tilt angle to point the nozzel at the ground where the target sits
  //  found this formula on wikipedia, remeber that y is negative
  //  use the minus root for the flatest trajectory
  float vsqr = pow(_nozzelVelocity,2);
  float rootOp = pow(_nozzelVelocity,4) - 
    _gravity*(_gravity*pow((float)_distance,2) + 2*_nozzelAboveGroundDistance*vsqr);
  //if root operand is neg then target is out of range of the water pressure/velocity. 
      //Note: this case above shouldnt happen as I'm testing for this condition during target selection.  
      //so I can prevent the case of selecting a target during the target selection process
      //and then later ignoring it here due to the target being out of range and thus getting a deadlock going
  if (rootOp < 0) {return;}   //didnt shoot at anything                 
  float root = sqrt(rootOp);
  float gx = _gravity*_distance;
  float rootMinusAngle = 57.295 * atan((vsqr-root)/gx);
  
  //this servo has a 180 degree range and is pointed upwards by some angle
  //  its range of motion is limited to 90 degrees by the pan tilt mechanisim though.
  //  the servo is tilted so it is "level", this seems to be 110 degrees.
  byte adjustedTiltServoAngle = _tiltServoNeutralAngle + rootMinusAngle;

  //don't let it get to extremes the servo cant do well
  if (adjustedTiltServoAngle < 60) 
    adjustedTiltServoAngle = 60;
  if (adjustedTiltServoAngle > 160)
    adjustedTiltServoAngle = 160;  

  // move servos to hit target 
  int panAngle = (180-(int)_angle); 
  if (getManualMode()){
    _bottomServo.write(panAngle);               // go to desired position 
    _topServo.write(adjustedTiltServoAngle);    // go to desired position 
  }  else {
    panAngle -= wiggleAmount;
    adjustedTiltServoAngle -= wiggleAmount;
    _bottomServo.write(panAngle);               // go to desired position minus wiggle amount degree         
    _topServo.write(adjustedTiltServoAngle);    // go to desired position minus wiggle amount degree       
    myDelay(200);                               // wait for servos to finish moving.  
    // will open the valve for about 1 second.  Takes a while for the water to get going.
    // will also wiggle the two servos in a pattern left right up down a few times
    openValve();
    for (int repeat = 0; repeat < 1; repeat += 1)
    {
      for (int wiggle = 0; wiggle < (2 * wiggleAmount); wiggle += 1)
      {
        _topServo.write(adjustedTiltServoAngle + wiggle);
        _bottomServo.write(panAngle + wiggle);
        myDelay(60);
      }
      for (int wiggle = (2 * wiggleAmount); wiggle < 0; wiggle -= 1)
      {
        _topServo.write(adjustedTiltServoAngle + wiggle);
        _bottomServo.write(panAngle + wiggle);
        myDelay(60);
      }
    }
    closeValve();
  }
}