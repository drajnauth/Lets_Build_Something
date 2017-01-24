#ifndef _UART_H_
#define _UART_H_


#define RBUFF 16		// Max RS232 Buffer Size
#define MAX_COMMAND_ENTRIES 3 

char ProcessSerial ( void );
unsigned char ParseSerial ( char *str );
void ResetSerial (void);
void ErrorOut ( void );

#endif // _UART_H_




