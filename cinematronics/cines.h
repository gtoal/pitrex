#ifndef CINES_H
#define CINES_H

#define uchar unsigned char
#define uint unsigned int

struct CPUSTATE {
        uint	ram[256];		// RAM data
	uint	accVal;			// Last Acc value
	uint	cmpVal;			// Compare value
	uint	pa0Cf;			// A0 bit & C-flg
	uint	eRegPC;			// PC-reg
	uint	eRegA;			// A-reg
	uint	eRegB;			// B-reg
	uint	eRegI;			// I-reg
	uint	eRegJ;			// J-reg
	uchar	eRegP;			// P-reg
	uchar	eCState;		// current state
};

#ifdef DEBUGGER

//#define	TRACE			0
//#define	WRITEADR		0

#define	DISBRK		0xFFFF		// indicates no break point set

#define	STATE_A		0
#define	STATE_A2	1
#define	STATE_B		2
#define	STATE_B2	3

#define	MAXINSLEN	2				// maximum instruction length

extern uchar	*ProgData;
extern uint	ramBank[256];			        // RAM registers

// Instruction cycle counters

extern uint	insCount;				// instruction counter

// CPU flags and temp storage

extern uint	accVal;					// last ACC
extern uint	cmpVal;					// new Acc

// Do not change order of next two labels

extern uchar	pa0;					// Bit zero of A-reg at last ACC access
extern uchar	cFlag;					// carry flag indicator

// Graphics data

extern uint	Xstart;
extern uint	Xend;
extern uint	Ystart;
extern uint	Yend;

// By saving the following, the state of the CPU can be saved

extern uint	ioOutputs;				// output values

extern uint	eRegPC;					// PC-reg
extern uint	eRegA;					// A-reg
extern uint	eRegB;					// B-reg
extern uint	eRegI;					// I-reg
extern uint	eRegJ;					// J-reg
extern uchar	eRegP;					// P-reg
extern uchar	eCState;					// current state

extern uchar	eMiFlg;					// minus flag
extern uchar	eLtFlg;					// less than flag
extern uchar	eEqFlg;					// equal flag
extern uchar	eCyFlg;					// carry flag
extern uchar	eA0Flg;					// PA0 flag
extern uchar	eBfFlg;					// B flip/flop flag

// Debugger data

extern uint	oRegPC;					// PC-reg
extern uint	oRegA;					// A-reg
extern uint	oRegB;					// B-reg
extern uint	oRegI;					// I-reg
extern uint	oRegJ;					// J-reg
extern uchar	oRegP;					// P-reg
extern uchar	oCState;					// current state
extern uchar	oMiFlg;					// minus flag
extern uchar	oLtFlg;					// less than flag
extern uchar	oEqFlg;					// equal flag
extern uchar	oCyFlg;					// carry flag
extern uchar	oA0Flg;					// PA0 flag
extern uchar	oBfFlg;					// B flip/flop flag

extern uint	eInsBrk0;				// break address 0
extern uint	eInsBrk1;				// break address 1
extern uint	eInsBrk2;				// break address 2
extern uint	eInsBrk3;				// break address 3
extern uint	eInsGo;					// "GO" break address

#ifdef TRACE
extern uint	tracePtr;				// trace pointer index
extern uint	traceBfr[16];			        // last 16 instructions
#endif

#ifdef WRITEADR
extern uint	writeAdr;				// write address breakpoint
#endif

extern int   cineExec(uint aInsCount);
extern uint *findBrkPt(uint adr);
extern uint *getAvlBrkPt(void);
extern uint  isAnyBrkPt(void);
extern uint decodeCmd(char *aCmdStr);
extern void hotkeys(int &cc);

#else /* NOT DEBUGGER */
extern int cineExec(void);
#endif

extern void cineSetGameName(char *name);
extern void cineSetJMI(int aJMIflag);
extern void cineSetMSize(int aMemSizeFlag);
extern void cineSetMonitor(int aMonitor);
extern void cineSetRate(uint aRefresh);
extern void cineSetSw(uint aSwitches, uint aInputs);
extern void cineInit(uchar *, uchar *);
extern void cineReset(void);
extern void vgSetMode(int mode);
extern void vgSetCineSize(int Xmin, int Ymin, int Xmax, int Ymax);
extern void vgSetMonitor(int aMonitor, int RB, int GB, int BB, int RC, int GC, int BC);
extern void vgSetTwinkle(int aTwinkle);
extern void vgSetRotateFlip(int rotate, int flipX, int flipY);
extern void vgInit(void);
extern void memfcs(uchar *block, uint count, uint *fcs);
extern void cineSetMouse(uint aMouseType, uint aMouseSpeedX, uint aMouseSpeedY, uint aKeyMask, uchar *aMouseSeg);

// Cine I/O stuff
//extern int checkMouse(void);

#endif /* CINES_H */
