/* Translation of SVGAlib function calls into calls to the Vector drawing library for the PiTrex Vectrex interface cartridge.
   Version 0.2 - only bare minimum of functions implemented to get something working.
   Changes:
    2020-06-16 V. 0.2 - Added colour to intensity translation.

   Kevin Koster, 2020 */

#include <unistd.h>
#include <stdio.h>
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include "svgalib-vectrex.h"
#include "vectrextokeyboard.h"

int vgacolor;
int vgabackgroundcolor;
int vgamode;
char beamintensity;
char svgalib_initialised = 0;
#include "intensitypalette.h"

struct info infotable[] =
{
    {80, 25, 16, 160, 0},	/* VGAlib VGA modes */
    {320, 200, 16, 40, 0},
    {640, 200, 16, 80, 0},
    {640, 350, 16, 80, 0},
    {640, 480, 16, 80, 0},
    {320, 200, 256, 320, 1},
    {320, 240, 256, 80, 0},
    {320, 400, 256, 80, 0},
    {360, 480, 256, 90, 0},
    {640, 480, 2, 80, 0},

    {640, 480, 256, 640, 1},	/* VGAlib SVGA modes */
    {800, 600, 256, 800, 1},
    {1024, 768, 256, 1024, 1},
    {1280, 1024, 256, 1280, 1},

    {320, 200, 1 << 15, 640, 2},	/* Hicolor/truecolor modes */
    {320, 200, 1 << 16, 640, 2},
    {320, 200, 1 << 24, 320 * 3, 3},
    {640, 480, 1 << 15, 640 * 2, 2},
    {640, 480, 1 << 16, 640 * 2, 2},
    {640, 480, 1 << 24, 640 * 3, 3},
    {800, 600, 1 << 15, 800 * 2, 2},
    {800, 600, 1 << 16, 800 * 2, 2},
    {800, 600, 1 << 24, 800 * 3, 3},
    {1024, 768, 1 << 15, 1024 * 2, 2},
    {1024, 768, 1 << 16, 1024 * 2, 2},
    {1024, 768, 1 << 24, 1024 * 3, 3},
    {1280, 1024, 1 << 15, 1280 * 2, 2},
    {1280, 1024, 1 << 16, 1280 * 2, 2},
    {1280, 1024, 1 << 24, 1280 * 3, 3},

    {800, 600, 16, 100, 0},	/* SVGA 16-color modes */
    {1024, 768, 16, 128, 0},
    {1280, 1024, 16, 160, 0},

    {720, 348, 2, 90, 0},	/* Hercules emulation mode */

    {320, 200, 1 << 24, 320 * 4, 4},
    {640, 480, 1 << 24, 640 * 4, 4},
    {800, 600, 1 << 24, 800 * 4, 4},
    {1024, 768, 1 << 24, 1024 * 4, 4},
    {1280, 1024, 1 << 24, 1280 * 4, 4},

    {1152, 864, 16, 144, 0},
    {1152, 864, 256, 1152, 1},
    {1152, 864, 1 << 15, 1152 * 2, 2},
    {1152, 864, 1 << 16, 1152 * 2, 2},
    {1152, 864, 1 << 24, 1152 * 3, 3},
    {1152, 864, 1 << 24, 1152 * 4, 4},

    {1600, 1200, 16, 200, 0},
    {1600, 1200, 256, 1600, 1},
    {1600, 1200, 1 << 15, 1600 * 2, 2},
    {1600, 1200, 1 << 16, 1600 * 2, 2},
    {1600, 1200, 1 << 24, 1600 * 3, 3},
    {1600, 1200, 1 << 24, 1600 * 4, 4},

    {320, 240, 256, 320, 1},	
    {320, 240, 1<<15, 320*2, 2},
    {320, 240, 1<<16, 320*2, 2},
    {320, 240, 1<<24, 320*3, 3},
    {320, 240, 1<<24, 320*4, 4},
     
    {400, 300, 256, 400, 1},
    {400, 300, 1<<15, 400*2, 2},
    {400, 300, 1<<16, 400*2, 2},
    {400, 300, 1<<24, 400*3, 3},
    {400, 300, 1<<24, 400*4, 4},
     
    {512, 384, 256, 512, 1},		
    {512, 384, 1<<15, 512*2, 2},
    {512, 384, 1<<16, 512*2, 2},
    {512, 384, 1<<24, 512*3, 3},
    {512, 384, 1<<24, 512*4, 4},

    {960, 720, 256, 960, 1},		
    {960, 720, 1<<15, 960*2, 2},
    {960, 720, 1<<16, 960*2, 2},
    {960, 720, 1<<24, 960*3, 3},
    {960, 720, 1<<24, 960*4, 4},

    {1920, 1440, 256, 1920, 1},		
    {1920, 1440, 1<<15, 1920*2, 2},
    {1920, 1440, 1<<16, 1920*2, 2},
    {1920, 1440, 1<<24, 1920*3, 3},
    {1920, 1440, 1<<24, 1920*4, 4},

