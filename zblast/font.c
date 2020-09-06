/* zblast & xzb - simple shoot-em-up.
 * Copyright (C) 1993-1997,2000 Russell Marks. See zblast.c for license.
 *
 * font.c - font stuff, borrowed from zgv.
 */


/* all the line-based font stuff is hard-coded and a bit ugly, so you
 * probably won't want much to look at that (hint) :-)
 */

 
#include <stdio.h>
#include <string.h>
#ifdef USE_X
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#else
#include <svgalib-vectrex/svgalib-vectrex.h>
#endif
#include "font.h"

#define TABSIZE  64   /* size of a tab, in pixels */

#ifdef USE_X
extern Display *disp;
extern Pixmap *pixmap;
extern GC gc;

#define vga_drawpixel(xx,yy) \
  XDrawPoint(disp,*pixmap,gc,xx,yy)
#define vga_drawline(aa,bb,cc,dd) \
  XDrawLine(disp,*pixmap,gc,aa,bb,cc,dd)
#else
extern int current_colour;

#define vga_drawpixel(x,y) gl_setpixel(x,y,current_colour)
#define vga_drawline(a,b,c,d) gl_line(a,b,c,d,current_colour)
#endif

static int go_through_the_motions=0;
  /* if 1, we don't draw it, just do the width */

static int stop_after_this_x=NO_CLIP_FONT;


/* prototypes */
int vgadrawtext(int x, int y, int siz, char *str);
void fontcircle(int x, int y, int r);
void fontc_l(int x, int y, int r);
void fontc_u(int x, int y, int r);
void fontc_r(int x, int y, int r);
void fontc_left(int x, int y, int r);
void fontc_ul(int x, int y, int r);
void fontc_ur(int x, int y, int r);
void fontc_ll(int x, int y, int r);
void fontc_lr(int x, int y, int r);
int vgatextsize(int sizearg, char *str);
void set_max_text_width(int width);


