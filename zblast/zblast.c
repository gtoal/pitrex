/* zblast - simple shoot-em-up.
 * Copyright (C) 1993-2000 Russell Marks.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#ifdef linux
#include <sys/soundcard.h>
#endif

#ifdef USE_X
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "xzb.icon"
#else
#include "svgalib-vectrex/svgalib-vectrex.h"
#include "svgalib-vectrex/vectrextokeyboard.h"
#include "pitrex/pitrexio-gpio.h"
#include "vectrex/vectrexInterface.h"
#endif

#include "font.h"
#include "levels.h"
#include "anim3d.h"
#include "hiscore.h"

#ifdef MUSIC_SUPPORT
#include "sod2.h"
extern int play_done,realstereo,preconstruct;
int samp_ret[2],samp_idx;
#endif

#ifdef JOYSTICK_SUPPORT
#define BUTTON_FIRE	0
#define BUTTON_PDC	1

#include "joy.h"
#endif

int soundfd=-1,freq=16000;
int do_retrace=0;
int parentpid;
int sndpipe[2];
int music=1,stereo=1;
int sound_bufsiz;
int have_joy=0;
int autofire=0;

int got_sigquit=0;

int fullquit=0;

#ifndef USE_X
GraphicsContext scrn;
int current_colour=15;
int scrnmode;
#endif

#define DELAY_AUTO_FIRE		5

#define INIT_FRAMEDELAY		14285
#define FRAME_DECR		8000

/* use 256 byte frags */
#define BASE_SOUND_FRAG_PWR	8
#define BASE_SOUND_BUFSIZ	(1<<BASE_SOUND_FRAG_PWR)


/* virtual sound channels, used for mixing.
 * each can play at most one sample at a time.
 */
#define PSHOT_CHANNEL	0	/* player shot */
#define SMALLHIT_CHANNEL 1	/* small baddie being hit */
#define BIGHIT_CHANNEL	2	/* big baddie being hit */
#define BSHOT_CHANNEL	3	/* baddie shot channel */
#define EFFECT_CHANNEL  4	/* effects like level end noises */

#define NUM_CHANNELS	5

/* other signals to sound player */
#define TURN_SOUND_ON	128
#define TURN_SOUND_OFF	129

#define MUSIC_TITLE	130
#define MUSIC_GAME	131
#define MUSIC_GM_OVER	132

#define KILL_SNDSERV	255	/* signal to sndserv to die */


struct channel_tag
  {
  struct sample_tag *sample;	/* pointer to sample struct, NULL if none */
  int offset;			/* position in sample */
  } channel[NUM_CHANNELS];


/* sample filenames */
/* type 2 is shot and is omitted */
char samplename[][16]=
  {
  "pshot.raw",
  "type1hit.raw",
  "type3hit.raw",
  "type4hit.raw",
  "type5hit.raw",
  "type6hit.raw",
  "type10hit.raw",
  "type11hit.raw",
  "type1dead.raw",
  "type3dead.raw",
  "type4dead.raw",
  "type5dead.raw",
  "type6dead.raw",
  "type10dead.raw",
  "type11dead.raw",
  "bshot.raw",
  "phit.raw",
  "pwave.raw",
  "lvlend.raw"
  };

/* sample offsets in sample[] */
#define PSHOT_SAMPLE	0	/* player shooting */
#define TYPE1HIT_SAMPLE	1	/* noises each type of baddie makes */
#define TYPE3HIT_SAMPLE	2	/* ...when hit */
#define TYPE4HIT_SAMPLE	3
#define TYPE5HIT_SAMPLE	4
#define TYPE6HIT_SAMPLE	5
#define TYPE10HIT_SAMPLE 6
#define TYPE11HIT_SAMPLE 7
#define TYPE1DEAD_SAMPLE 8	/* noises each type of baddie makes */
#define TYPE3DEAD_SAMPLE 9	/* ...when killed */
#define TYPE4DEAD_SAMPLE 10
#define TYPE5DEAD_SAMPLE 11
#define TYPE6DEAD_SAMPLE 12
#define TYPE10DEAD_SAMPLE 13
#define TYPE11DEAD_SAMPLE 14
#define BSHOT_SAMPLE	15 /* baddie shooting */
#define PHIT_SAMPLE	16	/* player hit */
#define PWAVE_SAMPLE	17	/* player wave shot (PDC) */
#define LVLEND_SAMPLE   18
#define NUM_SAMPLES	19

/* for in-memory samples */
struct sample_tag
  {
  unsigned char *data;		/* pointer to sample, NULL if none */
  int length;			/* length of sample */
  } sample[NUM_SAMPLES];


#define GRIDDEPTH  100
#define SHOTNUM     64
#define BADDIENUM   40
#define STARNUM     10
#define SHIELDLIFE  30
#define NORMDEBRISNUM 64	/* in-game */
#define MAXDEBRISNUM 256	/* during "game over" bit */


/* array is used like an array of [GRIDDEPTH][-32 to 32] :
 * that is, we add 32 to x index when referencing it.
 */
struct grid
  {
  int x,y;
  } gridxy[GRIDDEPTH][65];


struct
  {
  int x,y;   /* -32<=x<=32, 0<=y<=GRIDDEPTH */
  char active;
  } shot[SHOTNUM];

struct
  {
  int x,y;
  int dx,dy;
  char active;	/* 0=not active, 1=`normal', 2=flashout effect */
  char bounce;
  char line;
  int col;	/* used for flashout - decrements to 0 */
  char decr;	/* used for flashout - decrement to use (must hit 0) */
  } debris[MAXDEBRISNUM];

int debrisnum=NORMDEBRISNUM;

struct
  {
  int x,y;
  int col;   /* colour */
  char speed;
  } star[STARNUM];

struct
  {
  char which;   /* which type of baddie (0 for inactive) */
  int x,y;
  int dx,dy;
  int hits;    /* hits left till death */
  char hitnow;  /* have been hit recently */
  } baddie[BADDIENUM];

#define sgnval(a) ((a)>0)?1:(((a)<0)?-1:0)

int movx=0,movy=0,gtmpx,gtmpy,textx=0,texty=0;

#define lowrand()	(rand()&32767)

#ifdef USE_X
#define vga_setcolor(xx) \
  {if(usemono) \
    XSetForeground(disp,gc,(xx)?whitepix:blackpix); \
  else \
    XSetForeground(disp,gc,colourtable[xx]);}
#define vga_drawpixel(xx,yy) \
  XDrawPoint(disp,*pixmap,gc,xx,yy)
#define vga_drawline(aa,bb,cc,dd) \
  XDrawLine(disp,*pixmap,gc,aa,bb,cc,dd)
#define gridpset(ix,iy) vga_drawpixel(gridxy[iy][ix].x,gridxy[iy][ix].y)
#define gridmoveto(ix,iy) movx=gridxy[iy][ix].x,movy=gridxy[iy][ix].y
#define gridlineto(ix,iy) vga_drawline(movx,movy,\
			    gtmpx=gridxy[iy][ix].x,gtmpy=gridxy[iy][ix].y), \
                            movx=gtmpx,movy=gtmpy
#else /* !USE_X */
#define gridpset(ix,iy) gl_setpixel(gridxy[iy][ix].x,gridxy[iy][ix].y)
#define gridmoveto(ix,iy) movx=gridxy[iy][ix].x,movy=gridxy[iy][ix].y
#define gridlineto(ix,iy) gl_line(movx,movy,\
			    gtmpx=gridxy[iy][ix].x,gtmpy=gridxy[iy][ix].y,\
                            current_colour),\
                            movx=gtmpx,movy=gtmpy
#define vga_setcolor(x) current_colour=(x)
#define vga_drawpixel(x,y) gl_setpixel(x,y,current_colour)
#define vga_drawline(a,b,c,d) gl_line(a,b,c,d,current_colour)
#endif /* !USE_X */

#define drawshot(nval,clr) \
  { \
  vga_setcolor(clr); \
  gridmoveto(shot[nval].x+32,shot[nval].y); \
  gridlineto(shot[nval].x+32,shot[nval].y+1); \
  }

int lives,lostlife,pdc,sound=1;
int gameover_count;
int score;
int tweenwave,paused;
int usedebris=1,usestars=1;
int count,donetrace,endtrace;


#ifdef USE_X
Display *disp;
int screen,depth;
Window window;
GC gc;
Colormap cmap;
Pixmap pixmap1,pixmap2,*pixmap;
Pixmap icon_pixmap;
unsigned long blackpix,whitepix;
int usemono=1;
int wait_for_expose=1;

/* needed to catch wm delete-window message */
static Atom proto_atom=None,delete_atom=None;

unsigned long colourtable[16];
#endif /* USE_X */

unsigned short colourrgb[16][3]=
  {
  {0x0000, 0x0000, 0x0000},
  {0x0000, 0x0000, 0xbfff},
  {0x0000, 0xbfff, 0x0000},
  {0x0000, 0xbfff, 0xbfff},
  {0xbfff, 0x0000, 0x0000},
  {0xbfff, 0x0000, 0xbfff},
  {0xbfff, 0xbfff, 0x0000},
  {0xbfff, 0xbfff, 0xbfff},
  {0x0000, 0x0000, 0x0000},
  {0x0000, 0x0000, 0xffff},
  {0x0000, 0xffff, 0x0000},
  {0x0000, 0xffff, 0xffff},
  {0xffff, 0x0000, 0x0000},
  {0xffff, 0x0000, 0xffff},
  {0xffff, 0xffff, 0x0000},
  {0xffff, 0xffff, 0xffff}
  };

