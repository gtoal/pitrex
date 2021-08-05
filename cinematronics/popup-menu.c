#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include <string.h>
#include <stdlib.h> // for exit()
extern int optimizationON;
// prototype pop-up menu

#ifndef TRUE
#define TRUE (0==0)
#define FALSE (0!=0)
#endif

enum { TOP = 0x1, BOTTOM = 0x2, RIGHT = 0x4, LEFT = 0x8 };
//static enum { FALSE, TRUE };
typedef unsigned int outcode;

static outcode compute_outcode (int x, int y, int xmin, int ymin, int xmax, int ymax) {
  outcode oc = 0;

  if (y > ymax)
    oc |= TOP;
  else if (y < ymin)
    oc |= BOTTOM;
  if (x > xmax)
    oc |= RIGHT;
  else if (x < xmin)
    oc |= LEFT;
  return oc;
}

// I'm tempted to restructure this so that instead of updating the parameters and letting the caller
// draw the lines, it just draws them itself, the same way I did with the 'hole' clipping later in the program.

static int retain_after_clipping (int *x1p, int *y1p, int *x2p, int *y2p,   /* line endpoints */
				  int xmin, int ymin, int xmax, int ymax) { /* window */
#define x1 (*x1p) /* since plain C doesn't have "int &x1" parameters */
#define y1 (*y1p) /* (or as Atlas Autocode would have put it, %integername x1) */
#define x2 (*x2p)
#define y2 (*y2p)
  outcode outcode1, outcode2;

  outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
  outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
  for (;;) {
    if (outcode1 == 0 && outcode2 == 0) {
      // Case 1: both endpoints are within the clipping region.
      return TRUE;
    } else if (outcode1 & outcode2) {
      // Case 2: both endpoints share an excluded region, impossible for a line between them to be within the clipping region.
      return FALSE;
    } else {
      int x, y;
      // Case 3: the endpoints are in different regions, and the segment is partially within
      // the clipping rectangle, selects one of the endpoints outside the clipping rectangle.
      int outcode_ex = outcode1 ? outcode1 : outcode2;
      // calculate the intersection of the line with the clipping rectangle using parametric line equations
      if (outcode_ex & TOP) {
        x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
        y = ymax;
      } else if (outcode_ex & BOTTOM) {
        x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
        y = ymin;
      } else if (outcode_ex & RIGHT) {
        y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
        x = xmax;
      } else {
        y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
        x = xmin;
      }

      // update the point after clipping and recalculate outcodes:

      // This handles the tricky case of these two situations, which both have the
      // same initial outcodes, but get handled properly when clipped with the
      // new points being where the X's are:
  
      //
      //         |            |
      //         |            |
      //         |            |
      // --------+------------+--------
      //         |            |
      //         |            |
      //         |            |   * x1,y1
      // --------+------------+--X-----
      //         |            | /
      //         |            |/
      //         |            X
      //         |           /|
      //         |    x2,y2 * |
      //
  
      // The diagram above has the same initial outcodes as the diagram below, but not when updated after computing the intersections...

      // This situation necessitates going round the loop two extra times.
      
      //         |            |
      //         |            |
      //         |            |
      // --------+------------+--------
      //         |            |   * x1,y1
      //         |            |  /
      //         |            | /
      //         |            |/
      //         |            X
      //         |           /|
      // --------+----------X-+--------
      //         |         /  |
      //         |  x2,y2 *   |
      //
  
      if (outcode_ex == outcode1) {
        x1 = x;
        y1 = y;
        outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
      } else {
        x2 = x;
        y2 = y;
        outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
      }
    }
  }
#undef x1
#undef y1
#undef x2
#undef y2
}


// https://stackoverflow.com/questions/47884592/how-to-reverse-cohen-sutherland-algorithm



// draw a line but ignore the menu hole-clipping.  This is used to draw within the
// menu area, if drawing is needed (currently only used for the frame) or by the
// hole-clipping draw code itself to draw allowed or partial vectors.

