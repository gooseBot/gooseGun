#include <Servo.h> 
#include <SPI.h>                 // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include "TrueRandom.h"
#include <Average.h>
#include <histogram.h>

// all other globals   
EthernetUDP _Udp;                 //must be a global and declared before ethernet client, don't know why
EthernetClient _ethernetClient;   //must be a global
double _xBandBuckets[] = { 0, 1, 2, 3, 5, 10, 20, 1000 };
Histogram _hist(8, _xBandBuckets); //motion sensor data

void setup()               
{
  initializeUPD();       
  initializeTargeting();
  controlScanner(false);   //ensure scanner is also off
}

void loop()                          
{ 
  if (detectMovement(false) && !getDisableGun()) {
    manageAttack();
  }
  listenForUDP();                           //is an Android connected?
}

void manageAttack() {
  unsigned long lastTargetTime = 0;
  static unsigned long lastAttackTime = 0;
  static unsigned long disarmTimeSpan = 0;
  unsigned long attackStartTime = 0;
  // if scanner off, and we have motion, and more then 10min since last attack, then turn on scanner
  if (((millis() - lastAttackTime) > disarmTimeSpan) || getKidMode()) {
    controlScanner(true);                     // turn on scanner
    //keep track of scanner on time, will turn it off if nothing happens for a while
    lastTargetTime = millis();               //keep track of the last shot time
    attackStartTime = millis();              //keep track of when the attack begain    
    captureBaseScan();                       //get one scan and save it to array    
    if (!getDataOff()) postBaseScanToAgol();                    // save scan to agol
    // continue scanning if less than 2 minutes since last target
    //   stop scanning if attack has gone on more than 5 minutes (if not in kid mode)
    //   stop scanning if gun disabled
    while (((millis() - lastTargetTime) < 120000UL) && !getDisableGun() && (((millis() - attackStartTime) < 300000UL) || getKidMode())) {
      getScanData(false);                    //get a scan
      processScanData();                     //look for targets   
      if (getDistance() > 0) {
        lastTargetTime = millis();
        moveServosAndShootTarget();
        if (!getDataOff()) postTargetToAgol();     // if shot at something then post the fact
      }
      listenForUDP();                           //is an Android connected?
    }
    // attack is done
    controlScanner(false);                 // turn off the scanner 
    detectMovement(true);         //reset the running average used to trigger a detection 
    if (getDisableGun()) {
      disarmTimeSpan = 0UL;  //set disarm time period to 0 min if gun was manually disabled, avoids disarm when renabled             
    } else {
      disarmTimeSpan = 600000UL;         //set disarm time period to 10 min if not manually disabled  
    }
    lastAttackTime = millis();              //reset last attack time 
    postMessageToAgol();
  }
}


