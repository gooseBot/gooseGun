#include <Servo.h> 
#include <SPI.h>                 // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include "TrueRandom.h"
#include <Average.h>
#include <histogram.h>
//#include <SoftwareSerial.h>
//#include <serLCD.h>
//#include <MemoryFree.h>
//#include <SdFat.h>
//serLCD lcd(7);
// commit to GH test

// items that might go into EEPROM config
boolean _dataOff=false;

float _scannerAngle2statePlane = 22.0*71/4068;   // scanner angle to statePlane in yard
float _nozzelVelocity = 30.0;        //feet/sec - Seems accurate for yard
//float _nozzelVelocity = 20.0;        //feet/sec - indoor testing
float _nozzelAboveGroundDistance = -3;  
// kid mode disables most time outs.  the 5 minute attack timeout and the 10 min rest are disabled
boolean _kidMode=false;                
boolean _disableGun=false;

// all other globals
char _packetBuffer[UDP_TX_PACKET_MAX_SIZE];      //buffer to hold incoming packet,    
long _uuidNumber=0;                   // a unique number to help track attack sessions and their data
const int _bytesPerScan = 720;
const int _numScanReturns = 360;
byte _baseScan[_numScanReturns];
byte _scan[_numScanReturns];
byte _tiltServoNeutralAngle = 110;
float _gravity = 32.0;               //feet/sec/sec
float _maxRange = 0;
float _angle=0;
byte _distance=0;
long _totDifferences=0;
float _arcLength=0;
byte _scannerMosfetPin = 8;                 
byte _valveMosfetPin = 4;
byte _bucketDoorPin = 6;
byte _bottomServoPin = 9;
byte _topServoPin = 5;  
Servo _bottomServo;
Servo _topServo;
Servo _doorServo;
const byte _base=1;
const byte _currentScan=2;
const byte _targets=3;
const byte _ignoredTargets=4;
const byte _messages=5;
volatile int _pulsesPerSec=0;
float _stdDeviationPulseRate=0;
float _pulsesPerSecAvg=2;             //start off with something or will get an instant detection.
boolean _scannerOff = true;
unsigned long _lastTargetTime = 0;
unsigned long _lastAttackTime = 0;
unsigned long _disarmTimeSpan = 0;
unsigned long _attackStartTime = 0;
double xBandBuckets[] = {0,1,2,3,5,10,20,1000 };
Histogram _hist(8, xBandBuckets);
EthernetUDP _Udp;
EthernetClient _ethernetClient;

void setup()                    // run once, when the sketch starts
{
  byte _mac[] = {0x90,0xA2,0xDA,0x0E,0x70,0x30};
  IPAddress _ip(192,168,1,178);            // ip address from my router not using dhcp
  Ethernet.begin(_mac,_ip);
  delay(1000); 
  _Udp.begin(8888);  
  
  // this code moves door servo to 90 degrees initially seems to be needed.  Otherwise open/close
  //    code doesnt seem to work.  This servo is a waterproof servo and was modified by 
  //    servo city to work at 180 degrees, maybe this is part of the reason?
  _doorServo.attach(_bucketDoorPin,950,2025);  
  _doorServo.write(90);
  myDelay(500);
  controlDoor(false);  //close door
  
  // ensure valve and scanner are off
  pinMode(_scannerMosfetPin, OUTPUT);      
  pinMode(_valveMosfetPin, OUTPUT);      
  digitalWrite(_scannerMosfetPin, LOW);
  digitalWrite(_valveMosfetPin, LOW);  
  
  //max range for current water velocity
  setMaxRange();
}

void loop()                          
{  
  //get motion rate if scanner not already on
  boolean movement=true;                
  if (_scannerOff) {
    movement=getXbandRate(false);  
  }  
  
  // if scanner off, and we have motion, and more then 10min since last attack, then turn on scanner
  unsigned long timeNow = millis();
  if (movement && _scannerOff && (((timeNow-_lastAttackTime)>_disarmTimeSpan) || _kidMode)) {   
    _uuidNumber = TrueRandom.random();            // setup a random (for this date at least) ID for this session.      
    //open door
    controlDoor(true);
    //enable nozzle servos
    _bottomServo.attach(_bottomServoPin,544,2400);  
    _topServo.attach(_topServoPin,1050,2400);     
    _topServo.write(_tiltServoNeutralAngle);       // put tilt servo at "level" position 
    myDelay(100);                                  // give tilt servo time to move
    digitalWrite(_scannerMosfetPin, HIGH);    // turn on the scanner    
    delay(9000);                              // wait for scanner to go green
    // Used PLS/LSI software to set permanent baud.  See Help topic in software on how to do this via SICK Diagnosis
    //   If decide to use temporary method must be in setup mode first (sends password which is SICK_PLS)  
    byte startMeasures[] = {0x2,0x0,0x2,0x0,0x20,0x24,0x34,0x8};
    Serial.begin(38400,SERIAL_8E1);   
    Serial.write(startMeasures,sizeof(startMeasures));  //request all scan data continuously  
    Serial.end();
    //keep track of scanner on time, will turn it off if nothing happens for a while
    _lastTargetTime = millis();       
    _attackStartTime = millis();              //keep track of when the attack begain    
    getScanData(_base);                       //get one scan and save it to array    
    postDataToAgol(_base);                    // save scan to agol
    _scannerOff = false;
  }
  // start targeting if scanner is on
  if (!_scannerOff) {
    // continue scanning if less than 2 minutes since last target
    //   stop scanning if attack has gone on more than 5 minutes (if not in kid mode)
    unsigned long timeNow = millis();
    if ((((timeNow-_lastTargetTime) < 120000UL) && !_disableGun && (((timeNow-_attackStartTime) < 300000UL) || _kidMode))) {
        getScanData(false);                    //get a scan
        processScanData();                     //look for targets   
        if (_distance > 0) {
          _lastTargetTime = millis();          
          moveServosAndShootTarget();  
          postDataToAgol(_targets);          // if shot at something then post the fact
          //postDataToAgol(_currentScan);          // for troubleshooting also post the current scan
        }    
    } else {
      //nothing has happend for 2 minutes or we have attacked more than 5 minutes
      Serial.end();
      digitalWrite(_scannerMosfetPin, LOW);   // turn off the scanner
      controlDoor(false);                     // and close the door
      _topServo.detach();  
      _bottomServo.detach();      
      _scannerOff = true;      
      _lastAttackTime = millis();    //reset last attack time 
      _disarmTimeSpan = 600000UL;    //set disarm time period to 10 min              
      //wait for servos to finish relaxing before starting up detection, otherwise motion gets
      //  caught and rolling average is in accurate.
      delay(1000);                                                 
      movement=getXbandRate(true);            //reset the running average used to trigger a detection      
    }
  }      
  listenForUDP();                        //is an Android connected?
}




