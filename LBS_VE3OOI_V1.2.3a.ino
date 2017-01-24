


/*********************************************************************
This is an example sketch for our Monochrome Nokia 5110 LCD Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/338

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution

Modified by N6QW to include LSB/USB select and a tone for Tune up
3/1/2015

Peel Amateur Radio Club
Modified K.Heise and K.Chase Sep 2015
Add direct conversion receiver oscillator mode select


********************************************************************/
// TUNE tone to a speaker
// Connection is very similar to a piezo or standard speaker. Except, instead
// of connecting one speaker wire to ground you connect both speaker wires to Arduino pins.
// Add an inline 100 ohm resistor between one of the two pins and the speaker wire.
// CONNECTION:
//   Pins  9 & 10 - ATmega328, ATmega128, ATmega640, ATmega8, Uno, Leonardo, etc.
//   Pins 11 & 12 - ATmega2560/2561, ATmega1280/1281, Mega
//   Pins 12 & 13 - ATmega1284P, ATmega644
//   Pins 14 & 15 - Teensy 2.0
//   Pins 25 & 26 - Teensy++ 2.0

#include "Rotary.h"
#include "Wire.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <toneAC.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

#include "VE3OOI_Si5351_v1.3.h"         // VE3OOI Si5351 Routines
#include "LBS_VE3OOI_V1.3.h"
#include "Skinny_UART.h"

// If UPDATE_EEPROM is defined then, messages are storeded in EEPROM and the "L" command is used to copy text into EEPROM one
// line at a time.  If UPDATE_EEPROM is NOT defined, then messages stored in Program Memory is used for messages.
//#define UPDATE_EEPROM     


extern char commands[MAX_COMMAND_ENTRIES];
extern unsigned char command_entries;
extern unsigned long numbers[MAX_COMMAND_ENTRIES];

// This defines the various variables (See Silicon Labs AN619 Note)
extern Si5351_def multisynth;

#define NOTE_B5      988
#define TUNE_VOLUME  5         // Speaker volume setting
#define ENCODER_B    2         // Encoder pin B on D2
#define ENCODER_A    3         // Encoder pin A on D3

//----#define ENCODER_BTN  A3        // Encoder button - Original PARC Design
#define ENCODER_BTN  A2        // Encoder button A2 - VE3OOI design

#define DC_MODE_BTN  11        // Direct conversion mode button 

#define LSB_BTN      A1        // Select USB/LSB mode pin 

//----#define TUNE_BTN     A2        // Turns on RF output for tuning - Original PARC Design
#define TUNE_BTN     A3        //Turns on RF output for tuning - VE3OOI design

#define BACK_LIGHT   125       // Sets the backlight level for the display - original was 125
#define CONTRAST     50        // Sets the contrast level for the display - original was 90
#define XMIT_ON      A6        // Output used to enable transmitter for tuning

#define CW_ON        12        // Input used to determine if to send CW. Must key transmitter externally
#define SENSOR       A7        // A7 connected to audio signal. ADC does a peak detect


/*
// Original PARC design: 
// Display interface - Software SPI (slower updates, more flexible pin options):
// pin D8 - Serial clock out (SCLK)
// pin D7 - Serial data out (DIN)
// pin D6 - Data/Command select (D/C)
// pin D4 - LCD chip select (CS)
// pin D5 - LCD reset (RST)
// note there are different pin configurations on some display modules


New Values for VE3OOI PCB Board
// pin D8 - Serial clock out (SCLK)
// pin D7 - Serial data out (DIN)
// pin D6 - Data/Command select (D/C)
// pin D5 - LCD chip select (CS)
// pin D4 - LCD reset (RST)
*/

//----Adafruit_PCD8544 display = Adafruit_PCD8544(8, 7, 6, 4, 5);  // Original PARC Design
Adafruit_PCD8544 display = Adafruit_PCD8544(8, 7, 6, 5, 4); // VE3OOI design

const int Backlight = 9; // Analog output pin that the LED is attached to
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};