static void no_menu_line(int xl, int yb, int xr, int yt, int intensity, VectorPipelineBase *baseVector) {
  const int outer_window_xl = 0, outer_window_yb = 0, outer_window_xr = 255, outer_window_yt = 255; // hard-wired for this app for now.

// these are just a tweak for the pacman code.  Not relevant in the greater scheme of things...
#define MOVE_RIGHT 12
#define MOVE_DOWN 4

  if (retain_after_clipping (&xl, &yb, &xr, &yt, outer_window_xl, outer_window_yb, outer_window_xr, outer_window_yt)) {
    v_directDraw32((xl-128+MOVE_RIGHT)*128, -(yb-128+MOVE_DOWN)*128, (xr-128+MOVE_RIGHT)*128, -(yt-128+MOVE_DOWN)*128, intensity);
  }
}

static void menu_windowed_line (VectorPipelineBase *baseVector,
				int x1, int y1, int x2, int y2,
				int xmin, int ymin, int xmax, int ymax,
				int intensity) {   // used to blank a window inside the image, for drawing a pop-up over.

  // baseVector->force |= PL_BASE_FORCE_EMPTY;

  int accept;
  int done;
  outcode outcode1, outcode2;

  accept = FALSE;
  done = FALSE;
  outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
  outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
  do {
    if (outcode1 == 0 && outcode2 == 0) {
      done = TRUE;
    } else if (outcode1 & outcode2) {
      accept = TRUE;
      done = TRUE;
    } else {
      int x, y;
      int outcode_ex = outcode1 ? outcode1 : outcode2;

      if (outcode_ex & TOP) {
        x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
        y = ymax;
      } else if (outcode_ex & BOTTOM) {
        x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
        y = ymin;
      } else if (outcode_ex & RIGHT) {
        y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
        x = xmax;
      } else {
        y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
        x = xmin;
      }

      if (outcode_ex == outcode1) {
	// Need a callback to draw the two parts of a vector that crosses the whole window
        no_menu_line (x1, y1, x, y, intensity, baseVector);
        x1 = x;
        y1 = y;
        outcode1 = compute_outcode (x1, y1, xmin, ymin, xmax, ymax);
      } else {
        no_menu_line (x, y, x2, y2, intensity, baseVector);
        x2 = x;
        y2 = y;
        outcode2 = compute_outcode (x2, y2, xmin, ymin, xmax, ymax);
      }
    }
  } while (done == FALSE);
  if (accept == TRUE) {
    no_menu_line (x1, y1, x2, y2, intensity, baseVector);
  }
  return;
}

//-----------------------------------------------------------------------------

// first hack at a menu interface, may update later
// (eg if we add icons as well as text)
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
} menu_context;

extern int bufferType; // 0 = none, 1 = double buffer, 2 = auto buffer (if pipeline is empty -> use previous

