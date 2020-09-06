/* PiTrex I/O functions for initialising Raspberry Pi GPIO, and Reading/Writing from/to Vectrex
 * Kevin Koster, 2020.
 * "Short" I/O routines by Chris Salomon.
 * V. 1.0
 */

#include <unistd.h>
#include <stdio.h>

#include "bcm2835.h"
#include "pitrexio-gpio.h"
#include "via6522.h"

#ifndef FREESTANDING
 #include <fcntl.h>
 #include <string.h>
#endif

//Comment this out to poll RDY input instead of using Event Detection Register.
//Note: *_end functions may not work reliably without using the EDR.
//#define USE_EDR

/*
UART0
    This configures the GPIO to use UART0 instead of UART1 on bits 14 and 15.
    This is the default configuration used by Linux when run on Raspberry Pis without WiFi support.

 For bare-metal (FREESTANDING):
    This is a fixed setting determining the GPIO configuration used.
	In practice the serial terminal should work fine in bare-metal on both the Pi0 and Pi0W _without_
	UART0 defined.

 For Linux (not FREESTANDING):
    The library now tries to auto-detect the Pi model by reading /proc/cpuinfo (see the
	vectrexinit function). If this fails, then it defaults to configuring for the Pi0W. Unless UART0
	is defined, in which case it configures for the Pi0.

https://www.raspberrypi.org/documentation/configuration/uart.md
*/
//#define UART0

/*
  Port Assignments:
  ^^^^^^^^^^^^^^^^^
Descrip.|Bit    |Pin    |
D0      |16     |36     |
D1      |17     |11     |
D2      |18     |12     |
D3      |19     |35     |
D4      |20     |38     |
D5      |21     |40     |
D6      |22     |15     |
D7      |23     |16     |

A0      |7      |26     |
A1      |8      |24     |
A2      |9      |21     |
A3      |10     |19     |
A12     |11     |23     |
A14     |12     |32     |
R/#W    |13     |33     |

PB6     |5      |29     |
IRQ_LATCH |6	|31		|

RDY(#OE)|24     |18     |
LATCH_EN|25     |22     |
IRQ     |26     |37     |
#HALT	|27	|13		| -Jumper to GND
#D/A    |N/A    |9      | -GND

3V3     |N/A    |1      |
5V      |N/A    |2      |
GND     |N/A    |6      |

Note: Pins counted from 1, bits counted from 0.
*/

// These variables are set depending on UART configuration based on Pi model
unsigned int writeconfig_GPFSEL1;
unsigned int readconfig_GPFSEL1;

#define PIZERO_GPIO_CONFIG	1
#define PIZEROW_GPIO_CONFIG	2

/*
  Fast, AKA "short" I/O routines by Chris Salomon
  ===============================================
 */
 
#define CARTRIDGE_RW_GPIO         13
#define CARTRIDGE_LATCH_EN_GPIO   25
#define CARTRIDGE_READY_GPIO      24


volatile uint32_t *bcm2835_gpio_GPCLR0;
volatile uint32_t *bcm2835_gpio_GPFSEL1;
volatile uint32_t *bcm2835_gpio_GPFSEL2;
volatile uint32_t *bcm2835_gpio_GPSET0;
volatile uint32_t *bcm2835_gpio_GPLEV0;


// GPIO Pins 10-13 = OUTPUT| 14-15 = TTY | 16-19 = INPUT
#define PIN_READ_DEF do{*bcm2835_gpio_GPFSEL1=readconfig_GPFSEL1;__sync_synchronize();*bcm2835_gpio_GPFSEL2=readconfig_GPFSEL1;} while(0)
#define PIN_WRITE_DEF do{*bcm2835_gpio_GPFSEL1=writeconfig_GPFSEL1;__sync_synchronize();*bcm2835_gpio_GPFSEL2=writeconfig_GPFSEL1;} while(0)

// -1 unkown
// 1 = yes we are in write mode
// 0 = no we are not in write mode
static int isWriteMode = -1;

#define WAIT_TILL_READY_LOW  while ((((*bcm2835_gpio_GPLEV0) & (1 << CARTRIDGE_READY_GPIO)) ? HIGH : LOW))
#define WAIT_TILL_READY_HIGH while (!(((*bcm2835_gpio_GPLEV0) & (1 << CARTRIDGE_READY_GPIO)) ? HIGH : LOW))

