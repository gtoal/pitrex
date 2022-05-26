/* $XFree86: xc/programs/Xserver/hw/xfree86/input/mouse/mouse.c,v 1.87 2007/04/14 23:38:01 tsi Exp $ */
/*
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 * Copyright 2002 by SuSE Linux AG, Author: Egbert Eich
 * Copyright 2002 by Paul Elliott
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of copyright holders not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The copyright holders
 * make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * Copyright (c) 1994-2005 by The XFree86 Project, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *   1.  Redistributions of source code must retain the above copyright
 *       notice, this list of conditions, and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the
 *       distribution, and in the same place and form as other copyright,
 *       license and disclaimer information.
 *
 *   3.  The end-user documentation included with the redistribution,
 *       if any, must include the following acknowledgment: "This product
 *       includes software developed by The XFree86 Project, Inc
 *       (http://www.xfree86.org/) and its contributors", in the same
 *       place and form as other third-party acknowledgments.  Alternately,
 *       this acknowledgment may appear in the software itself, in the
 *       same form and location as other such third-party acknowledgments.
 *
 *   4.  Except as contained in this notice, the name of The XFree86
 *       Project, Inc shall not be used in advertising or otherwise to
 *       promote the sale, use or other dealings in this Software without
 *       prior written authorization from The XFree86 Project, Inc.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE XFREE86 PROJECT, INC OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* Patch for PS/2 Intellimouse - Tim Goodwin 1997-11-06. */

/*
 * [JCH-96/01/21] Added fourth button support for PROT_GLIDEPOINT mouse
 * protocol.
 */

/*
 * [TVO-97/03/05] Added microsoft IntelliMouse support
 */

/*
 * [PME-02/08/11] Added suport for drag lock buttons
 * for use with 4 button trackballs for convenience
 * and to help limited dexterity persons
 */

#define NEED_EVENTS
#include <X11/X.h>
#include <X11/Xproto.h>

#include "xf86.h"

#ifdef XINPUT
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "extnsionst.h"
#include "extinit.h"
#else
#include "inputstr.h"
#endif

#include "xf86Xinput.h"
#include "xf86_OSproc.h"
#include "xf86OSmouse.h"
#define NEED_XF86_TYPES	/* for xisb.h when !XFree86LOADER */
#include "xf86_ansic.h"
#include "compiler.h"

#include "xisb.h"
#include "mouse.h"
#include "mousePriv.h"
#include "mipointer.h"

enum {
    /* number of bits in mapped nibble */
    NIB_BITS=4,
    /* size of map of nibbles to bitmask */
    NIB_SIZE= (1 << NIB_BITS),
    /* mask for map */
    NIB_MASK= (NIB_SIZE -1),
    /* number of maps to map all the buttons */
    NIB_COUNT = ((MSE_MAXBUTTONS+NIB_BITS-1)/NIB_BITS)
};

/*data to be used in implementing trackball drag locks.*/
typedef struct _DragLockRec {

    /* Fields used to implement trackball drag locks. */
    /* mask for those buttons that are ordinary drag lock buttons */
    int lockButtonsM;

    /* mask for the master drag lock button if any */
    int masterLockM;

    /* button state up/down from last time adjusted for drag locks */
    int lockLastButtons;

    /*
     * true if master lock state i.e. master drag lock
     * button has just been pressed
     */
    int masterTS;

    /* simulate these buttons being down although they are not */
    int simulatedDown;

    /*
     * data to map bits for drag lock buttons to corresponding
     * bits for the target buttons
     */
    int nib_table[NIB_COUNT][NIB_SIZE];

} DragLockRec, *DragLockPtr;



#ifdef XFree86LOADER
static const OptionInfoRec *MouseAvailableOptions(void *unused);
#endif
static InputInfoPtr MousePreInit(InputDriverPtr drv, IDevPtr dev, int flags);
#if 0
static void MouseUnInit(InputDriverPtr drv, InputInfoPtr pInfo, int flags);
#endif

static int MouseProc(DeviceIntPtr device, int what);
static Bool MouseConvert(LocalDevicePtr local, int first, int num, int v0,
		 	     int v1, int v2, int v3, int v4, int v5, int *x,
		 	     int *y);

static void MouseCtrl(DeviceIntPtr device, PtrCtrl *ctrl);
static void MousePostEvent(InputInfoPtr pInfo, int buttons,
			   int dx, int dy, int dz, int dw);
static void MouseReadInput(InputInfoPtr pInfo);
static void MouseBlockHandler(pointer data, struct timeval **waitTime,
			      pointer LastSelectMask);
static void MouseWakeupHandler(pointer data, int i, pointer LastSelectMask);
static void FlushButtons(MouseDevPtr pMse);

static Bool SetupMouse(InputInfoPtr pInfo);
static Bool initMouseHW(InputInfoPtr pInfo);
#ifdef SUPPORT_MOUSE_RESET
static Bool mouseReset(InputInfoPtr pInfo, unsigned char val);
static void ps2WakeupHandler(pointer data, int i, pointer LastSelectMask);
static void ps2BlockHandler(pointer data, struct timeval **waitTime,
			    pointer LastSelectMask);
#endif

/* mouse autoprobe stuff */
static const char *autoOSProtocol(InputInfoPtr pInfo, int *protoPara);
static void autoProbeMouse(InputInfoPtr pInfo, Bool inSync, Bool lostSync);
static void checkForErraticMovements(InputInfoPtr pInfo, int dx, int dy);
static Bool collectData(MouseDevPtr pMse, unsigned char u);
static void SetMouseProto(MouseDevPtr pMse, MouseProtocolID protocolID);
static Bool autoGood(MouseDevPtr pMse);

#undef MOUSE
InputDriverRec MOUSE = {
	1,
	"mouse",
	NULL,
	MousePreInit,
	/*MouseUnInit,*/NULL,
	NULL,
	0
};

typedef enum {
    OPTION_ALWAYS_CORE,
    OPTION_SEND_CORE_EVENTS,
    OPTION_CORE_POINTER,
    OPTION_SEND_DRAG_EVENTS,
    OPTION_HISTORY_SIZE,
    OPTION_DEVICE,
    OPTION_PROTOCOL,
    OPTION_BUTTONS,
    OPTION_EMULATE_3_BUTTONS,
    OPTION_EMULATE_3_TIMEOUT,
    OPTION_CHORD_MIDDLE,
    OPTION_FLIP_XY,
    OPTION_INV_X,
    OPTION_INV_Y,
    OPTION_ANGLE_OFFSET,
    OPTION_Z_AXIS_MAPPING,
    OPTION_SAMPLE_RATE,
    OPTION_RESOLUTION,
    OPTION_EMULATE_WHEEL,
    OPTION_EMU_WHEEL_BUTTON,
    OPTION_EMU_WHEEL_INERTIA,
    OPTION_X_AXIS_MAPPING,
    OPTION_Y_AXIS_MAPPING,
    OPTION_AUTO_SOFT,
    OPTION_DRAGLOCKBUTTONS,
    OPTION_PNP,
    OPTION_CLEAR_DTR,
    OPTION_CLEAR_RTS,
    OPTION_BAUD_RATE,
    OPTION_DATA_BITS,
    OPTION_STOP_BITS,
    OPTION_PARITY,
    OPTION_FLOW_CONTROL,
    OPTION_VTIME,
    OPTION_VMIN
} MouseOpts;

#ifdef XFree86LOADER
static const OptionInfoRec mouseOptions[] = {
    { OPTION_ALWAYS_CORE,	"AlwaysCore",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SEND_CORE_EVENTS,	"SendCoreEvents", OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_CORE_POINTER,	"CorePointer",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SEND_DRAG_EVENTS,	"SendDragEvents", OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HISTORY_SIZE,	"HistorySize",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_DEVICE,		"Device",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_PROTOCOL,		"Protocol",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_BUTTONS,		"Buttons",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_EMULATE_3_BUTTONS,	"Emulate3Buttons",OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_EMULATE_3_TIMEOUT,	"Emulate3Timeout",OPTV_INTEGER,	{0}, FALSE },
    { OPTION_CHORD_MIDDLE,	"ChordMiddle",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_FLIP_XY,		"FlipXY",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_INV_X,		"InvX",		  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_INV_Y,		"InvY",		  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_ANGLE_OFFSET,	"AngleOffset",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_Z_AXIS_MAPPING,	"ZAxisMapping",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_SAMPLE_RATE,	"SampleRate",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_RESOLUTION,	"Resolution",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_EMULATE_WHEEL,	"EmulateWheel",	  OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_EMU_WHEEL_BUTTON,	"EmulateWheelButton", OPTV_INTEGER, {0}, FALSE },
    { OPTION_EMU_WHEEL_INERTIA,	"EmulateWheelInertia", OPTV_INTEGER, {0}, FALSE },
    { OPTION_X_AXIS_MAPPING,	"XAxisMapping",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_Y_AXIS_MAPPING,	"YAxisMapping",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_AUTO_SOFT,		"AutoSoft",	  OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_PNP,		"PnP",		  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_DRAGLOCKBUTTONS,	"DragLockButtons",OPTV_STRING,	{0}, FALSE },
    /* serial options */
    { OPTION_CLEAR_DTR,		"ClearDTR",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_CLEAR_RTS,		"ClearRTS",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_BAUD_RATE,		"BaudRate",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_DATA_BITS,		"DataBits",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_STOP_BITS,		"StopBits",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_PARITY,		"Parity",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_FLOW_CONTROL,	"FlowControl",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_VTIME,		"VTime",	  OPTV_INTEGER,	{0}, FALSE },
    { OPTION_VMIN,		"VMin",		  OPTV_INTEGER,	{0}, FALSE },
    /* end serial options */
    { -1,			NULL,		  OPTV_NONE,	{0}, FALSE }
};
#endif

#define RETRY_COUNT 4

/*
 * Microsoft (all serial models), Logitech MouseMan, First Mouse, etc,
 * ALPS GlidePoint, Thinking Mouse.
 */
static const char *msDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"7",
	"StopBits",	"1",
	"Parity",	"None",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
/* MouseSystems */
static const char *mlDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"8",
	"StopBits",	"2",
	"Parity",	"None",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
/* MMSeries */
static const char *mmDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"8",
	"StopBits",	"1",
	"Parity",	"Odd",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
#if 0 
/* Logitech series 9 *//* same as msc: now mlDefaults */
static const char *logiDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"8",
	"StopBits",	"2",
	"Parity",	"None",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
#endif
/* Hitachi Tablet */
static const char *mmhitDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"8",
	"StopBits",	"1",
	"Parity",	"None",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
/* AceCad Tablet */
static const char *acecadDefaults[] = {
	"BaudRate",	"9600",
	"DataBits",	"8",
	"StopBits",	"1",
	"Parity",	"Odd",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};

static MouseProtocolRec mouseProtocols[] = {

    /* Serial protocols */
    { "Microsoft",		MSE_SERIAL,	msDefaults,	PROT_MS },
    { "MouseSystems",		MSE_SERIAL,	mlDefaults,	PROT_MSC },
    { "MMSeries",		MSE_SERIAL,	mmDefaults,	PROT_MM },
    { "Logitech",		MSE_SERIAL,	mlDefaults,	PROT_LOGI },
    { "MouseMan",		MSE_SERIAL,	msDefaults,	PROT_LOGIMAN },
    { "MMHitTab",		MSE_SERIAL,	mmhitDefaults,	PROT_MMHIT },
    { "GlidePoint",		MSE_SERIAL,	msDefaults,	PROT_GLIDE },
    { "IntelliMouse",		MSE_SERIAL,	msDefaults,	PROT_IMSERIAL },
    { "ThinkingMouse",		MSE_SERIAL,	msDefaults,	PROT_THINKING },
    { "AceCad",			MSE_SERIAL,	acecadDefaults,	PROT_ACECAD },
    { "ValuMouseScroll",	MSE_SERIAL,	msDefaults,	PROT_VALUMOUSESCROLL },

    /* Standard PS/2 */
    { "PS/2",			MSE_PS2,	NULL,		PROT_PS2 },
    { "GenericPS/2",		MSE_PS2,	NULL,		PROT_GENPS2 },

    /* Extended PS/2 */
    { "ImPS/2",			MSE_XPS2,	NULL,		PROT_IMPS2 },
    { "ExplorerPS/2",		MSE_XPS2,	NULL,		PROT_EXPPS2 },
    { "ThinkingMousePS/2",	MSE_XPS2,	NULL,		PROT_THINKPS2 },
    { "MouseManPlusPS/2",	MSE_XPS2,	NULL,		PROT_MMPS2 },
    { "GlidePointPS/2",		MSE_XPS2,	NULL,		PROT_GLIDEPS2 },
    { "NetMousePS/2",		MSE_XPS2,	NULL,		PROT_NETPS2 },
    { "NetScrollPS/2",		MSE_XPS2,	NULL,		PROT_NETSCPS2 },

    /* Bus Mouse */
    { "BusMouse",		MSE_BUS,	NULL,		PROT_BM },

    /* Auto-detect (PnP) */
    { "Auto",			MSE_AUTO,	NULL,		PROT_AUTO },

    /* Misc (usually OS-specific) */
    { "SysMouse",		MSE_MISC,	mlDefaults,	PROT_SYSMOUSE },

    /* end of list */
    { NULL,			MSE_NONE,	NULL,		PROT_UNKNOWN }
};

#ifdef XFree86LOADER
/*ARGSUSED*/
static const OptionInfoRec *
MouseAvailableOptions(void *unused)
{
    return (mouseOptions);
}
#endif

