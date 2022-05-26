/* Vectrex Controller driver pretending to be a mouse, for PiTrex.
   Based on mouse, joystick (wrongly - it's broken), sample, dmc,
   void, and citron drivers.
   
   Note: There won't be any way to use the second controller here.

   Kevin Koster, 2022.
*/

#include <misc.h>
#include <xf86.h>
#include <xf86_ansic.h>
#include <xf86_OSproc.h>
#include <xf86Xinput.h>
#include <xf86OSmouse.h>
#include <xf86_OSlib.h>    /* Says in doc/DESIGN that driver must not include this, but xisb.h doesn't build without it... */
/* #define NEED_XF86_TYPES -- this is in mouse driver but doesn't solve the problem */
#include <xisb.h>
#include <exevents.h>		/* Needed for InitValuator/Proximity stuff */
#include <X11/keysym.h>

#include <xf86Module.h>

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
/*
#include "mipointer.h"
*/

/******************************************************************************
 * debugging macro
 *****************************************************************************/
#ifdef DBG
#undef DBG
#endif
#ifdef DEBUG
#undef DEBUG
#endif

static int      debug_level = 5;
#define DEBUG 1
#if DEBUG
#define DBG(lvl, f) {if ((lvl) <= debug_level) f;}
#else
#define DBG(lvl, f)
#endif

/******************************************************************************
 * device records
 *****************************************************************************/

typedef struct 
{
  OsTimerPtr    vecmouseTimer;      /* timer object */
  int           vecmouseTimeout;    /* timeout to poll device */
  int           vecmouseOldX;       /* previous X position */
  int           vecmouseOldY;       /* previous Y position */
  uint8_t       vecmouseOldButtons; /* previous buttons state */
  int           vecmouseMaxX;       /* max X value */
  int           vecmouseMaxY;       /* max Y value */
  int           vecmouseMinX;       /* min X value */
  int           vecmouseMinY;       /* min Y value */
  int           vecmouseCenterX;    /* X center value */
  int           vecmouseCenterY;    /* Y center value */
  int           vecmouseDelta;      /* delta cursor */
  unsigned char vecmouseRelative;   /* Relative Movement mode*/
} VectrexMouseDevRec, *VectrexMouseDevPtr;

static InputInfoPtr VecMousePreInit(InputDriverPtr drv, IDevPtr dev, int flags);
static void xf86VectrexMouseUnplug (pointer	p);
static pointer xf86VectrexMousePlug (ModuleDescPtr module,
	     pointer	options,
	     int	*errmaj,
	     int	*errmin );

#undef VECTREXMOUSE
InputDriverRec VECTREXMOUSE = {
	1,				/* driver version */
	"vectrexmouse",		/* driver name */
	NULL,				/* identify */
	VecMousePreInit,	/* pre-init */
	NULL,				/* un-init */
	NULL,				/* module */
	0				/* ref count */
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
    OPTION_PNP
} VecMouseOpts;

#ifdef XFree86LOADER
static const OptionInfoRec vecmouseOptions[] = {
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
    { OPTION_DRAGLOCKBUTTONS,	"DragLockButtons",OPTV_STRING,	{0}, FALSE }
};

/*ARGSUSED*/
static const OptionInfoRec *
VecMouseAvailableOptions(void *unused)
{
    return (vecmouseOptions);
}
#endif

/* This seems to be used to configure drivers automatically for
   different device hardware types. Set to NULL like void driver
   for now.
*/
static const char *VecMouseDefaults[] = {
    NULL
};

/*
 ***************************************************************************
 *
 * xf86VectrexMouseConvert --
 *	Convert valuators to X and Y.
 *
 * DMC driver does scaling here (ConvertProc).
 *
 ***************************************************************************
 */
static Bool
xf86VectrexMouseConvert(LocalDevicePtr local,
		int		first,
		int		num,
		int		v0,
		int		v1,
		int		v2,
		int		v3,
		int		v4,
		int		v5,
		int*		x,
		int*		y)
{
    if (first != 0 || num != 2)
      return FALSE;

    *x = v0;
    *y = v1;

    return TRUE;
}

/*
 * xf86VectrexMouseEvents --
 *      Read the new events from the device, and enqueue them.
 *       Called from timer.
 */
