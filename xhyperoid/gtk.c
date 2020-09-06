/* gtk.c */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "misc.h"	/* for POINT and MAX_COORD */

#include "graphics.h"


/* bitmaps */
#include "xpm/icon.xpm"

#include "xpm/num0.xpm"
#include "xpm/num1.xpm"
#include "xpm/num2.xpm"
#include "xpm/num3.xpm"
#include "xpm/num4.xpm"
#include "xpm/num5.xpm"
#include "xpm/num6.xpm"
#include "xpm/num7.xpm"
#include "xpm/num8.xpm"
#include "xpm/num9.xpm"
#include "xpm/blank.xpm"
#include "xpm/bomb.xpm"
#include "xpm/level.xpm"
#include "xpm/life.xpm"
#include "xpm/plus.xpm"
#include "xpm/score.xpm"
#include "xpm/shield.xpm"

#define NUM_BITMAPS	17

/* pointers to above (all 16x16) */
static char **orig_xpms[NUM_BITMAPS]=
  {
  num0_xpm,num1_xpm,num2_xpm,num3_xpm,num4_xpm,
  num5_xpm,num6_xpm,num7_xpm,num8_xpm,num9_xpm,
  blank_xpm,bomb_xpm,level_xpm,life_xpm,plus_xpm,
  score_xpm,shield_xpm
  };

/* pixmap conversions of above */
GdkPixmap *bitmaps[NUM_BITMAPS];

/* indicies in above arrays */
#define BMP_NUM0	0
#define BMP_NUM1	1
#define BMP_NUM2	2
#define BMP_NUM3	3
#define BMP_NUM4	4
#define BMP_NUM5	5
#define BMP_NUM6	6
#define BMP_NUM7	7
#define BMP_NUM8	8
#define BMP_NUM9	9
#define BMP_BLANK	10
#define BMP_BOMB	11
#define BMP_LEVEL	12
#define BMP_LIFE	13
#define BMP_PLUS	14
#define BMP_SCORE	15
#define BMP_SHIELD	16


static int quit=0;

static GtkWidget *window,*drawing_area,*score_area;

static GdkColor colourtable[16];
static GdkGC *gc[16];
static int current_col=0;

static int need_resize=0;

static int need_keyrep_restore=0;
static int key_f1=0,key_tab=0,key_s=0;
static int key_left=0,key_right=0,key_down=0,key_up=0;
static int key_space=0,key_esc=0;

static int width,height,mindimhalf;

/* this oddity is needed to simulate the mapping the original used a
 * Windows call for. MAX_COORD is a power of 2, so when optimised
 * this isn't too evil.
 */
#define convx(x)	(width/2+(x)*mindimhalf/MAX_COORD)
#define convy(y)	(height/2-(y)*mindimhalf/MAX_COORD)



int IsKeyDown(int key)
{
switch(key)
  {
  case KEY_F1:		return(key_f1);
  case KEY_TAB:		return(key_tab);
  case KEY_S:		return(key_s);
  case KEY_LEFT:	return(key_left);
  case KEY_RIGHT:	return(key_right);
  case KEY_DOWN:	return(key_down);
  case KEY_UP:		return(key_up);
  case KEY_SPACE:	return(key_space);
  case KEY_ESC:		return(key_esc);
  default:
    return(0);
  }
}


void Polyline(POINT *pts,int n)
{
static GdkPoint points[1024];
int f;

if(n<2 || quit) return;

for(f=0;f<=n;f++)
  points[f].x=convx(pts[f].x),points[f].y=convy(pts[f].y);

gdk_draw_lines(drawing_area->window,gc[current_col],points,n);
}


/* doesn't set current_[xy] because hyperoid.c doesn't need it to */
void SetPixel(int x,int y,int c)
{
if(!quit)
  gdk_draw_point(drawing_area->window,gc[c],convx(x),convy(y));
}


void set_colour(int c)
{
current_col=c;
}


/* SetIndicator - set a quantity indicator */

int SetIndicator( char * npBuff, char bitmap, int nQuant )
{
	if (nQuant > 5)
	{
		*npBuff++ = bitmap; *npBuff++ = bitmap;
		*npBuff++ = bitmap; *npBuff++ = bitmap;
		*npBuff++ = BMP_PLUS;
	}
	else
	{
		int nBlank = 5 - nQuant;
		while (nQuant--) *npBuff++ = bitmap;
		while (nBlank--) *npBuff++ = BMP_BLANK;
	}
	return( 5 );
}


/* score_graphics - draw score and rest of status display */

