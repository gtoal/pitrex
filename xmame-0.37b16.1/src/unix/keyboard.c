/*
 * X-Mame keyboard specifics code
 *
 */
#define __KEYBOARD_C_

/*
 * Include files.
 */
#include "xmame.h"
#include "driver.h"
#include "keyboard.h"
#include "sysdep/fifo.h"
 
#if defined svgalib || defined svgafx
#include <vgakeyboard.h>
#define sysdep_update_keyboard keyboard_update
#endif

/* we use standard pc scancodes these have a one-on-one mapping for
   svgalib, for other targets we need a lookup table anyway */
struct KeyboardInfo keylist[] =
{
	{ "A",		KEY_A,		KEYCODE_A },
	{ "B",		KEY_B,		KEYCODE_B },
	{ "C",		KEY_C,		KEYCODE_C },
	{ "D",		KEY_D,		KEYCODE_D },
	{ "E",		KEY_E,		KEYCODE_E },
	{ "F",		KEY_F,		KEYCODE_F },
	{ "G",		KEY_G,		KEYCODE_G },
	{ "H",		KEY_H,		KEYCODE_H },
	{ "I",		KEY_I,		KEYCODE_I },
	{ "J",		KEY_J,		KEYCODE_J },
	{ "K",		KEY_K,		KEYCODE_K },
	{ "L",		KEY_L,		KEYCODE_L },
	{ "M",		KEY_M,		KEYCODE_M },
	{ "N",		KEY_N,		KEYCODE_N },
	{ "O",		KEY_O,		KEYCODE_O },
	{ "P",		KEY_P,		KEYCODE_P },
	{ "Q",		KEY_Q,		KEYCODE_Q },
	{ "R",		KEY_R,		KEYCODE_R },
	{ "S",		KEY_S,		KEYCODE_S },
	{ "T",		KEY_T,		KEYCODE_T },
	{ "U",		KEY_U,		KEYCODE_U },
	{ "V",		KEY_V,		KEYCODE_V },
	{ "W",		KEY_W,		KEYCODE_W },
	{ "X",		KEY_X,		KEYCODE_X },
	{ "Y",		KEY_Y,		KEYCODE_Y },
	{ "Z",		KEY_Z,		KEYCODE_Z },
	{ "0",		KEY_0,		KEYCODE_0 },
	{ "1",		KEY_1,		KEYCODE_1 },
	{ "2",		KEY_2,		KEYCODE_2 },
	{ "3",		KEY_3,		KEYCODE_3 },
	{ "4",		KEY_4,		KEYCODE_4 },
	{ "5",		KEY_5,		KEYCODE_5 },
	{ "6",		KEY_6,		KEYCODE_6 },
	{ "7",		KEY_7,		KEYCODE_7 },
	{ "8",		KEY_8,		KEYCODE_8 },
	{ "9",		KEY_9,		KEYCODE_9 },
	{ "0 PAD",	KEY_0_PAD,	KEYCODE_0_PAD },
	{ "1 PAD",	KEY_1_PAD,	KEYCODE_1_PAD },
	{ "2 PAD",	KEY_2_PAD,	KEYCODE_2_PAD },
	{ "3 PAD",	KEY_3_PAD,	KEYCODE_3_PAD },
	{ "4 PAD",	KEY_4_PAD,	KEYCODE_4_PAD },
	{ "5 PAD",	KEY_5_PAD,	KEYCODE_5_PAD },
	{ "6 PAD",	KEY_6_PAD,	KEYCODE_6_PAD },
	{ "7 PAD",	KEY_7_PAD,	KEYCODE_7_PAD },
	{ "8 PAD",	KEY_8_PAD,	KEYCODE_8_PAD },
	{ "9 PAD",	KEY_9_PAD,	KEYCODE_9_PAD },
	{ "F1",		KEY_F1,		KEYCODE_F1 },
	{ "F2",		KEY_F2,		KEYCODE_F2 },
	{ "F3",		KEY_F3,		KEYCODE_F3 },
	{ "F4",		KEY_F4,		KEYCODE_F4 },
	{ "F5",		KEY_F5,		KEYCODE_F5 },
	{ "F6",		KEY_F6,		KEYCODE_F6 },
	{ "F7",		KEY_F7,		KEYCODE_F7 },
	{ "F8",		KEY_F8,		KEYCODE_F8 },
	{ "F9",		KEY_F9,		KEYCODE_F9 },
	{ "F10",	KEY_F10,	KEYCODE_F10 },
	{ "F11",	KEY_F11,	KEYCODE_F11 },
	{ "F12",	KEY_F12,	KEYCODE_F12 },
	{ "ESC",	KEY_ESC,	KEYCODE_ESC },
	{ "~",		KEY_TILDE,	KEYCODE_TILDE },
	{ "-",		KEY_MINUS,	KEYCODE_MINUS },
	{ "=",		KEY_EQUALS,	KEYCODE_EQUALS },
	{ "BKSPACE",	KEY_BACKSPACE,	KEYCODE_BACKSPACE },
	{ "TAB",	KEY_TAB,	KEYCODE_TAB },
	{ "[",		KEY_OPENBRACE,	KEYCODE_OPENBRACE },
	{ "]",		KEY_CLOSEBRACE,	KEYCODE_CLOSEBRACE },
	{ "ENTER",	KEY_ENTER,	KEYCODE_ENTER },
	{ ";",		KEY_COLON,	KEYCODE_COLON },
	{ ":",		KEY_QUOTE,	KEYCODE_QUOTE },
	{ "\\",		KEY_BACKSLASH,	KEYCODE_BACKSLASH },
	{ "<",		KEY_BACKSLASH2,	KEYCODE_BACKSLASH2 },
	{ ",",		KEY_COMMA,	KEYCODE_COMMA },
	{ ".",		KEY_STOP,	KEYCODE_STOP },
	{ "/",		KEY_SLASH,	KEYCODE_SLASH },
	{ "SPACE",	KEY_SPACE,	KEYCODE_SPACE },
	{ "INS",	KEY_INSERT,	KEYCODE_INSERT },
	{ "DEL",	KEY_DEL,	KEYCODE_DEL },
	{ "HOME",	KEY_HOME,	KEYCODE_HOME },
	{ "END",	KEY_END,	KEYCODE_END },
	{ "PGUP",	KEY_PGUP,	KEYCODE_PGUP },
	{ "PGDN",	KEY_PGDN,	KEYCODE_PGDN },
	{ "LEFT",	KEY_LEFT,	KEYCODE_LEFT },
	{ "RIGHT",	KEY_RIGHT,	KEYCODE_RIGHT },
	{ "UP",		KEY_UP,		KEYCODE_UP },
	{ "DOWN",	KEY_DOWN,	KEYCODE_DOWN },
	{ "/ PAD",	KEY_SLASH_PAD,	KEYCODE_SLASH_PAD },
	{ "* PAD",	KEY_ASTERISK,	KEYCODE_ASTERISK },
	{ "- PAD",	KEY_MINUS_PAD,	KEYCODE_MINUS_PAD },
	{ "+ PAD",	KEY_PLUS_PAD,	KEYCODE_PLUS_PAD },
	{ ". PAD",	KEY_DEL_PAD,	KEYCODE_DEL_PAD },
	{ "ENTER PAD",	KEY_ENTER_PAD,	KEYCODE_ENTER_PAD },
	{ "PRTSCR",	KEY_PRTSCR,	KEYCODE_PRTSCR },
	{ "PAUSE",	KEY_PAUSE,	KEYCODE_PAUSE },
	{ "PAUSE",	KEY_PAUSE_ALT,	KEYCODE_PAUSE },
	{ "LSHIFT",	KEY_LSHIFT,	KEYCODE_LSHIFT },
	{ "RSHIFT",	KEY_RSHIFT,	KEYCODE_RSHIFT },
	{ "LCTRL",	KEY_LCONTROL,	KEYCODE_LCONTROL },
	{ "RCTRL",	KEY_RCONTROL,	KEYCODE_RCONTROL },
	{ "ALT",	KEY_ALT,	KEYCODE_LALT },
	{ "ALTGR",	KEY_ALTGR,	KEYCODE_RALT },
	{ "LWIN",	KEY_LWIN,	KEYCODE_OTHER },
	{ "RWIN",	KEY_RWIN,	KEYCODE_OTHER },
	{ "MENU",	KEY_MENU,	KEYCODE_OTHER },
	{ "SCRLOCK",	KEY_SCRLOCK,	KEYCODE_SCRLOCK },
	{ "NUMLOCK",	KEY_NUMLOCK,	KEYCODE_NUMLOCK },
	{ "CAPSLOCK",	KEY_CAPSLOCK,	KEYCODE_CAPSLOCK },
	{ 0, 0, 0 }	/* end of table */
};

