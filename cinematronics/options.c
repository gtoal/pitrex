/*****************************************************************************
* Routines for dealing with the options in the INI file.							  *
*****************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
//#include	<fcntl.h>
#include	<ctype.h>
#include	<errno.h>

//#include	<memory.h>
//#include	<unistd.h>

#include <vectrex/vectrexInterface.h>
#include <vectrex/osWrapper.h>

#include        "cines.h"
#include	    "options.h"
#include        "inifile.h"

// globals setup by options

uchar OptMemSize = 1;		// 0=4k, 1=8k, 2=16k, 3=32k
uchar OptJMI = 1;		// 0=JEI, 1=JMI
uchar OptSpeed = 0;		// 0=Standard, 1=Full Speed
uchar OptSwitches = 0x80;	// Bit0 = Switch 1, etc.
uint OptInputs = 0xFFFFF;	// Initial Input values - probably should be FFF ???
char OptRomImages[8 * 128];	// Name of Rom images
char OptStateFile[129];		// Name of CPU state file

int OptWinXmin = 0;		// X Min size of Cine' window
int OptWinYmin = 0;		// Y Min size of Cine' window
int OptWinXmax = 1024;		// X Max size of Cine' window
int OptWinYmax = 768;		// Y Max size of Cine' window
uchar OptMonitor = 0;		// 0=BiLevel, 1=16Level, 2=64Level, 3=Color
uchar OptTwinkle = 8;		// 1-9 Set twinkle level
uchar OptRotate = 0;		// 0=Normal, 1=Rotate 90 degrees
uchar OptFlipX = 0;		// 0=Don't flip, 1=Flip X image
uchar OptFlipY = 0;		// 0=Don't flip, 1=Flip Y image

// won't be used:
uchar OptRedB = 100;		// RGB values for brightness
uchar OptGreenB = 100;
uchar OptBlueB = 100;
uchar OptRedC = 67;		// RGB values for contrast
uchar OptGreenC = 67;
uchar OptBlueC = 67;
uchar OptMouseType = 0;		// 0=None, 1=Tailgunner, 2=SpeedFreak,

// 3=BoxingBugs
uchar OptMouseValid = 0;	// mask of valid mouse keys
uint OptMouseSpeedX = 1024;	// mouse scaling
uint OptMouseSpeedY = 1024;	// mouse scaling

uchar OptKeyMap[(INMAPSIZ * MAXINPUT) + 128];	// Key map array memory
static KEYNAM_S KeyNames[MAXINPUT];	// Key name array
uchar OptMKeyMap[MOUSEINPUTS];	// Mouse key map memory
static int KeyIdx;		// index into KeyMapArray
static int NamIdx;		// index into KeyNames

// CPU routines
int rtnMemSize (char *);
int rtnJMI (char *);
int rtnSpeed (char *);
int rtnSwitches (char *);
int rtnInputs (char *);
int rtnRomImages (char *);
int rtnStateFile (char *);
int rtnMouse (char *);
int rtnMouseSpeedX (char *);
int rtnMouseSpeedY (char *);

struct INIOPTION_S IniCPU[] = {
   "MemSize", rtnMemSize,
   "JMI", rtnJMI,
   "Speed", rtnSpeed,
   "Switches", rtnSwitches,
   "Inputs", rtnInputs,
   "RomImages", rtnRomImages,
   "Mouse", rtnMouse,
   "MouseSpeedX", rtnMouseSpeedX,
   "MouseSpeedY", rtnMouseSpeedY,
//      "HighScores", rtnStateFile,
   0, 0
};

// Video routines
int rtnWinSize (char *);
int rtnMonitor (char *);
int rtnTwinkle (char *);
int rtnRotate (char *);
int rtnFlipX (char *);
int rtnFlipY (char *);
int rtnBrightness (char *);
int rtnContrast (char *);

struct INIOPTION_S IniVideo[] = {
   "WinSize", rtnWinSize,
   "Monitor", rtnMonitor,
   "Twinkle", rtnTwinkle,
   "Rotate", rtnRotate,
   "FlipX", rtnFlipX,
   "FlipY", rtnFlipY,
   "Brightness", rtnBrightness,
   "Contrast", rtnContrast,
   0, 0
};
int filelength(FILE *fd)		// also used in inifile.c so not static
{
   int fsize;
   fseek (fd, 0, SEEK_END);
// my book says return 0 on success!
   fsize = ftell(fd); 
   (void) fseek (fd, 0, SEEK_SET);
   return fsize;
}

int setOption (struct INIOPTION_S *options)
{
   char *parm1, *parm2;
   int err;

   if ((err = findDualParams (&parm1, &parm2)) != iniErrOk)
      return (err);

   while (options->optName != 0) {
      if (strpicmp (options->optName, parm1, '=') == 0)
	 return (options->optRtn (parm2));

      options++;
   }

   pIniError ("Unknown option");
   return (iniErrOpt);
}

int setAllOptions (struct INIOPTION_S *options)
{
   int err;

   do {
      err = setOption (options);
   } while (err == iniErrOk);

   // out of options?

   if (err == iniErrPar)
      err = iniErrOk;		// if out of options, no error

   return (err);
}

int initKeyMap (void)
{
   // OptKeyMap = (uchar *) paraAdj (KeyMapArrayM); // place on 16 byte boundry

   // clear arrays
   memset (OptKeyMap, 0, INMAPSIZ * MAXINPUT);	// zero inputs
   memset (OptKeyMap + (INMAPSIZ * MAXINPUT), 0xFF, 128);	// no keys yet
   memset (KeyNames, 0, MAXINPUT * sizeof (KEYNAM_S));	// clear key names
   return (iniErrOk);
}

void freeKeyNames (void)
{
}

void freeKeyMap (void)
{
}

int checkMouse (void)
{
   return 0;
}

int initMKeyMap (void)
{
   if (OptMouseType != 0) {
      if (checkMouse ()) {
	 OptMouseType = 0;	// reset mouse driver
	 OptMouseValid = 0;
	 pIniError ("No mouse driver loaded");
	 return (iniErrMouse);	// no driver loaded
      }
      // up to MOUSEINPUTS keys allowed
      // OptMKeyMap = (uchar *) paraAdj (MKeyMapArrayM); // place on 16 byte
      // boundry

      // clear array
      memset (OptMKeyMap, 0, MOUSEINPUTS);	// zero table inputs
      return (iniErrOk);
   }
   return (iniErrOk);
}

void freeMKeyMap (void)
{
   OptMouseType = 0;
   OptMouseValid = 0;
}

int getInputMap (void)
{
   char *parm1, *parm2;
   int ii, err;
   unsigned long value;

   // point to both parameters

   err = findDualParams (&parm1, &parm2);	// point to input mapping

   if (err != iniErrOk)
      return (err);

   // copy the name of the input and the offset of the associated 'input'
   // mask table to the name array.

   strpcpy (KeyNames[NamIdx].keyName, parm1, '=');

   // check to see if name has already been defined

   for (ii = 0; ii < NamIdx; ii++) {
      if (strcasecmp (KeyNames[ii].keyName, KeyNames[NamIdx].keyName) == 0) {
	 pIniError ("Input already defined");
	 return (iniErrInpDef);
      }
   }

   KeyNames[NamIdx].inpTbl = KeyIdx >> 4;	// point to input table
   NamIdx++;			// point to next name

   // Build input mask table

   // get the word offset into the 'keyirq' input mask

   value = strtoul (parm2, &parm2, 16);	// get first value

   // place 32 bit value in table

   OptKeyMap[KeyIdx++] = (uchar) (value & 0xFF);
   OptKeyMap[KeyIdx++] = (uchar) ((value >> 8) & 0xFF);
   OptKeyMap[KeyIdx++] = (uchar) ((value >> 16) & 0xFF);
   OptKeyMap[KeyIdx++] = (uchar) ((value >> 24) & 0xFF);

   // get three more parameters

   for (ii = 0; ii < 3; ii++) {
      err = sksep (&parm2, ',');	// skip past ','

      if (err != iniErrOk) {
	 pIniError ("Too few parameters");
	 return (err);
      }
      value = strtoul (parm2, &parm2, 16);	// get next value

      // place 32 bit value in table

      OptKeyMap[KeyIdx++] = (uchar) (value & 0xFF);
      OptKeyMap[KeyIdx++] = (uchar) ((value >> 8) & 0xFF);
      OptKeyMap[KeyIdx++] = (uchar) ((value >> 16) & 0xFF);
      OptKeyMap[KeyIdx++] = (uchar) ((value >> 24) & 0xFF);
   }
   return (iniErrOk);
}

int getAllInputs (void)
{
   int err;

   KeyIdx = 0;			// zero KeyMap index
   NamIdx = 0;			// zero Name Structure index

   do {
      err = getInputMap ();
   } while (err == iniErrOk && NamIdx < MAXINPUT);

   if (err == iniErrPar)	// out of parameters?
      err = iniErrOk;		// not an error

   if (NamIdx == MAXINPUT) {
      pIniError ("Too many INPUTS specified");
      err = iniErrInpMax;	// too many inputs
   }
   return (err);
}

int lookupKey (void)
{
   char *parm1, *parm2;
   int ii, err;
   unsigned long value;

   // point to both parameters

   err = findDualParams (&parm1, &parm2);	// get a key mapping

   if (err != iniErrOk)
      return (err);

   // find the key name in the KeyNames array

   for (ii = 0; ii < NamIdx; ii++)
      if (strpicmp (KeyNames[ii].keyName, parm1, '=') == 0)
	 break;

   if (ii == NamIdx) {
      pIniError ("Keyname not found in [Inputs] section");
      return (iniErrKeyInp);
   }

   if (isdigit (parm2[0])) {
      // get the word offset into the 'keyirq' input mask

      value = strtoul (parm2, &parm2, 16);	// get first value

      if (value > 127) {
	 pIniError ("\nScan code out of range (0-127) in [KeyMapping] section");
	 return (iniErrKeyScan);
      }
      // point to proper input table

      OptKeyMap[(MAXINPUT * INMAPSIZ) + (int) value] = KeyNames[ii].inpTbl;
   } else if (OptMouseType != 0) {
      if (strpicmp ("MB1", parm2, '\0') == 0) {
	 OptMKeyMap[0] = KeyNames[ii].inpTbl;
	 OptMouseValid |= 0x01;
      }

      else if (strpicmp ("MB2", parm2, '\0') == 0) {
	 OptMKeyMap[1] = KeyNames[ii].inpTbl;
	 OptMouseValid |= 0x02;
      }

      else if (strpicmp ("MB3", parm2, '\0') == 0) {
	 OptMKeyMap[2] = KeyNames[ii].inpTbl;
	 OptMouseValid |= 0x04;
      }

      else {
	 pIniError ("Unknown keyname in [KeyMapping] section");
	 return (iniErrKeyScan);
      }
   } else {
      pIniError ("Unknown keyname in [KeyMapping] section");
      return (iniErrKeyScan);
   }
   return (iniErrOk);
}

int lookupAllKeys (void)
{
   int err, count;

   do {
      err = lookupKey ();
   } while (err == iniErrOk);

   if (err == iniErrPar)	// out of parameters?
      err = iniErrOk;		// not an error

   return (err);
}

// Returns 0=No, 1=Yes, 2=Unknown

int isYes (char *parm)
{
   int parmLen;

   parmLen = strplen (parm, '\0');

   if (parmLen == 1 && toupper (*parm) == 'N')
      return (0);		// No

   if (parmLen == 1 && toupper (*parm) == 'Y')
      return (1);		// Yes

   if (strpicmp ("No", parm, '\0') == 0)
      return (0);		// No

   if (strpicmp ("Yes", parm, '\0') == 0)
      return (1);		// Yes

   pIniError ("Must be: YES or NO");
   return (2);			// Unknown
}

/*****************************************************************************
 * CPU routines - these just vet param ranges and don't actually act on them *
 *****************************************************************************/