//        !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /   
//  0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?   
//  @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O   
//  P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _   
//  `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o   
//  p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~      
unsigned char lcrasterline1[] = {
  0x00, 0x18, 0x6c, 0x6c, 0x30, 0x00, 0x38, 0x60, 0x18, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
  0x7c, 0x30, 0x78, 0x78, 0x1c, 0xfc, 0x38, 0xfc, 0x78, 0x78, 0x00, 0x00, 0x18, 0x00, 0x60, 0x78, 
  0x7c, 0x30, 0xfc, 0x3c, 0xf8, 0xfe, 0xfe, 0x3c, 0xcc, 0x78, 0x1e, 0xe6, 0xf0, 0xc6, 0xc6, 0x38, 
  0xfc, 0x78, 0xfc, 0x78, 0xfc, 0xcc, 0xcc, 0xc6, 0xc6, 0xcc, 0xfe, 0x78, 0xc0, 0x78, 0x10, 0x00, 
  0x30, 0x00, 0xe0, 0x00, 0x1c, 0x00, 0x38, 0x00, 0xe0, 0x30, 0x0c, 0xe0, 0x70, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x18, 0xe0, 0x76, 0x00, 
};
unsigned char lcrasterline2[] = {
  0x00, 0x3c, 0x6c, 0x6c, 0x7c, 0xc6, 0x6c, 0x60, 0x30, 0x30, 0x66, 0x30, 0x00, 0x00, 0x00, 0x0c, 
  0xc6, 0x70, 0xcc, 0xcc, 0x3c, 0xc0, 0x60, 0xcc, 0xcc, 0xcc, 0x30, 0x30, 0x30, 0x00, 0x30, 0xcc, 
  0xc6, 0x78, 0x66, 0x66, 0x6c, 0x62, 0x62, 0x66, 0xcc, 0x30, 0x0c, 0x66, 0x60, 0xee, 0xe6, 0x6c, 
  0x66, 0xcc, 0x66, 0xcc, 0xb4, 0xcc, 0xcc, 0xc6, 0xc6, 0xcc, 0xc6, 0x60, 0x60, 0x18, 0x38, 0x00, 
  0x30, 0x78, 0x60, 0x78, 0x0c, 0x78, 0x60, 0x76, 0x6c, 0x00, 0x00, 0x66, 0x30, 0xcc, 0xf8, 0x78, 
  0xdc, 0x76, 0xdc, 0x7c, 0x7c, 0xcc, 0xcc, 0xc6, 0xc6, 0xcc, 0xfc, 0x30, 0x18, 0x30, 0xdc, 0x00, 
};
unsigned char lcrasterline3[] = {
  0x00, 0x3c, 0x00, 0xfe, 0xc0, 0xcc, 0x38, 0xc0, 0x60, 0x18, 0x3c, 0x30, 0x00, 0x00, 0x00, 0x18, 
  0xce, 0x30, 0x0c, 0x0c, 0x6c, 0xf8, 0xc0, 0x0c, 0xcc, 0xcc, 0x30, 0x30, 0x60, 0xfc, 0x18, 0x0c, 
  0xde, 0xcc, 0x66, 0xc0, 0x66, 0x68, 0x68, 0xc0, 0xcc, 0x30, 0x0c, 0x6c, 0x60, 0xfe, 0xf6, 0xc6, 
  0x66, 0xcc, 0x66, 0xe0, 0x30, 0xcc, 0xcc, 0xc6, 0x6c, 0xcc, 0x8c, 0x60, 0x30, 0x18, 0x6c, 0x00, 
  0x18, 0x0c, 0x7c, 0xcc, 0x7c, 0xcc, 0xf0, 0xcc, 0x76, 0x70, 0x0c, 0x6c, 0x30, 0xfe, 0xcc, 0xcc, 
  0x66, 0xcc, 0x76, 0xc0, 0x30, 0xcc, 0xcc, 0xd6, 0x6c, 0xcc, 0x98, 0x30, 0x18, 0x30, 0x00, 0x00, 
};
unsigned char lcrasterline4[] = {
  0x00, 0x18, 0x00, 0x6c, 0x78, 0x18, 0x76, 0x00, 0x60, 0x18, 0xfe, 0xfc, 0x00, 0xfc, 0x00, 0x30, 
  0xde, 0x30, 0x38, 0x38, 0xcc, 0x0c, 0xf8, 0x18, 0x78, 0x7c, 0x00, 0x00, 0xc0, 0x00, 0x0c, 0x18, 
  0xde, 0xcc, 0x7c, 0xc0, 0x66, 0x78, 0x78, 0xce, 0xfc, 0x30, 0x0c, 0x78, 0x60, 0xfe, 0xde, 0xc6, 
  0x7c, 0xcc, 0x7c, 0x70, 0x30, 0xcc, 0xcc, 0xd6, 0x38, 0x78, 0x18, 0x60, 0x18, 0x18, 0xc6, 0x00, 
  0x00, 0x7c, 0x66, 0xc0, 0xcc, 0xfc, 0x60, 0xcc, 0x66, 0x30, 0x0c, 0x78, 0x30, 0xfe, 0xcc, 0xcc, 
  0x66, 0xcc, 0x66, 0x78, 0x30, 0xcc, 0xcc, 0xfe, 0x38, 0xcc, 0x30, 0xe0, 0x00, 0x1c, 0x00, 0x00, 
};
unsigned char lcrasterline5[] = {
  0x00, 0x18, 0x00, 0xfe, 0x0c, 0x30, 0xdc, 0x00, 0x60, 0x18, 0x3c, 0x30, 0x00, 0x00, 0x00, 0x60, 
  0xf6, 0x30, 0x60, 0x0c, 0xfe, 0x0c, 0xcc, 0x30, 0xcc, 0x0c, 0x00, 0x00, 0x60, 0x00, 0x18, 0x30, 
  0xde, 0xfc, 0x66, 0xc0, 0x66, 0x68, 0x68, 0xce, 0xcc, 0x30, 0xcc, 0x6c, 0x62, 0xd6, 0xce, 0xc6, 
  0x60, 0xdc, 0x6c, 0x1c, 0x30, 0xcc, 0xcc, 0xfe, 0x38, 0x30, 0x32, 0x60, 0x0c, 0x18, 0x00, 0x00, 
  0x00, 0xcc, 0x66, 0xcc, 0xcc, 0xc0, 0x60, 0x7c, 0x66, 0x30, 0xcc, 0x6c, 0x30, 0xd6, 0xcc, 0xcc, 
  0x7c, 0x7c, 0x60, 0x0c, 0x34, 0xcc, 0x78, 0xfe, 0x6c, 0x7c, 0x64, 0x30, 0x18, 0x30, 0x00, 0x00, 
};
unsigned char lcrasterline6[] = {
  0x00, 0x00, 0x00, 0x6c, 0xf8, 0x66, 0xcc, 0x00, 0x30, 0x30, 0x66, 0x30, 0x30, 0x00, 0x30, 0xc0, 
  0xe6, 0x30, 0xcc, 0xcc, 0x0c, 0xcc, 0xcc, 0x30, 0xcc, 0x18, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x00, 
  0xc0, 0xcc, 0x66, 0x66, 0x6c, 0x62, 0x60, 0x66, 0xcc, 0x30, 0xcc, 0x66, 0x66, 0xc6, 0xc6, 0x6c, 
  0x60, 0x78, 0x66, 0xcc, 0x30, 0xcc, 0x78, 0xee, 0x6c, 0x30, 0x66, 0x60, 0x06, 0x18, 0x00, 0x00, 
  0x00, 0x76, 0xdc, 0x78, 0x76, 0x78, 0xf0, 0x0c, 0xe6, 0x78, 0xcc, 0xe6, 0x78, 0xc6, 0xcc, 0x78, 
  0x60, 0x0c, 0xf0, 0xf8, 0x18, 0x76, 0x30, 0x6c, 0xc6, 0x0c, 0xfc, 0x30, 0x18, 0x30, 0x00, 0x00, 
};
unsigned char lcrasterline7[] = {
  0x00, 0x18, 0x00, 0x6c, 0x30, 0xc6, 0x76, 0x00, 0x18, 0x60, 0x00, 0x00, 0x70, 0x00, 0x30, 0x80, 
  0x7c, 0xfc, 0xfc, 0x78, 0x1e, 0x78, 0x78, 0x30, 0x78, 0x70, 0x30, 0x70, 0x18, 0x00, 0x60, 0x30, 
  0x78, 0xcc, 0xfc, 0x3c, 0xf8, 0xfe, 0xf0, 0x3e, 0xcc, 0x78, 0x78, 0xe6, 0xfe, 0xc6, 0xc6, 0x38, 
  0xf0, 0x1c, 0xe6, 0x78, 0x78, 0xfc, 0x30, 0xc6, 0xc6, 0x78, 0xfe, 0x78, 0x02, 0x78, 0x00, 0xfe, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xf0, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x1c, 0x18, 0xe0, 0x00, 0x00, 
};

