/* XAA vector drawing functions for PiTrex
 * Based on drivers/nv/riva_xaa.c
 *
 * Note: Still need a way to do single points (stars). Not accomodated
 *       by XAA.
 */

#include <unistd.h>
#include <stdio.h>
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include "dummy.h"
#include "pitrex_xaa.h"
#include "xaalocal.h"
#include "xaarop.h"
#include "miline.h"
#include "os.h"

char beamintensity;
unsigned int xdim;
unsigned int ydim;
OsTimerPtr waitRecalTimer;
unsigned char started = 0;
int lines = 0;
/* Note that logging all debug messages causes extreme slow-down in display */
//#define PITREX_DEBUG

static CARD32 PitrexPaddingFrame (OsTimerPtr timer, CARD32 now, pointer arg)
{
        ScrnInfoPtr pScrn = (ScrnInfoPtr) arg;
	DUMMYPtr dPtr = DUMMYPTR(pScrn);

        if ( (GET (VIA_int_flags) & 0x20) && started)
        {
#ifdef PITREX_DEBUG
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "X-vectrex: Padding Frame.\n");
#endif
         v_WaitRecal_buffered(0);
         v_readButtons();
        }
        return dPtr->refreshInterval;
}

static void
PitrexSetupForSolidLine(ScrnInfoPtr pScrn, int color, int rop, unsigned planemask)
{
  DUMMYPtr dPtr = DUMMYPTR(pScrn);
 /* TODO: Configurable intensity maps from colour like in svgalib-vectrex. */
  beamintensity = (char)((color >> (pScrn->depth - 4)) + dPtr->intensityOffset);
#ifdef PITREX_DEBUG
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "X-vectrex: line set-up. Colour=%d (%d scaled)\n", color,
	           (color >> (pScrn->depth - 4)) + dPtr->intensityOffset);
#endif
}

int __pitrex_xaa_vectrex_scalexcoordinate(int coordinate, ScrnInfoPtr pScrn)
{       /* Urgent-TODO: Where do I get the current resolution?
           -Part of pScrn structure in DUMMYPreInit? - And DUMMYScreenInit?*/
        DUMMYPtr dPtr = DUMMYPTR(pScrn);
        return ( (int)(((coordinate / (float)xdim) * 65535) - 32767) * dPtr->xscale ) /2 + dPtr->xoffset;
}

int __pitrex_xaa_vectrex_scaleycoordinate(int coordinate, ScrnInfoPtr pScrn)
{
        DUMMYPtr dPtr = DUMMYPTR(pScrn);
        return ( (~((int)(((coordinate / (float)ydim) * 65535) - 32767)) + 1) * dPtr->yscale ) /2 + dPtr->yoffset;
}

static void 
PitrexSubsequentSolidTwoPointLine(ScrnInfoPtr pScrn, int x1, int y1,
                              int x2, int y2, int flags)
{
        if (beamintensity <= 0 || lines == MAX_PIPELINE/2) return;
/*
#ifdef PITREX_DEBUG
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "X-vectrex: Draw Input = %d,%d-%d,%d.\n", x1, y1, x2, y2);
#endif
*/
#ifdef WINDOW
        v_brightness(beamintensity);
        v_line(x1, y1, x2, y2);
#else
        v_directDraw32  (
                                 __pitrex_xaa_vectrex_scalexcoordinate(x1, pScrn),
                                 __pitrex_xaa_vectrex_scaleycoordinate(y1, pScrn),
                                 __pitrex_xaa_vectrex_scalexcoordinate(x2, pScrn),
                                 __pitrex_xaa_vectrex_scaleycoordinate(y2, pScrn),
                                 beamintensity
                                );
#endif
	lines++;
}

/* Horizontal lines always drawn to the right (DEGREES_0)
 * Vertical lines always drawn down (DEGREES_270)
 */
static void 
PitrexSubsequentSolidHorVertLine(ScrnInfoPtr pScrn, int x, int y, int len, int dir)
{
 if (dir == DEGREES_0)
  PitrexSubsequentSolidTwoPointLine(pScrn, x, y, x+len, y, 0);
 if (dir == DEGREES_270)
  PitrexSubsequentSolidTwoPointLine(pScrn, x, y, x, y+len, 0);
#ifdef PITREX_DEBUG
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "X-vectrex: hor-vert_line.\n");
#endif
}