/* returns width of text drawn in pixels */
int vgadrawtext(int x,int y,int siz,char *str)
{
int f,a,b,c,s1,s2,s3,s4,s5,s6,gap;

b=y;
a=x;
s1=siz; s2=s1<<1; s3=s1+s2; s4=s2<<1; s5=s4+s1; s6=s3<<1;

for(f=0;f<strlen(str);f++)
  {
   gap=s1; if(gap==0) gap=1;
  /* s3+s4 is the size that an ellipsis will take up (s3), plus the
   * widest possible letter (M = s4).
   */
  if(a-x>stop_after_this_x-s3-s4)
    {
    int tmp;
    
    /* print an ellipsis... well, three dots :-) */
    /* blast the width restriction to stop possible infinite recursion */
    tmp=stop_after_this_x;
    set_max_text_width(NO_CLIP_FONT);
    a+=vgadrawtext(a,y,siz,"...");
    stop_after_this_x=tmp;
    /* now give up */
    break;
    }
  c=str[f];
  
  /*** 1st step: cover some common occurances ***/
  if(!go_through_the_motions)   /* only draw it if we really want to */
    {
    if(strchr("abdgopq68",c)!=NULL)   /* common circle position */
      fontcircle(a+s1,b+s3,siz);
    else
      {    /* common part-circle positions */
      if(strchr("cehmnrs",c)!=NULL)
        fontc_ul(a+s1,b+s3,siz);
      if(strchr("ehmnrBS35",c)!=NULL)
        fontc_ur(a+s1,b+s3,siz);
      if(strchr("cetuyCGJOQSUl035",c)!=NULL)
        fontc_ll(a+s1,b+s3,siz);
      if(strchr("suyBCDGJOQSU035",c)!=NULL)
        fontc_lr(a+s1,b+s3,siz);
      /* common line */
      if(strchr("BDEFHKLMNPR",c)!=NULL)
        vga_drawline(a,b,a,b+s4);
      }
    }

  /*** 2nd step: fill in rest - this is the *really* long, messy bit :-) ***/
  
  /*** 2a: lowercase letters ***/
  if(!go_through_the_motions)
    switch(c)
      {
      case 'a':
        vga_drawline(a+s2,b+s2,a+s2,b+s4); break;
      case 'b':
        vga_drawline(a,b,a,b+s4); break;
      case 'c':
        vga_drawline(a+s1,b+s2,a+s2,b+s2);
        vga_drawline(a+s1,b+s4,a+s2,b+s4); break;
      case 'd':
        vga_drawline(a+s2,b,a+s2,b+s4); break;
      case 'e':
        vga_drawline(a,b+s3,a+s2,b+s3);
        vga_drawline(a+s1,b+s4,a+s2,b+s4); break;
      case 'f':
        fontc_ul(a+s1,b+s1,siz); vga_drawline(a,b+s1,a,b+s4);
        vga_drawline(a,b+s2,a+s1,b+s2); break;
      case 'g':
        vga_drawline(a+s2,b+s2,a+s2,b+s5);
        fontc_l(a+s1,b+s5,siz); break;
      case 'h':
        vga_drawline(a,b,a,b+s4); vga_drawline(a+s2,b+s3,a+s2,b+s4); break;
      case 'i':
        vga_drawpixel(a,b+s1);
        vga_drawline(a,b+s2,a,b+s4);
        a+=-s1+1; break;
      case 'j':
        vga_drawline(a+s1,b+s2,a+s1,b+s5);
        fontc_lr(a,b+s5,siz);
        vga_drawpixel(a+s1,b+s1); break;
      case 'k':
        vga_drawline(a,b,a,b+s4); vga_drawline(a,b+s3,a+s1,b+s2);
        vga_drawline(a,b+s3,a+s1,b+s4); break;
      case 'l':
        vga_drawline(a,b,a,b+s3); break;
      case 'm':
        vga_drawline(a,b+s2,a,b+s4); vga_drawline(a+s2,b+s3,a+s2,b+s4);
        vga_drawline(a+s4,b+s3,a+s4,b+s4); fontc_u(a+s3,b+s3,siz);
        break;
      case 'n':
        vga_drawline(a,b+s2,a,b+s4); vga_drawline(a+s2,b+s3,a+s2,b+s4);
        break;
      case 'p':
        vga_drawline(a,b+s2,a,b+s6); break;
      case 'q':
        vga_drawline(a+s2,b+s2,a+s2,b+s6); break;
      case 'r':
        vga_drawline(a,b+s2,a,b+s4); break;
      case 's':
        vga_drawline(a,b+s3,a+s2,b+s3); vga_drawline(a+s1,b+s2,a+s2,b+s2);
        vga_drawline(a,b+s4,a+s1,b+s4);
        break;
      case 't':
        vga_drawline(a,b+s1,a,b+s3); vga_drawline(a,b+s2,a+s1,b+s2);
        break;
      case 'u':
        vga_drawline(a,b+s2,a,b+s3); vga_drawline(a+s2,b+s2,a+s2,b+s4);
        break;
      case 'v':
        vga_drawline(a,b+s2,a+s1,b+s4); vga_drawline(a+s1,b+s4,a+s2,b+s2);
        break;
      case 'w':
        vga_drawline(a,b+s2,a+s1,b+s4); vga_drawline(a+s1,b+s4,a+s2,b+s3);
        vga_drawline(a+s2,b+s3,a+s3,b+s4); vga_drawline(a+s3,b+s4,a+s4,b+s2);
        break;
      case 'x':
        vga_drawline(a,b+s2,a+s2,b+s4); vga_drawline(a,b+s4,a+s2,b+s2);
        break;
      case 'y':
        vga_drawline(a,b+s2,a,b+s3);
        vga_drawline(a+s2,b+s2,a+s2,b+s5);
        fontc_l(a+s1,b+s5,siz); break;
      case 'z':
        vga_drawline(a,b+s2,a+s2,b+s2); vga_drawline(a+s2,b+s2,a,b+s4);
        vga_drawline(a,b+s4,a+s2,b+s4); break;
      
      /*** 2b: uppercase letters ***/
  
      case 'A':
        vga_drawline(a,b+s4,a+s1,b); vga_drawline(a+s1,b,a+s2,b+s4);
        vga_drawline(a+(s1>>1),b+s2,a+s2-(s1>>1),b+s2); break;
      case 'B':
        fontc_r(a+s1,b+s1,siz);
        vga_drawline(a,b,a+s1,b); vga_drawline(a,b+s2,a+s1,b+s2);
        vga_drawline(a,b+s4,a+s1,b+s4); break;
      case 'C':
        fontc_u(a+s1,b+s1,siz); vga_drawline(a,b+s1,a,b+s3);
        break;
      case 'D':
        vga_drawline(a,b,a+s1,b);
        vga_drawline(a,b+s4,a+s1,b+s4); fontc_ur(a+s1,b+s1,siz);
        vga_drawline(a+s2,b+s1,a+s2,b+s3); break;
      case 'E':
        vga_drawline(a,b,a+s2,b); vga_drawline(a,b+s2,a+s1,b+s2);
        vga_drawline(a,b+s4,a+s2,b+s4); break;
      case 'F':
        vga_drawline(a,b,a+s2,b); vga_drawline(a,b+s2,a+s1,b+s2);
        break;
      case 'G':
        fontc_u(a+s1,b+s1,siz); vga_drawline(a,b+s1,a,b+s3);
        vga_drawline(a+s1,b+s2,a+s2,b+s2); vga_drawline(a+s2,b+s2,a+s2,b+s3);
        break;
      case 'H':
        vga_drawline(a,b+s2,a+s2,b+s2); vga_drawline(a+s2,b,a+s2,b+s4);
        break;
      case 'I':
        vga_drawline(a,b,a+s2,b); vga_drawline(a+s1,b,a+s1,b+s4);
        vga_drawline(a,b+s4,a+s2,b+s4); break;
      case 'J':
        vga_drawline(a+s2,b,a+s2,b+s3); break;
      case 'K':
        vga_drawline(a+s2,b,a,b+s2); vga_drawline(a,b+s2,a+s2,b+s4); break;
      case 'L':
        vga_drawline(a,b+s4,a+s2,b+s4); break;
      case 'M':
        vga_drawline(a,b,a+s1+(s1>>1),b+s2);
        vga_drawline(a+s1+(s1>>1),b+s2,a+s3,b);
        vga_drawline(a+s3,b,a+s3,b+s4); a-=s1; break;
      case 'N':
        vga_drawline(a,b,a+s2,b+s4); vga_drawline(a+s2,b+s4,a+s2,b); break;
      case 'Q':
        vga_drawline(a+s1,b+s3,a+s2,b+s4);
        /* FALLS THROUGH and adds an O, finishing the Q */ 
      case '0': 
	/* Hack to make score a little more readable on Vectrex --SVGAmod */
	gap += 3;
      case 'O':   /* all other numbers done later */
        fontc_u(a+s1,b+s1,siz); vga_drawline(a,b+s1,a,b+s3);
        vga_drawline(a+s2,b+s1,a+s2,b+s3); break;
      case 'R':
        vga_drawline(a+s1,b+s2,a+s2,b+s4);
        /* FALLS THROUGH and adds a P, finishing the R */
      case 'P':
        fontc_r(a+s1,b+s1,siz); vga_drawline(a,b,a+s1,b);
        vga_drawline(a,b+s2,a+s1,b+s2); break;
      case 'S':
        fontc_left(a+s1,b+s1,siz); fontc_ur(a+s1,b+s1,siz); break;
      case 'T':
        vga_drawline(a,b,a+s2,b); vga_drawline(a+s1,b,a+s1,b+s4); break;
      case 'U':
        vga_drawline(a,b,a,b+s3); vga_drawline(a+s2,b,a+s2,b+s3); break;
      case 'V':
        vga_drawline(a,b,a+s1,b+s4); vga_drawline(a+s1,b+s4,a+s2,b); break;
      case 'W':
        vga_drawline(a,b,a+s1,b+s4); vga_drawline(a+s1,b+s4,a+s2,b+s2);
        vga_drawline(a+s2,b+s2,a+s3,b+s4); vga_drawline(a+s3,b+s4,a+s4,b);
        break;
      case 'X':
        vga_drawline(a,b,a+s2,b+s4); vga_drawline(a+s2,b,a,b+s4); break;
      case 'Y':
        vga_drawline(a,b,a+s1,b+s2); vga_drawline(a+s2,b,a+s1,b+s2);
        vga_drawline(a+s1,b+s2,a+s1,b+s4); break;
      case 'Z':
        vga_drawline(a,b,a+s2,b); vga_drawline(a+s2,b,a,b+s4);
        vga_drawline(a,b+s4,a+s2,b+s4); break;
      
      /*** 2c: numbers ***/
    
      /* 0 already done */
      case '1':
        vga_drawline(a,b+s1,a+s1,b); vga_drawline(a+s1,b,a+s1,b+s4);
        vga_drawline(a,b+s4,a+s2,b+s4);
	/* Hack to make score a little more readable on Vectrex --SVGAmod */
	gap += 3; break;
      case '2':
        fontc_u(a+s1,b+s1,siz); vga_drawline(a+s2,b+s1,a,b+s4);
        vga_drawline(a,b+s4,a+s2,b+s4);
	gap += 3; break;
      case '3':
        fontc_u(a+s1,b+s1,siz); fontc_lr(a+s1,b+s1,siz);
	gap += 3; break;
      case '4':
        vga_drawline(a+s1,b+s4,a+s1,b); vga_drawline(a+s1,b,a,b+s2);
        vga_drawline(a,b+s2,a+s2,b+s2);
	gap += 3; break;
      case '5':
        vga_drawline(a+s2,b,a,b); vga_drawline(a,b,a,b+s2);
        vga_drawline(a,b+s2,a+s1,b+s2);
	gap += 3; break;
      case '6':
        fontc_u(a+s1,b+s1,siz); vga_drawline(a,b+s1,a,b+s3);
	gap += 3; break;
      case '7':
        vga_drawline(a,b,a+s2,b); vga_drawline(a+s2,b,a+s1,b+s4);
	gap += 3; break;
      case '9':
        vga_drawline(a+s2,b,a+s2,b+s4);
        /* FALLS THROUGH and does top circle of 8 to complete the 9 */
      case '8':
        fontcircle(a+s1,b+s1,siz);
	gap += 3; break;
      
      /* 2d: some punctuation (not much!) */
      
      case '-':
        vga_drawline(a,b+s2,a+s1,b+s2); break;
      case '.':
        vga_drawpixel(a,b+s4); a+=-s1+1; break;
      case '(':
        fontc_ul(a+s1,b+s1,siz); fontc_ll(a+s1,b+s3,siz);
        vga_drawline(a,b+s1,a,b+s3); break;
      case ')':
        fontc_ur(a,b+s1,siz); fontc_lr(a,b+s3,siz);
        vga_drawline(a+s1,b+s1,a+s1,b+s3); break;
      case '%':
        vga_drawpixel(a,b);
        vga_drawpixel(a+s2,b+s4);
        /* FALLS THROUGH drawing the slash to finish the % */
      case '/':
        vga_drawline(a,b+s4,a+s2,b); break;
      case '?':
        fontc_u(a+s1,b+s1,siz);
        vga_drawline(a+s2,b+s1,a+s1,b+s2);
        vga_drawline(a+s1,b+s2,a+s1,b+s3);
        vga_drawpixel(a+s1,b+s4);
        break;
      }

    
  /*** 3rd part: finally, move along for the next letter ***/
  /* we do this even if go_through_the_motions is set */
  if(strchr("ltfijk-. ()",c)!=NULL)
    a+=s1;
  else
    {
    if(strchr("?/%abcdeghnopqrsuvxyz"
              "ABCDEFGHIJKLNOPQRSTUVXYZ0123456789",c)!=NULL)
      a+=s2;
    else
      {
      if(strchr("mwMW",c)!=NULL)
        a+=s4;
      else
        {
        if(c==9)
          {
          /* congratulations madam, it's a tab */
          a=((a/TABSIZE)+1)*TABSIZE;
          }
        else
          {
          /* oh, don't know this one. do an underscore */
          /* (we don't if go_through_the_motions is set) */
          if(!go_through_the_motions)
            vga_drawline(a,b+s4,a+s2,b+s4);
          a+=s2;
          }
        }
      }
    }
  a+=gap;   /* and add a gap */
  }
return(a-x);
}