unsigned char *lcrasterlines[7] = {
  lcrasterline1, lcrasterline2, lcrasterline3, lcrasterline4, lcrasterline5, lcrasterline6, lcrasterline7
};

// This is only for testing...
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termio.h>

#define KEY_ESCAPE 27
#define KEY_UP     257
#define KEY_DOWN   258
#define KEY_LEFT   259
#define KEY_RIGHT  260

int kbhit(void)
{
  char buff[2];
  struct termios original;
  struct termios term;
  int characters_buffered = 0;

  tcgetattr(STDIN_FILENO, &original);
  memcpy(&term, &original, sizeof(term)); // term = original;
  term.c_lflag &= ~ICANON;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
  ioctl(STDIN_FILENO, FIONREAD, &characters_buffered);
  tcsetattr(STDIN_FILENO, TCSANOW, &original);

  if (characters_buffered-- > 0) {
    int rc = read(STDIN_FILENO, buff, 1);
    if ((characters_buffered > 1) && (buff[0] == KEY_ESCAPE)) {
      char escbuff[2];
      rc = read(STDIN_FILENO, escbuff, 2);
      if (escbuff[0] == '[') {
	if (escbuff[1] == 'A') return KEY_UP;
	if (escbuff[1] == 'B') return KEY_DOWN;
	if (escbuff[1] == 'C') return KEY_RIGHT;
	if (escbuff[1] == 'D') return KEY_LEFT;
      }
    }
    return buff[0];
  }
  
  return -1;
}

