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

// items that might go into EEPROM config
boolean _dataOff=false;
boolean _kidMode=false;                     // kid mode disables most time outs.    
boolean _disableGun=false;

// all other globals
char _packetBuffer[UDP_TX_PACKET_MAX_SIZE];      //buffer to hold incoming packet,    
const int _numScanReturns = 360;
byte _baseScan[_numScanReturns];
byte _scan[_numScanReturns];
float _maxRange = 0;
float _angle=0;
byte _distance=0;
long _totDifferences=0;
float _arcLength=0;
                 
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
unsigned long _timeNow = 0;

EthernetUDP _Udp;                 //must be a global and declared before ethernet client, don't know why
EthernetClient _ethernetClient;   //must be a global
double _xBandBuckets[] = { 0, 1, 2, 3, 5, 10, 20, 1000 };
Histogram _hist(8, _xBandBuckets); //motion sensor data

void setup()                    // run once, when the sketch starts
{
  initializeUPD();         //setup UDP
  controlScanner(false);   //ensure scanner is also off
  closeValve();
}

void loop()                          
{  
  //get motion rate if scanner not already on
  boolean movement=true;                
  if (_scannerOff) { 
    movement=getXbandRate(false);  
  }  
  // if scanner off, and we have motion, and more then 10min since last attack, then turn on scanner
  _timeNow = millis();
  if ((movement && _scannerOff && (((_timeNow-_lastAttackTime) > _disarmTimeSpan) || _kidMode))) {   
    generateUUID(); 
	  controlScanner(true);                     // turn on scanner
    //keep track of scanner on time, will turn it off if nothing happens for a while
    _lastTargetTime = millis();               //keep track of the last shot time
    _attackStartTime = millis();              //keep track of when the attack begain    
    getScanData(_base);                       //get one scan and save it to array    
    postDataToAgol(_base);                    // save scan to agol
  }
  // start targeting if scanner is on
  if (!_scannerOff) {
    // continue scanning if less than 2 minutes since last target
    //   stop scanning if attack has gone on more than 5 minutes (if not in kid mode)
    //   stop scanning if gun disabled
    _timeNow = millis();
    if (((_timeNow-_lastTargetTime) < 120000UL) && !_disableGun && (((_timeNow-_attackStartTime) < 300000UL) || _kidMode)) {
      getScanData(false);                    //get a scan
      processScanData();                     //look for targets   
      if (_distance > 0) {
        _lastTargetTime = millis();          
        moveServosAndShootTarget();  
        postDataToAgol(_targets);            // if shot at something then post the fact
        //postDataToAgol(_currentScan);      // for troubleshooting also post the current scan
      }    
    } else {
      //nothing has happend for 2 minutes or we have attacked more than 5 minutes or gun was disabled
      controlScanner(false);                 // turn off the scanner 
      movement = getXbandRate(true);         //reset the running average used to trigger a detection 
      if (_disableGun){
        _disarmTimeSpan = 0UL;  //set disarm time period to 0 min if gun was manually disabled, avoids disarm when renabled             
      } else {
        _disarmTimeSpan = 600000UL;         //set disarm time period to 10 min if not manually disabled  
      }
      _lastAttackTime = millis();              //reset last attack time 
      postDataToAgol(_messages);
    }
  }      
  listenForUDP();                           //is an Android connected?
}




