/*fre****************************************************************************
* INIFILE.C                                                                  *
*                                                                            *
* Author:   Zonn Moore                                                       *
* Date:      6/30/97                                                         *
*                                                                            *
* Routines for parsing a '.ini' file.                                        *
*                                                                            *
* To use:                                                                    *
*                                                                            *
*   Call 'openIniFile( filename)', to open and read the .INI file.           *
*                                                                            *
*   Call 'findSection( section_name)' to located the section, and set the    *
*   internal line pointer to the first non-comment line.                     *
*                                                                            *
*   Call 'findDualParams( **parm1, **parm2)' to retrieve the address of      *
*   the two parameters on both sides of the '=' sign.  Each call will        *
*   return the addresses of the next two parameters in the section, until    *
*   a 'no-parameters'error is returned.                                      *
*                                                                            *
*****************************************************************************/
#include      <stdio.h>
#include      <stdlib.h>
#include      <string.h>
//#include      <fcntl.h>
#include      <ctype.h>


#include    "inifile.h"
#include    "options.h"		// for extern off_t filelength(int fd);
#include    <vectrex/osWrapper.h>

static char IniBfr[65536L];	// buffer that contains all of INI file
static char *BfrPtr = 0;	// point to current position in buffer
static char *StartLine = 0;	// point to start of current line in buffer
static char *ErrStart = 0;	// start of previous "in error" line
static char *Section = 0;	// pointer to current '[xxx]' section

/*****************************************************************************
* Is End of Buffer?                                                          *
*****************************************************************************/
int iseob (char cc)
{
   if (cc == '\0' || cc == '\x1A')
      return (1);		// end of buffer found
   else
      return (0);		// not end of line
}

/*****************************************************************************
* Is End of Section?                                                         *
*****************************************************************************/
int iseos (char cc)
{
   if (iseob (cc) || cc == '[')
      return (1);
   else
      return (0);
}

/*****************************************************************************
* Is End of Line?                                                            *
*****************************************************************************/
int iseol (char cc)
{
   if (iseos (cc) || cc == '\r' || cc == '\n' || cc == ';' || cc == '#'
       || cc == '>')
      return (1);		// end of line found
   else
      return (0);		// not end of line
}

/*****************************************************************************
* Is Real End of Line?                                                       *
*****************************************************************************/
int isreol (char cc)
{
   if (iseob (cc) || cc == '\r' || cc == '\n')
      return (1);
   else
      return (0);
}

/*****************************************************************************
* Is a white Space character?                                                *
*****************************************************************************/
int isspc (char cc)
{
   if (cc == ' ' || cc == '\t')
      return (1);
   return (0);
}

/*****************************************************************************
* Get the length of a parameter.                                             *
*                                                                            *
* Searches for whitespace, or the termination character 'cc'.                *
*****************************************************************************/
int strplen (const char *parm, char cc)
{
   int ii;

   ii = 0;

   while (!iseol (*parm) && !isspc (*parm) && *parm != cc) {
      parm++;
      ii++;
   }
   return (ii);
}

int strplen1 (const char *parm)
{
   return (strplen (parm, '='));
}

int strplen2 (const char *parm)
{
   return (strplen (parm, '\0'));
}

/*****************************************************************************
* Copy a parameter to a string.                                              *
*                                                                            *
* Copies for the length of the parameter, adds a '\0' to end of string.      *
*                                                                            *
* Returns length of string, 0 if nothing copied. (Still adds '\0')           *
*****************************************************************************/
int strpcpy (char *str, const char *parm, char cc)
{
   int ii;

   ii = strplen (parm, cc);	// get length of parameter

   if (ii != 0)
      memcpy (str, parm, ii);	// copy parameter

   str[ii] = '\0';		// add terminator
   return (ii);			// return length
}

