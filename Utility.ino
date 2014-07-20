void myDelay(int mseconds) {
  // this delay keeps the arduino working, built in delay stops most activity
  //   this type of delay seems to be needed to make the servo libarary work
  //   not sure why.
  unsigned long starttime = millis();   //going to count for a fixed time
  unsigned long endtime = starttime;
  while ((endtime - starttime) <= mseconds) // do the loop
  {
    endtime = millis();                  //keep the arduino awake.
  }  
}

