#ifndef _POPUP_H_
#define _POPUP_H_ 1
typedef struct menu_context menu_context;
typedef void (*menu_callback) (menu_context *ctx);
typedef struct menu_context {
  int active;
  int selected; // initial cursor location, then selected item.
  char selected_str[24]; // initial cursor location, then selected item.
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
  char buff[32][24]; // max 12 lines of up to 22 characters
};
extern unsigned char lcrasterline1[];
extern unsigned char lcrasterline2[];
extern unsigned char lcrasterline3[];
extern unsigned char lcrasterline4[];
extern unsigned char lcrasterline5[];
extern unsigned char lcrasterline6[];
extern unsigned char lcrasterline7[];
extern unsigned char *lcrasterlines[7];
extern int kbhit(void)
extern void echoOff(void)
extern void echoOn(void)
extern void CreateMenu(menu_context *m, char *title, char *options);
extern int DrawMenu(menu_context *m);
extern void m_line(int xl, int yb, int xr, int yt, int col, menu_context *menu); // to do: add pattern
// menu versions of v_* which will fail to draw a hole for the popup menu:
extern void m_directDraw32 (int xl, int yb, int xr, int yt, int bri, menu_context *menu);
extern void m_directDraw32Patterned (int xl, int yb, int xr, int yt, int pattern, int bri, menu_context *menu);
extern void CursorUp(menu_context *m);
extern void CursorDown(menu_context *m);
extern void Select(menu_context *m);  // B4 - perform action if 'selected' is an action. If it's a submenu enter it. (TO DO)
extern void Cancel(menu_context *m);  // B3 - pop up if submenu, exit menu if top-level (TO DO)
extern void Down(menu_context *m);    // B2 = make B2 behave like joystick down
extern void Up(menu_context *m);      // B1 = make B1 behave like joystick up
#endif // _POPUP_H_