int keypress_up,keypress_down,keypress_left,keypress_right;
int keypress_fire,keypress_pdc,keypress_sound,keypress_pause,keypress_quit;

#ifdef USE_X
int pwidth=320,pheight=240;
#else
int pwidth=320,pheight=200;
#endif

/* Doublebuf now default because it works better for SVGAlib-Vectrex */
int bigwindow=1,doublebuf=1;
int framedelay;
unsigned int frames;
int col3d;



/* prototypes */
void initialise(int argc,char *argv[]);
int titlewait(void);
void playgame(void);
void queuesam(int chan,int sam);
void uninitialise(void);
void wipeshots(void);
int getnewwave(int wavenum,int looped);
void showpaused(int showthem);
void firemany(int y);
void showstatus(int always);
void addnewshot(int x,int y);
int deadyet(int x1,int y1);
void do_death(int x,int y);
void centretextsize(int y,int size,char *str);
void centretext(int y,char *str);
void drawship(int x,int y,int col);
void drawframe(void);
void showtweenwave(int wavenum,int showthem,int looped);
int countbaddies(void);
void addnewdebris(int x,int y,int dx,int dy);
void drawbaddie(int x,int y,int which,int col);
void makedebrisfor(int index);
void addnewbaddie(int x,int y,int dx,int dy,int hits,int which);
void doendwave(void);
void doendlevel(int looped);
void showscore(void);
void drawstars(void);
void drawshots(void);
void drawdebris(void);
void drawbaddies(void);
int getscorefor(int which);
void makeflashout(int index);
void drawdebrisfor(int index,int col);
int findavailabledebris(void);
int findavailableshot(void);
int findavailablebaddie(void);
void init3dgrid(void);
void snd_main(void);
void snd_playchunk(void);




int main(int argc,char *argv[])
{
int quit=0;

initialise(argc,argv);

while(!quit && !fullquit)
  {
  quit=titlewait();
  if(!quit) playgame();
  }

queuesam(KILL_SNDSERV,0);
wait(NULL);

uninitialise();
exit(0);
}


void titledraw(void)
{
/* Draw score if there is one: */
if (score > 0)
 showscore();

/* - Skip heading
int a,b,i,f,ysub;
int xypos[]=
  {
   20,50, 120,50,  20,90, 120,90, 120,50, 220,70, 120,70, 220,90, 120,90,
  220,90, 220,50, 220,90, 320,90, 320,50, 420,90, 520,90, 420,50, 620,50,
  570,50, 570,90, -1,-1
  };

#ifndef USE_X
gl_clearscreen(0);
#else
if(doublebuf) pixmap=&window;

XSetForeground(disp,gc,blackpix);
XFillRectangle(disp,*pixmap,gc,0,0,pwidth,pheight);
#endif

if(!bigwindow)
  {
  i=0;
  while(xypos[i]!=-1)
    xypos[i]>>=1,xypos[i+1]>>=1,i+=2;
  }

vga_setcolor(11);
ysub=(bigwindow?30:15);
for(f=-4;f<=4;f++)
  {
  i=a=b=0;
  while(xypos[i]!=-1)
    {
    if(i>0)
      vga_drawline(f+a,b-ysub,f+xypos[i],xypos[i+1]-ysub);
    a=xypos[i]; b=xypos[i+1];
    i+=2;
    }
  }
*/
/* Vectrex Heading: */
#ifndef USE_X
v_printString(-128, 70, "ZBLAST", 48, 90);
#endif
/*
vga_setcolor(3);
centretextsize(90,bigwindow?2:1,
	"(C) 1993-2003 Russell Marks, PiTrex Version 2020 Kevin Koster");
*/
vga_setcolor(10);
centretextsize(375,bigwindow?6:3,
	"Hit 3 to play game");

vga_setcolor(2);
centretextsize(450,bigwindow?3:1,
	"in-game...  4 - Fire  3 - PDC  2 - Pause");

/* finally, draw hi-scores - or not...
#ifdef USE_X
drawhiscores(bigwindow,1);
#else
drawhiscores(bigwindow,0);
#endif
*/
}


int titlewait(void)
{
int done=0,quit=0;
#ifdef USE_X
XEvent event;
KeySym key;
char text[5];
int len;
#endif
#ifdef JOYSTICK_SUPPORT
struct joystate_tag *joystate;
#endif

init3d();	/* init anim */

if(music) queuesam(MUSIC_TITLE,0);

#ifdef USE_X
/* the first Expose event should draw the title screen first time around */
if(!wait_for_expose) titledraw();
wait_for_expose=0;

/* now need an complete X event loop (sigh) */
while(!done)
  {
  col3d=0;	/* undraw */
  do3dframe();
  incr3dpos();
  col3d=11;	/* draw */
  do3dframe();
  
  usleep(50000);

#ifdef JOYSTICK_SUPPORT
  if((joystate=joy_update())!=NULL && joystate->but[BUTTON_FIRE])
    done=1;
#endif
  
  while(XPending(disp))
    {
    XNextEvent(disp,&event);
    switch(event.type)
      {
      case MappingNotify:
        XRefreshKeyboardMapping((XMappingEvent *)&event);
        break;
#if 0		/* should only need Expose */
      case VisibilityNotify:
      case ConfigureNotify: /* when resized */
#endif
      case Expose:          /* when bit/all of window uncovered */
        /* wait for last expose of a group */
        if(event.xexpose.count==0)
          titledraw();
        break;
      case KeyRelease: break;	/* ignore */
      case KeyPress:
        len=XLookupString((XKeyEvent *)&event,text,4,&key,0);
        switch(key)
          {
          case XK_Return: done=1;		break;
          case XK_Escape: done=quit=1;	break;
          }
      case ClientMessage:
        if(event.xclient.data.l[0]==delete_atom)
          done=quit=1;
      }
    }
  }

#else /* !USE_X */

/* Need to move this to the loop since we don't have a buffer yet.
titledraw();
*/

keyboard_update();
/* while(keyboard_keypressed(SCANCODE_ESCAPE))
  usleep(20000),keyboard_update(); */

while(!done)
  {
  titledraw();
  col3d=0;	/* undraw */
  do3dframe();
  incr3dpos();
  col3d=11;	/* draw */
  do3dframe();
  if(doublebuf) gl_copyscreen(&scrn);
  
#ifdef JOYSTICK_SUPPORT
  if((joystate=joy_update())!=NULL && joystate->but[BUTTON_FIRE])
    done=1;
#endif
  
  keyboard_update();
  if(keyboard_keypressed(SCANCODE_ENTER))
    {
/*    while(keyboard_keypressed(SCANCODE_ENTER))
      usleep(20000),keyboard_update(); */
    done=1;
    }
  
/*  if(keyboard_keypressed(SCANCODE_ESCAPE))
    done=quit=1;  --SVGAmod - Use 1+2+3+4 to quit game entirely*/
  
  usleep(50000);
  }

#endif /* !USE_X */

return(quit);
}


/* this is used by anim3d.c to draw lines */
void drawofsline(int x1,int y1,int x2,int y2)
{
if(!bigwindow) x1/=2,y1/=2,x2/=2,y2/=2;

x1+=pwidth/2;	x2+=pwidth/2;
y1+=pheight/2;	y2+=pheight/2;

vga_setcolor(col3d);
vga_drawline(x1,y1,x2,y2);
}


/* this is used by hiscore.c to set colour (for vgadrawtext) */
void ext_setcol(int c)
{
vga_setcolor(c);
}