#define INIT_VIA_READ \
	do \
	{ \
		if (isWriteMode != 0) \
		{ \
			isWriteMode = 0; \
			PIN_READ_DEF; \
		} \
	} while(0)

#define INIT_VIA_WRITE \
	do \
	{ \
		if (isWriteMode != 1) \
		{ \
			__sync_synchronize(); \
			isWriteMode = 1; \
			PIN_WRITE_DEF; \
		} \
	} while(0)

// only __sync_synchronize
// when TWO writes are directly in row
// everything else is fine!

// Read a byte from the Vectrex at the specified address
unsigned char vectrexread_short(unsigned int address)
{
	// start

	//Sets 2 bits of A12 AND A14. Result is ORed with lowest four bits of address.
	//Shifted to position on 32bit bus
	unsigned long shortaddress = (0x30 | (address & 0x0F)) << 7;

	// was: bcm2835_peri_write (bcm2835_gpio_GPCLR0, 0xFFFFFFFF);	//Set all outputs Low
	*bcm2835_gpio_GPCLR0 = 0xFFFFFFFF;

	//Set Data bus to Input mode:
	INIT_VIA_READ;

	// cartridge port R/#W = 1 -> READ
	// was: bcm2835_gpio_set (CARTRIDGE_RW_GPIO);
	*bcm2835_gpio_GPSET0= (1 << CARTRIDGE_RW_GPIO);

	// Set Address Bus
	// was: bcm2835_peri_write ((bcm2835_gpio_GPSET0, shortaddress);
	*bcm2835_gpio_GPSET0 = shortaddress;

	// Loop until RDY goes High
	WAIT_TILL_READY_HIGH;

	// LATCH EN High
	// was: bcm2835_gpio_set (CARTRIDGE_LATCH_EN_GPIO);
	*bcm2835_gpio_GPSET0= (1 << CARTRIDGE_LATCH_EN_GPIO);

	// end
	unsigned char datain;

	// Poll RDY for LOW to HIGH transistion:
	// Loop until RDY goes Low
	WAIT_TILL_READY_LOW;

	// Loop until RDY goes High
	WAIT_TILL_READY_HIGH;

	// Read data byte
	// was: datain = ( bcm2835_peri_read (bcm2835_gpio + BCM2835_GPLEV0/4) >> 16) & 0xFF;
	datain = ((*bcm2835_gpio_GPLEV0)>> 16) & 0xFF;

	// was: bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFDFFF); //LATCH EN Low, IRQ LATCH Low, R/#W High
	*bcm2835_gpio_GPCLR0 = 0xFFFFDFFF;
	return datain;
}


// Write a byte to the Vectrex at the specified address
void vectrexwrite_short(unsigned int address, unsigned char data)
{
	//Sets 2 bits of A12 AND A14. Result is ORed with lowest four bits of address.
	//Shifted to position on 32bit bus
	unsigned long shortaddress = (0x30 | (address & 0x0F)) << 7;

	// was: bcm2835_peri_write (bcm2835_gpio_GPCLR0, 0xFFFFFFFF);	//Set all outputs Low
	*bcm2835_gpio_GPCLR0 = 0xFFFFFFFF;

	//Set Data bus to Output mode:
	INIT_VIA_WRITE;

__sync_synchronize();
	// Data on Data Bus and Address on Address Bus
	// was: bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPSET0/4), (data << 16) | shortaddress );
	*bcm2835_gpio_GPSET0 = (data << 16) | shortaddress;

__sync_synchronize();

	// LATCH EN High
	// was: bcm2835_gpio_set (CARTRIDGE_LATCH_EN_GPIO);
        *bcm2835_gpio_GPSET0= (1 << CARTRIDGE_LATCH_EN_GPIO);

	// end
	// Poll RDY for LOW to HIGH transistion:

	// theoretically return HERE
	// start a cycle counter (we have max 3)

	// and upon next call of VIA
	// see how many cycles have passed and if needed "block" than

	WAIT_TILL_READY_LOW;
	WAIT_TILL_READY_HIGH;

	// was: bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFFFFF);
	//      bcm2835_gpio_set (13); // R/#W High

	*bcm2835_gpio_GPCLR0 = 0xFFFFDFFF;
}

