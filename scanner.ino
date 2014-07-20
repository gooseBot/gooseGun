void getScanData (boolean getBaseScan) {
  
  int c=0;   //count for finding measures header 
  int i=0;   //count for number of bytes in a scan
  int j=0;   //count to make sure measures header is read consecutively
  int k=0;   //count for storing measures as bytes in feet
  int offset=0;
  int myByte=0;
  byte byte1=0;
  byte distFeet=0;
  boolean readMeasureBytes=false;  
  byte measuresHeader[] = {0x2,0x80,0xD6,0x2,0xB0,0x69,0x1};
  boolean done = false;
  
  // Used PLS/LSI software to set permanent baud.  See Help topic in software on how to do this via SICK Diagnosis
  //   If decide to use temporary method must be in setup mode first (sends password which is SICK_PLS)  
  Serial.begin(38400,SERIAL_8E1);   // must start and stop Serial before after scans or weird stuff happens
  
  while (!done) { 
    if (Serial.available() > 0) {
      myByte = Serial.read();   
      j++;                              //increment consecutive byte counter
      
      if (readMeasureBytes) {           //are we reading measures or looking for a header?
        if (i < _bytesPerScan) {         //get all the bytes for a single scan
          if (((i + 1) % 2 == 0)) {
            // added 0.5 to the float prior to truncation to get a rounded value
            distFeet = (float((byte1 + myByte*256))/30.0)+0.5;
            if (getBaseScan) {
              _baseScan[k] = distFeet;
            } else {
              _scan[k] = distFeet;
            }             
            k++;
          } else {
            byte1 = myByte;
          }
          i++;                          //increment byte count
        } else {
          readMeasureBytes = false;     //done with scan
          done = true;                  //only need one scan
        }
      } else {
        //looking for a header
        if (myByte == measuresHeader[c]){
          c++;                          //get ready to test the next rec byte against the next header byte
          if ((c > 6) && (j == c))  {   // a header must match all 7 bytes and be consecutive bytes
            //got a full header, remaining 722 bytes are the measures
            readMeasureBytes = true;    //header is good if it was offset 10 from the prior
            offset = 0;                 //handy to know how many bytes read until found a header
            i = 0;
            c = 0;                      //reset header index counter
            j = 0;                      //reset consecutive byte counter              
          }
        } else { 
          c = 0;                        //not a header yet, start over
          j = 0;                        //and make sure bytes are consecutive
        }  
        offset++;                       //increment offset from header counter
      }
    }
  }   
  Serial.end();
}

void controlDoor(boolean doorOpen) {
  _doorServo.attach(_bucketDoorPin,950,2025);  
  if (doorOpen) {
    _doorServo.write(175);
  } else {    
    _doorServo.write(10);
  }  
  myDelay(1000);           //this delay is needed or door won't close all the way
  _doorServo.detach();
}