/* Process options common to all mouse types. */
static void
MouseCommonOptions(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    MessageType buttons_from = X_CONFIG;
    char *s;
    int origButtons;

    pMse = pInfo->private;

    pMse->buttons = xf86SetIntOption(pInfo->options, "Buttons", 0);
    if (!pMse->buttons) {
	pMse->buttons = MSE_DFLTBUTTONS;
	buttons_from = X_DEFAULT;
    }
    origButtons = pMse->buttons;

    pMse->emulate3Buttons = xf86SetBoolOption(pInfo->options,
					      "Emulate3Buttons", FALSE);
    if (!xf86FindOptionValue(pInfo->options,"Emulate3Buttons"))
	pMse->emulate3ButtonsSoft = TRUE;
    
    pMse->emulate3Timeout = xf86SetIntOption(pInfo->options,
					     "Emulate3Timeout", 50);
    if (pMse->emulate3Buttons || pMse->emulate3ButtonsSoft) {
	MessageType from = X_CONFIG;
	if (pMse->emulate3ButtonsSoft)
	    from = X_DEFAULT;
	xf86Msg(from, "%s: Emulate3Buttons, Emulate3Timeout: %d\n",
		pInfo->name, pMse->emulate3Timeout);
    }

    pMse->chordMiddle = xf86SetBoolOption(pInfo->options, "ChordMiddle", FALSE);
    if (pMse->chordMiddle)
	xf86Msg(X_CONFIG, "%s: ChordMiddle\n", pInfo->name);
    pMse->flipXY = xf86SetBoolOption(pInfo->options, "FlipXY", FALSE);
    if (pMse->flipXY)
	xf86Msg(X_CONFIG, "%s: FlipXY\n", pInfo->name);
    if (xf86SetBoolOption(pInfo->options, "InvX", FALSE)) {
	pMse->invX = -1;
	xf86Msg(X_CONFIG, "%s: InvX\n", pInfo->name);
    } else
	pMse->invX = 1;
    if (xf86SetBoolOption(pInfo->options, "InvY", FALSE)) {
	pMse->invY = -1;
	xf86Msg(X_CONFIG, "%s: InvY\n", pInfo->name);
    } else
	pMse->invY = 1;
    pMse->angleOffset = xf86SetIntOption(pInfo->options, "AngleOffset", 0);
    

    if (pMse->pDragLock)
	xfree(pMse->pDragLock);
    pMse->pDragLock = NULL;
      
    s = xf86SetStrOption(pInfo->options, "DragLockButtons", NULL);

    if (s) {
	int lock;             /* lock button */
	int target;           /* target button */
	int lockM,targetM;    /* bitmasks for drag lock, target */
	int i, j;             /* indexes */
	char *s1;             /* parse input string */
	DragLockPtr pLock;
      
	pLock = pMse->pDragLock = xcalloc(1, sizeof(DragLockRec));
	/* init code */

	/* initial string to be taken apart */
	s1 = s;
      
	/* keep getting numbers which are buttons */
	while ((s1 != NULL) && (lock = strtol(s1, &s1, 10)) != 0) {

	    /* check sanity for a button */
	    if ((lock < 0) || (lock > MSE_MAXBUTTONS)) {
		xf86Msg(X_WARNING, "DragLock: Invalid button number = %d\n",
			lock);
		break;
	    };
	    /* turn into a button mask */
	    lockM = 1 << (lock - 1);

	    /* try to get drag lock button */
	    if ((s1 == NULL) || ((target=strtol(s1, &s1, 10)) == 0)) {
		/*if no target, must be a master drag lock button */
		/* save master drag lock mask */
		pLock->masterLockM = lockM;
		xf86Msg(X_CONFIG, 
			"DragLock button %d is master drag lock", 
			lock);
	    } else {
		/* have target button number*/
		/* check target button number for sanity */
		if ((target < 0) || (target > MSE_MAXBUTTONS)) {
		    xf86Msg(X_WARNING, 
			    "DragLock: Invalid button number for target=%d\n",
			    target);
		    break;
		}

		/* target button mask */
		targetM = 1 << (target - 1);

		xf86Msg(X_CONFIG, 
			"DragLock: button %d is drag lock for button %d\n", 
			lock,target);
		lock--;

		/* initialize table that maps drag lock mask to target mask */
		pLock->nib_table[lock / NIB_BITS][1 << (lock % NIB_BITS)] = 
			targetM;

		/* add new drag lock to mask of drag locks */
		pLock->lockButtonsM |= lockM;
	    }

	} 

	/*
	 * fill out rest of map that maps sets of drag lock buttons
	 * to sets of target buttons, in the form of masks
	 */

	/* for each nibble */
	for (i = 0; i < NIB_COUNT; i++) {
	    /* for each possible set of bits for that nibble */
	    for (j = 0; j < NIB_SIZE; j++) {
		int ff, fM, otherbits;

		/* get first bit set in j*/
		ff = ffs(j) - 1;
		/* if 0 bits set nothing to do */
		if (ff >= 0) {
		    /* form mask for fist bit set */
		    fM = 1 << ff;
		    /* mask off first bit set to get remaining bits set*/
		    otherbits = j & ~fM;
		    /*
		     * if otherbits =0 then only 1 bit set
		     * so j=fM
		     * nib_table[i][fM] already calculated if fM has
		     * only 1 bit set.
		     * nib_table[i][j] has already been filled in
		     * by previous loop. otherwise
		     * otherbits < j so nibtable[i][otherbits]
		     * has already been calculated.
		     */
		    if (otherbits)
			pLock->nib_table[i][j] = 
				     pLock->nib_table[i][fM] |
				     pLock->nib_table[i][otherbits];

		}
	    }
	}
    }

    s = xf86SetStrOption(pInfo->options, "ZAxisMapping", NULL);
    if (s) {
	int b1 = 0, b2 = 0, b3 = 0, b4 = 0;
	char *msg = NULL;

	if (!xf86NameCmp(s, "x")) {
	    pMse->negativeZ = pMse->positiveZ = MSE_MAPTOX;
	    pMse->negativeW = pMse->positiveW = MSE_MAPTOX;
	    msg = xstrdup("X axis");
	} else if (!xf86NameCmp(s, "y")) {
	    pMse->negativeZ = pMse->positiveZ = MSE_MAPTOY;
	    pMse->negativeW = pMse->positiveW = MSE_MAPTOY;
	    msg = xstrdup("Y axis");
	} else if (sscanf(s, "%d %d %d %d", &b1, &b2, &b3, &b4) >= 2 &&
		 b1 > 0 && b1 <= MSE_MAXBUTTONS &&
		 b2 > 0 && b2 <= MSE_MAXBUTTONS) {
	    msg = xstrdup("buttons XX and YY");
	    if (msg)
		sprintf(msg, "buttons %d and %d", b1, b2);
	    pMse->negativeZ = pMse->negativeW = 1 << (b1-1);
	    pMse->positiveZ = pMse->positiveW = 1 << (b2-1);
	    if (b3 > 0 && b3 <= MSE_MAXBUTTONS &&
		b4 > 0 && b4 <= MSE_MAXBUTTONS) {
		if (msg)
		    xfree(msg);
		msg = xstrdup("buttons XX, YY, ZZ and WW");
		if (msg)
		    sprintf(msg, "buttons %d, %d, %d and %d", b1, b2, b3, b4);
		pMse->negativeW = 1 << (b3-1);
		pMse->positiveW = 1 << (b4-1);
	    }
	    if (b1 > pMse->buttons) pMse->buttons = b1;
	    if (b2 > pMse->buttons) pMse->buttons = b2;
	    if (b3 > pMse->buttons) pMse->buttons = b3;
	    if (b4 > pMse->buttons) pMse->buttons = b4;
	} else {
	    pMse->negativeZ = pMse->positiveZ = MSE_NOZMAP;
	    pMse->negativeW = pMse->positiveW = MSE_NOZMAP;
	}
	if (msg) {
	    xf86Msg(X_CONFIG, "%s: ZAxisMapping: %s\n", pInfo->name, msg);
	    xfree(msg);
	} else {
	    xf86Msg(X_WARNING, "%s: Invalid ZAxisMapping value: \"%s\"\n",
		    pInfo->name, s);
	}
    }
    if (xf86SetBoolOption(pInfo->options, "EmulateWheel", FALSE)) {
	Bool yFromConfig = FALSE;
	int wheelButton;

	pMse->emulateWheel = TRUE;
	wheelButton = xf86SetIntOption(pInfo->options,
					"EmulateWheelButton", 4);
	if (wheelButton < 0 || wheelButton > MSE_MAXBUTTONS) {
	    xf86Msg(X_WARNING, "%s: Invalid EmulateWheelButton value: %d\n",
			pInfo->name, wheelButton);
	    wheelButton = 4;
	}
	pMse->wheelButtonMask = 1 << (wheelButton - 1);
	
	pMse->wheelInertia = xf86SetIntOption(pInfo->options,
					"EmulateWheelInertia", 10);
	if (pMse->wheelInertia <= 0) {
	    xf86Msg(X_WARNING, "%s: Invalid EmulateWheelInertia value: %d\n",
			pInfo->name, pMse->wheelInertia);
	    pMse->wheelInertia = 50;
	}

	pMse->negativeX = MSE_NOAXISMAP;
	pMse->positiveX = MSE_NOAXISMAP;
	s = xf86SetStrOption(pInfo->options, "XAxisMapping", NULL);
	if (s) {
	    int b1 = 0, b2 = 0;
	    char *msg = NULL;

	    if ((sscanf(s, "%d %d", &b1, &b2) == 2) &&
		 b1 > 0 && b1 <= MSE_MAXBUTTONS &&
		 b2 > 0 && b2 <= MSE_MAXBUTTONS) {
		msg = xstrdup("buttons XX and YY");
		if (msg)
		    sprintf(msg, "buttons %d and %d", b1, b2);
		pMse->negativeX = b1;
		pMse->positiveX = b2;
		if (b1 > pMse->buttons) pMse->buttons = b1;
		if (b2 > pMse->buttons) pMse->buttons = b2;
	    } else {
		xf86Msg(X_WARNING, "%s: Invalid XAxisMapping value: \"%s\"\n",
			pInfo->name, s);
	    }
	    if (msg) {
		xf86Msg(X_CONFIG, "%s: XAxisMapping: %s\n", pInfo->name, msg);
		xfree(msg);
	    }
	}
	s = xf86SetStrOption(pInfo->options, "YAxisMapping", NULL);
	if (s) {
	    int b1 = 0, b2 = 0;
	    char *msg = NULL;

	    if ((sscanf(s, "%d %d", &b1, &b2) == 2) &&
		 b1 > 0 && b1 <= MSE_MAXBUTTONS &&
		 b2 > 0 && b2 <= MSE_MAXBUTTONS) {
		msg = xstrdup("buttons XX and YY");
		if (msg)
		    sprintf(msg, "buttons %d and %d", b1, b2);
		pMse->negativeY = b1;
		pMse->positiveY = b2;
		if (b1 > pMse->buttons) pMse->buttons = b1;
		if (b2 > pMse->buttons) pMse->buttons = b2;
		yFromConfig = TRUE;
	    } else {
		xf86Msg(X_WARNING, "%s: Invalid YAxisMapping value: \"%s\"\n",
			pInfo->name, s);
	    }
	    if (msg) {
		xf86Msg(X_CONFIG, "%s: YAxisMapping: %s\n", pInfo->name, msg);
		xfree(msg);
	    }
	}
	if (!yFromConfig) {
	    pMse->negativeY = 4;
	    pMse->positiveY = 5;
	    if (pMse->negativeY > pMse->buttons)
		pMse->buttons = pMse->negativeY;
	    if (pMse->positiveY > pMse->buttons)
		pMse->buttons = pMse->positiveY;
	    xf86Msg(X_DEFAULT, "%s: YAxisMapping: buttons %d and %d\n",
		    pInfo->name, pMse->negativeY, pMse->positiveY);
	}
	xf86Msg(X_CONFIG, "%s: EmulateWheel, EmulateWheelButton: %d, "
			  "EmulateWheelInertia: %d\n",
		pInfo->name, wheelButton, pMse->wheelInertia);
    }
    if (origButtons != pMse->buttons)
	buttons_from = X_CONFIG;
    xf86Msg(buttons_from, "%s: Buttons: %d\n", pInfo->name, pMse->buttons);
    
}
/*
 * map bits corresponding to lock buttons.
 * for each bit for a lock button,
 * turn on bit corresponding to button button that the lock
 * button services.
 */

static int
lock2targetMap(DragLockPtr pLock, int lockMask)
{
    int result,i;
    result = 0;

    /*
     * for each nibble group of bits, use
     * map for that group to get corresponding
     * bits, turn them on.
     * if 4 or less buttons only first map will
     * need to be used.
     */
    for (i = 0; (i < NIB_COUNT) && lockMask; i++) {
	result |= pLock->nib_table[i][lockMask& NIB_MASK];

	lockMask &= ~NIB_MASK;
	lockMask >>= NIB_BITS;
    }
    return result;
}

static void
MouseHWOptions(InputInfoPtr pInfo)
{
    MouseDevPtr  pMse = pInfo->private;
    mousePrivPtr mPriv = (mousePrivPtr)pMse->mousePriv;
    
    if (mPriv == NULL) 
	    return;

    if ((mPriv->soft
	 = xf86SetBoolOption(pInfo->options, "AutoSoft", FALSE))) {
	xf86Msg(X_CONFIG, "Don't initialize mouse when auto-probing\n");
    }
    if ((mPriv->noPnP
	 = !xf86SetBoolOption(pInfo->options, "PnP", TRUE))) {
	xf86Msg(X_CONFIG, "Don't do PnP probing.\n");
    }
    pMse->sampleRate = xf86SetIntOption(pInfo->options, "SampleRate", 0);
    if (pMse->sampleRate) {
	xf86Msg(X_CONFIG, "%s: SampleRate: %d\n", pInfo->name,
		pMse->sampleRate);
    }
    pMse->resolution = xf86SetIntOption(pInfo->options, "Resolution", 0);
    if (pMse->resolution) {
	xf86Msg(X_CONFIG, "%s: Resolution: %d\n", pInfo->name,
		pMse->resolution);
    }
}

static void
MouseSerialOptions(InputInfoPtr pInfo)
{
    MouseDevPtr  pMse = pInfo->private;
    Bool clearDTR, clearRTS;
    
    
    pMse->baudRate = xf86SetIntOption(pInfo->options, "BaudRate", 0);
    if (pMse->baudRate) {
	xf86Msg(X_CONFIG, "%s: BaudRate: %d\n", pInfo->name,
		pMse->baudRate);
    }

    if ((clearDTR = xf86SetBoolOption(pInfo->options, "ClearDTR",FALSE)))
	pMse->mouseFlags |= MF_CLEAR_DTR;
	
    
    if ((clearRTS = xf86SetBoolOption(pInfo->options, "ClearRTS",FALSE)))
	pMse->mouseFlags |= MF_CLEAR_RTS;
	
    if (clearDTR || clearRTS) {
	xf86Msg(X_CONFIG, "%s: ", pInfo->name);
	if (clearDTR) {
	    xf86ErrorF("ClearDTR");
	    if (clearRTS)
		xf86ErrorF(", ");
	}
	if (clearRTS) {
	    xf86ErrorF("ClearRTS");
	}
	xf86ErrorF("\n");
    }
}

static MouseProtocolID
ProtocolNameToID(const char *name)
{
    int i;

    for (i = 0; mouseProtocols[i].name; i++)
	if (xf86NameCmp(name, mouseProtocols[i].name) == 0)
	    return mouseProtocols[i].id;
    return PROT_UNKNOWN;
}

static const char *
ProtocolIDToName(MouseProtocolID id)
{
    int i;

    switch (id) {
    case PROT_UNKNOWN:
	return "Unknown";
    case PROT_UNSUP:
	return "Unsupported";
    default:
	for (i = 0; mouseProtocols[i].name; i++)
	    if (id == mouseProtocols[i].id)
		return mouseProtocols[i].name;
	return "Invalid";
    }
}

const char *
xf86MouseProtocolIDToName(MouseProtocolID id)
{
	return ProtocolIDToName(id);
}

MouseProtocolID
xf86MouseProtocolNameToID(const char *name)
{
    return ProtocolNameToID(name);
}

static int
ProtocolIDToClass(MouseProtocolID id)
{
    int i;

    switch (id) {
    case PROT_UNKNOWN:
    case PROT_UNSUP:
	return MSE_NONE;
    default:
	for (i = 0; mouseProtocols[i].name; i++)
	    if (id == mouseProtocols[i].id)
		return mouseProtocols[i].class;
	return MSE_NONE;
    }
}

static MouseProtocolPtr
GetProtocol(MouseProtocolID id) {
    int i;

    switch (id) {
    case PROT_UNKNOWN:
    case PROT_UNSUP:
	return NULL;
    default:
	for (i = 0; mouseProtocols[i].name; i++)
	    if (id == mouseProtocols[i].id) {
		return &mouseProtocols[i];
	    }
	return NULL;
    }
}

static OSMouseInfoPtr osInfo = NULL;

static Bool
InitProtocols(void)
{
    int classes;
    int i;
    const char *osname = NULL;

    if (osInfo)
	return TRUE;

    osInfo = xf86OSMouseInit(0);
    if (!osInfo)
	return FALSE;
    if (!osInfo->SupportedInterfaces)
	return FALSE;

    classes = osInfo->SupportedInterfaces();
    if (!classes)
	return FALSE;
    
    /* Mark unsupported interface classes. */
    for (i = 0; mouseProtocols[i].name; i++)
	if (!(mouseProtocols[i].class & classes))
	    mouseProtocols[i].id = PROT_UNSUP;

    for (i = 0; mouseProtocols[i].name; i++)
	if (mouseProtocols[i].class & MSE_MISC)
	    if (!osInfo->CheckProtocol ||
		!osInfo->CheckProtocol(mouseProtocols[i].name))
		mouseProtocols[i].id = PROT_UNSUP;

    /* NetBSD uses PROT_BM for "PS/2". */
    xf86GetOS(&osname, NULL, NULL, NULL);
    if (osname && xf86NameCmp(osname, "netbsd") == 0)
	for (i = 0; mouseProtocols[i].name; i++)
	    if (mouseProtocols[i].id == PROT_PS2)
		mouseProtocols[i].id = PROT_BM;

    return TRUE;
}