/*
              Standard I/O Functions
              ======================
 */

//Begin read operation
//Returns 1 if address is valid, otherwise 0.
char vectrexread_begin (unsigned int address)
{
	unsigned char highaddressbits = 0;
	
	if ((address & 0x1000) == 0x1000)
	 highaddressbits += 0x10; //A12 set
	
	if ((address & 0xC000) == 0xC000)
	 highaddressbits += 0x20; //A15 and A14 set
	
	if ((highaddressbits == 0) || ((address & 0x2800) > 0) )
	 return 0; //6522 (VIA) not selected, or invalid address
	
	//Sets highest 2 bits per result of A15 AND A14, as well as A12 value. Result is ORed with lowest four bits of address.
	//Shifted to position on 32bit bus
	unsigned long shortaddress = ((address & 0x0F) | highaddressbits) << 7;
	
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFFFFF);	//Set all outputs Low
	
	//Set Data bus to Input mode:
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL1/4), readconfig_GPFSEL1);		// GPIO Pins 10-13 = OUTPUT| 14-15 = TTY | 16-19 = INPUT
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL2/4), 0x8000);		// GPIO Pins 20-24 = INPUT | 25    = OUTPUT | 26-29 = INPUT
	
	bcm2835_gpio_set (13); // R/#W High
	// Set Address Bus
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPSET0/4), shortaddress); // Set Highs

#ifdef USE_EDR
	bcm2835_gpio_hen (24); // Detect when pin High
	bcm2835_gpio_set_eds (24); //Clear Event Detection Register
	while (!bcm2835_gpio_eds (24)); //Loop
	bcm2835_gpio_clr_hen (24); // Disable High detection
#else
	char status;
	do
	{
	 status = bcm2835_gpio_lev (24);
	}
	while (!status); // Loop until RDY goes High
#endif
	bcm2835_gpio_set (25); // LATCH EN High

#ifdef USE_EDR
	// Detect RDY rising edge with EDR:
	bcm2835_gpio_ren (24); //Enable detection of RDY (#OE) rising edge.
	bcm2835_gpio_set_eds (24); //Clear Event Detection Register
#endif
	return 1;
}

//Return read byte from previously started read operation.
//Returns after operation completed if it hasn't already by the time that it is called.
unsigned char vectrexread_end (void)
{
	unsigned char datain;
	char status;
#ifdef USE_EDR
	while (!bcm2835_gpio_eds (24)); //Loop
	bcm2835_gpio_clr_ren (24); // Disable rising edge detection
#else
	// Poll RDY for LOW to HIGH transistion:
	do
	{
	 status = bcm2835_gpio_lev (24);
	}
	while (status); // Loop until RDY goes Low
	
	do
	{
	 status = bcm2835_gpio_lev (24);
	}
	while (!status); // Loop until RDY goes High
#endif
	
	datain = ( bcm2835_peri_read (bcm2835_gpio + BCM2835_GPLEV0/4) >> 16) & 0xFF; // Read data byte
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFDFFF); //LATCH EN Low, IRQ LATCH Low, R/#W High
	return datain;
}

// Read a byte from the Vectrex at the specified address
unsigned char vectrexread (unsigned int address)
{
	if (vectrexread_begin (address))
	 return vectrexread_end ();
	else
	 return 0;
}


//Uses IRQ LATCH instead of LATCH EN output, causing the read to begin immediately after an IRQ signal is received from the 6522
//Begin read operation
//Returns 1 if address is valid, otherwise 0.
char vectrexread_begin_on_irq (unsigned int address)
{
	unsigned char highaddressbits = 0;
	
	if ((address & 0x1000) == 0x1000)
	 highaddressbits += 0x10; //A12 set
	
	if ((address & 0xC000) == 0xC000)
	 highaddressbits += 0x20; //A15 and A14 set
	
	if ((highaddressbits == 0) || ((address & 0x2800) > 0) )
	 return 0; //6522 (VIA) not selected, or invalid address
	
	//Sets highest 2 bits per result of A15 AND A14, as well as A12 value. Result is ORed with lowest four bits of address.
	//Shifted to position on 32bit bus
	unsigned long shortaddress = ((address & 0x0F) | highaddressbits) << 7;
	
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFFFFF);	//Set all outputs Low
	
	//Set Data bus to Input mode:
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL1/4), readconfig_GPFSEL1);		// GPIO Pins 10-13 = OUTPUT| 14-15 = TTY | 16-19 = INPUT
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL2/4), 0x8000);		// GPIO Pins 20-24 = INPUT | 25    = OUTPUT | 26-29 = INPUT
	
	bcm2835_gpio_set (13); // R/#W High
	// Set Address Bus
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPSET0/4), shortaddress); // Set Highs