#ifndef UPDATE_EEPROM  
const char helpmsg[] PROGMEM = {
  "Press characters and numbers and press Enter key to execute a function\r\n"
  "Characters define a function and numbers set levels\r\n"
  "  E.g. C is for calibration, R is for reset, D is for display, etc\r\n"
  "  E.g. If you press 'CM 9' and press enter key, this tells the arduino to execute function CM using value 9\r\n"
  "\r\nConsole command summary\r\n"
  "C is use for Calibration\r\n"
  " C - Display all saved calibration parameters\r\n"
  " CW - Manual write calibration parameters to EEPROM\r\n"
  " CS n f - Enter Si5351 calibration value n and set freq to f in Hz\r\n"
  "   Eg: CS 60 10000000 - this sets Si Calibration to 60 and frequecy set to 10 MHz\r\n"
  " CM n - Calibrate Smeter to S level n. Only supports S9 to S5\r\n"
  "   Eg: CM 9 - this expects a S9 signal source connnect to antenna and calibrates SMeter\r\n"
  " CMO n - Enter Smeter offset n between 100 to 150\r\n"
  "   Eg: CMO 115 - this shifts the Smeter display by 115\r\n"
  " CMD n - Enter Smeter delay or sensitivity n between 0 to 20\r\n"
  "   Eg: CMD 1 - this causes the display to pause by 1 unit before updating\r\n"
  "D - Display all saved parameters\r\n"
  "R - Reset LBS software\r\n"};
  
const char guidemsg[] PROGMEM = {
  "Si5351 Calibration Guide\r\n"
  "1) Connect Frequency counter to any Si5351 Clock output\r\n"
  "2) Enter 'CS 100 10000000' to set Calibration to 100 for 10000000 Hz. Verify Clock output accuracy\r\n"
  "3) Reenter CS command with change calibration value to adjust frequency\r\n"
  "   Eg: CS 90 10000000, lowers calibration value from 100\r\n"
  "4) Enter R to reset back to normal LBS Radio mode.  Alternatively power off/on arduino\r\n"
  "\r\nSmeter Calibration Guide\r\n"
  "1) Connect S5 to S9 signal source to antenna\r\n"
  "2) Enter 'CM 9' to set Calibration for S9 signal source, 'CM 5' for S5, etc\r\n"
  "3) After a few seconds, enter 'CW' to save calibration\r\n"
  "4) Enter 'CMO 115' to set initial Smeter offset. Check Smeter display to see if it lines up on suitable mark\r\n"
  "5) Reenter a new offset to adjust display.  E.g. 113 will shift display left, 117 will shift display right\r\n"
  "6) Connect the radio to an antenna with a real signal and check Smeter sensitivity\r\n"
  "7) If sensitive is too slow or too fast use 'CMD' to adjust sensitivity.\r\n"
  "'CMD 0' is most sensitive and 'CMO 10' is least sensitive"};

 const char bannermsg[] PROGMEM = {"\r\nPARC LBS Build (VE3OOI) V1.2.3a\r\n"}; 
#endif 

// code for si5351 clock generator module from Adafruit
// it is controlled via i2c buss on pins a4 and a5
//////Si5351 si5351;

int_fast32_t rx = DEFAULT_RX_FREQ;    // Starting frequency of VFO
int_fast32_t rx2 = 1;           // variable to hold the updated frequency
int_fast32_t increment = 100;    // starting VFO update increment in HZ.
int_fast32_t LSBbfoFreq = 0;    // The current LSB bfo frequency
int_fast32_t USBbfoFreq = 0;    // The current USB bfo frequency
int_fast32_t bfo2 = 0;         // Prior frequency of BFO
int_fast32_t bfo = 4913700L;
int_fast32_t DC_RX_Freq = rx - bfo; // initial value for direct conversion receive frequency

String hertz = " 100";
byte ones, tens, hundreds, thousands, tenthousands, hundredthousands, millions ; //Placeholders

unsigned char DC_RX_mode = 0;  // High = direct conversion on CLK0 is enabled

unsigned char LSB_Mode = 1;

unsigned char  EncButtonState = 0;
unsigned char  TuneButtonState = 0;
unsigned char  lastButtonState = 0;

lbs_struture lbsmem;
unsigned long timeLapse;
unsigned long flags;
unsigned int SmeterDelay;
double uvLevel;

Rotary EncoderInput = Rotary(ENCODER_B, ENCODER_A); // sets the pins the rotary encoder uses.  Must be interrupt pins.

