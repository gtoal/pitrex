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

***************************************************************************/
#define __SDL_C

#undef SDL_DEBUG

#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <SDL/SDL.h>
#include "xmame.h"
#include "devices.h"
#include "keyboard.h"
#include "driver.h"
#include "SDL-keytable.h"

/* For DispmanX/GLES2 */
#include <assert.h>
#include <bcm_host.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "dispmanx_gles2.h"

#define DISPMANX_WIDTH	640
#define DISPMANX_HEIGHT	480
#define MAXCOLORS_8BIT	256

/* TO use 8-bit textures, define USE_8BIT in the Makefile */

static int vsync;
static int smooth;
static int display_border;
static int Vid_width;
static int Vid_height;
static int Vid_depth;
static void *screen_buffer;
static unsigned short *pi_palette;
static int cursor_state; /* previous mouse cursor state */
static SDL_Surface* sdlscreen;

typedef void (*update_func_t)(struct osd_bitmap *bitmap);

update_func_t update_function;

static int sdl_mapkey(struct rc_option *option, const char *arg, int priority);
static void exitfunc();
static void dispmanx_init(void);
static void dispmanx_deinit(void);
static void dispmanx_set_video_mode(int width,int height, int depth);
static void DisplayScreen(struct osd_bitmap *bitmap);

struct rc_option display_opts[] = {
    /* name, shortname, type, dest, deflt, min, max, func, help */
   { "DispmanX Related",  NULL,    rc_seperator,  NULL,
       NULL,         0,       0,             NULL,
       NULL },
#ifdef RASPI
   { "vsync",       NULL,    rc_bool,       &vsync,
      "1",           0,       0,             NULL,
      "Use vsync to reduce flicker/tearing" },
   { "smooth",      NULL,    rc_bool,       &smooth,
      "1",           0,       0,             NULL,
      "Use smooth scaling" },
   { "display_border",NULL,  rc_int,        &display_border,
      "0",           0,       64,            NULL,
      "Display Border size scaling." },
#endif
   { "sdlmapkey",	"sdlmk",	rc_use_function,	NULL,
     NULL,		0,			0,		sdl_mapkey,
     "Set a specific key mapping, see xmamerc.dist" },
   { NULL,           NULL,    rc_end,        NULL,
      NULL,          0,       0,             NULL,
      NULL }
};


void sdl_update_8_to_8bpp (struct osd_bitmap *bitmap);
void sdl_update_8_to_16bpp (struct osd_bitmap *bitmap);
void sdl_update_16_to_16bpp(struct osd_bitmap *bitmap);

int sysdep_init(void)
{
#ifdef DISPMANX_DEBUG
    printf ("dispmanx: sysdep_init()\n");
#endif
    //Initialise dispmanx
    bcm_host_init();

    //Clean exits, hopefully!
    atexit(exitfunc);

    return OSD_OK;
}

void sysdep_close(void)
{
#ifdef DISPMANX_DEBUG
    printf ("dispmanx: sysdep_close()\n");
#endif
    bcm_host_deinit();
    SDL_Quit();
}

int sysdep_create_display(int visual_width, int visual_height, int depth)
{
	static int init_once = 1;

#ifdef DISPMANX_DEBUG
    printf ("dispmanx: sysdep_create_display %dx%d %dbpp\n", visual_width, visual_height, depth);
#endif

	/* These are used by the blitter and palette information */
	Vid_height = visual_height*heightscale;
	Vid_width = visual_width*widthscale;
	Vid_depth = 16;

	/* Only do this once, xMAME in theory can call this function multiple times, though very unlikely */
	if (init_once)
	{
		/* Define blitter */
		if( depth == 8 )
		{
#ifdef USE_8BIT
			update_function = &sdl_update_8_to_8bpp;
#else
			update_function = &sdl_update_8_to_16bpp;
#endif
		}
		else if( depth == 16 )
		{
			update_function = &sdl_update_16_to_16bpp;
		}
		else
		{
			fprintf (stderr, "Game requires unsupported depth=%d\n", depth);
			exit (OSD_NOT_OK);
		}

		/* Need this so that basic SDL functionality will work
		 *
		 * Seeing as we need to do this, we could use this buffer for the blit buffer
		 */
		sdlscreen = SDL_SetVideoMode(0,0, 16, SDL_SWSURFACE);

		/* Creating event mask */
		SDL_EventState(SDL_KEYUP, SDL_ENABLE);
		SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
		SDL_EnableUNICODE(1);
	   
		/* dispmanx init */
		dispmanx_init();

		/* fill the display_palette_info struct */
		memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));

