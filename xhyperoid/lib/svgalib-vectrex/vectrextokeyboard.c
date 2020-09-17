/* Translation of SVGAlib function calls into calls to the Vector drawing library for the PiTrex Vectrex interface cartridge.
 * Version 0.1 - only bare minimum of functions implemented to get something working.
 * - Functions to take Vectrex controller inputs as keyboard inputs.
 * Kevin Koster, 2020
 */

#include <stdio.h>
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include "svgalib-vectrex.h"
#include "vectrextokeyboard.h"
#include "vectrexcontrollermap.h"

static int translatemode = 0;
static unsigned char state[NR_KEYS];

int keyboard_init(void)
{   /* This will all have been done already if vga_init has been called */
	if (!svgalib_initialised)
	{
	  if (single_point_of_init() == -1) return -1;
	  svgalib_initialised = 1;
	  return 0;
	}
	else
	 return 0;
}

void keyboard_close(void)
{

}

static int checkscancode(int scancode)
{
    if (scancode < 0 || scancode >= NR_KEYS) {
	printf("svgalib: keyboard scancode out of range (%d).\n",
	       scancode);
	return 1;
    }
    return 0;
}

static void default_handler(int scancode, int newstate)
{
    if (checkscancode(scancode))
	return;
	
    if (translatemode & TRANSLATE_DIAGONAL)
/* Translate two keypad cursor keys to diagonal keypad keys. */   
     switch (scancode)
     {
     	case SCANCODE_CURSORLEFT:
	        if (state[SCANCODE_CURSORUP] && newstate)
             state[SCANCODE_CURSORUPLEFT] = 1;
            if (state[SCANCODE_CURSORDOWN] && newstate)
             state[SCANCODE_CURSORDOWNLEFT] = 1;
            break;
        case SCANCODE_CURSORRIGHT:
           if (state[SCANCODE_CURSORUP] && newstate)
             state[SCANCODE_CURSORUPRIGHT] = 1;
           if (state[SCANCODE_CURSORDOWN] && newstate)
            state[SCANCODE_CURSORDOWNRIGHT] = 1;
           break;
        case SCANCODE_CURSORUP:
           if (state[SCANCODE_CURSORLEFT] && newstate)
             state[SCANCODE_CURSORUPLEFT] = 1;
           if (state[SCANCODE_CURSORRIGHT] && newstate)
             state[SCANCODE_CURSORUPRIGHT] = 1;
            break;
        case SCANCODE_CURSORDOWN:
           if (state[SCANCODE_CURSORLEFT] && newstate)
             state[SCANCODE_CURSORDOWNLEFT] = 1;
           if (state[SCANCODE_CURSORRIGHT] && newstate)
             state[SCANCODE_CURSORDOWNRIGHT] = 1;
            break;
     }
    else
	{
     switch (scancode)
     {
     	case SCANCODE_CURSORLEFT:
	    /* If UP is also pressed, change state of SCANCODE_CURSORUPLEFT instead of SCANCODE_CURSORLEFT.
	       Clear SCANCODE_CURSORUP because we're not meant to be in TRANSLATE_DIAGONAL mode (this is all
	       backwards compared to how things work in real SVGAlib).*/
	    if (state[SCANCODE_CURSORUP])
	    {
	     scancode = SCANCODE_CURSORUPLEFT;
	     if (newstate)
	      state[SCANCODE_CURSORUP] = 0;
	    }
	    if (state[SCANCODE_CURSORDOWN])
	    {
	     scancode = SCANCODE_CURSORDOWNLEFT;
	     if (newstate)
	      state[SCANCODE_CURSORDOWN] = 0;
	    }
	    break;
	 case SCANCODE_CURSORRIGHT:
	    if (state[SCANCODE_CURSORUP])
	    {
	     scancode = SCANCODE_CURSORUPRIGHT;
	     if (newstate)
	      state[SCANCODE_CURSORUP] = 0;
	    }
	    if (state[SCANCODE_CURSORDOWN])
	    {
	     scancode = SCANCODE_CURSORDOWNRIGHT;
	     if (newstate)
	      state[SCANCODE_CURSORDOWN] = 0;
	    }
	    break;
	 case SCANCODE_CURSORUP:
	    if (state[SCANCODE_CURSORRIGHT])
	    {
	     scancode = SCANCODE_CURSORUPRIGHT;
	     if (newstate)
	      state[SCANCODE_CURSORRIGHT] = 0;
	    }
	    if (state[SCANCODE_CURSORLEFT])
	    {
	     scancode = SCANCODE_CURSORUPLEFT;
	     if (newstate)
	      state[SCANCODE_CURSORLEFT] = 0;
	    }
	    break;
	 case SCANCODE_CURSORDOWN:
	    if (state[SCANCODE_CURSORRIGHT])
	    {
	     scancode = SCANCODE_CURSORDOWNRIGHT;
	     if (newstate)
	      state[SCANCODE_CURSORRIGHT] = 0;
	    }
	    if (state[SCANCODE_CURSORLEFT])
	    {
	     scancode = SCANCODE_CURSORDOWNLEFT;
	     if (newstate)
	      state[SCANCODE_CURSORLEFT] = 0;
	    }
	    break;
	 }
	}

#if 0				/* This happens very often. EDIT - Or did with real SVGAlib. */
    if (state[scancode] == newstate) {
	printf("svgalib: keyboard event does not match (scancode = %d)\n",
	       scancode);
	return;
    }
#endif
    state[scancode] = newstate;
}