#ifdef USE_EDR
	bcm2835_gpio_hen (24); // Detect when pin High
	bcm2835_gpio_set_eds (24); //Clear Event Detection Register
	while (!bcm2835_gpio_eds (24)); //Loop
	bcm2835_gpio_clr_hen (24); // Disable High detection
#else
	char status;
	do
	{
	 status = bcm2835_gpio_lev (24);
	}
	while (!status); // Loop until RDY goes High
#endif
	bcm2835_gpio_set (6); // IRQ LATCH High

#ifdef USE_EDR
	// Detect RDY rising edge with EDR:
	bcm2835_gpio_ren (24); //Enable detection of RDY (#OE) rising edge.
	bcm2835_gpio_set_eds (24); //Clear Event Detection Register
#endif
	return 1;
}

// Read a byte from the Vectrex at the specified address after #IRQ asserted by the Vectrex
unsigned char vectrexread_on_irq (unsigned int address)
{
	if (vectrexread_begin_on_irq (address))
	 return vectrexread_end ();
	else
	 return 0;
}

// Begin writing a byte to the Vectrex
//Returns 1 if address is valid, otherwise 0.
char vectrexwrite_begin (unsigned int address, unsigned char data)
{
	unsigned char highaddressbits = 0;
	
	if ((address & 0x1000) == 0x1000)
	 highaddressbits += 0x10; //A12 set
	
	if ((address & 0xC000) == 0xC000)
	 highaddressbits += 0x20; //A15 and A14 set
	
	if ((highaddressbits == 0) || ((address & 0x2800) > 0) )
	 return 0; //6522 (VIA) not selected, or invalid address
	
	//Sets highest 2 bits per result of A15 AND A14, as well as A12 value. Result is ORed with lowest four bits of address.
	//Shifted to position on 32bit bus
	unsigned long shortaddress = ((address & 0x0F) | highaddressbits) << 7;
	
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFFFFF); //LATCH EN Low  R/#W Low
	// Data Output Mode:
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL1/4), writeconfig_GPFSEL1);		// GPIO Pins 10-13 = OUTPUT| 14-15 = TTY | 16-19 = OUTPUT
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL2/4), 0x8249);			// GPIO Pins 20-23 = OUTPUT | 24 = INPUT | 25    = OUTPUT | 26-29 = INPUT

	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPSET0/4), (data << 16) | shortaddress ); // Data on Data Bus and Address on Address Bus

	bcm2835_gpio_set (25); //LATCH EN High
	
#ifdef USE_EDR
	// Detect RDY rising edge with EDR:
	bcm2835_gpio_ren (24); //Enable detection of RDY (#OE) rising edge.
	bcm2835_gpio_set_eds (24); //Clear Event Detection Register
#endif
	return 1;
}

// Detect whether write operation has completed, if so returns 1, else returns 0.
// If loop == 1, does not return until interrupt is detected.
char vectrexwrite_end (char loop)
{
	char status;
#ifdef USE_EDR
	do
	 {
	  status = bcm2835_gpio_eds (24);
	 }
	while (!status && loop); // Loop until RDY goes Low-to-High
	
	if (loop)
	 bcm2835_gpio_clr_ren (24); // Disable rising edge detection

#else
	// Poll RDY for LOW to HIGH transistion:
	if (loop)
	{
	 do
	 {
	  status = bcm2835_gpio_lev (24);
	 }
	 while (status); // Loop until RDY goes Low
	}
	
	do
	{
	 status = bcm2835_gpio_lev (24);
	}
	while (!status && loop); // Loop until RDY goes High
#endif
	
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFFFFF); //LATCH EN Low IRQ LATCH Low  R/#W Low
	bcm2835_gpio_set (13); // R/#W High
	return status;
}


