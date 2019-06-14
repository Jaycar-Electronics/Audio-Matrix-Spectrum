//audio matrix spectrum re-write 

//reading off audio A0
#define AUDIO A0 

//128 samples, log output 
#define FHT_N 128
#define LOG_OUT 1

//now include the library, from params above
//use FHT library to perform "FFT-like" conversion
#include <FHT.h>

//peak timeout before dropping 
#define PEAK_TIMEOUT 40


//matrix code
#define LATCH 2 
#define CLK 3
#define DATA 4
#define EN 5
//ROW(1) => Pin 6; change to your set up if different.
#define ROW(x) (x+5)

int peaks[16] = {0};
int peakChanges[16] = {0};

void setup(){

  
	TIMSK0 = 0;	// turn off timer0 for lower jitter
	ADCSRA = 0xe5; // set the adc to free running mode
	ADMUX = 0x40;  // use adc0
	DIDR0 = 0x01;  // turn off the digital input for adc0

  pinMode( LATCH, OUTPUT);  //set up pins
  pinMode( CLK, OUTPUT);
  pinMode( DATA, OUTPUT);
  pinMode( EN, OUTPUT);
  pinMode( ROW(1), OUTPUT);
  pinMode( ROW(2), OUTPUT);
  pinMode( ROW(3), OUTPUT);
  pinMode( ROW(4), OUTPUT);
}

void loop(){
  for(short i = 0; i < FHT_N; i++){

    while(!(ADCSRA & 0x10)) //wait for ADC to be ready
      ;

    ADCSRA = 0xF5; //restart ADC
    byte m = ADCL;  //ADC lower reg
    byte j = ADCH;  //   higher reg

    int k = (j << 8) | m; //formed into an int
    k -= 0x0200;  //center; (signed int)

    fht_input[i]  = k << 6; //as a 16b signed int
  }

  fht_window();  // window the data for better frequency response
  fht_reorder(); // reorder the data before doing the fht
  fht_run();	 // process the data in the fht
  fht_mag_log(); // take the output of the fht
  
  for(short i = 0; i < 16; i++){

    long fht = fht_log_out[2+(i*2)]; // we only care about half of the first 32 values; except for the 2 fundies

    fht = (fht < 32) ? 0 : fht-32; //zero it out if it's under 32; otherwise shift it down a notch
    fht *= 2; //amplify it 

    //size of the column is between a vlue of 0 - 16
    // so divide by 16 for how many pixels need to light up ( 16*16 = 256 )

    int str = fht / 16; 

    //check the peaks, is it larger? then slowly reduce, else re-max it;
    if (peaks[i] < str) {
      peaks[i] = str; 
      peakChanges[i] = PEAK_TIMEOUT;
    }
    else {
      if (peakChanges[i] > 0)
        peakChanges[i] -= 1;  //each pass will stay here, until peakChanges drop.
      else
        peaks[i] -= 1; 
    }
    //peaks[i] = peaks[i] > str ? --peaks[i] : str;

    int peak = (0x1 << peaks[i]); 
    int column = (0x1 << str)-1; 

    matrixData(i, column | peak );
    delay(1);
  }
}
void matrixData(short row, int bitmask){

  digitalWrite(EN, HIGH);
  digitalWrite(ROW(1),row & 1); // 0001
  digitalWrite(ROW(2),row & 2); // 0010
  digitalWrite(ROW(3),row & 4); // 0100
  digitalWrite(ROW(4),row & 8); // 1000
  digitalWrite(LATCH, LOW);

  shiftOut(DATA, CLK, LSBFIRST, ~((bitmask)      & 0xFF)); //lower portion
  shiftOut(DATA, CLK, LSBFIRST, ~((bitmask >> 8) & 0xFF)); //higher portion
  digitalWrite(LATCH, HIGH);
  digitalWrite(EN, LOW);
}