static void PitrexSubsequentSolidBresenhamLine(ScrnInfoPtr pScrn,
        int x, int y, int major, int minor, int err, int len, int octant)
{
#ifdef PITREX_DEBUG
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "X-vectrex: Bresenham Line.\n");
#endif
}

static void
PitrexSetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop, unsigned planemask)
{
  DUMMYPtr dPtr = DUMMYPTR(pScrn);
 /* TODO: Configurable intensity maps from colour like in svgalib-vectrex. */
  beamintensity = (char)((color >> (pScrn->depth - 4)) + dPtr->intensityOffset);
//        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "X-vectrex: solid set-up.\n");
}

static void
PitrexSubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
 DUMMYPtr dPtr = DUMMYPTR(pScrn);
 if (h > dPtr->refreshBoxHeight)
 {
  v_WaitRecal_buffered(1);
  v_readButtons();
  started = 1;
  lines = 0;
 }
 else
 {
   PitrexSubsequentSolidTwoPointLine(pScrn, x, y, x+w, y, 0);
   PitrexSubsequentSolidTwoPointLine(pScrn, x+w, y, x+w, y+h, 0);
   PitrexSubsequentSolidTwoPointLine(pScrn, x+w, y+h, x, y+h, 0);
   PitrexSubsequentSolidTwoPointLine(pScrn, x, y+h, x, y, 0);
 }

#ifdef PITREX_DEBUG
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "X-vectrex: Rect.\n");
#endif
}

void PitrexSync(ScrnInfoPtr pScrn)
{
/* only needed if we implement a raster-readable vector buffer */
//         v_WaitRecal(); /* This almost works, but there are gaps between calls */
}

Bool pitrexXAAinit(ScreenPtr pScreen)
{
    lines=0;
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DUMMYPtr dPtr = DUMMYPTR(pScrn);
//    pitrexPtr pPitrex = pitrexPTR(pScrn);

//    pPitrex->AccelInfoRec = 
    infoPtr = XAACreateInfoRec();
    if(!infoPtr) return FALSE;

    /* sync */
    infoPtr->Sync = PitrexSync;

    /* Line drawing */
    infoPtr->SolidLineFlags = 0;
    infoPtr->SetupForSolidLine = PitrexSetupForSolidLine;
    infoPtr->SubsequentSolidTwoPointLine = 
		PitrexSubsequentSolidTwoPointLine;
    infoPtr->SubsequentSolidHorVertLine =
                PitrexSubsequentSolidHorVertLine;
    infoPtr->SubsequentSolidBresenhamLine =
                PitrexSubsequentSolidBresenhamLine;

    infoPtr->SetupForSolidFill = PitrexSetupForSolidFill;
    infoPtr->SubsequentSolidFillRect = PitrexSubsequentSolidFillRect;
/* TODO:

    infoPtr->SubsequentSolidFillTrap = PiTrexSubsequentSolidFillTrap;
    infoPtr->SetupForDashedLine = PitrexSetupForDashedLine;
    infoPtr->SubsequentDashedTwoPointLine = PitrexSubsequentDashedTwoPointLine;
*/
    /* Based on xf86SetDpi in common/xf86Helper.c, also RivaScreenInit in drivers/nv/riva_driver.c
     * -Is this / Can I be ignoring the window size? */
    xdim = pScrn->virtualX;
    ydim = pScrn->virtualY;
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Starting Vectrex Display\n");
    if (!vectrexinit(1) )
    {
     xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Could Not Initialise Vectrex Connection\n");
     return FALSE;
    }
     v_setName("X_generic"); /* per-game settings via XF86Config files */
     v_init();
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Started Vectrex Display\n");

     beamintensity = 100;
     v_setRefresh (50);
     v_directDraw32(20,20,60,60,90);

     /* Set timer for automatic screen refreshes */
     waitRecalTimer = TimerSet(NULL, 0, dPtr->refreshInterval, PitrexPaddingFrame, pScrn);
#ifdef WINDOW
        v_window(0, ydim, xdim, 0, NO_CLIP);
#endif

     return(XAAInit(pScreen, infoPtr));
}
