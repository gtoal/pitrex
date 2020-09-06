#ifndef _VMMSTDDEF_H_
#define _VMMSTDDEF_H_

/*****************************************************************************
* Vector Mame Menu
*
* Author:  Chad Gray
* Created: 20/06/11
*
* Standard definitions
*
*****************************************************************************/

#ifndef _VCHARS_H_
   #include "vchars.h"
#endif

#define FRAMES_PER_SEC 60        // number of frames drawn in a second
#define EDGE_BRI       25        // Bright intensity
#define EDGE_NRM       15        // Normal intensity
#define EDGE_DIM       5         // Dim intensity
#define BORD           5         // space between edge border frames
#define NUM_ASTEROIDS  20        // number of asteroids on screensaver and exit screen
#define NUM_STARS      40        // number of stars

#define X_MIN          (-512)
#define X_MAX          511
#define Y_MIN          (-384)
#define Y_MAX          383

#define DEBUG 0                  // set to 1 to enable debug output

/*** default key values ***/
#if defined(linux) || defined(__linux) || (_WIN32) || defined (__WIN32__)
   #include "SDL.h"
   //SDL key codes:
   #define GRAVE       SDL_SCANCODE_GRAVE   // Settings (use service switch)
   #define UP          SDL_SCANCODE_UP      // Up
   #define DOWN        SDL_SCANCODE_DOWN    // Down
   #define LEFT        SDL_SCANCODE_LEFT    // (Rotate) Left
   #define RIGHT       SDL_SCANCODE_RIGHT   // (Rotate) Right
   #define ESC         SDL_SCANCODE_ESCAPE  // Esc (1P Start + 2P Start on IPAC)
   #define FIRE        SDL_SCANCODE_LCTRL   // Left CTRL key
   #define THRUST      SDL_SCANCODE_LALT    // Left ALT key
   #define RSHIFT      SDL_SCANCODE_RSHIFT  // Right Shift key
   #define LSHIFT      SDL_SCANCODE_LSHIFT  // Left Shift key
   #define HYPSPACE    SDL_SCANCODE_SPACE   // Space bar
   #define CREDIT      SDL_SCANCODE_5       // 5 key
   #define START1      SDL_SCANCODE_1       // 1 key
   #define START2      SDL_SCANCODE_2       // 2 key
#else // DOS key values
   #define GRAVE       0x2960               // Settings
   #define UP          0x4800               // Up
   #define DOWN        0x5000               // Down
   #define LEFT        0x4b00               // (Rotate) Left
   #define RIGHT       0x4d00               // (Rotate) Right
   #define ESC         0x011b               // Esc (1P Start + 2P Start on IPAC)
   #define FIRE        0x04                 // Left CTRL key
   #define THRUST      0x08                 // Left ALT key
   #define RSHIFT      0x01                 // Right Shift key
   #define LSHIFT      0x02                 // Left Shift key
   #define HYPSPACE    0x3920               // Space bar
   #define CREDIT      0x0635               // 5 key
   #define START1      0x0231               // 1 key
   #define START2      0x0332               // 2 key
#endif

/**vector object settings **/
#define NewDir()       (((int)(rand()/(RAND_MAX/101)) < 50) ? -1 : 1)     // random -1 or +1
#define NewXPos()      (int)((rand()/(RAND_MAX/(X_MAX-X_MIN)+1.0))-X_MAX) // random between X_MIN and X_MAX
#define NewYPos()      (int)((rand()/(RAND_MAX/(Y_MAX-Y_MIN)+1.0))-Y_MAX) // random between Y_MIN and Y_MAX
#define NewScale()     (int)((rand()/(RAND_MAX/3))+1)*2                   // 2, 4, or 6
#define NewXYInc()     (int)((rand()/(RAND_MAX/10))+1)                    // random between 1 and 10
#define NewTheta()     (int) (rand()/(RAND_MAX/5))                        // random between 0 and 4
#define NewAst()       (int) (rand()/(RAND_MAX/4))                        // 0, 1, 2 or 3
#define NewAstColour() (int) (rand()/(RAND_MAX/7))                        // 0 to 6
#define NewStarSpeed() (int)((rand()/(RAND_MAX/5))+1)                     // 1 to 5

// Colours used in the menu
#define vred           0
#define vmagenta       1
#define vcyan          2
#define vblue          3
#define vyellow        4
#define vgreen         5
#define vwhite         6

//LED codes
#define                S_LED 1      // Scroll lock LED
#define                N_LED 2      // Number lock LED
#define                C_LED 4      // Caps lock LED

// Index used to reference colour/intensity array
#define c_col          0
#define c_int          1
#define c_glist        0
#define c_sgame        1
#define c_sman         2
#define c_man          3
#define c_pnman        4
#define c_arrow        5
#define c_asts         6

// Index of key codes
#define k_menu         0
#define k_options      1
#define k_pman         2
#define k_nman         3
#define k_pgame        4
#define k_ngame        5
#define k_pclone       6
#define k_nclone       7
#define k_start        8
#define k_quit         9
#define k_random       10

// Index of user options
enum options {
   o_rot,
   o_stars,
   o_ucase,
   o_togpnm,
   o_cpanel,
   o_redozvg,
   o_mouse,
   o_msens,
   o_mrevX,
   o_mrevY,
   o_mpoint,
   o_dovga,
   o_attmode,
   o_fontsize,
   o_borders,
   o_volume
};

typedef struct {
   float x, y;
} point;

/*******************************************************
Define a vector object, having:
   A vector shape                        (outline)
   Position                              (pos.x, pos.y)
   Velocity                              (inc.x, inc.y)
   Scale                                 (scale.x, scale.y)
   An angle and rotation factor          (angle, theta)
   A point around which rotation occurs  (cent.x, cent.y)
   Brightness / colour                   (colour, bright)
   Edge behaviour (wrap or bounce)       (edge)
*******************************************************/
typedef struct {
   vShape   outline;
   point    pos;
   point    inc;
   point    scale;
   int      angle, theta;
   point    cent;
   int      colour;
   int      bright;
   int      edge;
} vObject;

/******************************************************
Define a vStar, having:
   position    pos.x, pos.y
   direction   delta.x, delta.y
   change      change.x, change.y
   speed       1-5
*******************************************************/
typedef struct {
   point    pos;
   point    delta;
   point    change;
   float    speed;
} vStar;

#endif