void echoOff(void)
{
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);

  term.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void echoOn(void)
{
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);

  term.c_lflag |= ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void CreateMenu(menu_context *m, char *title, char *options) {
  char *s1, *s2;
  char s[128];
  int i, w;
      m->active = FALSE;
      m->selected = 1;   // was originally 0 for unselected but this is simpler
      strcpy(m->selected_str, "");
      m->xl = 48;        // early draft hardwires menu size
      m->yb = 64;
      m->xr = 176;
      m->yt = 192;       // Drawing within the menu is done using virtual coordinates same as the application
      m->lines = 0; m->displayable_lines = 0; m->scroll_base = 0;
      m->display_line = 0;
      // up/down/left/right actions are implemented by the DrawMenu code, but the user can supply a procedure to augment the action
      m->up = NULL;
      m->down = NULL;
      m->left = NULL;
      m->right = NULL;
      // The user has to supply actions for each button. We only handle menu navigation.
      m->b1 = NULL;
      m->b2 = NULL;
      m->b3 = NULL;
      m->b4 = NULL;
      m->title = title; // Centred. Later, may embellish strings with left/right/center justification codes?
      m->options = options; // 1..n (0 is reserved for now. May use it for exiting menu with no choice.)

  // move this chunk to CreateMenu and save in variables instead?
  // Doing it here simplifies having the user change the menu options on the fly...
  strcpy(s, m->options); // make writable
  s1 = s;
  m->lines = 0;
  while ((s2 = strchr(s1, '\n')) != NULL) {
    *s2++ = '\0';
    w = strlen(s1); if (w > m->width) m->width = w;
    if (w <= 22) strcpy(&m->buff[m->lines][0], (w <= 22 ? s1 : "CODER ERROR"));
    s1 = s2;
    m->lines += 1;
  }
  w = strlen(s1); if (w > m->width) m->width = w;
  if (w <= 22) strcpy(&m->buff[m->lines][0], (w <= 22 ? s1 : "CODER ERROR"));
  // we now know lines and width.

  for (i = 0; i <= m->lines; i++) {
    w = strlen(&m->buff[i][0]);
    m->buff[i][w++] = ' '; m->buff[i][w] = '\0'; // extra space on all strings avoids issues with raster printing
  }
  m->lines += 1;

}