static CARD32
xf86VectrexMouseEvents(OsTimerPtr        timer,
               CARD32            atime,
               pointer           arg)
{
  DeviceIntPtr          device = (DeviceIntPtr)arg;
  VectrexMouseDevPtr        priv = (VectrexMouseDevPtr) XI_PRIVATE(device);
  int                   timeout = priv->vecmouseTimeout;

  /* Check whether inputs have changed and send events if they have.
     Analogue range: -128, 0, +127 int8_t currentJoy1X; int8_t currentJoy1Y;
     Buttons: uint8_t currentButtonState;
      bit 0 - Joyport 1 button 1
      bit 1 - Joyport 1 button 2
      bit 2 - Joyport 1 button 3
      bit 3 - Joyport 1 button 4
      bit 5 - Joyport 2 button 1
      bit 6 - Joyport 2 button 2
      bit 7 - Joyport 2 button 3
      bit 8 - Joyport 2 button 4
      
      Linux (old) joystick X-Y range: "1 being the minimum, 128 the center,
       and 255 maximum value"
       https://www.kernel.org/doc/html/latest/input/joydev/joystick-api.html#backward-compatibility
   */

    unsigned char loop;
    int length = priv->vecmouseMaxX - priv->vecmouseMinX;
    int height = priv->vecmouseMaxY - priv->vecmouseMinY;

  DBG(5, ErrorF("xf86VectrexMouseEvents BEGIN device=0x%x priv=0x%x"
                " timeout=%d timer=0x%x\n",
                device, priv, timeout, priv->vecmouseTimer));

    /* This copies the joystick driver, xf86PostMotionEvent called the same way
       in mouse.c, so maybe this will work? */
    v_readJoystick1Analog();
    v_readButtons();
    int x = (int)currentJoy1X + 128;
    int y = (int)currentJoy1Y + 128;
    int oldX,oldY;

    int v0 = (((x - priv->vecmouseMinX) * priv->vecmouseDelta) / length -
        (priv->vecmouseDelta / 2)) + 38;
    int v1 = (((y - priv->vecmouseMinY) * priv->vecmouseDelta) / height -
        (priv->vecmouseDelta / 2)) + 38;

    if (priv->vecmouseRelative > 0)
    {
      oldX = v0;
      oldY = v1;
      v0 -= priv->vecmouseOldX;
      v1 -= priv->vecmouseOldY;
      if (v0 != 0 && v1 != 0)
      {
        xf86PostMotionEvent(device, 0, 0, 2, v0, v1);
        DBG(4, ErrorF("v_readJoystick1Analog Relative x=%d y=%d centerX=%d centerY=%d v0=%d "
            "v1=%d buttons=%d\n", x, y, priv->vecmouseCenterX, priv->vecmouseCenterY,
             v0, v1, currentButtonState));
        priv->vecmouseOldX = oldX;
        priv->vecmouseOldY = oldY;
      }
    }
    else
    {
     if ((abs(v0) > (priv->vecmouseDelta / 20)) ||
         (abs(v1) > (priv->vecmouseDelta / 20)))
       {
         xf86PostMotionEvent(device, 0, 0, 2, v0, v1);
         DBG(4, ErrorF("v_readJoystick1Analog x=%d y=%d centerX=%d centerY=%d v0=%d "
             "v1=%d buttons=%d\n", x, y, priv->vecmouseCenterX, priv->vecmouseCenterY,
              v0, v1, currentButtonState));
       }
    }

    /* Assuming that four buttons isn't do-able for a mouse. */
    /*    MAME will only know about three max anyway. */
    /* Button 2 = Left, Button 3 = Middle, Button 4 = Right */
    for(loop=1; loop<4; loop++)
      {
        if (((priv->vecmouseOldButtons >> loop) & 1) != ((currentButtonState >> loop) & 1))
          {
	    xf86PostButtonEvent(device, 0, loop, ((currentButtonState >> loop) & 1),
						0, 0);
          }
      }
    priv->vecmouseOldButtons = currentButtonState;

  DBG(5, ErrorF("xf86VectrexMouseEvents END   device=0x%x priv=0x%x"
                " timeout=%d timer=0x%x\n",
                device, priv, timeout, priv->vecmouseTimer));

    return timeout;
}

static void
xf86VectrexMouseControlProc(DeviceIntPtr	device,
                    PtrCtrl		*ctrl)
{
  DBG(2, ErrorF("xf86VectrexMouseControlProc\n"));
}

/*
 * xf86VectrexMouseProc --
 *      Handle the initialization, etc.
 */