void setup() {

  Serial.begin(9600); // connect to the serial port

  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

  pinMode(LSB_BTN, INPUT);    // Selects either LSB when high
  digitalWrite(LSB_BTN, HIGH);

  pinMode(TUNE_BTN, INPUT);    //Tune mode
  digitalWrite(TUNE_BTN, HIGH);

  pinMode(DC_MODE_BTN, INPUT);  // Select direct conversion mode if this input is LOW
  digitalWrite(DC_MODE_BTN, HIGH);

  pinMode(ENCODER_BTN, INPUT); // Connect to encoder button that goes to GND on push
  digitalWrite(ENCODER_BTN, HIGH);

  DC_RX_mode=digitalRead(DC_MODE_BTN); // Set mode during initialization. If you want to change it, remove jumper/switch and reboot

  pinMode(XMIT_ON, OUTPUT);

  ResetLBS (); 
}


void ResetLBS (void) 
{
  LSBbfoFreq = LSB_BFO_FREQ;
  USBbfoFreq = USB_BFO_FREQ;
  timeLapse = millis();
  flags = 0;
  SmeterDelay = 0;
  uvLevel = 0;        // For S9=-34dbu, -34=20log(S9_ADCLevel/1uV_ADC_Level), Solve for 1uV_ADC_LEVEL = S9_ADCLevel * 50

  // Setup Display
  display.begin();  // init display
  // set backlight & contrast level
  analogWrite(Backlight, BACK_LIGHT);
  display.setContrast(CONTRAST);
  // show splashscreen
  display.display(); 
  delay(250);
  
  // clears the screen and display initial screen
  display.clearDisplay();   
  setupScreen ();
  showFreq();
  showMode();

  // Read saved setting to overwrite defaults
  ReadSettings (); 
  
  //  initialize the Si5351
  ResetSi5351 (SI_CRY_LOAD_8PF);
  
  // Setup si5351 for direct conversion on CLK0 or dual output otherwise. 
  if (!DC_RX_mode) {
    SetFrequency (SI_CLK0, SI_PLL_A, (unsigned long int)rx, SI_CLK_8MA);
  }
  else {
    SetFrequency (SI_CLK0, SI_PLL_A, (unsigned long int)rx, SI_CLK_8MA);
    SetFrequency (SI_CLK2, SI_PLL_B, (unsigned long int)bfo, SI_CLK_8MA);
  }
  
  // Reset TTY
  ResetSerial ();
#ifdef UPDATE_EEPROM  
  Serial.println ("PARC LBS Build (VE3OOI) V1.2.3a");
#else      
  pgmMessage (bannermsg);
#endif

  
     
}


ISR(PCINT2_vect) {
  unsigned char result = EncoderInput.process();

  if (result) {
    if (result == DIR_CW) {
      rx += increment;
    } else if (result == DIR_CCW){
      rx -= increment;
    };

    if (rx >= 12216700) {
      rx = rx2;
    }; // UPPER VFO LIMIT
    if (rx <= 11913700) {
      rx = rx2;
    }; // LOWER VFO LIMIT
  }
}


void loop ()
{


  while (flags & CALIBRATE_SI5351 || flags & CALIBRATE_SMETER) {
    ProcessSerial ();
    
    if (flags & CALIBRATE_SMETER) {
      SmeterDelay = peakDetect (PKDETECT_SAMPLES);
      lbsmem.uVLevel = SmeterDelay*uvLevel;
      showSmeter ();
      delay(100);
    }    
  }

  ProcessSerial ();
  
  if (SmeterDelay++ > lbsmem.uVDelay) {
    showSmeter();
    SmeterDelay = 0;
  }

  checkMode(); 

  //If LSB_BTN is true do the following.
  LSB_Mode = digitalRead(LSB_BTN);
  if (LSB_Mode) { 
    bfo = LSBbfoFreq;
  } else {               //Otherwise use USB
    bfo = USBbfoFreq;
  }
  if (bfo2 != bfo) {
    bfo2 = bfo;
    SetFrequency (SI_CLK2, SI_PLL_B, (unsigned long int)bfo, SI_CLK_8MA);
  }

  if  (rx != rx2) {
    showFreq();


    if (!DC_RX_mode) {
      DC_RX_Freq = rx - bfo;
      SetFrequency (SI_CLK0, SI_PLL_A, (unsigned long int)DC_RX_Freq, SI_CLK_8MA);
    } else {
      SetFrequency (SI_CLK0, SI_PLL_A, (unsigned long int)rx, SI_CLK_8MA);
      SetFrequency (SI_CLK2, SI_PLL_B, (unsigned long int)bfo, SI_CLK_8MA);
    }

    rx2 = rx;
      
    lbsmem.rx = rx;
    lbsmem.bfo = bfo;
    lbsmem.increment = increment;
    hertz.toCharArray(lbsmem.hertz, sizeof(lbsmem.hertz)) ;
    flags |= UPDATE;    
  }

  EncButtonState = digitalRead(ENCODER_BTN);
  if (EncButtonState == LOW) {
    setincrement();
    delay(200);
    showFreq();
     
    lbsmem.rx = rx;
    lbsmem.bfo = bfo;
    lbsmem.increment = increment;
    hertz.toCharArray(lbsmem.hertz, sizeof(lbsmem.hertz)) ;
    flags |= UPDATE;
  }

  showMode();

  if (millis() - timeLapse > EEPROM_WRITE_TIME) {
    if (flags & UPDATE) {
      EEPROMWrite (0, (char *)&lbsmem, sizeof(lbsmem));
      flags &= ~UPDATE;
    }
    timeLapse = millis();
  } 
}

