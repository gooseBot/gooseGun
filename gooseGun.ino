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
unsigned long lastTargetTime = 0;
unsigned long lastAttackTime = 0;
unsigned long disarmTimeSpan = 0;
unsigned long attackStartTime = 0;
unsigned long timeNow = 0;

EthernetUDP _Udp;                 //must be a global and declared before ethernet client, don't know why
EthernetClient _ethernetClient;   //must be a global

void setup()                    // run once, when the sketch starts
{
  initializeUPD();         //setup UDP
  controlDoor(false);      //ensure door is closed (in case of power outage on prior run)
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
  timeNow = millis();
  if (movement && _scannerOff && (((timeNow-lastAttackTime) > disarmTimeSpan) || _kidMode)) {   
    generateUUID();
    controlDoor(true);                            //open scanner door 
    controlNozzelServos(true);                    // get pan and tilt nozzel servos going
	  controlScanner(true);                          // turn on scanner

    //keep track of scanner on time, will turn it off if nothing happens for a while
    lastTargetTime = millis();       
    attackStartTime = millis();              //keep track of when the attack begain    
    getScanData(_base);                       //get one scan and save it to array    
    postDataToAgol(_base);                    // save scan to agol
  }
  // start targeting if scanner is on
  if (!_scannerOff) {
    // continue scanning if less than 2 minutes since last target
    //   stop scanning if attack has gone on more than 5 minutes (if not in kid mode)
    timeNow = millis();
    if ((((timeNow-lastTargetTime) < 120000UL) && !_disableGun && (((timeNow-attackStartTime) < 300000UL) || _kidMode))) {
        getScanData(false);                    //get a scan
        processScanData();                     //look for targets   
        if (_distance > 0) {
          lastTargetTime = millis();          
          moveServosAndShootTarget();  
          postDataToAgol(_targets);            // if shot at something then post the fact
          //postDataToAgol(_currentScan);          // for troubleshooting also post the current scan
        }    
    } else {
      //nothing has happend for 2 minutes or we have attacked more than 5 minutes
      controlScanner(false);                  // turn off the scanner
      controlDoor(false);                     // and close the door
      controlNozzelServos(false);           
      lastAttackTime = millis();              //reset last attack time 
      disarmTimeSpan = 600000UL;             //set disarm time period to 10 min                                               
      movement=getXbandRate(true);            //reset the running average used to trigger a detection      
    }
  }      
  listenForUDP();                        //is an Android connected?
}




