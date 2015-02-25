static char _packetBuffer[UDP_TX_PACKET_MAX_SIZE];      //buffer to hold incoming packet, 
static boolean _dataOff = false;
static boolean _kidMode = false;                     // kid mode disables most time outs.    
static boolean _disableGun = false;
static boolean _manualMode = false;     
static unsigned long _lastTrgCmdReceivedTime = 0UL;
char* _commands[] = { "don", "dof", "kon", "kof", "gon", "gof", "sts", "mon", "mof", "von", "vof", "trg" };

char * getPacketBuffer(){
  return _packetBuffer;
}

unsigned long getLastTrgCmdReceivedTime(){
  return _lastTrgCmdReceivedTime;
}

boolean getDataOff(){
  return _dataOff;
}

boolean getKidMode(){
  return _kidMode;
}

boolean getDisableGun(){
  return _disableGun;
}

boolean getManualMode(){
  return _manualMode;
}

boolean setManualMode(boolean mode){
  _manualMode = mode;
}

void initializeUPD() {
  pinMode(4, OUTPUT);      //ensure sd card is off or an ethernet issue may occur
  digitalWrite(4, HIGH);   // SD Card not active
  byte mac[] = { 0xDE, 0xA2, 0xDA, 0x41, 0x70, 0x37 };
  IPAddress ip(192, 168, 1, 178);            // ip address from my router not using dhcp
  Ethernet.begin(mac, ip);
  myDelay(2000);
  _Udp.begin(8888);
}

void sendUDP(char *response, int responseSize) {
  _Udp.beginPacket(_Udp.remoteIP(), _Udp.remotePort());
  _Udp.write(response, responseSize);  
  _Udp.endPacket(); 
}

void sendUDPcamTrigger(char *response, int responseSize) {
  IPAddress ip(255, 255, 255, 255);            // broadcast local network
  _Udp.beginPacket(ip, 2004);
  _Udp.write(response, responseSize);
  _Udp.endPacket();
}

void listenForUDP () {  
  while (_Udp.parsePacket()) {
    memset(_packetBuffer, 0, sizeof(_packetBuffer));        //clear the buffer
    _Udp.read(_packetBuffer, UDP_TX_PACKET_MAX_SIZE);      // read the packet into packetBufffer
    //loop the commands looking for a match to the packet 
    for (int i = 0; i < 12; i++) {
      if (strncmp(_packetBuffer, (char*)_commands[i], 3) == 0) {
        switch (i) {
          case 0:    //don
            _dataOff = false; break;
          case 1:    //dof
            _dataOff = true; break;
          case 2:    //kon
            _kidMode = true; break;
          case 3:    //kof
            _kidMode = false; break;
          case 4:    //gon
            _disableGun = false; break;
          case 5:    //gof
            _disableGun = true; 
            if (_manualMode == true) {                       //turn off manual mode when gun is disabled
              _manualMode = false;
            }        
            break;
          case 7:    //mon      
            if (_disableGun == false) {
              _manualMode = true;
              _lastTrgCmdReceivedTime = millis();            //init time out for man mode, dont want to burn out servos              
            }
            break;
          case 8:    //mof             
            _manualMode = false; 
            sendUDPcamTrigger("1234567890", 11);
            break;
          case 9:    //von
            if (_manualMode == true && _disableGun == false) openValve();
            break;
          case 10:   //vof
            if (_manualMode == true && _disableGun == false) closeValve();
            break;
          case 11:   //trg packet contains target coordinates when in manual mode  trgAaaa.aDd (Aaa.a=angle, Dd=distance)
            if (_manualMode == true && _disableGun == false) {      //unpack the coordinates if packet is correct size
              if (strlen(_packetBuffer) == 10) {
                char angle[6];
                char distance[3];
                memcpy(angle, &_packetBuffer[3], 5);
                memcpy(distance, &_packetBuffer[8], 2);
                setAngle(atof(angle));
                setDistance(atoi(distance));
                moveServosAndShootTarget();
                _lastTrgCmdReceivedTime = millis();
              }
            }
            break;
          default: break;
        }
        // reply with what was sent and with status for sts command.  dont reply for trg commands would slow down action
        if (i != 11){
          prepareStatusResponse();
          sendUDP(_packetBuffer, strlen(_packetBuffer));
        }
        //commented out to speed up manual mode, TODO: clean up
        //postDataToAgol(_messages);  //record info about the UDP command
      }
    }
  }
}

void prepareStatusResponse(){
  memset(_packetBuffer, 0, sizeof(_packetBuffer));        //clear the buffer
  strcpy(_packetBuffer, "sts");
  if (!_dataOff)                                          //fill the buffer with gun status
    strcat(_packetBuffer, _commands[0]);
  if (_kidMode)
    strcat(_packetBuffer, _commands[2]);
  if (!_disableGun)
    strcat(_packetBuffer, _commands[4]);
  if (_manualMode)
    strcat(_packetBuffer, _commands[7]);
}