static InputInfoPtr
MousePreInit(InputDriverPtr drv, IDevPtr dev, int flags)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    mousePrivPtr mPriv;
    MessageType protocolFrom = X_DEFAULT, deviceFrom = X_CONFIG;
    const char *protocol, *osProt = NULL;
    const char *device;
    MouseProtocolID protocolID;
    MouseProtocolPtr pProto;
    Bool detected;
    
    if (!InitProtocols())
	return NULL;

    if (!(pInfo = xf86AllocateInput(drv, 0)))
	return NULL;

    /* Initialise the InputInfoRec. */
    pInfo->name = dev->identifier;
    pInfo->type_name = XI_MOUSE;
    pInfo->flags = XI86_POINTER_CAPABLE | XI86_SEND_DRAG_EVENTS;
    pInfo->device_control = MouseProc;
    pInfo->read_input = MouseReadInput;
    pInfo->motion_history_proc = xf86GetMotionEvents;
    pInfo->history_size = 0;
    pInfo->control_proc = NULL;
    pInfo->close_proc = NULL;
    pInfo->switch_mode = NULL;
    pInfo->conversion_proc = MouseConvert;
    pInfo->reverse_conversion_proc = NULL;
    pInfo->fd = -1;
    pInfo->dev = NULL;
    pInfo->private_flags = 0;
    pInfo->always_core_feedback = 0;
    pInfo->conf_idev = dev;

    /* Check if SendDragEvents has been disabled. */
    if (!xf86SetBoolOption(dev->commonOptions, "SendDragEvents", TRUE)) {
	pInfo->flags &= ~XI86_SEND_DRAG_EVENTS;
    }

    /* Allocate the MouseDevRec and initialise it. */
    /*
     * XXX This should be done by a function in the core server since the
     * MouseDevRec is defined in the os-support layer.
     */
    if (!(pMse = xcalloc(sizeof(MouseDevRec), 1)))
	return pInfo;
    pInfo->private = pMse;
    pMse->Ctrl = MouseCtrl;
    pMse->PostEvent = MousePostEvent;
    pMse->CommonOptions = MouseCommonOptions;
    
    /* Find the protocol type. */
    protocol = xf86SetStrOption(dev->commonOptions, "Protocol", NULL);
    if (protocol) {
	protocolFrom = X_CONFIG;
    } else if (osInfo->DefaultProtocol) {
	protocol = osInfo->DefaultProtocol();
	protocolFrom = X_DEFAULT;
    }
    if (!protocol) {
	xf86Msg(X_ERROR, "%s: No Protocol specified\n", pInfo->name);
	return pInfo;
    }

    protocolID = ProtocolNameToID(protocol);
    do {
	detected = TRUE;
	switch (protocolID) {
	case PROT_AUTO:
	    if (osInfo->SetupAuto) {
		if ((osProt = osInfo->SetupAuto(pInfo,NULL))) {
		    MouseProtocolID id = ProtocolNameToID(osProt);
		    if (id == PROT_UNKNOWN || id == PROT_UNSUP) {
			protocolID = id;
			protocol = osProt;
			detected = FALSE;
		    }
		}
	    }
	    break;
	case PROT_UNKNOWN:
	    /* Check for a builtin OS-specific protocol,
	     * and call its PreInit. */
	    if (osInfo->CheckProtocol
		&& osInfo->CheckProtocol(protocol)) {
		if (!xf86CheckStrOption(dev->commonOptions, "Device", NULL) &&
		    HAVE_FIND_DEVICE && osInfo->FindDevice) {
		    xf86Msg(X_WARNING, "%s: No Device specified, "
			    "looking for one...\n", pInfo->name);
		    if (!osInfo->FindDevice(pInfo, protocol, 0)) {
			xf86Msg(X_ERROR, "%s: Cannot find which device "
				"to use.\n", pInfo->name);
		    } else
			deviceFrom = X_PROBED;
		}
		if (osInfo->PreInit) {
		    osInfo->PreInit(pInfo, protocol, 0);
		}
		return pInfo;
	    }
	    xf86Msg(X_ERROR, "%s: Unknown protocol \"%s\"\n",
		    pInfo->name, protocol);
	    return pInfo;
	case PROT_UNSUP:
	    xf86Msg(X_ERROR,
		    "%s: Protocol \"%s\" is not supported on this "
		    "platform\n", pInfo->name, protocol);
	    return pInfo;
	default:
	    break;
	    
	}
    } while (!detected);
    
    if (!xf86CheckStrOption(dev->commonOptions, "Device", NULL) &&
	HAVE_FIND_DEVICE && osInfo->FindDevice) {
	xf86Msg(X_WARNING, "%s: No Device specified, looking for one...\n",
		pInfo->name);
	if (!osInfo->FindDevice(pInfo, protocol, 0)) {
	    xf86Msg(X_ERROR, "%s: Cannot find which device to use.\n",
		    pInfo->name);
	} else {
	    deviceFrom = X_PROBED;
	    xf86MarkOptionUsedByName(dev->commonOptions, "Device");
	}
    }

    device = xf86CheckStrOption(dev->commonOptions, "Device", NULL);
    if (device)
	xf86Msg(deviceFrom, "%s: Device: \"%s\"\n", pInfo->name, device);
	
    xf86Msg(protocolFrom, "%s: Protocol: \"%s\"\n", pInfo->name, protocol);
    if (!(pProto = GetProtocol(protocolID)))
	return pInfo;

    pMse->protocolID = protocolID;
    pMse->oldProtocolID = protocolID;  /* hack */

    pMse->autoProbe = FALSE;

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, pProto->defaults, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* XXX should handle this OS dependency elsewhere. */
#ifndef __OS2ELF__
    /* OS/2 has a mouse handled by the OS - it cannot fail here */

    /* Check if the device can be opened. */
    pInfo->fd = xf86OpenSerial(pInfo->options);
    if (pInfo->fd == -1) {
	if (xf86GetAllowMouseOpenFail())
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    xf86Msg(X_ERROR, "%s: cannot open input device\n", pInfo->name);
	    if (pMse->mousePriv)
		xfree(pMse->mousePriv);
	    xfree(pMse);
	    pInfo->private = NULL;
	    return pInfo;
	}
    }
    xf86CloseSerial(pInfo->fd);
#endif
    pInfo->fd = -1;

    if (!(mPriv = (pointer) xcalloc(sizeof(mousePrivRec), 1)))
	return pInfo;
    pMse->mousePriv = mPriv;
    pMse->CommonOptions(pInfo);
    pMse->checkMovements = checkForErraticMovements;
    pMse->autoProbeMouse = autoProbeMouse;
    pMse->collectData = collectData;
    pMse->dataGood = autoGood;
    
    MouseHWOptions(pInfo);
    MouseSerialOptions(pInfo);
    
    pInfo->flags |= XI86_CONFIGURED;
    return pInfo;
}


