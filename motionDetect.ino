double xBandBuckets[] = { 0, 1, 2, 3, 5, 10, 20, 1000 };
Histogram _hist(8, xBandBuckets);

Histogram getHistogram(){
	return _hist;
}

boolean getXbandRate(boolean resetRollingAvgNumbers)
{
  _pulsesPerSec=0;                       //reset the count
  static int index = 0;                  // the index of the current reading
  static int total = 0;                  // the running total
  static int readingCount = 0;
  static int zero = 0;
  
  // reset the numbers used to compute rolling average, needed after an attack is done
  //  Im reusing the scan array to compute the rolling average
  //  this works since there is an enforced 10min rest time between attacks
  if (resetRollingAvgNumbers) {
    index=0;
    total=0;
    readingCount=0;
    for (int i = 0; i < _numScanReturns; i++) {_scan[i] = 0;}
    _hist.clear();                  //also clear the histogram data
    zero=0;    
    return false;    
  }
  
  if (_disableGun) {return false;}  
    
  // get num of pulses for one second from the xband
  unsigned long starttime = millis();      //going to count for a fixed time
  attachInterrupt(1, onPulse, RISING);     //hooked xband to pin 3 (IRQ 1)    
  unsigned long endtime = starttime;
  while ((endtime - starttime) <=1000UL)   // do this loop for up to 1000mS
  {
    endtime = millis();                    //keep the arduino awake.
  }
  detachInterrupt(1);    

  //if new value is over threshold then return that, no other work to do.
  if ((float)_pulsesPerSec > _pulsesPerSecAvg) {return true;}

  _hist.add(_pulsesPerSec);                  // add current rate to the histogram, including zeros
  // update the rolling average but dont include the zeros, skews the result too much.
  if (_pulsesPerSec>=1) {                     
    total= total - _scan[index];               // subtract the last reading:
    _scan[index] = _pulsesPerSec;   
    total= total + _scan[index];               // add the reading to the total:
    index = (index + 1) % (_numScanReturns/3);     // advance to the next position in the array:  
    if (readingCount < (_numScanReturns/3)) readingCount++;    // increment readingcount, if needed
    _stdDeviationPulseRate = stddev(_scan,readingCount);          //calc the deviation
    _pulsesPerSecAvg = (float)total/readingCount;              // calculate the rolling average:   
    _pulsesPerSecAvg = _pulsesPerSecAvg*1.5;           //increase value by 20%
  } else {
    zero++;                                    //keep track of the zeros
  }

  //rebuild the history frequencies when the rolling average rolls over
  //  this will keep the frequency information on a timeframe similar to the rolling average
  if (index==0) {
    _hist.clear();
    for (int i=0;i<(_numScanReturns/3);i++) {_hist.add(_scan[i]);}   
    for (int i=0;i<(zero);i++) {_hist.add(0);} 
    zero=0;
  } 
  
  // look at the frequency of pulse rates to see if it's raining and increase the avg to compensate
  if (_hist.frequency(1)>=0.010) {                           // if the histogram "ones" geater than 1% then its raining.
    _pulsesPerSecAvg = 5;                                      
    if (_hist.frequency(2)>=0.010) {                         // if the histogram "twos" greater than 1% then its raining hard
      _pulsesPerSecAvg = 10;                                  
      if (_hist.frequency(3)>=0.010) {                        // if the histogram "threes" greater than 1% then its raining very hard
        _pulsesPerSecAvg = 15;                                
        if (_hist.frequency(4)>=0.010) {                      // if the histogram "4 and 5s" greater than 1% then its raining really hard
          _pulsesPerSecAvg = 18;                              
          if (_hist.frequency(5)>=0.010) {                      // if the histogram "5-10" greater than 1% then its raining really hard
            _pulsesPerSecAvg = 21;     
			if (_hist.frequency(6) >= 0.010) {                      // if the histogram "10-20" greater than 1% then its raining really hard
				_pulsesPerSecAvg = 30;
			}
          }
        }
      }
    }
  } 
  return false;
}

void onPulse() {
  _pulsesPerSec++;                        //interrupt ISR
}