void ExecuteSerial (char *str)
{
  
// num defined the actual number of entries process from the serial buffer
// i is a generic counter
  unsigned char num;
  unsigned int i;
  
// This function called when serial input in present in the serial buffer
// The serial buffer is parsed and characters and numbers are scraped and entered
// in the commands[] and numbers[] variables.
  num = ParseSerial (str);

// Process the commands
// Note: Whenever a parameter is stated as [CLK] the square brackets are not entered. The square brackets means
// that this is a command line parameter entered after the command.
// E.g. F [CLK] [FREQ] would be mean "F 0 7000000" is entered (no square brackets entered)
  switch (commands[0]) {

    // Calibrate the Si5351.
    // Syntax: C S [CAL] [FREQ], where CAL is the new Calibration value and FREQ is the frequency to output
    // Syntax: C M [CAL], where CAL is the new Calibration value for S Meter
    // Syntax: C M [S], where S is between 1 and 9 to indicate S Level input
    // Syntax: C , If no parameters specified, it will display current calibration values
    case 'C':             // Calibrate
      // First, Check inputs to validate
      if (commands[1] == 'W') {
        EEPROMWrite(0, (char *)&lbsmem, sizeof(lbsmem));
        ResetLBS ();
        
      } else if (commands[1] == 'S') {
        if (numbers[1] < SI_MIN_OUT_FREQ || numbers[1] > SI_MAX_OUT_FREQ) {
          ErrorOut ();
          break;
        }
      
        // New value defined so read the old values and display what will be done
        ReadSettings ();
        Serial.print ("Old: ");
        Serial.println (multisynth.correction);
        Serial.print ("New: ");
        Serial.println (numbers[0]);
        Serial.println ("Enter CW to End");

        // Store the new value entered, reset the Si5351 and then display frequency based on new setting     
        multisynth.correction = lbsmem.correction = numbers[0];
        EEPROMWrite(0, (char *)&lbsmem, sizeof(lbsmem));
  
        ResetSi5351 (SI_CRY_LOAD_8PF);
        ReadSettings ();
        SetFrequency (SI_CLK0, SI_PLL_A, numbers[1], SI_CLK_8MA);
        SetFrequency (SI_CLK1, SI_PLL_B, numbers[1], SI_CLK_8MA);
        SetFrequency (SI_CLK2, SI_PLL_B, numbers[1], SI_CLK_8MA);
        flags |= CALIBRATE_SI5351;
        
      } else if (commands[1] == 'M' && !commands[2]) {
          if (numbers[0] > 1000 && numbers[0] < 10000) {
            lbsmem.uVLevel = numbers[0];
          } else if (numbers[0]>4 && numbers[0]<10) { 
            uvLevel = numbers[0];
            switch (numbers[0]) {
              case 9:
                uvLevel = -34;
                break;
              case 8:
                uvLevel = -34;
                break;
              case 7:
                uvLevel = -34;
                break;
              case 6:
                uvLevel = -34;
                break;
              case 5:
                uvLevel = -34;
                break;
              default:
                uvLevel = 0;
                ErrorOut ();
            }  
            uvLevel = pow(10, -uvLevel/20.0);
            flags |= CALIBRATE_SMETER;
            Serial.println ("Enter CW to End");
            
          } else {
            ErrorOut ();
          }
          
      } else if (commands[1] == 'M' && commands[2] == 'O') {
          if (numbers[0] > 100 && numbers[0] < 150) {
            lbsmem.uVOffset = numbers[0];
            EEPROMWrite(0, (char *)&lbsmem, sizeof(lbsmem));
          }
          
      } else if (commands[1] == 'M' && commands[2] == 'D') {
          if (numbers[0] < 20) {
            lbsmem.uVDelay = numbers[0];
            EEPROMWrite(0, (char *)&lbsmem, sizeof(lbsmem));
          }
          
      } else if (!numbers[0] && !numbers[1]) {
          DumpEEPROM();
            
      }  
      break;

    case 'D':             // Dump EEProm
      DumpEEPROM ();
      break;


    // Help Screen. This consumes a ton of memory but necessary for those
    // without much computer or programming experience.
    case 'H':             // Load Message
#ifdef UPDATE_EEPROM  
      ReadEEMessage();
#else      
      pgmMessage (helpmsg);
      pgmMessage (guidemsg);
#endif
      break;     

#ifdef UPDATE_EEPROM  
    case 'L':             // Load Message
      LoadEEMessage();
      break;          
#endif 

    case 'R':             // Reset
      ResetLBS ();
      break;

    // If an undefined command is entered, display an error message
    default:
      ErrorOut ();
  }
  
}