void score_graphics(int level,int score,int lives,int shield,int bomb)
{
static char szScore[40];
char szBuff[sizeof(szScore)];
char *npBuff = szBuff;
int nLen, x, y;

if(quit) return;

*npBuff++ = BMP_LEVEL;
sprintf( npBuff, "%2.2d", level );
while (isdigit( *npBuff ))
	*npBuff = (char)(*npBuff + BMP_NUM0 - '0'), ++npBuff;
*npBuff++ = BMP_BLANK;
*npBuff++ = BMP_SCORE;
sprintf( npBuff, "%7.7d", score );
while (isdigit( *npBuff ))
	*npBuff = (char)(*npBuff + BMP_NUM0 - '0'), ++npBuff;
*npBuff++ = BMP_BLANK;
npBuff += SetIndicator( npBuff, BMP_LIFE, lives );
npBuff += SetIndicator( npBuff, BMP_SHIELD, shield );
npBuff += SetIndicator( npBuff, BMP_BOMB, bomb );
nLen = npBuff - szBuff;

for(y=0;y<16;y++)
  for(x=0;x<nLen;x++)
    gdk_window_copy_area(score_area->window,gc[0],
    		x*16,0,bitmaps[(int)szBuff[x]],0,0,16,16);
}



void cb_quit(GtkWidget *widget,gpointer data)
{
quit=1;
}


gint da_keypress(GtkWidget *widget,GdkEventKey *event)
{
switch(event->keyval)
  {
  case GDK_F1:		key_f1=1; break;  
  case GDK_Tab:		key_tab=1; break;
  case GDK_s:		key_s=1; break;
  case GDK_Left:	key_left=1; break;
  case GDK_Right:	key_right=1; break;
  case GDK_Down:	key_down=1; break;
  case GDK_Up:		key_up=1; break;
  case GDK_space:	key_space=1; break;
  case GDK_Escape:	key_esc=1; break;
  }

return(TRUE);
}


gint da_keyrelease(GtkWidget *widget,GdkEventKey *event)
{
switch(event->keyval)
  {
  case GDK_F1:		key_f1=0; break;  
  case GDK_Tab:		key_tab=0; break;
  case GDK_s:		key_s=0; break;
  case GDK_Left:	key_left=0; break;
  case GDK_Right:	key_right=0; break;
  case GDK_Down:	key_down=0; break;
  case GDK_Up:		key_up=0; break;
  case GDK_space:	key_space=0; break;
  case GDK_Escape:	key_esc=0; break;
  }

return(TRUE);
}


gint da_resized(GtkWidget *widget,GdkEventConfigure *event)
{
static int first=1;

if(first)
  {
  first=0;
  return(FALSE);
  }

need_resize=1;
return(FALSE);
}


gint focus_change_event(GtkWidget *widget,GdkEventFocus *event)
{
if(event->in)
  gdk_key_repeat_disable(),need_keyrep_restore=1;
else
  gdk_key_repeat_restore(),need_keyrep_restore=0;

return(FALSE);	/* just in case anything else needs it */
}


void init_window(void)
{
/* we have a 16-pixel high window at top for score etc., and the rest is
 * the game window. Both are plain old drawingareas.
 */
GtkWidget *vbox;

window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
GTK_WIDGET_SET_FLAGS(window,GTK_CAN_FOCUS);
gtk_signal_connect(GTK_OBJECT(window),"destroy",
			GTK_SIGNAL_FUNC(cb_quit),NULL);
gtk_window_set_title(GTK_WINDOW(window),"xhyperoid");

gtk_window_set_default_size(GTK_WINDOW(window),448,420);

vbox=gtk_vbox_new(FALSE,0);
GTK_WIDGET_UNSET_FLAGS(vbox,GTK_CAN_FOCUS);
gtk_container_add(GTK_CONTAINER(window),vbox);
gtk_widget_show(vbox);

/* score area */
score_area=gtk_drawing_area_new();
GTK_WIDGET_UNSET_FLAGS(score_area,GTK_CAN_FOCUS);
gtk_box_pack_start(GTK_BOX(vbox),score_area,FALSE,FALSE,0);
gtk_widget_set_usize(score_area,16,16);
gtk_widget_show(score_area);

/* the main drawing area */
drawing_area=gtk_drawing_area_new();
GTK_WIDGET_UNSET_FLAGS(drawing_area,GTK_CAN_FOCUS);
gtk_box_pack_start(GTK_BOX(vbox),drawing_area,TRUE,TRUE,0);
gtk_widget_set_usize(drawing_area,32,32);

/* we don't bother with expose events as we're constantly redrawing */
gtk_signal_connect(GTK_OBJECT(drawing_area),"configure_event",
		GTK_SIGNAL_FUNC(da_resized),NULL);
gtk_widget_set_events(drawing_area,GDK_STRUCTURE_MASK);

gtk_widget_show(drawing_area);

gtk_signal_connect(GTK_OBJECT(window),"key_press_event",
		GTK_SIGNAL_FUNC(da_keypress),NULL);
gtk_signal_connect(GTK_OBJECT(window),"key_release_event",
		GTK_SIGNAL_FUNC(da_keyrelease),NULL);

/* need to get focus change events so we can fix the auto-repeat
 * when focus is changed, not just when we start/stop.
 */
gtk_signal_connect(GTK_OBJECT(window),"focus_in_event",
		GTK_SIGNAL_FUNC(focus_change_event),NULL);
gtk_signal_connect(GTK_OBJECT(window),"focus_out_event",
		GTK_SIGNAL_FUNC(focus_change_event),NULL);

gtk_widget_set_events(window,
	GDK_KEY_PRESS_MASK|GDK_KEY_RELEASE_MASK|
	GDK_FOCUS_CHANGE_MASK);

gtk_widget_grab_focus(window);
gtk_widget_show(window);

gdk_key_repeat_disable(),need_keyrep_restore=1;

while(gtk_events_pending())
  gtk_main_iteration();

width=drawing_area->allocation.width;
height=drawing_area->allocation.height;
mindimhalf=((width<height)?width:height)/2;
}