int DrawMenu(menu_context *m) {
  char s[128], title[128];
  int c, i, x, y, txty, tab;
  
  m->active = TRUE;
  v_setBrightness(120);

  // We expect programmers to ensure menu items fit in 11 chars, but if they don't we have a short-term
  // fallback to allow up to 22 chars.  However if a programmer sees the small text menu occurring, they
  // should probably think about restructuring the code or menu data so that it isn't necessary.
  // (Note that a lot of raster text on screen will cause flicker, so the current default is chosen
  // to be minimally disruptive)
  
  // Note the reason for these shenannigans boils down to the fact that raster text uses a different
  // coordinate system from graphic line drawing.  Ultimately that should be corrected which would
  // make scaling and positioning text much easier.  We could do that now with vector text, but it
  // is far more expensive than raster text.

  // these two branches should be parameterised and merged...
  if (m->width > 11) {
    // small text
    txty = 54+16; // 12 lines 22 chars
    m->displayable_lines = 16;
    // menu border.
    no_menu_line (m->xl, m->yb, m->xl, m->yt, 120, NULL);
    no_menu_line (m->xl, m->yt, m->xr, m->yt, 120, NULL);
    if (m->title) {
      // title separator - allow two lines for the smaller text version
      tab = (22-strlen(m->title))/2; if (tab < 0) tab = 0;
      m->displayable_lines -= 2;
      for (i = 0; i < tab; i++) {
	title[i] = ' ';
      }
      strcpy(title+i, m->title); strcat(title, " ");
      no_menu_line (m->xl, m->yb+16+1, m->xr, m->yb+16+1, 120, NULL);
      v_printStringRaster(-59-4, txty, title, 35, -3, '\0'); // later: side-scroll using buff[i][offset] ...
      txty -= 16; // skip 2 lines
    }
    no_menu_line (m->xr, m->yt, m->xr, m->yb, 120, NULL);
    no_menu_line (m->xr, m->yb, m->xl, m->yb, 120, NULL);
    for (i = m->scroll_base; i < m->lines; i++) {
      strcpy(s, (&m->buff[i][0]));
      if (i+1 == m->selected) {
	strcpy(m->selected_str, &m->buff[i][0]); // the selected option will always be displayed so safe to get here
	m->selected_str[strlen(m->selected_str)-1] = '\0';
	strcat(s, "< ");
      }
      v_printStringRaster(-59, txty, s, 35, -3, '\0'); // later: side-scroll using buff[i][offset] ...
      txty -= 8;
      if (i-m->scroll_base+1 == m->displayable_lines) break;
    }  
  } else {
    // larger (default size) text
    txty = 50+16; // 7 lines 11 chars
    m->displayable_lines = 8;
    no_menu_line (m->xl, m->yb, m->xl, m->yt, 120, NULL);
    no_menu_line (m->xl, m->yt, m->xr, m->yt, 120, NULL);
    if (m->title) {
      // title separator - allow two lines for the smaller text version
      tab = (11-strlen(m->title))/2; if (tab < 0) tab = 0;
      m->displayable_lines -= 1;
      for (i = 0; i < tab; i++) {
	title[i] = ' ';
      }
      strcpy(title+i, m->title); strcat(title, " ");
      no_menu_line (m->xl, m->yb+16+4, m->xr, m->yb+16+4, 120, NULL); // bleh. we have our b/t's and +/-'s confused.
      v_printStringRaster(-59, txty+3, title, 70, -7, '\0'); // later: side-scroll using buff[i][offset] ...
      txty -= 16;
    }
    no_menu_line (m->xr, m->yt, m->xr, m->yb, 120, NULL);
    no_menu_line (m->xr, m->yb, m->xl, m->yb, 120, NULL);
    for (i = m->scroll_base; i < m->lines; i++) {
      strcpy(s, (&m->buff[i][0]));
      if (i+1 == m->selected) {
	strcpy(m->selected_str, &m->buff[i][0]); // the selected option will always be displayed so safe to get here
	m->selected_str[strlen(m->selected_str)-1] = '\0';
	strcat(s, "< ");
      }
      v_printStringRaster(-59, txty, s, 70, -7, '\0'); // later: side-scroll using buff[i][offset] ...
      txty -= 16;
      if (i-m->scroll_base+1 == m->displayable_lines) break;
    }  
  }
  // We do our action selection after drawing the menu.  If the action affects the menu display,
  // it will be updated on the next iteration.
  // During development we'll cheat and use Unix terminal I/O.
  c = kbhit();
  if (c < 0) return 0;
  switch (c) {
  case 'a':
  case 'A': // b1 (leftmost)
    if (m->b1) m->b1(m);
    return m->selected << 24;
  case 's':
  case 'S': // b2
    if (m->b2) m->b2(m);
    return m->selected << 16;
  case 'd':
  case 'D': // b3
    if (m->b3) m->b3(m);
    return m->selected << 8;
  case 13:
  case 10:
  case 'f':
  case 'F': // b4
    if (m->b4) m->b4(m);
    return m->selected;
  case '8': // up
  case KEY_UP:
    if (m->up) m->up(m);

    if (m->display_line == 0) {
      if (m->selected == 1) return 0;
      m->scroll_base -= 1; // scroll the list downwards when you hit the top of the display (not of the options)
    } else {
      m->display_line -= 1; // move cursor up a line, don't scroll options
    }
    m->selected -= 1;
    
    return 0;
  case '2': // down
  case KEY_DOWN:
    if (m->down) m->down(m);

    if (m->selected >= m->lines) return 0;
    if (m->display_line+1 >= m->displayable_lines) {
      m->scroll_base += 1; // scroll the list upwards when you hit the bottom of the display (not of the options)
    } else {
      m->display_line += 1; // move cursor down a line, don't scroll options
    }
    m->selected += 1;

    return 0;
  case '4': // left - back to previous menu level?
  case KEY_LEFT:
    if (m->left) m->left(m);
    return 0;
  case '6': // right - down to next menu level?
  case KEY_RIGHT:
    if (m->right) m->right(m);
    return 0;
    
  default: return 0;
  }
}

