/*
 ******************* X-Mame header file *********************
 * file "xmame.h"
 *
 * by jantonio@dit.upm.es
 *
 ************************************************************
*/

#ifndef __XMAME_H_
#define __XMAME_H_

#ifdef __MAIN_C_
#define EXTERN
#else
#define EXTERN extern
#endif

/*
 * Include files.
 */

#ifdef openstep
#include <libc.h>
#include <math.h>
#endif /* openstep */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "osdepend.h"
#include "mame.h"
#include "sysdep/sysdep_palette.h"
#include "sysdep/rc.h"
#include "sysdep/sound_stream.h"

/*
 * Definitions.
 */
 
#ifndef TRUE
#define	TRUE			(1)
#endif

#ifndef FALSE
#define FALSE			(0)
#endif

#define OSD_OK			(0)
#define OSD_NOT_OK		(1)

#define DEBUG(x)

#define FRAMESKIP_LEVELS 12

/* Taken from common.h since the rest of common.h creates to many conflicts
   with X headers */
struct my_rectangle
{
	int min_x,max_x;
	int min_y,max_y;
};

/*
 * Global variables.
 */

/* dirty stuff */
EXTERN unsigned char *dirty_lines;
EXTERN unsigned char **dirty_blocks;

/* global variables and miscellaneous flags */

EXTERN int		sound_enabled;
EXTERN int	 	widthscale;        /* X scale */
EXTERN int	 	heightscale;       /* Y SCALE */
EXTERN int		video_colors_used; /* max colors used by any palette */
EXTERN int		video_fps;
EXTERN char		*home_dir;
EXTERN char		title[50];
EXTERN int		use_mouse;
EXTERN int		use_dirty;
EXTERN int		throttle;
EXTERN int		autoframeskip;
EXTERN int		frameskip;
EXTERN int		game_index;
EXTERN int		use_scanlines;
EXTERN int		cabview;
EXTERN char		*cabname;
EXTERN float		display_aspect_ratio;
EXTERN int 		sleep_idle;
EXTERN int 		max_autoframeskip;
EXTERN int		use_aspect_ratio;
EXTERN int		normal_use_aspect_ratio;
EXTERN struct sysdep_palette_info display_palette_info;
EXTERN struct sysdep_palette_struct *current_palette;
EXTERN struct sysdep_palette_struct *normal_palette;
EXTERN struct sysdep_palette_struct *debug_palette;
EXTERN struct sound_stream_struct *sound_stream;
#ifdef MESS
EXTERN char		*crcdir;
#endif

/* visual is the visual part of the bitmap */
EXTERN int 		visual_width;
EXTERN int		visual_height;
EXTERN struct my_rectangle visual;

/* File descripters for stdout / stderr redirection, without svgalib inter
   fering */
EXTERN FILE *stdout_file;
EXTERN FILE *stderr_file;

/* system dependent functions */
int  sysdep_init(void);
void sysdep_close(void);
#ifdef DISPMANX
int  sysdep_create_display(int width, int height, int depth);
#else
int  sysdep_create_display(int depth);
#endif
void sysdep_display_close(void);
int  sysdep_display_alloc_palette(int writable_colors);
int  sysdep_display_set_pen(int pen, unsigned char red, unsigned char green, unsigned char blue);
int  sysdep_display_16bpp_capable(void);
void sysdep_update_display(struct osd_bitmap *bitmap);
int  sysdep_set_video_mode(void);
void sysdep_set_text_mode(void);
void sysdep_set_leds(int leds);

/* input related */
int  osd_input_initpre(void);
int  osd_input_initpost(void);
void osd_input_close(void);
void osd_poll_joysticks(void);
void sysdep_update_keyboard (void);
void sysdep_mouse_poll(void);

/* dirty functions */
int  osd_dirty_init(void);
void osd_dirty_close(void);

/* network funtions */
int  osd_net_init(void);
void osd_net_close(void);

/* debug functions */
int  osd_debug_init(void);
void osd_debug_close(void);

/* mode handling functions */
int mode_disabled(int width, int height, int depth);
int mode_match(int width, int height);

/* frameskip functions */
int dos_skip_next_frame(int show_fps_counter, struct osd_bitmap *bitmap);
int barath_skip_next_frame(int show_fps_counter, struct osd_bitmap *bitmap);

/* miscelaneous */
int config_init (int argc, char *argv[]);
void config_exit(void);
int frontend_list(char *gamename);
int frontend_ident(char *gamename);
void init_rom_path(void);
#ifndef HAVE_SNPRINTF
int snprintf(char *s, size_t maxlen, const char *fmt, ...);
#endif

/* option structs */
extern struct rc_option video_opts[];
extern struct rc_option display_opts[];
extern struct rc_option mode_opts[];
extern struct rc_option sound_opts[];
extern struct rc_option input_opts[];
extern struct rc_option network_opts[];
extern struct rc_option fileio_opts[];
extern struct rc_option frontend_list_opts[];
extern struct rc_option frontend_ident_opts[];

#undef EXTERN
#endif