void get_colours(int *palrgb)
{
GdkColormap *cmap=gdk_window_get_colormap(window->window);
GdkColor col;
int f;
int do_mono=0;

if(gdk_visual_get_best_depth()<4) do_mono=1;

/* get 16 read-only */
if(!do_mono)
  for(f=0;f<16;f++)
    {
    col.red  =((palrgb[f*3  ]<<8)|palrgb[f*3  ]);
    col.green=((palrgb[f*3+1]<<8)|palrgb[f*3+1]);
    col.blue =((palrgb[f*3+2]<<8)|palrgb[f*3+2]);
    
    if(gdk_colormap_alloc_color(cmap,&col,FALSE,TRUE))
      colourtable[f]=col;
    else
      {
      /* XXX in practice we never seem to get here because
       * we'll get `best fit' colours no matter how terrible
       * they are! :-/
       */
      do_mono=1;
      fprintf(stderr,
        "xhyperoid: can't allocate all colours, using mono...\n");
      if(f>0)
        gdk_colormap_free_colors(cmap,colourtable,f);
      break;
      }
    }

if(!do_mono) return;

colourtable[0]=window->style->black;
for(f=1;f<16;f++)
  colourtable[f]=window->style->white;
}


void conv_xpms(void)
{
int f;

for(f=0;f<NUM_BITMAPS;f++)
  if((bitmaps[f]=gdk_pixmap_create_from_xpm_d(window->window,
  			NULL,colourtable+BLACK,orig_xpms[f]))==NULL)
    fprintf(stderr,"XPM conversion failed!\n"),exit(1);
}


void gc_init(void)
{
int f;

for(f=0;f<16;f++)
  {
  gc[f]=gdk_gc_new(drawing_area->window);
  gdk_gc_set_background(gc[f],colourtable+BLACK);
  gdk_gc_set_foreground(gc[f],colourtable+f);
  
  /* probably not necessary, but just in case */
  gdk_gc_set_line_attributes(gc[f],
  	0,GDK_LINE_SOLID,GDK_CAP_NOT_LAST,GDK_JOIN_MITER);
  }

/* XXX maybe I'm being dim :-), but I can't see an obvious reason why
 * I need this. Still, it avoids "invalid resource ID chosen for this
 * connection" errors ("error_code 14") on exit...
 */
gdk_flush();
}


/* note that this actually changes palrgb[] (ouch :-)) */
void graphics_init(int argc,char *argv[],int *palrgb)
{
GdkPixmap *icon;
GdkBitmap *icon_mask;

gtk_init(&argc,&argv);

init_window();

get_colours(palrgb);

gdk_window_set_background(drawing_area->window,colourtable+BLACK);
gdk_window_clear(drawing_area->window);

gdk_window_set_background(score_area->window,colourtable+BLACK);
gdk_window_clear(score_area->window);

conv_xpms();

/* set icon */
/* XXX this is the original 32x32 one - could do with being larger */
icon=gdk_pixmap_create_from_xpm_d(window->window,&icon_mask,NULL,icon_xpm);
gdk_window_set_icon(window->window,NULL,icon,icon_mask);

gc_init();

/* XXX this doesn't work - should set a flag which would exit from
 * graphics_update().
 */
signal(SIGTERM,(void (*)())cb_quit);
}


void graphics_update(void)
{
if(quit)
  {
  graphics_exit();
  gtk_exit(0);
  }

while(gtk_events_pending())
  gtk_main_iteration();

gdk_flush();

if(need_resize)
  {
  need_resize=0;
  width=drawing_area->allocation.width;
  height=drawing_area->allocation.height;
  mindimhalf=((width<height)?width:height)/2;
  }
}


void graphics_exit(void)
{
if(!quit)
  gdk_colormap_free_colors(gdk_window_get_colormap(window->window),
  	colourtable,16);

if(need_keyrep_restore)
  gdk_key_repeat_restore();
}