int rtnMemSize (char *parm)
{
   if (strpicmp ("4K", parm, '\0') == 0)
      OptMemSize = 0;

   else if (strpicmp ("8K", parm, '\0') == 0)
      OptMemSize = 1;

   else if (strpicmp ("16K", parm, '\0') == 0)
      OptMemSize = 2;

   else if (strpicmp ("32K", parm, '\0') == 0)
      OptMemSize = 3;

   else {
      pIniError ("Must be: 4K, 8K, 16K or 32K");
      return (iniErrOpt);	// unknown parameter
   }

   return (iniErrOk);
}

int rtnJMI (char *parm)
{
   if ((OptJMI = isYes (parm)) > 1)
      return (iniErrOpt);	// unknown parameter
   return (iniErrOk);
}

int rtnSpeed (char *parm)
{
   if (strpicmp ("Standard", parm, '\0') == 0)
      OptSpeed = 0;
   else if (strpicmp ("Full", parm, '\0') == 0)
      OptSpeed = 1;
   else {
      pIniError ("Must be: Standard or Full");
      return (iniErrOpt);	// unknown parameter
   }
   return (iniErrOk);
}

int rtnSwitches (char *parm)
{
   static uchar switchOrder[] = { 0x40, 0x02, 0x04, 0x08, 0x01, 0x20, 0x10
   };

   uchar ii;

   if (strplen (parm, '\0') != 7) {
      pIniError ("Must be: Seven digits of 0 or 1");
      return (iniErrOpt);	// only 7 switches allowed
   }

   OptSwitches = 0x80;		// always reset Coin switch

   for (ii = 0; ii < 7; ii++) {
      if (*parm == '0')
	 OptSwitches |= switchOrder[ii];
      else if (*parm != '1') {
	 pIniError ("Must be: Seven digits of 0 or 1");
	 return (iniErrOpt);
      }
      parm++;
   }
   return (iniErrOk);
}

