/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/dummy/dummy.h,v 1.3 2004/06/25 15:50:32 tsi Exp $ */

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* Supported chipsets */
typedef enum {
    DUMMY_CHIP
} DUMMYType;

/* function prototypes */

extern Bool DUMMYSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
extern void DUMMYAdjustFrame(int scrnIndex, int x, int y, int flags);

/* globals */
typedef struct _color
{
    int red;
    int green;
    int blue;
} dummy_colors;

typedef struct dummyRec 
{
    /* proc pointer */
    CloseScreenProcPtr CloseScreen;

    dummy_colors colors[256];
    pointer* FBBase;
} DUMMYRec, *DUMMYPtr;

/* The privates of the DUMMY driver */
#define DUMMYPTR(p)	((DUMMYPtr)((p)->driverPrivate))

