#include <stdio.h>

/*
 * no_interface.c: null display for Atari Vector game simulator
 *
 * Copyright 1993, 1996 Eric Smith
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: no_interface.c,v 1.1 2018/07/31 01:19:45 pi Exp $
 */


#include "display.h"


int smallwindow;
int window_width, window_height;


void init_graphics (/*int argc, char *argv[],*/ int p_smallwindow, 
		    int p_use_pixmap, int p_line_width, char *window_name)
{
}


void term_graphics (void)
{
}


void handle_input (void)
{
}

int xl=9999, yb=9999, xr=-9999, yt=-9999;
void cx(int x) {
  if (x < xl) xl = x;
  if (x > xr) xr = x;
}
void cy(int y) {
  if (y > yt) yt = y;
  if (y < yb) yb = y;
}
void draw_line (int x1, int y1, int x2, int y2, int c, int z)
{
  //cx(x1);cx(x2);cy(y1);cy(y2);
  //fprintf(stderr, "(%d,%d) -> (%d,%d)\n", xl,yb, xr,yt);

}


void open_page (int step)
{
}


void close_page (void)
{
}


int repeat_frame (void)
{
  return (0);
}


void bell (void)
{
}