/* never mind. Character building, wasn't it? (groan) */

void fontcircle(int x,int y,int r)
{
fontc_u(x,y,r);
fontc_l(x,y,r);
}

void fontc_l(int x,int y,int r)
{
fontc_ll(x,y,r);
fontc_lr(x,y,r);
}

void fontc_u(int x,int y,int r)
{
fontc_ul(x,y,r);
fontc_ur(x,y,r);
}

void fontc_r(int x,int y,int r)
{
fontc_ur(x,y,r);
fontc_lr(x,y,r);
}

void fontc_left(int x,int y,int r)
{
fontc_ul(x,y,r);
fontc_ll(x,y,r);
}

void fontc_ul(int x,int y,int r)
{
int r34;

if(go_through_the_motions) return;
r34=((r*3)>>2);
vga_drawline(x-r,y,x-r34,y-r34);
vga_drawline(x-r34,y-r34,x,y-r);
}

void fontc_ur(int x,int y,int r)
{
int r34;

if(go_through_the_motions) return;
r34=((r*3)>>2);
vga_drawline(x+r,y,x+r34,y-r34);
vga_drawline(x+r34,y-r34,x,y-r);
}

void fontc_ll(int x,int y,int r)
{
int r34;

if(go_through_the_motions) return;
r34=((r*3)>>2);
vga_drawline(x-r,y,x-r34,y+r34);
vga_drawline(x-r34,y+r34,x,y+r);
}

void fontc_lr(int x,int y,int r)
{
int r34;

if(go_through_the_motions) return;
r34=((r*3)>>2);
vga_drawline(x+r,y,x+r34,y+r34);
vga_drawline(x+r34,y+r34,x,y+r);
}


/* this gets how wide text will be */
int vgatextsize(int sizearg,char *str)
{
int r;

go_through_the_motions=1;
r=vgadrawtext(0,0,sizearg,str);
go_through_the_motions=0;
return(r);
}


void set_max_text_width(int width)
{
stop_after_this_x=width;
}