// Write a byte to the Vectrex at the specified address.
// Returns after write is completed.
void vectrexwrite (unsigned int address, unsigned char data)
{
	if (vectrexwrite_begin (address, data))
	 vectrexwrite_end (1);
}

//Uses IRQ LATCH instead of LATCH EN output, causing the write to begin immediately after an IRQ signal is received from the 6522
// Begin writing a byte to the Vectrex
//Returns 1 if address is valid, otherwise 0.
char vectrexwrite_begin_on_irq (unsigned int address, unsigned char data)
{
	unsigned char highaddressbits = 0;
	
	if ((address & 0x1000) == 0x1000)
	 highaddressbits += 0x10; //A12 set
	
	if ((address & 0xC000) == 0xC000)
	 highaddressbits += 0x20; //A15 and A14 set
	
	if ((highaddressbits == 0) || ((address & 0x2800) > 0) )
	 return 0; //6522 (VIA) not selected, or invalid address
	
	//Sets highest 2 bits per result of A15 AND A14, as well as A12 value. Result is ORed with lowest four bits of address.
	//Shifted to position on 32bit bus
	unsigned long shortaddress = ((address & 0x0F) | highaddressbits) << 7;
	
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFFFFF); //LATCH EN Low  R/#W Low
	// Data Output Mode:
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL1/4), writeconfig_GPFSEL1);		// GPIO Pins 10-13 = OUTPUT| 14-15 = TTY | 16-19 = OUTPUT
	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL2/4), 0x8249);			// GPIO Pins 20-23 = OUTPUT | 24 = INPUT | 25    = OUTPUT | 26-29 = INPUT

	bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPSET0/4), (data << 16) | shortaddress ); // Data on Data Bus and Address on Address Bus

	bcm2835_gpio_set (6); //IRQ LATCH High
	
#ifdef USE_EDR
	// Detect RDY rising edge with EDR:
	bcm2835_gpio_ren (24); //Enable detection of RDY (#OE) rising edge.
	bcm2835_gpio_set_eds (24); //Clear Event Detection Register
#endif
	return 1;
}

// Write a byte to the Vectrex at the specified address after #IRQ is asserted.
// Returns when written.
void vectrexwrite_on_irq (unsigned int address, unsigned char data)
{
	if (vectrexwrite_begin_on_irq (address, data))
	 vectrexwrite_end (1);
}

//Enable detection of Interrupt Request signal from the Vectrex
void vectrexinterrupt_begin (void)
{
#ifdef USE_EDR
	 bcm2835_gpio_hen (26); // Detect pin High
	 bcm2835_gpio_set_eds (26); //Clear Event Detection Register
#endif
}

/* Checks whether #IRQ signal was detected, if so returns 1, else returns 0.
*  If loop == 1, does not return until interrupt is detected.
*  Optionally reads Interrupt Flag Register, and only returns after specified interrupt is set.
*  If loop == 1, other interrupts than the one specified will be cleared automatically if they
*  are set to trigger #IRQ in the Interrupt Enable Register.
*
*  Values for "interrupt" are defined as VECTREX_VIA_IFR_* in pitrexio-gpio.h
*  If interrupt == 7 (VECTREX_VIA_IFR_IRQ_STATUS_FLAG), returns on any interrupt (doesn't check EFR).
*/
char vectrexinterrupt_end (char interrupt, char loop)
{
	char status = 0;
	unsigned char ier;

	if (interrupt != 7 && loop)
	 ier = vectrexread (VIA_int_enable);

	do
	{
	  if (interrupt != 7 && loop)
	   // Clear any other interrupts that are set in IER to trigger #IRQ (bits written "1" to IFR are cleared):
	   vectrexwrite (VIA_int_flags, ((0xFE << interrupt) | (0x7F >> (7 - interrupt))) & ier);
#ifdef USE_EDR
	 do
	  {
	   status = bcm2835_gpio_eds (26);
	   bcm2835_gpio_set_eds (26); //Clear Event Detection Register
	  }
	 while (!status && loop); // Loop until IRQ goes High
	 
	 if (loop)
	  bcm2835_gpio_clr_ren (24); // Disable rising edge detection
#else
	 do
	 {
	  status = bcm2835_gpio_lev (26);
	 }
	 while (!status && loop); // Loop until IRQ goes High
#endif
	}
	while (interrupt != 7 && !(vectrexread(VIA_int_flags) & (1 << interrupt)) && loop );
#ifdef USE_EDR
	bcm2835_gpio_clr_hen (26); // Disable High detection
#endif
	return status; // status == 1 if interrupt request signal received, else 0.
}
		

