/* 
	Editor: http://www.visualmicro.com
	        visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
	        the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
	        all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
	        note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Arduino Uno, Platform=avr, Package=arduino
*/

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define __AVR_ATmega328p__
#define __AVR_ATmega328P__
#define ARDUINO 106
#define ARDUINO_MAIN
#define __AVR__
#define __avr__
#define F_CPU 16000000L
#define __cplusplus
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__

#define __builtin_va_list
#define __builtin_va_start
#define __builtin_va_end
#define __DOXYGEN__
#define __attribute__(x)
#define NOINLINE __attribute__((noinline))
#define prog_void
#define PGM_VOID_P int
            
typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}

//
//
void postDataToAgol(byte scanType);
int sendAgolData(int scanType);
int sendScanData(int scanType);
int sendTargetData();
int sendMessage();
void getStatePlaneCoords(int radius, float angleInDeg, float &pointX, float &pointY);
void initializeUPD();
void sendUDP(byte *response, int responseSize);
void listenForUDP ();
void myDelay(int mseconds);
Histogram getHist();
boolean getXbandRate(boolean resetRollingAvgNumbers);
void onPulse();
void getScanData (boolean getBaseScan);
void controlDoor(boolean doorOpen);
void controlScanner(boolean scannerOn);
void controlNozzelServos(boolean turnOn);
void initializeTargeting();
void processScanData();
void moveServosAndShootTarget();

#include "C:\Program Files (x86)\Arduino\hardware\arduino\cores\arduino\arduino.h"
#include "C:\Program Files (x86)\Arduino\hardware\arduino\variants\standard\pins_arduino.h" 
#include "C:\Users\Eric\Documents\Arduino\gooseGun\gooseGun.ino"
#include "C:\Users\Eric\Documents\Arduino\gooseGun\Agol.ino"
#include "C:\Users\Eric\Documents\Arduino\gooseGun\UDP.ino"
#include "C:\Users\Eric\Documents\Arduino\gooseGun\Utility.ino"
#include "C:\Users\Eric\Documents\Arduino\gooseGun\motionDetect.ino"
#include "C:\Users\Eric\Documents\Arduino\gooseGun\scanner.ino"
#include "C:\Users\Eric\Documents\Arduino\gooseGun\targeting.ino"
#endif
