// Peggy2 (Serial Mod)
// Word Clock

#include <Peggy2Serial.h>
#include <Time.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include "avr/pgmspace.h"
#include "Peggy2_memory_map.h"

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

Peggy2 firstframe;     // Make a first frame buffer object, called firstframe
byte x,y;
uint32_t rows[25];               // 2X display (16 columns per display) for swapping back and forth
int addr = 0;

// language constants
uint8_t n_minute_led;               // number of minute hack leds
uint8_t n_minute_state = 30;             // number of minute hack states to cycle through
uint8_t n_byte_per_display = 4;         // number of bytes used for each 5 minunte time incriment

// globals
int display_idx;                  // display index (0 or 1)
uint8_t last_min_hack_inc = 0;    // last mininte hack incriment (to know when it has changed)
uint8_t last_time_inc = 289;        // last time incriment (to know when it has changed)
uint8_t last_pwm = 0;

void setup()                    // run once, when the sketch starts
{
  Serial.begin(115200);
  Serial.println("Peggy2 WordClock 1.0!");
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");
  firstframe.HardwareInit();   // Call this once to init the hardware. 
                                    // (Only needed once, even if you've got lots of frames.)
  //asm("cli");  // Optional: Disable global interrupts for a smoother display
  firstframe.Clear(); 
  y = 0;   
  while (y < 25) {
    firstframe.Clear();
    x = 0;
    while (x < 25) {
      firstframe.SetPoint(x, y);
      x++;
    }
    firstframe.RefreshAll(10); //Draw frame buffer
    y++;
  }
}

void loop(){ 
  uint8_t word[3];                // will store start_x, start_y, length of word
  uint16_t time_inc;
  uint8_t minute_hack_inc;
  
  if (Serial.available()) {
    processSyncMessage();
  }
  if (timeStatus()== timeNotSet) {
     firstframe.SetPoint(23, 24);
     time_t test=0;
     EEPROM_readAnything(addr,test);
     setTime(test);
  }
  
  time_t spm = now() % 86400; // seconds past midnight
  time_inc = spm / 300;  // 5-minute time increment are we in
  // which mininute hack incriment are we on?
  minute_hack_inc = (spm % 300) / (300. / float(n_minute_state));
  
  // if minute hack or time has changed, update the display
  if(minute_hack_inc != last_min_hack_inc || time_inc != last_time_inc){
    EEPROM_writeAnything(addr, now());
    firstframe.Clear(); // clear the display
    for(uint8_t i = 0; i < 25; i++){ // clear the display
      rows[i] = 0;
    }
    // read display for next time incement
    getdisplay(time_inc, rows);
    
    // read minutes hack for next time incement
    //rows[24] = minutes_hack(minute_hack_inc);
    
    // display
    uint32_t column = 0;
    for(uint8_t a = 0; a < 25; a++){
      column = rows[a];
      for(uint8_t b = 0; b < 25; b++){
        if (bitRead(column, b) ) {
          firstframe.SetPoint(b, a);
        }
      }
    }
    last_min_hack_inc = minute_hack_inc;
    last_time_inc = time_inc;
  }  
  firstframe.RefreshAll(1); //Draw frame buffer one time
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

/*
 * Prepare out to display ith time increment
 *   i -- 0 to 287 time increment indexa
 * out -- points to column data to prepare
 * * DISPLAYS: 288 32 bit settings.  one for each 5-minute period.  up to 32 words per setting.
 * To turn on word one in ith display: 10000000000000000000000000000000
 * WORDS:  3 * n + 1 bytes.  first byte is # words followed by ordered words.
 *                                    x         y       l
 *        Each word is defined by startcol, startrow, len
 */
void getdisplay(int i, uint32_t* rows){
  uint8_t bits;     // holds the on off state for 8 words at a time
  uint8_t word[3];  // start columm, start row, length of the current word
  uint8_t x, y, len;

  for(uint8_t j = 0; j < n_byte_per_display; j++){ // j is a byte index 
    // read the state for the next set of 8 words
    bits = pgm_read_byte(DISPLAYS + 1 + (i * n_byte_per_display) + j);
    for(uint8_t k = 0; k < 8; k++){                     // k is a bit index
      if((bits >> k) & 1){                              // check to see if word is on or off
        int idx = 3 *(j * 8 + k);                       // if on, read location and length
        x = pgm_read_byte(WORDS + idx + 1);
        y = pgm_read_byte(WORDS + idx + 2);
        len = pgm_read_byte(WORDS + idx + 3);
        for(int m=x; m < x + len; m++){ // and display it
          rows[2 * y    ] |= (uint32_t)1 << (2 * m);    // rows/cols swapped
          rows[2 * y + 1] |= (uint32_t)1 << (2 * m) ;   // double up
          rows[2 * y    ] |= (uint32_t)1 << (2 * m + 1);// double up
          rows[2 * y + 1] |= (uint32_t)1 << (2 * m + 1);// double up
        }
      }
    }
  } 
}

/*
 * minutes_hack -- prepare display to show ith minute hack intrement
 *   i -- minute hack index
 * out -- display to prepare
 */
uint32_t minutes_hack(uint8_t i){
  // 0 <= i < 26 , 26 states
  // out, the display
  uint32_t out = 0;
  uint8_t start;

  if(i < 5){
    start = 0;
  }
  else{
    start = i - 5;
  }
  for(uint8_t j = start; j < i && j < 24; j++){
    out |= (((uint32_t)1) << j) * (j < i);
  }
  return out;
}

