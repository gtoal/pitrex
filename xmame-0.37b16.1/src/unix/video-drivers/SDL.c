/***************************************************************************
                                          
 This is the SDL XMAME display driver.
 FIrst incarnation by Tadeusz Szczyrba <trevor@pik-nel.pl>,
 based on the Linux SVGALib adaptation by Phillip Ezolt.

 updated and patched by Ricardo Calixto Quesada (riq@core-sdi.com)

 patched by Patrice Mandin (pmandin@caramail.com)
  modified support for fullscreen modes using SDL and XFree 4
  added toggle fullscreen/windowed mode (Alt + Return)
  added title for the window
  hide mouse cursor in fullscreen mode
  added command line switch to start fullscreen or windowed
  modified the search for the best screen size (SDL modes are sorted by
    Y size)

 patched by Dan Scholnik (scholnik@ieee.org)
  added support for 32bpp XFree86 modes
  new update routines: 8->32bpp & 16->32bpp

 TODO: Test the HERMES code.
       Test the 16bpp->24bpp update routine
       Test the 16bpp->32bpp update routine
       Improve performance.
       Test mouse buttons (which games use them?)

***************************************************************************/
#define __SDL_C

#undef SDL_DEBUG

/*
 * Defining OFFSCREEN sets the SDL driver to the original behaviour.
 * ie. It's pretty slow as it's using a two buffer approach
 * The non-OFFSCREEN mode writes directly to videoram and is 
 * considerably faster
 */
//#define OFFSCREEN

#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <SDL/SDL.h>
#include "xmame.h"
#include "devices.h"
#include "keyboard.h"
#include "driver.h"
#include "SDL-keytable.h"

#ifdef OFFSCREEN
#define BLIT_PIXELS Offscreen_surface->pixels
#define BLIT_SURFACE Offscreen_surface
#else
#define BLIT_PIXELS ((Surface->pixels)+Vid_offset)
#define BLIT_SURFACE Surface
#endif

#ifdef RASPI
static int doublebuf=0;
static int vsync=0;
#endif
static int Vid_width;
static int Vid_height;
static int Vid_depth = 8;
#ifdef OFFSCREEN
static SDL_Surface* Offscreen_surface;
#else
static int Vid_offset;
#endif
static SDL_Surface* Surface;
static int hardware=1;
static int mode_number=-1;
static int list_modes=0;
static int start_fullscreen=0;
SDL_Color *Colors=NULL;
static int cursor_state; /* previous mouse cursor state */

typedef void (*update_func_t)(struct osd_bitmap *bitmap);

update_func_t update_function;

static int sdl_mapkey(struct rc_option *option, const char *arg, int priority);

struct rc_option display_opts[] = {
    /* name, shortname, type, dest, deflt, min, max, func, help */
   { "SDL Related",  NULL,    rc_seperator,  NULL,
       NULL,         0,       0,             NULL,
       NULL },
   { "listmodes",    NULL,    rc_bool,       &list_modes,
      "0",           0,       0,             NULL,
      "List all posible full-screen modes" },
   { "fullscreen",   NULL,    rc_bool,       &start_fullscreen,
      "0",           0,       0,             NULL,
      "Start fullscreen" },
#ifdef RASPI
   { "doublebuf",   NULL,    rc_bool,       &doublebuf,
      "0",           0,       0,             NULL,
      "Use double buffering to reduce flicker/tearing" },
   { "vsync",       NULL,    rc_bool,       &vsync,
      "0",           0,       0,             NULL,
      "Use vsync to reduce flicker/tearing" },
#endif
   { "modenumber",   NULL,    rc_int,        &mode_number,
      "-1",          0,       0,             NULL,
      "Try to use the 'n' possible full-screen mode" },
   { "sdlmapkey",	"sdlmk",	rc_use_function,	NULL,
     NULL,		0,			0,		sdl_mapkey,
     "Set a specific key mapping, see xmamerc.dist" },
   { NULL,           NULL,    rc_end,        NULL,
      NULL,          0,       0,             NULL,
      NULL }
};

