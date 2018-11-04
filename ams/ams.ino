//Duinotech Audio Matrix Spectrum
//audio sample > FFT > display as spectrum
//need to adjust pot on mic module until null value is around midpoint (about when LED flickers on and off)
//uses timer1 to implement audio sampling rate and scanning
//FFT code from http://forum.arduino.cc/index.php?topic=38153.0
//modified to work with newer versions of Arduino
//1500Hz sample rate gives 0-750Hz (not very precise though)
#include "fix_fft.h"
#define AUDIO A7
char im[256];       //only needs 32 bytes, but we've got heaps of RAM
char data[256];     //only needs 32 bytes, but we've got heaps of RAM
#define MATRIXLAT 2
#define MATRIXCLK 3
#define MATRIXDI 4
#define MATRIXG 5
#define MATRIXA 6
#define MATRIXB 7
#define MATRIXC 8
#define MATRIXD 9
#define MATRIXFREQ 1500L
unsigned int matrixdata[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};    //for display data
byte level[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};                 //for current level
byte peak[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};                  //for peak hold
byte matrixscan=0;                                                //column being scanned
byte i=31;                                                        //sample number

void setup() {
  matrixsetup();                                      //set up matrix control pins and timer
  Serial.begin(9600); //we've found this to be a quick and easy way to set up the
                      //correct interrupt timing functions, almost purely by accident.
}

void loop() {
  if(!i){                                                             //wait until all data is sampled
    fix_fft(data,im,5,0);                                             //do the FFT
    for(int j=0;j<128;j++){
      data[j] = sqrt(data[j] * data[j] + im[j] * im[j]);              //get magnitude of FFT
    }
    for(int j=0;j<16;j++){
      level[j]=data[j+1];                                             //put in array, except DC
      if(level[j]>15){level[j]=15;}                                   //avoid clipping
      if(peak[j]){peak[j]--;}                                         //fade peak level
      if(level[j]>peak[j]){peak[j]=level[j];}                         //check peaks
      matrixdata[j]=(65536L-(32768>>(level[j])))|(32768>>peak[j]);    //convert to bar graph
    }                                                                 //display is constantly updated, so that's all we need to do.
  i=31;                                                               //start again
  }
}

void matrixsetup(){
  pinMode( MATRIXLAT, OUTPUT);  //set up pins
  pinMode( MATRIXCLK, OUTPUT);
  pinMode( MATRIXDI, OUTPUT);
  pinMode( MATRIXG, OUTPUT);
  pinMode( MATRIXA, OUTPUT);
  pinMode( MATRIXB, OUTPUT);
  pinMode( MATRIXC, OUTPUT);
  pinMode( MATRIXD, OUTPUT);
  // Timer 1 set up as a FREQ Hz sample interrupt, only common thing this affects is servo library
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10);
  TCNT1 = 0;
  OCR1A = F_CPU / MATRIXFREQ;
  TIMSK1 = _BV(OCIE1A);
}

ISR(TIMER1_COMPA_vect) {    //gets triggered FREQ times/second
  int a;
  a=analogRead(AUDIO)-512;  //read a sample
  if(i){                    //save sample if we aren't busy processing
    data[i] = a;
    im[i] = 0;
    i--;
  }
  matrixscan++;                         //next column on matrix
  digitalWrite(MATRIXG,HIGH);           //blank for data shuffle
  digitalWrite(MATRIXA, (matrixscan&1));//set row
  digitalWrite(MATRIXB, (matrixscan&2));
  digitalWrite(MATRIXC, (matrixscan&4));
  digitalWrite(MATRIXD, (matrixscan&8));
  digitalWrite(MATRIXLAT, LOW);
  shiftOut(MATRIXDI, MATRIXCLK, LSBFIRST, 255-(matrixdata[matrixscan&15]&255));    //output row data
  shiftOut(MATRIXDI, MATRIXCLK, LSBFIRST, 255-((matrixdata[matrixscan&15]>>8)&255));
  digitalWrite(MATRIXLAT, HIGH); //latch data
  digitalWrite(MATRIXG,LOW); //unblank
}
