//#define FILESYSTEM 1
//#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef FILESYSTEM
#include <string.h>
#endif
#include <pitrex/pitrexio-gpio.h>
#include <pitrex/bcm2835.h>
#include <vectrex/vectrexInterface.h>
#include <vectrex/osWrapper.h>

#include "e6809.h"
#include "e8910.h"
#include "e6522.h"
#include "edac.h"
#include "vecx.h"
//#include "ser.h"


enum
{
	EMU_TIMER = 20, /* the emulators heart beats at 20 milliseconds */

	DEFAULT_WIDTH = 495,
	DEFAULT_HEIGHT = 615
};
static void quit(void);


#ifdef FILESYSTEM
/* command line arguments */
static char *bios_filename = "/opt/pitrex/roms/vectrex/bios.bin"; // hard-coded path temporary for now
static char *cart_filename = NULL;
static char *overlay_filename = NULL;
static char fullscreen = 0;


static void load_bios(void)
{
	FILE *f;
	if (!(f = fopen(bios_filename, "rb")))
	{
		perror(bios_filename);
		quit();
	}
	if (fread(rom, 1, sizeof(rom), f) != sizeof(rom))
	{
		fprintf(stderr, "Invalid bios length\n");
		quit();
	}
	fclose(f);
}

static void load_cart(void)
{
	memset(cart, 0, sizeof(cart));
	if (cart_filename)
	{
		FILE *f;
		if (!(f = fopen(cart_filename, "rb")))
		{
			perror(cart_filename);
			return;
		}
		fread(cart, 1, sizeof(cart), f);
		fclose(f);
	}
}

static void load_overlay()
{
#ifdef USE_SDL
	if (overlay_filename)
	{
		SDL_Texture *image = IMG_LoadTexture(renderer, overlay_filename);
		if (image)
		{
			overlay = image;
			SDL_SetTextureBlendMode(image, SDL_BLENDMODE_BLEND);
			SDL_SetTextureAlphaMod(image, 128);
		}
		else
		{
			fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
		}
	}
#endif
}

void parse_args(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			puts("Usage: vecx [options] [cart_file]");
			puts("Options:");
			puts("  --help            Display this help message");
			puts("  --bios <file>     Load bios file");
			puts("  --overlay <file>  Load overlay file");
			puts("  --fullscreen      Launch in fullscreen mode");
			exit(0);
		}
		else if (strcmp(argv[i], "--bios") == 0 || strcmp(argv[i], "-b") == 0)
		{
			bios_filename = argv[++i];
		}
		else if (strcmp(argv[i], "--overlay") == 0 || strcmp(argv[i], "-o") == 0)
		{
			overlay_filename = argv[++i];
		}
		else if (strcmp(argv[i], "--fullscreen") == 0 || strcmp(argv[i], "-f") == 0)
		{
			fullscreen = 1;
		}
		else if (i == argc - 1)
		{
			cart_filename = argv[i];
		}
		else
		{
			printf("Unkown flag: %s\n", argv[i]);
			exit(0);
		}
	}
}
#endif