void playgame(void)
{
int quit,dead,wavenum,xpos,ypos,oldx,oldy,wasfire,waspdc,shield;
int looped;
int waspause;
int f,x;
#ifndef USE_X
char *keymap=keyboard_getstate();
#endif
#ifdef JOYSTICK_SUPPORT
struct joystate_tag *joystate;
#endif

#ifdef USE_X
if(doublebuf) pixmap=&pixmap1;
#endif

debrisnum=NORMDEBRISNUM;

for(x=0;x<BADDIENUM;x++)
  baddie[x].which=0;

framedelay=INIT_FRAMEDELAY;
frames=0;

lives=startlives;
quit=dead=wasfire=waspdc=waspause=paused=gameover_count=0;
wavenum=startlevel;
tweenwave=0;
looped=0;
pdc=5;
xpos=0; ypos=90;
shield=SHIELDLIFE;
score=0;

wipeshots();
#ifdef USE_X
XSetForeground(disp,gc,blackpix);
XFillRectangle(disp,*pixmap,gc,0,0,pwidth,pheight);
XSetForeground(disp,gc,whitepix);
#else
gl_clearscreen(0);
#endif
getnewwave(wavenum,looped);
donetrace=0;

/* turn off any old sounds */
for(f=0;f<NUM_CHANNELS;f++) channel[f].sample=NULL;

/* unpress keys */
keypress_up=keypress_down=keypress_left=keypress_right=0;
keypress_fire=keypress_pdc=keypress_sound=keypress_pause=keypress_quit=0;

if(music) queuesam(MUSIC_GAME,0);

/* about the while() condition -
 * when you die, dead=1, but while gameover_count is >0,
 * it's displaying the "game over" bit, so it still runs.
 */

while(!quit && (!dead || (dead && gameover_count>0)))
  {
  oldx=xpos; oldy=ypos;
  if(keypress_pause && waspause==0)		/* pause */
    {
    waspause=1;
    paused=!paused;
    if(paused==0)
      showpaused(0);
    }
  else
    if(keypress_pause==0 || keypress_quit) waspause=0;

  if(keypress_quit)  quit=1;                /* exit (esc) */
  
  if(!paused && !dead)
    {
    if(keypress_left)  if(xpos>-28) xpos-=2;        /* left */
    if(keypress_right) if(xpos< 28) xpos+=2;       /* right */
    if(keypress_up)    if(ypos> 80) ypos-=2;          /* up */
    if(keypress_down)  if(ypos< 96) ypos+=2;        /* down */
    if(keypress_pdc && waspdc==0 && pdc>0)
      {
      waspdc=1;
      pdc--;
      queuesam(PSHOT_CHANNEL,PWAVE_SAMPLE);
      firemany(ypos);
      showstatus(0);
      }
    else
      if(keypress_pdc==0) waspdc=0;
      
    if(keypress_fire)
      {
      if(wasfire<=0)
        {
        if(autofire)
          wasfire=DELAY_AUTO_FIRE;
        else
          wasfire=1;
        addnewshot(xpos-3,ypos-2);                   /* fire */
        addnewshot(xpos+3,ypos-2);
        queuesam(PSHOT_CHANNEL,PSHOT_SAMPLE);
        }
      else
        if(autofire)
          wasfire--;
      }
    else
      wasfire=0;

    if(shield)
      shield--;
    else
      if(deadyet(xpos,ypos))
        {
        lives--;
        lostlife++;
        if(lives<0)
          {
          dead=1;
          lives=0;	/* or else the display looks funny */
          do_death(xpos,ypos);
          }
        else
          {
          shield=SHIELDLIFE;
          queuesam(BIGHIT_CHANNEL,PHIT_SAMPLE);
          }
        showstatus(0);
        }
    }
  
  if(dead && gameover_count>0)
    {
    gameover_count--;
/*    
    vga_setcolor(1);
    centretext(0,"Press 1"); */ /* --SVGAmod */
    }
  
  drawship(oldx,oldy,0);
  if(!dead)
    drawship(xpos,ypos,(shield>0)?9:10);
  drawframe();

  if(paused)
    showpaused(1);
  else
    {
    if(tweenwave)
      {
      tweenwave--;
      showtweenwave(wavenum,(tweenwave<10)?0:1,looped);
      if(tweenwave==0) lostlife=0;
      }
    else
      {
      if(!dead && (count=countbaddies())==0)
        {
        wavenum++;
        looped=0;
        if(getnewwave(wavenum,looped)==-1)
          {
          /* speed up and loop back to start */
          looped=1;
          if(framedelay==INIT_FRAMEDELAY) framedelay-=FRAME_DECR;
          wavenum=1;
          getnewwave(wavenum,looped);
          }
        queuesam(EFFECT_CHANNEL,LVLEND_SAMPLE);
        }
      }
    }
  
  if(doublebuf) showstatus(1);
  
#ifndef USE_X

  /* actually draw the screen */  
  if(doublebuf) gl_copyscreen(&scrn);

  keyboard_update();
  keypress_up   =keymap[SCANCODE_CURSORBLOCKUP];
  keypress_down =keymap[SCANCODE_CURSORBLOCKDOWN];
  keypress_left =keymap[SCANCODE_CURSORBLOCKLEFT];
  keypress_right=keymap[SCANCODE_CURSORBLOCKRIGHT];
  keypress_fire =keymap[SCANCODE_SPACE];
  keypress_pdc  =keymap[SCANCODE_ENTER];
  keypress_pause=keymap[SCANCODE_P];
  keypress_quit =keymap[SCANCODE_ESCAPE];

#else /* USE_X */

  /* get all pending events */
  while(XPending(disp))
    {
    static XEvent event;
    static KeySym key;
    static char text[5];
    static int down,len;
    
    XNextEvent(disp,&event);
    switch(event.type)
      {
      case MappingNotify:
        XRefreshKeyboardMapping((XMappingEvent *)&event);
        break;
      case VisibilityNotify:
      case ConfigureNotify: /* when resized */
      case Expose:          /* when bit/all of window uncovered */
        /* do nothing, as we redisplay all the time.
         * except... redisplay status.
         * but, wait for last expose of a group.
         */
        if(event.xexpose.count==0)
          showstatus(1);
        break;
      case KeyPress:
      case KeyRelease:
        down=((event.type==KeyPress)?1:0);
        len=XLookupString((XKeyEvent *)&event,text,4,&key,0);
        switch(key)
          {
          /* KEYS HERE. (Well, it's easier to grep for :-))
           * These are keysyms - look in /usr/include/X11/keysymdef.h
           * for definitions, or use prefix "XK_" to most
           * alphanumerics.
           */
          case XK_Up: keypress_up=down;	break;
          case XK_Down: keypress_down=down;	break;
          case XK_Left: keypress_left=down;	break;
          case XK_Right: keypress_right=down;	break;
          case XK_space: keypress_fire=down;	break;
          case XK_p: case XK_P: keypress_pause=down;	break;
          case XK_Return: keypress_pdc=down;	break;
          case XK_Escape: keypress_quit=down;	break;
          }
        break;
      case ClientMessage:
        if(event.xclient.data.l[0]==delete_atom)
          quit=fullquit=1;
      }
    }
  
  /* actually draw the screen */
  if(doublebuf)
    {
    XCopyArea(disp,*pixmap,window,gc,0,0,pwidth,pheight,0,0);
    if(pixmap==&pixmap1) pixmap=&pixmap2; else pixmap=&pixmap1;
    }
  
  XSync(disp,0);
#endif /* USE_X */

#ifdef JOYSTICK_SUPPORT
  /* we need to use bit 1 for joystick stuff so it still works
   * in X, where we can't constantly monitor keyboard state.
   * So it's a bit fiddly. :-) Here we mask bit 1 out.
   */
  keypress_up&=1;
  keypress_down&=1;
  keypress_left&=1;
  keypress_right&=1;
  keypress_fire&=1;
  keypress_pdc&=1;
  
  if((joystate=joy_update())!=NULL)
    {
    /* these are meant to add to (or you could say, AND with) any
     * keyboard movement/buttons.
     */
    if(joystate->digital_axis[0])
      {
      if(joystate->digital_axis[0]<0) keypress_left=2;
      if(joystate->digital_axis[0]>0) keypress_right=2;
      }
    if(joystate->digital_axis[1])
      {
      if(joystate->digital_axis[1]<0) keypress_up=2;
      if(joystate->digital_axis[1]>0) keypress_down=2;
      }
    
    if(joystate->but[BUTTON_FIRE]) keypress_fire=2;
    if(joystate->but[BUTTON_PDC]) keypress_pdc=2;
    }
#endif
  
  if(framedelay>1000) usleep(framedelay);

#ifdef USE_X
  if(doublebuf)
    {
    XSetForeground(disp,gc,blackpix);
    XFillRectangle(disp,*pixmap,gc,0,0,pwidth,pheight);
    XSetForeground(disp,gc,whitepix);
    }
#else
  if(doublebuf) gl_clearscreen(0);
#endif
  
  frames++;
  }

/* end of game, save score */
if(!autofire)
  writescore(score);
}


/* this adds the "game over" thing, explodes the player, and sets
 * gameover_count. x,y are player pos.
 */
void do_death(int x,int y)
{
int org,mdx,mdy,pdx,pdy,dx,dy;
int a,b,f;

/* `game over' music (just something quiet) */
if(music) queuesam(MUSIC_GM_OVER,0);

/* first, remove player shots. Don't want any posthumous scoring. :-)
 */
for(f=0;f<SHOTNUM;f++)
  {
  if(shot[f].active)
    drawshot(f,0);
  shot[f].active=0;
  }

gameover_count=300;	/* probably about 10 sec or so */

/* ...normally you'd press Esc to quit it - you don't have to wait
 * at all. It just *always* ends after N secs, no matter what.
 *
 * --SVGAmod: No longer have the Esc, so reduce delay 
 */

debrisnum=MAXDEBRISNUM;	/* allow wildly massive amounts of debris */

/* make sure the debris bounces around a bit */
for(f=0;f<MAXDEBRISNUM;f++)
  debris[f].bounce=3;

/* have a nice BIG explosion for the player's ship.
 * this is deliberately very, very OTT. :-)
 */
mdx=-1; mdy=-1;
pdx= 1; pdy= 1;

for(b=-5;b<=3;b++)
  for(a=-2;a<=2;a++)
    {
    addnewdebris(x+a-1,y+b-1,mdx*(1+rand()%2),mdy*(1+rand()%2));
    addnewdebris(x+a+1,y+b-1,pdx*(1+rand()%2),mdy*(1+rand()%2));
    addnewdebris(x+a-1,y+b+1,mdx*(1+rand()%2),pdy*(1+rand()%2));
    addnewdebris(x+a+1,y+b+1,pdx*(1+rand()%2),pdy*(1+rand()%2));
    }

/* what the hell, give 'em a bit of revenge, blow up EVERYTHING >;-) */
for(f=0;f<BADDIENUM;f++)
  if(baddie[f].which)
    {
    drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,0);
    makedebrisfor(f);
    baddie[f].which=0;
    }

/* the game over message is composed of 8 baddies, one for each letter.
 */

/* the message is 47 wide, so starting at -23 centres it */
org=-23; dx=0; dy=1;
addnewbaddie(org+ 0,0,dx,dy,999,100);	/* G */
addnewbaddie(org+ 6,3,dx,dy,999,101);	/* A */
addnewbaddie(org+12,6,dx,dy,999,102);	/* M */
addnewbaddie(org+18,9,dx,dy,999,103);	/* E */
addnewbaddie(org+28,9,dx,dy,999,104);	/* O */
addnewbaddie(org+31,6,dx,dy,999,105);	/* V */
addnewbaddie(org+37,3,dx,dy,999,106);	/* E */
addnewbaddie(org+43,0,dx,dy,999,107);	/* R */

