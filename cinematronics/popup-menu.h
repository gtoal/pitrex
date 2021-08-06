#ifndef _POPUP_H_
#define _POPUP_H_ 1

// The support for menus and especially the m_direct* calls should be added to the default
// library calls rather than asking the user to call an extra layer.  I have it at the back
// of my mind that the windowing support in the system calls is slightly broken which is
// why I've split off this local application-specific layer, but I'm thinking now I really
// need to go back to the system version and just make sure that everything there works and
// that both standard windowing *and* reverse-windowing for the menu hole are implemented
// at that level.

#define KEY_ESCAPE 27
#define KEY_UP     257
#define KEY_DOWN   258
#define KEY_LEFT   259
#define KEY_RIGHT  260


#define MAX_MENU_LINES 32
#define MAX_MENUITEM_WIDTH 64
typedef struct menu_context menu_context;
typedef void (*menu_callback) (menu_context *ctx);
typedef struct menu_context {
  int active;
  int selected; // initial cursor location, then selected item.
  char selected_str[MAX_MENUITEM_WIDTH]; // initial cursor location, then selected item.
  int xl, yb, xr, yt; // in virtual coordinates
  int txt_xl, txt_yb, txt_xr, txt_yt; // in raw coordinates for raster text display. Up to user to position them properly
  int scroll_base; // If the menu has scrolled 2 lines off the top then scroll_base would equal 2.
  int width, lines; // calculated from menu string
  int displayable_lines; // calculated when menu is created
  int display_line; // calculated when menu is created
  //void (*up) (struct menu_context *ctx);
  menu_callback up; // called on joystick movement before menu code updates screen
  menu_callback down;
  menu_callback left;
  menu_callback right;
  menu_callback b1; // called when button pressed (also before menu code updates screen)
  menu_callback b2;
  menu_callback b3;
  menu_callback b4;
  char *title; // if NULL, no title bar
  char *options; // text to display menu options
  char buff[MAX_MENU_LINES][MAX_MENUITEM_WIDTH]; // max 12 lines of up to 22 characters - bounds checking not yet added to CreateMenu!  BE CAREFUL!
} menu_context;

extern menu_context *GLOBAL_MENU; // for code where it is not practical to pass the menu in each call.

// I'm tempted to replace the default font with this lower-case-supporting version everywhere, since
// I'm generally adding it to every single program...
extern unsigned char lcrasterline1[];
extern unsigned char lcrasterline2[];
extern unsigned char lcrasterline3[];
extern unsigned char lcrasterline4[];
extern unsigned char lcrasterline5[];
extern unsigned char lcrasterline6[];
extern unsigned char lcrasterline7[];
extern unsigned char *lcrasterlines[7];

// These shoulkd either go, or be fully expanded to ncurses-like utility.
extern int kbhit(void);
extern void echoOff(void);
extern void echoOn(void);

extern void CreateMenu(menu_context *m, char *title, char *options);
extern void DrawMenu(menu_context *m);

// replacement line-drawing code. There nNeeds to also be a non-clipped call for drawing in
// the menu body itself. I think that's missing from this header as I've never done that yet.
// Currently the menu only contains text (which we can't clip accurately) and boundary lines.
extern void m_line(int xl, int yb, int xr, int yt, int col, menu_context *menu); // to do: add pattern
// menu versions of v_* which will fail to draw a hole for the popup menu:
extern void m_directDraw32 (int xl, int yb, int xr, int yt, int bri); // Uses GLOBAL_MENU
extern void m_directDraw32Patterned (int xl, int yb, int xr, int yt, int pattern, int bri); // Uses GLOBAL_MENU

// These are actually user-level procedures, I should remove them from the popup-menu.c library code.
// Which I will do shortly after I confirm these are not needed in a typical user program.

// called on button presses:
extern  int popup_menu_Select(menu_context *m);  // B4 - perform action if 'selected' is an action. If it's a submenu enter it. (TO DO)
extern void popup_menu_Cancel(menu_context *m);  // B3 - pop up if submenu, exit menu if top-level (TO DO)
extern void popup_menu_Down(menu_context *m);    // B2 = make B2 behave like joystick down
extern void popup_menu_Up(menu_context *m);      // B1 = make B1 behave like joystick up

#endif // _POPUP_H_