#ifdef USE_8BIT
		/* dispmanx setup */
		dispmanx_set_video_mode(visual_width,visual_height,depth);
#else
		/* dispmanx setup */
		dispmanx_set_video_mode(visual_width,visual_height,Vid_depth/* always 16-bit */);
#endif
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


		init_once = 0;
	}

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
      /* printf("trying to map %x to %x\n", from, to); */
      if (from >= SDLK_FIRST && from < SDLK_LAST && to >= 0 && to <= 127)
      {
         klookup[from] = to;
	 return OSD_OK;
      }
      /* stderr_file isn't defined yet when we're called. */
      printf("Invalid keymapping %s. Ignoring...\n", arg);
   }
   return OSD_NOT_OK;
}

/* Update routines */
void sdl_update_8_to_8bpp (struct osd_bitmap *bitmap)
{
#define DEST_WIDTH Vid_width
#define DEST screen_buffer
#define SRC_PIXEL unsigned char
#define DEST_PIXEL unsigned char

#include "blit_dispmanx.h"

#undef DEST_PIXEL
#undef SRC_PIXEL
#undef DEST
#undef DEST_WIDTH
}

void sdl_update_8_to_16bpp(struct osd_bitmap *bitmap)
{
//#define BLIT_16BPP_HACK
#define INDIRECT current_palette->lookup
#define SRC_PIXEL  unsigned char
#define DEST_PIXEL unsigned short
#define DEST screen_buffer
#define DEST_WIDTH Vid_width

#include "blit_dispmanx.h"

#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef INDIRECT
//#undef BLIT_16BPP_HACK
}

void sdl_update_16_to_16bpp (struct osd_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned short
#define DEST screen_buffer
#define DEST_WIDTH Vid_width
   if(current_palette->lookup)
   {
#define INDIRECT current_palette->lookup
#include "blit_dispmanx.h"
#undef INDIRECT
   }
   else
   {
#include "blit_dispmanx.h"
   }
#undef DEST
#undef DEST_WIDTH
#undef SRC_PIXEL
#undef DEST_PIXEL
}


void sysdep_update_display(struct osd_bitmap *bitmap)
{
#ifdef DISPMANX_DEBUG
    printf ("dispmanx: sysdep_update_display()\n");
#endif
    int old_use_dirty = use_dirty;

    if (current_palette->lookup_dirty)
	use_dirty = 0;

    (*update_function)(bitmap);

    DisplayScreen(bitmap);

    use_dirty = old_use_dirty;
}

/* shut up the display */
void sysdep_display_close(void)
{
#ifdef DISPMANX_DEBUG
    printf ("dispmanx: sysdep_display_close()\n");
#endif
    if(sdlscreen)
    {
		SDL_FreeSurface(sdlscreen);
		sdlscreen = NULL;
    }
    dispmanx_deinit();

    if(pi_palette)
    {
		free(pi_palette);
		pi_palette=0;
    }
}


int sysdep_display_alloc_palette(int totalcolors)
{
#ifdef DISPMANX_DEBUG
   printf ("dispmanx: sysdep_alloc_palette()\n");
#endif
   pi_palette=(unsigned short *)memalign(32,MAXCOLORS_8BIT*sizeof(unsigned short));
   memset(pi_palette, 0, MAXCOLORS_8BIT*sizeof(unsigned short));
   return 0;
}


int sysdep_display_set_pen(int pen,unsigned char red, unsigned char green, unsigned char blue)
{
#ifdef DISPMANX_DEBUG
    printf("sysdep_display_set_pen(%d:%2.2x%2.2x%2.2x)\n", pen, red, green, blue);
#endif
	if (pi_palette)
	{
		if (pen < MAXCOLORS_8BIT)
		{
		    pi_palette[pen]=(((red>>3)&0x1f) << 11) | (((green>>2)&0x3f) << 5 ) | ((blue>>3)&0x1f);
		    gles2_palette_changed();
		}
		else
		{
		    fprintf(stderr, "ERROR: Rasp Pi Palette out of range: sysdep_display_set_pen(%d:%2.2x%2.2x%2.2x)\n", pen, red, green, blue);
		}
	}
	else
	{
		fprintf(stderr, "ERROR: Rasp Pi Palette not yet init: sysdep_display_set_pen(%d:%2.2x%2.2x%2.2x)\n", pen, red, green, blue);
	}

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
   
   if (sdlscreen) {
      while(SDL_PollEvent(&event)) {
         kevent.press = 0;
         
         switch (event.type)
         {
            case SDL_KEYDOWN:
               kevent.press = 1;

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
               printf( "SDL: Debug: Other event\n");
#endif /* SDL_DEBUG */
               break;
         }
      }
   }
}