/* want as huge a sound as possible */
queuesam(SMALLHIT_CHANNEL,TYPE5DEAD_SAMPLE);
queuesam(BIGHIT_CHANNEL,TYPE11DEAD_SAMPLE);
queuesam(PSHOT_CHANNEL,PHIT_SAMPLE);
queuesam(BSHOT_CHANNEL,PWAVE_SAMPLE);
queuesam(EFFECT_CHANNEL,LVLEND_SAMPLE);
}


void firemany(int y)
{
int f;

for(f=0;f<10;f++)
  {
  addnewshot(f*3,89+f);
  addnewshot(-(f*3),89+f);
  }
}


int countbaddies(void)
{
int f,t;

for(f=0,t=0;f<BADDIENUM;f++)
  {
  if(baddie[f].which>0) t++;
  }
return(t);
}


/* returns -1 if no more waves */
int getnewwave(int wavenum,int looped)
{
if(createwave(wavenum)==-1)
  {
  /* wow, finished! */
  return(-1);
  }

tweenwave=60;   /* end wave */
if(wavenum>1)
  {
  doendwave();
  if(((wavenum-2)%5)==4)
    {
    tweenwave=200;   /* end level */
    doendlevel(looped);
    }
  }

showstatus(0);
count=countbaddies();

return(0);
}


void showpaused(int showthem)
{
vga_setcolor(showthem?14:0);
centretext(214," --- PAUSED --- ");
}


void showtweenwave(int wavenum,int showthem,int looped)
{
char buf[80];

if(wavenum>1 || looped)
  {
  if(lostlife==0)
    {
    vga_setcolor(showthem?14:0);
    centretext(267,"No hits sustained from wave - energy increased 10");
    }

  if(lostlife<10)
    {
    vga_setcolor(showthem?3:0);
    centretext(321,"Low energy loss bonus - extra PDC");
    }

  if(((wavenum-2)%5)==4)
    {
    vga_setcolor(showthem?15:0);
    sprintf(buf,"Level %d Cleared",(wavenum-1)/5);
    centretext(142,buf);
    vga_setcolor(showthem?14:0);
    if(looped)
      centretext(374,"All Levels Complete - energy increased 40");
    else
      centretext(374,"Level Complete Bonus - energy increased 10");
    }
  }

vga_setcolor(showthem?13:0);
sprintf(buf,"Prepare for Wave %d",wavenum);
centretext(214,buf);
}


void doendwave(void)
{
if(lostlife==0) lives+=10;
if(lostlife<10) pdc++;
}


void doendlevel(int looped)
{
lives+=10;
if(looped) lives+=30;
}


void centretextsize(int y,int size,char *str)
{
if(!bigwindow)
#ifdef USE_X
  y>>=1;
#else
  y=(y*200)/480;
#endif

vgadrawtext((pwidth-vgatextsize(size,str))>>1,y,size,str);
}


void centretext(int y,char *str)
{
centretextsize(y,bigwindow?3:2,str);
}


void showstatus(int always)
{
char buf[80];
int f;

if(doublebuf && !always) return;

vga_setcolor(0);
if(!doublebuf)
  {
  if(bigwindow)
    for(f=0;f<16;f++) vga_drawline(550,f,639,f);
  else
    for(f=0;f<16;f++) vga_drawline(260,f,319,f);
  }

vga_setcolor(12);
sprintf(buf,"%03d",lives);
vgadrawtext(bigwindow?560:280,0,bigwindow?3:2,buf);
sprintf(buf,"%03d",pdc);
vgadrawtext(bigwindow?600:300,0,bigwindow?3:2,buf);


showscore();
}


void showscore(void)
{
char buf[80];
int f;

sprintf(buf,"%08d",score);
if(!doublebuf)
  {
  vga_setcolor(0);
  for(f=0;f<16;f++) vga_drawline(0,f,72,f);
  }
vga_setcolor(12);
vgadrawtext(0,0,bigwindow?3:2,buf);
}


void drawframe(void)
{
drawstars();
drawshots();
if(usedebris)  drawdebris();
if(!tweenwave) drawbaddies();
}


/* notice that this routine also moves stars */
void drawstars(void)
{
int f;

if(!usestars) return;

for(f=0;f<STARNUM;f++)
  {
  if(!doublebuf)
    {
    vga_setcolor(0);
    vga_drawpixel(gridxy[star[f].y][32+star[f].x].x,
              gridxy[star[f].y][32+star[f].x].y);
    }
  star[f].y+=star[f].speed;
  if(star[f].y>=GRIDDEPTH)
    {
    star[f].y=0;
    star[f].x=(lowrand()%65)-32;
    }
  vga_setcolor(star[f].col);
  vga_drawpixel(gridxy[star[f].y][32+star[f].x].x,
            gridxy[star[f].y][32+star[f].x].y);

  }
}


