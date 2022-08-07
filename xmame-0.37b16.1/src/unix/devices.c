/*
 * file devices.c
 *
 * Routines for Pointers device processing
 *
 * Joystick and Mouse
 *
 * original idea from Chris Sharp <sharp@uk.ibm.com>
 *
 */

#define __DEVICES_C_
#include "xmame.h"
#include "devices.h"
#include "input.h"
#include "keyboard.h"

/* local variables */
static struct JoystickInfo joy_list[JOY_LIST_LEN+MOUSE_LIST_LEN+1];
/* will be used to store names for the above */
static char joy_list_names[JOY_LIST_LEN+MOUSE_LIST_LEN][JOY_NAME_LEN];
static int analogstick = 0;

/* input relelated options */
struct rc_option input_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Input Related",	NULL,			rc_seperator,	NULL,
      NULL,		0,			0,		NULL,
      NULL },
   { "joytype",		"jt",			rc_int,		&joytype,
      "6",      	0,			6,		NULL,
      "Select type of joystick support to use:\n0 No joystick\n1 i386 style joystick driver (if compiled in)\n2 Fm Town Pad support (if compiled in)\n3 X11 input extension joystick (if compiled in)\n4 new i386 linux 1.x.x joystick driver(if compiled in)\n5 NetBSD USB joystick driver (if compiled in)\n6 SDL joystick support (if compiled in)" },
   { "analogstick",	"as",			rc_bool,	&analogstick,
     "0",		0,			0,		NULL,
     "Use Joystick as analog for analog controls" },
   { NULL,		NULL,			rc_link,	joy_i386_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_link,	joy_pad_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_link,	joy_x11_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_link,	joy_usb_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { "mouse",		"m",			rc_bool,	&use_mouse,
     "1",		0,			0,		NULL,
     "Enable/disable mouse (if supported)" },
   { "hotrod",		"hr",			rc_set_int,	&use_hotrod,
      NULL,      	1,			0,		NULL,
      "Enable HotRod joystick support" },
   { "hotrodse",	"hr",			rc_set_int,	&use_hotrod,
      NULL,      	2,			0,		NULL,
      "Select HotRod SE joystick support" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};


static int joy_list_equiv[][2] =
{
	{ JOY_AXIS_CODE(0,0,0), JOYCODE_1_LEFT },
	{ JOY_AXIS_CODE(0,0,1), JOYCODE_1_RIGHT },
	{ JOY_AXIS_CODE(0,1,0), JOYCODE_1_UP },
	{ JOY_AXIS_CODE(0,1,1), JOYCODE_1_DOWN },
	{ JOY_BUTTON_CODE(0,0), JOYCODE_1_BUTTON1 },
	{ JOY_BUTTON_CODE(0,1), JOYCODE_1_BUTTON2 },
	{ JOY_BUTTON_CODE(0,2), JOYCODE_1_BUTTON3 },
	{ JOY_BUTTON_CODE(0,3), JOYCODE_1_BUTTON4 },
	{ JOY_BUTTON_CODE(0,4), JOYCODE_1_BUTTON5 },
	{ JOY_BUTTON_CODE(0,5), JOYCODE_1_BUTTON6 },
	{ JOY_AXIS_CODE(1,0,0), JOYCODE_2_LEFT },
	{ JOY_AXIS_CODE(1,0,1), JOYCODE_2_RIGHT },
	{ JOY_AXIS_CODE(1,1,0), JOYCODE_2_UP },
	{ JOY_AXIS_CODE(1,1,1), JOYCODE_2_DOWN },
	{ JOY_BUTTON_CODE(1,0), JOYCODE_2_BUTTON1 },
	{ JOY_BUTTON_CODE(1,1), JOYCODE_2_BUTTON2 },
	{ JOY_BUTTON_CODE(1,2), JOYCODE_2_BUTTON3 },
	{ JOY_BUTTON_CODE(1,3), JOYCODE_2_BUTTON4 },
	{ JOY_BUTTON_CODE(1,4), JOYCODE_2_BUTTON5 },
	{ JOY_BUTTON_CODE(1,5), JOYCODE_2_BUTTON6 },
	{ JOY_AXIS_CODE(2,0,0), JOYCODE_3_LEFT },
	{ JOY_AXIS_CODE(2,0,1), JOYCODE_3_RIGHT },
	{ JOY_AXIS_CODE(2,1,0), JOYCODE_3_UP },
	{ JOY_AXIS_CODE(2,1,1), JOYCODE_3_DOWN },
	{ JOY_BUTTON_CODE(2,0), JOYCODE_3_BUTTON1 },
	{ JOY_BUTTON_CODE(2,1), JOYCODE_3_BUTTON2 },
	{ JOY_BUTTON_CODE(2,2), JOYCODE_3_BUTTON3 },
	{ JOY_BUTTON_CODE(2,3), JOYCODE_3_BUTTON4 },
	{ JOY_BUTTON_CODE(2,4), JOYCODE_3_BUTTON5 },
	{ JOY_BUTTON_CODE(2,5), JOYCODE_3_BUTTON6 },
	{ JOY_AXIS_CODE(3,0,0), JOYCODE_4_LEFT },
	{ JOY_AXIS_CODE(3,0,1), JOYCODE_4_RIGHT },
	{ JOY_AXIS_CODE(3,1,0), JOYCODE_4_UP },
	{ JOY_AXIS_CODE(3,1,1), JOYCODE_4_DOWN },
	{ JOY_BUTTON_CODE(3,0), JOYCODE_4_BUTTON1 },
	{ JOY_BUTTON_CODE(3,1), JOYCODE_4_BUTTON2 },
	{ JOY_BUTTON_CODE(3,2), JOYCODE_4_BUTTON3 },
	{ JOY_BUTTON_CODE(3,3), JOYCODE_4_BUTTON4 },
	{ JOY_BUTTON_CODE(3,4), JOYCODE_4_BUTTON5 },
	{ JOY_BUTTON_CODE(3,5), JOYCODE_4_BUTTON6 },
	{ 0,0 }
};

/* 2 init routines one for creating the display and one after that, since some
   (most) init stuff needs a display */

int osd_input_initpre (void)
{
   int i, j, k, joy_list_count = 0;
   
   joy_poll_func = NULL;
   
   memset(joy_data,   0, sizeof(joy_data));
   memset(mouse_data, 0, sizeof(mouse_data));
   
   for(i=0; i<JOY; i++)
   {
      joy_data[i].fd = -1;
      for(j=0; j<JOY_AXIS; j++)
      {
         joy_data[i].axis[j].min = -10;
         joy_data[i].axis[j].max =  10;
         for(k=0; k<JOY_DIRS; k++)
         {
            snprintf(joy_list_names[joy_list_count], JOY_NAME_LEN,
               "Joy %d axis %d %s", i+1, j+1, (k)? "pos":"neg");
            joy_list_count++;
         }
      }
      for(j=0; j<JOY_BUTTONS; j++)
      {
         snprintf(joy_list_names[joy_list_count], JOY_NAME_LEN,
            "Joy %d button %d", i+1 ,j+1);
         joy_list_count++;
      }
   }
   
   for(i=0; i<MOUSE; i++)
   {
      for(j=0; j<MOUSE_BUTTONS; j++)
      {
         snprintf(joy_list_names[joy_list_count], JOY_NAME_LEN,
            "Mouse %d button %d", i+1, j+1);
         joy_list_count++;
      }
   }
   
   /* terminate array */
   joy_list[joy_list_count].name = 0;
   joy_list[joy_list_count].code = 0;
   joy_list[joy_list_count].standardcode = 0;
   
   /* fill in codes */
   for (i=0; i<joy_list_count; i++)
   {
      joy_list[i].code = i;
      joy_list[i].name = joy_list_names[i];
      joy_list[i].standardcode = JOYCODE_OTHER;

      for(j=0; joy_list_equiv[j][1]; j++)
      {
         if (joy_list_equiv[j][0] == joy_list[i].code)
         {
            joy_list[i].standardcode = joy_list_equiv[j][1];
            break;
         }
      }
   }
   
   if (use_mouse)
      fprintf (stderr_file, "Mouse/Trakball selected.\n");
   
   return OSD_OK;
}

int osd_input_initpost (void)
{
   int i;
   
   /* init the keyboard */
   if (keyboard_init())
      return OSD_NOT_OK;
   
   /* joysticks */
   switch (joytype)
   {
      case JOY_NONE:
         break;
#ifdef I386_JOYSTICK
      case JOY_I386NEW:
      case JOY_I386:
         joy_i386_init();
         break;
#endif
#ifdef LIN_FM_TOWNS
      case JOY_PAD:
         joy_pad_init ();
         break;
#endif
#ifdef X11_JOYSTICK
      case JOY_X11:
         joy_x11_init();
         break;
#endif
#ifdef USB_JOYSTICK
      case JOY_USB:
         joy_usb_init();
         break;
#endif
#ifdef SDL_JOYSTICK
      case JOY_SDL:
         joy_SDL_init();
         break;
#endif
      default:
         fprintf (stderr_file, "OSD: Warning: unknown joytype: %d, or joytype not compiled in.\n"
            "   Disabling joystick support.\n", joytype);
         joytype = JOY_NONE;
         return OSD_OK;
   }
   
   if(joytype)
   {
      int found = FALSE;
      
      for (i=0; i<JOY; i++)
      {
         if(joy_data[i].num_axis || joy_data[i].num_buttons)
         {
            fprintf(stderr_file, "OSD: Info: Joystick %d, %d axis, %d buttons\n",
               i, joy_data[i].num_axis, joy_data[i].num_buttons);
            found = TRUE;
         }
      }
   
      if (!found)
      {
         fprintf(stderr_file, "OSD: Warning: No joysticks found disabling joystick support\n");
         joytype = JOY_NONE;
      }
   }
   
   return OSD_OK;
}

void osd_input_close (void)
{
   int i;
   
   keyboard_exit();
   
   for(i=0;i<JOY;i++)
      if(joy_data[i].fd >= 0)
         close(joy_data[i].fd);
}

/* return a list of all available joys */
const struct JoystickInfo *osd_get_joy_list(void)
{
	return joy_list;
}

void osd_trak_read(int player,int *deltax,int *deltay)
{
   if (player < MOUSE && use_mouse)
   {
      *deltax = mouse_data[player].deltas[0];
      *deltay = mouse_data[player].deltas[1];
   }
   else
   {
      *deltax = 0;
      *deltay = 0;
   }
}

void osd_poll_joysticks (void)
{
   if (use_mouse)
      sysdep_mouse_poll ();
   if (joytype)
      (*joy_poll_func) ();
}

int osd_is_joy_pressed (int joycode)
{
   if (joycode >= (JOY_LIST_LEN+MOUSE_LIST_LEN))
      return FALSE;
      
   if (MOUSE_IS_BUTTON(joycode))
   {
      int mouse  = MOUSE_GET_MOUSE(joycode);
      int button = MOUSE_GET_BUTTON(joycode);
      return mouse_data[mouse].buttons[button];
   }
   else
   {
      int joy = JOY_GET_JOY(joycode);
      
      if (JOY_IS_AXIS(joycode))
      {
         int axis = JOY_GET_AXIS(joycode);
         int dir  = JOY_GET_DIR(joycode);
         return joy_data[joy].axis[axis].dirs[dir];
      }
      else
      {
         int button = JOY_GET_BUTTON(joycode);
         return joy_data[joy].buttons[button];
      }
   }
}

/*
 * given a new x an y joystick axis value convert it to a move definition
 */

void joy_evaluate_moves (void)
{
   int i, j, treshold;

   for (i=0; i<JOY; i++)
   {
      for (j=0; j<joy_data[i].num_axis; j++)
      {
         memset(joy_data[i].axis[j].dirs, FALSE, JOY_DIRS*sizeof(int));

         /* auto calibrate */
         if (joy_data[i].axis[j].val > joy_data[i].axis[j].max)
            joy_data[i].axis[j].max = joy_data[i].axis[j].val;
         else if (joy_data[i].axis[j].val < joy_data[i].axis[j].min)
            joy_data[i].axis[j].min = joy_data[i].axis[j].val;

         treshold = (joy_data[i].axis[j].max - joy_data[i].axis[j].center) >> 1;

         if (joy_data[i].axis[j].val < (joy_data[i].axis[j].center - treshold))
            joy_data[i].axis[j].dirs[0] = TRUE;
         else if (joy_data[i].axis[j].val > (joy_data[i].axis[j].center + treshold))
            joy_data[i].axis[j].dirs[1] = TRUE;
      }
   }
}

/* 
 * return a value in the range -128 .. 128 (yes, 128, not 127)
 */
void osd_analogjoy_read(int player,int *analog_x, int *analog_y)
{
   int i,val;
   
   if (player < JOY && analogstick && joy_data[player].num_axis >= 2)
   {
      for (i=0; i<2; i++)
      {
         if (joy_data[player].axis[i].val > joy_data[player].axis[i].center)
            val = (128 *
                   (joy_data[player].axis[i].val - joy_data[player].axis[i].center)) /
                  (joy_data[player].axis[i].max - joy_data[player].axis[i].center);
         else
            val = (128 *
                   (joy_data[player].axis[i].val - joy_data[player].axis[i].center)) /
                  (joy_data[player].axis[i].center - joy_data[player].axis[i].min);
         switch(i)
         {
            case 0:
               *analog_x = val;
               break;
            case 1:
               *analog_y = val;
               break;
         }
      }
   }
   else
      *analog_x = *analog_y = 0;
}

int osd_joystick_needs_calibration (void)
{
   /* xmame uses the kernels joystick drivers calibration, or autocalibration
      and thus never needs this */
   return 0;
}

void osd_joystick_start_calibration (void)
{
}

const char *osd_joystick_calibrate_next (void)
{
   return NULL;
}

void osd_joystick_calibrate (void)
{
}

void osd_joystick_end_calibration (void)
{
}
