/******************************************************************
   USB-DVG drivers for Win32 and Linux
   Code by Mario Montminy, 2020
*******************************************************************/
#ifndef _ZVGFRAME_H_
#define _ZVGFRAME_H_

#include <stdint.h>
#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif


#define X_MIN   (-512)
#define X_MAX   511
#define Y_MIN   (-384)
#define Y_MAX   383

// Prototypes

extern void     zvgBanner(uint32_t speeds, void *id);
extern void     zvgError(uint32_t err);
extern int      zvgFrameOpen(void);
extern void     zvgFrameClose(void);
extern void     zvgFrameSetRGB15(uint8_t red, uint8_t green, uint8_t blue);
extern void     zvgFrameSetClipWin(int xMin, int yMin, int xMax, int yMax);
extern uint32_t zvgFrameVector(int xStart, int yStart, int xEnd, int yEnd);
extern uint32_t zvgFrameSend(void);

#ifdef __cplusplus
}
#endif

#endif