static int
xf86VectrexMouseProc(DeviceIntPtr       pVectrexMouse,
		     int                what)
{
  LocalDevicePtr        local = (LocalDevicePtr)pVectrexMouse->public.devicePrivate;
  VectrexMouseDevPtr        priv = (VectrexMouseDevPtr)XI_PRIVATE(pVectrexMouse);

  DBG(2, ErrorF("BEGIN xf86VectrexMouseProc dev=0x%x priv=0x%x xf86VectrexMouseEvents=0x%x\n",
                pVectrexMouse, priv, xf86VectrexMouseEvents));

/* mouse.c - mutated into something more like xf86Jstk.c */
/*      InputInfoPtr pInfo;
    MouseDevPtr pMse;
    mousePrivPtr mPriv;
*/
    unsigned char map[4];
    char i;
    
/*    pInfo = device->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device 8= device;
*/
    switch (what)
    {
    case DEVICE_INIT:
        DBG(1, ErrorF("xf86VectrexMouseProc pVectrexMouse=0x%x what=INIT\n", pVectrexMouse));
	pVectrexMouse->public.on = FALSE;
        priv->vecmouseOldX = 0;
        priv->vecmouseOldY = 0;
	for (i = 0; i < 3; i++)
	    map[i + 1] = i + 1;
/*
	InitPointerDeviceStruct(pVectrexMouse, map,
				min(pMse->buttons, MSE_MAXBUTTONS),
				miPointerGetMotionEvents, pMse->Ctrl,
				miPointerGetMotionBufferSize());
*/
	if (InitButtonClassDeviceStruct(pVectrexMouse, 3, map) == FALSE)
	{
	  ErrorF("unable to allocate Button class device\n");
          return !Success;
        }
	if (InitValuatorClassDeviceStruct(pVectrexMouse, 2, xf86GetMotionEvents,
		local->history_size, 0) == FALSE)
	{
	  ErrorF("unable to allocate Valuator class device\n");
          return !Success;
        }
	if (InitPtrFeedbackClassDeviceStruct(pVectrexMouse, xf86VectrexMouseControlProc) == FALSE)
	{
	  ErrorF("unable to allocate Feedback class device\n");
          return !Success;
        }

	/* X valuator */
	xf86InitValuatorAxisStruct(pVectrexMouse, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pVectrexMouse, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(pVectrexMouse, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pVectrexMouse, 1);
	xf86MotionHistoryAllocate(local);
       break;
/* end mouse.c */ 
     case DEVICE_ON:
        DBG(1, ErrorF("xf86VectrexMouseProc  pVectrexMouse=0x%x what=ON\n", pVectrexMouse));
        priv->vecmouseTimer = TimerSet(NULL, 0, /*TimerAbsolute,*/
                                   priv->vecmouseTimeout,
                                   xf86VectrexMouseEvents,
                                   (pointer)pVectrexMouse);
        pVectrexMouse->public.on = TRUE;
        DBG(2, ErrorF("priv->vecmouseTimer=0x%x\n", priv->vecmouseTimer));
      break;
    case DEVICE_OFF:
    case DEVICE_CLOSE:
      DBG(1, ErrorF("xf86VectrexMouseProc  pVectrexMouse=0x%x what=%s\n", pVectrexMouse,
                    (what == DEVICE_CLOSE) ? "CLOSE" : "OFF"));

      /* Disable event update timer */
        TimerFree(priv->vecmouseTimer);
        priv->vecmouseTimer = NULL;

        pVectrexMouse->public.on = FALSE;
      break;

    default:
      ErrorF("unsupported mode=%d\n", what);
      return !Success;
      break;
    }
  DBG(2, ErrorF("END   xf86VectrexMouseProc dev=0x%x priv=0x%x xf86VectrexMouseEvents=0x%x\n",
                pVectrexMouse, priv, xf86VectrexMouseEvents));

  return Success;	
}

/* Options / module config. set-up */
static InputInfoPtr
VecMousePreInit(InputDriverPtr drv, IDevPtr dev, int flags)
{
 DBG(2, ErrorF("VecMousePreInit\n"));
  InputInfoPtr pInfo;
  VectrexMouseDevPtr priv;
  MessageType from = X_DEFAULT;
  char *s;
 /* Internal config/defaults	*/
  if (!(pInfo = xf86AllocateInput(drv, 0)))
   return NULL;

  /* Initialise the InputInfoRec. */
 DBG(5, ErrorF("VecMousePreInit - set defaults\n"));
  pInfo->name = "Vectrex Controller Mouse Device";
  pInfo->flags = XI86_POINTER_CAPABLE | XI86_SEND_DRAG_EVENTS;
  pInfo->device_control = xf86VectrexMouseProc;
  pInfo->read_input = NULL;
  pInfo->close_proc = NULL;
  pInfo->control_proc = NULL;
  pInfo->switch_mode = NULL;
  pInfo->conversion_proc = xf86VectrexMouseConvert;
  pInfo->fd = -1;  
  pInfo->atom = 0;
  pInfo->dev = NULL;
  pInfo->private_flags = 0;
  pInfo->always_core_feedback = 0;
  pInfo->type_name = XI_MOUSE;
  pInfo->history_size  = 0;
  pInfo->conf_idev = dev;

  DBG(5, ErrorF("VecMousePreInit - end defaults\n"));

    if (!(priv = xcalloc(sizeof(VectrexMouseDevRec), 1)))
        return pInfo;
    pInfo->private = priv;

  DBG(5, ErrorF("VecMousePreInit - alloc\n"));

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, VecMouseDefaults, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

  DBG(5, ErrorF("VecMousePreInit - option start\n"));

    /* Optional configuration */
    debug_level = xf86SetIntOption(pInfo->options, "DebugLevel", 0);
    if (debug_level > 0) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: debug level set to %d\n", debug_level);
    }
    priv->vecmouseMaxX = xf86SetIntOption(pInfo->options, "MaxX", 1000);
    if (priv->vecmouseMaxX != 1000) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: max x = %d\n", priv->vecmouseMaxX);
    }
    priv->vecmouseMaxY = xf86SetIntOption(pInfo->options, "MaxY", 1000);
    if (priv->vecmouseMaxY != 1000) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: max y = %d\n", priv->vecmouseMaxY);
    }
    priv->vecmouseMinX = xf86SetIntOption(pInfo->options, "MinX", 0);
    if (priv->vecmouseMinX != 0) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: min x = %d\n", priv->vecmouseMinX);
    }
    priv->vecmouseMinY = xf86SetIntOption(pInfo->options, "MinY", 0);
    if (priv->vecmouseMinY != 0) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: min y = %d\n", priv->vecmouseMinY);
    }
    priv->vecmouseCenterX = xf86SetIntOption(pInfo->options, "CenterX", -1);
    if (priv->vecmouseCenterX != -1) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: center x = %d\n", priv->vecmouseCenterX);
    }
    priv->vecmouseCenterY = xf86SetIntOption(pInfo->options, "CenterY", -1);
    if (priv->vecmouseCenterY != -1) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: center y = %d\n", priv->vecmouseCenterY);
    }
    priv->vecmouseTimeout = xf86SetIntOption(pInfo->options, "Timeout", 20);
    if (priv->vecmouseTimeout != 20) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: timeout = %d\n", priv->vecmouseTimeout);
    }
    priv->vecmouseDelta = xf86SetIntOption(pInfo->options, "Delta", 100);
    if (priv->vecmouseDelta != 100) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: delta = %d\n", priv->vecmouseDelta);
    }
    priv->vecmouseRelative = xf86SetBoolOption(pInfo->options, "RelativeMovement", TRUE);
    if (priv->vecmouseRelative != 1) {
	xf86Msg(X_CONFIG, "VECTREX MOUSE: relative movement mode disabled\n");
    }
  DBG(5, xf86Msg(X_INFO, "VecMousePreInit - option end\n"));

    /* Mark the device configured */
    pInfo->flags |= XI86_CONFIGURED;

    /* Return the configured device */
    return pInfo;
}