int rtnInputs (char *parm)
{
   ulong tempValue;

   tempValue = strtoul (parm, &parm, 16);	// read parameter
   if (tempValue > 65535L) {
      pIniError ("Must be between 0000 and FFFF hexidecimal");
      return (iniErrOpt);
   }
   OptInputs = (uint) tempValue;
   return (iniErrOk);
}
int rtnRomImages (char *parm)
{
   OptRomImages[0]=0;
   strpcpy (OptRomImages, parm, '\0');
   return (iniErrOk);
}

int rtnMouse (char *parm)
{
   if (OptMouseType != 0)
      freeMKeyMap ();

   if (strpicmp ("TAILGUNNER", parm, '\0') == 0)
      OptMouseType = 1;

   else if (strpicmp ("SPEEDFREAK", parm, '\0') == 0)
      OptMouseType = 2;

   else if (strpicmp ("BOXINGBUGS", parm, '\0') == 0)
      OptMouseType = 3;

   else if (strpicmp ("UNUSED", parm, '\0') != 0) {
      pIniError ("\n Must be set to: UNUSED, TAILGUNNER, SPEEDFREAK or BOXINGBUGS");
      return (iniErrOpt);	// unknown parameter
   }

   return (initMKeyMap ());
}

int rtnMouseSpeedX (char *parm)
{
   long tempValue;

   tempValue = strtol (parm, &parm, 10);	// read parameter
   if (tempValue < -32768L || tempValue > 32767L) {
      pIniError ("Speed setting must be between -32768 and 32767");
      return (iniErrOpt);
   }
   OptMouseSpeedX = (uint) tempValue;
   return (iniErrOk);
}

