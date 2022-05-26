/* Vectrex Controller driver pretending to be a keyboard, for PiTrex.
   Based on vectrexmouse driver and kbd.c.
   Supports US keyboard layout.

   Kevin Koster, 2022.
*/

#define NEED_EVENTS
#include <X11/X.h>
#include <X11/Xproto.h>

#include <xf86.h>
#include <atKeynames.h>
#include <xf86Privstr.h>
#include <xf86_ansic.h>
#include <compiler.h>
#include <xf86_OSproc.h>
#include <xf86Xinput.h>
#include <linux/lnx_kbd.h>
#include <xf86Keymap.h>

#include <xf86Module.h>

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

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
  OsTimerPtr    veckbdTimer;				/* timer object */
  int           veckbdTimeout;				/* timeout to poll device (miliseconds) */
  char          veckbdAnalogue;             /* enable analogue joystick reads */
  int           veckbdAnalogueThreshold;    /* minimum registering analogue joystick movement */
  int           veckbdAnalogueDebounce;     /* mid-position count before taking it seriously */
  int8_t        veckbdOldX;					/* previous X position */
  int8_t        veckbdOldY;					/* previous Y position */
  int           veckbdXzeros;               /* X mid-position count */
  int           veckbdYzeros;               /* Y mid-position count */
  uint8_t       veckbdOldButtons;			/* previous buttons state */
  uint8_t       veckbdMultiButtonA;         /* multi-button state */
  uint8_t       veckbdMultiButtonB;
/* This should be "veckbdKeyAssign[5][3]", but setting priv->veckbdKeyAssign[5][0]
   was overwriting priv->veckbdKeyAssign[4][3] as well! (same thing if a static global
   variable is used instead) */
  unsigned char veckbdKeyAssign[5][4];		/* button to key asignments */
} VectrexKbdDevRec, *VectrexKbdDevPtr;

static InputInfoPtr VecKbdPreInit(InputDriverPtr drv, IDevPtr dev, int flags);
static void xf86VectrexKbdUnplug (pointer	p);
static pointer xf86VectrexKbdPlug (ModuleDescPtr module,
	     pointer	options,
	     int	*errmaj,
	     int	*errmin );

#undef VECTREXKBD
InputDriverRec VECTREXKBD = {
	1,				/* driver version */
	"vectrexkbd",		/* driver name */
	NULL,				/* identify */
	VecKbdPreInit,	/* pre-init */
	NULL,				/* un-init */
	NULL,				/* module */
	0				/* ref count */
};

/* TODO - Revise options */
typedef enum {
    OPTION_ALWAYS_CORE,
    OPTION_SEND_CORE_EVENTS,
    OPTION_CORE_KEYBOARD,
    OPTION_DEVICE,
    OPTION_PROTOCOL,
    OPTION_AUTOREPEAT,
    OPTION_XLEDS,
    OPTION_XKB_DISABLE,
    OPTION_XKB_KEYMAP,
    OPTION_XKB_KEYCODES,
    OPTION_XKB_TYPES,
    OPTION_XKB_COMPAT,
    OPTION_XKB_SYMBOLS,
    OPTION_XKB_GEOMETRY,
    OPTION_XKB_RULES,
    OPTION_XKB_MODEL,
    OPTION_XKB_LAYOUT,
    OPTION_XKB_VARIANT,
    OPTION_XKB_OPTIONS,
    OPTION_PANIX106,
    OPTION_CUSTOM_KEYCODES
} VecKbdOpts;

#ifdef XFree86LOADER
static const OptionInfoRec veckbdOptions[] = {
    { OPTION_ALWAYS_CORE,	"AlwaysCore",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SEND_CORE_EVENTS,	"SendCoreEvents", OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_CORE_KEYBOARD,	"CoreKeyboard",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_DEVICE,		"Device",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_PROTOCOL,		"Protocol",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_AUTOREPEAT,	"AutoRepeat",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XLEDS,		"XLeds",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_DISABLE,	"XkbDisable",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_XKB_KEYMAP,	"XkbKeymap",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_KEYCODES,	"XkbKeycodes",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_TYPES,		"XkbTypes",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_COMPAT,	"XkbCompat",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_SYMBOLS,	"XkbSymbols",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_GEOMETRY,	"XkbGeometry",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_RULES,		"XkbRules",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_MODEL,		"XkbModel",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_LAYOUT,	"XkbLayout",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_VARIANT,	"XkbVariant",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_XKB_OPTIONS,	"XkbOptions",	  OPTV_STRING,	{0}, FALSE },
    { OPTION_PANIX106,		"Panix106",	  OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_CUSTOM_KEYCODES,   "CustomKeycodes", OPTV_BOOLEAN,	{0}, FALSE },
    { -1,			NULL,		  OPTV_NONE,	{0}, FALSE }
};