void DumpEEPROM (void)
{
  unsigned char i;
  
  ReadSettings (); 
  Serial.print (" Si: ");
  Serial.print (lbsmem.correction);
  Serial.print (" Sm: ");
  Serial.print (lbsmem.uVLevel);
  Serial.print (" Off: ");
  Serial.print (lbsmem.uVOffset);
  Serial.print (" Dly: ");
  Serial.print (lbsmem.uVDelay);
  Serial.print (" Rx: ");
  Serial.print (lbsmem.rx);
  Serial.print (" Inc: ");
  Serial.print (lbsmem.increment);
  Serial.print (" BFO: ");
  Serial.print (lbsmem.bfo);
  Serial.print (" Txt: ");
  for (i=0; i<sizeof(lbsmem.hertz); i++) Serial.print (lbsmem.hertz[i]);
  Serial.println();     
}

#ifdef UPDATE_EEPROM  
void LoadEEMessage (void)
{
  char tmp;
  unsigned int i, j;
  unsigned int eeaddr;

  Serial.println ("+++ to end");
  eeaddr = MSGSTART;
  i = 0;
  EEPROMWrite (eeaddr, (char *)&i, sizeof(i));
  
  i = 0;
  j = 0; 
  eeaddr = MSGSTART + sizeof(i);     
  timeLapse = millis();

  while ( i < (MAXEEPROM-MSGSTART-2) && timeLapse && (millis()-timeLapse)< 240000) {
    while (Serial.available() > 0) {
      tmp = Serial.read();
      if (tmp == '+' && j++ >= 2) {          // End data input with "+++" detected
          timeLapse = 0;
      } else if (tmp != '+') {
        Serial.print (MAXEEPROM-MSGSTART-2-i);
        Serial.print (":");
        Serial.println (tmp);
        EEPROM.write(eeaddr, tmp);
        i++;
        eeaddr++;
     }
    }  
   
  }
  
  timeLapse = millis();  
  if (i) {
    eeaddr = MSGSTART;
    EEPROMWrite (eeaddr, (char *)&i, sizeof(i));
  }
}  


void ReadEEMessage (void)
{
  unsigned int i, j;
  char tmp;
  unsigned int len;
  unsigned int eeaddr;

  eeaddr = MSGSTART;
  EEPROMRead (eeaddr, (char *)&len, sizeof(len));
  Serial.println (len);
  
  if (len <= 10 || len > (MAXEEPROM-MSGSTART-2)) {
    return;
  }
  
  eeaddr = MSGSTART + sizeof(len);
  for (i=0; i<len; i++) {
    tmp = EEPROM.read(eeaddr+i);
    Serial.print (tmp);
    Serial.flush();
  }
}  
#else
void pgmMessage (const char *msg) 
{
  char tmp = 0xd;
  int i = 0;
  
  while (tmp) {
    tmp = (char)pgm_read_byte(msg+i);
    i++;
    if (tmp) Serial.print (tmp);
  }
  Serial.println("\r\n");

}
#endif 