int rtnMouseSpeedY (char *parm)
{
   long tempValue;

   tempValue = strtol (parm, &parm, 10);	// read parameter
   if (tempValue < -32768L || tempValue > 32767L) {
      pIniError ("Speed setting must be between -32768 and 32767");
      return (iniErrOpt);
   }
   OptMouseSpeedY = (uint) tempValue;
   return (iniErrOk);
}

int rtnStateFile (char *parm)
{
   OptStateFile[0] = 0;
   strpcpy (OptStateFile, parm, '\0');
   return (iniErrOk);
}

/*****************************************************************************
* Video routines                                                             *
*****************************************************************************/
int rtnWinSize (char *parm)
{
   long value[4];
   int ii, err;

   value[0] = strtoul (parm, &parm, 10);	// get first value

   // get three more parameters

   for (ii = 1; ii < 4; ii++) {
      err = sksep (&parm, ',');	// skip past ','

      if (err != iniErrOk) {
	 pIniError ("Too few parameters");
	 return (err);
      }
      value[ii] = strtoul (parm, &parm, 10);	// get next value
   }

   if (value[0] < -100 || value[0] > 100 ||
       value[1] < -100 || value[1] > 100 ||
       value[2] < 924 || value[2] > 1124 ||
       value[3] < 668 || value[3] > 868) {
      pIniError ("Must be: -100 to 100, -100 to 100, 924 to 1124, 668 to 868");
      return (iniErrOpt);
   }
   OptWinXmin = (int) value[0];
   OptWinYmin = (int) value[1];
   OptWinXmax = (int) value[2];
   OptWinYmax = (int) value[3];
   return (iniErrOk);
}

