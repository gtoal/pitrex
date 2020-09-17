/* Translation of SVGAlib function calls into calls to the Vector drawing library for the PiTrex Vectrex interface cartridge.
   Version 0.3 - only bare minimum of functions implemented to get something working.
   Kevin Koster, 2020 */

#ifndef VGA_VECTREX_H
#define VGA_VECTREX_H

#define TEXT 	     0		/* Compatible with VGAlib v1.2 */
#define G320x200x16  1
#define G640x200x16  2
#define G640x350x16  3
#define G640x480x16  4
#define G320x200x256 5
#define G320x240x256 6
#define G320x400x256 7
#define G360x480x256 8
#define G640x480x2   9

#define G640x480x256 10
#define G800x600x256 11
#define G1024x768x256 12

#define G1280x1024x256 13	/* Additional modes. */

#define G320x200x32K 14
#define G320x200x64K 15
#define G320x200x16M 16
#define G640x480x32K 17
#define G640x480x64K 18
#define G640x480x16M 19
#define G800x600x32K 20
#define G800x600x64K 21
#define G800x600x16M 22
#define G1024x768x32K 23
#define G1024x768x64K 24
#define G1024x768x16M 25
#define G1280x1024x32K 26
#define G1280x1024x64K 27
#define G1280x1024x16M 28

#define G800x600x16 29
#define G1024x768x16 30
#define G1280x1024x16 31

#define G720x348x2 32		/* Hercules emulation mode */

#define G320x200x16M32 33	/* 32-bit per pixel modes. */
#define G640x480x16M32 34
#define G800x600x16M32 35
#define G1024x768x16M32 36
#define G1280x1024x16M32 37

/* additional resolutions */
#define G1152x864x16 38
#define G1152x864x256 39
#define G1152x864x32K 40
#define G1152x864x64K 41
#define G1152x864x16M 42
#define G1152x864x16M32 43

#define G1600x1200x16 44
#define G1600x1200x256 45
#define G1600x1200x32K 46
#define G1600x1200x64K 47
#define G1600x1200x16M 48
#define G1600x1200x16M32 49

#define G320x240x256V 50
#define G320x240x32K 51
#define G320x240x64K 52
#define G320x240x16M 53
#define G320x240x16M32 54

#define G400x300x256 55
#define G400x300x32K 56
#define G400x300x64K 57
#define G400x300x16M 58
#define G400x300x16M32 59

#define G512x384x256 60
#define G512x384x32K 61
#define G512x384x64K 62
#define G512x384x16M 63
#define G512x384x16M32 64

#define G960x720x256 65
#define G960x720x32K 66
#define G960x720x64K 67
#define G960x720x16M 68
#define G960x720x16M32 69

#define G1920x1440x256 70
#define G1920x1440x32K 71
#define G1920x1440x64K 72
#define G1920x1440x16M 73
#define G1920x1440x16M32 74

/* The following modes have been introduced by SciTech Display Doctor */

#define G320x400x256V 75
#define G320x400x32K 76
#define G320x400x64K 77
#define G320x400x16M 78
#define G320x400x16M32 79

#define G640x400x256 80
#define G640x400x32K 81
#define G640x400x64K 82
#define G640x400x16M 83
#define G640x400x16M32 84

#define G320x480x256 85
#define G320x480x32K 86
#define G320x480x64K 87
#define G320x480x16M 88
#define G320x480x16M32 89

#define G720x540x256 90
#define G720x540x32K 91
#define G720x540x64K 92
#define G720x540x16M 93
#define G720x540x16M32 94

#define G848x480x256 95
#define G848x480x32K 96
#define G848x480x64K 97
#define G848x480x16M 98
#define G848x480x16M32 99

#define G1072x600x256 100
#define G1072x600x32K 101
#define G1072x600x64K 102
#define G1072x600x16M 103
#define G1072x600x16M32 104

#define G1280x720x256 105
#define G1280x720x32K 106
#define G1280x720x64K 107
#define G1280x720x16M 108
#define G1280x720x16M32 109

#define G1360x768x256 110
#define G1360x768x32K 111
#define G1360x768x64K 112
#define G1360x768x16M 113
#define G1360x768x16M32 114

#define G1800x1012x256 115
#define G1800x1012x32K 116
#define G1800x1012x64K 117
#define G1800x1012x16M 118
#define G1800x1012x16M32 119

#define G1920x1080x256 120
#define G1920x1080x32K 121
#define G1920x1080x64K 122
#define G1920x1080x16M 123
#define G1920x1080x16M32 124

