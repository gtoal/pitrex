#ifndef _DVGTIMER_H_
#define _DVGTIMER_H_
/*****************************************************************************
* ZVG timer routines.
*
* Author:       Zonn Moore
* Last Updated: 05/21/03
*
* History:
* 
* 100623 Updated for Windows by Steve Johnson
*
* (c) Copyright 2003-2004, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
 *
 *	here are the original functions.  I changed the
 *  names to things that made more sense to me:
 *
 *	extern void		tmrInit( void);				(tmrInit)
 *	extern void		tmrRestore( void);			NOT NEEDED
 *	extern void		tmrSetFrameRate( int fps);	(tmrSetFrameRate)
 *	extern uint		tmrTestFrame( void);		(tmrNumberFramesSkipped)
 *	extern uint		tmrWaitFrame( void);		(tmrWaitForFrame)
 *	extern Timer_t	tmrRead( void);				(tmrReadTimer)
 *	extern bool		tmrTestTicks				(tmrTestTicks)
 *	extern bool		tmrTestms					(tmrTestMillis)
 *	extern bool		tmrTestFrameCount			(tmrTestFrameCount)
 *
 *************************************************/

extern int				tmrInit(void);
extern void				tmrSetFrameRate(int);
extern unsigned int			tmrNumberFramesSkipped(void);
extern unsigned int			tmrWaitForFrame(void);
extern long long int			tmrReadTimer(void);
extern int				tmrTestTicks( long long int, int);
extern int				tmrTestMillis( long long int, int);
extern int				tmrTestFrameCount( unsigned int, unsigned int);
extern long long int			tmrGetTicksInFrame();

#ifdef __cplusplus
}
#endif

#endif