int rtnMonitor (char *parm)
{
   if (strpicmp ("BiLevel", parm, '\0') == 0)
      OptMonitor = 0;

   else if (strpicmp ("16Level", parm, '\0') == 0)
      OptMonitor = 1;

   else if (strpicmp ("64Level", parm, '\0') == 0)
      OptMonitor = 2;

   else if (strpicmp ("Color", parm, '\0') == 0)
      OptMonitor = 3;

   else {
      pIniError ("Must be: BiLevel, 16Level, 64Level or Color");
      return (iniErrOpt);
   }
   return (iniErrOk);
}

int rtnTwinkle (char *parm)
{
   long tempValue;

   tempValue = strtol (parm, &parm, 10);

   if (tempValue < 0 || tempValue > 10) {
      pIniError ("Must be between 0 and 9");
      return (iniErrOpt);
   }
   OptTwinkle = (uchar) tempValue;
   return (iniErrOk);
}

int rtnRotate (char *parm)
{
   if ((OptRotate = isYes (parm)) > 1)
      return (iniErrOpt);	// unknown parameter
   return (iniErrOk);
}

int rtnFlipX (char *parm)
{
   if ((OptFlipX = isYes (parm)) > 1)
      return (iniErrOpt);	// unknown parameter
   return (iniErrOk);
}

int rtnFlipY (char *parm)
{
   if ((OptFlipY = isYes (parm)) > 1)
      return (iniErrOpt);	// unknown parameter
   return (iniErrOk);
}

int getRGB (char *parm, uchar * red, uchar * green, uchar * blue)
{
   long value[3];
   int ii, err;

   value[0] = strtoul (parm, &parm, 10);	// get first value

   // get three more parameters

   for (ii = 1; ii < 3; ii++) {
      err = sksep (&parm, ',');	// skip past ','

      if (err != iniErrOk) {
	 pIniError ("Too few parameters");
	 return (err);
      }
      value[ii] = strtoul (parm, &parm, 10);	// get next value
   }

   if (value[0] < 0 || value[0] > 100 ||
       value[1] < 0 || value[1] > 100 || value[2] < 0 || value[2] > 100) {
      pIniError ("All RGB values must be between 0 and 100 percent");
      return (iniErrOpt);
   }
   *red = (uchar) value[0];
   *green = (uchar) value[1];
   *blue = (uchar) value[2];
   return (iniErrOk);
}

