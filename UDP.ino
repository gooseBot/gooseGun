
void initializeUPD() {
  byte _mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0x70, 0x30 };
  IPAddress _ip(192, 168, 1, 178);            // ip address from my router not using dhcp
  Ethernet.begin(_mac, _ip);
  delay(1000);
  _Udp.begin(8888);
}

void sendUDP(byte *response, int responseSize) {
  _Udp.beginPacket(_Udp.remoteIP(), _Udp.remotePort());
  _Udp.write(response, responseSize);         
  _Udp.endPacket(); 
  myDelay(100); 
}

void listenForUDP () {  
  int packetSize = _Udp.parsePacket();       // if there's data available, read a packet
  byte dataOff[] = {"dof"};
  byte dataOn[] = {"don"};
  byte kidOn[] = {"kon"};
  byte kidOff[] = {"kof"};
  byte gunOn[] = {"gon"};
  byte gunOff[] = {"gof"};
  if(packetSize)
  {
    // read the packet into packetBufffer
    memset(_packetBuffer,0,sizeof(_packetBuffer));       //clear the buffer
    _Udp.read(_packetBuffer,UDP_TX_PACKET_MAX_SIZE);
    postDataToAgol(_messages);
    
    if (strcmp(_packetBuffer,(char*)dataOff)==0)
    {
      _dataOff=true;
      sendUDP(dataOff,3);
    }
    if (strcmp(_packetBuffer,(char*)dataOn)==0)
    {
      _dataOff=false;
      sendUDP(dataOn,3);
    }
    if (strcmp(_packetBuffer,(char*)kidOn)==0)
    {
      _kidMode=true;
      sendUDP(kidOn,3);
    }
    if (strcmp(_packetBuffer,(char*)kidOff)==0)
    {
      _kidMode=false;
      sendUDP(kidOff,3);
    }
    if (strcmp(_packetBuffer,(char*)gunOn)==0)
    {
      _disableGun=false;
      sendUDP(gunOn,3);
    }
    if (strcmp(_packetBuffer,(char*)gunOff)==0)
    {
      _disableGun=true;
      sendUDP(gunOff,3);
    }
  }
}