    {320, 400, 1<<8,  320,   1},
    {320, 400, 1<<15, 320*2, 2},
    {320, 400, 1<<16, 320*2, 2},
    {320, 400, 1<<24, 320*3, 3},
    {320, 400, 1<<24, 320*4, 4},

    {640, 400, 256, 640, 1},
    {640, 400, 1<<15, 640*2, 2},
    {640, 400, 1<<16, 640*2, 2},
    {640, 400, 1<<24, 640*3, 3},
    {640, 400, 1<<24, 640*4, 4},

    {320, 480, 256, 320, 1},
    {320, 480, 1<<15, 320*2, 2},
    {320, 480, 1<<16, 320*2, 2},
    {320, 480, 1<<24, 320*3, 3},
    {320, 480, 1<<24, 320*4, 4},

    {720, 540, 256, 720, 1},
    {720, 540, 1<<15, 720*2, 2},
    {720, 540, 1<<16, 720*2, 2},
    {720, 540, 1<<24, 720*3, 3},
    {720, 540, 1<<24, 720*4, 4},

    {848, 480, 256, 848, 1},
    {848, 480, 1<<15, 848*2, 2},
    {848, 480, 1<<16, 848*2, 2},
    {848, 480, 1<<24, 848*3, 3},
    {848, 480, 1<<24, 848*4, 4},

    {1072, 600, 256, 1072, 1},
    {1072, 600, 1<<15, 1072*2, 2},
    {1072, 600, 1<<16, 1072*2, 2},
    {1072, 600, 1<<24, 1072*3, 3},
    {1072, 600, 1<<24, 1072*4, 4},

    {1280, 720, 256, 1280, 1},
    {1280, 720, 1<<15, 1280*2, 2},
    {1280, 720, 1<<16, 1280*2, 2},
    {1280, 720, 1<<24, 1280*3, 3},
    {1280, 720, 1<<24, 1280*4, 4},

    {1360, 768, 256, 1360, 1},
    {1360, 768, 1<<15, 1360*2, 2},
    {1360, 768, 1<<16, 1360*2, 2},
    {1360, 768, 1<<24, 1360*3, 3},
    {1360, 768, 1<<24, 1360*4, 4},

    {1800, 1012, 256, 1800, 1},
    {1800, 1012, 1<<15, 1800*2, 2},
    {1800, 1012, 1<<16, 1800*2, 2},
    {1800, 1012, 1<<24, 1800*3, 3},
    {1800, 1012, 1<<24, 1800*4, 4},

    {1920, 1080, 256, 1920, 1},
    {1920, 1080, 1<<15, 1920*2, 2},
    {1920, 1080, 1<<16, 1920*2, 2},
    {1920, 1080, 1<<24, 1920*3, 3},
    {1920, 1080, 1<<24, 1920*4, 4},

    {2048, 1152, 256, 2048, 1},
    {2048, 1152, 1<<15, 2048*2, 2},
    {2048, 1152, 1<<16, 2048*2, 2},
    {2048, 1152, 1<<24, 2048*3, 3},
    {2048, 1152, 1<<24, 2048*4, 4},

    {2048, 1536, 256, 2048, 1},
    {2048, 1536, 1<<15, 2048*2, 2},
    {2048, 1536, 1<<16, 2048*2, 2},
    {2048, 1536, 1<<24, 2048*3, 3},
    {2048, 1536, 1<<24, 2048*4, 4},

    {512, 480, 256, 512, 1},		
    {512, 480, 1<<15, 512*2, 2},
    {512, 480, 1<<16, 512*2, 2},
    {512, 480, 1<<24, 512*3, 3},
    {512, 480, 1<<24, 512*4, 4},

    {400, 600, 256, 400, 1},
    {400, 600, 1<<15, 400*2, 2},
    {400, 600, 1<<16, 400*2, 2},
    {400, 600, 1<<24, 400*3, 3},
    {400, 600, 1<<24, 400*4, 4},
};

#define MAX_MODES (sizeof(infotable) / sizeof(struct info))

extern int vga_init(void)
{
    if (!svgalib_initialised)
	{
	 if (!vectrexinit(1) )
	 {
	  printf("Could Not Initialise Vectrex Connection\n");
	  return -1;
	 }
	 v_init();
	 svgalib_initialised = 1;
	}

	vgacolor = 0xFF;
	vgabackgroundcolor = 0;
	beamintensity = 100;
	return 0;
}

extern void vga_disabledriverreport(void)
{

}

/* Could use this to limit to optimal resolutions? */
extern int vga_hasmode(int mode)
{
	if (mode == TEXT)
	 return 1;
    if (mode < 0 || mode > MAX_MODES)
	 return 0;
	return 1;
}

/* Need to note the resolution set here */
extern int vga_setmode(int mode)
{
	vgamode = mode;
	return 0;
}

/* Colour turns into beam intensity setting from intensitypalette.c */
extern int vga_setcolor(int color)
{
 beamintensity=intensityPal[color];
 return 0;
}