int sysdep_display_16bpp_capable(void)
{
   /* Should check to see if Raspberry Pi is currently in 8-BPP mode */
   return 1;
}


/******************/
/*                */
/* Start dispmanx */
/*                */
/******************/

// create two resources for 'page flipping'
static DISPMANX_RESOURCE_HANDLE_T   resource0;
static DISPMANX_RESOURCE_HANDLE_T   resource_bg;

DISPMANX_ELEMENT_HANDLE_T dispman_element;
DISPMANX_ELEMENT_HANDLE_T dispman_element_bg;
DISPMANX_DISPLAY_HANDLE_T dispman_display;
DISPMANX_UPDATE_HANDLE_T dispman_update;

EGLDisplay display = NULL;
EGLSurface surface = NULL;
static EGLContext context = NULL;
static EGL_DISPMANX_WINDOW_T nativewindow;

static uint32_t display_adj_width, display_adj_height;          // display size minus border
static int surface_width;
static int surface_height;


static void exitfunc()
{
    bcm_host_deinit();
    SDL_Quit();
}


static void dispmanx_init(void)
{
    int ret;
   
    uint32_t display_width, display_height;

#ifdef DISPMANX_DEBUG
    printf( "dispmanx_init()\n");
#endif /* DISPMANX_DEBUG */

    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    graphics_get_display_size(0 /* LCD */, &display_width, &display_height);

    dispman_display = vc_dispmanx_display_open( 0 );
    assert( dispman_display != 0 );
   
    // Add border around bitmap for TV
    display_width -= display_border * 2;
    display_height -= display_border * 2;

    //Create two surfaces for flipping between
    //Make sure bitmap type matches the source for better performance
    uint32_t unused;
    resource0 = vc_dispmanx_resource_create(VC_IMAGE_RGB565, DISPMANX_WIDTH, DISPMANX_HEIGHT, &unused);

    vc_dispmanx_rect_set( &dst_rect, display_border, display_border,
		     display_width, display_height);
    vc_dispmanx_rect_set( &src_rect, 0, 0, DISPMANX_WIDTH << 16, DISPMANX_HEIGHT << 16);

    //Make sure mame and background overlay the menu program
    dispman_update = vc_dispmanx_update_start( 0 );

    // create the 'window' element - based on the first buffer resource (resource0)
    dispman_element = vc_dispmanx_element_add(  dispman_update,
					  dispman_display,
					  1,
					  &dst_rect,
					  resource0,
					  &src_rect,
					  DISPMANX_PROTECTION_NONE,
					  0,
					  0,
					  (DISPMANX_TRANSFORM_T) 0 );

    ret = vc_dispmanx_update_submit_sync( dispman_update );
}



static void dispmanx_deinit(void)
{
    int ret;

#ifdef DISPMANX_DEBUG
    printf( "dispmanx_deinit()\n");
#endif /* DISPMANX_DEBUG */
    gles2_destroy();

    // Release OpenGL resources
    eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( display, surface );
    eglDestroyContext( display, context );
    eglTerminate( display );

	// Release DispmanX resource
    dispman_update = vc_dispmanx_update_start( 0 );
    ret = vc_dispmanx_element_remove( dispman_update, dispman_element );
    ret = vc_dispmanx_element_remove( dispman_update, dispman_element_bg );
    ret = vc_dispmanx_update_submit_sync( dispman_update );
    ret = vc_dispmanx_resource_delete( resource0 );
    ret = vc_dispmanx_resource_delete( resource_bg );
    ret = vc_dispmanx_display_close( dispman_display );

	// Release screen buffer
    if(screen_buffer)
    {
		free(screen_buffer);
		screen_buffer = 0;
    }
}