struct kbd_fifo_struct;
static struct kbd_fifo_struct *kbd_fifo = NULL;
static char key[KEY_MAX];

extern int use_hotrod;

#ifdef RASPI
unsigned int trying_to_quit = 0;
#endif


/* private methods */
FIFO(INLINE, kbd, struct keyboard_event)

/* public methods (in keyboard.h / osdepend.h) */
int keyboard_init(void)
{
   memset(key, 0, KEY_MAX);

   kbd_fifo = kbd_fifo_create(256);
   if(!kbd_fifo)
      return -1;
   
   return 0;
}

void keyboard_exit()
{
   if(kbd_fifo)
      kbd_fifo_destroy(kbd_fifo);
}

void keyboard_register_event(struct keyboard_event *event)
{
   /* register the event in our event fifo */
   kbd_fifo_put(kbd_fifo, *event);
   
   /* and update the key array */
   key[event->scancode] = event->press;
}

void keyboard_clear(void)
{
   kbd_fifo_empty(kbd_fifo);
   memset(key, 0, KEY_MAX);
}

/* return a list of all available keys */
const struct KeyboardInfo *osd_get_key_list(void)
{
   return keylist;
}

#ifndef MESS
#ifndef TINY_COMPILE
#ifndef CPSMAME
extern struct GameDriver driver_neogeo;
#endif
#endif
#endif