/*****************************************************************************
* Compares a parameter and a string, using a case-insensitive comparison.    *
*****************************************************************************/
int strpicmp (const char *str, const char *parm, char cc)
{
   int lenStr;			// length of string
   int lenParm;			// length of parameter

   lenStr = strlen (str);	// get length of name
   lenParm = strplen (parm, cc);	// get length of parameter

   lenStr -= lenParm;		// are strings equal?

   if (lenStr != 0)
      return (lenStr);		// return inequality

   return (strncasecmp (str, parm, lenParm));
}

/*****************************************************************************
* Skip Space                                                                 *
*                                                                            *
* Move pointer pointed to by 'bfrptr' past all whitespaces                   *
*****************************************************************************/
void skspc (char **bfrptr)
{
   while (isspc (**bfrptr))
      (*bfrptr)++;		// skip white space characters
}

/*****************************************************************************
* Skip Seperator                                                             *
*                                                                            *
* Move pointed to pointer past 'cc', and trailing whitespace                 *
*                                                                            *
* Returns an error if seperation character not found.                        *
*****************************************************************************/
int sksep (char **bfrptr, char cc)
{
   skspc (bfrptr);		// skip white space

   // check if separator found

   if (**bfrptr == cc) {
      (*bfrptr)++;		// skip seperator
      skspc (bfrptr);		// skip white space
      return (iniErrOk);	// no error
   }
   return (iniErrSep);		// no seperator found
}

/*****************************************************************************
* Skip Parameter and Separator.                                              *
*                                                                            *
* Move pointed to pointer past parameter, 'cc' and trailing space.           *
*****************************************************************************/
int skprmsep (char **bfrptr, char cc)
{
   // skip past parameter
   (*bfrptr) += strplen (*bfrptr, cc);	// move pointer forward by len of
					// parm

   // skip parameter
   return (sksep (bfrptr, cc));	// skip seperation char and whitespace
}

/*****************************************************************************
* Skip REAL line.                                                            *
*                                                                            *
* Move pointer past hardcoded line, regardless of what's on line.            *
* Sets a pointer to start of new line, then skip whitespace.                 *
*                                                                            *
* This routine is only interested in *real* ASCII end of line characters,    *
* and is used to skip to the start of the next ASCII line.                   *
*****************************************************************************/
void skRealLine (char **bfrptr)
{
   // move to real end of line

   while (!isreol (**bfrptr))
      (*bfrptr)++;

   // skip all *real* end of line characters

   while (!iseob (**bfrptr) && isreol (**bfrptr))
      (*bfrptr)++;

   StartLine = *bfrptr;		// keep track of start of line
   skspc (bfrptr);		// skip white space at start of next line
}

/*****************************************************************************
* Skip to next logical line.                                                 *
*                                                                            *
* Skips current line, and all 'comment' lines.                               *
*                                                                            *
* This routine skips to the next ASCII encoded line, then checks to see if   *
* line is blank, or contains only comments.  If so, then ignores the lines   *
* and continues until a non-comment line is found.                           *
*                                                                            *
* If called at the end of a section block, this will skip past the end of    *
* the section.                                                               *
*****************************************************************************/
void skLLine (char **bfrptr)
{
   do {
      skRealLine (bfrptr);	// skip to next real line
   } while (!iseos (**bfrptr) && iseol (**bfrptr));
}

/*****************************************************************************
* Skip line.                                                                 *
*                                                                            *
* Skip current line and all 'comment' lines, but won't go past end of a      *
* section.                                                                   *
*****************************************************************************/
void skLine (char **bfrptr)
{
   if (!iseos (**bfrptr))
      skLLine (bfrptr);
}

/*****************************************************************************
* Print an .INI file error message                                           *
*                                                                            *
* Prints the error message supplied by '*errstr', followed by the current    *
* Section, and the current command line inside the section.                  *
*****************************************************************************/
void pIniError (const char *errstr)
{
   printf ("Error in configuration file: %s.\n\n", errstr);

   // print offending section

   while (*Section != ']') {
      fputc (*Section, stderr);
      Section++;
   }
   fputs ("]\n", stderr);

   // print offending line

   while (*ErrStart != '\0' && *ErrStart != '\r' && *ErrStart != '\n') {
      fputc (*ErrStart, stderr);
      ErrStart++;
   }
   fputc ('\n', stderr);
}


