//float _scannerX = 1002152;         //scanner x location in wa state plane south inside house
//float _scannerY = 692505;          //scanner y location in wa state plane south inside house
const double _scannerX = 1002185;         //scanner x location in wa state plane south in yard
const double _scannerY = 692538;          //scanner y location in wa state plane south in yard
const double _scannerAngle2statePlane = 22.0 * 71 / 4068;   // scanner angle to statePlane in yard
static long _uuidNumber = 0;                   // a unique number to help track attack sessions and their data
const char* _postStr1 = "POST /25Iz4FI030a91YVh/arcgis/rest/services/";
const char* _postStr2 = "/FeatureServer/0/addFeatures HTTP/1.1";

void generateUUID(){
  _uuidNumber = TrueRandom.random();            // setup a random (for this date at least) ID for this session.
}

void postDataToAgol(byte scanType) {
  int contentLength=0;

  if (_dataOff && scanType!=_messages) return;
    
  if (_ethernetClient.connect("www.example.com", 80)) {
    //first get the content length.  The operations will fail but will calculate the length without having
    //  to build an array and consume what little memory is left!  Using example as the host 
    //  as I think arcgis.com locks out failed attempts
    contentLength = sendAgolData(scanType);

    // close down the google connection now that sending is done  
    if (_ethernetClient.connected()) {
      _ethernetClient.stop();
    }
    // now that we have the length send it for real to arcgis.com
    if (_ethernetClient.connect("services2.arcgis.com", 80)) {
      // determine which post to use based on feature service we are posting to
      switch (scanType) {
      case _base:
        sendPostHeader("Base_Scans_4"); break;
      case _currentScan:
        sendPostHeader("currentScans"); break;
      case _targets:
        sendPostHeader("Targets"); break;
      case _ignoredTargets:
        sendPostHeader("Ignored_Targets"); break;
      case _messages:
        sendPostHeader("messages"); break;
      }
      _ethernetClient.println(F("Host: services2.arcgis.com"));
      _ethernetClient.println(F("Content-Type: application/x-www-form-urlencoded"));
      _ethernetClient.println(F("Connection: close"));
      _ethernetClient.print(F("Content-Length: "));
      _ethernetClient.println(contentLength);
      _ethernetClient.println();
      sendAgolData(scanType);                     //send the data based on type
      _ethernetClient.println();
    }
    if (_ethernetClient.connected()) {
      _ethernetClient.stop();
    }
  } else {
    //initializeUPD();  // if connect fails try reintializing things.
  }
}

void sendPostHeader(char * serviceName) {
  _ethernetClient.print(_postStr1);
  _ethernetClient.print(serviceName);
  _ethernetClient.println(_postStr2);
}

int sendAgolData(int scanType) {
  int dataLength=0;
  switch (scanType) {
    case _base:
    case _currentScan:
      // both base and current scans use same send method
      dataLength=sendScanData(scanType);
      break;
    case _messages:
      dataLength=sendMessage();
      break;
    case _targets:
    case _ignoredTargets:
      //targets and ignoredtargest both fall through to same method
      dataLength=sendTargetData();  
      break;
  }
  return dataLength;
}

int sendScanData(int scanType) {   
  int dataLength=0;  
  float pointX=0.0;
  float pointY=0.0;  
  char buf[10] = "";
  char buffer[22] = "";
  
  dataLength=_ethernetClient.print(F("features=[{\"geometry\":{\"paths\":[["));
  for (int i=0; i < _numScanReturns; i++) {
    //convert polar coords to state plane relative to scanner
    if (scanType==_base) {
      getStatePlaneCoords(_baseScan[i], (i*0.5), pointX, pointY);    
    } else {
      getStatePlaneCoords(_scan[i], (i*0.5), pointX, pointY);
    }
    //print to ethernet the x,y pairs as whole strings.  cant print them piece by piece or get errors when running over 3g or 4g
    //  not sure why, wired is fine.  I'm using dtostrf etc trying to avoid String class which adds to memory needs and
    //  is supposedly less stable.
    dtostrf(pointX, 8, 1, buf);  // number, width, decimal places, buffer
    strcpy(buffer, "[");
    strcat(buffer, buf);
    strcat(buffer, ",");
    dtostrf(pointY, 8, 1, buf);  // number, width, decimal places, buffer
    strcat(buffer, buf);
    strcat(buffer, "]");
    if (i < (_numScanReturns - 1)) { strcat(buffer, ","); }
    dataLength += _ethernetClient.print(buffer);    
  }
  dataLength+=_ethernetClient.print(F("]],"));
  dataLength+=_ethernetClient.print(F("\"spatialReference\":{\"wkid\":2286}},"));
  dataLength+=_ethernetClient.print(F("\"attributes\":{\"scanType\":"));
  dataLength+=_ethernetClient.print(scanType);   
  dataLength+=_ethernetClient.print(F(",\"motionPulseRate\":"));  
  dataLength+=_ethernetClient.print(_pulsesPerSec);   
  dataLength+=_ethernetClient.print(F(",\"thresholdMotionPulseRate\":"));  
  dataLength+=_ethernetClient.print(_pulsesPerSecAvg);   
  dataLength+=_ethernetClient.print(F(",\"stdDeviationPulseRate\":"));  
  dataLength+=_ethernetClient.print(_stdDeviationPulseRate);   
  dataLength+=_ethernetClient.print(F(",\"maxRange\":"));  
  dataLength+=_ethernetClient.print(_maxRange);   
  dataLength+=_ethernetClient.print(F(",\"sessionUUID\":"));  
  dataLength+=_ethernetClient.print(_uuidNumber);   
  dataLength += _ethernetClient.print(F(",\"messages\":'"));
  for (int j = 0; j < _hist.size(); j++)
  {
    dataLength += _ethernetClient.print(_hist.bucket(j));
    dataLength += _ethernetClient.print(F("-"));
    dataLength += _ethernetClient.print(_hist.frequency(j));
    if (j < _hist.size() - 1) { dataLength += _ethernetClient.print(F(",")); };
  }
  dataLength+=_ethernetClient.print(F("'}}]"));
  return dataLength;
}