void osd_customize_inputport_defaults(struct ipd *defaults)
{
	if (use_hotrod)
	{
		while (defaults->type != IPT_END)
		{
			int j;
			for(j=0;j<SEQ_MAX;++j)
			{
				if (defaults->seq[j] == KEYCODE_UP) defaults->seq[j] = KEYCODE_8_PAD;
				if (defaults->seq[j] == KEYCODE_DOWN) defaults->seq[j] = KEYCODE_2_PAD;
				if (defaults->seq[j] == KEYCODE_LEFT) defaults->seq[j] = KEYCODE_4_PAD;
				if (defaults->seq[j] == KEYCODE_RIGHT) defaults->seq[j] = KEYCODE_6_PAD;
			}
			if (defaults->type == IPT_UI_SELECT) seq_set_1(&defaults->seq, KEYCODE_LCONTROL);
			if (defaults->type == IPT_START1) seq_set_1(&defaults->seq, KEYCODE_1);
			if (defaults->type == IPT_START2) seq_set_1(&defaults->seq, KEYCODE_2);
			if (defaults->type == IPT_COIN1)  seq_set_1(&defaults->seq, KEYCODE_3);
			if (defaults->type == IPT_COIN2)  seq_set_1(&defaults->seq, KEYCODE_4);
			if (defaults->type == (IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_R);
			if (defaults->type == (IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_F);
			if (defaults->type == (IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_D);
			if (defaults->type == (IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_G);
			if (defaults->type == (IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_8_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_2_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_4_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_6_PAD);
			if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_LCONTROL);
			if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_LALT);
			if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_SPACE);
			if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_LSHIFT);
			if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_Z);
			if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_X);
			if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_A);
			if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_S);
			if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_Q);
			if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_W);
			if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_E);
			if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_OPENBRACE);

#ifndef MESS
#ifndef TINY_COMPILE
#ifndef CPSMAME
			if (use_hotrod == 2 &&
					(Machine->gamedrv->clone_of == &driver_neogeo ||
					(Machine->gamedrv->clone_of && Machine->gamedrv->clone_of->clone_of == &driver_neogeo)))
			{
				if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_C);
				if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_LSHIFT);
				if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_Z);
				if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_X);
				if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON7 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON8 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_CLOSEBRACE);
				if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_W);
				if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_E);
				if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_OPENBRACE);
				if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON7 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON8 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_NONE);
			}
#endif
#endif
#endif

			defaults++;
		}
	}
}

int osd_is_key_pressed(int keycode)
{
   /* blames to the dos-people who want to check key states before
      the display (and under X thus the keyboard) is initialised */
   if (!kbd_fifo)
      return 0;
   
   if (keycode >= KEY_MAX)
      return 0;
#ifdef RASPI
    /* special case: if we're trying to quit, fake up/down/up/down */
    if (keycode == KEY_ESC && trying_to_quit)
    {
	static int dummy_state = 1;
	return dummy_state ^= 1;
    }
#endif


   sysdep_update_keyboard();
	
   return key[keycode];
}

int osd_readkey_unicode(int flush)
{
   struct keyboard_event event;
   
   /* blames to the dos-people who want to check key states before
      the display (and under X thus the keyboard) is initialised */
   if (!kbd_fifo)
      return 0;
   
   if (flush)
      keyboard_clear();
   
   sysdep_update_keyboard();
   
   if (!kbd_fifo_get(kbd_fifo, &event) && event.press)
      return event.unicode;
   else
      return 0;
}