/*ARGSUSED*/
static const OptionInfoRec *
VecKbdAvailableOptions(void *unused)
{
    return (veckbdOptions);
}
#endif

/* This seems to be used to configure drivers automatically for
   different device hardware types. Set to NULL like void driver
   for now.
*/
static const char *VecKbdDefaults[] = {
    NULL
};

static void
xf86VectrexKbdBell(int percent, DeviceIntPtr dev, pointer ctrl, int unused)
{
  /* I guess we can make a beep here if we really want to */
}

static int
xf86VectrexKbdCtrl( DeviceIntPtr device, KeybdCtrl *ctrl)
{
  /* This seems to be all about handling the keyboard LEDs */
  return (Success);
}

/*
 * xf86VectrexKbdEvents --
 *      Read the new events from the device, and enqueue them.
 *       Called from timer.
 */
static CARD32
xf86VectrexKbdEvents(OsTimerPtr        timer,
               CARD32            atime,
               pointer           arg)
{
  DeviceIntPtr          device = (DeviceIntPtr)arg;
  VectrexKbdDevPtr      priv = (VectrexKbdDevPtr) XI_PRIVATE(device);
  int                   timeout = priv->veckbdTimeout;
  uint8_t               loop,modifier,skip,modloop;

  /* Check whether inputs have changed and send events if they have.
     Digital range: -1, 0, +1 int8_t currentJoy1X; int8_t currentJoy1Y;
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
   */

  DBG(5, ErrorF("xf86VectrexKbdEvents BEGIN device=0x%x priv=0x%x"
                " timeout=%d timer=0x%x\n",
                device, priv, timeout, priv->veckbdTimer));

/*    v_readJoystick1Digital(); TODO - Need to fix random "down" problem in vectrex interface lib. */
    v_readButtons();
    skip = 4;

  /* Detect changes of button state */
    for(loop=0; loop<4; loop++)
      {
      	if (loop != skip)
      	{
          if (((priv->veckbdOldButtons >> loop) & 1) != ((currentButtonState >> loop) & 1))
          {
            /* Multi-Button: Check for two-button presses */
            /* Multi Release */
            if (loop == priv->veckbdMultiButtonB ||
                (loop == priv->veckbdMultiButtonA && priv->veckbdMultiButtonB < 4))
            {
             xf86PostKeyboardEvent(device,
               priv->veckbdKeyAssign[priv->veckbdMultiButtonA][priv->veckbdMultiButtonB]
               + MIN_KEYCODE, 0);
             DBG(3, ErrorF("Vec Multi Key Event - key:%d status:0\n",
                 priv->veckbdKeyAssign[priv->veckbdMultiButtonA][priv->veckbdMultiButtonB]
                 + MIN_KEYCODE));
             priv->veckbdMultiButtonB = 4;
            }
            else
            {
              /* Multi Press (unless already done one) */
              if (~loop & currentButtonState && skip == 4)
              {
                modifier = loop;
                for (modloop=0; modloop<4; modloop++)
                {
                  if (modloop != modifier &&
                      ((currentButtonState >> modloop) & 1) &&
                      priv->veckbdKeyAssign[loop][modloop] != KEY_NOTUSED)
                  {
                     xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[loop][modloop]
                                           + MIN_KEYCODE, 1);
                     DBG(3, ErrorF("Vec Multi Key Event - key:%d status:1\n",
                         priv->veckbdKeyAssign[loop][modloop] + MIN_KEYCODE));
                     skip = modloop;
                     priv->veckbdMultiButtonA = loop;
                     priv->veckbdMultiButtonB = modloop;
                  }
                }
              }
            }
            /* Single button press, or multiple buttons without a combined meaning */
            if (skip == 4 && priv->veckbdKeyAssign[4][loop] != 0)
            {
              if (loop == priv->veckbdMultiButtonA)
              {
              	/* Don't release never-pressed multi-button */
              	priv->veckbdMultiButtonA = 4;
              }
              else
              {
                xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[4][loop] + MIN_KEYCODE,
                                      ((currentButtonState >> loop) & 1));
                DBG(3, ErrorF("Vec Single Key Event - loop:%d key:%d status:%d\n",
                    loop, priv->veckbdKeyAssign[4][loop] + MIN_KEYCODE,
                    ((currentButtonState >> modloop) & 1)));
              }
            }
          }
        }
      }

  priv->veckbdOldButtons = currentButtonState;
  /* TODO - second controller buttons */

  if (priv->veckbdAnalogue)
  {
    v_readJoystick1Analog();  	
    if (currentJoy1X > priv->veckbdAnalogueThreshold)
    {
      currentJoy1X = 1;
      DBG(4, ErrorF("Vec Analogue Direction Key Event - RIGHT\n"));
      priv->veckbdXzeros = 0;
    }
    else
    {
      if (currentJoy1X < (0 - priv->veckbdAnalogueThreshold))
      {
        currentJoy1X = -1;
        DBG(4, ErrorF("Vec Analogue Direction Key Event - LEFT\n"));
        priv->veckbdXzeros = 0;
      }
      else
      {
      	if (priv->veckbdXzeros == priv->veckbdAnalogueDebounce)
      	{
      	  currentJoy1X = 0;
      	  DBG(4, ErrorF("Vec Analogue Direction Key Event - X-MID\n"));
      	  priv->veckbdXzeros = 0;
        }
        else
        {
          priv->veckbdXzeros++;
        }
      }
    }

    if (currentJoy1Y > priv->veckbdAnalogueThreshold)
    {
      currentJoy1Y = 1;
      DBG(4, ErrorF("Vec Analogue Direction Key Event - UP\n"));
      priv->veckbdYzeros = 0;
    }
    else
    {
      if (currentJoy1Y < (0 - priv->veckbdAnalogueThreshold))
      {
        currentJoy1Y = -1;
        DBG(4, ErrorF("Vec Analogue Direction Key Event - DOWN\n"));
        priv->veckbdYzeros = 0;
      }
      else
      	if (priv->veckbdYzeros > priv->veckbdAnalogueDebounce)
      	{
      	  currentJoy1Y = 0;
      	  DBG(4, ErrorF("Vec Analogue Direction Key Event - Y-MID\n"));
      	  priv->veckbdYzeros = 0;
        }
        else
        {
          priv->veckbdYzeros++;
        }
    }
  }
  else
  {
    v_readJoystick1Digital();
  }
  /* convert direction inputs to keyboard scancodes */
  if (currentJoy1X != priv->veckbdOldX && priv->veckbdXzeros == 0)
  {
    if (currentJoy1X == 0)
    {
      /* joystick centered, so release last pressed direction button */
      if (priv->veckbdOldX > 0 && priv->veckbdKeyAssign[5][3] != 0)
      	xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][3] + MIN_KEYCODE, 0);
      if (priv->veckbdOldX < 0 && priv->veckbdKeyAssign[5][2] != 0)
      	xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][2] + MIN_KEYCODE, 0);
    }
    else
    {
      if (currentJoy1X > 0 &&
          priv->veckbdKeyAssign[5][3] != 0) /* Right */
      {
      	xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][3] + MIN_KEYCODE, 1);
        if (priv->veckbdOldX < 0 &&
            priv->veckbdKeyAssign[5][2] != 0) /* jumped left to right */
          xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][2] + MIN_KEYCODE, 0);
      }
      if (currentJoy1X < 0 &&
          priv->veckbdKeyAssign[5][2] != 0) /* Left */
      {
      	xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][2] + MIN_KEYCODE, 1);
      	if (priv->veckbdOldX > 0 &&
      	    priv->veckbdKeyAssign[5][3] != 0) /* jumped right to left */
          xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][3] + MIN_KEYCODE, 0);
      }
    }
    priv->veckbdOldX = currentJoy1X;
  }

  if (currentJoy1Y != priv->veckbdOldY && priv->veckbdYzeros == 0)
  {
    if (currentJoy1Y == 0)
    {
      /* joystick centered, so release last pressed direction button */
      if (priv->veckbdOldY > 0 && priv->veckbdKeyAssign[5][0] != 0)
      	xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][0] + MIN_KEYCODE, 0);
      if (priv->veckbdOldY < 0 && priv->veckbdKeyAssign[5][1] != 0)
      	xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][1] + MIN_KEYCODE, 0);
    }
    else
    {
      if (currentJoy1Y > 0 &&
          priv->veckbdKeyAssign[5][0] != 0) /* Up */
      {
      	xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][0] + MIN_KEYCODE, 1);
        if (priv->veckbdOldY < 0 &&
            priv->veckbdKeyAssign[5][1] != 0) /* jumped down to up */
          xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][1] + MIN_KEYCODE, 0);
      }
      if (currentJoy1Y < 0 &&
          priv->veckbdKeyAssign[5][1] != 0) /* Down */
      {
      	xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][1] + MIN_KEYCODE, 1);
      	if (priv->veckbdOldY > 0 &&
      	    priv->veckbdKeyAssign[5][0] != 0) /* jumped up to down */
          xf86PostKeyboardEvent(device, priv->veckbdKeyAssign[5][0] + MIN_KEYCODE, 0);
      }
    }
    priv->veckbdOldY = currentJoy1Y;
  }

