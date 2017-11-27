#ifndef _MAIN_H_
#define _MAIN_H_

void showSmeter (void); 
void showInc (void); 
void showFreq (void); 
void setincrement (void); 
void setupScreen (void);
void showTune (void);

// Flags
#define UPDATE 1
#define CALIBRATE_SI5351 2
#define CALIBRATE_SMETER 4

#define PKDETECT_SAMPLES 100  
#define SMETER_CALIBRATION -34

#define EEPROM_WRITE_TIME 60000    // Ever 1 minutes update EEPROM   
#define LSB_BFO_FREQ 4913700L
#define USB_BFO_FREQ 4916700L

#define  LOW_RX_FREQ     11913700
#define  HIGH_RX_FREQ    12213700
#define  DEFAULT_RX_FREQ 12113700

#define HELPMSG sizeof(lbs_struture);

void EEPROMWrite (unsigned int memAddr, char *ptr, unsigned int memlen);
void EEPROMRead (unsigned int memAddr, char *ptr, unsigned int memlen);
void ReadSettings (void);
void ExecuteSerial (char *str);
void ResetLBS (void); 
unsigned int peakDetect (unsigned int samples);
void ReadEEMessage (void);
void LoadEEMessage (void);
void pgmMessage (const char *msg);
void DumpEEPROM (void);

typedef struct {
  long int correction;
  unsigned long rx;
  unsigned long increment;
  unsigned long bfo;
  unsigned int uVLevel;
  unsigned int uVOffset;
  unsigned int uVDelay;
  char hertz[5];
} lbs_struture;

#define MSGSTART sizeof(lbs_struture)
#define MAXMSGBUF 100
#define MAXEEPROM 512


#endif // _MAIN_H_