// Wait on interrupt from Vectrex
// Returns when Interrupt Request signal (#IRQ) detected as asserted.
void vectrexinterrupt (char interrupt)
{
	vectrexinterrupt_begin ();
	vectrexinterrupt_end (interrupt, 1);
}

/*
              Initialisation Functions
              ========================
 */

// Initialise GPIO for communication with the Vectrex
// Vectrex can be initialised after the GPIO if viaconfig == 1,
// which sets it to the same initialisation state as the Vectrex BIOS would.
// With viaconfig == 0, the VIA and PSG registers are unchanged.
int vectrexinit (char viaconfig)
{
	unsigned char psgaddress;
	isWriteMode = -1;
//	bcm2835_debug(1); //Set debugging mode (GPIO not set, but actions reported)

	int result = bcm2835_init();
	if (result)
	{
	 bcm2835_gpio_GPCLR0 =  (bcm2835_gpio + BCM2835_GPCLR0  /4);
	 bcm2835_gpio_GPFSEL1 = (bcm2835_gpio + BCM2835_GPFSEL1 /4);
	 bcm2835_gpio_GPFSEL2 = (bcm2835_gpio + BCM2835_GPFSEL2 /4);
	 bcm2835_gpio_GPSET0 =  (bcm2835_gpio + BCM2835_GPSET0  /4);
	 bcm2835_gpio_GPLEV0 =  (bcm2835_gpio + BCM2835_GPLEV0  /4);

     char uartdefault = 0;
#ifndef FREESTANDING
	/* In Linux, figure out which Pi model we're using from /proc/cpuinfo, if available.
	 * Use the corresponding UART configuration.
	 * See:
	 *  https://www.raspberrypi.org/documentation/hardware/raspberrypi/revision-codes/
	 *  https://elinux.org/RPi_HardwareHistory
	 */
  FILE *stream;
  static char index[8] = {"Revision"};
  char fwareid[16];
  unsigned char indexcharno;
  unsigned char offsetcharno;
  unsigned char currchar;

  stream = fopen("/proc/cpuinfo","rb");
  if(stream == NULL)
  {
   printf("Failed to open /proc/cpuinfo.\n");
  }
  else
  {
   indexcharno = 0;  // compare string index[n] offset
   do
    {
     currchar = getc(stream);

     if (currchar == index[indexcharno])
     {
      indexcharno++;
     }
     else
     {
      indexcharno = 0;
      if (currchar == EOF)
       break;
     }

    }while(indexcharno < 8);

    do
    {
     currchar = getc(stream);
    }while (currchar == ' '
            || currchar == ':'
            || currchar == '\t');

   for (indexcharno = 0; currchar != '\n'; indexcharno++)
   {
    if (currchar != EOF)
     fwareid[indexcharno] = currchar;
    else
     break;

    currchar = getc(stream);
   }
  fclose(stream);
  }
  
  if (indexcharno < 4) // Revision ID should be longer than 3 characters.
  {
#ifdef UART0
   uartdefault = PIZERO_GPIO_CONFIG;
   printf("Pi model not found, defaulting to Raspberry Pi Zero config.\n");
#else
   uartdefault = PIZEROW_GPIO_CONFIG;
   printf("Pi model not found, defaulting to Raspberry Pi Zero W config.\n");
#endif
  }
  else
  {
   fwareid[indexcharno] = '\0';

   //If firmware ID starts with "1000", this just means that the Pi has been over-volted.
   //Remove leading "1000":
   if (fwareid[0] == '1')
   {
    for (offsetcharno=1; fwareid[offsetcharno] == '0' && offsetcharno < 4; offsetcharno++)
	 ;
	if (offsetcharno == 4)
	{
	 indexcharno = 0;
	 do
	 {
	  fwareid[indexcharno++] = fwareid[offsetcharno++];
	 }while (fwareid[offsetcharno] != '\0');
	}
   }
  
   printf("Pi revision ID is \"%s\", so it must be a... ",fwareid);
   if (strcmp(fwareid,"Beta") == 0)
   {
    printf("Raspberry Pi B Beta (wow!), 256MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0002") == 0
            || strcmp(fwareid,"0003") == 0)
   {
    printf("Raspberry Pi B V1.0, 256MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0004") == 0
            || strcmp(fwareid,"0005") == 0
			|| strcmp(fwareid,"0006") == 0)
   {
    printf("Raspberry Pi B V2.0, 256MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0007") == 0
            || strcmp(fwareid,"0008") == 0
			|| strcmp(fwareid,"0009") == 0)
   {
    printf("Raspberry Pi A V2.0, 256MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"000d") == 0
            || strcmp(fwareid,"000e") == 0
			|| strcmp(fwareid,"000f") == 0)
   {
    printf("Raspberry Pi B V2.0, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0010") == 0)
   {
    printf("Raspberry Pi B+ V1.0, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0011") == 0)
   {
    printf("Raspberry Pi Compute Module 1 V1.0, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0012") == 0)
   {
    printf("Raspberry Pi A+ V1.1, 256MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0013") == 0)
   {
    printf("Raspberry Pi B+ V1.2, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0014") == 0)
   {
    printf("Raspberry Pi Compute Module 1 V1.0, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"0015") == 0)
   {
    printf("Raspberry Pi A+ V1.1, 256MB/512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a01040") == 0)
   {
    printf("Raspberry Pi 2 Model B V1.0, 1GB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a01041") == 0
            || strcmp(fwareid,"a21041") == 0)
   {
    printf("Raspberry Pi 2 Model B V1.1, 1GB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a22042") == 0)
   {
    printf("Raspberry Pi 2 Model B (with BCM2837) V1.2, 1GB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"900021") == 0)
   {
    printf("Raspberry Pi Model A+ V1.1, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"900032") == 0)
   {
    printf("Raspberry Pi Model B+ V1.2, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"900092") == 0
            || strcmp(fwareid,"920092") == 0)
   {
    printf("Raspberry Pi Zero V1.2, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"900093") == 0
            || strcmp(fwareid,"920093") == 0)
   {
    printf("Raspberry Pi Zero V1.3, 512MB\n");
	uartdefault = PIZERO_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"9000c1") == 0)
   {
    printf("Raspberry Pi Zero W V1.1, 512MB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a02082") == 0)
   {
    printf("Raspberry Pi 3 Model B V1.2, 1GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a020a0") == 0)
   {
    printf("Raspberry Pi Compute Module 3 (and CM3 Lite) V1.0, 1GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a22082") == 0
            || strcmp(fwareid,"a32082") == 0)
   {
    printf("Raspberry Pi 3 Model B V1.2, 1GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a020d3") == 0)
   {
    printf("Raspberry Pi 3 Model B+ V1.3, 1GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"9020e0") == 0)
   {
    printf("Raspberry Pi 3 Model A+ V1.3, 512MB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a02100") == 0)
   {
    printf("Raspberry Pi Compute Module 3+ V1.0, 1GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"a03111") == 0)
   {
    printf("Raspberry Pi 4 Model B V1.1, 1GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"b03111") == 0)
   {
    printf("Raspberry Pi 4 Model B V1.1, 2GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"b03112") == 0)
   {
    printf("Raspberry Pi 4 Model B V1.2, 2GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"c03111") == 0)
   {
    printf("Raspberry Pi 4 Model B V1.1, 4GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"c03112") == 0)
   {
    printf("Raspberry Pi 4 Model B V1.2, 4GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else if (strcmp(fwareid,"d03114") == 0)
   {
    printf("Raspberry Pi 4 Model B V1.4, 8GB\n");
	uartdefault = PIZEROW_GPIO_CONFIG;
   }
   else
   {
#ifdef UART0
    uartdefault = PIZERO_GPIO_CONFIG;
    printf("sorry, never seen one of those before. Defaulting to Raspberry Pi Zero config.\n");
#else
	uartdefault = PIZEROW_GPIO_CONFIG;
    printf("sorry, never seen one of those before. Defaulting to Raspberry Pi Zero W config.\n");
#endif
   }
  }

#else
	// For Bare-Metal, assume Raspberry Pi Zero W config. unless UART0 is defined
#ifdef UART0
   uartdefault = PIZERO_GPIO_CONFIG;
#else
   uartdefault = PIZEROW_GPIO_CONFIG;
#endif
#endif

   //Set UART defaults
   //For ALT0 (UART0) on 14-15, use 0x24249. For ALT5 (UART1), use 0x12249.
   if (uartdefault == PIZERO_GPIO_CONFIG)
   {
    //Use ALT0 (UART0)
    readconfig_GPFSEL1=0x24249;
    writeconfig_GPFSEL1=0x9264249;
   }
   else if (uartdefault == PIZEROW_GPIO_CONFIG)
   {
    //Use ALT5 (UART1)
    readconfig_GPFSEL1=0x12249;
    writeconfig_GPFSEL1=0x9252249;
   }


	 //Set Initial State (ideally set to configured defaults at power-on):
	 //NOTE: This will disable other functions of these GPIO pins (except for the serial terminal on the UART)
	  //Input/Output Modes:
	 bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL0/4), 0x9240000);	// GPIO Pins 0-5   = INPUT | 6-9   = OUTPUT
	 bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL1/4), readconfig_GPFSEL1);		// GPIO Pins 10-13 = OUTPUT| 14-15 = TTY | 16-19 = INPUT
	 bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPFSEL2/4), 0x8000);		// GPIO Pins 20-24 = INPUT | 25    = OUTPUT | 26-29 = INPUT
	  //Output Levels:
	 bcm2835_peri_write ((bcm2835_gpio + BCM2835_GPCLR0/4), 0xFFFFDFFF);	//Set all outputs Low except R/#W
	 bcm2835_gpio_set_pud (27, BCM2835_GPIO_PUD_UP);  // Enable Pull-Up on #HALT
	 bcm2835_gpio_set_pud (24, BCM2835_GPIO_PUD_OFF); // Disable PU/D on RDY
	 bcm2835_gpio_set (13); // R/#W High
	 
	 //Check that Vectrex CPU is halted.
	 if (bcm2835_gpio_lev (27))
	  result = VECTREX_HALT_NOT_ASSERTED;
	 else
	 {
	  //Initialise 6522 (VIA) and AY3-8912 (PSG) in standard Vectrex start-up state (with reference to Vectrex BIOS "INITALL" routine):
	  if (viaconfig == 1)
	  {
	   vectrexwrite (VIA_DDR_b, 0x9F); // Set all direction bits to Outputs except COMPARE input and PB6 at Cartridge Port
	   vectrexwrite (VIA_DDR_a, 0xFF); // Set all direction bits to Outputs
	   vectrexwrite (VIA_aux_cntl, 0x98); //Shift Reg. Enabled, T1 PB7 Enabled
	   vectrexwrite (VIA_t1_cnt_lo, 0x7F); // default scale = 0x7f
	   vectrexwrite (VIA_t2_lo, 0x30);
	   vectrexwrite (VIA_t2_hi, 0x75); // T2 set to Refresh = MSEC20. Reset T2 Interrupt Flag and start Frame Timer.
 
	   // Clear Programmable Sound Generator (PSG) registers
	   //  Should be done by the hardware reset signal anyway. But monkey see, monkey do.
	   for (psgaddress = 0x0E; psgaddress > 0; psgaddress--)
	   {
	    vectrexwrite (VIA_port_a, psgaddress); // Address
	    vectrexwrite (VIA_port_b, 0x19);
	    vectrexwrite (VIA_port_b, 1); // PSG Address Latch
	    vectrexwrite (VIA_port_a, 0); // Data
	    vectrexwrite (VIA_port_b, 0x11);
	    vectrexwrite (VIA_port_b, 1); // PSG Data Latch
	   }
	  }
	  //Add alternative initialisation states here?
	 }
	}

	return result;
}

int vectrexclose (void)
{
	int result = bcm2835_close();
	return result;
}