/*
 * mouse association - Old?
 */
/*
DeviceAssocRec vecmouse_assoc =
{
  "vectrexmouse",
  VecMousePreInit
};
*/

/*
 ***************************************************************************
 *
 * Dynamic loading functions
 *
 ***************************************************************************
 */
#ifdef XFree86LOADER

ModuleInfoRec VecMouseInfo = {
    1,
    "VECTREXMOUSE",
    NULL,
    0,
    VecMouseAvailableOptions,
};

/*
 * xf86VectrexMouseUnplug --
 *
 * called when the module subsection is found in XF86Config
 */
static void
xf86VectrexMouseUnplug(pointer	p)
{
/* This seems to be empty in all the newer drivers, so commented out.

    LocalDevicePtr local = (LocalDevicePtr) p;
    VectrexMouseDevPtr priv = (VectrexMouseDevPtr) local->private;
    
    ErrorF("xf86VectrexMouseUnplug\n");
    
    xf86VectrexMouseProc(local->dev, DEVICE_OFF);
    
    xfree (priv);
    xfree (local);
*/
}

/* called when the module subsection is found in XF86Config */
static pointer
xf86VectrexMousePlug(ModuleDescPtr	module,
	     pointer	options,
	     int	*errmaj,
	     int	*errmin )
{
    ErrorF("xf86VectrexMousePlug\n");
    static Bool Initialised = FALSE;

    if (!Initialised)
    {
	Initialised = TRUE;
#ifndef REMOVE_LOADER_CHECK_MODULE_INFO
	if (xf86LoaderCheckSymbol("xf86AddModuleInfo"))
#endif
	xf86AddModuleInfo(&VecMouseInfo, module);
    }
    xf86AddInputDriver(&VECMOUSE, module, 0);
    return module;
}

static XF86ModuleVersionInfo xf86VectrexMouseVersionRec =
{
	"vectrexmouse",
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

XF86ModuleData vecmouseModuleData = {&xf86VectrexMouseVersionRec,
					 xf86VectrexMousePlug,
					 xf86VectrexMouseUnplug};
#endif /* XFree86LOADER */
/* end of vecmouse.c */