/* notice that this routine also moves baddies */
void drawbaddies(void)
{
int f,g;

/* baddies being shot at was tested for in drawshots(), but
 * we'll still need to undraw the baddie if baddie[f].hits = 0.
 */


for(f=0;f<BADDIENUM;f++)
  {
  switch(baddie[f].which)
    {
    case 1:
    case 3:
    case 4:
      drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,0);
      if(baddie[f].hits<=0)
        {
        score+=getscorefor(baddie[f].which);
        baddie[f].which=0;
        }
      else
        {
        if(!paused)
          {
          baddie[f].x+=baddie[f].dx;
          if((baddie[f].x>30)||(baddie[f].x<-30))
            {
            baddie[f].dx=-baddie[f].dx;
            baddie[f].x+=2*baddie[f].dx;
            }
          baddie[f].y+=baddie[f].dy;
          if((baddie[f].y>97)||(baddie[f].y<1))
            {
            baddie[f].dy=-baddie[f].dy;
            baddie[f].y+=2*baddie[f].dy;
            }
          if(baddie[f].which==4)
            {
            if(lowrand()<800)                                /* shoot? */
              addnewbaddie(baddie[f].x,baddie[f].y+2,2,1+(lowrand()&1),1,2);
            }
          else
            if(lowrand()<100+500*(baddie[f].which-1))         /* shoot? */
              addnewbaddie(baddie[f].x,baddie[f].y+2,0,1+(lowrand()&1),1,2);
          }
        if(baddie[f].hitnow)
          {
          drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,15);
          baddie[f].hitnow--;
          }
        else
          drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,
                   15-baddie[f].which);
        }
      break;

    case 2:
      drawbaddie(baddie[f].x,baddie[f].y,2,0);
      if((baddie[f].y>=97)||(baddie[f].hits==0))
        {
        score+=getscorefor(baddie[f].which);
        baddie[f].which=0;
        }
      else
        {
        if(!paused)
          {
          baddie[f].x+=baddie[f].dx;
          if((baddie[f].x>32)||(baddie[f].x<-32))
            {
            baddie[f].dx=-baddie[f].dx;
            baddie[f].x+=2*baddie[f].dx;
            }
          baddie[f].y+=baddie[f].dy;
          if(baddie[f].y<1)	/* only bounce off top */
            {
            baddie[f].dy=3;	/* so they shoot down quick (heh, heh) */
            baddie[f].dx*=2;	/* make this a bit evil too */
            baddie[f].y=2;
            }
          }
        drawbaddie(baddie[f].x,baddie[f].y,2,15-(baddie[f].y%6));
        }
      break;

    case 6:
      drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,0);
      if(baddie[f].hits<=0)
        {
        score+=getscorefor(baddie[f].which);
        baddie[f].which=0;
        }
      else
        {
        if(!paused)
          {
          baddie[f].x+=baddie[f].dx;
          if((baddie[f].x>26)||(baddie[f].x<-26))
            {
            baddie[f].dx=-baddie[f].dx;
            baddie[f].x+=2*baddie[f].dx;
            }
          baddie[f].y+=baddie[f].dy;
          if((baddie[f].y>76)||(baddie[f].y<3))
            {
            baddie[f].dy=-baddie[f].dy;
            baddie[f].y+=2*baddie[f].dy;
            }
          if(lowrand()<4000)         /* shoot one way? */
            for(g=0;g<8;g+=2)
              {
              addnewbaddie(baddie[f].x-g/2,baddie[f].y+1+g,
              			0,3,1,2);
              addnewbaddie(baddie[f].x+g/2,baddie[f].y+1+g,
              			0,3,1,2);
              }
          if(lowrand()<500)         /* shoot 2nd way? */
            {
            addnewbaddie(baddie[f].x-1,baddie[f].y+5,-1,-1,1,2);
            addnewbaddie(baddie[f].x+1,baddie[f].y+5, 1,-1,1,2);
            }
          }
        if(baddie[f].hitnow)
          {
          drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,15);
          baddie[f].hitnow--;
          }
        else
          drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,14);
        }
      break;

    case 7:
      drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,0);
      if(baddie[f].hits<=0)
        {
        score+=getscorefor(baddie[f].which);
        baddie[f].which=0;
        }
      else
        {
        if(!paused)
          {
          baddie[f].x+=baddie[f].dx;
          if((baddie[f].x>28)||(baddie[f].x<-28))
            {
            baddie[f].dx=-baddie[f].dx;
            baddie[f].x+=2*baddie[f].dx;
            }
          baddie[f].y+=baddie[f].dy;
          if((baddie[f].y>76)||(baddie[f].y<3))
            {
            baddie[f].dy=-baddie[f].dy;
            baddie[f].y+=2*baddie[f].dy;
            }
          if(lowrand()<1000)         /* shoot one way? */
            for(g=0;g<6;g+=2)
              {
              if(baddie[f].y-2-g>0)
                {
                addnewbaddie(baddie[f].x-g/2,baddie[f].y-2-g,-2,-1,1,2);
                addnewbaddie(baddie[f].x+g/2,baddie[f].y-2-g, 2,-1,1,2);
                }
              }
          }
        if(baddie[f].hitnow)
          {
          drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,15);
          baddie[f].hitnow--;
          }
        else
          drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,13);
        }
      break;

    case 10:
      drawbaddie(baddie[f].x,baddie[f].y,10,0);
      if(baddie[f].hits<=0)
        {
        score+=getscorefor(baddie[f].which);
        baddie[f].which=0;
        }
      else
        {
        if(!paused)
          {
          baddie[f].x+=baddie[f].dx;
          if((baddie[f].x>24)||(baddie[f].x<-24))
            {
            baddie[f].dx=-baddie[f].dx;
            baddie[f].x+=2*baddie[f].dx;
            }
          baddie[f].y+=baddie[f].dy;
          if((baddie[f].y>72)||(baddie[f].y<8))
            {
            baddie[f].dy=-baddie[f].dy;
            baddie[f].y+=2*baddie[f].dy;
            }

          if(lowrand()<500)                /* shoot a type 3? */
            addnewbaddie(baddie[f].x,baddie[f].y+2,(lowrand()&1)?-1:1,
            				1+(lowrand()&1),3,3);   /* 3 hits */
          if(lowrand()<2000)                /* shoot? */
            {
            addnewbaddie(baddie[f].x-6,baddie[f].y+8,0,2,1,2);
            addnewbaddie(baddie[f].x+6,baddie[f].y+8,0,2,1,2);
            }
          }

        if(baddie[f].hitnow)
          {
          drawbaddie(baddie[f].x,baddie[f].y,10,15);
          baddie[f].hitnow--;
          }
        else
          drawbaddie(baddie[f].x,baddie[f].y,10,5);
        }
      break;

    case 11:
      drawbaddie(baddie[f].x,baddie[f].y,11,0);
      if(baddie[f].hits<=0)
        {
        score+=getscorefor(baddie[f].which);
        baddie[f].which=0;
        }
      else
        {
        if(!paused)
          {
          baddie[f].x+=baddie[f].dx;
          if((baddie[f].x>8)||(baddie[f].x<-8))
            {
            baddie[f].dx=-baddie[f].dx;
            baddie[f].x+=2*baddie[f].dx;
            }
          baddie[f].y+=baddie[f].dy;
          if((baddie[f].y>64)||(baddie[f].y<16))
            {
            baddie[f].dy=-baddie[f].dy;
            baddie[f].y+=2*baddie[f].dy;
            }

          if(lowrand()<200)                /* shoot a type 10!? */
            addnewbaddie(baddie[f].x-((lowrand()&1)?-15:15),baddie[f].y+8,
                         (lowrand()&1)?-1:1,1,10,10);   /* 10 hits */

          if(lowrand()<2000)                /* shoot a type 1? */
            {
            addnewbaddie(baddie[f].x-24,baddie[f].y+12,0,2,1,2);
            addnewbaddie(baddie[f].x+24,baddie[f].y+12,0,2,1,2);
            }
          }
        if(baddie[f].hitnow)
          {
          drawbaddie(baddie[f].x,baddie[f].y,11,15);
          baddie[f].hitnow--;
          }
        else
          drawbaddie(baddie[f].x,baddie[f].y,11,5);
        }
      break;

    case 5:
      drawbaddie(baddie[f].x,baddie[f].y,5,0);
      if(baddie[f].hits<=0)
        {
        score+=getscorefor(baddie[f].which);
        baddie[f].which=0;
        }
      else
        {
        if(!paused)
          {
          baddie[f].x+=baddie[f].dx;
          baddie[f].y+=baddie[f].dy;

          if(baddie[f].x<=-28)
            {
            baddie[f].x=-27;
            baddie[f].dy=baddie[f].dx;
            baddie[f].dx=0;
            }
          if(baddie[f].y<=4)
            {
            baddie[f].y=5;
            baddie[f].dx=-baddie[f].dy;
            baddie[f].dy=0;
            }
          if(baddie[f].x>=28)
            {
            baddie[f].x=27;
            baddie[f].dy=baddie[f].dx;
            baddie[f].dx=0;
            }
          if(baddie[f].y>=94)
            {
            baddie[f].y=93;
            baddie[f].dx=-baddie[f].dy;
            baddie[f].dy=0;
            }

          if(lowrand()<500)                /* shoot? */
            addnewbaddie(baddie[f].x,baddie[f].y+4,0,2,1,2);
          }

        if(baddie[f].hitnow)
          {
          drawbaddie(baddie[f].x,baddie[f].y,5,15);
          baddie[f].hitnow--;
          }
        else
          drawbaddie(baddie[f].x,baddie[f].y,5,4);
        }
      break;
    
    
    /* the remainder are for the "game over" message. Each letter
     * is an independent baddie.
     */
    
    case 100: case 101: case 102: case 103:
    case 104: case 105: case 106: case 107:
      drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,0);
      if(!paused)
        {
        baddie[f].x+=baddie[f].dx;
        if((baddie[f].x>28)||(baddie[f].x<-32))
          baddie[f].dx=-baddie[f].dx,baddie[f].x+=2*baddie[f].dx;
        baddie[f].y+=baddie[f].dy;
        if((baddie[f].y>90)||(baddie[f].y<0))
          baddie[f].dy=-baddie[f].dy,baddie[f].y+=2*baddie[f].dy;
        }
      drawbaddie(baddie[f].x,baddie[f].y,baddie[f].which,
      		2+(baddie[f].which+frames/2)%4);
      break;
    }
  }
}


int deadyet(int x1,int y1)
{
int f,x2,y2;

/* one way to be generous and make it quicker:
 * only 'midpoints' (that is, baddie[f].x and .y) in a box from
 * x-2,y-2 to x+2,y+1.
 */

x2=x1+2; y2=y1+1;
x1-=2;   y1-=2;

for(f=0;f<BADDIENUM;f++)
  {
  if(baddie[f].which>0)
    if((baddie[f].x>=x1)&&(baddie[f].x<=x2)&&
       (baddie[f].y>=y1)&&(baddie[f].y<=y2))
      return(1);   /* you only need to be hit once to die...! */
  }
return(0);
}


void seeifshothitbaddie(void)
{
int f,g;
int x1,y1,x2,y2;
int die;
int oldscore;

oldscore=score;
for(f=0;f<BADDIENUM;f++)
  {
  if(baddie[f].which!=0)
    {
    switch(baddie[f].which)
      {
      case 1:
      case 3:
      case 4:
        /* we treat types 1,3,4 as a box from x-2,y-1 to x+2,y+2 */
        x1=baddie[f].x; y1=baddie[f].y;
        x2=x1+2;        y2=y1+2;
        x1-=2;          y1--;
        break;

      case 5:
        x1=baddie[f].x; y1=baddie[f].y;
        x2=x1+4;        y2=y1+4;
        x1-=4;          y1-=4;
        break;

      case 2:
        /* you can't hit a normal baddie shot */
        continue;

      case 6:
        /* type 6 treated as box from x-5,y-3 to x+5,y+1 */
        x1=baddie[f].x; y1=baddie[f].y;
        x2=x1+5;        y2=y1+1;
        x1-=5;		y1-=3;
        break;

      case 7:
        /* type 7 treated as box from x-5,y-3 to x+5,y+1 */
        x1=baddie[f].x; y1=baddie[f].y;
        x2=x1+5;        y2=y1+1;
        x1-=5;		y1-=3;
        break;

      case 10:
        /* we treat type 10 as a box from x-8,y-6 to x+8,y+2 */
        x1=baddie[f].x; y1=baddie[f].y;
        x2=x1+8;        y2=y1+2;
        x1-=8;          y1-=6;
        break;

      case 11:
        /* type 11 is treated as a box from x-18,y-12 to x+18,y+4 */
        x1=baddie[f].x; y1=baddie[f].y;
        x2=x1+18;       y2=y1+4;
        x1-=18;         y1-=12;
        break;
      
      default:
        /* must be one of the game-over letters - ignore it */
        continue;
      }

    die=0;
    for(g=0;(g<SHOTNUM)&&(!die);g++)
      {
      if(shot[g].active)
        {
        if((shot[g].x>=x1)&&(shot[g].x<=x2)&&
           (shot[g].y>=y1)&&(shot[g].y<=y2))
          {
          drawshot(g,0);
          shot[g].active=0;
          baddie[f].hits--;
          baddie[f].hitnow=3;
          die=1;
          score++;
          
          if(baddie[f].hits==0)
            {
            makeflashout(f);
            makedebrisfor(f);
            switch(baddie[f].which)
              {
              case  1: queuesam(SMALLHIT_CHANNEL,TYPE1DEAD_SAMPLE); break;
              case  3: queuesam(SMALLHIT_CHANNEL,TYPE3DEAD_SAMPLE); break;
              case  4: queuesam(SMALLHIT_CHANNEL,TYPE4DEAD_SAMPLE); break;
              case  5: queuesam(SMALLHIT_CHANNEL,TYPE5DEAD_SAMPLE); break;
              case  6: queuesam(BIGHIT_CHANNEL,  TYPE6DEAD_SAMPLE); break;
              case 10: queuesam(BIGHIT_CHANNEL,  TYPE10DEAD_SAMPLE); break;
              case 11: queuesam(BIGHIT_CHANNEL,  TYPE11DEAD_SAMPLE); break;
              }
            }
          else
            {
            switch(baddie[f].which)
              {
              case  1: queuesam(SMALLHIT_CHANNEL,TYPE1HIT_SAMPLE); break;
              case  3: queuesam(SMALLHIT_CHANNEL,TYPE3HIT_SAMPLE); break;
              case  4: queuesam(SMALLHIT_CHANNEL,TYPE4HIT_SAMPLE); break;
              case  5: queuesam(SMALLHIT_CHANNEL,TYPE5HIT_SAMPLE); break;
              case  6: queuesam(BIGHIT_CHANNEL,  TYPE6HIT_SAMPLE); break;
              case 10: queuesam(BIGHIT_CHANNEL,  TYPE10HIT_SAMPLE); break;
              case 11: queuesam(BIGHIT_CHANNEL,  TYPE11HIT_SAMPLE); break;
              }
            }
          }
        }
      }
    }
  }

  // on vectrex this lets the score blink!
// if(score!=oldscore) showscore();
}