int rtnBrightness (char *parm)
{
   return (getRGB (parm, &OptRedB, &OptGreenB, &OptBlueB));
}

int rtnContrast (char *parm)
{
   return (getRGB (parm, &OptRedC, &OptGreenC, &OptBlueC));
}

// Print "Section not found" error message

void pSectErr (char *ss)
{
   printf ("Error: No [%s] section found in configuration file.\n", ss);
}

// Print all printable comments in .INI file.

void pComments (void)
{
   char *iniPtr;

   // Start at top of .INI file and print all lines starting with a '>'

   iniPtr = startIniFile ();	// point to start of file

   while (!iseob (*iniPtr)) {
      if (*iniPtr == '>') {
	 iniPtr++;		// skip '>' character

	 // print everything up to end of line

	 while (!isreol (*iniPtr)) {
	    fputc (*iniPtr, stdout);
	    iniPtr++;
	 }
	 fputc ('\n', stdout);
      }
      // skip to start of next line

      skRealLine (&iniPtr);
   }
}

int execIniFile (void)
{
   int err;

   // do CPU options

   err = findSection ("[CPU]");

   if (err != iniErrOk) {
      pSectErr ("CPU");
      return (err);
   }

   err = setAllOptions (IniCPU);

   if (err != iniErrOk)
      return (err);

   // do Video options

   err = findSection ("[Video]");

   if (err != iniErrOk) {
      pSectErr ("Video");
      return (err);
   }

   err = setAllOptions (IniVideo);

   if (err != iniErrOk)
      return (err);

   // initialize Mouse array.
   // Must be called after "CPU" and before "KEYBOARD" stuff

   err = initMKeyMap ();	// create mouse's keyboard table

   if (err != iniErrOk)
      return (err);

   // do Keyboard options

   err = initKeyMap ();		// initialize key map arrays

   if (err != iniErrOk)
      return (err);

   err = findSection ("[Inputs]");

   if (err != iniErrOk) {
      pSectErr ("Inputs");
      return (err);
   }

   err = getAllInputs ();	// read all the inputs

   if (err != iniErrOk)
      return (err);

#ifdef NEVER
   err = findSection ("[KeyMapping]");

   if (err != iniErrOk) {
      pSectErr ("KeyMapping");
      return (err);
   }
#endif

   err = lookupAllKeys ();

   freeKeyNames ();

   if (err != iniErrOk)
      return (err);

   return (err);
}

#define MAX_ROMS 8
#define MAX_ROM_SIZE 4096

unsigned char rom1[MAX_ROM_SIZE];
unsigned char rom2[MAX_ROM_SIZE];
unsigned char rom3[MAX_ROM_SIZE];
unsigned char rom4[MAX_ROM_SIZE];
unsigned char rom5[MAX_ROM_SIZE];
unsigned char rom6[MAX_ROM_SIZE];
unsigned char rom7[MAX_ROM_SIZE];
unsigned char rom8[MAX_ROM_SIZE];

unsigned char *romStorage[MAX_ROMS] = {
   rom1, rom2, rom3, rom4, rom5, rom6, rom7, rom8
};

#define MAX_ROM_NAME 128
unsigned char rom1name[MAX_ROM_NAME];
unsigned char rom2name[MAX_ROM_NAME];
unsigned char rom3name[MAX_ROM_NAME];
unsigned char rom4name[MAX_ROM_NAME];
unsigned char rom5name[MAX_ROM_NAME];
unsigned char rom6name[MAX_ROM_NAME];
unsigned char rom7name[MAX_ROM_NAME];
unsigned char rom8name[MAX_ROM_NAME];