static void
MouseReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    int j, buttons, dx, dy, dz, dw, baddata;
    int pBufP;
    int c;
    unsigned char *pBuf, u;


    pMse = pInfo->private;
    pBufP = pMse->protoBufTail;
    pBuf = pMse->protoBuf;

    /*
     * Set blocking to -1 on the first call because we know there is data to
     * read. Xisb automatically clears it after one successful read so that
     * succeeding reads are preceeded by a select with a 0 timeout to prevent
     * read from blocking indefinitely.
     */
    XisbBlockDuration(pMse->buffer, -1);

    while ((c = XisbRead(pMse->buffer)) >= 0) {
	u = (unsigned char)c;

#if defined (EXTMOUSEDEBUG) || defined (MOUSEDATADEBUG)
	ErrorF("mouse byte: %2.2x\n",u);
#endif

#if 1
	/* if we do autoprobing collect the data */
	if (pMse->collectData && pMse->autoProbe)
	    if (pMse->collectData(pMse,u))
		continue;
#endif
#ifdef SUPPORT_MOUSE_RESET
	if (mouseReset(pInfo,u)) {
	    pBufP = 0;
	    continue;
	}
#endif
	if (pBufP >= pMse->protoPara[4]) {
	    /*
	     * Buffer contains a full packet, which has already been processed:
	     * Empty the buffer and check for optional 4th byte, which will be
	     * processed directly, without being put into the buffer first.
	     */
	    pBufP = 0;
	    if ((u & pMse->protoPara[0]) != pMse->protoPara[1] &&
		(u & pMse->protoPara[5]) == pMse->protoPara[6]) {
		/*
		 * Hack for Logitech MouseMan Mouse - Middle button
		 *
		 * Unfortunately this mouse has variable length packets: the
		 * standard Microsoft 3 byte packet plus an optional 4th byte
		 * whenever the middle button status changes.
		 *
		 * We have already processed the standard packet with the
		 * movement and button info.  Now post an event message with
		 * the old status of the left and right buttons and the
		 * updated middle button.
		 */
		/*
		 * Even worse, different MouseMen and TrackMen differ in the
		 * 4th byte: some will send 0x00/0x20, others 0x01/0x21, or
		 * even 0x02/0x22, so I have to strip off the lower bits.
		 * [CHRIS-211092]
		 *
		 * [JCH-96/01/21]
		 * HACK for ALPS "fourth button".  (It's bit 0x10 of the
		 * "fourth byte" and it is activated by tapping the glidepad
		 * with the finger! 8^) We map it to bit bit3, and the
		 * reverse map in xf86Events just has to be extended so that
		 * it is identified as Button 4.  The lower half of the
		 * reverse-map may remain unchanged.
		 */
		/*
		 * [KAZU-030897]
		 * Receive the fourth byte only when preceeding three bytes
		 * have been detected (pBufP >= pMse->protoPara[4]).  In the
		 * previous versions, the test was pBufP == 0; we may have
		 * mistakingly received a byte even if we didn't see anything
		 * preceeding the byte.
		 */
#ifdef EXTMOUSEDEBUG
		ErrorF("mouse 4th byte %02x\n",u);
#endif
		dx = dy = dz = dw = 0;
		buttons = 0;
		switch (pMse->protocolID) {

		/*
		 * [KAZU-221197]
		 * IntelliMouse, NetMouse (including NetMouse Pro) and Mie
		 * Mouse always send the fourth byte, whereas the fourth byte
		 * is optional for GlidePoint and ThinkingMouse.  The fourth
		 * byte is also optional for MouseMan+ and FirstMouse+ in
		 * their native mode.  It is always sent if they are in the
		 * IntelliMouse compatible mode.
		 */ 
		case PROT_IMSERIAL:	/* IntelliMouse, NetMouse, Mie Mouse, 
					   MouseMan+ */
		    dz = (u & 0x08) ?
				(u & 0x0f) - 16 : (u & 0x0f);
		    if ((dz >= 7) || (dz <= -7))
			dz = 0;
		    buttons |=  ((int)(u & 0x10) >> 3)
			      | ((int)(u & 0x20) >> 2) 
			      | (pMse->lastButtons & 0x05);
		    break;

		case PROT_GLIDE:
		case PROT_THINKING:
		    buttons |= ((int)(u & 0x10) >> 1);
		    /* fall through */

		default:
		    buttons |= ((int)(u & 0x20) >> 4) |
			       (pMse->lastButtons & 0x05);
		    break;
		}
		goto post_event;
	    }
	}
	/* End of packet buffer flush and 4th byte hack. */

	/*
	 * Append next byte to buffer (which is empty or contains an
	 * incomplete packet); iterate if packet (still) not complete.
	 */
	pBuf[pBufP++] = u;
	if (pBufP != pMse->protoPara[4]) continue;
#ifdef EXTMOUSEDEBUG2
	{
	    int i;
	    ErrorF("received %d bytes",pBufP);
	    for ( i=0; i < pBufP; i++)
		ErrorF(" %02x",pBuf[i]);
	    ErrorF("\n");
	}
#endif

	/*
	 * Hack for resyncing: We check here for a package that is:
	 *  a) illegal (detected by wrong data-package header)
	 *  b) invalid (0x80 == -128 and that might be wrong for MouseSystems)
	 *  c) bad header-package
	 *
	 * NOTE: b) is a violation of the MouseSystems-Protocol, since values
	 *       of -128 are allowed, but since they are very seldom we can
	 *       easily  use them as package-header with no button pressed.
	 * NOTE/2: On a PS/2 mouse any byte is valid as a data byte.
	 *       Furthermore, 0x80 is not valid as a header byte. For a PS/2
	 *       mouse we skip checking data bytes.  For resyncing a PS/2
	 *       mouse we require the two most significant bits in the header
	 *       byte to be 0. These are the overflow bits, and in case of
	 *       an overflow we actually lose sync. Overflows are very rare,
	 *       however, and we quickly gain sync again after an overflow
	 *       condition. This is the best we can do. (Actually, we could
	 *       use bit 0x08 in the header byte for resyncing, since that
	 *       bit is supposed to be always on, but nobody told Microsoft...)
	 */
	
	/*
	 * [KAZU,OYVIND-120398]
	 * The above hack is wrong!  Because of b) above, we shall see
	 * erroneous mouse events so often when the MouseSystem mouse is
	 * moved quickly.  As for the PS/2 and its variants, we don't need 
	 * to treat them as special cases, because protoPara[2] and 
	 * protoPara[3] are both 0x00 for them, thus, any data bytes will 
	 * never be discarded.  0x80 is rejected for MMSeries, Logitech 
	 * and MMHittab protocols, because protoPara[2] and protoPara[3] 
	 * are 0x80 and 0x00 respectively.  The other protocols are 7-bit 
	 * protocols; there is no use checking 0x80.  
	 * 
	 * All in all we should check the condition a) only.
	 */

	/*
	 * [OYVIND-120498]
	 * Check packet for valid data:
	 * If driver is in sync with datastream, the packet is considered
	 * bad if any byte (header and/or data) contains an invalid value.
	 * 
	 * If packet is bad, we discard the first byte and shift the buffer.
	 * Next iteration will then check the new situation for validity.
	 * 
	 * If flag MF_SAFE is set in proto[7] and the driver
	 * is out of sync, the packet is also considered bad if
	 * any of the data bytes contains a valid header byte value.
	 * This situation could occur if the buffer contains
	 * the tail of one packet and the header of the next.
	 *
	 * Note: The driver starts in out-of-sync mode (pMse->inSync = 0).
	 */

	baddata = 0;

	/* All databytes must be valid. */
	for (j = 1; j < pBufP; j++ )
	    if ((pBuf[j] & pMse->protoPara[2]) != pMse->protoPara[3])
		baddata = 1;

	/* If out of sync, don't mistake a header byte for data. */
	if ((pMse->protoPara[7] & MPF_SAFE) && !pMse->inSync)
	    for (j = 1; j < pBufP; j++ )
		if ((pBuf[j] & pMse->protoPara[0]) == pMse->protoPara[1])
		    baddata = 1;

	/* Accept or reject the packet ? */
	if ((pBuf[0] & pMse->protoPara[0]) != pMse->protoPara[1] || baddata) {
	    if (pMse->inSync) {
#ifdef EXTMOUSEDEBUG
		ErrorF("mouse driver lost sync\n");
#endif
	    }
#ifdef EXTMOUSEDEBUG
	    ErrorF("skipping byte %02x\n",*pBuf);
#endif
	    /* Tell auto probe that we are out of sync */
	    if (pMse->autoProbeMouse && pMse->autoProbe) 
		pMse->autoProbeMouse(pInfo, FALSE, pMse->inSync);
	    pMse->protoBufTail = --pBufP;
	    for (j = 0; j < pBufP; j++)
		pBuf[j] = pBuf[j+1];
	    pMse->inSync = 0;
	    continue;
	}
	/* Tell auto probe that we were successful */
	if (pMse->autoProbeMouse && pMse->autoProbe) 
	    pMse->autoProbeMouse(pInfo, TRUE, FALSE);
	
	if (!pMse->inSync) {
#ifdef EXTMOUSEDEBUG
	    ErrorF("mouse driver back in sync\n");
#endif
	    pMse->inSync = 1;
	}

  	if (!pMse->dataGood(pMse))
  	    continue;
	
	/*
	 * Packet complete and verified, now process it ...
	 */
    REDO_INTERPRET:
	dz = dw = 0;
	switch (pMse->protocolID) {
	case PROT_LOGIMAN:	/* MouseMan / TrackMan   [CHRIS-211092] */
	case PROT_MS:		/* Microsoft */
	    if (pMse->chordMiddle)
		buttons = (((int) pBuf[0] & 0x30) == 0x30) ? 2 :
				  ((int)(pBuf[0] & 0x20) >> 3)
				| ((int)(pBuf[0] & 0x10) >> 4);
	    else
        	buttons = (pMse->lastButtons & 2)
			| ((int)(pBuf[0] & 0x20) >> 3)
			| ((int)(pBuf[0] & 0x10) >> 4);
	    dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
	    dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
	    break;

	case PROT_GLIDE:	/* ALPS GlidePoint */
	case PROT_THINKING:	/* ThinkingMouse */
	case PROT_IMSERIAL:	/* IntelliMouse, NetMouse, Mie Mouse, MouseMan+ */
	    buttons =  (pMse->lastButtons & (8 + 2))
		     | ((int)(pBuf[0] & 0x20) >> 3)
		     | ((int)(pBuf[0] & 0x10) >> 4);
	    dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
	    dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
	    break;

	case PROT_MSC:		/* Mouse Systems Corp */
	    buttons = (~pBuf[0]) & 0x07;
	    dx =    (char)(pBuf[1]) + (char)(pBuf[3]);
	    dy = - ((char)(pBuf[2]) + (char)(pBuf[4]));
	    break;
      
	case PROT_MMHIT:	/* MM_HitTablet */
	    buttons = pBuf[0] & 0x07;
	    if (buttons != 0)
		buttons = 1 << (buttons - 1);
	    dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
	    dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
	    break;

	case PROT_ACECAD:	/* ACECAD */
	    /* ACECAD is almost exactly like MM but the buttons are different */
	    buttons = (pBuf[0] & 0x02) | ((pBuf[0] & 0x04) >> 2) |
		      ((pBuf[0] & 1) << 2);
	    dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
	    dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
	    break;

	case PROT_MM:		/* MM Series */
	case PROT_LOGI:		/* Logitech Mice */
	    buttons = pBuf[0] & 0x07;
	    dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
	    dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
	    break;

	case PROT_BM:		/* BusMouse */
	    buttons = (~pBuf[0]) & 0x07;
	    dx =   (char)pBuf[1];
	    dy = - (char)pBuf[2];
	    break;

	case PROT_PS2:		/* PS/2 mouse */
	case PROT_GENPS2:	/* generic PS/2 mouse */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2;        /* Left */
	    dx = (pBuf[0] & 0x10) ?    (int)pBuf[1]-256  :  (int)pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -((int)pBuf[2]-256) : -(int)pBuf[2];
	    break;

	/* PS/2 mouse variants */
	case PROT_IMPS2:	/* IntelliMouse PS/2 */
	case PROT_NETPS2:	/* NetMouse PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      (pBuf[0] & 0x40) >> 3 |       /* button 4 */
		      (pBuf[0] & 0x80) >> 3;        /* button 5 */
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    /*
	     * The next cast must be 'signed char' for platforms (like PPC)
	     * where char defaults to unsigned.
	     */
	    dz = (signed char)(pBuf[3] | ((pBuf[3] & 0x08) ? 0xf8 : 0));
	    if ((pBuf[3] & 0xf8) && ((pBuf[3] & 0xf8) != 0xf8)) {
		if (pMse->autoProbe) {
		    SetMouseProto(pMse, PROT_EXPPS2);
		    xf86Msg(X_INFO,
			    "Mouse autoprobe: Changing protocol to %s\n",
			    pMse->protocol); 
		    
		    goto REDO_INTERPRET; 
		} else  
		    dz = 0;
	    }
	    break;

	case PROT_EXPPS2:	/* IntelliMouse Explorer PS/2 */
	    if (pMse->autoProbe && (pBuf[3] & 0xC0)) {
		SetMouseProto(pMse, PROT_IMPS2);
		xf86Msg(X_INFO,"Mouse autoprobe: Changing protocol to %s\n",
			pMse->protocol); 
		goto REDO_INTERPRET;
	    }
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      (pBuf[3] & 0x10) >> 1 |       /* button 4 */
		      (pBuf[3] & 0x20) >> 1;        /* button 5 */
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    dz = (pBuf[3] & 0x08) ? (pBuf[3] & 0x0f) - 16 : (pBuf[3] & 0x0f);
	    break;

	case PROT_MMPS2:	/* MouseMan+ PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2;        /* Left */
	    dx = (pBuf[0] & 0x10) ? pBuf[1] - 256 : pBuf[1];
	    if (((pBuf[0] & 0x48) == 0x48) &&
		(abs(dx) > 191) &&
		((((pBuf[2] & 0x03) << 2) | 0x02) == (pBuf[1] & 0x0f))) {
		/* extended data packet */
		switch ((((pBuf[0] & 0x30) >> 2) | ((pBuf[1] & 0x30) >> 4))) {
		case 1:		/* wheel data packet */
		    buttons |= ((pBuf[2] & 0x10) ? 0x08 : 0) | /* 4th button */
		               ((pBuf[2] & 0x20) ? 0x10 : 0);  /* 5th button */
		    dx = dy = 0;
		    dz = (pBuf[2] & 0x08) ? (pBuf[2] & 0x0f) - 16 :
					    (pBuf[2] & 0x0f);
		    break;
		case 2:		/* Logitech reserves this packet type */
		    /* 
		     * IBM ScrollPoint uses this packet to encode its
		     * stick movement.
		     */
		    buttons |= (pMse->lastButtons & ~0x07);
		    dx = dy = 0;
		    dz = (pBuf[2] & 0x80) ? ((pBuf[2] >> 4) & 0x0f) - 16 :
					    ((pBuf[2] >> 4) & 0x0f);
		    dw = (pBuf[2] & 0x08) ? (pBuf[2] & 0x0f) - 16 :
					    (pBuf[2] & 0x0f);
		    break;
		case 0:		/* device type packet - shouldn't happen */
		default:
		    buttons |= (pMse->lastButtons & ~0x07);
		    dx = dy = 0;
		    dz = 0;
		    break;
		}
	    } else {
		buttons |= (pMse->lastButtons & ~0x07);
		dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
		dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    }
	    break;

	case PROT_GLIDEPS2:	/* GlidePoint PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      ((pBuf[0] & 0x08) ? 0 : 0x08);/* fourth button */
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    break;

	case PROT_NETSCPS2:	/* NetScroll PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      ((pBuf[3] & 0x02) ? 0x08 : 0) | /* button 4 */
		      ((pBuf[3] & 0x01) ? 0x10 : 0);  /* button 5 */
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    dz = (pBuf[3] & 0x10) ? pBuf[4] - 256 : pBuf[4];
	    break;

	case PROT_THINKPS2:	/* ThinkingMouse PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      ((pBuf[0] & 0x08) ? 0x08 : 0);/* fourth button */
	    pBuf[1] |= (pBuf[0] & 0x40) ? 0x80 : 0x00;
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    break;

	case PROT_SYSMOUSE:	/* sysmouse */
	    buttons = (~pBuf[0]) & 0x07;
	    dx =    (char)(pBuf[1]) + (char)(pBuf[3]);
	    dy = - ((char)(pBuf[2]) + (char)(pBuf[4]));
	    /* FreeBSD sysmouse sends additional data bytes */
	    if (pMse->protoPara[4] >= 8) {
		/*
		 * These casts must be 'signed char' for platforms (like PPC)
		 * where char defaults to unsigned.
		 */
		dz = ((signed char)(pBuf[5] << 1) +
		      (signed char)(pBuf[6] << 1)) >> 1;
		buttons |= (int)(~pBuf[7] & 0x7f) << 3;
	    }
	    break;

	case PROT_VALUMOUSESCROLL:	/* Kensington ValuMouseScroll */
            buttons = ((int)(pBuf[0] & 0x20) >> 3)
                      | ((int)(pBuf[0] & 0x10) >> 4)
                      | ((int)(pBuf[3] & 0x10) >> 3);
            dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
            dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
	    dz = (pBuf[3] & 0x08) ? ((int)(pBuf[3] & 0x0F) - 0x10) : 
                                    ((int)(pBuf[3] & 0x0F));
	    break;

	default: /* There's a table error */
#ifdef EXTMOUSEDEBUG
	    ErrorF("mouse table error\n");
#endif
	    continue;
	}
#ifdef EXTMOUSEDEBUG
	ErrorF("packet");
	for ( j=0; j < pBufP; j++)
	    ErrorF(" %02x",pBuf[j]);
	ErrorF("\n");
#endif

post_event:
#ifdef EXTMOUSEDEBUG
	ErrorF("dx=%i dy=%i dz=%i dw=%i buttons=%x\n",dx,dy,dz,dw,buttons);
#endif
	/* When auto-probing check if data makes sense */
	if (pMse->checkMovements && pMse->autoProbe)
	    pMse->checkMovements(pInfo,dx,dy);
	/* post an event */
	pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);

	/* 
	 * We don't reset pBufP here yet, as there may be an additional data
	 * byte in some protocols. See above.
	 */
    }
    pMse->protoBufTail = pBufP;
}

/*
 * MouseCtrl --
 *      Alter the control parameters for the mouse. Note that all special
 *      protocol values are handled by dix.
 */

static void
MouseCtrl(DeviceIntPtr device, PtrCtrl *ctrl)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;

    pInfo = device->public.devicePrivate;
    pMse = pInfo->private;

#ifdef EXTMOUSEDEBUG
    ErrorF("MouseCtrl pMse=%p\n", pMse);
#endif
    
    pMse->num       = ctrl->num;
    pMse->den       = ctrl->den;
    pMse->threshold = ctrl->threshold;
}

/*
 ***************************************************************************
 *
 * MouseProc --
 *
 ***************************************************************************
 */