void list_sdl_modes(void);
void sdl_update_8_to_8bpp(struct osd_bitmap *bitmap);
void sdl_update_8_to_16bpp(struct osd_bitmap *bitmap);
void sdl_update_8_to_24bpp(struct osd_bitmap *bitmap);
void sdl_update_8_to_32bpp(struct osd_bitmap *bitmap);
void sdl_update_16_to_16bpp(struct osd_bitmap *bitmap);
void sdl_update_16_to_24bpp(struct osd_bitmap *bitmap);
void sdl_update_16_to_32bpp(struct osd_bitmap *bitmap);
void sdl_update_rgb_direct_32bpp(struct osd_bitmap *bitmap);

int sysdep_init(void)
{
   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf (stderr, "SDL: Error: %s\n",SDL_GetError());
      return OSD_NOT_OK;
   } 
   fprintf (stderr, "SDL: Info: SDL initialized\n");
   atexit (SDL_Quit);
   return OSD_OK;
}

void sysdep_close(void)
{
   SDL_Quit();
}

int sysdep_create_display(int depth)
{
   SDL_Rect** vid_modes;
   const SDL_VideoInfo* video_info;
   int vid_modes_i;
   int vid_mode_flag; /* Flag to set the video mode */

   if(list_modes){
      list_sdl_modes();
      exit (OSD_OK);
   }

   video_info = SDL_GetVideoInfo();

#ifdef SDL_DEBUG
   fprintf (stderr,"SDL: create_display(%d): \n",depth);
   fprintf (stderr,"SDL: Info: HW blits %d\n"
      "SDL: Info: SW blits %d\n"
      "SDL: Info: Vid mem %d\n"
      "SDL: Info: Best supported depth %d\n",
      video_info->blit_hw,
      video_info->blit_sw,
      video_info->video_mem,
      video_info->vfmt->BitsPerPixel);
#endif

   Vid_depth = video_info->vfmt->BitsPerPixel;

   vid_modes = SDL_ListModes(NULL,SDL_FULLSCREEN);
   vid_modes_i = 0;

   hardware = video_info->hw_available;

   if ( (! vid_modes) || ((long)vid_modes == -1)) {
#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: Info: Possible all video modes available\n");
#endif
      Vid_height = visual_height*heightscale;
      Vid_width = visual_width*widthscale;
   } else {
      int best_vid_mode; /* Best video mode found */
      int best_width,best_height;
      int i;

#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: visual w:%d visual h:%d\n", visual_width, visual_height);
#endif
      best_vid_mode = 0;
      best_width = vid_modes[best_vid_mode]->w;
      best_height = vid_modes[best_vid_mode]->h;
      for (i=0;vid_modes[i];++i)
      {
         int cur_width, cur_height;

         cur_width = vid_modes[i]->w;
         cur_height = vid_modes[i]->h;

#ifdef SDL_DEBUG
         fprintf (stderr, "SDL: Info: Found mode %d x %d\n", cur_width, cur_height);
#endif /* SDL_DEBUG */

         /* If width and height too small, skip to next mode */
         if ((cur_width < visual_width*widthscale) || (cur_height < visual_height*heightscale)) {
            continue;
         }

         /* If width or height smaller than current best, keep it */
         if ((cur_width < best_width) || (cur_height < best_height)) {
            best_vid_mode = i;
            best_width = cur_width;
            best_height = cur_height;
         }
      }

#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: Info: Best mode found : %d x %d\n",
         vid_modes[best_vid_mode]->w,
         vid_modes[best_vid_mode]->h);
#endif /* SDL_DEBUG */

      vid_modes_i = best_vid_mode;

      /* mode_number is a command line option */
      if( mode_number != -1) {
         if( mode_number >vid_modes_i)
            fprintf(stderr, "SDL: The mode number is invalid... ignoring\n");
         else
            vid_modes_i = mode_number;
      }
      if( vid_modes_i<0 ) {
         fprintf(stderr, "SDL: None of the modes match :-(\n");
         Vid_height = visual_height*heightscale;
         Vid_width = visual_width*widthscale;
      } else {
         if(*(vid_modes+vid_modes_i)==NULL) 
            vid_modes_i=vid_modes_i-1;

         Vid_width = (*(vid_modes + vid_modes_i))->w;
         Vid_height = (*(vid_modes + vid_modes_i))->h;
      }
   }

   if( depth == 8 ) {
      switch( Vid_depth ) {
      case 32:
         update_function = &sdl_update_8_to_32bpp;
         break;
      case 24:
         update_function = &sdl_update_8_to_24bpp;
         break;
      case 16:
         update_function = &sdl_update_8_to_16bpp;
         break;
      case 8:
         update_function = &sdl_update_8_to_8bpp;
         break;
      default:
         fprintf (stderr, "SDL: Unsupported Vid_depth=%d in depth=%d\n", Vid_depth,depth);
         SDL_Quit();
         exit (OSD_NOT_OK);
         break;
      }
   }
   else if( depth == 16 )
   {
      switch( Vid_depth ) {
      case 32:
         update_function = &sdl_update_16_to_32bpp;
         break;
      case 24:
         update_function = &sdl_update_16_to_24bpp;
         break;
      case 16:
         update_function = &sdl_update_16_to_16bpp;
         break;
      default:
         fprintf (stderr, "SDL: Unsupported Vid_depth=%d in depth=%d\n", Vid_depth,depth);
         SDL_Quit();
         exit (OSD_NOT_OK);
         break;
      }
   }
   else if (depth == 32)
   {
      if (Vid_depth == 32 && Machine->drv->video_attributes & VIDEO_RGB_DIRECT)
      {
         update_function = &sdl_update_rgb_direct_32bpp; 
      }
      else
      {
         fprintf (stderr, "SDL: Unsupported Vid_depth=%d in depth=%d\n",
            Vid_depth, depth);
         SDL_Quit();
         exit (OSD_NOT_OK);
      }
   }
   else
   {
      fprintf (stderr, "SDL: Unsupported depth=%d\n", depth);
      SDL_Quit();
      exit (OSD_NOT_OK);
   }


   /* Set video mode according to flags */
   vid_mode_flag = SDL_HWSURFACE;
   if (start_fullscreen) {
      vid_mode_flag |= SDL_FULLSCREEN;
   }
