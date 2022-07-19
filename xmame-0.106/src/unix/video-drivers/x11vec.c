/*****************************************************************

  X11 vector routines

  For PiTrex. Based on fxvec.c, glvec.c, and vidhrdw/vector.c
  (and yet I'm still confused).

*****************************************************************/

#include <string.h>
#include <X11/Xlib.h>
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

/* from mame's vidhrdw/vector.h */
#define VCLEAN  0
#define VDIRTY  1
#define VCLIP   2
#define MAX_POINTS 10000	/* Maximum # of points we can queue in a vector list */

#define XSCALE 6.1
#define YSCALE 3.4
#define SEGMENT_BUFFER

int x11vec_renderer(point *pt, int num_points)
{
  int orig_width,orig_height,vecx=0,vecy=0;
  double vecscalex,vecscaley;
#ifdef SEGMENT_BUFFER
  int old_intensity;
  XSegment lines[MAX_POINTS/2];
  XSegment *linePt = lines;
#else
  int x1,x2,y1,y2,oldx=0,oldy=0;
#endif
  /* Frame sync for PiTrex driver */
  /* XFillRectangle(display, window, XDefaultGC(display,0), 0, 0, 101, 101); */
  XClearWindow (display, window);

  if (num_points)
  {
    /* Scaling set-up (copied from fxvec.c and glgen.c) */
    /* Scale variables go back to visible_area members of the _running_machine
     * structure defined in mame.h:120. The min/max variables are of type int
     * (from a rectangle structure, mamecore.h:96), but I couldn't find any
     * word about how they should be scaled to the screen area/resolution. They
     * seem to be consistent between games though, so there's probably a system
     * to it.
     */
    if(sysdep_display_params.vec_dest_bounds)
    {
     if (sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY)
     {
      orig_width  = sysdep_display_params.height;
      orig_height = sysdep_display_params.width;
     }
     else
     {
      orig_width  = sysdep_display_params.width;
      orig_height = sysdep_display_params.height;
     }
       vecx = sysdep_display_params.vec_dest_bounds->min_x/
         orig_width;
       vecy = sysdep_display_params.vec_dest_bounds->min_y/
         orig_height;
/*
       vecscalex = (65536.0 * orig_width *
         (sysdep_display_params.vec_src_bounds->max_x-
          sysdep_display_params.vec_src_bounds->min_x)) /
         ((sysdep_display_params.vec_dest_bounds->max_x + 1) -
          sysdep_display_params.vec_dest_bounds->min_x);
       vecscaley = (65536.0 * orig_height *
         (sysdep_display_params.vec_src_bounds->max_y-
          sysdep_display_params.vec_src_bounds->min_y)) /
          ((sysdep_display_params.vec_dest_bounds->max_y + 1) -
          sysdep_display_params.vec_dest_bounds->min_y);
*/
/*       printf ("dest bounds\norig_width=%i, orig_height=%i\n", orig_width, orig_height); */
       vecscalex = ((sysdep_display_params.vec_src_bounds->max_x + 1) -
        sysdep_display_params.vec_src_bounds->min_x) * (orig_width/XSCALE);
       vecscaley = ((sysdep_display_params.vec_src_bounds->max_y + 1) -
        sysdep_display_params.vec_src_bounds->min_y) * (orig_height/YSCALE);
    }
    else
    {
     vecscalex = ((sysdep_display_params.vec_src_bounds->max_x + 1) -
      sysdep_display_params.vec_src_bounds->min_x) * (640/XSCALE);// * 65536.0;
     vecscaley = ((sysdep_display_params.vec_src_bounds->max_y + 1) -
      sysdep_display_params.vec_src_bounds->min_y) * (480/YSCALE);// * 65536.0;
    }
/*
     printf ("min_x=%i max_x=%i, min_y=%i max_y=%i -- scale x: %f, y: %f\n",
	     sysdep_display_params.vec_src_bounds->min_x,
	     sysdep_display_params.vec_src_bounds->max_x,
	     sysdep_display_params.vec_src_bounds->min_y,
	     sysdep_display_params.vec_src_bounds->max_y,
	     vecscalex, vecscaley);
*/


#ifdef SEGMENT_BUFFER
    /* first point */
    while (pt->status == VCLIP)
    {
       pt++;
       num_points--;
    }
    linePt->x1 = 0;
    linePt->y1 = 0;
    linePt->x2 = vecx + pt->x/vecscalex;
    linePt->y2 = vecy + pt->y/vecscaley;
    linePt++;
    pt++;
    num_points--;
    old_intensity = 0;

    /* draw remaining points */
    while(num_points)
    {
      if (pt->status == VCLIP)
      {
        /* Ignoring clipping for now

        xmin = vecx + pt->x/vecscalex;
        ymin = vecy + pt->y/vecscaley;
        xmax = vecx + pt->arg1/vecscalex;
        ymax = vecy + pt->arg2/vecscaley;
        */
      }
      else
      {
        /* Draw buffered lines in "lines" array whenever intensity changes */
	if (pt->intensity != old_intensity)
	{
	  if (old_intensity)
	  {
	    XSetForeground (display, window_gc, intensity_table[old_intensity]);
	    XDrawSegments (display, window, window_gc, lines, linePt - lines);
	  }
	  old_intensity = pt->intensity;
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
	linePt->x2 = vecx + pt->x/vecscalex;
	linePt->y2 = vecy + pt->y/vecscaley;
	linePt++;
      }
      pt++; 
      num_points--;
     }
     if (linePt - lines && old_intensity) /* Draw leftovers */
     {
       XSetForeground (display, window_gc, intensity_table[old_intensity]);
       XDrawSegments (display, window, window_gc, lines, linePt - lines);
     }


#else
    /* draw each point */
    while(num_points)
    {
      if (pt->status == VCLIP)
      {
        /* Ignoring clipping for now

        xmin = vecx + pt->x/vecscalex;
        ymin = vecy + pt->y/vecscaley;
        xmax = vecx + pt->arg1/vecscalex;
        ymax = vecy + pt->arg2/vecscaley;
        */
      }
      else
      { 
      	x1 = oldx;
        y1 = oldy;
        x2 = vecx + pt->x/vecscalex;
        y2 = vecy + pt->y/vecscaley;
        oldx = x2;
        oldy = y2;
	if (pt->intensity)
	{
          XSetForeground (display, window_gc, intensity_table[pt->intensity]);
	  XDrawLine (display, window, window_gc, x1, y1, x2, y2);
/*	  printf ("Drawing x: %i y: %i | x2: %i, y2: %i (from x: %i, y: %i)\n", x1, y1, x2, y2, pt->x, pt->y); */
	}
      }
      pt++; 
      num_points--;
     }
#endif


    }
  /* return 1 to render bitmap as well */
  return 0;
}