// This routines are NOT part of the Si5351 and should not be included as part of the Si5351 routines.
// Note that some arduino do not have eeprom and would generate an error during compile.
// If you plan to use a Arduino without eeprom then you need to hard code a calibration value.
void EEPROMWrite (unsigned int memAddr, char *cptr, unsigned int memlen)
// write the calibration value to Arduino eeprom
{
  unsigned int i;
  for (i=0; i<memlen; i++) {
    EEPROM.write((memAddr+i), (unsigned char)*cptr);
    cptr++;
  }
}

void EEPROMRead (unsigned int memAddr, char *cptr, unsigned int memlen)
// read the calibratio value from Arduino eeprom
{
  unsigned int i;
  for (i=0; i<memlen; i++) {
    *cptr =  EEPROM.read(memAddr+i);
    cptr++;
  }
}


void ReadSettings (void)
{
  EEPROMRead(0, (char *)&lbsmem, sizeof(lbsmem)); 

  increment = lbsmem.increment;
  rx = lbsmem.rx;
  bfo = lbsmem.bfo;
  hertz = String (lbsmem.hertz);
  
  if (lbsmem.correction < -1000 || lbsmem.correction > 1000) {
    multisynth.correction = lbsmem.correction = 1;
  } else multisynth.correction = lbsmem.correction;
 
  if (increment > 100000) {
    increment = 100;
    hertz = " 100";
  }
 
  if (rx < LOW_RX_FREQ || rx > HIGH_RX_FREQ) {
    rx = DEFAULT_RX_FREQ; 
  }
  if (bfo < LSB_BFO_FREQ || bfo > LSB_BFO_FREQ) {
    bfo = LSB_BFO_FREQ;
  }  
}  


void setincrement (void) 
{
  if (increment == 10) {
    increment = 100;
    hertz = "100";
  } else if (increment == 100) {
    increment = 1000;
    hertz = " 1K";
  } else if (increment == 1000) {
    increment = 10000;
    hertz = "10K";
  } else if (increment == 10000) {
    increment = 100000;
    hertz = "1xK";
  } else if (increment == 100000) {
    increment = 1000000;
    hertz = " 1M";
  } else {
    increment = 10;
    hertz = " 10";
  };
  
}


void showFreq (void) 
{
  // Clear the display
  display.fillRect(0, 0, 84, 16, WHITE); 
  
  millions = int((rx - bfo) / 1000000);
  hundredthousands = (((rx - bfo ) / 100000) % 10);
  tenthousands = (((rx - bfo ) / 10000) % 10);
  thousands = (((rx - bfo ) / 1000) % 10);
  hundreds = (((rx - bfo ) / 100) % 10);
  tens = (((rx - bfo ) / 10) % 10);
  ones = (((rx - bfo ) / 1) % 10);
  
  display.setTextColor(BLACK);
  display.setTextSize(2);
  if (millions > 9) {
    display.setCursor(0, 0);
  }
  else {
    display.setCursor(12, 0);
  }
  display.print(millions);
  // display.print(".");
  display.print(hundredthousands);
  display.print(tenthousands);
  display.print(thousands);
  display.setTextSize(1);
  //display.print(".");

  display.print(hundreds);
  display.print(tens);
  display.print(ones);
  //display.print("Nhz.");

// Display Increment
  display.setCursor(60, 8);
  display.print(hertz);

  display.display();

}


void setupScreen (void) 
{
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor (24, 18);
  display.print("PARC50-LBS");
  display.setCursor (60, 26);
  display.print("XCVR");
  display.setCursor (40, 26);
  display.print("SSB");
  display.setCursor (20, 26);
  display.print("40M");


  display.fillRect(1, 35, 83, 12, WHITE); // Makes S Mtr Background & tick marks for scaling the S Meter  S1, S3, S5, S9, 30/9
  display.fillRect(1, 35, 83, 12, BLACK);
  display.fillRect(40, 37, 2, 2, WHITE);
  display.fillRect(30, 37, 2, 2, WHITE);
  display.fillRect(20, 37, 2, 2, WHITE);
  display.fillRect(55, 37, 2, 2, WHITE);
  display.fillRect(75, 37, 2, 2, WHITE);
  //display.display();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(3, 38);
  display.print("S=");
  display.setTextSize(1);
  
//  display.setTextSize(1);
//  display.setTextColor(BLACK);
//  display.setCursor(3, 38);
//  display.print("by VE3OOI");
 
  display.display();
}