// This is a test program to draw something (doesn't matter what), and then add
// a pop-up text menu on top of it.

void m_line(int xl, int yb, int xr, int yt, menu_context *menu) {
  if (menu && menu->active) {
    menu_windowed_line (NULL, xl, yb, xr, yt,
  			      menu->xl, menu->yb, menu->xr, menu->yt,
			48); // dim the background when we draw a menu...
  } else {
    no_menu_line (xl, yb, xr, yt, 120, NULL);
  }
}

void m_directDraw32(int xl, int yb, int xr, int yt, int bri, menu_context *menu) {
  if (menu && menu->active) {
#ifdef NEVER
    w_directDraw32(NULL, xl, yb, xr, yt,
		   menu->xl, menu->yb, menu->xr, menu->yt, /* the hole */
			 bri/2); // dim the background when we draw a menu...
#endif
  } else {
    v_directDraw32(xl, yb, xr, yt, 120/*, NULL*/);
  }
}

void m_directDraw32Patterned(int xl, int yb, int xr, int yt, int pattern, int bri, menu_context *menu) {
  if (menu && menu->active) {
#ifdef NEVER
    w_directDraw32Patterned(NULL, xl, yb, xr, yt,
  			    menu->xl, menu->yb, menu->xr, menu->yt, /* the hole */
			    pattern, bri/2); // dim the background when we draw a menu...
#endif
  } else {
    v_directDraw32(xl, yb, xr, yt, 120/*, NULL*/);
  }
}

