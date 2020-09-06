#ifndef INIFILE_H
#define INIFILE_H
/*****************************************************************************
* INIFILE.H
*
* Header file for INIFILE.CPP
*
* Author:       Zonn Moore
* Last Updated: 02/04/00
*
* (c) Copyright 2000, Zonn Moore.  All rights reserved.
*****************************************************************************/

enum INIERROR
{	iniErrOk,					// no error
	iniErrSect,					// no section found
	iniErrPar,					// no parameters left in section
	iniErrPar2,					// could not find parameter 2
	iniErrSep,					// no seperator found
	iniErrMem,					// out of memory
	iniErrOpen,					// error while opening/reading INI file
	iniErrInpMsk,				// no mask information given
	iniErrInpDef,				// input already defined
	iniErrInpMax,				// too many inputs in [Inputs]
	iniErrKeyInp,				// input not found in [KeyMap]
	iniErrKeyScan,				// error invalid scan code in [KeyMap]
	iniErrOpt,					// unknown option given
	iniErrMouse,				// no mouse driver loaded
};

extern int	iseob( char cc);
extern int	iseos( char cc);
extern int	iseol( char cc);
extern int	isreol( char cc);
extern int	isspc( char cc);
extern int	strplen( const char *bfrptr, char cc);
extern int	strpcpy( char *str, const char *parm, char cc);
extern int	strpicmp( const char *str, const char *parm, char cc);
extern void	skspc( char **bfrptr);
extern int	sksep( char **bfrptr, char cc);
extern int	skprmsep( char **bfrptr, char cc);
extern void	skRealLine( char **bfrptr);
extern void	skLLine( char **bfrptr);
extern void	skLine( char **bfrptr);
extern void	pIniError( const char *errstr);
extern int	openIniFile( const char *filename);
extern int	closeIniFile( void);
extern int	findSection( const char *section);
extern int	findDualParams( char **parm1, char **parm2);
extern char *startIniFile( void);
#endif