void drawshots(void)
{
int f,shotfin;

if((!tweenwave)&&(!paused)) seeifshothitbaddie();
for(f=0;f<SHOTNUM;f++)
  {
  if(shot[f].active)
    {
    drawshot(f,0);
    if(!paused) shot[f].y-=2;
    if(shot[f].y<0) shotfin=1; else shotfin=0;
    if(shotfin)
      shot[f].active=0;
    else
      drawshot(f,12);
    }
  }
}


void drawdebris(void)
{
int f;
int dead=(debrisnum==MAXDEBRISNUM);
int xl,xr,yt,yb,xsiz,ysiz;

for(f=0;f<debrisnum;f++)
  {
  switch(debris[f].active)
    {
    case 1:
      /* normal debris, (lines) */
      drawdebrisfor(f,0);
  
      if(!paused)
        {
        /* move */
        debris[f].x+=debris[f].dx;
        debris[f].y+=debris[f].dy;
        }
  
      if((debris[f].x<-32)||(debris[f].x>32)||
         (debris[f].y<  0)||(debris[f].y>98))
        if(!dead)
          debris[f].active=0;
        else
          {
          /* if game over, let 'em bounce around a bit */
          if(debris[f].x<-32 || debris[f].x>32)
            debris[f].dx=-debris[f].dx,debris[f].x+=2*debris[f].dx;
          if(debris[f].y<  0 || debris[f].y>98)
            debris[f].dy=-debris[f].dy,debris[f].y+=2*debris[f].dy;
          debris[f].bounce--;
          if(debris[f].bounce<=0) debris[f].active=0;
          }
      else /* if not out of bounds... */
        drawdebrisfor(f,debris[f].line?(dead?(2+frames%6):2):10);
      
      break;
    
    case 2:
      /* flashout.
       * we draw a *big* star on the grid, centered at x,y.
       * note that because it's drawn over itself and fades to
       * black, we never need to undraw it.
       */
      
      /* XXX test - just do the easy ones :-) up and across. */
      vga_setcolor(debris[f].col);
      
      xsiz=3*5,ysiz=2*5;
      
      xl=debris[f].x-xsiz;
      xr=debris[f].x+xsiz;
      if(xl<-32) xl=-32;
      if(xr> 32) xr= 32;
      yt=debris[f].y-ysiz;
      yb=debris[f].y+ysiz;
      if(yt<0) yt=0;
      if(yb>GRIDDEPTH-2) yb=GRIDDEPTH-2;
      
      /* horiz */
      gridmoveto(32+xl,debris[f].y);
      gridlineto(32+xr,debris[f].y);
      
      /* vert */
      gridmoveto(32+debris[f].x,yt);
      gridlineto(32+debris[f].x,yb);
      
      debris[f].col-=debris[f].decr;
      if(debris[f].col<0)
        debris[f].active=0;
      
      break;
    }
  }
}


void drawdebrisfor(int index,int col)
{
int x,y;

if(doublebuf && col==0) return;

vga_setcolor(col);
x=debris[index].x; y=debris[index].y;
if(debris[index].line)
  {
  gridmoveto(32+x,y);
  gridlineto(32+x-(sgnval(debris[index].dx)),
              y-(sgnval(debris[index].dy)));
  }
else
  vga_drawpixel(gridxy[y][32+x].x,gridxy[y][32+x].y);
}


void makedebrisfor(int index)
{
int x,y,dx,dy,mdx,mdy,pdx,pdy;

 x=baddie[index].x;   y=baddie[index].y;
dx=baddie[index].dx; dy=baddie[index].dy;
mdx=dx-2; if(mdx==0) mdx=-1;
mdy=dy-2; if(mdy==0) mdy=-1;
pdx=dx+2; if(pdx==0) pdx= 1;
pdy=dy+2; if(pdy==0) pdy= 1;

switch(baddie[index].which)
  {
  case 1: case 3: case 4:
    addnewdebris(x-1,y-1,mdx,mdy);
    addnewdebris(x+1,y-1,pdx,mdy);
    addnewdebris(x-1,y+1,mdx,pdy);
    addnewdebris(x+1,y+1,pdx,pdy);
    break;

  case 5:
    addnewdebris(x-2,y-2,mdx,mdy);
    addnewdebris(x+2,y-2,pdx,mdy);
    addnewdebris(x-2,y+2,mdx,pdy);
    addnewdebris(x+2,y+2,pdx,pdy);

    addnewdebris(x  ,y-2,  0,mdy);
    addnewdebris(x-2,y  ,mdx,  0);
    addnewdebris(x+2,y  ,pdx,  0);
    addnewdebris(x  ,y+2,  0,pdy);
    break;

  case 6: case 7: case 10: case 11:
    addnewdebris(x-4,y-4,mdx,mdy);
    addnewdebris(x+4,y-4,pdx,mdy);
    addnewdebris(x-4,y+4,mdx,pdy);
    addnewdebris(x+4,y+4,pdx,pdy);

    addnewdebris(x  ,y-4,  0,mdy);
    addnewdebris(x-4,y  ,mdx,  0);
    addnewdebris(x+4,y  ,pdx,  0);
    addnewdebris(x  ,y+4,  0,pdy);
    break;
  }
}


void makeflashout(int index)
{
int n;

if((n=findavailabledebris())!=-1)
  {
  debris[n].x=baddie[index].x;
  debris[n].y=baddie[index].y;
  debris[n].dx=0;
  debris[n].dy=0;
  debris[n].active=2;
  debris[n].line=0;
  debris[n].col=15;
  debris[n].decr=3;
  }
}


void addnewshot(int x,int y)
{
int n;

if((n=findavailableshot())!=-1)
  {
  shot[n].x=x;
  shot[n].y=y;
  shot[n].active=1;
  }
}


void addnewdebris(int x,int y,int dx,int dy)
{
int n;

if((n=findavailabledebris())!=-1)
  {
  debris[n].x=x;
  debris[n].y=y;
  debris[n].dx=dx;
  debris[n].dy=dy;
  debris[n].active=1;
  debris[n].line=1;
  }
}


void addnewbaddie(int x,int y,int dx,int dy,int hits,int which)
{
int n;

if((n=findavailablebaddie())!=-1)
  {
  baddie[n].x=x;
  baddie[n].y=y;
  baddie[n].dx=dx;
  baddie[n].dy=dy;
  baddie[n].hits=hits;
  baddie[n].which=which;
  baddie[n].hitnow=0;
  if(which==2)
    queuesam(BSHOT_CHANNEL,BSHOT_SAMPLE);
  }
}


void initscrn(int argc,char *argv[])
{
int f;

#ifdef USE_X

/* X init */

XWMHints wmhints;
XSizeHints hint;
XColor col;

disp=XOpenDisplay(NULL);	/* XXX should do getenv etc. */
if(disp==(Display *)NULL)
  {
  fprintf(stderr,"xzb: couldn't open display\n");
  exit(1);
  }
screen=DefaultScreen(disp);
blackpix=BlackPixel(disp,screen);
whitepix=WhitePixel(disp,screen);

hint.min_width =hint.max_width =hint.base_width =pwidth;
hint.min_height=hint.max_height=hint.base_height=pheight;
hint.flags=PPosition|PSize|PMinSize|PMaxSize|PBaseSize;
depth=DefaultDepth(disp,screen);

window=XCreateSimpleWindow(disp,DefaultRootWindow(disp),
  0,0,pwidth,pheight,5,whitepix,blackpix);
pixmap1=XCreatePixmap(disp,window,pwidth,pheight,depth);
pixmap2=XCreatePixmap(disp,window,pwidth,pheight,depth);
if(doublebuf)
  pixmap=&pixmap1;
else
  pixmap=&window;

icon_pixmap=XCreateBitmapFromData(disp,window,xzb_bits,xzb_width,xzb_height);

XSetStandardProperties(disp,window,"X ZBlast","xzb",
		icon_pixmap,argv,argc,&hint);

/* delete-window message stuff, from xloadimage via dosemu. */
proto_atom =XInternAtom(disp,"WM_PROTOCOLS",False);
delete_atom=XInternAtom(disp,"WM_DELETE_WINDOW",False);
if(proto_atom!=None && delete_atom!=None)
  XChangeProperty(disp,window,proto_atom,XA_ATOM,32,
  		PropModePrepend,(char *)&delete_atom,1);

gc=XCreateGC(disp,window,0,0);
XSetGraphicsExposures(disp,gc,0);
XSetBackground(disp,gc,blackpix);
XSetForeground(disp,gc,whitepix);

wmhints.flags=IconPixmapHint | InputHint;
wmhints.input=True;
wmhints.icon_pixmap=icon_pixmap;
XSetWMHints(disp,window,&wmhints);
XSelectInput(disp,window,KeyPressMask | KeyReleaseMask | StructureNotifyMask |
                         VisibilityChangeMask | ExposureMask);
XMapRaised(disp,window);

if(doublebuf)
  {
  XSetForeground(disp,gc,blackpix);
  XFillRectangle(disp,pixmap1,gc,0,0,pwidth,pheight);
  XFillRectangle(disp,pixmap2,gc,0,0,pwidth,pheight);
  XSetForeground(disp,gc,whitepix);
  }

XSync(disp,0);

cmap=DefaultColormap(disp,screen);

/* do colour stuff */
if(depth>4)
  {
  usemono=0;
  col.flags=(DoRed|DoGreen|DoBlue);
  /* get 16 read-only */
  for(f=0;f<16;f++)
    {
    col.red  =colourrgb[f][0];
    col.green=colourrgb[f][1];
    col.blue =colourrgb[f][2];
    if(!XAllocColor(disp,cmap,&col))
      {
      fprintf(stderr,"xzb: can't allocate all colours, using mono\n");
      usemono=1;
      return;
      }
    colourtable[f]=col.pixel;
    }
  }

#else /* !USE_X */

/* svgalib init */
int oldgid=getegid();

vga_disabledriverreport();
vga_init();
setegid(oldgid);	/* make sure we can write scores file! */
scrnmode=(bigwindow?G640x480x256:G320x200x256);
vga_setmode(scrnmode);
gl_setcontextvga(scrnmode);
if(doublebuf)
  {
//  scrn=currentcontext;
  gl_setcontextvgavirtual(scrnmode);
  }
gl_enableclipping();
keyboard_init();
keyboard_translatekeys(DONT_CATCH_CTRLC & TRANSLATE_DIAGONAL);

for(f=0;f<16;f++)
  gl_setpalettecolor(f,
  	colourrgb[f][0]&63,colourrgb[f][1]&63,colourrgb[f][2]&63);

#endif /* !USE_X */

#ifdef JOYSTICK_SUPPORT
have_joy=joy_init();
#endif
}


