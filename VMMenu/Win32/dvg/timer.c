/*****************************************************************************
* Timer routines.
*
* Author:  Zonn Moore
* Created: 11/06/02
*
* History:
*
* 100623 Updated by Steve Johnson for Windows
*
* (c) Copyright 2002-2010, Zektor, LLC.  All Rights Reserved.
*****************************************************************************/
#include	<time.h> //SCJ: for LINUX equivalent of Windows HRT.
#include	"timer.h"
#if defined(__WIN32__) || defined(_WIN32)
   #include <winbase.h>
#endif


static long long int	frameZeroTime, ticksInFrame, ticksPerMs, frequency;
static unsigned int		frameCount = 0;

/******************************************************
 *  We still need an init function to set the frequency
 ******************************************************/
int tmrInit(void)
{
	// LINUX timer is in nanoseconds (1/1000 ms)

	ticksPerMs = (long long int)1000;	 // ticks per millisecond
	frequency  = (long long int)1000000000; // ticks per second

	return 1;
}


/******************************************************
 *  This simply reads the High-Performance timer
 *	Needs to be passed a reference to a 64 bit integer (long long int)
 *	Return value is boolean success/failure
 ******************************************************/

long long int tmrReadTimer(void)
{
	long long int thetime;
	struct timespec time_now;

	#ifdef __WIN32__
	long int ms = GetTickCount(); 
	
    if (ms < 1000)
	{
        time_now.tv_sec = 0;
        time_now.tv_nsec = ms * 1000000;
    }
    else
	{
        time_now.tv_sec = ms / 1000;
        time_now.tv_nsec = (ms - time_now.tv_sec * 1000) * 1000000;
    }
	
	#else
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time_now);
	#endif
	thetime = (long long int)((time_now.tv_sec * frequency) + (time_now.tv_nsec));

	return thetime;
}


/******************************************************
 *  This sets several variables:
 *		- ticksInFrame: number of clock ticks in a frame
 *		- frameZeroTime: the datum for computing number
 *			of frames, based on clock ticks
 *		- ticksPerMs: number of ticks in a millisecond
 *
 *	parameters: int frames per second
 *	return value: boolean success/failure
 ******************************************************/
void tmrSetFrameRate(int fps)
{
	if (fps == 0)
		fps = 1;	// if zero, set to one for error

	// calculate the number of ticks needed for given frame rate
	//		o ticks in a frame = freq / fps

	ticksInFrame = frequency / (long long int)fps;
	frameZeroTime = tmrReadTimer();
}

/*****************************************************************************
* Test for end of frame.
*
* Returns zero if not end of frame.  If not zero, the returns the number of
* frames skipped, normally this is one, indicating one frame has passed.
*
* If the return value is greater than one, it indicates more than one frame
* has passed since last called.
*
* Resets the frame timer so that subsequent calls will return zero until the
* next frame time has passed.
*
* Called with:
*    NONE
*
* Returns with:
*    0 = Not end of frame. If not zero, then returns frames skipped.
*****************************************************************************/
unsigned int tmrNumberFramesSkipped(void)
{
	long long int	timer = 0;
	unsigned int 	frame;

	frame = frameCount;	// save the current Frame Count
	timer = tmrReadTimer();	// read current time

	// Calculate the number of frames that have passed
	while ((timer - frameZeroTime) >= ticksInFrame)
	{
		frameCount++;					// we passed a frame boundary
		frameZeroTime += ticksInFrame;  // add a frame width to the datum
	}
	return (frameCount - frame);		// return the # frames passed since last call
}

/*****************************************************************************
* Wait for end of frame.
*
* Returns when end of frame is reached.
*
* Called with:
*    NONE
*
* Returns with:
*    Number of frames that have passed since last called.
*****************************************************************************/
unsigned int tmrWaitForFrame(void)
{
	unsigned int	frames = 0;

	while ((frames = tmrNumberFramesSkipped()) == 0)
		;

	return (frames);
}

/*****************************************************************************
* Simple Accessor.  Gets the number of ticks in a frame.
*
*****************************************************************************/
long long int tmrGetTicksInFrame()
{
	return ticksInFrame;
}

/*****************************************************************************
* Test a timer using HP Timer ticks.
*
* Returns a non-zero value when 'ticks' number of HP Timer ticks have passed
* since the given timer value was read.
*
* Called with:
*    timer = Timer value obtained by a previous call to 'tmrReadTimer()'.
*    ticks = Number of 8254 ticks to test for.
*
* Returns with:
*    0 if timer has not timed out, not zero indicates timer has timed out.
*****************************************************************************/
int tmrTestTicks( long long int timer, int ticks)
{
	//long long int timeNow = 0;

	if ((tmrReadTimer() - timer) < (long long int)ticks)
		return 0;

	return 1;
}

/*****************************************************************************
* Test a timer using milliseconds.
*
* Returns a non-zero value when 'ms' number of milliseconds have passed
* since the given timer value was read.
*
* Called with:
*    timer = Timer value obtained by a previous call to 'tmrReadTimer()'.
*    ms    = Number of milliseconds to test for.
*
* Returns with:
*    0 if timer has not timed out, not zero indicates timer has timed out.
*****************************************************************************/
int tmrTestMillis( long long int timer, int ms)
{
	long long int timeNow = 0;
	timeNow = tmrReadTimer();

	if ((timeNow - timer) < ((long long int)ms * ticksPerMs))
		return 0;
	else
	return 1;
}

/*****************************************************************************
* Test to see if a number of frames have passed.
*
* Returns TRUE if number of frames have passed since the 'frameCount' was
* read.  FALSE if number of frames have not passed.
*
* This routine does not update the 'frameCount', so it will continue to
* return TRUE, until 'frameCount' is updated by the calling program.
*
* Called with:
*    frameCount = Frame counter, read at start of delay loop.
*    frames     = Number of frames to test for.
*
* Returns with:
*    TRUE  - if number of 'frames' have passed since 'frameCount' was read.
*    FALSE - if number of 'frames' have not yet passed.
*****************************************************************************/
int	tmrTestFrameCount( unsigned int newFrameCount, unsigned int frames)
{
	if (frameCount - newFrameCount < frames)
		return 0;

	return 1;
}
