
#define uchar unsigned char
#define uint unsigned int


#define	INMAPSIZ	16		// 16 bytes to map an input
#define	MAXINPUT	64		// maximum number of inputs allowed
#define	MOUSEINPUTS	3		// maximum number of mouse inputs allowed

typedef struct	INIOPTION_S
{	char	*optName;
	int	(*optRtn)(char *options);
} INIOPTION_S;

#define	KEYNAMSIZ	20

typedef struct	KEYNAM_S
{	char	keyName[KEYNAMSIZ+1];
	uchar	inpTbl;
} KEYNAM_S;

// globals setup by options

extern uchar	OptMemSize;			// 0=4k, 1=8k, 2=16k
extern uchar	OptJMI;				// 0=JEI, 1=JMI
extern uchar	OptSpeed;			// 0=Standard, 1=Space War, 2=Full
extern uchar	OptSwitches;			// Bit0 = Switch 1, etc.
extern uint	OptInputs;			// Initial Inputs
extern char	OptRomImages[8*128];		// Name of Rom images
extern char	OptCpuState[129];		// Name of CPU state file
extern int	OptWinXmin;			// X Min size of Cine' window
extern int	OptWinYmin;			// Y Min size of Cine' window
extern int	OptWinXmax;			// X Max size of Cine' window
extern int	OptWinYmax;			// Y Max size of Cine' window
extern uchar	OptMonitor;			// 0=BiLevel, 1=16Level, 2=64Level, 3=Color
extern uchar	OptTwinkle;			// Twinkle level
extern uchar	OptRotate;			// 0=Normal, 1=Rotate 90 degrees
extern uchar	OptFlipX;			// 0=Don't flip, 1=Flip X image
extern uchar	OptFlipY;			// 0=Don't flip, 1=Flip Y image
extern uchar	OptRedB;			// RGB values for brightness
extern uchar	OptGreenB;
extern uchar	OptBlueB;
extern uchar	OptRedC;			// RGB values for contrast
extern uchar	OptGreenC;
extern uchar	OptBlueC;

extern uchar	OptKeyMap[];			// Key map array
extern uchar	OptMKeyMap[];			// Key map array

extern uchar	OptMouseType;			// 0=None, 1=Tailgunner, 2=SpeedFreak, 3=BoxingBugs
extern uchar	OptMouseValid;			// mask of valid mouse keys
extern uint	OptMouseSpeedX;			// mouse scaling
extern uint	OptMouseSpeedY;			// mouse scaling

extern int	execIniFile(void);
extern uint	readRoms(char *aFileName, uchar *objCode, uchar OptMemSize);
extern void	pComments(void);

extern int filelength(FILE *fd);
