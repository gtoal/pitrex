#ifndef __DEVICES_H_
#define __DEVICES_H_

#ifdef __DEVICES_C_
#define EXTERN
#else
#define EXTERN extern
#endif

#define JOY 6
#define JOY_BUTTONS 16
#define JOY_AXIS 8
#define JOY_DIRS 2
#define JOY_LIST_AXIS_ENTRIES (JOY_AXIS * JOY_DIRS)
#define JOY_LIST_TOTAL_ENTRIES (JOY_LIST_AXIS_ENTRIES + JOY_BUTTONS)
#define JOY_LIST_LEN (JOY * JOY_LIST_TOTAL_ENTRIES)
#define JOY_NAME_LEN 20

/* only one mouse for now */
#define MOUSE 1
#define MOUSE_BUTTONS 8
#define MOUSE_AXIS 8

/* now axis entries in the mouse_list, these are get through another way,
   like the analog joy-values */
#define MOUSE_LIST_TOTAL_ENTRIES MOUSE_BUTTONS
#define MOUSE_LIST_LEN (MOUSE * MOUSE_LIST_TOTAL_ENTRIES)


#define JOY_BUTTON_CODE(joy, button) \
 (joy * JOY_LIST_TOTAL_ENTRIES + JOY_LIST_AXIS_ENTRIES + button)

#define MOUSE_BUTTON_CODE(mouse, button) \
 (JOY_LIST_LEN + mouse * MOUSE_LIST_TOTAL_ENTRIES + button)
  
#define JOY_AXIS_CODE(joy, axis, dir) \
 (joy * JOY_LIST_TOTAL_ENTRIES + JOY_DIRS * axis + dir)

/* mouse doesn't support axis this way */
 
#define JOY_GET_JOY(code) \
 (code / JOY_LIST_TOTAL_ENTRIES)

#define MOUSE_GET_MOUSE(code) \
 ((code - JOY_LIST_LEN) / MOUSE_LIST_TOTAL_ENTRIES)
 
#define JOY_IS_AXIS(code) \
 ((code < JOY_LIST_LEN) && \
  ((code % JOY_LIST_TOTAL_ENTRIES) <  JOY_LIST_AXIS_ENTRIES))
  
/* mouse doesn't support axis */

#define JOY_IS_BUTTON(code) \
 ((code < JOY_LIST_LEN) && \
  (((code % JOY_LIST_TOTAL_ENTRIES) >= JOY_LIST_AXIS_ENTRIES))

#define MOUSE_IS_BUTTON(code) \
 (code >= JOY_LIST_LEN) 

#define JOY_GET_AXIS(code) \
 ((code % JOY_LIST_TOTAL_ENTRIES) / JOY_DIRS)
 
/* mouse doesn't support axis this way */
 
#define JOY_GET_DIR(code) \
 ((code % JOY_LIST_TOTAL_ENTRIES) % JOY_DIRS)
 
/* mouse doesn't support axis this way */

#define JOY_GET_BUTTON(code) \
 ((code % JOY_LIST_TOTAL_ENTRIES) -  JOY_LIST_AXIS_ENTRIES)

#define MOUSE_GET_BUTTON(code) \
 ((code - JOY_LIST_LEN) % MOUSE_LIST_TOTAL_ENTRIES)

enum { JOY_NONE, JOY_I386, JOY_PAD, JOY_X11, JOY_I386NEW, JOY_USB, JOY_SDL };

/*** variables ***/

struct axisdata_struct
{
   /* current value */
   int val;
   /* calibration data */
   int min;
   int center;
   int max;
   /* boolean values */
   int dirs[JOY_DIRS];
};

struct joydata_struct
{
   int fd;
   int num_axis;
   int num_buttons;
   struct axisdata_struct axis[JOY_AXIS];
   int buttons[JOY_BUTTONS];
};

struct mousedata_struct
{
   int buttons[MOUSE_BUTTONS];
   int deltas[MOUSE_AXIS];
};

EXTERN struct joydata_struct joy_data[JOY];
EXTERN struct mousedata_struct mouse_data[MOUSE];
EXTERN void (*joy_poll_func) (void);
EXTERN int joytype;
EXTERN int use_hotrod;

extern struct rc_option joy_i386_opts[];
extern struct rc_option joy_pad_opts[];
extern struct rc_option joy_x11_opts[];
extern struct rc_option joy_usb_opts[];

/*** prototypes ***/
void joy_evaluate_moves(void);
void joy_i386_init(void);
void joy_pad_init(void);
void joy_x11_init(void);
void joy_usb_init(void);

#undef EXTERN
#endif
