/***************************************************************************
 *
 * Retrocade - Video game emulator
 * Copyright 1998, The Retrocade group
 *
 * Permission to distribute the Retrocade executable *WITHOUT GAME ROMS* is
 * granted to all. It may not be sold without the express written permission
 * of the Retrocade development team or appointed representative.
 *
 * Source code is *NOT* distributable without the express written
 * permission of the Retrocade group.
 *
 * Cinematronics CPU header file
 *
 ***************************************************************************/

#ifndef _C_CPU_H_
#define	_C_CPU_H_


/*============================================================================================*

	HERE BEGINS THE MAME-SPECIFIC ADDITIONS TO THE CCPU INTERFACE.

 *============================================================================================*/

/* added these includes */
/*#include "osd_cpu.h"*/
/*#include "memory.h"*/

enum {
	CCPU_PC=1, CCPU_ACC, CCPU_CMP, CCPU_PA0, CCPU_CFLAG,
	CCPU_A, CCPU_B, CCPU_I, CCPU_J, CCPU_P, CCPU_CSTATE };

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	(!FALSE)
#endif

/* constants for configuring the system */
#define CCPU_PORT_IOSWITCHES   	0
#define CCPU_PORT_IOINPUTS     	1
#define CCPU_PORT_IOOUTPUTS    	2
#define CCPU_PORT_IN_JOYSTICKX 	3
#define CCPU_PORT_IN_JOYSTICKY 	4
#define CCPU_PORT_MAX          	5

#define CCPU_MEMSIZE_4K        	0
#define CCPU_MEMSIZE_8K        	1
#define CCPU_MEMSIZE_16K       	2
#define CCPU_MEMSIZE_32K       	3

#define CCPU_MONITOR_BILEV  	0
#define CCPU_MONITOR_16LEV  	1
#define CCPU_MONITOR_64LEV  	2
#define CCPU_MONITOR_WOWCOL 	3

/*============================================================================================*

	BELOW LIES THE CORE OF THE CCPU. THE CODE WAS KINDLY GIVEN TO MAME BY ZONN MOORE,
	JEFF MITCHELL, AND NEIL BRADLEY. I HAVE PRETTY HEAVILY CLEANED IT UP.

 *============================================================================================*/


/* Define new types for the c-cpu emulator */
typedef short unsigned int CINEWORD;      /* 12bits on the C-CPU */
typedef unsigned char      CINEBYTE;      /* 8 (or less) bits on the C-CPU */
typedef short signed int   CINESWORD;     /* 12bits on the C-CPU */
typedef signed char        CINESBYTE;     /* 8 (or less) bits on the C-CPU */

typedef U_LONG CINELONG;

typedef enum
{
	state_A = 0,
	state_AA,
	state_B,
	state_BB
} CINESTATE;                              /* current state */

/* NOTE: These MUST be in this order! */

struct scCpuStruct
{
	CINEWORD	accVal;				/* CCPU Accumulator value */
	CINEWORD	cmpVal;				/* Comparison value */
	CINEBYTE	pa0;
	CINEBYTE	cFlag;
	CINEWORD	eRegPC;
	CINEWORD	eRegA;
	CINEWORD	eRegB;
	CINEWORD	eRegI;
	CINEWORD	eRegJ;
	CINEBYTE	eRegP;
	CINESTATE	eCState;
};

typedef struct scCpuStruct CONTEXTCCPU;

#endif
