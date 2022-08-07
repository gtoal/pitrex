/*
 * X-Mame video specifics code
 *
 */
#ifdef x11
#define __X11_WINDOW_C_

/*
 * Include files.
 */

/* for FLT_MAX */
#include <float.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#ifdef USE_MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif
#include "xmame.h"
#include "x11.h"
#include "driver.h"
#include "x11vec.h"
/* for xscreensaver support */
#include "vroot.h"

static void x11_window_update_8_to_8bpp (struct osd_bitmap *bitmap);
static void x11_window_update_8_to_16bpp (struct osd_bitmap *bitmap);
static void x11_window_update_8_to_24bpp (struct osd_bitmap *bitmap);
static void x11_window_update_8_to_32bpp (struct osd_bitmap *bitmap);
static void x11_window_update_8_to_8bpp_direct (struct osd_bitmap *bitmap);
static void x11_window_update_16_to_16bpp (struct osd_bitmap *bitmap);
static void x11_window_update_16_to_24bpp (struct osd_bitmap *bitmap);
static void x11_window_update_16_to_32bpp (struct osd_bitmap *bitmap);
static void x11_window_update_32_to_32bpp_direct (struct osd_bitmap *bitmap);
static void (*x11_window_update_display_func) (struct osd_bitmap *bitmap) = NULL;

/* hmm we need these to do the clean up correctly, or we could just 
   trust unix & X to clean up after us but lett's keep things clean */
#ifdef USE_MITSHM
static int mit_shm_attached = 0;
static XShmSegmentInfo shm_info;
static int use_mit_shm = 1;  /* use mitshm if available */
#endif
static int private_cmap_allocated = 0;
static XImage *image = NULL;
XColor color_info;
static int orig_widthscale, orig_heightscale;
static int image_width;
enum { X11_NORMAL, X11_MITSHM, X11_XIL };
static int x11_window_update_method = X11_NORMAL;
static int startx = 0;
static int starty = 0;
static unsigned long black_pen;
static int use_xsync = 0;
static int root_window_id; /* root window id (for swallowing the mame window) */
static char *geometry = NULL;
static char vecgame = 0;

/* we need to look a lookup table for pseudo modes since X doesn't give us full
   access to the palette */
static char *pseudo_color_allocated;
static unsigned long *pseudo_color_lookup;
static int pseudo_color_lookup_dirty;
static int pseudo_color_use_rw_palette;
static int pseudo_color_warn_low_on_colors;

struct rc_option x11_window_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "X11-window Related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "cursor",		"cu",			rc_bool,       &show_cursor,
     "1",		0,			0,		NULL,
     "Show/ don't show the cursor" },
#ifdef USE_MITSHM
   { "mitshm",		"ms",			rc_bool,	&use_mit_shm,
     "1",		0,			0,		NULL,
     "Use / don't use MIT Shared Mem (if available and compiled in)" },