/*
  From kbd.c:
  keycode = scanCode + MIN_KEYCODE;
  xf86PostKeyboardEvent(device, keycode, down);
*/
  DBG(5, ErrorF("xf86VectrexKbdEvents END   device=0x%x priv=0x%x"
                " timeout=%d timer=0x%x\n",
                device, priv, timeout, priv->veckbdTimer));

    return timeout;
}

static void
xf86VectrexKbdControlProc(DeviceIntPtr	device,
                    PtrCtrl		*ctrl)
{
  DBG(2, ErrorF("xf86VectrexKbdControlProc\n"));
}

/*
 * xf86VectrexKbdProc --
 *      Handle the initialization, etc.
 */
static int
xf86VectrexKbdProc(DeviceIntPtr       pVectrexKbd,
		     int                what)
{
  InputInfoPtr          pInfo = pVectrexKbd->public.devicePrivate;
  VectrexKbdDevPtr      priv = (VectrexKbdDevPtr) pInfo->private;

  DBG(2, ErrorF("BEGIN xf86VectrexKbdProc dev=0x%x priv=0x%x xf86VectrexKbdEvents=0x%x\n",
                pVectrexKbd, priv, xf86VectrexKbdEvents));
  KeySymsRec           keySyms;
  CARD8                modMap[MAP_LENGTH];
  int i;

    switch (what)
    {
    case DEVICE_INIT:
        DBG(1, ErrorF("xf86VectrexKbdProc pVectrexKbd=0x%x what=INIT\n", pVectrexKbd));
/*        pKbd->KbdGetMapping(pInfo, &keySyms, modMap);  */
	/* keySyms matches key codes to actual keys/symbols (from xf86Keymap.h).
	 * I think modMap keeps track of modifier keys such as Ctrl.
	 * I'm not handling modifier keys if I don't have to. */
	for (i = 0; i < MAP_LENGTH; i++)
          modMap[i] = NoSymbol;

        keySyms.map = map;
        keySyms.mapWidth   = GLYPHS_PER_KEY;
        keySyms.minKeyCode = MIN_KEYCODE;
        keySyms.maxKeyCode = MAX_KEYCODE;
	
        pVectrexKbd->public.on = FALSE;
            InitKeyboardDeviceStruct((DevicePtr) pVectrexKbd,
                             &keySyms,
                             modMap,
                             xf86VectrexKbdBell,
                             (KbdCtrlProcPtr)xf86VectrexKbdCtrl);
    break;
/* end kbd.c */ 
     case DEVICE_ON:
        DBG(1, ErrorF("xf86VectrexKbdProc  pVectrexKbd=0x%x what=ON\n", pVectrexKbd));

        priv->veckbdTimer = TimerSet(NULL, 0, /*TimerAbsolute,*/
                                   priv->veckbdTimeout,
                                   xf86VectrexKbdEvents,
                                   (pointer)pVectrexKbd);
        pVectrexKbd->public.on = TRUE;
        DBG(2, ErrorF("priv->veckbdTimer=0x%x\n", priv->veckbdTimer));
        priv->veckbdMultiButtonA = priv->veckbdMultiButtonB = 4;
      break;
    case DEVICE_OFF:
    case DEVICE_CLOSE:
      DBG(1, ErrorF("xf86VectrexKbdProc  pVectrexKbd=0x%x what=%s\n", pVectrexKbd,
                    (what == DEVICE_CLOSE) ? "CLOSE" : "OFF"));

      /* Disable event update timer */
        TimerFree(priv->veckbdTimer);
        priv->veckbdTimer = NULL;

        pVectrexKbd->public.on = FALSE;
      break;

    default:
      ErrorF("unsupported mode=%d\n", what);
      return !Success;
      break;
    }
  DBG(2, ErrorF("END   xf86VectrexKbdProc dev=0x%x priv=0x%x xf86VectrexKbdEvents=0x%x\n",
                pVectrexKbd, priv, xf86VectrexKbdEvents));

  return Success;	
}

