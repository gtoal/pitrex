/*
 * display.h: Atari DVG and AVG simulators
 *
 * Copyright 1991, 1992, 1996 Eric Smith
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
 * $Id: display.h 2 2003-08-20 01:51:05Z eric $
 */

/*
 * These three variables should be initialized depending on the
 * game.
 */
extern int dvg;  /* set true for Atari Digital Vector Generator */
extern int portrait;  /* set for vertical display orientation */
extern int drop_frames;  /* if non-0, show only one frame in drop_frames */
extern unsigned vector_mem_offset;
extern char* ProgName;

extern unsigned long vg_done_cyc; /* cycle after which VG will be done */

#ifdef VG_DEBUG
extern int trace_vgo;
extern int vg_step; /* single step the vector generator */
extern int vg_print;
extern unsigned long last_vgo_cyc;
extern unsigned long vgo_count;
#endif /* VG_DEBUG */

int vg_done (unsigned long cyc);
void vg_go (unsigned long cyc);
void vg_reset (unsigned long cyc);

void avg_init(unsigned short int vram, unsigned short int cram);

/*
 * Device dependent display functions
 */

void init_graphics (int smallwindow, int use_pixmap, int line_width, char *window_name);

void term_graphics ();

void draw_line (int x1, int y1, int x2, int y2, int c, int z);

void open_page (int step);

void close_page (void);

int repeat_frame (void);
/*
 * Called if the vector generator is told to go but the vector list is
 * identical to the previous one.  If repeat_frame() returns true, the
 * normal drawing occurs (open_page, draw_line, close_page).  Most
 * implementations should just return false.
 */

void handle_input (void);

/* the following function is a temporary hack and will probably go away! */
void bell (void);
