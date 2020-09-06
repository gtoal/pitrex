/* Translation of SVGAlib function calls into calls to the Vector drawing library for the PiTrex Vectrex interface cartridge.
 * Version 0.1 - only bare minimum of functions implemented to get something working.
 * - Functions to take Vectrex controller inputs as keyboard inputs.
 * Kevin Koster, 2020
 */

/* Missing functions:
 int keyboard_init_return_fd(void);
 
 typedef void (*__keyboard_handler) (int scancode, int press);
    void keyboard_seteventhandler(__keyboard_handler handler);

 void keyboard_setdefaulteventhandler(void);
*/

#ifndef VECTREXTOKEYBOARD_H
#define VECTREXTOKEYBOARD_H

#define SCANCODE_ESCAPE			1

#define SCANCODE_1			2
#define SCANCODE_2			3
#define SCANCODE_3			4
#define SCANCODE_4			5
#define SCANCODE_5			6
#define SCANCODE_6			7
#define SCANCODE_7			8
#define SCANCODE_8			9
#define SCANCODE_9			10
#define SCANCODE_0			11

#define SCANCODE_MINUS			12
#define SCANCODE_EQUAL			13

#define SCANCODE_BACKSPACE		14
#define SCANCODE_TAB			15

#define SCANCODE_Q			16
#define SCANCODE_W			17
#define SCANCODE_E			18
#define SCANCODE_R			19
#define SCANCODE_T			20
#define SCANCODE_Y			21
#define SCANCODE_U			22
#define SCANCODE_I			23
#define SCANCODE_O			24
#define SCANCODE_P			25
#define SCANCODE_BRACKET_LEFT		26
#define SCANCODE_BRACKET_RIGHT		27

#define SCANCODE_ENTER			28

#define SCANCODE_LEFTCONTROL		29

#define SCANCODE_A			30
#define SCANCODE_S			31
#define SCANCODE_D			32
#define SCANCODE_F			33
#define SCANCODE_G			34
#define SCANCODE_H			35
#define SCANCODE_J			36
#define SCANCODE_K			37
#define SCANCODE_L			38
#define SCANCODE_SEMICOLON		39
#define SCANCODE_APOSTROPHE		40
#define SCANCODE_GRAVE			41

#define SCANCODE_LEFTSHIFT		42
#define SCANCODE_BACKSLASH		43

#define SCANCODE_Z			44
#define SCANCODE_X			45
#define SCANCODE_C			46
#define SCANCODE_V			47
#define SCANCODE_B			48
#define SCANCODE_N			49
#define SCANCODE_M			50
#define SCANCODE_COMMA			51
#define SCANCODE_PERIOD			52
#define SCANCODE_SLASH			53

#define SCANCODE_RIGHTSHIFT		54
#define SCANCODE_KEYPADMULTIPLY		55

#define SCANCODE_LEFTALT		56
#define SCANCODE_SPACE			57
#define SCANCODE_CAPSLOCK		58

#define SCANCODE_F1			59
#define SCANCODE_F2			60
#define SCANCODE_F3			61
#define SCANCODE_F4			62
#define SCANCODE_F5			63
#define SCANCODE_F6			64
#define SCANCODE_F7			65
#define SCANCODE_F8			66
#define SCANCODE_F9			67
#define SCANCODE_F10			68

#define SCANCODE_NUMLOCK		69
#define SCANCODE_SCROLLLOCK		70

