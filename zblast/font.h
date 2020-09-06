/* zblast & xzb - simple shoot-em-up.
 * Copyright (C) 1993-1997,2000 Russell Marks. See zblast.c for license.
 *
 * font.h - font stuff, borrowed from zgv.
 */
 
#define NO_CLIP_FONT  0x7FFFFFFF

extern int vgadrawtext(int x,int y,int siz,char *str);
extern int vgatextsize(int sizearg,char *str);
extern void set_max_text_width(int width);