/* Checks for whether a recal is required.
 * Always draws up to the maximum recal interval - a traditional WaitRecal
 * may allow for a cleaner display, but only possible with buffered vector
 * lists. Or is it?
 * Based on v_WaitRecal(), if it works maybe add the button/reset checks as well,
 * and preferably get it integrated into the vector drawing library.   
 */
void __svgalib_vectrex_recalcheck(void)
{
	if (GET (VIA_int_flags) & 0x20)
	{
	 v_WaitRecal();
/*	 v_readJoystick1Digital();
	 printf ("joystick state X,Y: %d,%d\n",currentJoy1X,currentJoy1Y);
	 printf ("button state: 0x%x\n",currentButtonState);
*/
/*	 ioDone = 0;
	 SWITCH_BEAM_OFF();
#ifdef PITREX_DEBUG
	 if (currentButtonState ==0x9) // button 1 + 4 pressed - print speed info
	 {
		uint16_t timeLeft = ((Vec_Rfrsh&0xff)*256+(Vec_Rfrsh>>8)) - GET(VIA_t2_cnt_hi)*256;
		printf("Refresh usage: %i%% \r\n", (timeLeft*100/((Vec_Rfrsh&0xff)*256+(Vec_Rfrsh>>8)))  );
	 }
#endif

	 v_resetDetection();
	 if (currentButtonState ==0xf) // button 1+ 2 + 3+4 -> go menu
	 {
#ifndef FREESTANDING
	 exit(0); // pressing all four buttons exits
#endif
	      printf("Restarting kernel...\r\n");
	      uint32_t progSapce = 0xb00000;//0x1f000000;
	      void (*progStart)(void) = (void (*)(void))progSapce;
	      progStart();
	 }

	 if (currentButtonState ==0x6) // button 2 + 3 pressed - enter calibration menu1
	  inCalibration = 1;
	 if (inCalibration)
	  v_calibrate();

	 // reset T2 VIA timer to 50Hz
	 SETW (VIA_t2, Vec_Rfrsh);
	 currentZSH = 0x100;
	 consecutiveDraws = 0;
	 v_deflok();
	 v_resetIntegratorOffsets();
*/
	}
}

int __svgalib_vectrex_scalexcoordinate(int coordinate)
{
	return (int)((coordinate / (float)infotable[vgamode].xdim) * 65535) - 32767;
}

int __svgalib_vectrex_scaleycoordinate(int coordinate)
{
	return (~((int)((coordinate / (float)infotable[vgamode].ydim) * 65535) - 32767)) + 1;
}

/* Scale to vector drawing library co-ordinates
 * TODO: Skip lines drawn adjacent to previously drawn ones, eg. for filled areas or to make think lines
 * TODO: Buffered drawing with automatic refresh
 * TODO: Allow for erasing lines by drawing pixels/lines/filled-shapes over them in the same colour
 *  -None of these things actually seem to be required for either of the known vector SVGAlib games though, so not much point.
 */
extern int vga_drawline(int x1, int y1, int x2, int y2)
{
	if (beamintensity <= 0) return 0;

	__svgalib_vectrex_recalcheck();
#ifdef PITREX_DEBUG
	printf("SVGAlib-vectrex: Draw Input = %d,%d-%d,%d.\n", x1, y1, x2, y2);
#endif
	v_directDraw32	(
				 __svgalib_vectrex_scalexcoordinate(x1),
				 __svgalib_vectrex_scaleycoordinate(y1),
				 __svgalib_vectrex_scalexcoordinate(x2),
				 __svgalib_vectrex_scaleycoordinate(y2),
				 beamintensity
				);
	return 0;
}

/* Scale to vector drawing library co-ordinates */
extern int vga_drawpixel(int x, int y)
{
	if (beamintensity <= 0) return 0;

	__svgalib_vectrex_recalcheck();
#ifdef PITREX_DEBUG
	printf("SVGAlib-vectrex: Draw Input = %d,%d.\n", x, y);
#endif
	x = __svgalib_vectrex_scalexcoordinate(x);
	y = __svgalib_vectrex_scaleycoordinate(y);
	v_directDraw32	(x, y, x+1, y, beamintensity);
	return 0;
}

/* Ignore screen buffering entirely.
 * [Or maybe a good place for a v_WaitRecal?]  */
void gl_copyscreen(GraphicsContext * gc)
{

}

void gl_clearscreen(int c)
{

}

void gl_enableclipping(void)
{

}

void gl_line(int x1, int y1, int x2, int y2, int c)
{
 beamintensity=intensityPal[c];
 vga_drawline(x1, y1, x2, y2);
}

int gl_setcontextvga(int m)
{
 return !vga_hasmode(m);
}

int gl_setcontextvgavirtual(int m)
{
 return !vga_hasmode(m);
}

void gl_setpalettecolor(int c, int r, int b, int g)
{

}

void gl_setpixel(int x, int y, int c)
{
 beamintensity=intensityPal[c];
 vga_drawpixel(x, y);
}

extern int vga_setpalvec(int start, int num, int *pal)
{
 return num;
}
 
