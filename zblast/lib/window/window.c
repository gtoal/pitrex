#include <stdio.h>
#include <string.h> // for strrchr
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include "window.h"
extern void setCustomClipping(int enabled, int x0, int y0, int x1, int y1); // should be in vectrexInterface.h

#ifndef TRUE
#define TRUE (0==0)
#define FALSE (!TRUE)
#endif

#define CLIP TRUE
#define NO_CLIP FALSE

typedef int bool;

#define VECTREX_DEFAULT 0
// rotate 90 degrees anticlockwise for each step:
#define VECTREX_VERTICAL 1
#define VECTREX_HORIZONTAL 2
#define VECTREX_INVERTED 3
#define VECTREX_HORIZONTAL_INVERTED 4

int v_intensity = 60;

static int v_hardware_orientation = VECTREX_VERTICAL;

static int64_t v_ScaleXMul=1LL, v_ScaleXDiv=1LL, v_ScaleXOffsetPre=0LL, v_ScaleXOffsetPost=0LL,
               v_ScaleYMul=1LL, v_ScaleYDiv=1LL, v_ScaleYOffsetPre=0LL, v_ScaleYOffsetPost=0LL;

static int v_swap_xy = FALSE;
static int v_flip_x = FALSE;
static int v_flip_y = FALSE;
static int v_xl=0, v_yb=0, v_xr=0, v_yt=0; // or -20000,-20000 to 20000,20000 ?? or -18000,-24000 to 18000,24000 ???

static int tx(int x) { // transform x from window to viewport
  if (v_flip_x) x = v_xr-(x-v_xl); 
  return (int)(((((int64_t)x)+v_ScaleXOffsetPre)*v_ScaleXMul)/v_ScaleXDiv + v_ScaleXOffsetPost);
}

static int ty(int y) { // and transform y
  if (v_flip_y) y = v_yt-(y-v_yb); 
  return (int)(((((int64_t)y)+v_ScaleYOffsetPre)*v_ScaleYMul)/v_ScaleYDiv + v_ScaleYOffsetPost);
}

void v_set_hardware_orientation(int orientation) { // called by low-level to tell us what user has set orientation to in main menu
  v_hardware_orientation = orientation;
  if (orientation == VECTREX_VERTICAL) {
    v_flip_x = FALSE; v_flip_y = FALSE; v_swap_xy = FALSE;
  } else if (orientation == VECTREX_HORIZONTAL) {
    v_flip_x = TRUE; v_flip_y = TRUE; v_swap_xy = TRUE;
  } else if (orientation == VECTREX_INVERTED) {
    v_flip_x = TRUE; v_flip_y = TRUE; v_swap_xy = FALSE;
  } else if (orientation == VECTREX_HORIZONTAL_INVERTED) {
    v_flip_x = FALSE; v_flip_y = FALSE; v_swap_xy = TRUE;
  }
}

void v_brightness(int intensity) { // until integrated in a library
  if (intensity < 0) intensity = 0;
  if (intensity > 127) intensity = 127;
  v_setBrightness(v_intensity = intensity);
}

void v_line(int xl, int yb, int xr, int yt) {
  static int oxl = 0, oyb = 0, oxr = 0, oyt = 0;
  if (v_swap_xy) { int tmp; tmp = xl; xl = yb; yb = tmp; tmp = xr; xr = yt; yt = tmp; xl = v_xr-(xl-v_xl); xr = v_xr-(xr-v_xl); }
  if (xl != oxl && yb != oyb && xr != oxl && yt != oyb &&
      xl != oxr && yb != oyt && xr != oxr && yt != oyt) {
    // doesn't join with prev vector
    v_directDraw32Hinted(tx(xl),ty(yb), tx(xr),ty(yt), v_intensity, PL_BASE_FORCE_ZERO | PL_BASE_FORCE_RESET_ZERO_REF);
  } else {
    v_directDraw32(tx(xl),ty(yb), tx(xr),ty(yt), v_intensity);
  }
  oxl = xl; oyb = yb;
  oxr = xr; oyt = yt; 
}

void v_window(int xl, int yb, int xr, int yt,
	      bool clip_to_window) {
  // We will use normalised viewport coordinates of x: -18000 : 18000 and y: -24000 : 24000
  int64_t width, height;
  int xc, yc;
  int oxl = xl, oyb = yb, oxr = xr, oyt = yt;
  v_xl=xl; v_yb=yb; v_xr=xr; v_yt=yt;
  width = (int64_t)xr-(int64_t)xl;
  height = (int64_t)yt-(int64_t)yb;
  if (width*4 > height*3) {
    // window is wider than aspect ratio, so we will have black bars at the top and bottom
    height = (width * 4) / 3;
    yc = (yb+yt)/2;
    yb = yc - height/2;
    yt = yc + height/2;
  } else if (width*4 < height*3) {
    // window is taller than aspect ratio, so we will have black bars at the sides
    width = (height*3) / 4;
    xc = (xl+xr)/2;
    xl = xc - width/2;
    xr = xc + width/2;
  }
  v_ScaleXMul = 36000LL; v_ScaleXDiv = width; v_ScaleXOffsetPre = -width/2LL; v_ScaleXOffsetPost = 0LL; v_ScaleXOffsetPost = (tx(xr) - tx(oxr)) / 2LL;
  v_ScaleYMul = 48000LL; v_ScaleYDiv = height; v_ScaleYOffsetPre = -height/2LL; v_ScaleYOffsetPost = 0LL; v_ScaleYOffsetPost = (ty(yt) - ty(oyt)) / 2LL;
  // transform world (window) coordinates to viewport (normalised device coordinates) before
  // clipping.  That way clipping code does not need to know about world coordinates.
  // The offset above was supposed to center the window when rotated 90 or 270 but doesn't seem to be working.
  // - possible that I have x and y swapped?
  if (clip_to_window) {
    if (v_swap_xy) {
      setCustomClipping(TRUE, ty(oyb), tx(oxl), ty(oyt), tx(oxr));
    } else {
      setCustomClipping(TRUE, tx(oxl), ty(oyb), tx(oxr), ty(oyt));
    }
  }
}

void v_clip(int xl, int yb, int xr, int yt) { // set explicit clipping window using world coordinates
  if (v_swap_xy) {
    setCustomClipping(TRUE, ty(yb), tx(xl), ty(yt), tx(xr));
  } else {
    setCustomClipping(TRUE, tx(xl), ty(yb), tx(xr), ty(yt));
  }
}
