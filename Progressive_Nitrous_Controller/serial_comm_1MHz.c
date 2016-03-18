/*******************************

	ECET 17900 - Introduction to Digital Systems

	Serial functions required to utilize STDIO with the Atmel microcontrollers

	Name:	Jeffrey J. Richardson
	revised: Mark T. Carnes
	Date:	October 29, 2011

	Description:
		revA :  add function "clearScreen"			2-10-12	MTC
		revB :  Set up for 1 MHz clock operation	2-15-14 MTC

********************************/
#define F_CPU	1000000UL

#define line1	0x80
#define line2	0xC0
#define line3	0x94
#define line4	0xD4
void init_UART(void);
static int uart_putchar(char ch, FILE *stream);
static int uart_getch(FILE *stream);
void clearLCD(void);
void clearTerminal(void);
void setCursor(uint8_t);


static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, uart_getch, _FDEV_SETUP_RW);	/* Assign I/O stream to UART */


void init_UART(void)
{

	UCSR0A = 0x02;	// Double UART transmission speed for better accuracy	
	UCSR0B = 0x18;			// enable UART TX and RX
	UCSR0C = 0x06;			// set the UART for N, 8, 1
	UBRR0L = 0b00001100;	// set BAUD Rate for 9600 with 1 MHz clock
	UBRR0H = 0;

	stdout = &mystdout;		// define the output stream
	stdin = &mystdout;		// define the input stream	


	//putchar(0x0D);			// setting Baud rate

	//putchar(0x7C);	// command flag		
	//putchar(0x09);    // toggling splash screen OFF
	
	//putchar(0x7C);		// command flag
	//putchar(0x03);		// setting LCD type 20 characters
	//putchar(0x7C);		//command flag
	//putchar(0x05);		// setting LCD to 4 lines
}


// the following function sends a single character out of the serial port
static int uart_putchar(char ch, FILE *stream)
{

    while ( (UCSR0A & (1 << UDRE0)) == 0 );	/* wait until there is room in the 
												transmit buffer */

    UDR0 = ch;								/* load the character into the UART
											   data register */

    return 0;
}

// the following function waits for a serial character to be received
static int uart_getch(FILE *stream)
{
   unsigned char ch;   						/* create a variable to hold the 
												received value */
   
   while ( (UCSR0A & (1<<RXC0)) == 0 );		/* wait until "received character" 
												flag is set */
     
   ch=UDR0;  								/* load the received character into
												the local variable */


   return ch;								// return the value
}

void clearLCD(void)
{
	putchar(0xFE);  //Next byte is a COMMAND
	putchar(0x01);	// Clear LCD screen and set cursor to line 1
}

// The following function clears the hyperterminal screen

void clearTerminal(void)
{
	putchar(0x1B);  //ASCII for Escape key
  	putchar('[');
  	putchar('2');
  	putchar('J');
}

void setCursor(uint8_t position)
{
	putchar(0xFE);  //Next byte is a COMMAND
	putchar(position);	// Move cursor
}