unsigned char *romName[MAX_ROMS] = {
   rom1name, rom2name, rom3name, rom4name, rom5name, rom6name, rom7name,
    rom8name
};
static int currentROM = 0;

void assign_rom_names (char *roms, int count)
{
   int romseq = 0;
   char *s = roms;
   char *t;

   for (;;) {
      if (*s == '\0') {
	 // not enough rom names
      }
      t = romName[romseq];
      while (*s != '\0' && *s != ',') {
	 *t++ = *s++;
      }
      *t = '\0';
      //fprintf (stderr, "Rom[%d] = \"%s\"\n", romseq, romName[romseq]);
      romseq += 1;		// got another one OK.
      if (romseq == count) {
	 if (*s == ',')
	    s++;
	 // Good if no more filenames
	 if (*s == '\0')
	    return;
	 // ERROR - extra file names given
	 return;
      }
      if (*s == ',')
	 s++;
   }
}

/*****************************************************************************
 * Read ROM images.                                                          *
 *                                                                           *
 * Returns:                                                                  *
 *      0 - Ok                                                               *
 *      1 - Error, (error message already displayed)                         *
 *****************************************************************************/
uint readRoms (char *ListOfRoms, uchar * objCode, uchar OptMemSize)
{
   FILE *inFile;
   char *aFileName;
   uint nameLen;
   int romidx, objidx, ii;
   unsigned int sizeRead;
   uint romSize, romCount, extIdx, evnOdd;
   ulong tempSize;
   uchar *romBfr = 0;

   if (currentROM < MAX_ROMS)
      romBfr = romStorage[currentROM++];

   switch (OptMemSize) {
   case 0:			// 4k games
      romSize = 2048;
      romCount = 2;
      break;

   case 1:			// 8k games
      romSize = 2048;
      romCount = 4;
      break;

   case 2:			// 16k games
      romSize = 4096;
      romCount = 4;
      break;

   case 3:			// 32k game
      romSize = 4096;
      romCount = 8;
      break;
   }
   if (romBfr == 0) {
      fputs ("Not enough memory!", stderr);
      return (1);
   }

   assign_rom_names (ListOfRoms, romCount);

   evnOdd = 0;			// zero even odd index

   for (ii = 0; ii < romCount; ii++) {

      // NEW STYLE IS TO LIST INDIVIDUAL ROM NAMES IN ORDER
      aFileName = romName[ii];

      inFile = fopen (aFileName, "r");
      if (inFile == 0)
      {
	 printf ("readRoms: error opening file: \"%s\"\n", aFileName);
	 return (1);
      }
      tempSize = (int) filelength (inFile);	// get length of file BEFORE
                                                // ANY READING IS DONE

      if (tempSize != romSize)
      {
	fclose (inFile);
	 printf ("readRoms: file \"%s\" wrong size. (Must be %u bytes.)\n", aFileName, romSize);
	 return (1);
      }
      sizeRead = fread (romBfr, 1, romSize, inFile);
      if (sizeRead != romSize)
      {
        fclose (inFile);
        printf ("readRoms: error reading file: \"%s\"(%i,%i)\r\n", aFileName, sizeRead, romSize);
        return (1);
      }
      fclose (inFile);
      // move the ROM data into the Object code buffer

      for (objidx = ((ii >> 1) * romSize * 2) + evnOdd, romidx = 0;
	   romidx < romSize;
	   objidx += 2, romidx++) {
        printf("%s: loaded at %04x\n", aFileName, objidx);
	break;
      }

      for (objidx = ((ii >> 1) * romSize * 2) + evnOdd, romidx = 0;
	   romidx < romSize;
	   objidx += 2, romidx++) {
        //printf("objCode[%04x] = romBfr[%04x] = %02x;\n", objidx, romidx, romBfr[romidx]);
	objCode[objidx] = romBfr[romidx];
      }

      evnOdd ^= 0x0001;
   }
   return (0);
}