#ifdef RASPI
   if (doublebuf) {
      vid_mode_flag |= SDL_DOUBLEBUF;
   }
   if (!vsync) {
      vid_mode_flag |= SDL_ASYNCBLIT;
   }
#endif
   if(! (Surface = SDL_SetVideoMode(Vid_width, Vid_height,Vid_depth, vid_mode_flag))) {
      fprintf (stderr, "SDL: Error: Setting video mode failed\n");
      SDL_Quit();
      exit (OSD_NOT_OK);
   } else {
      fprintf (stderr, "SDL: Info: Video mode set as %d x %d, depth %d\n", Vid_width, Vid_height, Vid_depth);
   }

#ifdef OFFSCREEN
   Offscreen_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,Vid_width,Vid_height,Vid_depth,0,0,0,0); 
   if(Offscreen_surface==NULL) {
      SDL_Quit();
      exit (OSD_NOT_OK);
   }
#else
   /* Center the display */
   Vid_offset = 
       (
	   (((Vid_height - visual_height*heightscale ) >> 1) * Vid_width) +	/* offset Y */
	   ((Vid_width - visual_width*widthscale ) >> 1)			/* offset X */
       ) << (Vid_depth>>4);
#endif

   printf("SDL x:%d y:%d d:%d\n", Vid_width, Vid_height, Vid_depth);

   /* Creating event mask */
   SDL_EventState(SDL_KEYUP, SDL_ENABLE);
   SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
   SDL_EnableUNICODE(1);
   
    /* fill the display_palette_info struct */
    memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
    display_palette_info.depth = Vid_depth;
    if (Vid_depth == 8)
         display_palette_info.writable_colors = 256;
    else if (Vid_depth == 16) {
      display_palette_info.red_mask = 0xF800;
      display_palette_info.green_mask = 0x07E0;
      display_palette_info.blue_mask   = 0x001F;
    }
    else {
      display_palette_info.red_mask   = 0x00FF0000;
      display_palette_info.green_mask = 0x0000FF00;
      display_palette_info.blue_mask  = 0x000000FF;
    };

   /* Hide mouse cursor and save its previous status */
   cursor_state = SDL_ShowCursor(0);

   /* Set window title */
   SDL_WM_SetCaption(title, NULL);

   return OSD_OK;
}

