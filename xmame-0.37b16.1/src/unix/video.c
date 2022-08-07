/*
 * X-Mame generic video code
 *
 */
#define __VIDEO_C_
#include <math.h>
#include "xmame.h"

#ifdef xgl
	#include "video-drivers/glmame.h"
#endif

#include "driver.h"
#include "profiler.h"
#include "input.h"
#include "keyboard.h"
/* for uclock */
#include "sysdep/misc.h"

#define FRAMESKIP_DRIVER_COUNT 2

static const int safety = 16;
static float beam_f;
static int normal_widthscale = 1, normal_heightscale = 1;
static char *vector_res = NULL;
static int use_auto_double = 1;
static int frameskipper = 0;
static int brightness = 100;
static float brightness_paused_adjust = 1.0;
static int bitmap_depth;
static struct osd_bitmap *scrbitmap = NULL;
static int debugger_has_focus = 0;
/*struct sysdep_palette_struct *debug_palette = NULL;*/
static struct my_rectangle normal_visual;
static struct my_rectangle debug_visual;

float gamma_correction = 1.0;

#ifdef RASPI
#include "osinline.h"
/* average FPS calculation */
static INT64 start_time = 0;
static INT64 end_time;
static int frames_displayed;
static int frames_to_display;
int use_packed_gfx;
extern UINT8 trying_to_quit;
#endif


#ifdef xgl
	static int bitmap4GLTexture = 0;
#endif