/* Options / module config. set-up */
static InputInfoPtr
VecKbdPreInit(InputDriverPtr drv, IDevPtr dev, int flags)
{
 DBG(2, ErrorF("VecKbdPreInit\n"));
  InputInfoPtr pInfo;
  VectrexKbdDevPtr priv;
  MessageType from = X_DEFAULT;
  char *s;
 /* Internal config/defaults	*/
  if (!(pInfo = xf86AllocateInput(drv, 0)))
   return NULL;

  /* Initialise the InputInfoRec. */
 DBG(5, ErrorF("VecKbdPreInit - set defaults\n"));
  pInfo->name = "Vectrex Controller Keyboard Device";
  pInfo->flags = XI86_KEYBOARD_CAPABLE;
  pInfo->device_control = xf86VectrexKbdProc;
  pInfo->read_input = NULL;
  pInfo->close_proc = NULL;
  pInfo->control_proc = NULL;
  pInfo->switch_mode = NULL;
  pInfo->conversion_proc = NULL;
  pInfo->fd = -1;  
  pInfo->atom = 0;
  pInfo->dev = NULL;
  pInfo->private_flags = 0;
  pInfo->always_core_feedback = 0;
  pInfo->type_name = XI_KEYBOARD;
  pInfo->history_size  = 0;
  pInfo->conf_idev = dev;

  DBG(5, ErrorF("VecKbdPreInit - end defaults\n"));

    if (!(priv = xcalloc(sizeof(VectrexKbdDevRec), 1)))
        return pInfo;
    pInfo->private = priv;

  DBG(5, ErrorF("VecKbdPreInit - alloc\n"));

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, VecKbdDefaults, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

  DBG(5, ErrorF("VecKbdPreInit - option start\n"));

    /* Optional configuration */
    s = xf86SetStrOption(pInfo->options, "DeviceName", NULL);
    if (s != NULL)
	pInfo->name = s;
    xf86Msg(X_CONFIG, "%s name is %s\n", pInfo->type_name, pInfo->name);
    
    debug_level = xf86SetIntOption(pInfo->options, "DebugLevel", 0);
    if (debug_level > 0) {
	xf86Msg(X_CONFIG, "VECTREX KBD: debug level set to %d\n", debug_level);
    }
    priv->veckbdTimeout = xf86SetIntOption(pInfo->options, "Timeout", 20);
    if (priv->veckbdTimeout != 20) {
	xf86Msg(X_CONFIG, "VECTREX KBD: timeout = %d\n", priv->veckbdTimeout);
    }
    priv->veckbdAnalogue = xf86SetBoolOption(pInfo->options, "ReadAnalogue", TRUE);
    if (priv->veckbdAnalogue != 1) {
	xf86Msg(X_CONFIG, "VECTREX KBD: Analogue joystick mode disabled\n");
    }
    priv->veckbdAnalogueThreshold = xf86SetIntOption(pInfo->options, "AnalogueThreshold", 75);
    if (priv->veckbdAnalogueThreshold != 75) {
	xf86Msg(X_CONFIG, "VECTREX KBD: Analogue Threshold = %d\n", priv->veckbdAnalogueThreshold);
    }
    priv->veckbdAnalogueDebounce = xf86SetIntOption(pInfo->options, "AnalogueDebounce", 2);
    if (priv->veckbdAnalogueDebounce != 2) {
	xf86Msg(X_CONFIG, "VECTREX KBD: Analogue Debounce = %d\n", priv->veckbdAnalogueDebounce);
    }

  /* Button to key assignments */
  /* KEY_ definitions in: hw/xfree86/common/atKeynames.h */
  priv->veckbdKeyAssign[4][0] = (unsigned char)xf86SetIntOption(pInfo->options, "B1Key", KEY_LCtrl);
    if (priv->veckbdTimeout != KEY_LCtrl) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B1Key = %d\n", priv->veckbdKeyAssign[4][0]);
    }
  priv->veckbdKeyAssign[4][1] = (unsigned char)xf86SetIntOption(pInfo->options, "B2Key", KEY_Alt);
    if (priv->veckbdTimeout != KEY_Alt) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B2Key = %d\n", priv->veckbdKeyAssign[4][1]);
    }
  priv->veckbdKeyAssign[4][2] = (unsigned char)xf86SetIntOption(pInfo->options, "B3Key", KEY_Enter);
    if (priv->veckbdTimeout != KEY_Enter) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B3Key = %d\n", priv->veckbdKeyAssign[4][2]);
    }
  priv->veckbdKeyAssign[4][3] = (unsigned char)xf86SetIntOption(pInfo->options, "B4Key", KEY_Space);
    if (priv->veckbdTimeout != KEY_Space) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B4Key = %d\n", priv->veckbdKeyAssign[4][3]);
    }

  priv->veckbdKeyAssign[0][1] = (unsigned char)xf86SetIntOption(pInfo->options, "B1+2Key", KEY_NOTUSED);
  priv->veckbdKeyAssign[1][0] = priv->veckbdKeyAssign[0][1];
    if (priv->veckbdTimeout != KEY_NOTUSED) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B1+2Key = %d\n", priv->veckbdKeyAssign[0][1]);
    }
  priv->veckbdKeyAssign[0][2] = (unsigned char)xf86SetIntOption(pInfo->options, "B1+3Key", KEY_NOTUSED);
  priv->veckbdKeyAssign[2][0] = priv->veckbdKeyAssign[0][2];
    if (priv->veckbdTimeout != KEY_NOTUSED) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B1+3Key = %d\n", priv->veckbdKeyAssign[0][2]);
    }
  priv->veckbdKeyAssign[0][3] = (unsigned char)xf86SetIntOption(pInfo->options, "B1+4Key", KEY_NOTUSED);
  priv->veckbdKeyAssign[3][0] = priv->veckbdKeyAssign[0][3];
    if (priv->veckbdTimeout != KEY_NOTUSED) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B1+4Key = %d\n", priv->veckbdKeyAssign[0][3]);
    }
  priv->veckbdKeyAssign[1][2] = (unsigned char)xf86SetIntOption(pInfo->options, "B2+3Key", KEY_NOTUSED);
  priv->veckbdKeyAssign[2][1] = priv->veckbdKeyAssign[1][2];
    if (priv->veckbdTimeout != KEY_NOTUSED) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B2+3Key = %d\n", priv->veckbdKeyAssign[1][2]);
    }
  priv->veckbdKeyAssign[1][3] = (unsigned char)xf86SetIntOption(pInfo->options, "B2+4Key", KEY_NOTUSED);
  priv->veckbdKeyAssign[3][1] = priv->veckbdKeyAssign[1][3];
    if (priv->veckbdTimeout != KEY_NOTUSED) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B2+4Key = %d\n", priv->veckbdKeyAssign[1][3]);
    }
  priv->veckbdKeyAssign[2][3] = (unsigned char)xf86SetIntOption(pInfo->options, "B3+4Key", KEY_NOTUSED);
  priv->veckbdKeyAssign[3][2] = priv->veckbdKeyAssign[2][3];
    if (priv->veckbdTimeout != KEY_NOTUSED) {
	xf86Msg(X_CONFIG, "VECTREX KBD: B3+4Key = %d\n", priv->veckbdKeyAssign[2][3]);
    }

  priv->veckbdKeyAssign[5][0] = (unsigned char)xf86SetIntOption(pInfo->options, "UpKey", KEY_Up);
    if (priv->veckbdTimeout != KEY_Up) {
	xf86Msg(X_CONFIG, "VECTREX KBD: UpKey = %d\n", priv->veckbdKeyAssign[5][0]);
    }
  priv->veckbdKeyAssign[5][1] = (unsigned char)xf86SetIntOption(pInfo->options, "DownKey", KEY_Down);
    if (priv->veckbdTimeout != KEY_Down) {
	xf86Msg(X_CONFIG, "VECTREX KBD: DownKey = %d\n", priv->veckbdKeyAssign[5][1]);
    }
  priv->veckbdKeyAssign[5][2] = (unsigned char)xf86SetIntOption(pInfo->options, "LeftKey", KEY_Left);
    if (priv->veckbdTimeout != KEY_Left) {
	xf86Msg(X_CONFIG, "VECTREX KBD: LeftKey = %d\n", priv->veckbdKeyAssign[5][2]);
    }
  priv->veckbdKeyAssign[5][3] = (unsigned char)xf86SetIntOption(pInfo->options, "RightKey", KEY_Right);
    if (priv->veckbdTimeout != KEY_Right) {
	xf86Msg(X_CONFIG, "VECTREX KBD: RightKey = %d\n", priv->veckbdKeyAssign[5][3]);
    }
  DBG(5, xf86Msg(X_INFO, "VecKbdPreInit - option end\n"));
    /* Mark the device configured */
    pInfo->flags |= XI86_CONFIGURED;

    /* Return the configured device */
    return pInfo;
}