static int
MouseProc(DeviceIntPtr device, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    mousePrivPtr mPriv;
    unsigned char map[MSE_MAXBUTTONS + 1];
    int i;
    
    pInfo = device->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = device;

    switch (what)
    {
    case DEVICE_INIT:
	device->public.on = FALSE;
	/*
	 * [KAZU-241097] We don't know exactly how many buttons the
	 * device has, so setup the map with the maximum number.
	 */
	for (i = 0; i < MSE_MAXBUTTONS; i++)
	    map[i + 1] = i + 1;

	InitPointerDeviceStruct((DevicePtr)device, map,
				min(pMse->buttons, MSE_MAXBUTTONS),
				miPointerGetMotionEvents, pMse->Ctrl,
				miPointerGetMotionBufferSize());

	/* X valuator */
	xf86InitValuatorAxisStruct(device, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(device, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(device, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(device, 1);
	xf86MotionHistoryAllocate(pInfo);

#ifdef EXTMOUSEDEBUG
	ErrorF("assigning %p atom=%d name=%s\n", device, pInfo->atom,
		pInfo->name);
#endif
	break;

    case DEVICE_ON:
	pInfo->fd = xf86OpenSerial(pInfo->options);
	if (pInfo->fd == -1)
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    if (pMse->xisbscale)
		pMse->buffer = XisbNew(pInfo->fd, pMse->xisbscale * 4);
	    else
		pMse->buffer = XisbNew(pInfo->fd, 64);
	    if (!pMse->buffer) {
		xf86CloseSerial(pInfo->fd);
		pInfo->fd = -1;
	    } else {
		if (!SetupMouse(pInfo)) {
		    xf86CloseSerial(pInfo->fd);
		    pInfo->fd = -1;
		    XisbFree(pMse->buffer);
		    pMse->buffer = NULL;
		} else {
		    mPriv = (mousePrivPtr)pMse->mousePriv;
		    if (mPriv != NULL) {
			if ( pMse->protocolID != PROT_AUTO) {
			    pMse->inSync = TRUE; /* @@@ */
			    if (mPriv->soft)
				mPriv->autoState = AUTOPROBE_GOOD;
			    else
				mPriv->autoState = AUTOPROBE_H_GOOD;
			} else {
			    if (mPriv->soft)
				mPriv->autoState = AUTOPROBE_NOPROTO;
			    else
				mPriv->autoState = AUTOPROBE_H_NOPROTO;
			}
		    }
		    xf86FlushInput(pInfo->fd);
		    xf86AddEnabledDevice(pInfo);
		}
	    }
	}
	pMse->lastButtons = 0;
	pMse->emulateState = 0;
	pMse->emulate3Pending = FALSE;
	device->public.on = TRUE;
	FlushButtons(pMse);
	if (pMse->emulate3Buttons || pMse->emulate3ButtonsSoft)
	{
	    RegisterBlockAndWakeupHandlers (MouseBlockHandler, MouseWakeupHandler,
					    (pointer) pInfo);
	}
	break;
	    
    case DEVICE_OFF:
    case DEVICE_CLOSE:
	if (pInfo->fd != -1) {
	    xf86RemoveEnabledDevice(pInfo);
	    if (pMse->buffer) {
		XisbFree(pMse->buffer);
		pMse->buffer = NULL;
	    }
	    xf86CloseSerial(pInfo->fd);
	    pInfo->fd = -1;
	    if (pMse->emulate3Buttons || pMse->emulate3ButtonsSoft)
	    {
		RemoveBlockAndWakeupHandlers (MouseBlockHandler, MouseWakeupHandler,
					      (pointer) pInfo);
	    }
	}
	device->public.on = FALSE;
	usleep(300000);
	break;
    }
    return Success;
}

/*
 ***************************************************************************
 *
 * MouseConvert --
 *	Convert valuators to X and Y.
 *
 ***************************************************************************
 */
static Bool
MouseConvert(InputInfoPtr pInfo, int first, int num, int v0, int v1, int v2,
	     int v3, int v4, int v5, int *x, int *y)
{
    if (first != 0 || num != 2)
	return FALSE;

    *x = v0;
    *y = v1;

    return TRUE;
}

/**********************************************************************
 *
 * FlushButtons -- send button up events for sanity.
 *
 **********************************************************************/

static void
FlushButtons(MouseDevPtr pMse)
{

    /* If no button down is pending xf86PostButtonEvent()
     * will discard them. So we are on the safe side. */

    int i, blocked;

    pMse->lastButtons = 0;

    blocked = xf86BlockSIGIO ();
    for (i = 1; i <= 5; i++)
	xf86PostButtonEvent(pMse->device,0,i,0,0,0);
    xf86UnblockSIGIO (blocked);
}

/**********************************************************************
 *
 *  Emulate3Button support code
 *
 **********************************************************************/


/*
 * Lets create a simple finite-state machine for 3 button emulation:
 *
 * We track buttons 1 and 3 (left and right).  There are 11 states:
 *   0 ground           - initial state
 *   1 delayed left     - left pressed, waiting for right
 *   2 delayed right    - right pressed, waiting for left
 *   3 pressed middle   - right and left pressed, emulated middle sent
 *   4 pressed left     - left pressed and sent
 *   5 pressed right    - right pressed and sent
 *   6 released left    - left released after emulated middle
 *   7 released right   - right released after emulated middle
 *   8 repressed left   - left pressed after released left
 *   9 repressed right  - right pressed after released right
 *  10 pressed both     - both pressed, not emulating middle
 *
 * At each state, we need handlers for the following events
 *   0: no buttons down
 *   1: left button down
 *   2: right button down
 *   3: both buttons down
 *   4: emulate3Timeout passed without a button change
 * Note that button events are not deltas, they are the set of buttons being
 * pressed now.  It's possible (ie, mouse hardware does it) to go from (eg)
 * left down to right down without anything in between, so all cases must be
 * handled.
 *
 * a handler consists of three values:
 *   0: action1
 *   1: action2
 *   2: new emulation state
 *
 * action > 0: ButtonPress
 * action = 0: nothing
 * action < 0: ButtonRelease
 *
 * The comment preceeding each section is the current emulation state.
 * The comments to the right are of the form
 *      <button state> (<events>) -> <new emulation state>
 * which should be read as
 *      If the buttons are in <button state>, generate <events> then go to
 *      <new emulation state>.
 */
static signed char stateTab[11][5][3] = {
/* 0 ground */
  {
    {  0,  0,  0 },   /* nothing -> ground (no change) */
    {  0,  0,  1 },   /* left -> delayed left */
    {  0,  0,  2 },   /* right -> delayed right */
    {  2,  0,  3 },   /* left & right (middle press) -> pressed middle */
    {  0,  0, -1 }    /* timeout N/A */
  },
/* 1 delayed left */
  {
    {  1, -1,  0 },   /* nothing (left event) -> ground */
    {  0,  0,  1 },   /* left -> delayed left (no change) */
    {  1, -1,  2 },   /* right (left event) -> delayed right */
    {  2,  0,  3 },   /* left & right (middle press) -> pressed middle */
    {  1,  0,  4 },   /* timeout (left press) -> pressed left */
  },
/* 2 delayed right */
  {
    {  3, -3,  0 },   /* nothing (right event) -> ground */
    {  3, -3,  1 },   /* left (right event) -> delayed left (no change) */
    {  0,  0,  2 },   /* right -> delayed right (no change) */
    {  2,  0,  3 },   /* left & right (middle press) -> pressed middle */
    {  3,  0,  5 },   /* timeout (right press) -> pressed right */
  },
/* 3 pressed middle */
  {
    { -2,  0,  0 },   /* nothing (middle release) -> ground */
    {  0,  0,  7 },   /* left -> released right */
    {  0,  0,  6 },   /* right -> released left */
    {  0,  0,  3 },   /* left & right -> pressed middle (no change) */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 4 pressed left */
  {
    { -1,  0,  0 },   /* nothing (left release) -> ground */
    {  0,  0,  4 },   /* left -> pressed left (no change) */
    { -1,  0,  2 },   /* right (left release) -> delayed right */
    {  3,  0, 10 },   /* left & right (right press) -> pressed both */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 5 pressed right */
  {
    { -3,  0,  0 },   /* nothing (right release) -> ground */
    { -3,  0,  1 },   /* left (right release) -> delayed left */
    {  0,  0,  5 },   /* right -> pressed right (no change) */
    {  1,  0, 10 },   /* left & right (left press) -> pressed both */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 6 released left */
  {
    { -2,  0,  0 },   /* nothing (middle release) -> ground */
    { -2,  0,  1 },   /* left (middle release) -> delayed left */
    {  0,  0,  6 },   /* right -> released left (no change) */
    {  1,  0,  8 },   /* left & right (left press) -> repressed left */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 7 released right */
  {
    { -2,  0,  0 },   /* nothing (middle release) -> ground */
    {  0,  0,  7 },   /* left -> released right (no change) */
    { -2,  0,  2 },   /* right (middle release) -> delayed right */
    {  3,  0,  9 },   /* left & right (right press) -> repressed right */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 8 repressed left */
  {
    { -2, -1,  0 },   /* nothing (middle release, left release) -> ground */
    { -2,  0,  4 },   /* left (middle release) -> pressed left */
    { -1,  0,  6 },   /* right (left release) -> released left */
    {  0,  0,  8 },   /* left & right -> repressed left (no change) */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 9 repressed right */
  {
    { -2, -3,  0 },   /* nothing (middle release, right release) -> ground */
    { -3,  0,  7 },   /* left (right release) -> released right */
    { -2,  0,  5 },   /* right (middle release) -> pressed right */
    {  0,  0,  9 },   /* left & right -> repressed right (no change) */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 10 pressed both */
  {
    { -1, -3,  0 },   /* nothing (left release, right release) -> ground */
    { -3,  0,  4 },   /* left (right release) -> pressed left */
    { -1,  0,  5 },   /* right (left release) -> pressed right */
    {  0,  0, 10 },   /* left & right -> pressed both (no change) */
    {  0,  0, -1 },   /* timeout N/A */
  },
};

/*
 * Table to allow quick reversal of natural button mapping to correct mapping
 */

/*
 * [JCH-96/01/21] The ALPS GlidePoint pad extends the MS protocol
 * with a fourth button activated by tapping the PAD.
 * The 2nd line corresponds to 4th button on; the drv sends
 * the buttons in the following map (MSBit described first) :
 * 0 | 4th | 1st | 2nd | 3rd
 * And we remap them (MSBit described first) :
 * 0 | 4th | 3rd | 2nd | 1st
 */
static char reverseMap[32] = { 0,  4,  2,  6,  1,  5,  3,  7,
			       8, 12, 10, 14,  9, 13, 11, 15,
			      16, 20, 18, 22, 17, 21, 19, 23,
			      24, 28, 26, 30, 25, 29, 27, 31};


static char hitachMap[16] = {  0,  2,  1,  3, 
			       8, 10,  9, 11,
			       4,  6,  5,  7,
			      12, 14, 13, 15 };

#define reverseBits(map, b)	(((b) & ~0x0f) | map[(b) & 0x0f])

static CARD32
buttonTimer(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    int	sigstate;
    int id;

    pMse = pInfo->private;

    sigstate = xf86BlockSIGIO ();

    pMse->emulate3Pending = FALSE;
    if ((id = stateTab[pMse->emulateState][4][0]) != 0) {
        xf86PostButtonEvent(pInfo->dev, 0, abs(id), (id >= 0), 0, 0);
        pMse->emulateState = stateTab[pMse->emulateState][4][2];
    } else {
        ErrorF("Got unexpected buttonTimer in state %d\n", pMse->emulateState);
    }

    xf86UnblockSIGIO (sigstate);
    return 0;
}

static Bool
Emulate3ButtonsSoft(InputInfoPtr pInfo)
{
    MouseDevPtr pMse = pInfo->private;

    if (!pMse->emulate3ButtonsSoft)
	return TRUE;

    if (pMse->emulate3Pending)
	buttonTimer(pInfo);

    return FALSE;
}

static void MouseBlockHandler(pointer data,
			      struct timeval **waitTime,
			      pointer LastSelectMask)
{
    InputInfoPtr    pInfo = (InputInfoPtr) data;
    MouseDevPtr	    pMse = (MouseDevPtr) pInfo->private;
    int		    ms;

    if (pMse->emulate3Pending)
    {
	ms = pMse->emulate3Expires - GetTimeInMillis ();
	if (ms <= 0)
	    ms = 0;
	AdjustWaitForDelay (waitTime, ms);
    }
}

static void MouseWakeupHandler(pointer data,
			       int i,
			       pointer LastSelectMask)
{
    InputInfoPtr    pInfo = (InputInfoPtr) data;
    MouseDevPtr	    pMse = (MouseDevPtr) pInfo->private;
    int		    ms;
    
    if (pMse->emulate3Pending)
    {
	ms = pMse->emulate3Expires - GetTimeInMillis ();
	if (ms <= 0)
	    buttonTimer (pInfo);
    }
}

/*******************************************************************
 *
 * Post mouse events
 *
 *******************************************************************/

static void
MouseDoPostEvent(InputInfoPtr pInfo, int buttons, int dx, int dy)
{
    MouseDevPtr pMse;
    int truebuttons, emulateButtons;
    int id, change;
    int emuWheelDelta, emuWheelButton, emuWheelButtonMask;

    pMse = pInfo->private;

    truebuttons = buttons;
    if (pMse->protocolID == PROT_MMHIT)
	buttons = reverseBits(hitachMap, buttons);
    else
	buttons = reverseBits(reverseMap, buttons);

    /* Intercept wheel emulation. */
    if (pMse->emulateWheel && (buttons & pMse->wheelButtonMask)) {
	/* Y axis movement */
	if (pMse->negativeY != MSE_NOAXISMAP) {
	    pMse->wheelYDistance += dy;
	    if (pMse->wheelYDistance < 0) {
		emuWheelDelta = -pMse->wheelInertia;
		emuWheelButton = pMse->negativeY;
	    } else {
		emuWheelDelta = pMse->wheelInertia;
		emuWheelButton = pMse->positiveY;
	    }
	    emuWheelButtonMask = 1 << (emuWheelButton - 1);
	    while (abs(pMse->wheelYDistance) > pMse->wheelInertia) {
		pMse->wheelYDistance -= emuWheelDelta;

		/*
		 * Synthesize the press and release, but not when the button
		 * to be synthesized is already pressed "for real".
		 */
		if (!(emuWheelButtonMask & buttons) ||
		    (emuWheelButtonMask & pMse->wheelButtonMask)) {
		    xf86PostButtonEvent(pInfo->dev, 0, emuWheelButton, 1, 0, 0);
		    xf86PostButtonEvent(pInfo->dev, 0, emuWheelButton, 0, 0, 0);
		}
	    }
	}

	/* X axis movement */
	if (pMse->negativeX != MSE_NOAXISMAP) {
	    pMse->wheelXDistance += dx;
	    if (pMse->wheelXDistance < 0) {
		emuWheelDelta = -pMse->wheelInertia;
		emuWheelButton = pMse->negativeX;
	    } else {
		emuWheelDelta = pMse->wheelInertia;
		emuWheelButton = pMse->positiveX;
	    }
	    emuWheelButtonMask = 1 << (emuWheelButton - 1);
	    while (abs(pMse->wheelXDistance) > pMse->wheelInertia) {
		pMse->wheelXDistance -= emuWheelDelta;

		/*
		 * Synthesize the press and release, but not when the button
		 * to be synthesized is already pressed "for real".
		 */
		if (!(emuWheelButtonMask & buttons) ||
		    (emuWheelButtonMask & pMse->wheelButtonMask)) {
		    xf86PostButtonEvent(pInfo->dev, 0, emuWheelButton, 1, 0, 0);
		    xf86PostButtonEvent(pInfo->dev, 0, emuWheelButton, 0, 0, 0);
		}
	    }
	}

	/* Absorb the mouse movement and the wheel button press. */
	dx = 0;
	dy = 0;
	buttons &= ~pMse->wheelButtonMask;
    }

    if (dx || dy)
    {
	xf86PostMotionEvent(pInfo->dev, 0, 0, 2, dx, dy);
        ErrorF("v0=%d, v1=%d\n", dx, dy);
    }
    if (truebuttons != pMse->lastButtons) {

	if (pMse->protocolID == PROT_MMHIT)
	    change = buttons ^ reverseBits(hitachMap, pMse->lastButtons);
	else
	    change = buttons ^ reverseBits(reverseMap, pMse->lastButtons);

	/*
	 * adjust buttons state for drag locks!
	 * if there is drag locks
	 */
        if (pMse->pDragLock) {      
	    DragLockPtr   pLock;
	    int tarOfGoingDown, tarOfDown;
	    int realbuttons;

	    /* get drag lock block */
	    pLock = pMse->pDragLock;
	    /* save real buttons */
	    realbuttons = buttons;

	    /* if drag lock used */

	    /* state of drag lock buttons not seen always up */

	    buttons &= ~pLock->lockButtonsM;

	    /*
	     * if lock buttons being depressed changes state of
	     * targets simulatedDown.
	     */
	    tarOfGoingDown = lock2targetMap(pLock,
				realbuttons & change & pLock->lockButtonsM);
	    pLock->simulatedDown ^= tarOfGoingDown;

	    /* targets of drag locks down */
	    tarOfDown = lock2targetMap(pLock,
				realbuttons & pLock->lockButtonsM);

	    /*
	     * when simulatedDown set and target pressed, 
	     * simulatedDown goes false 
	     */
	    pLock->simulatedDown &= ~(realbuttons & change);

	    /*
	     * if master drag lock released  
	     * then master drag lock state on
	     */
	    pLock->masterTS |= (~realbuttons & change) & pLock->masterLockM;

	    /* if master state, buttons going down are simulatedDown */
	    if (pLock->masterTS) 
		pLock->simulatedDown |= (realbuttons & change);

	    /* if any button pressed, no longer in master drag lock state */
	    if (realbuttons & change)
		pLock->masterTS = 0;

	    /* if simulatedDown or drag lock down, simulate down */
	    buttons |= (pLock->simulatedDown | tarOfDown);

	    /* master button not seen */
	    buttons &= ~(pLock->masterLockM);

	    /* buttons changed since last time */
	    change = buttons ^ pLock->lockLastButtons;

	    /* save this time for next last time. */
	    pLock->lockLastButtons = buttons;
	}

        if (pMse->emulate3Buttons
	    && (!(buttons & 0x02) || Emulate3ButtonsSoft(pInfo))) {

            /* handle all but buttons 1 & 3 normally */

            change &= ~05;

            /* emulate the third button by the other two */

            emulateButtons = (buttons & 01) | ((buttons &04) >> 1);

            if ((id = stateTab[pMse->emulateState][emulateButtons][0]) != 0)
                xf86PostButtonEvent(pInfo->dev, 0, abs(id), (id >= 0), 0, 0);
            if ((id = stateTab[pMse->emulateState][emulateButtons][1]) != 0)
                xf86PostButtonEvent(pInfo->dev, 0, abs(id), (id >= 0), 0, 0);

            pMse->emulateState =
                stateTab[pMse->emulateState][emulateButtons][2];

            if (stateTab[pMse->emulateState][4][0] != 0) {
		pMse->emulate3Expires = GetTimeInMillis () + pMse->emulate3Timeout;
		pMse->emulate3Pending = TRUE;
            } else {
		pMse->emulate3Pending = FALSE;
            }
        }

	while (change) {
	    id = ffs(change);
	    change &= ~(1 << (id - 1));
	    xf86PostButtonEvent(pInfo->dev, 0, id,
				(buttons & (1 << (id - 1))), 0, 0);
	}

        pMse->lastButtons = truebuttons;
    }
}

static void
MousePostEvent(InputInfoPtr pInfo, int buttons, int dx, int dy, int dz, int dw)
{
    MouseDevPtr pMse;
    int zbutton = 0;

    pMse = pInfo->private;

    /* Map the Z axis movement. */
    /* XXX Could this go in the conversion_proc? */
    switch (pMse->negativeZ) {
    case MSE_NOZMAP:	/* do nothing */
	break;
    case MSE_MAPTOX:
	if (dz != 0) {
	    dx = dz;
	    dz = 0;
	}
	break;
    case MSE_MAPTOY:
	if (dz != 0) {
	    dy = dz;
	    dz = 0;
	}
	break;
    default:	/* buttons */
	buttons &= ~(pMse->negativeZ | pMse->positiveZ
		   | pMse->negativeW | pMse->positiveW);
	if (dw < 0 || dz < -1)
	    zbutton = pMse->negativeW;
	else if (dz < 0)
	    zbutton = pMse->negativeZ;
	else if (dw > 0 || dz > 1)
	    zbutton = pMse->positiveW;
	else if (dz > 0)
	    zbutton = pMse->positiveZ;
	buttons |= zbutton;
	dz = 0;
	break;
    }

    /* Apply angle offset */
    if (pMse->angleOffset != 0) {
	double rad = 3.141592653 * pMse->angleOffset / 180.0;
	int ndx = dx;
	dx = (int)((dx * cos(rad)) + (dy * sin(rad)) + 0.5);
	dy = (int)((dy * cos(rad)) - (ndx * sin(rad)) + 0.5);
    }

    dx = pMse->invX * dx;
    dy = pMse->invY * dy;
    if (pMse->flipXY) {
	int tmp = dx;
	dx = dy;
	dy = tmp;
    }
    MouseDoPostEvent(pInfo, buttons, dx, dy);

    /*
     * If dz has been mapped to a button `down' event, we need to cook up
     * a corresponding button `up' event.
     */
    if (zbutton) {
	buttons &= ~zbutton;
	MouseDoPostEvent(pInfo, buttons, 0, 0);
    }
}
/******************************************************************
 *
 * Mouse Setup Code
 *
 ******************************************************************/
/*
 * This array is indexed by the MouseProtocolID values, so the order of the
 * entries must match that of the MouseProtocolID enum in xf86OSmouse.h.
 */
static unsigned char proto[PROT_NUMPROTOS][8] = {
  /* --header--  ---data--- packet -4th-byte-  mouse   */
  /* mask   id   mask   id  bytes  mask   id   flags   */
							    /* Serial mice */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x23, 0x00, MPF_NONE },  /* MicroSoft */
  {  0xf8, 0x80, 0x00, 0x00,  5,   0x00, 0xff, MPF_SAFE },  /* MouseSystems */
  {  0xe0, 0x80, 0x80, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* MMSeries */
  {  0xe0, 0x80, 0x80, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* Logitech */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x23, 0x00, MPF_NONE },  /* MouseMan */
  {  0xe0, 0x80, 0x80, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* MM_HitTablet */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x33, 0x00, MPF_NONE },  /* GlidePoint */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x3f, 0x00, MPF_NONE },  /* IntelliMouse */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x33, 0x00, MPF_NONE },  /* ThinkingMouse */
  {  0x80, 0x80, 0x80, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* ACECAD */
  {  0x40, 0x40, 0x40, 0x00,  4,   0x00, 0xff, MPF_NONE },  /* ValuMouseScroll */
							    /* PS/2 variants */
  {  0xc0, 0x00, 0x00, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* PS/2 mouse */
  {  0xc8, 0x08, 0x00, 0x00,  3,   0x00, 0x00, MPF_NONE },  /* genericPS/2 mouse*/
  {  0x08, 0x08, 0x00, 0x00,  4,   0x00, 0xff, MPF_NONE },  /* IntelliMouse */
  {  0x08, 0x08, 0x00, 0x00,  4,   0x00, 0xff, MPF_NONE },  /* Explorer */
  {  0x80, 0x80, 0x00, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* ThinkingMouse */
  {  0x08, 0x08, 0x00, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* MouseMan+ */
  {  0xc0, 0x00, 0x00, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* GlidePoint */
  {  0x08, 0x08, 0x00, 0x00,  4,   0x00, 0xff, MPF_NONE },  /* NetMouse */
  {  0xc0, 0x00, 0x00, 0x00,  6,   0x00, 0xff, MPF_NONE },  /* NetScroll */
							    /* Bus Mouse */
  {  0xf8, 0x80, 0x00, 0x00,  5,   0x00, 0xff, MPF_NONE },  /* BusMouse */
  {  0xf8, 0x80, 0x00, 0x00,  5,   0x00, 0xff, MPF_NONE },  /* Auto (dummy) */
  {  0xf8, 0x80, 0x00, 0x00,  8,   0x00, 0xff, MPF_NONE },  /* SysMouse */
};


/*
 * SetupMouse --
 *	Sets up the mouse parameters
 */
static Bool
SetupMouse(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    int i;
    int protoPara[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    const char *name = NULL;
    Bool automatic = FALSE;

    pMse = pInfo->private;
    
    /* Handle the "Auto" protocol. */
    if (pMse->protocolID == PROT_AUTO) {
	/* 
	 * We come here when user specifies protocol "auto" in 
	 * the configuration file or thru the xf86misc extensions.
	 * So we initialize autoprobing here.
	 * Probe for PnP/OS mouse first. If unsuccessful 
	 * try to guess protocol from incoming data.
	 */
	automatic = TRUE;
	pMse->autoProbe = TRUE;
	name = autoOSProtocol(pInfo,protoPara);
	if (name)  {
#ifdef EXTMOUSEDEBUG
	    ErrorF("PnP/OS Mouse detected: %s\n",name);
#endif	 
	}
    }

    SetMouseProto(pMse, pMse->protocolID);

    if (automatic) {
	if (name) {
	    /* Possible protoPara overrides from SetupAuto. */
	    for (i = 0; i < sizeof(pMse->protoPara); i++)
		if (protoPara[i] != -1)
		    pMse->protoPara[i] = protoPara[i];
	    /* if we come here PnP/OS mouse probing was successful */
	} else {
#if 1
	    /* PnP/OS mouse probing wasn't successful; we look at data */
#else
  	    xf86Msg(X_ERROR, "%s: cannot determine the mouse protocol\n",
		    pInfo->name);
	    return FALSE;
#endif
	}
    }

    /*
     * If protocol has changed fetch the default options
     * for the new protocol.
     */
    if (pMse->oldProtocolID != pMse->protocolID) {
	pointer tmp = NULL;
	if ((pMse->protocolID >= 0)
	    && (pMse->protocolID < PROT_NUMPROTOS)
	    && mouseProtocols[pMse->protocolID].defaults)
	    tmp = xf86OptionListCreate(
		mouseProtocols[pMse->protocolID].defaults, -1, 0);
	pInfo->options = xf86OptionListMerge(pInfo->options, tmp);
	/*
	 * If baudrate is set write it back to the option
	 * list so that the serial interface code can access
	 * the new value. Not set means default.
	 */ 
	if (pMse->baudRate)
	    xf86ReplaceIntOption(pInfo->options, "BaudRate", pMse->baudRate);
	pMse->oldProtocolID = pMse->protocolID; /* hack */
    }


    /* Set the port parameters. */
    if (!automatic)
	xf86SetSerial(pInfo->fd, pInfo->options);

    if (!initMouseHW(pInfo))
	return FALSE;    

    pMse->protoBufTail = 0;
    pMse->inSync = 0;

    return TRUE;
}

/********************************************************************
 *
 * Mouse HW setup code
 *
 ********************************************************************/

/*
** The following lines take care of the Logitech MouseMan protocols.
** The "Logitech" protocol is for the old "series 9" Logitech products.
** All products since then use the "MouseMan" protocol.  Some models
** were programmable, but most (all?) of the current models are not.
**
** NOTE: There are different versions of both MouseMan and TrackMan!
**       Hence I add another protocol PROT_LOGIMAN, which the user can
**       specify as MouseMan in his XF86Config file. This entry was
**       formerly handled as a special case of PROT_MS. However, people
**       who don't have the middle button problem, can still specify
**       Microsoft and use PROT_MS.
**
** By default, these mice should use a 3 byte Microsoft protocol
** plus a 4th byte for the middle button. However, the mouse might
** have switched to a different protocol before we use it, so I send
** the proper sequence just in case.
**
** NOTE: - all commands to (at least the European) MouseMan have to
**         be sent at 1200 Baud.
**       - each command starts with a '*'.
**       - whenever the MouseMan receives a '*', it will switch back
**	 to 1200 Baud. Hence I have to select the desired protocol
**	 first, then select the baud rate.
**
** The protocols supported by the (European) MouseMan are:
**   -  5 byte packed binary protocol, as with the Mouse Systems
**      mouse. Selected by sequence "*U".
**   -  2 button 3 byte MicroSoft compatible protocol. Selected
**      by sequence "*V".
**   -  3 button 3+1 byte MicroSoft compatible protocol (default).
**      Selected by sequence "*X".
**
** The following baud rates are supported:
**   -  1200 Baud (default). Selected by sequence "*n".
**   -  9600 Baud. Selected by sequence "*q".
**
** Selecting a sample rate is no longer supported with the MouseMan!
**               [CHRIS-211092]
*/

/*
 * Do a reset wrap mode before reset.
 */
#define do_ps2Reset(x)  { \
    int i = RETRY_COUNT;\
     while (i-- > 0) { \
       xf86FlushInput(x->fd); \
       if (ps2Reset(x)) break; \
    } \
  }

		       
static Bool
initMouseHW(InputInfoPtr pInfo)
{
    MouseDevPtr pMse = pInfo->private;
    const char *s;
    unsigned char c;
    int speed;
    pointer options;
    unsigned char *param = NULL;
    int paramlen = 0;
    int count = RETRY_COUNT;
    Bool ps2Init = TRUE;
    
    switch (pMse->protocolID) {
	case PROT_LOGI:		/* Logitech Mice */
	    /* 
	     * The baud rate selection command must be sent at the current
	     * baud rate; try all likely settings.
	     */
	    speed = pMse->baudRate;
	    switch (speed) {
		case 9600:
		    s = "*q";
		    break;
		case 4800:
		    s = "*p";
		    break;
		case 2400:
		    s = "*o";
		    break;
		case 1200:
		    s = "*n";
		    break;
		default:
		    /* Fallback value */
		    speed = 1200;
		    s = "*n";
	    }
	    xf86SetSerialSpeed(pInfo->fd, 9600);
	    xf86WriteSerial(pInfo->fd, s, 2);
	    usleep(100000);
	    xf86SetSerialSpeed(pInfo->fd, 4800);
	    xf86WriteSerial(pInfo->fd, s, 2);
	    usleep(100000);
	    xf86SetSerialSpeed(pInfo->fd, 2400);
	    xf86WriteSerial(pInfo->fd, s, 2);
	    usleep(100000);
	    xf86SetSerialSpeed(pInfo->fd, 1200);
	    xf86WriteSerial(pInfo->fd, s, 2);
	    usleep(100000);
	    xf86SetSerialSpeed(pInfo->fd, speed);

	    /* Select MM series data format. */
	    xf86WriteSerial(pInfo->fd, "S", 1);
	    usleep(100000);
	    /* Set the parameters up for the MM series protocol. */
	    options = pInfo->options;
	    xf86CollectInputOptions(pInfo, mmDefaults, NULL);
	    xf86SetSerial(pInfo->fd, pInfo->options);
	    pInfo->options = options;

	    /* Select report rate/frequency. */
	    if      (pMse->sampleRate <=   0)  c = 'O';  /* 100 */
	    else if (pMse->sampleRate <=  15)  c = 'J';  /*  10 */
	    else if (pMse->sampleRate <=  27)  c = 'K';  /*  20 */
	    else if (pMse->sampleRate <=  42)  c = 'L';  /*  35 */
	    else if (pMse->sampleRate <=  60)  c = 'R';  /*  50 */
	    else if (pMse->sampleRate <=  85)  c = 'M';  /*  67 */
	    else if (pMse->sampleRate <= 125)  c = 'Q';  /* 100 */
	    else                               c = 'N';  /* 150 */
	    xf86WriteSerial(pInfo->fd, &c, 1);
	    break;

	case PROT_LOGIMAN:
	    speed = pMse->baudRate;
	    switch (speed) {
		case 9600:
		    s = "*q";
		    break;
		case 1200:
		    s = "*n";
		    break;
		default:
		    /* Fallback value */
		    speed = 1200;
		    s = "*n";
	    }
	    xf86SetSerialSpeed(pInfo->fd, 1200);
	    xf86WriteSerial(pInfo->fd, "*n", 2);
	    xf86WriteSerial(pInfo->fd, "*X", 2);
	    xf86WriteSerial(pInfo->fd, s, 2);
	    usleep(100000);
	    xf86SetSerialSpeed(pInfo->fd, speed);
	    break;

	case PROT_MMHIT:		/* MM_HitTablet */
	    /*
	     * Initialize Hitachi PUMA Plus - Model 1212E to desired settings.
	     * The tablet must be configured to be in MM mode, NO parity,
	     * Binary Format.  pMse->sampleRate controls the sensitivity
	     * of the tablet.  We only use this tablet for it's 4-button puck
	     * so we don't run in "Absolute Mode".
	     */
	    xf86WriteSerial(pInfo->fd, "z8", 2);	/* Set Parity = "NONE" */
	    usleep(50000);
	    xf86WriteSerial(pInfo->fd, "zb", 2);	/* Set Format = "Binary" */
	    usleep(50000);
	    xf86WriteSerial(pInfo->fd, "@", 1);	/* Set Report Mode = "Stream" */
	    usleep(50000);
	    xf86WriteSerial(pInfo->fd, "R", 1);	/* Set Output Rate = "45 rps" */
	    usleep(50000);
	    xf86WriteSerial(pInfo->fd, "I\x20", 2);	/* Set Incrememtal Mode "20" */
	    usleep(50000);
	    xf86WriteSerial(pInfo->fd, "E", 1);	/* Set Data Type = "Relative */
	    usleep(50000);
	    /*
	     * These sample rates translate to 'lines per inch' on the Hitachi
	     * tablet.
	     */
	    if      (pMse->sampleRate <=   40) c = 'g';
	    else if (pMse->sampleRate <=  100) c = 'd';
	    else if (pMse->sampleRate <=  200) c = 'e';
	    else if (pMse->sampleRate <=  500) c = 'h';
	    else if (pMse->sampleRate <= 1000) c = 'j';
	    else                               c = 'd';
	    xf86WriteSerial(pInfo->fd, &c, 1);
	    usleep(50000);
	    xf86WriteSerial(pInfo->fd, "\021", 1);	/* Resume DATA output */
	    break;

	case PROT_THINKING:		/* ThinkingMouse */
	    /* This mouse may send a PnP ID string, ignore it. */
	    usleep(200000);
	    xf86FlushInput(pInfo->fd);
	    /* Send the command to initialize the beast. */
	    for (s = "E5E5"; *s; ++s) {
		xf86WriteSerial(pInfo->fd, s, 1);
		if ((xf86WaitForInput(pInfo->fd, 1000000) <= 0))
		    break;
		xf86ReadSerial(pInfo->fd, &c, 1);
		if (c != *s)
		    break;
	    }
	    break;

	case PROT_MSC:		/* MouseSystems Corp */
	    usleep(100000);
	    xf86FlushInput(pInfo->fd);
	    break;

	case PROT_ACECAD:
	    /* initialize */
	    /* A nul character resets. */
	    xf86WriteSerial(pInfo->fd, "", 1);
	    usleep(50000);
	    /* Stream out relative mode high resolution increments of 1. */
	    xf86WriteSerial(pInfo->fd, "@EeI!", 5);
	    break;

	case PROT_BM:		/* bus/InPort mouse */
	    if (osInfo->SetBMRes)
		osInfo->SetBMRes(pInfo, pMse->protocol, pMse->sampleRate,
				 pMse->resolution);
	    break;

	case PROT_GENPS2:
	    ps2Init = FALSE;
	    break;

	case PROT_PS2:
	case PROT_GLIDEPS2:
	    break;
	
	case PROT_IMPS2:		/* IntelliMouse */
	{
	    static unsigned char seq[] = { 243, 200, 243, 100, 243, 80 };
	    param = seq;
	    paramlen = sizeof(seq);
	}
	break;

	case PROT_EXPPS2:		/* IntelliMouse Explorer */
	{
	    static unsigned char seq[] = { 243, 200, 243, 100, 243, 80,
					   243, 200, 243, 200, 243, 80 };
	
	    param = seq;
	    paramlen = sizeof(seq);
	}
	break;
    
	case PROT_NETPS2:		/* NetMouse, NetMouse Pro, Mie Mouse */
	case PROT_NETSCPS2:		/* NetScroll */
	{
	    static unsigned char seq[] = { 232, 3, 230, 230, 230 };
	
	    param = seq;
	    paramlen = sizeof(seq);
	}
	break;
    
	case PROT_MMPS2:		/* MouseMan+, FirstMouse+ */
	{
	    static unsigned char seq[] = { 230, 232, 0, 232, 3, 232, 2, 232, 1,
					   230, 232, 3, 232, 1, 232, 2, 232, 3 };
	    param = seq;
	    paramlen = sizeof(seq);
	}
	break;
    
	case PROT_THINKPS2:		/* ThinkingMouse */
	{
	    static unsigned char seq[] = { 243, 10, 232,  0, 243, 20, 243, 60,
					   243, 40, 243, 20, 243, 20, 243, 60,
					   243, 40, 243, 20, 243, 20 };
	    param = seq;
	    paramlen = sizeof(seq);
	}
	break;
	case PROT_SYSMOUSE:
	    if (osInfo->SetMiscRes)
		osInfo->SetMiscRes(pInfo, pMse->protocol, pMse->sampleRate,
				   pMse->resolution);
	    break;

	default:
	    /* Nothing to do. */
	    break;
    }

    if (pMse->class & (MSE_PS2 | MSE_XPS2)) {
	/*
	 * If one part of the PS/2 mouse initialization fails
	 * redo complete initialization. There are mice which
	 * have occasional problems with initialization and
	 * are in an unknown state.
	 */
	if (ps2Init) {
	REDO:
	    do_ps2Reset(pInfo);
	    if (paramlen > 0) {
		if (!ps2SendPacket(pInfo,param,paramlen)) {
		    usleep(30000);
		    xf86FlushInput(pInfo->fd);
		    if (!count--)
			return TRUE;
		    goto REDO;
		}
		ps2GetDeviceID(pInfo);
		usleep(30000);
		xf86FlushInput(pInfo->fd);
	    }
	    
	    if (osInfo->SetPS2Res) {
		osInfo->SetPS2Res(pInfo, pMse->protocol, pMse->sampleRate,
				  pMse->resolution);
	    } else {
		unsigned char c2[2];
		
		c = 0xE6;	/*230*/	/* 1:1 scaling */
		if (!ps2SendPacket(pInfo,&c,1)) {
		    if (!count--)
			return TRUE;
		    goto REDO;
		}
		c2[0] = 0xF3; /*243*/ /* set sampling rate */
		if (pMse->sampleRate > 0) {
		    if (pMse->sampleRate >= 200)
			c2[1] = 200;
		    else if (pMse->sampleRate >= 100)
			c2[1] = 100;
		    else if (pMse->sampleRate >= 80)
			c2[1] = 80;
		    else if (pMse->sampleRate >= 60)
			c2[1] = 60;
		    else if (pMse->sampleRate >= 40)
			c2[1] = 40;
		    else
			c2[1] = 20;
		} else {
		    c2[1] = 100;
		}
		if (!ps2SendPacket(pInfo,c2,2)) {
		    if (!count--)
			return TRUE;
		    goto REDO;
		}
		c2[0] = 0xE8; /*232*/	/* set device resolution */
		if (pMse->resolution > 0) {
		    if (pMse->resolution >= 200)
			c2[1] = 3;
		    else if (pMse->resolution >= 100)
			c2[1] = 2;
		    else if (pMse->resolution >= 50)
			c2[1] = 1;
		    else
			c2[1] = 0;
		} else {
		    c2[1] = 3; /* used to be 2, W. uses 3 */
		}
		if (!ps2SendPacket(pInfo,c2,2)) {
		    if (!count--)
			return TRUE;
		    goto REDO;
		}
		usleep(30000);
		xf86FlushInput(pInfo->fd);
		if (!ps2EnableDataReporting(pInfo)) {
		    xf86Msg(X_INFO, "%s: ps2EnableDataReporting: failed\n",
			    pInfo->name);
		    xf86FlushInput(pInfo->fd);
		    if (!count--)
			return TRUE;
		    goto REDO;
		} else {
		    xf86Msg(X_INFO, "%s: ps2EnableDataReporting: succeeded\n",
			    pInfo->name);
		}
	    }
	    /*
	     * The PS/2 reset handling needs to be rechecked.
	     * We need to wait until after the 4.3 release.
	     */
	}
    } else {
	if (paramlen > 0) {
	    if (xf86WriteSerial(pInfo->fd, param, paramlen) != paramlen)
		xf86Msg(X_ERROR, "%s: Mouse initialization failed\n",
			pInfo->name);
	    usleep(30000);
	    xf86FlushInput(pInfo->fd);
	}
    }

    return TRUE;
}

#ifdef SUPPORT_MOUSE_RESET
static Bool
mouseReset(InputInfoPtr pInfo, unsigned char val) 
{
    MouseDevPtr pMse = pInfo->private;
    mousePrivPtr mousepriv = (mousePrivPtr)pMse->mousePriv;
    CARD32 prevEvent = mousepriv->lastEvent;
    Bool expectReset = FALSE;
    Bool ret = FALSE;

    mousepriv->lastEvent = GetTimeInMillis();

#ifdef EXTMOUSEDEBUG
    ErrorF("byte: 0x%x time: %li\n",val,mousepriv->lastEvent);
#endif
    /*
     * We believe that the following is true:
     * When the mouse is replugged it will send a reset package
     * It takes several seconds to replug a mouse: We don't see
     * events for several seconds before we see the replug event package.
     * There is no significant delay between consecutive bytes
     * of a replug event package.
     * There are no bytes sent after the replug event package until
     * the mouse is reset.
     */
    
    if (mousepriv->current == 0
	&& (mousepriv->lastEvent - prevEvent) < 4000)
	return FALSE;

    if (mousepriv->current > 0
	&& (mousepriv->lastEvent - prevEvent) >= 1000) {
	mousepriv->inReset = FALSE;
	mousepriv->current = 0;
	return FALSE;
    }

    if (mousepriv->inReset)
	mousepriv->inReset = FALSE;

#ifdef EXTMOUSEDEBUG
    ErrorF("Mouse Current: %i 0x%x\n",mousepriv->current, val);
#endif
    
    /* here we put the mouse specific reset detction */
    /* They need to do three things:                 */
    /*  Check if byte may be a reset byte            */
    /*  If so: Set expectReset TRUE                  */
    /*  If convinced: Set inReset TRUE               */
    /*                Register BlockAndWakeupHandler */

    /* PS/2 */
    {
	unsigned char seq[] = { 0xaa, 0x00 };
	int len = sizeof(seq);

	if (seq[mousepriv->current] == val)
	    expectReset = TRUE;

	if (len == mousepriv->current + 1) {
	    mousepriv->inReset = TRUE;
	    mousepriv->expires = GetTimeInMillis() + 1000;

#ifdef EXTMOUSEDEBUG
	    ErrorF("Found PS/2 Reset string\n");
#endif
	    RegisterBlockAndWakeupHandlers (ps2BlockHandler,
					    ps2WakeupHandler, (pointer) pInfo);
	    ret = TRUE;
	}
    }
    
	if (!expectReset)
	    mousepriv->current = 0;
	else
	    mousepriv->current++;
	return ret;
}

static void
ps2BlockHandler(pointer data, struct timeval **waitTime,
		pointer LastSelectMask)
{
    InputInfoPtr    pInfo = (InputInfoPtr) data;
    MouseDevPtr	    pMse = (MouseDevPtr) pInfo->private;
    mousePrivPtr    mousepriv = (mousePrivPtr)pMse->mousePriv;
    int		    ms;

    if (mousepriv->inReset) {
	ms = mousepriv->expires - GetTimeInMillis ();
	if (ms <= 0)
	    ms = 0;
	AdjustWaitForDelay (waitTime, ms);
    } else
	RemoveBlockAndWakeupHandlers (ps2BlockHandler, ps2WakeupHandler,
				      (pointer) pInfo);
}

static void
ps2WakeupHandler(pointer data, int i, pointer LastSelectMask)
{
    InputInfoPtr    pInfo = (InputInfoPtr) data;
    MouseDevPtr	    pMse = (MouseDevPtr) pInfo->private;
    mousePrivPtr mousepriv = (mousePrivPtr)pMse->mousePriv;
    int		    ms;
    
    if (mousepriv->inReset) {
	unsigned char val;
	int blocked;

	ms = mousepriv->expires - GetTimeInMillis();
	if (ms > 0)
	    return;

	blocked = xf86BlockSIGIO ();

	xf86MsgVerb(X_INFO,3,
		    "Got reinsert event: reinitializing PS/2 mouse\n");
	val = 0xf4;
	if (xf86WriteSerial(pInfo->fd, &val, 1) != 1)
	    xf86Msg(X_ERROR, "%s: Write to mouse failed\n",
		    pInfo->name);
	xf86UnblockSIGIO(blocked);
    }
    RemoveBlockAndWakeupHandlers (ps2BlockHandler, ps2WakeupHandler,
				  (pointer) pInfo);
}
#endif /* SUPPORT_MOUSE_RESET */

/************************************************************
 *
 * Autoprobe stuff
 *
 ************************************************************/
#ifdef EXTMOUSEDEBUG
#  define AP_DBG(x) { ErrorF("Autoprobe: "); ErrorF x; }
#  define AP_DBGC(x) ErrorF x ;
# else
#  define AP_DBG(x)
#  define AP_DBGC(x)
#endif

MouseProtocolID hardProtocolList[] = { 	PROT_MSC, PROT_MM, PROT_LOGI, 
					PROT_LOGIMAN, PROT_MMHIT,
					PROT_GLIDE, PROT_IMSERIAL,
					PROT_THINKING, PROT_ACECAD,
					PROT_THINKPS2, PROT_MMPS2,
					PROT_GLIDEPS2, 
					PROT_NETSCPS2, PROT_EXPPS2,PROT_IMPS2,
					PROT_GENPS2, PROT_NETPS2,
					PROT_MS,
					PROT_UNKNOWN
};

MouseProtocolID softProtocolList[] = { 	PROT_MSC, PROT_MM, PROT_LOGI, 
					PROT_LOGIMAN, PROT_MMHIT,
					PROT_GLIDE, PROT_IMSERIAL,
					PROT_THINKING, PROT_ACECAD,
					PROT_THINKPS2, PROT_MMPS2,
					PROT_GLIDEPS2, 
					PROT_NETSCPS2 ,PROT_IMPS2,
					PROT_GENPS2,
					PROT_MS,
					PROT_UNKNOWN
};

static const char *
autoOSProtocol(InputInfoPtr pInfo, int *protoPara)
{
    MouseDevPtr pMse = pInfo->private;
    mousePrivPtr mPriv = (mousePrivPtr)pMse->mousePriv;
    const char *name = NULL;
    MouseProtocolID protocolID = PROT_UNKNOWN;

    /* Check if the OS has a detection mechanism. */
    if (osInfo->SetupAuto) {
	name = osInfo->SetupAuto(pInfo, protoPara);
	if (name) {
	    protocolID = ProtocolNameToID(name);
	    switch (protocolID) {
		case PROT_UNKNOWN:
		    /* Check for a builtin OS-specific protocol. */
		    if (osInfo->CheckProtocol && osInfo->CheckProtocol(name)) {
			/* We can only come here if the protocol has been
			 * changed to auto thru the xf86misc extension
			 * and we have detected an OS specific builtin
			 * protocol. Currently we cannot handle this */
			name = NULL;
		    } else
			name = NULL;
		    break;
		case PROT_UNSUP:
		    name = NULL;
		    break;
		default:
		    break;
	    }
	}
    }
#ifdef PNP_MOUSE
    if (!name && !mPriv->noPnP) {
	/* A PnP serial mouse? */
	protocolID = MouseGetPnpProtocol(pInfo);
	if (protocolID >= 0 && protocolID < PROT_NUMPROTOS) {
	    name = ProtocolIDToName(protocolID);
	    xf86Msg(X_PROBED, "%s: PnP-detected protocol: \"%s\"\n",
		    pInfo->name, name);
	}
    }
#endif
    if (!name && HAVE_GUESS_PROTOCOL && osInfo->GuessProtocol) {
	name = osInfo->GuessProtocol(pInfo, 0);
	if (name)
	    protocolID = ProtocolNameToID(name);
    }

    if (name) {
	pMse->protocolID = protocolID;
    }
    
    return name;
}

/*
 * createProtocolList() -- create a list of protocols which may
 * match on the incoming data stream.
 */
static void
createProtoList(MouseDevPtr pMse, MouseProtocolID *protoList)
{
    int i, j, k  = 0;
    MouseProtocolID prot;
    unsigned char *para;
    mousePrivPtr mPriv = (mousePrivPtr)pMse->mousePriv;
    MouseProtocolID *tmplist = NULL;
    int blocked;
    
    AP_DBGC(("Autoprobe: "));
    for (i = 0; i < mPriv->count; i++)
	AP_DBGC(("%2.2x ", (unsigned char) mPriv->data[i]));
    AP_DBGC(("\n"));

    blocked = xf86BlockSIGIO ();

    /* create a private copy first so we can write in the old list */
    if ((tmplist = xalloc(sizeof(MouseProtocolID) * NUM_AUTOPROBE_PROTOS))){
	for (i = 0; protoList[i] != PROT_UNKNOWN; i++) {
	    tmplist[i] = protoList[i];
	}
	tmplist[i] = PROT_UNKNOWN;
	protoList = tmplist;
    } else
	return;

    for (i = 0; ((prot = protoList[i]) != PROT_UNKNOWN 
		 && (k < NUM_AUTOPROBE_PROTOS - 1)) ; i++) {
	Bool bad = TRUE;
	unsigned char byte = 0;
	int count = 0;
	int next_header_candidate = 0;
	int header_count = 0;
	
	if (!GetProtocol(prot))
	    continue;
	para = proto[prot];

	AP_DBG(("Protocol: %s ", ProtocolIDToName(prot)));

#ifdef EXTMOUSEDEBUG
	for (j = 0; j < 7; j++)
	    AP_DBGC(("%2.2x ", (unsigned char) para[j]));
	AP_DBGC(("\n"));
#endif   
	j = 0;
	while (1) {
	    /* look for header */
	    while (j < mPriv->count) {
		if (((byte = mPriv->data[j++]) & para[0]) == para[1]){
		    AP_DBG(("found header %2.2x\n",byte));
		    next_header_candidate = j;
		    count = 1;
		    break;
		} else {
		    /* 
		     * Bail ot if number of bytes per package have
		     * been tested for header.
		     * Take bytes per package of leading garbage into
		     * account.
		     */
		    if (j > para[4] && ++header_count > para[4]) {
			j = mPriv->count;
			break;
		    }
		}
	    }
	    /* check if remaining data matches protocol */
	    while (j < mPriv->count) {
		byte = mPriv->data[j++];
		if (count == para[4]) {
		    count = 0;
		    /* check and eat excess byte */
		    if (((byte & para[0]) != para[1]) 
			&& ((byte & para[5]) == para[6])) {
			AP_DBG(("excess byte found\n"));
			continue; 
		    }
		}
		if (count == 0) {
		    /* validate next header */
		    bad = FALSE;
		    AP_DBG(("Complete set found\n"));
		    if ((byte & para[0]) != para[1]) {
			AP_DBG(("Autoprobe: header bad\n"));
			bad = TRUE;
			break;
		    } else {
			count++;
			continue;
		    }
		} 
		/* validate data */
		else if (((byte & para[2]) != para[3]) 
			 || ((para[7] & MPF_SAFE) 
			     && ((byte & para[0]) == para[1]))) {
		    AP_DBG(("data bad\n"));
		    bad = TRUE;
		    break;
		} else {
		    count ++;
		    continue;
		}
	    }
	    if (!bad) {
		/* this is a matching protocol */
		mPriv->protoList[k++] = prot;
		AP_DBG(("Autoprobe: Adding protocol %s to list (entry %i)\n",
			ProtocolIDToName(prot),k-1));
		break;
	    }
	    j = next_header_candidate;
	    next_header_candidate = 0;
	    /* we have tested number of bytes per package for header */
	    if (j > para[4] && ++header_count > para[4])
		break;
	    /* we have not found anything that looks like a header */
	    if (!next_header_candidate)
		break;
	    AP_DBG(("Looking for new header\n"));
	}
    }

    xf86UnblockSIGIO(blocked);
    
    mPriv->protoList[k] = PROT_UNKNOWN;

    xfree(tmplist);
}


/* This only needs to be done once */
void **serialDefaultsList = NULL;

/*
 * createSerialDefaultsLists() - create a list of the different default
 * settings for the serial interface of the known protocols.
 */
static void
createSerialDefaultsList(void)
{
    int i = 0, j, k;

    serialDefaultsList = (void **)xnfalloc(sizeof(void*));
    serialDefaultsList[0] = NULL;

    for (j = 0; mouseProtocols[j].name; j++) {
	if (!mouseProtocols[j].defaults)
	    continue;
	for (k = 0; k < i; k++)
	    if (mouseProtocols[j].defaults == serialDefaultsList[k])
		continue;
	i++;
	serialDefaultsList = (void**)xnfrealloc(serialDefaultsList,
						sizeof(void*)*(i+1));
	serialDefaultsList[i-1] = mouseProtocols[j].defaults;
	serialDefaultsList[i] = NULL;
    }
}

typedef enum {
    STATE_INVALID,
    STATE_UNCERTAIN,
    STATE_VALID
} validState;

/* Probing threshold values */
#define PROBE_UNCERTAINTY 50
#define BAD_CERTAINTY 6
#define BAD_INC_CERTAINTY 1
#define BAD_INC_CERTAINTY_WHEN_SYNC_LOST 2

static validState
validCount(mousePrivPtr mPriv, Bool inSync, Bool lostSync) 
{
    if (inSync) {
	if (!--mPriv->goodCount) {
	    /* we are sure to have found the correct protocol */
	    mPriv->badCount = 0;
	    return STATE_VALID;
	}
	AP_DBG(("%i successful rounds to go\n",
		mPriv->goodCount));
	return STATE_UNCERTAIN;
    }


    /* We are out of sync again */
    mPriv->goodCount = PROBE_UNCERTAINTY;
    /* We increase uncertainty of having the correct protocol */
    mPriv->badCount+= lostSync ? BAD_INC_CERTAINTY_WHEN_SYNC_LOST 
	: BAD_INC_CERTAINTY;

    if (mPriv->badCount < BAD_CERTAINTY) {
	/* We are not convinced yet to have the wrong protocol */
	AP_DBG(("Changing protocol after: %i rounds\n",
		BAD_CERTAINTY - mPriv->badCount));
	return STATE_UNCERTAIN;
    }
    return STATE_INVALID;
}

#define RESET_VALIDATION	mPriv->goodCount = PROBE_UNCERTAINTY;\
				mPriv->badCount = 0;\
				mPriv->prevDx = 0;\
				mPriv->prevDy = 0;\
				mPriv->accDx = 0;\
				mPriv->accDy = 0;\
				mPriv->acc = 0;

static void
autoProbeMouse(InputInfoPtr pInfo, Bool inSync, Bool lostSync) 
{
    MouseDevPtr pMse = pInfo->private;
    mousePrivPtr mPriv = (mousePrivPtr)pMse->mousePriv;

    MouseProtocolID *protocolList = NULL;
    
    while (1) {
	switch (mPriv->autoState) {
	case AUTOPROBE_GOOD:
 	    if (inSync)
		return;
	    AP_DBG(("State GOOD\n"));
	    RESET_VALIDATION;
	    mPriv->autoState = AUTOPROBE_VALIDATE1;
	    return;
	case AUTOPROBE_H_GOOD:
	    if (inSync)
		return;
	    AP_DBG(("State H_GOOD\n"));
	    RESET_VALIDATION;
	    mPriv->autoState = AUTOPROBE_H_VALIDATE2;
	    return;
	case AUTOPROBE_H_NOPROTO:
	    AP_DBG(("State H_NOPROTO\n"));
	    mPriv->protocolID = 0;
	    mPriv->autoState = AUTOPROBE_H_SETPROTO;
	    break;
	case AUTOPROBE_H_SETPROTO:
	    AP_DBG(("State H_SETPROTO\n"));
	    if ((pMse->protocolID = hardProtocolList[mPriv->protocolID++])
		== PROT_UNKNOWN) {
		mPriv->protocolID = 0;		    
		break;
	    } else if (GetProtocol(pMse->protocolID) &&  SetupMouse(pInfo)) {
		FlushButtons(pMse);
		RESET_VALIDATION;
		AP_DBG(("Autoprobe: Trying Protocol: %s\n",
			ProtocolIDToName(pMse->protocolID)));
		mPriv->autoState = AUTOPROBE_H_VALIDATE1;
		return;
	    }
	    break;
	case AUTOPROBE_H_VALIDATE1:
	    AP_DBG(("State H_VALIDATE1\n"));
	    switch (validCount(mPriv,inSync,lostSync)) {
	    case STATE_INVALID:
		mPriv->autoState = AUTOPROBE_H_SETPROTO;
		break;
	    case STATE_VALID:
		    xf86Msg(X_INFO,"Mouse autoprobe: selecting %s protocol\n",
			    ProtocolIDToName(pMse->protocolID));
		    mPriv->autoState = AUTOPROBE_H_GOOD;
		    return;
	    case STATE_UNCERTAIN:
		return;
	    default:
		break;
	    }
	    break;
	case AUTOPROBE_H_VALIDATE2:
	    AP_DBG(("State H_VALIDATE2\n"));
	    switch (validCount(mPriv,inSync,lostSync)) {
	    case STATE_INVALID:
		mPriv->autoState = AUTOPROBE_H_AUTODETECT;
		break;
	    case STATE_VALID:
		xf86Msg(X_INFO,"Mouse autoprobe: selecting %s protocol\n",
			ProtocolIDToName(pMse->protocolID));
		mPriv->autoState = AUTOPROBE_H_GOOD;
		return;
	    case STATE_UNCERTAIN:
		return;
	    }
	    break;
	case AUTOPROBE_H_AUTODETECT:
	    AP_DBG(("State H_AUTODETECT\n"));
	    pMse->protocolID = PROT_AUTO;
	    AP_DBG(("Looking for PnP/OS mouse\n"));
	    mPriv->count = 0;
	    SetupMouse(pInfo);
	    if (pMse->protocolID != PROT_AUTO)
		mPriv->autoState = AUTOPROBE_H_GOOD;
	    else
		mPriv->autoState = AUTOPROBE_H_NOPROTO;
	    break;
	case AUTOPROBE_NOPROTO:
	    AP_DBG(("State NOPROTO\n"));
	    mPriv->count = 0;
	    mPriv->serialDefaultsNum = -1;
	    mPriv->autoState = AUTOPROBE_COLLECT;
	    break;    
	case AUTOPROBE_COLLECT:
	    AP_DBG(("State COLLECT\n"));
	    if (mPriv->count <= NUM_MSE_AUTOPROBE_BYTES)
		return;
	    protocolList = softProtocolList;
	    mPriv->autoState = AUTOPROBE_CREATE_PROTOLIST;
	    break;
	case AUTOPROBE_CREATE_PROTOLIST:
	    AP_DBG(("State CREATE_PROTOLIST\n"));
	    createProtoList(pMse, protocolList);
	    mPriv->protocolID = 0;
	    mPriv->autoState = AUTOPROBE_SWITCH_PROTOCOL;
	    break;
	case AUTOPROBE_AUTODETECT:
	    AP_DBG(("State AUTODETECT\n"));
	    pMse->protocolID = PROT_AUTO;
	    AP_DBG(("Looking for PnP/OS mouse\n"));
	    mPriv->count = 0;
	    SetupMouse(pInfo);
	    if (pMse->protocolID != PROT_AUTO)
		mPriv->autoState = AUTOPROBE_GOOD;
	    else
		mPriv->autoState = AUTOPROBE_NOPROTO;
	    break;
	case AUTOPROBE_VALIDATE1:
	    AP_DBG(("State VALIDATE1\n"));
	    switch (validCount(mPriv,inSync,lostSync)) {
	    case STATE_INVALID:
		mPriv->autoState = AUTOPROBE_AUTODETECT;
		break;
	    case STATE_VALID:
		xf86Msg(X_INFO,"Mouse autoprobe: selecting %s protocol\n",
			ProtocolIDToName(pMse->protocolID));
		mPriv->autoState = AUTOPROBE_GOOD;
		break;
	    case STATE_UNCERTAIN:
		return;
	    }
	    break;
	case AUTOPROBE_VALIDATE2:
	    AP_DBG(("State VALIDATE2\n"));
	    switch (validCount(mPriv,inSync,lostSync)) {
	    case STATE_INVALID:
		protocolList = &mPriv->protoList[mPriv->protocolID];
		mPriv->autoState = AUTOPROBE_CREATE_PROTOLIST;
		break;
	    case STATE_VALID:
		xf86Msg(X_INFO,"Mouse autoprobe: selecting %s protocol\n",
			ProtocolIDToName(pMse->protocolID));
		mPriv->autoState = AUTOPROBE_GOOD;
		break;
	    case STATE_UNCERTAIN:
		return;
	    }
	    break;
	case AUTOPROBE_SWITCHSERIAL:
	{
	    pointer serialDefaults;
	    AP_DBG(("State SWITCHSERIAL\n"));
	    
	    if (!serialDefaultsList)
		createSerialDefaultsList();
	    
	    AP_DBG(("Switching serial params\n"));
	    if ((serialDefaults =
		 serialDefaultsList[++mPriv->serialDefaultsNum]) == NULL) {
		mPriv->serialDefaultsNum = 0;
	    } else {
		pointer tmp = xf86OptionListCreate(serialDefaults, -1, 0);
		xf86SetSerial(pInfo->fd, tmp);
		xf86OptionListFree(tmp);
		mPriv->count = 0;
		mPriv->autoState = AUTOPROBE_COLLECT;
	    }
	    break;
	}
	case AUTOPROBE_SWITCH_PROTOCOL:
	{
	    MouseProtocolID proto;
	    void *defaults;
	    AP_DBG(("State SWITCH_PROTOCOL\n"));
	    proto = mPriv->protoList[mPriv->protocolID++];
	    if (proto == PROT_UNKNOWN) 
		mPriv->autoState = AUTOPROBE_SWITCHSERIAL;
	    else if (!(defaults = GetProtocol(proto)->defaults)
		       || (mPriv->serialDefaultsNum == -1 
			   && (defaults == msDefaults))
		       || (mPriv->serialDefaultsNum != -1
			   && serialDefaultsList[mPriv->serialDefaultsNum]
			   == defaults)) {
		AP_DBG(("Changing Protocol to %s\n",
			ProtocolIDToName(proto)));
		SetMouseProto(pMse,proto);
		FlushButtons(pMse);
		RESET_VALIDATION;
		mPriv->autoState = AUTOPROBE_VALIDATE2;
		return;
	    }
	    break;
	}
	}
    }
}

static Bool
autoGood(MouseDevPtr pMse)
{
    mousePrivPtr mPriv = (mousePrivPtr)pMse->mousePriv;
    
    if (!pMse->autoProbe)
	return TRUE;

    switch (mPriv->autoState) {
    case AUTOPROBE_GOOD:
    case AUTOPROBE_H_GOOD:
	return TRUE;
    case AUTOPROBE_VALIDATE1: /* @@@ */
    case AUTOPROBE_H_VALIDATE1: /* @@@ */
    case AUTOPROBE_VALIDATE2:
    case AUTOPROBE_H_VALIDATE2:
	if (mPriv->goodCount < PROBE_UNCERTAINTY/2)
	    return TRUE;
    default:
	return FALSE;
    }
}


#define TOT_THRESHOLD 3000
#define VAL_THRESHOLD 40

/*
 * checkForErraticMovements() -- check if mouse 'jumps around'.
 */
static void
checkForErraticMovements(InputInfoPtr pInfo, int dx, int dy)
{
    MouseDevPtr pMse = pInfo->private;
    mousePrivPtr mPriv = (mousePrivPtr)pMse->mousePriv;
#if 1
    if (!mPriv->goodCount)
	return;
#endif
#if 0
    if (abs(dx - mPriv->prevDx) > 300 
	|| abs(dy - mPriv->prevDy) > 300)
	AP_DBG(("erratic1 behaviour\n"));
#endif
    if (abs(dx) > VAL_THRESHOLD) {
	if (sign(dx) == sign(mPriv->prevDx)) {
	    mPriv->accDx += dx;
	    if (abs(mPriv->accDx) > mPriv->acc) {
		mPriv->acc = abs(mPriv->accDx);
		AP_DBG(("acc=%i\n",mPriv->acc));
	    } 
	    else
		AP_DBG(("accDx=%i\n",mPriv->accDx));
	} else {
	    mPriv->accDx = 0;
	}
    }

    if (abs(dy) > VAL_THRESHOLD) {
	if (sign(dy) == sign(mPriv->prevDy)) {
	    mPriv->accDy += dy;
	    if (abs(mPriv->accDy) > mPriv->acc) {
		mPriv->acc = abs(mPriv->accDy);
		AP_DBG(("acc: %i\n",mPriv->acc));
	    } else
		AP_DBG(("accDy=%i\n",mPriv->accDy));
	} else {
	    mPriv->accDy = 0;
	}
    }
    mPriv->prevDx = dx;
    mPriv->prevDy = dy;
    if (mPriv->acc > TOT_THRESHOLD) {
	mPriv->goodCount = PROBE_UNCERTAINTY;
	mPriv->prevDx = 0;
	mPriv->prevDy = 0;
	mPriv->accDx = 0;
	mPriv->accDy = 0;
	mPriv->acc = 0;
	AP_DBG(("erratic2 behaviour\n"));
	autoProbeMouse(pInfo, FALSE,TRUE);
    }
}

static void
SetMouseProto(MouseDevPtr pMse, MouseProtocolID protocolID)
{
    pMse->protocolID = protocolID;
    pMse->protocol = ProtocolIDToName(pMse->protocolID);
    pMse->class = ProtocolIDToClass(pMse->protocolID);
    if ((pMse->protocolID >= 0) && (pMse->protocolID < PROT_NUMPROTOS))
	memcpy(pMse->protoPara, proto[pMse->protocolID],
	       sizeof(pMse->protoPara));
    
    if (pMse->emulate3ButtonsSoft)
	pMse->emulate3Buttons = TRUE;
}

/*
 * collectData() -- collect data bytes sent by mouse.
 */
static Bool
collectData(MouseDevPtr pMse, unsigned char u)
{
    mousePrivPtr mPriv = (mousePrivPtr)pMse->mousePriv;
    if (mPriv->count < NUM_MSE_AUTOPROBE_TOTAL) {
	mPriv->data[mPriv->count++] = u;
	if (mPriv->count <= NUM_MSE_AUTOPROBE_BYTES) {
		return TRUE;
	} 
    }
    return FALSE;
}

/**************** end of autoprobe stuff *****************/



#ifdef XFree86LOADER
ModuleInfoRec MouseInfo = {
    1,
    "MOUSE",
    NULL,
    0,
    MouseAvailableOptions,
};

static MODULETEARDOWNPROTO(xf86MouseUnplug);

static void
xf86MouseUnplug(pointer	p)
{
}

static MODULESETUPPROTO(xf86MousePlug);

static pointer
xf86MousePlug(ModuleDescPtr	module,
	    pointer	options,
	    int		*errmaj,
	    int		*errmin)
{
    static Bool Initialised = FALSE;

    if (!Initialised) {
	Initialised = TRUE;
#ifndef REMOVE_LOADER_CHECK_MODULE_INFO
	if (xf86LoaderCheckSymbol("xf86AddModuleInfo"))
#endif
	xf86AddModuleInfo(&MouseInfo, module);
    }

    xf86AddInputDriver(&MOUSE, module, 0);

    return module;
}

static XF86ModuleVersionInfo xf86MouseVersionRec =
{
    "mouse",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_XINPUT,
    ABI_XINPUT_VERSION,
    MOD_CLASS_XINPUT,
    {0, 0, 0, 0}		/* signature, to be patched into the file by */
				/* a tool */
};

XF86ModuleData mouseModuleData = {&xf86MouseVersionRec,
				  xf86MousePlug,
				  xf86MouseUnplug};

/*
  Look at hitachi device stuff.
*/
#endif /* XFree86LOADER */