int sendTargetData() {
  int dataLength=0; 
  float pointX=0.0;
  float pointY=0.0;  
  //convert polar coords in cartesian coords relative to scanners cartesian system
  getStatePlaneCoords(_distance, _angle, pointX, pointY);
  //print the first part
  dataLength=_ethernetClient.print(F("features=[{\"geometry\":{\"paths\":[[["));  
  //print the point
  dataLength+=_ethernetClient.print(_scannerX);
  dataLength+=_ethernetClient.print(F(","));
  dataLength+=_ethernetClient.print(_scannerY);
  dataLength+=_ethernetClient.print(F("],["));
  dataLength+=_ethernetClient.print(pointX);
  dataLength+=_ethernetClient.print(F(","));
  dataLength+=_ethernetClient.print(pointY);
  dataLength+=_ethernetClient.print(F("]]],"));
  //print the other stuff
  dataLength+=_ethernetClient.print(F("\"spatialReference\":{\"wkid\":2286}},"));
  //add the attributes
  dataLength+=_ethernetClient.print(F("\"attributes\":{\"angle\":"));
  dataLength+=_ethernetClient.print(_angle);   
  dataLength+=_ethernetClient.print(F(",\"distance\":"));
  dataLength+=_ethernetClient.print(_distance);   
  dataLength+=_ethernetClient.print(F(",\"sumOfDifferences\":"));
  dataLength+=_ethernetClient.print(_totDifferences);   
  dataLength+=_ethernetClient.print(F(",\"width\":"));
  dataLength+=_ethernetClient.print(_arcLength);   
  dataLength+=_ethernetClient.print(F(",\"sessionUUID\":"));  
  dataLength+=_ethernetClient.print(_uuidNumber);     
  dataLength+=_ethernetClient.print(F("}}]"));
  return dataLength;  
}

int sendMessage() {
  //"geometry" : {
  //  "x" : -122.41247978999991,
  //    "y" : 37.770630098000083
  //}
  int dataLength=0; 
  dataLength+=_ethernetClient.print(F("features=[{\"attributes\":{\"messages\":\""));
  dataLength += _ethernetClient.print(F("UDPbuf="));
  dataLength+=_ethernetClient.print(_packetBuffer);    
  dataLength+=_ethernetClient.print(F(" PulseRate="));
  dataLength+=_ethernetClient.print(_pulsesPerSec);
  dataLength+=_ethernetClient.print(F(" PulseAvg="));
  dataLength += _ethernetClient.print(_pulsesPerSecAvg);
  dataLength += _ethernetClient.print(F(" DisarmTimeSpan="));
  dataLength += _ethernetClient.print(_disarmTimeSpan); 
  //dataLength += _ethernetClient.print(F(" testPoint="));
  //dataLength += _ethernetClient.print(_buffer);
  dataLength+=_ethernetClient.print(F("\"}}]"));
  return dataLength;  
}

void getStatePlaneCoords(int radius, float angleInDeg, float &pointX, float &pointY) {
    float angleRad = (angleInDeg*71)/4068; //efficiently convert degrees to radians
    // MUST USE LONG for all Coordinates.  Not enough percision in a FLOAT    
    // DO NOT MIX FLOATS into the calculations below other than cos/sin for radians
    pointX=(float)radius*cos(angleRad);  //get X coordinate for location in feet
    pointY=(float)radius*sin(angleRad);  //get Y coordinate for location in feet
    //translate from scanner X horizontal to fit state plane X horizontal
    float pointXg=pointX*cos(_scannerAngle2statePlane) + pointY*sin(_scannerAngle2statePlane);   
    float pointYg=pointY*cos(_scannerAngle2statePlane) - pointX*sin(_scannerAngle2statePlane);    
    //convert to state plane at scanner location
    pointX=_scannerX+(long)(pointXg+0.5);    // added 0.5 to the float prior to truncation to get a rounded value
    pointY=_scannerY+(long)(pointYg+0.5);      
}