/*
 ***************************************************************************
 *
 * Dynamic loading functions
 *
 ***************************************************************************
 */
#ifdef XFree86LOADER

ModuleInfoRec VecKbdInfo = {
    1,
    "VECTREXKBD",
    NULL,
    0,
    VecKbdAvailableOptions,
};

/*
 * xf86VectrexKbdUnplug --
 *
 * called when the module subsection is found in XF86Config
 */
static void
xf86VectrexKbdUnplug(pointer	p)
{
/* This seems to be empty in all the newer drivers, so commented out.

    LocalDevicePtr local = (LocalDevicePtr) p;
    VectrexKbdDevPtr priv = (VectrexKbdDevPtr) local->private;
    
    ErrorF("xf86VectrexKbdUnplug\n");
    
    xf86VectrexKbdProc(local->dev, DEVICE_OFF);
    
    xfree (priv);
    xfree (local);
*/
}

/* called when the module subsection is found in XF86Config */
static pointer
xf86VectrexKbdPlug(ModuleDescPtr	module,
	     pointer	options,
	     int	*errmaj,
	     int	*errmin )
{
    ErrorF("xf86VectrexKbdPlug\n");
    static Bool Initialised = FALSE;

    if (!Initialised)
    {
	Initialised = TRUE;
#ifndef REMOVE_LOADER_CHECK_MODULE_INFO
	if (xf86LoaderCheckSymbol("xf86AddModuleInfo"))
#endif
	xf86AddModuleInfo(&VecKbdInfo, module);
    }
    xf86AddInputDriver(&VECKBD, module, 0);
    return module;
}

static XF86ModuleVersionInfo xf86VectrexKbdVersionRec =
{
	"vectrexkbd",
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

XF86ModuleData veckbdModuleData = {&xf86VectrexKbdVersionRec,
					 xf86VectrexKbdPlug,
					 xf86VectrexKbdUnplug};
#endif /* XFree86LOADER */
/* end of veckbd.c */
