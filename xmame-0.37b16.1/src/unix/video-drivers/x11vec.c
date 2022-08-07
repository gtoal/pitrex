/*****************************************************************

  X11 vector routines

  For PiTrex. Based on fxvec.c, glvec.c, and vidhrdw/vector.c
  (and yet I'm still confused). Scaling also inspired by mame.c.
  
  Rewritten in format of Glide vector driver (fxvec.c) based on
  XMAME 0.106 version.

*****************************************************************/

#ifdef x11

#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include "xmame.h"
#include "driver.h"
#include "vidhrdw/vector.h"
#include "x11.h"

unsigned char *vectorram;
size_t vectorram_size;
int translucency;

static double vecwidthdiv, vecheightdiv;
static int vector_orientation,vecshift,old_intensity;
static float flicker_correction = 0.0;
static float intensity_correction = 1.0;
static char started = 0;
static XSegment lines[MAX_POINTS/2];
static XSegment *linePt = lines;

/*
 * Initializes vector game video emulation
 */

int vector_vh_start (void)
{
  linePt = lines;
  started = 0;

  return 0;
}

/*
 * Stop the vector video hardware emulation. Free memory.
 */

void vector_vh_stop (void)
{
}

/*
 * Setup scaling. Currently the Sega games are stuck at VECSHIFT 15
 * and the the AVG games at VECSHIFT 16
 */

void vector_set_shift (int shift)
{
  vecshift=shift;
}

/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */

void vector_add_point(int x, int y, int color, int intensity)
{
 if(linePt - lines == MAX_POINTS/2)
   printf("Vector buffer overflow\n");
 else
 {
  int tmp;
  started = 1;

/*  printf ("x = %i, y = %i -- ",x>>vecshift,y>>vecshift); */
  x = (int)((double)(x>>vecshift)/vecwidthdiv);
  y = (int)((double)(y>>vecshift)/vecheightdiv);
/*  printf ("Scale = %f - x = %i, y = %i\n",scale,x,y); */
  if(Machine->orientation&ORIENTATION_SWAP_XY)
  {
    tmp=x;
    x=y;
    y=tmp;
  }

  if(Machine->orientation&ORIENTATION_FLIP_X)
    x=1-x;

  if(Machine->orientation&ORIENTATION_FLIP_Y)
    y=1-y;

  if (linePt == lines)
  {
    linePt->x1 = 0;
    linePt->y1 = 0;
    linePt->x2 = x;
    linePt->y2 = y;
    linePt++;
    old_intensity = 0;
   }
   else
   {  /* Omit lines with same start and end position at this resolution */
      if ( !((linePt - 1)->x2 == x && (linePt - 1)->y2 == y) ) 
      {
        /* Draw buffered lines in 'lines' array whenever intensity changes */
	if (intensity != old_intensity)
	{
	  if (old_intensity)
	  {
	    XSetForeground (display, window_gc, intensity_table[old_intensity]);
	    XDrawSegments (display, window, window_gc, lines, linePt - lines);
	  }
	  old_intensity = intensity;
	  /* Next starting point is the end of the last line */
	  lines[0].x1 = (linePt - 1)->x2;
	  lines[0].y1 = (linePt - 1)->y2;
	  /* Reset buffer */
	  linePt = lines;
	}
	else
	{
	  /* Next starting point is the end of the last line */
	  linePt->x1 = (linePt - 1)->x2;
	  linePt->y1 = (linePt - 1)->y2;
	}
	linePt->x2 = x;
	linePt->y2 = y;
/*	printf ("Drawing x: %i y: %i | x2: %i, y2: %i\n", linePt->x1, linePt->y1, x, y); */
	linePt++;
      }
   }
 }
}

/*
 * Add new clipping info to the list
 */

void vector_add_clip (int x1, int yy1, int x2, int y2)
{
}

/*
 * The vector CPU creates a new display list.
 */

void vector_clear_list(void)
{
  linePt = lines;
/*  printf ("cleared vector list\n"); */
}

/* Called when the frame is complete */

void vector_vh_update(struct osd_bitmap *bitmap)
{
/*
  printf ("mach_vis_width = %i, mach_vis_height = %i\n", Machine->visible_area.max_x -
	Machine->visible_area.min_x, Machine->visible_area.max_y -
	Machine->visible_area.min_y);
  printf ("drv_screen_width = %i, drv_screen_height = %i\n",
        Machine->drv->screen_width, Machine->drv->screen_height);
  printf ("bitmap_width = %i, bitmap_height = %i\n", bitmap->width, bitmap->height);
*/
  if(Machine->orientation & ORIENTATION_SWAP_XY)
  {
    vecwidthdiv = (double)(Machine->visible_area.max_y -
	Machine->visible_area.min_y) / (double)bitmap->height;

    vecheightdiv = (double)(Machine->visible_area.max_x -
	Machine->visible_area.min_x) / (double)bitmap->width;
  }
  else
  {
    vecwidthdiv = (double)(Machine->visible_area.max_x -
	Machine->visible_area.min_x) / (double)bitmap->width;

    vecheightdiv = (double)(Machine->visible_area.max_y -
	Machine->visible_area.min_y) / (double)bitmap->height;
  }

  if (linePt - lines && old_intensity) /* Draw leftovers */
  {
    XSetForeground (display, window_gc, intensity_table[old_intensity]);
    XDrawSegments (display, window, window_gc, lines, linePt - lines);
    XClearWindow (display, window);
  }  /* Don't draw blank frames */
  else if (started)
    XClearWindow (display, window);

  started = 0;
}

void vector_set_flip_x (int flip)
{
	if (flip)
		vector_orientation |= ORIENTATION_FLIP_X;
	else
		vector_orientation &= ~ORIENTATION_FLIP_X;
}

void vector_set_flip_y (int flip)
{
	if (flip)
		vector_orientation |= ORIENTATION_FLIP_Y;
	else
		vector_orientation &= ~ORIENTATION_FLIP_Y;
}

void vector_set_swap_xy (int swap)
{
	if (swap)
		vector_orientation |= ORIENTATION_SWAP_XY;
	else
		vector_orientation &= ~ORIENTATION_SWAP_XY;
}

void vector_set_flicker(float _flicker)
{
	flicker_correction = _flicker;
	/* flicker = (int)(flicker_correction * 2.55); */
}

float vector_get_flicker(void)
{
	return flicker_correction;
}

/* from vectorgen.c */

void vector_set_gamma(float gamma)
{
	sysdep_palette_set_gamma(normal_palette, gamma);
}

float vector_get_gamma(void)
{
	return sysdep_palette_get_gamma(normal_palette);
}

void vector_set_intensity(float _intensity)
{
	intensity_correction = _intensity;
}

float vector_get_intensity(void)
{
	return intensity_correction;
}

void vector_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	vector_vh_update(bitmap);
}

#endif /* ifdef x11 */
