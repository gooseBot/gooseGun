static char _packetBuffer[UDP_TX_PACKET_MAX_SIZE];      //buffer to hold incoming packet, 
// items that might go into EEPROM config
static boolean _dataOff = false;
static boolean _kidMode = false;                     // kid mode disables most time outs.    
static boolean _disableGun = false;

char * getPacketBuffer(){
  return _packetBuffer;
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
  //myDelay(200);
  _Udp.endPacket(); 
  //myDelay(200);
}

void listenForUDP () {  
  int packetSize = _Udp.parsePacket();       // if there's data available, read a packet
  char* commands[] = { "don", "dof", "kon", "kof", "gon", "gof" };

  if(packetSize)
  {  
    memset(_packetBuffer,0,sizeof(_packetBuffer));        //clear the buffer
    _Udp.read(_packetBuffer,UDP_TX_PACKET_MAX_SIZE);      // read the packet into packetBufffer
    //loop the commands looking for a match to the packet
    for (int i=0;i<6;i++){
      if (strcmp(_packetBuffer, (char*)commands[i]) == 0)
      {
        switch (i) 
        {
          case 0: 
            _dataOff = false; break;
          case 1: 
            _dataOff = true; break;
          case 2: 
            _kidMode = true; break;
          case 3: 
            _kidMode = false; break;
          case 4: 
            _disableGun = false; break;
          case 5: 
            _disableGun = true; break;
          default: break; 
        }
        sendUDP(commands[i], 3);
        postDataToAgol(_messages);  //record info about the UDP command
      }
    }
  }
}