#endif
   { "xsync",		"xs",			rc_bool,	&use_xsync,
     "1",		0,			0,		NULL,
     "Use / don't use XSync instead of XFlush as screen refresh method" },
   { "privatecmap",	"p",			rc_bool,	&use_private_cmap,
     "0",		0,			0,		NULL,
     "Enable/disable use of private color map" },
   { "xil",		"x",			rc_bool,	&use_xil,
     "1",		0,			0,		NULL,
     "Enable/disable use of XIL for scaling (if available and compiled in)" },
   { "mtxil",		"mtx",			rc_bool,	&use_mt_xil,
     "0",		0,			0,		NULL,
     "Enable/disable multi threading of XIL" },
   { "run-in-root-window", "root", 		rc_bool,	&run_in_root_window,
     "0",		0,			0,		NULL,
     "Enable/disable running in root window" },
   { "root_window_id",	"rid",			rc_int,		&root_window_id,
     "0",		0,			0,		NULL,
     "Create the xmame-window in an alternate root-window, mostly usefull for frontends!" },
   { "geometry",	"geo",			rc_string,	&geometry,
     "640x480",		0,			0,		NULL,
     "Specify the location of the window" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

/*
 * Create a display screen, or window, large enough to accomodate a bitmap
 * of the given dimensions.
 */

#ifdef USE_MITSHM
/* following routine traps missused MIT-SHM if not available */
int test_mit_shm (Display * display, XErrorEvent * error)
{
   char msg[256];
   unsigned char ret = error->error_code;

   XGetErrorText (display, ret, msg, 256);
   /* if MIT-SHM request failed, note and continue */
   if (ret == BadAccess)
   {
      use_mit_shm = 0;
      return 0;
   }
   /* else unspected error code: notify and exit */
   fprintf (stderr_file, "Unspected X Error %d: %s\n", ret, msg);
   exit (1);
   /* to make newer gcc's shut up, grrr */
   return 0;
}
#endif

/*
 * This function creates an invisible cursor.
 *
 * I got the idea and code fragment from in the Apple II+ Emulator
 * version 0.06 for Linux by Aaron Culliney
 * <chernabog@baldmountain.bbn.com>
 *
 * I also found a post from Steve Lamont <spl@szechuan.ucsd.edu> on
 * xforms@bob.usuf2.usuhs.mil.  His comments read:
 *
 * Lifted from unclutter
 * Mark M Martin. cetia 1991 mmm@cetia.fr
 * Version 4 changes by Charles Hannum <mycroft@ai.mit.edu>
 *
 * So I guess this code has been around the block a few times.
 */

static Cursor create_invisible_cursor (Display * display, Window win)
{
   Pixmap cursormask;
   XGCValues xgc;
   XColor dummycolour;
   Cursor cursor;
   GC gc;

   cursormask = XCreatePixmap (display, win, 1, 1, 1 /*depth */ );
   xgc.function = GXclear;
   gc = XCreateGC (display, cursormask, GCFunction, &xgc);
   XFillRectangle (display, cursormask, gc, 0, 0, 1, 1);
   dummycolour.pixel = 0;
   dummycolour.red = 0;
   dummycolour.flags = 04;
   cursor = XCreatePixmapCursor (display, cursormask, cursormask,
                                 &dummycolour, &dummycolour, 0, 0);
   XFreeGC (display, gc);
   XFreePixmap (display, cursormask);
   return cursor;
}

static int x11_find_best_visual(int bitmap_depth)
{
   XVisualInfo visualinfo;
   int screen_no = DefaultScreen (display);
   
   if(bitmap_depth == 8 &&
      XMatchVisualInfo (display, screen_no, 8, PseudoColor, &visualinfo))
   {
      xvisual = visualinfo.visual;
      depth   = 8;
      return 0;
   }
   if(XMatchVisualInfo (display, screen_no, 15, TrueColor, &visualinfo))
   {
      xvisual = visualinfo.visual;
      depth   = 15;
      return 0;
   }
   if(XMatchVisualInfo (display, screen_no, 16, TrueColor, &visualinfo))
   {
      xvisual = visualinfo.visual;
      depth   = 16;
      return 0;
   }
   if(XMatchVisualInfo (display, screen_no, 24, TrueColor, &visualinfo))
   {
      xvisual = visualinfo.visual;
      depth   = 24;
      return 0;
   }
   return -1;
}

int x11_window_16bpp_capable(void)
{
   return !x11_find_best_visual(16);
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int x11_window_create_display (int bitmap_depth)
{
   XSetWindowAttributes winattr;
   XGCValues xgcv;
   int screen_no;
   XEvent event;
   XSizeHints hints;
   XWMHints wm_hints;
   int image_height;
   int i;
   int event_mask;
   int my_use_private_cmap = use_private_cmap;
   
   /* set all the default values */
   vecgame = 0;
   window = 0;
   image  = NULL;
#ifdef USE_MITSHM
   mit_shm_attached = 0;
#endif
   private_cmap_allocated = 0;
   pseudo_color_allocated = NULL;
   pseudo_color_lookup = NULL;
   pseudo_color_lookup_dirty = FALSE;
   pseudo_color_use_rw_palette = FALSE;
   pseudo_color_warn_low_on_colors = TRUE;

   window_width     = widthscale  * visual_width;
   window_height    = heightscale * visual_height;
   image_width      = widthscale  * visual_width;
   image_height     = heightscale * visual_height;
   orig_widthscale  = widthscale;
   orig_heightscale = heightscale;
   screen           = DefaultScreenOfDisplay (display);
   screen_no        = DefaultScreen (display);
   
   if(run_in_root_window)
   {
      xvisual = DefaultVisual(display, screen_no);
      depth   = DefaultDepth(display, screen_no);
      my_use_private_cmap = 0;
   }
   else
   {
      if(x11_find_best_visual(bitmap_depth))
      {
         fprintf(stderr_file, "X11: Error: Couldn't find a suitable visual\n");
         return OSD_NOT_OK;
      }
      if ( (xvisual->class != DefaultVisual(display, screen_no)->class) ||
           (DefaultDepth(display, screen_no) != depth) )
      {
         my_use_private_cmap = TRUE;
      }
   }
      
   fprintf(stderr_file, "Using a Visual with a depth of %dbpp.\n", depth);
   
   /* check the available extensions if compiled in */
#ifdef USE_XIL
   if (use_xil)
   {
      init_xil ();
   }

   /*
    *  If the XIL initialization worked, then use_xil will still be set.
    */
   if (use_xil)
   {
      image_width  = visual_width;
      image_height = visual_height;
      widthscale   = 1;
      heightscale  = 1;
      x11_window_update_method = X11_XIL;
   }
#endif
#if defined USE_XIL && defined USE_MITSHM
   else
#endif
#ifdef USE_MITSHM
   if (use_mit_shm)             /* look for available Mitshm extensions */
   {
      /* get XExtensions to be if mit shared memory is available */
      if (XQueryExtension (display, "MIT-SHM", &i, &i, &i))
      {
         x11_window_update_method = X11_MITSHM;
      }
      else
      {
         fprintf (stderr_file, "X-Server Doesn't support MIT-SHM extension\n");
         use_mit_shm = 0;
      }
   }
#endif

   /* create / asign a colormap */
   if (my_use_private_cmap)
   {
      colormap = XCreateColormap (display, RootWindowOfScreen (screen), xvisual, AllocNone);
      private_cmap_allocated = 1;
      black_pen = 0;
      fprintf (stderr_file, "Using private color map\n");
   }
   else
   {
      colormap = DefaultColormapOfScreen (screen);
      black_pen = BlackPixelOfScreen (screen);
   }


   if (run_in_root_window)
   {
      int width  = DisplayWidth(display, screen_no);
      int height = DisplayHeight(display, screen_no);
      if (window_width > width || window_height > height)
      {
         fprintf (stderr_file, "OSD ERROR: Root window is to small: %dx%d, needed %dx%d\n",
            width, height, window_width, window_height);
         return OSD_NOT_OK;
      }
      
      startx        = ((width  - window_width)  / 2) & ~0x07;
      starty        = ((height - window_height) / 2) & ~0x07;
      window        = RootWindowOfScreen (screen);
      window_width  = width;
      window_height = height;
      use_mouse     = FALSE;
   }
   else
   {
      /*  Placement hints etc. */

#ifdef USE_XIL
      /*
       *  XIL allows us to rescale the window on the fly,
       *  so in this case, we don't prevent the user from
       *  resizing.
       */
      if (use_xil)
      {
         hints.flags = PSize;
      }
      else
#endif
         hints.flags = PSize | PMinSize | PMaxSize;

      hints.min_width  = hints.max_width  = hints.base_width  = window_width;
      hints.min_height = hints.max_height = hints.base_height = window_height;
      hints.x = hints.y = 0;
      hints.win_gravity = NorthWestGravity;
      
      i = XWMGeometry(display, screen_no, geometry, NULL, 0, &hints, &hints.x,
         &hints.y, &i, &i, &hints.win_gravity);
      if ((i&XValue) && (i&YValue))
         hints.flags |= PPosition | PWinGravity;
      
      /* Create and setup the window. No buttons, no fancy stuff. */
      
      winattr.background_pixel  = black_pen;
      winattr.border_pixel      = WhitePixelOfScreen (screen);
      winattr.bit_gravity       = ForgetGravity;
      winattr.win_gravity       = hints.win_gravity;
      winattr.backing_store     = NotUseful;
      winattr.override_redirect = False;
      winattr.save_under        = False;
      winattr.event_mask        = 0;
      winattr.do_not_propagate_mask = 0;
      winattr.colormap          = colormap;
      winattr.cursor            = None;
      if (root_window_id == 0)
      {
         root_window_id = RootWindowOfScreen (screen);
      }
      window = XCreateWindow (display, root_window_id, hints.x, hints.y,
                              window_width, window_height,
                              0, depth,
                              InputOutput, xvisual,
                              (CWBorderPixel | CWBackPixel | CWBitGravity |
                               CWWinGravity | CWBackingStore |
                               CWOverrideRedirect | CWSaveUnder | CWEventMask |
                               CWDontPropagate | CWColormap | CWCursor),
                              &winattr);
      if (!window)
      {
         fprintf (stderr_file, "OSD ERROR: failed in XCreateWindow().\n");
         return OSD_NOT_OK;
      }

      wm_hints.input = TRUE;
      wm_hints.flags = InputHint;

      XSetWMHints (display, window, &wm_hints);
      XSetWMNormalHints (display, window, &hints);
      XStoreName (display, window, title);

      /* Select event mask */
      
      event_mask = FocusChangeMask | ExposureMask |
         EnterWindowMask | LeaveWindowMask |
         KeyPressMask | KeyReleaseMask;
      if (use_mouse)
      {
         event_mask |= ButtonPressMask | ButtonReleaseMask;
      }
#ifdef USE_XIL
      if (use_xil)
      {
         event_mask |= StructureNotifyMask;
      }
#endif
      XSelectInput (display, window, event_mask);

      XMapRaised (display, window);
      XClearWindow (display, window);
      XWindowEvent (display, window, ExposureMask, &event);
   }
   
   /* create and setup the image */
   switch (x11_window_update_method)
   {
      case X11_XIL:
#ifdef USE_XIL
         /*
          *  XIL takes priority over MITSHM
          */
         setup_xil_images (image_width, image_height);
#endif
         break;
      case X11_MITSHM:
#ifdef USE_MITSHM
         /* Create a MITSHM image. */
         fprintf (stderr_file, "MIT-SHM Extension Available. trying to use... ");
         XSetErrorHandler (test_mit_shm);

         image = XShmCreateImage (display,
                                  xvisual,
                                  depth,
                                  ZPixmap,
                                  NULL,
                                  &shm_info,
                                  image_width,
                                  image_height);
         if (image)
         {
            shm_info.shmid = shmget (IPC_PRIVATE,
                                     image->bytes_per_line * image->height,
                                     (IPC_CREAT | 0777));
            if (shm_info.shmid < 0)
            {
               fprintf (stderr_file, "\nError: failed to create MITSHM block.\n");
               return OSD_NOT_OK;
            }

            /* And allocate the bitmap buffer. */
            /* new pen color code force double buffering in every cases */
            image->data = shm_info.shmaddr =
               (char *) shmat (shm_info.shmid, 0, 0);

            scaled_buffer_ptr = (unsigned char *) image->data;
            if (!scaled_buffer_ptr)
            {
               fprintf (stderr_file, "\nError: failed to allocate MITSHM bitmap buffer.\n");
               return OSD_NOT_OK;
            }

            shm_info.readOnly = FALSE;

            /* Attach the MITSHM block. this will cause an exception if */
            /* MIT-SHM is not available. so trap it and process         */
            if (!XShmAttach (display, &shm_info))
            {
               fprintf (stderr_file, "\nError: failed to attach MITSHM block.\n");
               return OSD_NOT_OK;
            }
            XSync (display, False);  /* be sure to get request processed */
            sleep (2);          /* enought time to notify error if any */
            XSetErrorHandler (None);  /* Restore error handler to default */
            /* Mark segment as deletable after we attach.  When all processes
               detach from the segment (progam exits), it will be deleted. 
               This way it won't be left in memory if we crash or something.
               Grr, have todo this after X attaches too since slowlaris doesn't
               like it otherwise */
            shmctl(shm_info.shmid, IPC_RMID, NULL);
            
            /* if use_mit_shm is still set we've succeeded */
            if (use_mit_shm)
            {
               fprintf (stderr_file, "Success.\nUsing Shared Memory Features to speed up\n");
               mit_shm_attached = 1;
               break;
            }
            /* else we have failed clean up before retrying without MITSHM */
            shmdt ((char *) scaled_buffer_ptr);
            scaled_buffer_ptr = NULL;
            XDestroyImage (image);
            image = NULL;
         }
         fprintf (stderr_file, "Failed\nReverting to normal XPutImage() mode\n");
         x11_window_update_method = X11_NORMAL;
#endif
      case X11_NORMAL:
         scaled_buffer_ptr = malloc (4 * image_width * image_height);
         if (!scaled_buffer_ptr)
         {
            fprintf (stderr_file, "Error: failed to allocate bitmap buffer.\n");
            return OSD_NOT_OK;
         }
         image = XCreateImage (display,
                               xvisual,
                               depth,
                               ZPixmap,
                               0,
                               (char *) scaled_buffer_ptr,
                               image_width, image_height,
                               32, /* image_width always is a multiple of 8 */
                               0);

         if (!image)
         {
            fprintf (stderr_file, "OSD ERROR: could not create image.\n");
            return OSD_NOT_OK;
         }
         break;
      default:
         fprintf (stderr_file, "Error unknown X11 update method, this shouldn't happen\n");
         return OSD_NOT_OK;
   }
   
   /* verify the number of bits per pixel and choose the correct update method */
#ifdef USE_XIL
   if (use_xil)
      /* XIL uses 8 bit visuals and does any conversion it self */
      depth = 8;
   else
#endif
      depth = image->bits_per_pixel;
   
   /* setup the palette_info struct now we have the depth */
   if (x11_init_palette_info() != OSD_OK)
      return OSD_NOT_OK;

   fprintf(stderr_file, "Actual bits per pixel = %d... ", depth);
   if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
   {
     x11_window_update_display_func = vector_vh_update;
     vecgame = 1;
   }
   else
   {
		if (bitmap_depth == 32)
		{
		if (depth == 32 && Machine->drv->video_attributes & VIDEO_RGB_DIRECT)
		 x11_window_update_display_func = x11_window_update_32_to_32bpp_direct;
		}
		else if (bitmap_depth == 16)
		{
		switch(depth)
		{
		 case 16:
		    x11_window_update_display_func = x11_window_update_16_to_16bpp;
		    break;
		 case 24:
		    x11_window_update_display_func = x11_window_update_16_to_24bpp;
		    break;
		 case 32:
		    x11_window_update_display_func = x11_window_update_16_to_32bpp;
		    break;
		}
		}
		else
		{
		switch(depth)
		{
		 case 8:
		    x11_window_update_display_func = x11_window_update_8_to_8bpp;
		    break;
		 case 16:
		    x11_window_update_display_func = x11_window_update_8_to_16bpp;
		    break;
		 case 24:
		    x11_window_update_display_func = x11_window_update_8_to_24bpp;
		    break;
		 case 32:
		    x11_window_update_display_func = x11_window_update_8_to_32bpp;
		    break;
		}
	}
   }
   
   if (x11_window_update_display_func == NULL)
   {
      fprintf(stderr_file, "Error: Unsupported bitmap depth = %dbpp, video depth = %dbpp\n", bitmap_depth, depth);
      return OSD_NOT_OK;
   }
   fprintf(stderr_file, "Ok\n");
   
   /* create gc */
   window_gc = XCreateGC (display, window, 0, &xgcv);

  /* set up colour mapping to intensity for direct vector drawing (PiTrex) */
  for (i = 0; i < 256; i++)
  {
    color_info.red = color_info.green = color_info.blue = i << 8;
    XAllocColor (display, DefaultColormap (display, DefaultScreen (display)),
	    	   &color_info);
    intensity_table[i] = color_info.pixel;
  }
   /* mouse pointer stuff */
   if (!use_mouse || (x11_grab_mouse &&  XGrabPointer (display, window, True,
      0, GrabModeAsync, GrabModeAsync, window, None, CurrentTime)))
      x11_grab_mouse = FALSE;

   if (!run_in_root_window)
   {
      normal_cursor = XCreateFontCursor (display, XC_trek);
      invisible_cursor = create_invisible_cursor (display, window);

      if (x11_grab_mouse || !show_cursor)
         XDefineCursor (display, window, invisible_cursor);
      else
         XDefineCursor (display, window, normal_cursor);
   }

   return OSD_OK;
}

/*
 * Shut down the display, also called by the core to clean up if any error
 * happens when creating the display.
 */
void x11_window_close_display (void)
{
   /* FIXME: free cursors */
   int i;

   widthscale  = orig_widthscale;
   heightscale = orig_heightscale;

   /* better free any allocated colors before freeing the colormap */
   if (pseudo_color_lookup)
   {
      if (pseudo_color_use_rw_palette)
      {
         XFreeColors (display, colormap, pseudo_color_lookup,
            display_palette_info.writable_colors, 0);
      }
      else
      {
         for (i = 0; i < display_palette_info.writable_colors; i++)
         {
            if (pseudo_color_allocated[i])
               XFreeColors (display, colormap, &pseudo_color_lookup[i], 1, 0);
         }
         free(pseudo_color_allocated);
      }
      free(pseudo_color_lookup);
   }
   
   /* This is only allocated/done if we succeeded to get a window */
   if (window)
   {
      if (x11_grab_mouse)
         XUngrabPointer (display, CurrentTime);
         
#ifdef USE_MITSHM
      if (use_mit_shm)
      {
         if (mit_shm_attached)
            XShmDetach (display, &shm_info);
         if (scaled_buffer_ptr)
            shmdt (scaled_buffer_ptr);
         scaled_buffer_ptr = NULL;
      }
#endif
      if (image)
      {
         XDestroyImage (image);
         scaled_buffer_ptr = NULL;
      }
      if (scaled_buffer_ptr)
         free (scaled_buffer_ptr);
   
      XDestroyWindow (display, window);
   }

   if (private_cmap_allocated)
      XFreeColormap (display, colormap);

   XSync (display, False);      /* send all events to sync; */
}

/*
 * Set the screen colors using the given palette.
 *
 */
int x11_window_alloc_palette (int writable_colors)
{
   int i;
   
   /* this is only relevant for 8bpp displays */
   if (depth != 8)
      return 0;
   
   if(!(pseudo_color_lookup = malloc(writable_colors * sizeof(unsigned long))))
   {
      fprintf(stderr_file, "X11-window: Error: Malloc failed for pseudo color lookup table\n");
      return -1;
   }
   
   /* set the palette to black */
   for (i = 0; i < writable_colors; i++)
      pseudo_color_lookup[i] = black_pen;

   /* allocate color cells */
   if (XAllocColorCells (display, colormap, 0, 0, 0, pseudo_color_lookup,
      writable_colors))
   {
      pseudo_color_use_rw_palette = 1;
      fprintf (stderr_file, "Using r/w palette entries to speed up, good\n");
      for (i = 0; i < writable_colors; i++)
         if (pseudo_color_lookup[i] != i) break;
         
      if (i == writable_colors)
      {
         x11_window_update_display_func = x11_window_update_8_to_8bpp_direct;
         fprintf (stderr_file, "Using direct copy to speed up, good\n");
      }
   }
   else
   {
      if (!(pseudo_color_allocated = calloc(writable_colors, sizeof(char))))
      {
         fprintf(stderr_file, "X11-window: Error: Malloc failed for pseudo color lookup table\n");
         XFreeColors (display, colormap, pseudo_color_lookup,
            writable_colors, 0);
         free(pseudo_color_lookup);
         pseudo_color_lookup=NULL;
         return -1;
      }
   }
   
   display_palette_info.writable_colors = writable_colors;
   return 0;
}

int x11_window_modify_pen (int pen, unsigned char red, unsigned char green,
   unsigned char blue)
{
   XColor color;

   /* Translate 0-255 values of new color to X 0-65535 values. */
   color.flags = (DoRed | DoGreen | DoBlue);
   color.red = (int) red << 8;
   color.green = (int) green << 8;
   color.blue = (int) blue << 8;
   color.pixel = pseudo_color_lookup[pen];

   if (pseudo_color_use_rw_palette)
   {
      XStoreColor (display, colormap, &color);
   }
   else
   {
      /* free previously allocated color */
      if (pseudo_color_allocated[pen])
      {
         XFreeColors (display, colormap, &pseudo_color_lookup[pen], 1, 0);
         pseudo_color_allocated[pen] = FALSE;
      }

      /* allocate new color and assign it to pen index */
      if (XAllocColor (display, colormap, &color))
      {
         if (pseudo_color_lookup[pen] != color.pixel)
            pseudo_color_lookup_dirty = TRUE;
         pseudo_color_lookup[pen] = color.pixel;
         pseudo_color_allocated[pen] = TRUE;
      }
      else /* try again with the closest match */
      {
         int i;
         XColor colors[256];
         int my_red   = (int)red << 8;
         int my_green = (int)green << 8;
         int my_blue  = (int)blue << 8;
         int best_pixel = black_pen;
         float best_diff = FLT_MAX;
         
         for(i=0;i<256;i++)
            colors[i].pixel = i;
         
         XQueryColors(display, colormap, colors, 256);
         for(i=0;i<256;i++)
         {
            #define SQRT(x) ((float)(x)*(x))
            float diff = SQRT(my_red - colors[i].red) + 
               SQRT(my_green - colors[i].green) +
               SQRT(my_blue - colors[i].blue);
            if (diff < best_diff)
            {
               best_pixel = colors[i].pixel;
               best_diff  = diff;
            }
         }
         
         color = colors[best_pixel];
         
         if (XAllocColor (display, colormap, &color))
         {
            if (pseudo_color_lookup[pen] != color.pixel)
               pseudo_color_lookup_dirty = TRUE;
            pseudo_color_lookup[pen] = color.pixel;
            pseudo_color_allocated[pen] = TRUE;
         }
         else
         {
            if (pseudo_color_warn_low_on_colors)
            {
               pseudo_color_warn_low_on_colors = 0;
               fprintf (stderr_file,
                  "X11-palette: Warning: Closest color match alloc failed\n"
                  "Couldn't allocate all colors, some parts of the emulation may be black\n"
                  "Try running xmame with the -privatecmap option\n");
            }
            
            /* If color allocation failed, use black to ensure the
               pen is not left set to an invalid color */
            pseudo_color_lookup[pen] = black_pen;
            return -1;
         }
      }
   }
   return 0;
}

/* invoked by main tree code to update bitmap into screen */
void x11_window_update_display (struct osd_bitmap *bitmap)
{
   int old_use_dirty = use_dirty;
   
   if (current_palette->lookup_dirty || pseudo_color_lookup_dirty)
   {
      use_dirty = 0;
      pseudo_color_lookup_dirty = 0;
      /* current_palette->lookup_dirty is cleared for us by
         sysdep_palette_update() */
   }

   (*x11_window_update_display_func) (bitmap);

#ifdef USE_XIL
   if (use_xil)
      refresh_xil_screen ();
#endif

   use_dirty = old_use_dirty;

   if (use_mouse &&
       keyboard_pressed (KEYCODE_LALT) &&
       keyboard_pressed_memory (KEYCODE_PGDN))
   {
      if (x11_grab_mouse)
      {
         XUngrabPointer (display, CurrentTime);
         if (show_cursor)
            XDefineCursor (display, window, normal_cursor);
         x11_grab_mouse = FALSE;
      }
      else
      {
         if (!XGrabPointer (display, window, True, 0, GrabModeAsync,
                            GrabModeAsync, window, None, CurrentTime))
         {
            if (show_cursor)
               XDefineCursor (display, window, invisible_cursor);
            x11_grab_mouse = TRUE;
         }
      }
   }

   /* some games "flickers" with XFlush, so command line option is provided */
   if (use_xsync)
      XSync (display, False);   /* be sure to get request processed */
   else
      XFlush (display);         /* flush buffer to server */
}

void x11_window_refresh_screen (void)
{
   if (!vecgame)
   {
	   switch (x11_window_update_method)
	   {
	      case X11_XIL:
#ifdef USE_XIL
        	 refresh_xil_screen ();
#endif
        	 break;
	      case X11_MITSHM:
#ifdef USE_MITSHM
        	 XShmPutImage (display, window, window_gc, image, 0, 0, 0, 0, image->width,
                	       image->height, FALSE);
#endif
        	 break;
	      case X11_NORMAL:
        	 XPutImage (display, window, window_gc, image, 0, 0, 0, 0, image->width,
                	    image->height);
        	 break;
	   }
   }
}

INLINE void x11_window_put_image (int x, int y, int width, int height)
{
   if (!vecgame)
   {
	   switch (x11_window_update_method)
	   {
	      case X11_XIL:
        	 /* xil doesn't need a put_image */
        	 break;
	      case X11_MITSHM:
#ifdef USE_MITSHM
        	 XShmPutImage (display, window, window_gc, image, x, y, x+startx, y+starty, width, height,
                	       FALSE);
#endif
        	 break;
	      case X11_NORMAL:
        	 XPutImage (display, window, window_gc, image, x, y, x+startx, y+starty, width, height);
        	 break;
	   }
    }
}

#define DEST_WIDTH image_width
#define DEST scaled_buffer_ptr
#define SRC_PIXEL unsigned char
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) x11_window_put_image(X, Y, WIDTH, HEIGHT);

#define DEST_PIXEL unsigned char

static void x11_window_update_8_to_8bpp_direct (struct osd_bitmap *bitmap)
{
#include "blit.h"
}

static void x11_window_update_8_to_8bpp (struct osd_bitmap *bitmap)
{
#define INDIRECT pseudo_color_lookup
#include "blit.h"
#undef INDIRECT
}

#undef DEST_PIXEL

#define INDIRECT current_palette->lookup

static void x11_window_update_8_to_16bpp (struct osd_bitmap *bitmap)
{
#define BLIT_16BPP_HACK
#define DEST_PIXEL unsigned short
#include "blit.h"
#undef DEST_PIXEL
#undef BLIT_16BPP_HACK
}

#define DEST_PIXEL unsigned int

static void x11_window_update_8_to_24bpp (struct osd_bitmap *bitmap)
{
#define PACK_BITS
#include "blit.h"
#undef PACK_BITS
}

static void x11_window_update_8_to_32bpp (struct osd_bitmap *bitmap)
{
#include "blit.h"
}

#undef  DEST_PIXEL

#undef  SRC_PIXEL
#define SRC_PIXEL unsigned short

static void x11_window_update_16_to_16bpp (struct osd_bitmap *bitmap)
{
#define DEST_PIXEL unsigned short

   if (current_palette->lookup)
   {
#include "blit.h"
   }
   else
   {
#undef  INDIRECT
#include "blit.h"
#define INDIRECT current_palette->lookup
   }

#undef DEST_PIXEL
}

#define DEST_PIXEL unsigned int

static void x11_window_update_16_to_24bpp (struct osd_bitmap *bitmap)
{
#define PACK_BITS
#include "blit.h"
#undef PACK_BITS
}

static void x11_window_update_16_to_32bpp (struct osd_bitmap *bitmap)
{
#include "blit.h"
}

#undef  INDIRECT
#undef  SRC_PIXEL
#define SRC_PIXEL unsigned int

static void x11_window_update_32_to_32bpp_direct(struct osd_bitmap *bitmap)
{
#include "blit.h"
}

#undef DEST_PIXEL
#undef SRC_PIXEL

#endif /* ifdef x11 */