static void dispmanx_set_video_mode(int width,int height, int depth)
{
    int ret;
    uint32_t display_width, display_height;
    uint32_t display_x=0, display_y=0;
    float display_ratio,game_ratio;

    printf( "dispmanx_set_video_mode(%d, %d, %d)\n", width, height, depth);

    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;

    surface_width = width;
    surface_height = height;

    assert(depth==8 || depth==16);
    screen_buffer=(void *)memalign(32, width*height*(depth>>3));
    memset(screen_buffer, 0, width*height*(depth>>3));

    // get an EGL display connection
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(display != EGL_NO_DISPLAY);

    // initialize the EGL display connection
    EGLBoolean result = eglInitialize(display, NULL, NULL);
    assert(EGL_FALSE != result);

    // get an appropriate EGL frame buffer configuration
    EGLint num_config;
    EGLConfig config;
    static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
    result = eglChooseConfig(display, attribute_list, &config, 1, &num_config);
    assert(EGL_FALSE != result);

    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);

    // create an EGL rendering context
    static const EGLint context_attributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
    assert(context != EGL_NO_CONTEXT);

    // create an EGL window surface
    int32_t success = graphics_get_display_size(0, &display_width, &display_height);
    assert(success >= 0);

    display_adj_width = display_width - (display_border * 2);
    display_adj_height = display_height - (display_border * 2);

    if (smooth)
    {
	    //We use the dispmanx scaler to smooth stretch the display
	    //so GLES2 doesn't have to handle the performance intensive postprocessing

		uint32_t sx, sy;

	    // Work out the position and size on the display
	    display_ratio = (float)display_width/(float)display_height;
	    game_ratio = (float)width/(float)height;
     
	    display_x = sx = display_adj_width;
	    display_y = sy = display_adj_height;

	    if (game_ratio>display_ratio)
		    sy = (float)display_adj_width/(float)game_ratio;
	    else
		    sx = (float)display_adj_height*(float)game_ratio;
     
	    // Centre bitmap on screen
	    display_x = (display_x - sx) / 2;
	    display_y = (display_y - sy) / 2;
   
	    vc_dispmanx_rect_set( &dst_rect,
				display_x + display_border,
				display_y + display_border,
				sx, sy);
	    vc_dispmanx_rect_set( &src_rect, 0, 0, width << 16, height << 16);
    }
    else
	{
	    vc_dispmanx_rect_set( &dst_rect,
				display_border,
				display_border,
				display_adj_width, display_adj_height);
	    vc_dispmanx_rect_set( &src_rect, 0, 0, display_adj_width << 16, display_adj_height << 16);
	};

/*
 For reference:
DISPMANX_TRANSFORM_T =
    enum(
      :DISPMANX_NO_ROTATE,  0,
      :DISPMANX_ROTATE_90,  1,
      :DISPMANX_ROTATE_180, 2,
      :DISPMANX_ROTATE_270, 3,
      :DISPMANX_FLIP_HRIZ,  1 << 16,
      :DISPMANX_FLIP_VERT,  1 << 17
    )
*/

    dispman_display = vc_dispmanx_display_open(0);
    dispman_update = vc_dispmanx_update_start(0);
    dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
				    10, &dst_rect, 0, &src_rect,
				    DISPMANX_PROTECTION_NONE, NULL, NULL, DISPMANX_NO_ROTATE);

    //Black background surface dimensions
    vc_dispmanx_rect_set( &dst_rect, 0, 0, display_width, display_height );
    vc_dispmanx_rect_set( &src_rect, 0, 0, 128 << 16, 128 << 16);

    //Create a blank background for the whole screen, make sure width is divisible by 32!
    uint32_t unused;
    resource_bg = vc_dispmanx_resource_create(VC_IMAGE_RGB565, 128, 128, &unused);
    dispman_element_bg = vc_dispmanx_element_add(  dispman_update, dispman_display,
					  9, &dst_rect, resource_bg, &src_rect,
					  DISPMANX_PROTECTION_NONE, 0, 0,
					  (DISPMANX_TRANSFORM_T) 0 );

    nativewindow.element = dispman_element;
    if (smooth) {
	    nativewindow.width = width;
	    nativewindow.height = height;
    }
    else {
	    nativewindow.width = display_adj_width;
	    nativewindow.height = display_adj_height;
    }

    vc_dispmanx_update_submit_sync(dispman_update);

    surface = eglCreateWindowSurface(display, config, &nativewindow, NULL);
    assert(surface != EGL_NO_SURFACE);

    // connect the context to the surface
    result = eglMakeCurrent(display, surface, surface, context);
    assert(EGL_FALSE != result);

    //Smooth stretch the display size for GLES2 is the size of the bitmap
    //otherwise let GLES2 upscale (NEAR) to the size of the display
    if (smooth)
	    gles2_create(width, height, width, height, depth);
    else
	    gles2_create(display_adj_width, display_adj_height, width, height, depth);

    // Set vsync mode
    if(vsync)
	    eglSwapInterval(display, 1);
    else
	    eglSwapInterval(display, 0);
}


static void DisplayScreen(struct osd_bitmap *bitmap)
{
    //Draw to the screen
    gles2_draw(screen_buffer, surface_width, surface_height, bitmap->depth, (unsigned short *)current_palette->lookup);
    eglSwapBuffers(display, surface);
}
