/* zblast - simple shoot-em-up.
 * Copyright (C) 1993-2000 Russell Marks. See zblast.c for license.
 *
 * joy.c - Linux joystick/joypad handler.
 */

/* I believe this requires version >=1.2.x of the driver, though
 * that's not especially clear. But that dates from 1998 or earlier,
 * so it's probably not that bad a requirement.
 */

/* XXX should read up on this, and check we have the right version
 * (at runtime)
 */

#ifdef JOYSTICK_SUPPORT

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include "joy.h"

#define JOYSTICK_DEV	"/dev/js0"

/* these are assumed to be equidistant from zero */
#define AXIS_MIN	(-32767)
#define AXIS_MAX	(32767)

/* there's a central `dead area' for each axis
 * (just under 10% either way of centre)
 */
#define DEAD_AREA_MIN	(-3000)
#define DEAD_AREA_MAX	(3000)


static struct joystate_tag joystate;

static int joyfd=-1;


int joy_init(void)
{
int f;

for(f=0;f<MAX_AXES;f++)
  joystate.axis[f]=joystate.digital_axis[f]=0;

for(f=0;f<MAX_BUTS;f++)
  joystate.but[f]=0;

if((joyfd=open(JOYSTICK_DEV,O_RDONLY|O_NONBLOCK))<0)
  return(0);

/* read joystick spec */
if(ioctl(joyfd,JSIOCGAXES,&joystate.num_axes)==-1)
  perror("joystick axes ioctl failed"),joystate.num_axes=2;
if(ioctl(joyfd,JSIOCGBUTTONS,&joystate.num_buts)==-1)
  perror("joystick buttons ioctl failed"),joystate.num_buts=2;

return(1);
}


struct joystate_tag *joy_update(void)
{
struct js_event ev;
int pos;

if(joyfd<0) return(NULL);

while(read(joyfd,&ev,sizeof(ev))>0)
  {
  switch(ev.type&~JS_EVENT_INIT)
    {
    case JS_EVENT_BUTTON:
      joystate.but[ev.number]=ev.value;
      break;
    
    case JS_EVENT_AXIS:
      joystate.axis[ev.number]=ev.value;
      pos=0;
      if(ev.value<DEAD_AREA_MIN) pos=-1;
      if(ev.value>DEAD_AREA_MAX) pos= 1;
      joystate.digital_axis[ev.number]=pos;
      break;
    }
  }

return(&joystate);
}


void joy_close(void)
{
if(joyfd>=0) close(joyfd);
}

#endif	/* JOYSTICK_SUPPORT */
