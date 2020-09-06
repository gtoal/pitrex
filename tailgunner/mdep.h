#ifndef _MDEP_H_
#define	_MDEP_H_
extern UINT8 rom[0x2000];

extern UINT8 bFlipX;
extern UINT8 bFlipY;
extern UINT8 bSwapXY;
/*UINT8 bOverlay;*/

extern UINT16 ioSwitches;
extern UINT16 ioInputs;
extern UINT8 ioOutputs;		/* Where are these used??? */

extern INT16 JoyX;
extern INT16 JoyY;
extern UINT8 bNewFrame;

extern INT32 sdwGameXSize;
extern INT32 sdwGameYSize;
extern INT32 sdwXOffset;
extern INT32 sdwYOffset;

  /* C-CPU context information begins --  */
extern CINEWORD register_PC;	/* C-CPU registers; program counter */
extern /*register */ CINEWORD register_A;	/* A-Register (accumulator) */
extern /*register */ CINEWORD register_B;	/* B-Register (accumulator) */
extern CINEWORD /*CINEBYTE*/ register_I;	/* I-Register (last access RAM location) */
extern CINEWORD register_J;	/* J-Register (target address for JMP opcodes) */
extern CINEWORD /*CINEBYTE*/ register_P;	/* Page-Register (4 bits, shifts to high short nibble for code, hight byte nibble for ram) */
extern CINEWORD FromX;		/* X-Register (start of a vector) */
extern CINEWORD FromY;		/* Y-Register (start of a vector) */
extern CINEWORD register_T;	/* T-Register (vector draw length timer) */
extern CINEWORD flag_C;		/* C-CPU flags; carry. Is word sized, instead
				 * of CINEBYTE, so we can do direct assignment
				 * and then change to BYTE during inspection.
				 */

extern CINEWORD cmp_old;	/* last accumulator value */
extern CINEWORD cmp_new;	/* new accumulator value */
extern CINEWORD /*CINEBYTE*/ acc_a0;	/* bit0 of A-reg at last accumulator access */

extern CINESTATE state;		/* C-CPU state machine current state */
extern CINEWORD ram[256];	/* C-CPU ram (for all pages) */

extern CINEWORD vgColour;
extern CINEBYTE vgShiftLength;	/* number of shifts loaded into length reg */
extern int bailOut;
extern int ccpu_ICount;		/* */

  /* -- Context information ends. */

extern int ccpudebug;		/* default is off */

#ifndef TRUE
#define TRUE (1==1)
#endif 

#ifndef FALSE
#define FALSE (1!=1)
#endif 

#endif