void CursorUp(menu_context *m) {
}

void CursorDown(menu_context *m) {
}

void Select(menu_context *m) { // B4 - perform action if 'selected' is an action. If it's a submenu enter it. (TO DO)
  m->active = FALSE; // must do this on any return from menu
  if (strcmp(m->selected_str, "Quit") == 0) {
    echoOn();
    exit(0); // haven't yet built the user side cleanly.
  }
}

void Cancel(menu_context *m) { // B3 - pop up if submenu, exit menu if top-level (TO DO)
  m->active = FALSE; // must do this on any return from menu
}

void Down(menu_context *m) { // B2 = make B2 behave like joystick down
  if (m->display_line+1 == m->displayable_lines) {
    if (m->selected == m->lines) return;
    m->scroll_base += 1; // scroll the list upwards when you hit the bottom of the display (not of the options)
  } else {
    m->display_line += 1; // move cursor down a line, don't scroll options
  }
  m->selected += 1;

  return;
}

void Up(menu_context *m) { // B1 = make B1 behave like joystick up
  if (m->display_line == 0) {
    if (m->selected == 1) return;
    m->scroll_base -= 1; // scroll the list downwards when you hit the top of the display (not of the options)
  } else {
    m->display_line -= 1; // move cursor up a line, don't scroll options
  }
  m->selected -= 1;
    
  return;
}

#ifdef DBMAIN
#include <errno.h>
int main(int argc, char **argv) {
  menu_context menu; // declared in the application. Not globally.
  int tick = 0;

  vectrexinit(1);
  v_setName("popup");
  v_init();
  v_set_font(lcrasterlines);
  optimizationON = 1;
  usePipeline = 1;
  bufferType = 2; // 0 = none, 1 = double buffer, 2 = auto buffer (if pipeline is empty -> use previous
  v_setRefresh(50);

  fprintf(stderr, "Under development.  Currently uses keyboard inputs, not controller.\n");

  // Current bug: "Action" is displayed in first row of menu, not in header field.
  // (it was working before, I must have broken something.)
  CreateMenu(&menu, "Action", "Setup Net\nCalibrate\nIntro\nMulticart\nArcade\nVectrex\nBasic\nBash\nQuit");
  //CreateMenu(&menu, NULL, "Setup Net\nCalibrate\nIntro\nMulticart\nArcade\nVectrex\nBasic\nBash\nQuit");
  //CreateMenu(&menu, "Action", "Setup Net\nCalibrate\nIntro\nMulticart\nArcade Games\nVectrex\nBasic\nBash\nQuit\n10\n11\n12\nhidden\nmore\ndone\nno\nnot\nreally");
  //CreateMenu(&menu, NULL, "Setup Net\nCalibrate\nIntro\nMulticart\nArcade Games\nVectrex\nBasic\nBash\nQuit\n10\n11\n12\nhidden\nmore\ndone\nno\nnot\nreally");
  menu.b1 = &Up;     // bind user procedures to buttons.
  menu.b2 = &Down;
  menu.b3 = &Cancel;
  menu.b4 = &Select;

  echoOff();
  menu.active = FALSE; // menus don't have to be created just before use. They can
  for (;;) {
    startFrame();

    // PacAnimate(); // restore these once bitmaps working again
    // Ghosts();
    // Pills();

    if (menu.active /* menu is already up */ || (kbhit() > 0) /* invoke menu */) {
      int choice = DrawMenu(&menu); // both creates and displays.  Later should separate those out into two procs.
      (void)choice; // programmer could use this return instead of a callback proc...
    }

    if (tick++ == 2000) { // temporary failsafe to exit if I mess up with menu actions
      echoOn();
      exit(0);
    }

    drawMaze(&menu);
  }
  echoOn();
  exit(0);
  return 0;
}
#endif