void sigquit_handler(int c)
{
got_sigquit=1;
}


void initialise(int argc,char *argv[])
{
int f;

/* XXX bleah */
for(f=1;f<argc;f++)
  {
  if(strcmp(argv[f],"-small")==0) bigwindow=0;
  if(strcmp(argv[f],"-nodbuf")==0) doublebuf=0;
  if(strcmp(argv[f],"-nomusic")==0) music=0;
  if(strcmp(argv[f],"-nosound")==0) sound=music=0;
  if(strcmp(argv[f],"-autofire")==0)
    {
    autofire=1;
    fprintf(stderr,"Note: `-autofire' disables hi-score recording.\n");
    }
  }

#ifndef USE_X
if(bigwindow)
  {
  /* check card can do 640x480x256 */
  if(!vga_hasmode(G640x480x256))
    {
    bigwindow=0;
    printf("zblast: no 640x480x256 mode available, using 320x200\n");
    }
  }
#endif

if(bigwindow) pwidth=640,pheight=480;

init3dgrid();
wipeshots();
for(f=0;f<BADDIENUM;f++)
  baddie[f].which=0;
for(f=0;f<STARNUM;f++)
  {
  star[f].x=(lowrand()%65)-32;
  star[f].y=lowrand()%GRIDDEPTH;
  switch(star[f].speed=(1+(lowrand()%3)))
    {
    case 1:
      star[f].col=3; break;
    case 2:
      star[f].col=3; break;
    case 3:
      star[f].col=7; break;
    }
  }
  
for(f=0;f<NUM_SAMPLES;f++) sample[f].data=NULL;

pipe(sndpipe);
fcntl(sndpipe[0],F_SETFL,O_NONBLOCK);
fcntl(sndpipe[1],F_SETFL,O_NONBLOCK);

parentpid=getpid();

if(fork())
  {
  /* parent - game */
#ifdef MUSIC_SUPPORT
  if(music)
    {
    signal(SIGQUIT,sigquit_handler);
    
    /* wait for the SIGQUIT, to say 'finished reading music' */
    while(!got_sigquit) pause();
    }
#endif
  
  initscrn(argc,argv);
  return;
  }

/* child - sound player */

#ifndef USE_X	/* no need for non-setuid X version */
setuid(getuid()); setgid(getgid());
#endif

sound_bufsiz=BASE_SOUND_BUFSIZ;
#ifndef SNDCTL_DSP_SETFRAGMENT
sound=0;
soundfd=-1;
#else
if(sound)
  {
  if((soundfd=open("/dev/dsp",O_WRONLY))<0)
    {
    sound=0;
    soundfd=-1;
    }
  else
    {
    int frag,tmp;
    FILE *in;
    char buf[256];
  
    /* try for stereo */
    tmp=1;
    if(ioctl(soundfd,SNDCTL_DSP_STEREO,&tmp)==-1) stereo=0;
    
    /* see if we can get freq (16kHz), else use 8 */
    tmp=freq;
    ioctl(soundfd,SNDCTL_DSP_SPEED,&tmp);
    if(tmp<freq-freq/32)		/* if less than 15500Hz */
      {
      tmp=freq=8000;
      ioctl(soundfd,SNDCTL_DSP_SPEED,&tmp);
      }
    
    /* want 8-bit unsigned also, but those are default anyway */
    
#ifdef MORE_SOUNDBUF
    frag=(0x40000|BASE_SOUND_FRAG_PWR);
#else
    frag=(0x20000|BASE_SOUND_FRAG_PWR);
#endif
    
    if(stereo) frag++,sound_bufsiz*=2;
    
    /* XXX would need changing for >16kHz */
    if(freq>8000) frag++,sound_bufsiz*=2;
    
    ioctl(soundfd,SNDCTL_DSP_SETFRAGMENT,&frag);
    
    /* load in the samples */
    for(f=0;f<NUM_SAMPLES;f++)
      {
      snprintf(buf,sizeof(buf),"%s/%s",SOUNDSDIR,samplename[f]);
      if((in=fopen(buf,"rb"))!=NULL)
        {
        fseek(in,0,SEEK_END);
        sample[f].length=ftell(in);
        if((sample[f].data=(unsigned char *)malloc(sample[f].length))==NULL)
          break;
        rewind(in);
        fread(sample[f].data,1,sample[f].length,in);
        fclose(in);
        }
      }
    }
  }
#endif

snd_main();
}


/* also wipes debris */
void wipeshots(void)
{
int f;

for(f=0;f<SHOTNUM;f++)
  shot[f].active=0;

for(f=0;f<MAXDEBRISNUM;f++)
  debris[f].active=0;
}


void uninitialise(void)
{
#ifdef USE_X
XFreeGC(disp,gc);
if(doublebuf)
  {
  XFreePixmap(disp,pixmap1);
  XFreePixmap(disp,pixmap2);
  }
XFreePixmap(disp,icon_pixmap);
XDestroyWindow(disp,window);
XCloseDisplay(disp);

#else /* !USE_X */

keyboard_close();
vga_setmode(TEXT);
#endif /* !USE_X */

if(soundfd!=-1) close(soundfd);

#ifdef JOYSTICK_SUPPORT
joy_close();
#endif
}


/* return first available shot as index into shot[], -1 on error */
int findavailableshot(void)
{
int f;

for(f=0;f<SHOTNUM;f++)
  if(shot[f].active==0) return(f);
return(-1);
}


int findavailabledebris(void)
{
int f;

for(f=0;f<debrisnum;f++)
  if(debris[f].active==0) return(f);
return(-1);
}


int findavailablebaddie(void)
{
int f;

for(f=0;f<BADDIENUM;f++)
  if(baddie[f].which==0) return(f);
return(-1);
}


