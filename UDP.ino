
void initializeUPD() {
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0x70, 0x30 };
  IPAddress ip(192, 168, 1, 178);            // ip address from my router not using dhcp
  Ethernet.begin(mac, ip);
  myDelay(2000);
  _Udp.begin(8888);
}

void sendUDP(char *response, int responseSize) {
  _Udp.beginPacket(_Udp.remoteIP(), _Udp.remotePort());
  _Udp.write(response, responseSize);         
  _Udp.endPacket(); 
  myDelay(100); 
}

void listenForUDP () {  
  int packetSize = _Udp.parsePacket();       // if there's data available, read a packet
  char* commands[] = { "don", "dof", "kon", "kof", "gon", "gof" };

  if(packetSize)
  {  
    memset(_packetBuffer,0,sizeof(_packetBuffer));        //clear the buffer
    _Udp.read(_packetBuffer,UDP_TX_PACKET_MAX_SIZE);      // read the packet into packetBufffer
    postDataToAgol(_messages);
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
      }
    }
  }
}