/*
 *  keyboard remapping routine
 *  invoiced in startup code
 *  returns 0-> success 1-> invalid from or to
 */
static int sdl_mapkey(struct rc_option *option, const char *arg, int priority)
{
   unsigned int from, to;
   /* ultrix sscanf() requires explicit leading of 0x for hex numbers */
   if (sscanf(arg, "0x%x,0x%x", &from, &to) == 2)
   {
      /* perform tests */
      /* fprintf(stderr,"trying to map %x to %x\n", from, to); */
      if (from >= SDLK_FIRST && from < SDLK_LAST && to >= 0 && to <= 127)
      {
         klookup[from] = to;
	 return OSD_OK;
      }
      /* stderr_file isn't defined yet when we're called. */
      fprintf(stderr,"Invalid keymapping %s. Ignoring...\n", arg);
   }
   return OSD_NOT_OK;
}

/* Update routines */
void sdl_update_8_to_8bpp (struct osd_bitmap *bitmap)
{
#define DEST_WIDTH Vid_width
#define DEST BLIT_PIXELS
#define SRC_PIXEL unsigned char
#define DEST_PIXEL unsigned char

#include "blit.h"

#undef DEST_PIXEL
#undef SRC_PIXEL
#undef DEST
#undef DEST_WIDTH
}

void sdl_update_8_to_16bpp(struct osd_bitmap *bitmap)
{
#define BLIT_16BPP_HACK
#define INDIRECT current_palette->lookup
#define SRC_PIXEL  unsigned char
#define DEST_PIXEL unsigned short
#define DEST BLIT_PIXELS
#define DEST_WIDTH Vid_width

#include "blit.h"

#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef INDIRECT
#undef BLIT_16BPP_HACK
}

void sdl_update_8_to_24bpp (struct osd_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned char
#define DEST_PIXEL unsigned int
#define PACK_BITS
#define DEST BLIT_PIXELS
#define DEST_WIDTH Vid_width
   if(current_palette->lookup)
   {
#define INDIRECT current_palette->lookup
#include "blit.h"
#undef INDIRECT
   }
   else
   {
#include "blit.h"
   }
#undef DEST_WIDTH
#undef DEST
#undef PACK_BITS
#undef DEST_PIXEL
#undef SRC_PIXEL
}

void sdl_update_8_to_32bpp (struct osd_bitmap *bitmap)
{
#define INDIRECT current_palette->lookup
#define SRC_PIXEL  unsigned char
#define DEST_PIXEL unsigned int
#define DEST BLIT_PIXELS
#define DEST_WIDTH Vid_width
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef INDIRECT
}

void sdl_update_16_to_16bpp (struct osd_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned short
#define DEST BLIT_PIXELS
#define DEST_WIDTH Vid_width
   if(current_palette->lookup)
   {
#define INDIRECT current_palette->lookup
#include "blit.h"
#undef INDIRECT
   }
   else
   {
#include "blit.h"
   }
#undef DEST
#undef DEST_WIDTH
#undef SRC_PIXEL
#undef DEST_PIXEL
}