static int keyboard_getevents(int wait)
{
/* Read keyboard device, and handle events. */
/* If wait == 1, process at least one event and return. */
/* If wait == 0, handle all accumulated events; return 0 if no events */
/* were handled, 1 otherwise. */
	unsigned char oldButtonState = currentButtonState;
	char oldJoy1X = currentJoy1X;
	char oldJoy1Y = currentJoy1Y;
	
	v_readJoystick1Digital();
	v_readButtons();

	if (currentButtonState == oldButtonState &&
	    currentJoy1X == oldJoy1X &&
	    currentJoy1Y == oldJoy1Y)
	{
	 /* If wait == 1, this loops until an input is changed. Want to refresh display from the
	    vector buffer here once we have one implemented. */
	 if (wait)
	  do
	  {
	   v_WaitRecal();
	   v_readJoystick1Digital();
	   v_readButtons();
	  } while (currentButtonState == oldButtonState &&
	           currentJoy1X == oldJoy1X &&
	           currentJoy1Y == oldJoy1Y);
	 else
	  return 0; /* Nothing changed */
	}

	 keyboard_clearstate();

	 if (currentButtonState & 0x01)
	  default_handler(PORT1BUT1, 1);
	 if (currentButtonState & 0x02)
	  default_handler(PORT1BUT2, 1);
	 if (currentButtonState & 0x04)
	  default_handler(PORT1BUT3, 1);
	 if (currentButtonState & 0x08)
	  default_handler(PORT1BUT4, 1);
	 if (currentButtonState & 0x10)
	  default_handler(PORT2BUT1, 1);
	 if (currentButtonState & 0x20)
	  default_handler(PORT2BUT2, 1);
	 if (currentButtonState & 0x40)
	  default_handler(PORT2BUT3, 1);
	 if (currentButtonState & 0x80)
	  default_handler(PORT2BUT4, 1);

	 if (currentJoy1Y == -1)
	  default_handler(JOY1DOWN, 1);
	 if (currentJoy1Y == 1)
	  default_handler(JOY1UP, 1);
	 if (currentJoy1X == -1)
	  default_handler(JOY1LEFT, 1);
	 if (currentJoy1X == 1)
	  default_handler(JOY1RIGHT, 1);

	 return 1;
}

char *
 keyboard_getstate(void)
{
    return state;
}

void keyboard_clearstate(void)
{
	default_handler(JOY1UP, 0);
	default_handler(JOY1DOWN, 0);
	default_handler(JOY1LEFT, 0);
	default_handler(JOY1RIGHT, 0);

	default_handler(PORT1BUT1, 0);
	default_handler(PORT1BUT2, 0);
	default_handler(PORT1BUT3, 0);
	default_handler(PORT1BUT4, 0);
	default_handler(PORT2BUT1, 0);
	default_handler(PORT2BUT2, 0);
	default_handler(PORT2BUT3, 0);
	default_handler(PORT2BUT4, 0);
	
	state[SCANCODE_CURSORUPLEFT] = 0;
	state[SCANCODE_CURSORUPRIGHT] = 0;
	state[SCANCODE_CURSORDOWNLEFT] = 0;
	state[SCANCODE_CURSORDOWNRIGHT] = 0;
}

int keyboard_update(void)
{
    return keyboard_getevents(0);	/* Don't wait. */
}

void keyboard_waitforupdate(void)
{
    keyboard_getevents(1);	/* Wait for event. */
    return;
}

void keyboard_translatekeys(int mask)
{
    translatemode = mask;
}

int keyboard_keypressed(int scancode)
{
    if (checkscancode(scancode))
	 return 0;
    return state[scancode];
}
