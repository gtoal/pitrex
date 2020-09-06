// PiTrex I/O functions for initialising Raspberry Pi GPIO, and Reading/Writing from/to Vectrex
// Kevin Koster, 2020.
// V. 1.0

#ifndef PITREXIO_GPIO_H
#define PITREXIO_GPIO_H

#define VECTREX_HALT_NOT_ASSERTED						-1

//VIA Interrupt Flag Register Bits:
#define VECTREX_VIA_IFR_CA2_INTERRUPT_FLAG				0
#define VECTREX_VIA_IFR_CA1_INTERRUPT_FLAG				1
#define VECTREX_VIA_IFR_SHIFT_REGISTER_INTERRUPT_FLAG	2
#define VECTREX_VIA_IFR_CB2_INTERRUPT_FLAG				3
#define VECTREX_VIA_IFR_CB1_INTERRUPT_FLAG				4
#define VECTREX_VIA_IFR_TIMER_2_INTERRUPT_FLAG			5
#define VECTREX_VIA_IFR_TIMER_1_INTERRUPT_FLAG			6
#define VECTREX_VIA_IFR_IRQ_STATUS_FLAG					7

/*
   PiTrex Discrete Read & Write Functions, via Raspberry Pi GPIO:
   ==============================================================
 */

//Fast read from the Vectrex at the specified address
unsigned char vectrexread_short(unsigned int address);

//Fast write to the Vectrex at the specified address
//Returns after write is completed.
void vectrexwrite_short(unsigned int address, unsigned char data);

//Begin read operation
//Returns 1 if address is valid, otherwise 0.
char vectrexread_begin (unsigned int address);

//Return read byte from previously started read operation.
//Returns after operation completed if it hasn't already by the time that it is called.
unsigned char vectrexread_end (void);

//Return a byte read from the Vectrex at the specified address
unsigned char vectrexread (unsigned int address);

//Uses IRQ LATCH instead of LATCH EN output, causing the read to begin immediately after an IRQ signal is received from the 6522
//Begin read operation
//Returns 1 if address is valid, otherwise 0.
char vectrexread_begin_on_irq (unsigned int address);

// Read a byte from the Vectrex at the specified address after #IRQ asserted by the Vectrex
unsigned char vectrexread_on_irq (unsigned int address);

// Begin writing a byte to the Vectrex
//Returns 1 if address is valid, otherwise 0.
char vectrexwrite_begin (unsigned int address, unsigned char data);

// Detect whether write operation has completed, if so returns 1, else returns 0.
// If loop == 1, does not return until interrupt is detected.
char vectrexwrite_end (char loop);

// Write a byte to the Vectrex at the specified address
// Returns after write is completed.
void vectrexwrite (unsigned int address, unsigned char data);

//Uses IRQ LATCH instead of LATCH EN output, causing the write to begin immediately after an IRQ signal is received from the 6522
// Begin writing a byte to the Vectrex
//Returns 1 if address is valid, otherwise 0.
char vectrexwrite_begin_on_irq (unsigned int address, unsigned char data);

// Write a byte to the Vectrex at the specified address after #IRQ is asserted.
// Returns when written.
void vectrexwrite_on_irq (unsigned int address, unsigned char data);

//Enable detection of Interrupt Request signal from the Vectrex
void vectrexinterrupt_begin (void);

/* Checks whether #IRQ signal was detected, if so returns 1, else returns 0.
*  If loop == 1, does not return until interrupt is detected.
*  Optionally Read Interrupt Flag Register, and only return after specified interrupt is set.
*  If loop == 1, other interrupts than the one specified will be cleared automatically if they
*  are set to trigger #IRQ in the Interrupt Enable Register.
*
*  Values for "interrupt" are defined as VECTREX_VIA_IFR_* in pitrexio-gpio.h
*  If interrupt == 7 (VECTREX_VIA_IFR_IRQ_STATUS_FLAG), returns on any interrupt.
*/
char vectrexinterrupt_end (char interrupt, char loop);

// Wait until interrupt from Vectrex
void vectrexinterrupt (char interrupt);

// Initialise GPIO for communication with the Vectrex
// Vectrex can be initialised after the GPIO if viaconfig == 1,
// which sets it to the same initialisation state as the Vectrex BIOS would.
// With viaconfig == 0, the VIA and PSG registers are unchanged.
int vectrexinit (char viaconfig);

// End communication with the Vectrex and end claim on GPIO
int vectrexclose (void);

#endif /* PITREXIO_GPIO_H */