void sdl_update_16_to_24bpp (struct osd_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned int
#define PACK_BITS
#define DEST BLIT_PIXELS
#define DEST_WIDTH Vid_width
   if(current_palette->lookup)
   {
#define INDIRECT current_palette->lookup
#include "blit.h"
#undef INDIRECT
   }
   else
   {
#include "blit.h"
   }
#undef DEST_WIDTH
#undef DEST
#undef PACK_BITS
#undef DEST_PIXEL
#undef SRC_PIXEL
}

void sdl_update_16_to_32bpp (struct osd_bitmap *bitmap)
{
#define INDIRECT current_palette->lookup
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned int
#define DEST BLIT_PIXELS
#define DEST_WIDTH Vid_width
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef INDIRECT
}

void sdl_update_rgb_direct_32bpp(struct osd_bitmap *bitmap)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned int
#define DEST BLIT_PIXELS
#define DEST_WIDTH Vid_width
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
}

void sysdep_update_display(struct osd_bitmap *bitmap)
{
#ifdef OFFSCREEN
   int old_use_dirty = use_dirty;
   SDL_Rect srect = { 0,0,0,0 };
   SDL_Rect drect = { 0,0,0,0 };
   srect.w = Vid_width;
   srect.h = Vid_height;

   /* Center the display */
   drect.x = (Vid_width - visual_width*widthscale ) >> 1;
   drect.y = (Vid_height - visual_height*heightscale ) >> 1;

   drect.w = Vid_width;
   drect.h = Vid_height;

   if (current_palette->lookup_dirty)
      use_dirty = 0;
   
   (*update_function)(bitmap);

   
   if(SDL_BlitSurface (Offscreen_surface, &srect, Surface, &drect)<0) 
      fprintf (stderr,"SDL: Warn: Unsuccessful blitting\n");

   if(hardware==0)
      SDL_UpdateRects(Surface,1, &drect);
   use_dirty = old_use_dirty;
#else
   int old_use_dirty = use_dirty;

   if (current_palette->lookup_dirty)
      use_dirty = 0;

   (*update_function)(bitmap);

   //if (doublebuf == 1)	// Assist with ports, always call SDL_Flip();
      SDL_Flip(Surface);

   use_dirty = old_use_dirty;
#endif
}

/* shut up the display */
void sysdep_display_close(void)
{
#ifdef OFFSCREEN
   SDL_FreeSurface(Offscreen_surface);
#endif

   /* Restore cursor state */
   SDL_ShowCursor(cursor_state);
}

/*
 * In 8 bpp we should alloc pallete - some ancient people  
 * are still using 8bpp displays
 */
int sysdep_display_alloc_palette(int totalcolors)
{
   int ncolors;
   int i;
   ncolors = totalcolors;

   fprintf (stderr, "SDL: sysdep_display_alloc_palette(%d);\n",totalcolors);
   if (Vid_depth != 8)
      return 0;

   Colors = (SDL_Color*) malloc (totalcolors * sizeof(SDL_Color));
   if( !Colors )
      return 1;
   for (i=0;i<totalcolors;i++) {
      (Colors + i)->r = 0xFF;
      (Colors + i)->g = 0x00;
      (Colors + i)->b = 0x00;
   }
   SDL_SetColors (BLIT_SURFACE,Colors,0,totalcolors-1);

   fprintf (stderr, "SDL: Info: Palette with %d colors allocated\n", totalcolors);
   return 0;
}

int sysdep_display_set_pen(int pen,unsigned char red, unsigned char green, unsigned char blue)
{
   static int warned = 0;
#ifdef SDL_DEBUG
   fprintf(stderr,"sysdep_display_set_pen(%d,%d,%d,%d)\n",pen,red,green,blue);
#endif

   if( Colors ) {
      (Colors + pen)->r = red;
      (Colors + pen)->g = green;
      (Colors + pen)->b = blue;
      if ( (! SDL_SetColors(BLIT_SURFACE, Colors + pen, pen,1)) && (! warned)) {
         printf ("Color allocation failed, or > 8 bit display\n");
         warned = 0;
      }
   }

#ifdef SDL_DEBUG
   fprintf(stderr, "STD: Debug: Pen %d modification: r %d, g %d, b, %d\n", pen, red,green,blue);
#endif /* SDL_DEBUG */
   return 0;
}