#define SCANCODE_KEYPAD7		71
#define SCANCODE_CURSORUPLEFT		71
#define SCANCODE_KEYPAD8		72
#define SCANCODE_CURSORUP		72
#define SCANCODE_KEYPAD9		73
#define SCANCODE_CURSORUPRIGHT		73
#define SCANCODE_KEYPADMINUS		74
#define SCANCODE_KEYPAD4		75
#define SCANCODE_CURSORLEFT		75
#define SCANCODE_KEYPAD5		76
#define SCANCODE_KEYPAD6		77
#define SCANCODE_CURSORRIGHT		77
#define SCANCODE_KEYPADPLUS		78
#define SCANCODE_KEYPAD1		79
#define SCANCODE_CURSORDOWNLEFT		79
#define SCANCODE_KEYPAD2		80
#define SCANCODE_CURSORDOWN		80
#define SCANCODE_KEYPAD3		81
#define SCANCODE_CURSORDOWNRIGHT	81
#define SCANCODE_KEYPAD0		82
#define SCANCODE_KEYPADPERIOD		83

#define SCANCODE_LESS			86

#define SCANCODE_F11			87
#define SCANCODE_F12			88

#define SCANCODE_KEYPADENTER		96
#define SCANCODE_RIGHTCONTROL		97
#define SCANCODE_CONTROL		97
#define SCANCODE_KEYPADDIVIDE		98
#define SCANCODE_PRINTSCREEN		99
#define SCANCODE_RIGHTALT		100
#define SCANCODE_BREAK			101	/* Beware: is 119     */
#define SCANCODE_BREAK_ALTERNATIVE	119	/* on some keyboards! */

#define SCANCODE_HOME			102
#define SCANCODE_CURSORBLOCKUP		103	/* Cursor key block */
#define SCANCODE_PAGEUP			104
#define SCANCODE_CURSORBLOCKLEFT	105	/* Cursor key block */
#define SCANCODE_CURSORBLOCKRIGHT	106	/* Cursor key block */
#define SCANCODE_END			107
#define SCANCODE_CURSORBLOCKDOWN	108	/* Cursor key block */
#define SCANCODE_PAGEDOWN		109
#define SCANCODE_INSERT			110
#define SCANCODE_REMOVE			111

#define SCANCODE_RIGHTWIN		126
#define SCANCODE_LEFTWIN		125

#define KEY_EVENTRELEASE 0
#define KEY_EVENTPRESS 1

/* This is normally defined in linux/keyboard.h as "(KEY_MAX+1)",
 * where KEY_MAX is defined in linux/input.h on my system as 0x1ff (511) */
#define NR_KEYS	512

#define MAX_KEYNAME_LEN 20

/* Initialize keyboard handler (brings keyboard into RAW mode). Returns */
/* 0 if succesful, -1 otherwise. */
    int keyboard_init(void);

/* Return keyboard to normal state. */
    void keyboard_close(void);

/* Read raw keyboard device and handle events. Returns 0 if no event. */
    int keyboard_update(void);
/* Similar to keyboard_update, but wait for an event to happen. */
/* [This doesn't seem to work very well -- use select on fd] EDIT - The Vectrex version ought to work OK */
    void keyboard_waitforupdate(void);
/* keyboard_init sets default event handler that keeps track of complete */
/* keyboard state: */

/* Result of keypressed. */
#define KEY_NOTPRESSED 0
#define KEY_PRESSED 1

/* Modes for translatekeys. */
#define TRANSLATE_CURSORKEYS 1	/* Map cursor block to keypad cursor. */
#define TRANSLATE_DIAGONAL 2	/* Map keypad diagonal to keypad cursor. */
#define TRANSLATE_KEYPADENTER 4	/* Map keypad enter to main enter key. */
#define DONT_CATCH_CTRLC 8	/* Disable Crtl-C check. */

/* Return pointer to buffer holding state of each key (scancode). */
/* Value 1 corresponds to key that is pressed, 0 means not pressed. */
    char *keyboard_getstate(void);
/* Force keyboard state to nothing pressed (all zeroes). */
    void keyboard_clearstate(void);
/* Let default handler translate cursor key block events to numeric keypad */
/* cursor key events and other translations. */
    void keyboard_translatekeys(int mask);

/* Return nonzero if key is depressed. */
    int keyboard_keypressed(int scancode);

#endif /* VECTREXTOKEYBOARD_H */