static int readevents(void)
{
	/*
vecx_input(VECTREX_PAD1_BUTTON1, currentButtonState&1);
vecx_input(VECTREX_PAD1_BUTTON2, currentButtonState&2);
vecx_input(VECTREX_PAD1_BUTTON3, currentButtonState&4);
vecx_input(VECTREX_PAD1_BUTTON4, currentButtonState&8);
*/
vecx_input(VECTREX_PAD1_X, currentJoy1X<-80?0x00:currentJoy1X>80?0xff:0x80);
vecx_input(VECTREX_PAD1_Y, currentJoy1Y<-80?0x00:currentJoy1Y>80?0xff:0x80);

/*
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_KEYDOWN:
			switch (e.key.keysym.sym)
			{
			case SDLK_ESCAPE: return 1;
			case SDLK_a: vecx_input(VECTREX_PAD1_BUTTON1, 1); break;
			case SDLK_s: vecx_input(VECTREX_PAD1_BUTTON2, 1); break;
			case SDLK_d: vecx_input(VECTREX_PAD1_BUTTON3, 1); break;
			case SDLK_f: vecx_input(VECTREX_PAD1_BUTTON4, 1); break;
			case SDLK_LEFT: vecx_input(VECTREX_PAD1_X, 0x00); break;
			case SDLK_RIGHT: vecx_input(VECTREX_PAD1_X, 0xff); break;
			case SDLK_UP: vecx_input(VECTREX_PAD1_Y, 0xff); break;
			case SDLK_DOWN: vecx_input(VECTREX_PAD1_Y, 0x00); break;
			}
			break;
		case SDL_KEYUP:
			switch (e.key.keysym.sym)
			{

			case SDLK_a: vecx_input(VECTREX_PAD1_BUTTON1, 0); break;
			case SDLK_s: vecx_input(VECTREX_PAD1_BUTTON2, 0); break;
			case SDLK_d: vecx_input(VECTREX_PAD1_BUTTON3, 0); break;
			case SDLK_f: vecx_input(VECTREX_PAD1_BUTTON4, 0); break;
			case SDLK_LEFT: vecx_input(VECTREX_PAD1_X, 0x80); break;
			case SDLK_RIGHT: vecx_input(VECTREX_PAD1_X, 0x80); break;
			case SDLK_UP: vecx_input(VECTREX_PAD1_Y, 0x80); break;
			case SDLK_DOWN: vecx_input(VECTREX_PAD1_Y, 0x80); break;
			}
			break;
		}
	}
	*/
	return 0;
}
extern uint32_t cycleCount;
uint32_t cycleCount_mark;
#define GET_SYSTEM_TIMER bcm2835_st_read()


static void emuloop(void)
{
	vecx_reset();
	cycleCount_mark = 0;
	uint32_t mark = (uint32_t)(GET_SYSTEM_TIMER&0xffffffff);
	for (;;)
	{
		vecx_emu(10000);
		uint32_t mark2 = (uint32_t)(GET_SYSTEM_TIMER&0xffffffff);

		if ( mark2 >= mark +1000000) // 1 second
		{
		  mark = mark2;
 			uint32_t percent = ((cycleCount-cycleCount_mark)*100) / (1500000);
		  printf("Emulation percent: %i\n\r", (int)percent);
		  cycleCount_mark = cycleCount;
		}
	}
}

//#define SETTINGS_SIZE 1024
//unsigned char settingsBlob[SETTINGS_SIZE];
static int init(void)
{
	vectrexinit (1);
	v_setName("vecxEmul");
	v_init();
        //v_loadSettings("vecxEmul", settingsBlob, SETTINGS_SIZE);
        return 1;
}

static void quit(void)
{
  vectrexclose ();
	exit(0);
}

static void render(void)
{
  // turn off sdl2 graphics here as unaccelerated graphics
  // are abysmally slow.  but we do want them on during debugging
  // so lookslike I need to follow instructions from
  // choccyhobnob.com/sdl2-2-0-8-on-raspberry-pi/
v_readButtons(); // read buttons to check if we should enter calibration on init
v_readJoystick1Analog(); // read buttons to check if we should enter calibration on init
v_WaitRecal();
readevents();
//printf("Count: %i\n\r", vector_draw_cnt);
		for (size_t v = 0; v < vector_draw_cnt; v++)
		{
//printf("original %i, %i, %i, %i, %d\n\r", vectors[v].x0,vectors[v].y0,vectors[v].x1,vectors[v].y1, vectors[v].color);
			uint8_t c = vectors[v].color ;//* 256 / VECTREX_COLORS;
			int32_t x0 = (vectors[v].x0 )-(DAC_MAX_X/2);
			int32_t y0 = -((vectors[v].y0 )-(DAC_MAX_Y/2));
			int32_t x1 = (vectors[v].x1 )-(DAC_MAX_Y/2) ;
			int32_t y1 = -((vectors[v].y1 )-(DAC_MAX_X/2) );
//printf("converted %i, %i, %i, %i, %d\n\r", x0,y0,x1,y1,  vectors[v].color );
 v_directDraw32(x0,y0,x1,y1, c );

		}
}

int main(int argc, char *argv[])
//void main(void)
{

	printf("Vectrex main()\n\r");
// testFileSystem();
	if (!init())
		quit();
#ifdef FILESYSTEM
	parse_args(argc, argv);


	// resize(); // resize what???
	load_bios();
	load_cart();
	load_overlay();
#endif
	e8910_init();
	vecx_render = render;

	emuloop();

	e8910_done();
	quit();
	while (1)
	{

	}
}