#define G2048x1152x256 125
#define G2048x1152x32K 126
#define G2048x1152x64K 127
#define G2048x1152x16M 128
#define G2048x1152x16M32 129

#define G2048x1536x256 130
#define G2048x1536x32K 131
#define G2048x1536x64K 132
#define G2048x1536x16M 133
#define G2048x1536x16M32 134

#define G512x480x256 135
#define G512x480x32K 136
#define G512x480x64K 137
#define G512x480x16M 138
#define G512x480x16M32 139

#define G400x600x256 140
#define G400x600x32K 141
#define G400x600x64K 142
#define G400x600x16M 143
#define G400x600x16M32 144

#define G640x854x128 145 /* a special fake mode for vectrex */

#define __GLASTMODE G640x854x128
#define GLASTMODE vga_lastmodenumber()

#define infotable __svgalib_infotable

/* vga functions */
void keyboard_close(void);
char *keyboard_getstate(void);
int keyboard_init(void);
int keyboard_keypressed(int scancode);
void keyboard_translatekeys(int mask);
int keyboard_update(void);

extern int vga_setpalvec(int start, int num, int *pal);

extern void vga_disabledriverreport(void);
extern int vga_drawline(int x1, int y1, int x2, int y2);
extern int vga_drawpixel(int x, int y);
extern int vga_hasmode(int mode);
extern int vga_init(void);
extern int vga_setcolor(int color);
extern int vga_setmode(int mode);

/* graphics mode information */
struct info {
    int xdim;
    int ydim;
    int colors;
    int xbytes;
    int bytesperpixel;
};

/* vgagl functions */
	typedef struct {
	void (*driver_setpixel_func) (int, int, int);
	int (*driver_getpixel_func) (int, int);
	void (*driver_hline_func) (int, int, int, int);
	void (*driver_fillbox_func) (int, int, int, int, int);
	void (*driver_putbox_func) (int, int, int, int, void *, int);
	void (*driver_getbox_func) (int, int, int, int, void *, int);
	void (*driver_putboxmask_func) (int, int, int, int, void *);
	void (*driver_putboxpart_func) (int, int, int, int, int, int, void *,
					int, int);
	void (*driver_getboxpart_func) (int, int, int, int, int, int, void *,
					int, int);
	void (*driver_copybox_func) (int, int, int, int, int, int);
    } framebufferfunctions;

    typedef struct {
	unsigned char modetype;	/* virtual, paged, linear, mode X */
	unsigned char modeflags;	/* or planar16 */
	unsigned char dummy;
	unsigned char flippage;
	int width;		/* width in pixels */
	int height;		/* height in pixels */
	int bytesperpixel;	/* bytes per pixel (1, 2, 3, or 4) */
	int colors;		/* number of colors */
	int bitsperpixel;	/* bits per pixel (8, 15, 16 or 24) */
	int bytewidth;		/* length of a scanline in bytes */
	char *vbuf;		/* address of framebuffer */
	int clip;		/* clipping enabled? */
	int clipx1;		/* top-left coordinate of clip window */
	int clipy1;
	int clipx2;		/* bottom-right coordinate of clip window */
	int clipy2;
	framebufferfunctions ff;
    } GraphicsContext;

    extern GraphicsContext currentcontext;

#define BYTESPERPIXEL (currentcontext.bytesperpixel)
#define BYTEWIDTH (currentcontext.bytewidth)
#define WIDTH (currentcontext.width)
#define HEIGHT (currentcontext.height)
#define VBUF (currentcontext.vbuf)
#define MODETYPE (currentcontext.modetype)
#define MODEFLAGS (currentcontext.modeflags)
#define BITSPERPIXEL (currentcontext.bitsperpixel)
#define COLORS (currentcontext.colors)

#define __clip (currentcontext.clip)
#define __clipx1 (currentcontext.clipx1)
#define __clipy1 (currentcontext.clipy1)
#define __clipx2 (currentcontext.clipx2)
#define __clipy2 (currentcontext.clipy2)
    
extern struct info infotable[];

extern void gl_clearscreen(int c);
extern void gl_copyscreen(GraphicsContext * gc);
extern void gl_enableclipping(void);
extern void gl_line(int x1, int y1, int x2, int y2, int c);
extern int gl_setcontextvga(int m);
extern int gl_setcontextvgavirtual(int m);
extern void gl_setpalettecolor(int c, int r, int b, int g);
extern void gl_setpixel(int x, int y, int c);

/* SVGAlib-Vectrex Declarations: */
extern int single_point_of_init(void); // temp name while restructuring
extern char svgalib_initialised;

#endif /* VGA_VECTREX_H */