/* some prototypes */
static int video_handle_scale(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_beam(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_flicker(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_bpp(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_vectorres(struct rc_option *option, const char *arg,
   int priority);
static void osd_free_colors(void);

struct rc_option video_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Video Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "bpp",		"b",			rc_int,		&options.color_depth,
     "0",		0,			0,		video_verify_bpp,
     "Specify the colordepth the core should render, one of: auto(0), 8, 16" },
   { "heightscale",	"hs",			rc_int,		&normal_heightscale,
     "1",		1,			8,		NULL,
     "Set Y-Scale aspect ratio" },
   { "widthscale",	"ws",			rc_int,		&normal_widthscale,
     "1",		1,			8,		NULL,
     "Set X-Scale aspect ratio" },
   { "scale",		"s",			rc_use_function, NULL,
     NULL,		0,			0,		video_handle_scale,
     "Set X-Y Scale to the same aspect ratio. For vector games scale (and also width- and heightscale) may have value's like 1.5 and even 0.5. For scaling of regular games this will be rounded to an int" },
   { "autodouble",	"adb",			rc_bool,	&use_auto_double,
     "1",		0,			0,		NULL,
     "Enable/disable automatic scale doubling for 1:2 pixel aspect ratio games" },
   { "dirty",		"dt",			rc_bool,	&use_dirty,
     "1",		0,			0,		NULL,
     "Enable/disable use of dirty rectangles" },
   { "scanlines",	"sl",			rc_bool,	&use_scanlines,
     "0",		0,			0,		NULL,
     "Enable/disable displaying simulated scanlines" },
   { "artwork",		"a",			rc_bool,	&options.use_artwork,
     "1",		0,			0,		NULL,
     "Use/don't use artwork if available" },
   { "frameskipper",	"fsr",			rc_int,		&frameskipper,
     "0",		0,			FRAMESKIP_DRIVER_COUNT-1, NULL,
     "Select which autoframeskip and throttle routines to use. Available choices are:\n0 Dos frameskip code\n1 Enhanced frameskip code by William A. Barath" },
   { "throttle",	"th",			rc_bool,	&throttle,
     "1",		0,			0,		NULL,
     "Enable/disable throttle" },
   { "sleepidle",	"si",			rc_bool,	&sleep_idle,
     "0",		0,			0,		NULL,
     "Enable/disable sleep during idle" },
   { "autoframeskip",	"afs",			rc_bool,	&autoframeskip,
     "1",		0,			0,		NULL,
     "Enable/disable autoframeskip" },
   { "maxautoframeskip", "mafs",		rc_int,		&max_autoframeskip,
     "8",		0,			FRAMESKIP_LEVELS-1, NULL,
     "Set highest allowed frameskip for autoframeskip" },
   { "frameskip",	"fs",			rc_int,		&frameskip,
     "0",		0,			FRAMESKIP_LEVELS-1, NULL,
     "Set frameskip when not using autoframeskip" },
   { "brightness",	"brt",			rc_int,		&brightness,
     "100",		0,			100,		NULL,
     "Set the brightness (0-100%%)" },
   { "gamma-correction", "gc",			rc_float,	&gamma_correction,
     "1.0",		0.5,			2.0,		NULL,
     "Set the gamma-correction (0.5-2.0)" },
   { "norotate",	"nr",			rc_set_int,	&options.norotate,
     NULL,		1,			0,		NULL,
     "Disable rotation" },
   { "ror",		"rr",			rc_set_int,	&options.ror,
     NULL,		1,			0,		NULL,
     "Rotate display 90 degrees rigth" },
   { "rol",		"rl",			rc_set_int,	&options.rol,
     NULL,		1,			0,		NULL,
     "Rotate display 90 degrees left" },
   { "flipx",		"fx",			rc_set_int,	&options.flipx,
     NULL,		1,			0,		NULL,
     "Flip X axis" },
   { "flipy",		"fy",			rc_set_int,	&options.flipy,
     NULL,		1,			0,		NULL,
     "Flip Y axis" },
#ifdef RASPI
   { "frames_to_run",   "ftr",                  rc_int,         &frames_to_display,
    "0",               0,                      0,              NULL,
    "Sets the number of frames to run within the game" },
   { "packed_gfx",      "pgfx",                 rc_bool,        &use_packed_gfx,
    "0",               0,                      0,              NULL,
    "Used packed graphics for storing ROM graphic regions" },
#endif
   { "Vector Games Related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "vectorres",	"vres",			rc_string,	&vector_res,
     NULL,		0,			0,		video_verify_vectorres,
     "Always scale vectorgames to XresxYres, keeping their aspect ratio. This overrides the scale options" },
   { "beam",		"B",			rc_float,	&beam_f,
     "1.0",		1.0,			16.0,		video_verify_beam,
     "Set the beam size for vector games" },
   { "flicker",		"f",			rc_float,	&options.vector_flicker,
     "0.0",		0.0,			100.0,		video_verify_flicker,
     "Set the flicker for vector games" },
   { "antialias",	"aa",			rc_bool,	&options.antialias,
     "1",		0,			0,		NULL,
     "Enable/disable antialiasing" },
   { "translucency",	"t",			rc_bool,	&options.translucency,
     "1",		0,			0,		NULL,
     "Enable/disable tranlucency" },
   { NULL,		NULL,			rc_link,	display_opts,
     NULL,		0,			0,		NULL,
     NULL },     
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

static int video_handle_scale(struct rc_option *option, const char *arg,
   int priority)
{
   if (rc_set_option2(video_opts, "widthscale", arg, priority))
      return -1;
   if (rc_set_option2(video_opts, "heightscale", arg, priority))
      return -1;
      
   option->priority = priority;
   
   return 0;
}

static int video_verify_beam(struct rc_option *option, const char *arg,
   int priority)
{
   options.beam = (int)(beam_f * 0x00010000);
   if (options.beam < 0x00010000)
      options.beam = 0x00010000;
   else if (options.beam > 0x00100000)
      options.beam = 0x00100000;

   option->priority = priority;
   
   return 0;
}

static int video_verify_flicker(struct rc_option *option, const char *arg,
   int priority)
{
   if (options.vector_flicker < 0.0)
      options.vector_flicker = 0.0;
   else if (options.vector_flicker > 100.0)
      options.vector_flicker = 100.0;

   option->priority = priority;
   
   return 0;
}

static int video_verify_bpp(struct rc_option *option, const char *arg,
   int priority)
{
   if( (options.color_depth != 0) &&
       (options.color_depth != 8) &&
       (options.color_depth != 15) &&
       (options.color_depth != 16) &&
       (options.color_depth != 32) )
   {
      options.color_depth = 0;
      fprintf(stderr, "error: invalid value for bpp: %s\n", arg);
      return -1;
   }

   option->priority = priority;
   
   return 0;
}

static int video_verify_vectorres(struct rc_option *option, const char *arg,
   int priority)
{
   if(sscanf(arg, "%dx%d", &options.vector_width, &options.vector_height) != 2)
   {
      options.vector_width = options.vector_height = 0;
      fprintf(stderr, "error: invalid value for vectorres: %s\n", arg);
      return -1;
   }

   option->priority = priority;
   
   return 0;
}

/* Create a bitmap. */
/* VERY IMPORTANT: the function must allocate also a "safety area" 16 pixels wide all */
/* around the bitmap. This is required because, for performance reasons, some graphic */
/* routines don't clip at boundaries of the bitmap. */
struct osd_bitmap *osd_alloc_bitmap(int width,int height,int depth)       /* ASG 980209 */
{
	struct osd_bitmap *bitmap;
#ifdef xgl
	int w,h;
#endif

	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		fprintf(stderr_file, "osd_alloc_bitmap() unknown depth %d\n", 
			depth);
		return NULL;
	}

	if ((bitmap = malloc(sizeof(struct osd_bitmap))) != 0)
	{
		unsigned char *bitmap_data=0;
		int i,rowlen,rdwidth, bytes_per_pixel;
		int y_rows;
		int bitmap_size;

		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		rdwidth = (width + 7) & ~7;     /* round width to a quadword */

		bytes_per_pixel=(depth+7)/8;

		rowlen = bytes_per_pixel * (rdwidth + 2 * safety) * sizeof(unsigned char);
		
		y_rows = height + 2 * safety;

		bitmap_size = y_rows * rowlen ;

#ifdef xgl
		if(bitmap4GLTexture) 
		{
			int tw_size, th_rows;

			/* JAU: find some n,m for 
				(w=2**n)>=text_width, (h=2**m)>=text_height */
			/* we try to use the bitmap for OpenGL 
			   textures directly .. no copies */

			text_width  = width;
			text_height = height;
			w=1; h=1;

			while (w<text_width)
			{ w*=2; }

			while (h<text_height)
			{ h*=2; }

			text_width=w;
			text_height=h;

			/* The rows must not be expanded, just the bitmap
			   memory - because of read access :-)

			   Question: 
			     Is the bitmap allocated after osd_create_display
			     the only, which must be displayed as a texture ?
			     If not - work must be done ..
			 */
			
			tw_size  = bytes_per_pixel * 
			           (text_width + 2 * safety) ;

			th_rows = text_height + 2 * safety ;
			
			bitmap_size = th_rows * tw_size;

			bitmap4GLTexture = FALSE; /* thats it */

		}
#endif

		if ((bitmap_data = malloc(bitmap_size)) == 0)
		{
			free(bitmap);
			return 0;
		}
	
		/* clear ALL bitmap, including safety area, to avoid garbage on right */
		/* side of screen is width is not a multiple of 4 */
		memset(bitmap_data, 0, (height + 2 * safety) * rowlen);

		if ((bitmap->line = malloc((height + 2 * safety) * sizeof(unsigned char *))) == 0)
		{
		    free(bitmap_data);
		    free(bitmap);
		    return 0;
		}

		for (i = 0; i < height + 2 * safety; i++)
		{
		    bitmap->line[i] = &bitmap_data[i * rowlen + safety * bytes_per_pixel];
		}

		bitmap->line += safety;

		bitmap->_private = bitmap_data ;
	}

	return bitmap;
}

void osd_free_bitmap(struct osd_bitmap *bitmap)
{
	if (bitmap)
	{
		bitmap->line -= safety;
		free(bitmap->line);
		free(bitmap->_private);
		free(bitmap);
		bitmap = NULL;
	}
}

int osd_create_display(int width, int height, int depth,
   int fps, int attributes, int orientation)
{
   bitmap_depth = depth;
   if (bitmap_depth == 15) bitmap_depth = 16;

   current_palette = normal_palette = NULL;
   debug_visual.min_x = 0;
   debug_visual.max_x = options.debug_width - 1;
   debug_visual.min_y = 0;
   debug_visual.max_y = options.debug_height - 1;

   /* Can we do dirty? */
   if ( (attributes & VIDEO_SUPPORTS_DIRTY) == 0 )
   {
      use_dirty = FALSE;
   }
   
   if(use_auto_double)
   {
      if ( (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) ==
         VIDEO_PIXEL_ASPECT_RATIO_1_2)
      {
         if (orientation & ORIENTATION_SWAP_XY)
            normal_widthscale  *= 2;
         else
            normal_heightscale *= 2;
      }
      if ( (attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) ==
         VIDEO_PIXEL_ASPECT_RATIO_2_1)
      {
         if (orientation & ORIENTATION_SWAP_XY)
            normal_heightscale *= 2;
         else
            normal_widthscale  *= 2;
      }
   }
  
   if (osd_dirty_init()!=OSD_OK) return -1;

#ifdef xgl
   /* yes create a OpenGL-Texture compatible bitmap this time ... 
      the next call to alloc_bitmap uses this information ...
    */
   bitmap4GLTexture=1; 
#endif

   visual_width     = width;
   visual_height    = height;
   widthscale       = normal_widthscale;
   heightscale      = normal_heightscale;
   use_aspect_ratio = normal_use_aspect_ratio;
   video_fps        = fps;
   
#ifdef DISPMANX
   if (sysdep_create_display(visual_width, visual_height, bitmap_depth) != OSD_OK)
#else
   if (sysdep_create_display(bitmap_depth) != OSD_OK)
#endif
      return -1;
   
   /* a lott of display_targets need to have the display initialised before
      initialising any input devices */
   if (osd_input_initpost()!=OSD_OK) return -1;
   
   if (use_dirty) fprintf(stderr_file,"Using dirty_buffer strategy\n");
   if (bitmap_depth==16) fprintf(stderr_file,"Using 16bpp video mode\n");
   
   return 0;
}   

void osd_close_display (void)
{
   osd_free_colors();
   sysdep_display_close();
   osd_dirty_close ();
#ifdef RASPI
   /* print a final result to the stdout */
   if (frames_displayed != 0)
   {
      fprintf(stderr_file, "Average FPS: %f (%d frames)\n", (double)CLOCKS_PER_SEC / (end_time - start_time) * frames_displayed, frames_displayed);
   }
#endif
}

static void osd_change_display_settings(struct my_rectangle *new_visual,
   struct sysdep_palette_struct *new_palette, int new_widthscale,
   int new_heightscale, int new_use_aspect_ratio)
{
   int new_visual_width, new_visual_height, palette_dirty = 0;
   
   /* always update the visual info */
   visual = *new_visual;
   
   /* calculate the new visual width / height */
   new_visual_width  = visual.max_x - visual.min_x + 1;
   new_visual_height = visual.max_y - visual.min_y + 1;
   
   if( current_palette != new_palette )
   {
      current_palette = new_palette;
      palette_dirty = 1;
   }
   
   if( (visual_width     != new_visual_width    ) ||
       (visual_height    != new_visual_height   ) ||
       (widthscale       != new_widthscale      ) ||
       (heightscale      != new_heightscale     ) ||
       (use_aspect_ratio != new_use_aspect_ratio) )
   {
      sysdep_display_close();
      
      visual_width     = new_visual_width;
      visual_height    = new_visual_height;
      widthscale       = new_widthscale;
      heightscale      = new_heightscale;
      use_aspect_ratio = new_use_aspect_ratio;
      
#ifdef DISPMANX
	  if (sysdep_create_display(visual_width, visual_height, bitmap_depth) != OSD_OK)
#else
      if (sysdep_create_display(bitmap_depth) != OSD_OK)
#endif
      {
         /* oops this sorta sucks */
         fprintf(stderr_file, "Argh, resizing the display failed in osd_set_visible_area, aborting\n");
         exit(1);
      }
      
      /* only realloc the palette if it has been initialised */
      if(current_palette)
      {
         if (sysdep_display_alloc_palette(video_colors_used))
         {
            /* better restore the video mode before calling exit() */
            sysdep_display_close();
            /* oops this sorta sucks */
            fprintf(stderr_file, "Argh, (re)allocating the palette failed in osd_set_visible_area, aborting\n");
            exit(1);
         }
         palette_dirty = 1;
      }
      
      /* mark the current visual area of the bitmap dirty */
      osd_mark_dirty(visual.min_x, visual.min_y,
         visual.max_x, visual.max_y);
      
      /* to stop keys from getting stuck */
      keyboard_clear();
         
      /* for debugging only */
      fprintf(stderr_file, "viswidth = %d, visheight = %d,"
              "visstartx= %d, visstarty= %d\n",
               visual_width, visual_height, visual.min_x,
               visual.min_y);
   }
   
   if (palette_dirty && current_palette)
      sysdep_palette_mark_dirty(current_palette);
}

void osd_set_visible_area(int min_x,int max_x,int min_y,int max_y)
{
   normal_visual.min_x = min_x;
   normal_visual.max_x = max_x;
   normal_visual.min_y = min_y;
   normal_visual.max_y = max_y;

   /* round to 8, since the new dirty code works with 8x8 blocks,
      and we need to round to sizeof(long) for the long copies anyway */
   if (normal_visual.min_x & 7)
   {
      if((normal_visual.min_x - (normal_visual.min_x & ~7)) < 4)
         normal_visual.min_x &= ~7;
       else
         normal_visual.min_x = (normal_visual.min_x + 7) & ~7;
   }
   if ((normal_visual.max_x+1) & 7)
   {
      if(((normal_visual.max_x+1) - ((normal_visual.max_x+1) & ~7)) > 4)
         normal_visual.max_x = ((normal_visual.max_x+1 + 7) & ~7) - 1;
       else
         normal_visual.max_x = ((normal_visual.max_x+1) & ~7) - 1;
   }
   
   /* rounding of the y-coordinates is only nescesarry when we are doing dirty */
   if (use_dirty)
   {
      if (normal_visual.min_y & 7)
      {
         if((normal_visual.min_y - (normal_visual.min_y & ~7)) < 4)
            normal_visual.min_y &= ~7;
          else
            normal_visual.min_y = (normal_visual.min_y + 7) & ~7;
      }
      if ((normal_visual.max_y+1) & 7)
      {
         if(((normal_visual.max_y+1) - ((normal_visual.max_y+1) & ~7)) > 4)
            normal_visual.max_y = ((normal_visual.max_y+1 + 7) & ~7) - 1;
          else
            normal_visual.max_y = ((normal_visual.max_y+1) & ~7) - 1;
      }
   }
   
   if(!debugger_has_focus)
      osd_change_display_settings(&normal_visual, normal_palette,
         normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
   
   set_ui_visarea (normal_visual.min_x, normal_visual.min_y, normal_visual.max_x, normal_visual.max_y);
}

int osd_allocate_colors(unsigned int totalcolors, const unsigned char *palette,
   unsigned int *pens, int modifiable, const unsigned char *debugger_palette,
   unsigned int *debugger_pens)
{
   int i;
   int writable_colors = 0;
   int max_colors = (bitmap_depth == 8)? 256:65536;
   int direct_mapped_15bpp = 0;

   if ((Machine->drv->video_attributes & VIDEO_RGB_DIRECT) 
      && bitmap_depth == 16)
   {
      direct_mapped_15bpp = 1;
      totalcolors = 32768;
   }

   /* calculate the size of the normal palette */
   if (totalcolors > max_colors)
   {
      fprintf(stderr_file,
         "Warning: More than %d colors (%d) are needed for this emulation,\n"
         "some parts of the screen may be corrupted\n", max_colors, 
         totalcolors);
      /* fill the remainder of the pens array with 0's to make sure */
      /* nothing strange happens                                    */
      for (i=max_colors; i<totalcolors; i++)
         pens[i] = 0;
      totalcolors = max_colors;
   }
   else
      fprintf(stderr_file, "Game uses %d colors\n", totalcolors);
   
   if ((bitmap_depth == 8) || modifiable || direct_mapped_15bpp)
   {
      writable_colors = totalcolors + 2;
      if (writable_colors > max_colors)
         writable_colors = max_colors;
   }
   
   /* create the normal palette */
printf("sysdep_palette_create(depth=%d writable_colors=%d)\n", bitmap_depth, writable_colors);
   if(!(normal_palette = sysdep_palette_create(bitmap_depth,
      writable_colors)))
      return 1;
   
   video_colors_used = writable_colors;
   
   /* create the debug palette */
   if (debugger_pens)
   {
      i = (bitmap_depth == 8)? DEBUGGER_TOTAL_COLORS:0;
      
      if(!(debug_palette = sysdep_palette_create(bitmap_depth, i)))
      {
         osd_free_colors();
         return 1;
      }
      
      if (i > video_colors_used)
         video_colors_used = i;
   }
   
   /* now alloc the total number of colors used by both palettes */
printf("sysdep_display_alloc_palette(video_colors_used=%d)\n", video_colors_used);
   if(sysdep_display_alloc_palette(video_colors_used))
   {
      osd_free_colors();
      return 1;
   }
      
   sysdep_palette_set_gamma(normal_palette, gamma_correction);
   sysdep_palette_set_brightness(normal_palette, brightness * brightness_paused_adjust);
   if(debug_palette)
   {
      sysdep_palette_set_gamma(debug_palette, gamma_correction);
      sysdep_palette_set_brightness(debug_palette, brightness * brightness_paused_adjust);
   }
   
   /* init the palette */
   if (writable_colors)
   {
      int color_start = (totalcolors < max_colors)? 1:0;
      int r, g, b;
      
      /* normal palette */
      if ((Machine->drv->video_attributes & VIDEO_RGB_DIRECT) 
         && bitmap_depth == 16)
      {
         i = 0;

         for (r = 0;r < 32;r++)
         {
            for (g = 0;g < 32;g++)
            {
               for (b = 0;b < 32;b++)
               {
                  sysdep_palette_set_pen(normal_palette, i,
                     (r << 3) | (r >> 2),
                     (g << 3) | (g >> 2),
                     (b << 3) | (b >> 2));
                  i++;
               }
            }
         }

         pens[0] = 0x7c00;
         pens[1] = 0x03e0;
         pens[2] = 0x001f;

         Machine->uifont->colortable[0] = 0x0000;
         Machine->uifont->colortable[1] = 0x7fff;
         Machine->uifont->colortable[2] = 0x7fff;
         Machine->uifont->colortable[3] = 0x0000;

         /* debug palette */
         if (debugger_pens)
         {
            for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
            {
               r = debugger_palette[3*i+0];
               g = debugger_palette[3*i+1];
               b = debugger_palette[3*i+2];
               debugger_pens[i] = r * pens[0] / 0xff + g * pens[1] / 0xff + b * pens[2] / 0xff;
               sysdep_palette_set_pen(debug_palette, i, debugger_palette[i*3],
                  debugger_palette[i*3+1], debugger_palette[i*3+2]);
            }
         }
      }
      else
      {
         for (i=0; i<totalcolors; i++)
         {
            pens[i] = i+color_start;
            sysdep_palette_set_pen(normal_palette, i+color_start, palette[i*3],
               palette[i*3+1], palette[i*3+2]);
         }

         Machine->uifont->colortable[0] = 0;
         Machine->uifont->colortable[1] = writable_colors - 1;
         Machine->uifont->colortable[2] = writable_colors - 1;
         Machine->uifont->colortable[3] = 0;

         if(color_start)
	 {
	    printf("set black pen = 0\n");
            sysdep_palette_set_pen(normal_palette, 0, 0, 0, 0);
	 }
         if( writable_colors > (totalcolors+color_start) )
	 {	
	    printf("set white pen = %d\n", writable_colors - 1);
            sysdep_palette_set_pen(normal_palette, writable_colors - 1, 0xFF, 0xFF, 0xFF);
	 }

         /* debug palette */
         if (debugger_pens)
         {
            for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
            {
               debugger_pens[i] = i;
               sysdep_palette_set_pen(debug_palette, i, debugger_palette[i*3],
                  debugger_palette[i*3+1], debugger_palette[i*3+2]);
            }
         }
      }
   }
   else
   {
      /* normal palette */
      for (i=0; i<totalcolors; i++)
      {
         pens[i] = sysdep_palette_make_pen(normal_palette, palette[i*3],
            palette[i*3+1], palette[i*3+2]);
      }
      Machine->uifont->colortable[0] = sysdep_palette_make_pen(normal_palette,
         0, 0, 0);
      Machine->uifont->colortable[1] = sysdep_palette_make_pen(normal_palette,
         0xFF, 0xFF, 0xFF);
      Machine->uifont->colortable[2] = sysdep_palette_make_pen(normal_palette,
         0xFF, 0xFF, 0xFF);
      Machine->uifont->colortable[3] = sysdep_palette_make_pen(normal_palette,
         0, 0, 0);

      /* debug palette */
      if (debugger_pens)
      {
         for (i=0; i<DEBUGGER_TOTAL_COLORS; i++)
         {
            debugger_pens[i] = sysdep_palette_make_pen(debug_palette,
               debugger_palette[i*3], debugger_palette[i*3+1],
               debugger_palette[i*3+2]);
         }
      }
   }
   
   /* set the current_palette to the normal_palette */
   current_palette = normal_palette;
   
   return 0;
}

static void osd_free_colors(void)
{
   if(normal_palette)
   {
      sysdep_palette_destroy(normal_palette);
      normal_palette = NULL;
   }
   if(debug_palette)
   {
      sysdep_palette_destroy(debug_palette);
      debug_palette = NULL;
   }
}

void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue)
{
    sysdep_palette_get_pen(normal_palette, pen, red, green, blue);
}

void osd_modify_pen(int pen, unsigned char red,unsigned char green,unsigned char blue) 
{
   sysdep_palette_set_pen(normal_palette, pen, red, green, blue);
}

static int skip_next_frame = 0;

typedef int (*skip_next_frame_func)(int show_fps_counter, struct osd_bitmap *bitmap);
static skip_next_frame_func skip_next_frame_functions[FRAMESKIP_DRIVER_COUNT] =
{
   dos_skip_next_frame,
   barath_skip_next_frame
};

int osd_skip_this_frame(void)
{   
   return skip_next_frame;
}

void osd_debugger_focus(int new_debugger_focus)
{
   if( (!debugger_has_focus &&  new_debugger_focus) || 
       ( debugger_has_focus && !new_debugger_focus))
   {
      if(new_debugger_focus)
         osd_change_display_settings(&debug_visual, debug_palette,
            1, 1, 0);
      else
         osd_change_display_settings(&normal_visual, normal_palette,
            normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
      
      debugger_has_focus = new_debugger_focus;
   }
}

/* Update the display. */
void osd_update_video_and_audio(struct osd_bitmap *normal_bitmap,
   struct osd_bitmap *debug_bitmap, int leds_status)
{
   static int showfps=0, showfpstemp=0; 
   int skip_this_frame;
   struct osd_bitmap *current_bitmap = normal_bitmap;
   
   /* save the active bitmap for use in osd_clearbitmap, I know this
      sucks blame the core ! */
   scrbitmap = normal_bitmap;
   
   if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
   {
      if (autoframeskip)
      {
	 autoframeskip = 0;
	 frameskip = 0;
      }
      else
      {
	 if (frameskip == FRAMESKIP_LEVELS-1)
	 {
	    frameskip = 0;
	    autoframeskip = 1;
	 }
	 else frameskip++;
      }

      if (showfps == 0) showfpstemp = 2 * Machine->drv->frames_per_second;
#ifdef RASPI
      /* reset the frame counter so we'll measure the average FPS on a consistent status */
      frames_displayed = 0;
#endif
   }

   if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
   {
      if (autoframeskip)
      {
	 autoframeskip = 0;
	 frameskip = FRAMESKIP_LEVELS-1;
      }
      else
      {
	 if (frameskip == 0) autoframeskip = 1;
	 else frameskip--;
      }
      
      if (showfps == 0)	showfpstemp = 2 * Machine->drv->frames_per_second;
#ifdef RASPI
      /* reset the frame counter so we'll measure the average FPS on a consistent status */
      frames_displayed = 0;
#endif
   }
   
   if (!keyboard_pressed(KEYCODE_LSHIFT) && !keyboard_pressed(KEYCODE_RSHIFT)
       && !keyboard_pressed(KEYCODE_LCONTROL) && !keyboard_pressed(KEYCODE_RCONTROL)
       && input_ui_pressed(IPT_UI_THROTTLE))
   {
      throttle ^= 1;
#ifdef RASPI
      /* reset the frame counter so we'll measure the average FPS on a consistent status */
      frames_displayed = 0;
#endif
   }
   
   if (input_ui_pressed(IPT_UI_THROTTLE) && (keyboard_pressed(KEYCODE_RSHIFT) || keyboard_pressed(KEYCODE_LSHIFT)))
   {
      sleep_idle ^= 1;
   }
   
   if (!keyboard_pressed(KEYCODE_LSHIFT) && !keyboard_pressed(KEYCODE_RSHIFT)
       && !keyboard_pressed(KEYCODE_LCONTROL) && !keyboard_pressed(KEYCODE_RCONTROL)
       && input_ui_pressed(IPT_UI_SHOW_FPS))
   {
      if (showfpstemp)
      {
	 showfpstemp = 0;
	 schedule_full_refresh();
      }
      else
      {
	 showfps ^= 1;
	 if (showfps == 0)
	 {
	    schedule_full_refresh();
	 }
      }
   }
   
   if (keyboard_pressed (KEYCODE_LCONTROL))
   { 
      if (keyboard_pressed_memory (KEYCODE_INSERT))
         frameskipper = 0;
      if (keyboard_pressed_memory (KEYCODE_HOME))
         frameskipper = 1;
   }
   
   if (debug_bitmap)
   {
      if (input_ui_pressed(IPT_UI_TOGGLE_DEBUG))
         osd_debugger_focus(!debugger_has_focus);
      if (debugger_has_focus)
         current_bitmap = debug_bitmap;
   }
   /* this should not happen I guess, but better safe then sorry */
   else if (debugger_has_focus)
      osd_debugger_focus(0);

   if (keyboard_pressed (KEYCODE_LSHIFT))
   {
      int widthscale_mod  = 0;
      int heightscale_mod = 0;
      
      if (keyboard_pressed_memory (KEYCODE_INSERT))
         widthscale_mod = 1;
      if (keyboard_pressed_memory (KEYCODE_DEL))
         widthscale_mod = -1;
      if (keyboard_pressed_memory (KEYCODE_HOME))
         heightscale_mod = 1;
      if (keyboard_pressed_memory (KEYCODE_END))
         heightscale_mod = -1;
      if (keyboard_pressed_memory (KEYCODE_PGUP))
      {
         widthscale_mod  = 1;
         heightscale_mod = 1;
      }
      if (keyboard_pressed_memory (KEYCODE_PGDN))
      {
         widthscale_mod  = -1;
         heightscale_mod = -1;
      }
      if (widthscale_mod || heightscale_mod)
      {
         normal_widthscale  += widthscale_mod;
         normal_heightscale += heightscale_mod;
         
         if (normal_widthscale > 8)
            normal_widthscale = 8;
         else if (normal_widthscale < 1)
            normal_widthscale = 1;
         if (normal_heightscale > 8)
            normal_heightscale = 8;
         else if (normal_heightscale < 1)
            normal_heightscale = 1;
         
         if (!debugger_has_focus)
            osd_change_display_settings(&normal_visual, normal_palette,
               normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
      }
   }
   
   if (showfpstemp)         /* MAURY_BEGIN: nuove opzioni */
   {
      showfpstemp--;
      if (showfpstemp == 0) schedule_full_refresh();
   }

   skip_this_frame = skip_next_frame;
   skip_next_frame =
      (*skip_next_frame_functions[frameskipper])(showfps || showfpstemp,
        normal_bitmap);
   
   if (sound_stream && sound_enabled)
      sound_stream_update(sound_stream);

   if (skip_this_frame == 0)
   {
#ifdef RASPI
      /* at the end, we need the current time */
      INT64 curr = osd_cycles();

      /* update stats for the FPS average calculation */
      if (start_time == 0)
      {
         /* start the timer going 1 second into the game */
         if (timer_get_time() > 1.0)
         start_time = curr;
      }
      else
      {
         frames_displayed++;
         if (frames_displayed + 1 == frames_to_display)
         trying_to_quit = 1;
         end_time = curr;
      }
#endif
      profiler_mark(PROFILER_BLIT);
      sysdep_palette_update(current_palette);
      sysdep_update_display(current_bitmap);
      profiler_mark(PROFILER_END);
   }
   
   sysdep_set_leds(leds_status);
   osd_poll_joysticks();
}

void osd_set_gamma(float gamma)
{
   sysdep_palette_set_gamma(normal_palette, gamma);
   if(debug_palette)
      sysdep_palette_set_gamma(debug_palette, gamma);
}

float osd_get_gamma(void)
{
   return sysdep_palette_get_gamma(normal_palette);
}

/* brightess = percentage 0-100% */
void osd_set_brightness(int _brightness)
{
   brightness = _brightness;
   sysdep_palette_set_brightness(normal_palette, brightness *
      brightness_paused_adjust);
   if (debug_palette)
      sysdep_palette_set_brightness(debug_palette, brightness);
}

int osd_get_brightness(void)
{
   return brightness;
}

#ifndef xgl
void osd_save_snapshot(struct osd_bitmap *bitmap)
{
   save_screen_snapshot(bitmap);
}
#endif

void osd_pause(int paused)
{
   if (paused)
      brightness_paused_adjust = 0.65;
   else
      brightness_paused_adjust = 1.0;
   
   osd_set_brightness(brightness);
}