/******************************************************************************
* Opens the INI file, and reads it into the internal buffer.                  *
*                                                                             *
* The entire file must be able to fit into memory, and must be less than 64k. *
******************************************************************************/
int openIniFile (const char *filename)
{
   FILE *inFile;
   unsigned int readSize;
   long tempSize;

   // open INI file
   inFile = fopen (filename, "r");
   if (inFile == 0)
   {
      printf ("openIniFile: could not open file \"%s\"\n", filename);
      return (iniErrOpen);
   }

   tempSize = filelength (inFile);	// get length of file

   if (tempSize > 65534L)
   {
      fclose (inFile);
      printf ("Error: File \"%s\" is greater than 65534 bytes.\n", filename);
      return (iniErrOpen);
   }

   readSize = fread(IniBfr, 1, (unsigned int) tempSize, inFile);

   if (readSize != tempSize)
   {
      printf ("Error: While reading file \"%s\"(%i/%i)\r\n", filename, readSize, tempSize);
      fclose (inFile);
      // IniBfr = 0;
      return (iniErrOpen);
   }
   fclose (inFile);

   IniBfr[(unsigned int) tempSize] = '\x1A';	// add an EOF marker
   return (iniErrOk);
}

/*****************************************************************************
* Close INI file and free memory.                                            *
*****************************************************************************/
int closeIniFile (void)
{
   return (iniErrOk);
}

/*****************************************************************************
* Return a pointer to start of .INI buffer.                                  *
*****************************************************************************/
char *startIniFile (void)
{
   return (IniBfr);
}

/*****************************************************************************
* Find the requested '[section]' by starting at the top of the buffer and    *
* searching the entire buffer.                                               *
*                                                                            *
* After section is found, pointer will be left pointing to the start of the  *
* next logical line.  A pointer to the section and a pointer to the          *
* start of the logical line will be saved -- to allow error processing.      *
*****************************************************************************/
int findSection (const char *section)
{
   int len;

   if (IniBfr == 0) {
      fputs ("\nNo open INI file\n", stderr);
      return (iniErrOpen);
   }

   BfrPtr = IniBfr;		// point to start of buffer
   StartLine = BfrPtr;		// keep track of line
   skspc (&BfrPtr);		// point to start of first line

   len = strlen (section);	// get length of section name

   while (!iseob (*BfrPtr))	// search until end of buffer
   {
      if (*BfrPtr == '[') {
	 if (strncasecmp (BfrPtr, section, len) == 0) {
	    Section = BfrPtr;	// point to section
	    skLLine (&BfrPtr);	// skip to start of options
	    return (iniErrOk);	// ok, string found
	 }
      }
      skLLine (&BfrPtr);	// skip to start of next line
   }
   return (iniErrSect);		// no section found
}

/*****************************************************************************
* Find, and return pointers to parameters on both sides of '='               *
*                                                                            *
* Returns pointers to both parameters, and then skips ahead to the next      *
* logical line, but will not skip past end of section.                       *
*****************************************************************************/
int findDualParams (char **parm1, char **parm2)
{
   int err;

   ErrStart = StartLine;	// save incase of error
   skspc (&BfrPtr);		// skip leading spaces

   if (iseol (*BfrPtr))
      return (iniErrPar);	// params not found (empty line)

   *parm1 = BfrPtr;		// point to 1st parameter

   // skip past parameter, '=' and point to parameter 2

   err = skprmsep (&BfrPtr, '=');

   if (err != iniErrOk) {
      pIniError ("Missing '='");
      return (err);		// seperator not found
   }

   if (iseol (*BfrPtr)) {
      pIniError ("Missing parameter after '='");
      return (iniErrPar2);
   }

   *parm2 = BfrPtr;
   skLine (&BfrPtr);		// skip to next line
   return (iniErrOk);		// no error, parameter(s) found
}
