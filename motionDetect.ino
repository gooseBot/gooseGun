//static float _stdDeviationPulseRate = 0;
static float _pulsesPerSecAvg = 2;
static volatile int _pulsesPerSec = 0;
static boolean _gooseDectectorEvent = false;

float getPulsesPerSecAvg(){
  return _pulsesPerSecAvg;
}

float getPulsesPerSec(){
  return _pulsesPerSec;
}

void setGooseDectectorEvent(){
  //if the current threshold is high it is probably raining
  //  the goose dectector has trouble with rain
  if (_pulsesPerSecAvg < 15.0) _gooseDectectorEvent = true;
}

boolean detectMovement(boolean resetRollingAvgNumbers)
{
  _pulsesPerSec=0;                       //reset the count
  static int index = 0;                  // the index of the current reading
  static int total = 0;                  // the running total
  static int readingCount = 0;
  static int zero = 0;

  // reset the numbers used to compute rolling average, needed after an attack is done
  //  Im reusing the scan array to compute the rolling average
  //  this works since there is an enforced 10min rest time between attacks
  if (resetRollingAvgNumbers) {
    index=0;
    total=0;
    readingCount=0;
    clearScanArray();
    _hist.clear();                  //also clear the histogram data
    zero=0;    
    return false;    
  }
    
  // get num of pulses for one second from the xband
  unsigned long starttime = millis();      //going to count for a fixed time
  attachInterrupt(1, onPulse, RISING);     //hooked xband to pin 3 (IRQ 1)    
  unsigned long endtime = starttime;
  while ((endtime - starttime) <=1000UL)   // do this loop for up to 1000mS
  {
    endtime = millis();                    //keep the arduino awake.
  }
  detachInterrupt(1);    

  //if new value is over threshold or UPD message from goose dectector camera
  if (((float)_pulsesPerSec > _pulsesPerSecAvg) || _gooseDectectorEvent) { 
    _gooseDectectorEvent = false;
    return true; 
  }

  _hist.add(_pulsesPerSec);                  // add current rate to the histogram, including zeros
  // update the rolling average but dont include the zeros, skews the result too much.
  if (_pulsesPerSec>=1) {                     
    total = total - getScanByte(index);                      // subtract the last reading:
    setScanByte(index, _pulsesPerSec);  
    total = total + getScanByte(index);                      // add the reading to the total:
    index = (index + 1) % (getNumScanReturns()/ 3);                // advance to the next position in the array:  
    if (readingCount < (getNumScanReturns()/ 3)) readingCount++;   // increment readingcount, if needed
    //_stdDeviationPulseRate = stddev(_scan,readingCount);      //calc the deviation
    _pulsesPerSecAvg = (float)total/readingCount;             // calculate the rolling average:   
    _pulsesPerSecAvg = _pulsesPerSecAvg*1.5;                  //increase value by 20%
  } else {
    zero++;                                    //keep track of the zeros
  }

  //rebuild the history frequencies when the rolling average rolls over
  //  this will keep the frequency information on a timeframe similar to the rolling average
  if (index==0) {
    _hist.clear();
    for (int i = 0; i<(getNumScanReturns()/ 3); i++) { _hist.add(getScanByte(i)); }
    for (int i=0;i<(zero);i++) {_hist.add(0);} 
    zero=0;
  } 
  
  // look at the frequency of pulse rates to see if it's raining and increase the avg to compensate
  //  the more buckets with 1% or higher of the total distribution then the higher the threshold to prevent rain triggering
  byte rateSteps[] = { 5, 10, 15, 18, 21 };
  for (byte i = 1; i < 6; i++){
    if (_hist.frequency(i) >= 0.010){
      _pulsesPerSecAvg = rateSteps[i-1];
    }
  }
  return false;
}

void onPulse() {
  _pulsesPerSec++;                        //interrupt ISR
}