unsigned int peakDetect (unsigned int samples)
{
  unsigned int i;
  unsigned long pkVoltage;
  int AnalogVoltIn = 0;

  pkVoltage = 0;
  for (i=0; i<samples; i++) {
    AnalogVoltIn = analogRead(SENSOR);
    if (AnalogVoltIn > pkVoltage) pkVoltage = AnalogVoltIn;
  }
  
  pkVoltage = (pkVoltage * 707)/1000; 
 
  return (unsigned int)pkVoltage; 
}  

void showSmeter (void) 
{
  unsigned int rmsVoltage;
  int SMeterVal = 0;

  rmsVoltage = peakDetect (PKDETECT_SAMPLES);
  if (!rmsVoltage) rmsVoltage = 1;

  SMeterVal = 20 * log( (double)rmsVoltage/(double)lbsmem.uVLevel);

  SMeterVal += lbsmem.uVOffset;
  if (SMeterVal < 0) SMeterVal = 1;
  if (SMeterVal > 68) SMeterVal = 68;
  display.fillRect(16, 40, 83, 3, BLACK);
  display.fillRect(16, 40, SMeterVal, 3, WHITE);
  display.display();
}

void showMode (void)
{

  // Display the mode
  display.fillRect(0, 25, 19, 9, BLACK); //
  display.setTextColor(WHITE);
  display.setCursor(1, 26); 
  
  //Use LSB_BTN to determing mode.
  if (LSB_Mode) { 
    display.println("LSB");
  }else{
    display.println("USB");
  }
  display.display();
  display.setTextColor(BLACK);
}


void showTune (void)
{
  display.setTextSize(1);
  display.fillRect(0, 0, 8, 25, BLACK); //
  display.setTextColor(WHITE); // white text on black background
  display.setCursor(1, 1);  // top left corner
  display.print("T");
  display.setCursor(1, 9);
  display.print("U");
  display.setCursor(1, 17);
  display.print("N");
  display.display();
  display.setTextColor(BLACK);
}  

void clearTune (void)
{
  display.setTextSize(1);    // This prints a white rectangle over the black TUNE and makes it disappear from the scereen
  display.fillRect(0, 0, 8, 25, WHITE);
  display.display();
  display.setTextColor(BLACK);
}  

void checkMode() {
  // creates a momentary tuning pulse @ 50% duty cycle and makes 'TUN' appear on the screen
  TuneButtonState = digitalRead(TUNE_BTN); 
  if (TuneButtonState != lastButtonState) {
    if (TuneButtonState == LOW) {
      digitalWrite(XMIT_ON, HIGH);
      showTune();
      delay(10);
      //
      //   toneAC( frequency [, volume [, length [, background ]]] ) - Play a note.
      //     Parameters:
      //       * frequency  - Play the specified frequency indefinitely, turn off with toneAC().
      //       * volume     - [optional] Set a volume level. (default: 10, range: 0 to 10 [0 = off])
      //       * length     - [optional] Set the length to play in milliseconds. (default: 0 [forever], range: 0 to 2^32-1)
      //       * background - [optional] Play note in background or pause till finished? (default: false, values: true/false)
      //   toneAC()    - Stop playing.
      //   noToneAC()  - Same as toneAC()
      toneAC(NOTE_B5, TUNE_VOLUME);

// Force BFO to change to centre frequency of crystal filter. With no audio, this is a pure carrier.
      bfo2 = 0;
      LSBbfoFreq = 4915200L;
      USBbfoFreq = 4915200L;
   
    } else {
      digitalWrite(XMIT_ON, LOW);
      clearTune();
      toneAC();  //turn off tone
      
// Force BFO to change back to LSB and USB frequencies      
      bfo2 = 0;
      LSBbfoFreq = LSB_BFO_FREQ;
      USBbfoFreq = USB_BFO_FREQ;
    }
    lastButtonState = TuneButtonState;
  }
 
}