void drawbaddie(int x,int y,int which,int col)
{
if(doublebuf && col==0) return;

x+=32;
vga_setcolor(col);
switch(which)
  {
  case 1:
  case 3:
  case 4:
    gridmoveto(x-2,y+1);
    gridlineto(x-2,y-1);
    gridlineto(x+2,y-1);
    gridlineto(x+2,y+1);
    gridlineto(x-2,y+1);
    gridlineto(x-1,y+2);

    gridmoveto(x+2,y+1);
    gridlineto(x+1,y+2);
    break;

  case 2:
    /* a line, for better visibility. notice only the lower end of the
     * the line is dangerous! */
    gridmoveto(x,y);
    gridlineto(x,y-1);
    break;

  case 5:
    gridmoveto(x-4,y-4);
    gridlineto(x+4,y-4);
    gridlineto(x+4,y+4);
    gridlineto(x-4,y+4);
    gridlineto(x-4,y-4);
    break;

  case 6:
    gridmoveto(x-6,y-3);
    gridlineto(x  ,y-2);
    gridlineto(x+6,y-3);
    gridlineto(x+1,y+4);
    gridlineto(x+3,y  );
    gridlineto(x+1,y  );
    gridlineto(x+1,y+1);
    gridlineto(x-1,y+1);
    gridlineto(x-1,y  );
    gridlineto(x-3,y  );
    gridlineto(x-1,y+4);
    gridlineto(x-6,y-3);
    break;

  case 7:
    gridmoveto(x-2,y+5);
    gridlineto(x-4,y+2);
    gridlineto(x-4,y  );
    gridlineto(x-3,y-2);
    gridlineto(x-1,y-1);
    gridlineto(x  ,y-2);
    gridlineto(x+1,y-1);
    gridlineto(x+3,y-2);
    gridlineto(x+4,y  );
    gridlineto(x+4,y+2);
    gridlineto(x+2,y+5);
    gridlineto(x+3,y+2);
    gridlineto(x+2,y  );
    gridlineto(x+1,y+1);
    gridlineto(x+1,y+2);
    gridlineto(x  ,y+4);
    gridlineto(x-1,y+2);
    gridlineto(x-1,y+1);
    gridlineto(x-2,y  );
    gridlineto(x-3,y+2);
    gridlineto(x-2,y+5);
    break;

  case 10:
    gridmoveto(x-6,y+8);
    gridlineto(x-4,y  );
    gridlineto(x+4,y  );
    gridlineto(x+6,y+8);
    gridlineto(x+8,y-4);
    gridlineto(x+6,y-4);
    gridlineto(x+2,y-8);
    gridlineto(x-2,y-8);
    gridlineto(x-6,y-4);
    gridlineto(x-8,y-4);
    gridlineto(x-6,y+8);
    break;

  case 11:
    gridmoveto(x   ,y+16);
    gridlineto(x+ 6,y+12);
    gridlineto(x+ 3,y+12);
    gridlineto(x+12,y   );
    gridlineto(x+18,y   );
    gridlineto(x+24,y+12);
    gridlineto(x+18,y-16);
    gridlineto(x+12,y- 8);
    gridlineto(x-12,y- 8);
    gridlineto(x-18,y-16);
    gridlineto(x-24,y+12);
    gridlineto(x-18,y   );
    gridlineto(x-12,y   );
    gridlineto(x- 3,y+12);
    gridlineto(x- 6,y+12);
    gridlineto(x   ,y+16);
    break;
  
  
  /* see note in drawbaddies() about these */
  case 100:
    /* G */
    gridmoveto(x+4,y+2);
    gridlineto(x+4,y+4);
    gridlineto(x+0,y+4);
    gridlineto(x+4,y+0);
    break;
  
  case 101:
    /* A */
    gridmoveto(x+0,y+4);
    gridlineto(x+0,y+0);
    gridlineto(x+4,y+4);
    gridmoveto(x+0,y+2);
    gridlineto(x+2,y+2);
    break;
  
  case 102:
    /* M */
    gridmoveto(x+0,y+4);
    gridlineto(x+0,y+0);
    gridlineto(x+2,y+4);
    gridlineto(x+2,y+0);
    gridlineto(x+4,y+4);
    break;
  
  case 103:
  case 106:
    /* E */
    gridmoveto(x+4,y+4);
    gridlineto(x+0,y+4);
    gridlineto(x+0,y+0);
    gridlineto(x+4,y+2);
    gridmoveto(x+0,y+2);
    gridlineto(x+2,y+2);
    break;
  
  case 104:
    /* O */
    gridmoveto(x+0,y+4);
    gridlineto(x+0,y+0);
    gridlineto(x+4,y+4);
    gridlineto(x+0,y+4);
    break;
  
  case 105:
    /* V */
    gridmoveto(x+0,y+0);
    gridlineto(x+4,y+4);
    gridlineto(x+4,y+0);
    break;
  
  /* 106 (E) was done above */
  
  case 107:
    /* R */
    gridmoveto(x+0,y+4);
    gridlineto(x+0,y+0);
    gridlineto(x+4,y+2);
    gridlineto(x+0,y+2);
    gridlineto(x+4,y+4);
    break;
  }
}


void drawship(int x,int y,int col)
{
if(doublebuf && col==0) return;

x+=32;
vga_setcolor(col);
gridmoveto(x+1,y  );
gridlineto(x  ,y-2);
gridlineto(x-1,y  );
gridlineto(x-3,y+1);
gridlineto(x+3,y+1);
gridlineto(x+1,y  );

gridmoveto(x+3,y+1);
gridlineto(x+3,y-2);

gridmoveto(x-3,y+1); 
gridlineto(x-3,y-2);
}


void init3dgrid(void)
{
double xmul,ymul;
int nx,ny;
struct grid tmp;

xmul=1.5; ymul=0.6;
for(ny=0;ny<GRIDDEPTH;ny++)
  {
  for(nx=-32;nx<=32;nx++)
    {
    if(bigwindow)
      {
      tmp.x=(int)((320.0+(double)nx*xmul));
      tmp.y=(int)((10.0+(double)ny*ymul)/350.0*480.0);
      }
    else
      {
      tmp.x=(int)((320.0+(double)nx*xmul)/2.0);
#ifdef USE_X
      tmp.y=(int)((10.0+(double)ny*ymul)/350.0*240.0);
#else
      tmp.y=(int)((10.0+(double)ny*ymul)/350.0*200.0);
#endif
      }
    gridxy[ny][nx+32]=tmp;
    }
  xmul*=1.021;
  ymul*=1.018;
  }
}


int getscorefor(int which)
{
switch(which)   /* he's a poet and he doesn't know it */
  {
  case 1:
    return(10);
  case 3:
    return(20);
  case 4:
    return(50);
  case 5:
    return(20);
  case 6:
    return(200);
  case 10:
    return(200);
  case 11:
    return(500);
  default:
    return(0);
  }
}


/* setup a new sample to be played on a given channel. */
void queuesam(int chan,int sam)
{
unsigned char buf[2];

buf[0]=chan;
buf[1]=sam;
write(sndpipe[1],buf,2);
}


#ifdef MUSIC_SUPPORT
void play_write(int c)
{
samp_ret[samp_idx++]=c;
}
#endif


void snd_main(void)
{
unsigned char buf[2];
int live=1,ret;

#ifdef MUSIC_SUPPORT
char csf1[1024],csf2[1024],csf3[1024];
struct sod2_context con1,con2,con3;

if(music)
  {
  fprintf(stderr,"Loading music, please wait");
  if(stereo) realstereo=1;
  preconstruct=1;
  play_set_rate(freq);
  snprintf(csf1,sizeof(csf1),"%s/%s",SOUNDSDIR,"title.csf");
  snprintf(csf2,sizeof(csf2),"%s/%s",SOUNDSDIR,"ingame.csf");
  snprintf(csf3,sizeof(csf3),"%s/%s",SOUNDSDIR,"gameover.csf");
  fputc('.',stderr);
  play_init(csf1,play_write);
  save_sod2_context(&con1);
  fputc('.',stderr);
  play_init(csf2,play_write);
  save_sod2_context(&con2);
  fputc('.',stderr);
  play_init(csf3,play_write);
  save_sod2_context(&con3);
  music=0;	/* don't play any music until told */
  
  /* tell parent we've finished reading music */
  fprintf(stderr,"\r                              \r");
  kill(parentpid,SIGQUIT);
  }
#endif

close(sndpipe[1]);

while(live)
  {
  while((ret=read(sndpipe[0],buf,2))>0)
    {
    switch(*buf)
      {
      case KILL_SNDSERV:
        live=0;
        break;
      
      case TURN_SOUND_ON:
        sound=1;
        break;
        
      case TURN_SOUND_OFF:
        sound=0;
        break;
        
      case MUSIC_TITLE:
#ifdef MUSIC_SUPPORT
        music=1;
        restore_sod2_context(&con1);
        play_init_existing();
#endif
        break;
        
      case MUSIC_GAME:
#ifdef MUSIC_SUPPORT
        music=1;
        restore_sod2_context(&con2);
        play_init_existing();
#endif
        break;
        
      case MUSIC_GM_OVER:
#ifdef MUSIC_SUPPORT
        music=1;
        restore_sod2_context(&con3);
        play_init_existing();
#endif
        break;
        
      default:
        channel[buf[0]].sample=&(sample[buf[1]]);
        channel[buf[0]].offset=0;
      }
    }
  
  /* stop if pipe was broken */
  if(ret==0) break;
  
  snd_playchunk();
  }

exit(0);
}



#define BYTEFIX(x)	(((x)<0)?0:(((x)>255)?255:(x)))

/* mix and play a chunk of sound to /dev/dsp. */
void snd_playchunk(void)
{
int f,g,v,last=128;
struct channel_tag *cptr;
static unsigned char soundbuf[2048];
int toggle=0;

if(soundfd==-1 || sound==0)
  {
  usleep(50000);
  return;
  }

for(f=0;f<sound_bufsiz;f++,toggle=!toggle)
  {
  v=0;
  if(freq>8000 && toggle)
    v=last;
  else
    {
    for(g=0,cptr=&(channel[0]);g<NUM_CHANNELS;g++,cptr++)
      if(cptr->sample!=NULL)
        {
        v+=(int)cptr->sample->data[cptr->offset++];
        if(cptr->offset>=cptr->sample->length)
          cptr->sample=NULL;
        }
      else
        v+=128;		/* make sure it doesn't click! */
    
    /* kludge to up the volume of sound effects - mmm, lovely distortion :-) */
    v-=128*NUM_CHANNELS;
    v=128+(v*3)/(2*NUM_CHANNELS);
    v=BYTEFIX(v);
    
    last=v;
    }
  
#ifdef MUSIC_SUPPORT
  /* add on music */
  if(music)
    {
    samp_idx=0;
    play_sample();
    if(play_done) play_init_existing();
    }
  else
    samp_ret[0]=samp_ret[1]=128;
  
  soundbuf[f]=(unsigned char)BYTEFIX(v+samp_ret[0]-128);
  if(stereo)
    soundbuf[++f]=(unsigned char)BYTEFIX(v+samp_ret[1]-128);

#else	/* !MUSIC_SUPPORT */
  soundbuf[f]=(unsigned char)v;
  if(stereo)
    soundbuf[++f]=(unsigned char)v;
#endif	/* !MUSIC_SUPPORT */
  }

write(soundfd,soundbuf,sound_bufsiz);
}