void sysdep_mouse_poll (void)
{
   int i;
   int x,y;
   Uint8 buttons;

   buttons = SDL_GetRelativeMouseState( &x, &y);
   mouse_data[0].deltas[0] = x;
   mouse_data[0].deltas[1] = y;
   for(i=0;i<MOUSE_BUTTONS;i++) {
      mouse_data[0].buttons[i] = buttons & (0x01 << i);
   }
}

/* Keyboard procs */
/* Lighting keyboard leds */
void sysdep_set_leds(int leds) 
{
}

void sysdep_update_keyboard() 
{
   struct keyboard_event kevent;
   SDL_Event event;
   
   if (Surface) {
      while(SDL_PollEvent(&event)) {
         kevent.press = 0;
         
         switch (event.type)
         {
            case SDL_KEYDOWN:
               kevent.press = 1;

               /* ALT-Enter: toggle fullscreen */
               if ( event.key.keysym.sym == SDLK_RETURN )
               {
                  if(event.key.keysym.mod & KMOD_ALT)
                     SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
               }

            case SDL_KEYUP:
               kevent.scancode = klookup[event.key.keysym.sym];
               kevent.unicode = event.key.keysym.unicode;
               keyboard_register_event(&kevent);
               if(!kevent.scancode)
                  fprintf (stderr, "Unknown symbol 0x%x\n",
                     event.key.keysym.sym);
#ifdef SDL_DEBUG
               fprintf (stderr, "Key %s %ssed\n",
                  SDL_GetKeyName(event.key.keysym.sym),
                  kevent.press? "pres":"relea");
#endif
               break;
            case SDL_QUIT:
               /* Shoult leave this to application */
               exit(OSD_OK);
               break;

#ifdef SDL_JOYSTICK
            case SDL_JOYAXISMOTION:
               if (event.jaxis.which < JOY_AXIS)
                  joy_data[event.jaxis.which].axis[event.jaxis.axis].val = event.jaxis.value;
#ifdef SDL_DEBUG
               fprintf (stderr,"Axis=%d,value=%d\n",event.jaxis.axis ,event.jaxis.value);
#endif
                break;
            case SDL_JOYBUTTONDOWN:

            case SDL_JOYBUTTONUP:
               if (event.jbutton.which < JOY_BUTTONS)
                  joy_data[event.jbutton.which].buttons[event.jbutton.button] = event.jbutton.state;
#ifdef SDL_DEBUG
               fprintf (stderr, "Button=%d,value=%d\n",event.jbutton.button ,event.jbutton.state);
#endif
                break;
#endif

            default:
#ifdef SDL_DEBUG
               fprintf(stderr, "SDL: Debug: Other event\n");
#endif /* SDL_DEBUG */
               break;
         }
      }
   }
}

/* added funcions */
int sysdep_display_16bpp_capable(void)
{
   const SDL_VideoInfo* video_info;
   video_info = SDL_GetVideoInfo();
   return ( video_info->vfmt->BitsPerPixel >=16);
}

void list_sdl_modes(void)
{
   SDL_Rect** vid_modes;
   int vid_modes_i;

   vid_modes = SDL_ListModes(NULL,SDL_FULLSCREEN);
   vid_modes_i = 0;

   if ( (! vid_modes) || ((long)vid_modes == -1)) {
      printf("This option only works in a full-screen mode (eg: linux's framebuffer)\n");
      return;
   }

   printf("Modes availables:\n");

   while( *(vid_modes+vid_modes_i) ) {
      printf("\t%d) Mode %d x %d\n",
         vid_modes_i,
         (*(vid_modes+vid_modes_i))->w,
         (*(vid_modes+vid_modes_i))->h
         );
   
      vid_modes_i++;
   }
}
