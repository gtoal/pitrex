/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

Important!	There are two types of NeoGeo romdump - MVS & MGD2.  They are both
converted to a standard format in the vh_start routines.


Graphics information:

0x00000 - 0xdfff	: Blocks of sprite data, each 0x80 bytes:
	Each 0x80 block is made up of 0x20 double words, their format is:
	Word: Sprite number (16 bits)
	Byte: Palette number (8 bits)
	Byte: Bit 0: X flip
		  Bit 1: Y flip
		  Bit 2: Automatic animation flag (4 tiles?)
		  Bit 3: Automatic animation flag (8 tiles?)
		  Bit 4: MSB of sprite number (confirmed, Karnov_r, Mslug). See note.
		  Bit 5: MSB of sprite number (MSlug2)
		  Bit 6: MSB of sprite number (Kof97)
		  Bit 7: Unknown for now

	Each double word sprite is drawn directly underneath the previous one,
	based on the starting coordinates.

0x0e000 - 0x0ea00	: Front plane fix tiles (8*8), 2 bytes each

0x10000: Control for sprites banks, arranged in words

	Bit 0 to 3 - Y zoom LSB
	Bit 4 to 7 - Y zoom MSB (ie, 1 byte for Y zoom).
	Bit 8 to 11 - X zoom, 0xf is full size (no scale).
	Bit 12 to 15 - Unknown, probably unused

0x10400: Control for sprite banks, arranged in words

	Bit 0 to 5: Number of sprites in this bank (see note below).
	Bit 6 - If set, this bank is placed to right of previous bank (same Y-coord).
	Bit 7 to 15 - Y position for sprite bank.

0x10800: Control for sprite banks, arranged in words
	Bit 0 to 5: Unknown
	Bit 7 to 15 - X position for sprite bank.

Notes:

* If rom set has less than 0x10000 tiles then msb of tile must be ignored
(see Magician Lord).

***************************************************************************/

#include "driver.h"
#include "common.h"
#include "usrintrf.h"
#include "vidhrdw/generic.h"

/*#define NEO_DEBUG */

static data16_t *neogeo_vidram16;
static data16_t *neogeo_paletteram16;	/* pointer to 1 of the 2 palette banks */
static data16_t *neogeo_palettebank[2]; /* 0x100*16 2 byte palette entries */
static int neogeo_palette_index;
static data16_t neogeo_vidram16_modulo;
static data16_t neogeo_vidram16_offset;
static int high_tile;
static int vhigh_tile;
static int vvhigh_tile;
int no_of_tiles;
static int palette_swap_pending;
static int fix_bank;

extern data16_t *neogeo_ram16;
extern unsigned int neogeo_frame_counter;
extern int neogeo_game_fix;

void NeoMVSDrawGfx(unsigned char **line,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		int zx,int zy,const struct rectangle *clip);

static char dda_x_skip[16];
static char dda_y_skip[17];
static char full_y_skip[16]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

#ifdef NEO_DEBUG

int dotiles = 0;
int screen_offs = 0x0000;
int screen_yoffs = 0;

#endif



/******************************************************************************/

void neogeo_vh_stop(void)
{
	if (neogeo_palettebank[0])
		free (neogeo_palettebank[0]);
	neogeo_palettebank[0] = NULL;

	if (neogeo_palettebank[1])
		free (neogeo_palettebank[1]);
	neogeo_palettebank[1] = NULL;

	if (neogeo_vidram16)
		free (neogeo_vidram16);
	neogeo_vidram16 = NULL;

	if (neogeo_ram16)
		free (neogeo_ram16);
	neogeo_ram16 = NULL;
}

static int common_vh_start(void)
{
	neogeo_palettebank[0] = NULL;
	neogeo_palettebank[1] = NULL;
	neogeo_vidram16 = NULL;

	neogeo_palettebank[0] = malloc(0x2000);
	if (!neogeo_palettebank[0])
	{
		neogeo_vh_stop();
		return 1;
	}

	neogeo_palettebank[1] = malloc(0x2000);
	if (!neogeo_palettebank[1])
	{
		neogeo_vh_stop();
		return 1;
	}

	/* 0x20000 bytes even though only 0x10c00 is used */
	neogeo_vidram16 = malloc(0x20000);
	if (!neogeo_vidram16)
	{
		neogeo_vh_stop();
		return 1;
	}
	memset(neogeo_vidram16,0,0x20000);

	neogeo_paletteram16 = neogeo_palettebank[0];
	neogeo_palette_index = 0;
	neogeo_vidram16_modulo = 1;
	neogeo_vidram16_offset = 0;
	fix_bank = 0;
	palette_swap_pending = 0;

	return 0;
}

#define MEMCPY128(DST,SRC) \
{ \
    unsigned int *src = (unsigned int *)SRC; \
    unsigned int *dest = (unsigned int *)DST; \
 \
    dest[0]=src[0]; \
    dest[1]=src[1]; \
    dest[2]=src[2]; \
    dest[3]=src[3]; \
    dest[4]=src[4]; \
    dest[5]=src[5]; \
    dest[6]=src[6]; \
    dest[7]=src[7]; \
    dest[8]=src[8]; \
    dest[9]=src[9]; \
    dest[10]=src[10]; \
    dest[11]=src[11]; \
    dest[12]=src[12]; \
    dest[13]=src[13]; \
    dest[14]=src[14]; \
    dest[15]=src[15]; \
    dest[16]=src[16]; \
    dest[17]=src[17]; \
    dest[18]=src[18]; \
    dest[19]=src[19]; \
    dest[20]=src[20]; \
    dest[21]=src[21]; \
    dest[22]=src[22]; \
    dest[23]=src[23]; \
    dest[24]=src[24]; \
    dest[25]=src[25]; \
    dest[26]=src[26]; \
    dest[27]=src[27]; \
    dest[28]=src[28]; \
    dest[29]=src[29]; \
    dest[30]=src[30]; \
    dest[31]=src[31]; \
}

static void decodetile(int tileno)
{
	unsigned char swap[128] __attribute__ ((__aligned__ (4)));
	UINT32 *gfxdata;
	int x,y;
	unsigned int pen;

	gfxdata = (UINT32 *)&memory_region(REGION_GFX2)[tileno<<7];

	//memcpy(swap,gfxdata,128);
	MEMCPY128(swap,gfxdata);

	for (y = 0;y < 16;y++)
	{
		UINT32 dw;

		dw = 0;
		for (x = 0;x < 8;x++)
		{
			pen  = ((swap[64 + (y<<2) + 3] >> x) & 1) << 3;
			pen |= ((swap[64 + (y<<2) + 1] >> x) & 1) << 2;
			pen |= ((swap[64 + (y<<2) + 2] >> x) & 1) << 1;
			pen |=  (swap[64 + (y<<2)    ] >> x) & 1;
			dw |= pen << ((7-x)<<2);
			Machine->gfx[2]->pen_usage[tileno] |= (1 << pen);
		}
		*(gfxdata++) = dw;

		dw = 0;
		for (x = 0;x < 8;x++)
		{
			pen  = ((swap[(y<<2) + 3] >> x) & 1) << 3;
			pen |= ((swap[(y<<2) + 1] >> x) & 1) << 2;
			pen |= ((swap[(y<<2) + 2] >> x) & 1) << 1;
			pen |=  (swap[(y<<2)    ] >> x) & 1;
			dw |= pen << ((7-x)<<2);
			Machine->gfx[2]->pen_usage[tileno] |= (1 << pen);
		}
		*(gfxdata++) = dw;
	}
}

int neogeo_mvs_vh_start(void)
{
	no_of_tiles=memory_region_length(REGION_GFX2)/128;
	if (no_of_tiles>0x10000) high_tile=1; else high_tile=0;
	if (no_of_tiles>0x20000) vhigh_tile=1; else vhigh_tile=0;
	if (no_of_tiles>0x40000) vvhigh_tile=1; else vvhigh_tile=0;
	Machine->gfx[2]->total_elements = no_of_tiles;
	if (Machine->gfx[2]->pen_usage)
		free(Machine->gfx[2]->pen_usage);
	Machine->gfx[2]->pen_usage = malloc(no_of_tiles * sizeof(int));
	memset(Machine->gfx[2]->pen_usage,0,no_of_tiles * sizeof(int));

	/* tiles are not decoded yet. They will be decoded later as they are used. */
	/* pen_usage is used as a marker of decoded tiles: if it is 0, then the tile */
	/* hasn't been decoded yet. */

	return common_vh_start();
}

/******************************************************************************/

static void swap_palettes(void)
{
	int i;

	for (i = 0; i < 0x2000 >> 1; i++)
	{
		data16_t newword = neogeo_paletteram16[i];
		int r,g,b;

		r = ((newword >> 7) & 0x1e) | ((newword >> 14) & 0x01);
		g = ((newword >> 3) & 0x1e) | ((newword >> 13) & 0x01);
		b = ((newword << 1) & 0x1e) | ((newword >> 12) & 0x01);

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_change_color(i, r, g, b);
	}

	palette_swap_pending = 0;
}

static void neogeo_setpalbank(int n)
{
	if (neogeo_palette_index != n)
	{
		neogeo_palette_index = n;
		neogeo_paletteram16 = neogeo_palettebank[n];
		palette_swap_pending = 1;
	}
}

WRITE16_HANDLER( neogeo_setpalbank0_16_w )
{
	neogeo_setpalbank(0);
}

WRITE16_HANDLER( neogeo_setpalbank1_16_w )
{
	neogeo_setpalbank(1);
}

READ16_HANDLER( neogeo_paletteram16_r )
{
	return neogeo_paletteram16[offset];
}

WRITE16_HANDLER( neogeo_paletteram16_w )
{
	data16_t oldword, newword;
	int r,g,b;

	oldword = newword = neogeo_paletteram16[offset];
	COMBINE_DATA(&newword);

	if (oldword == newword)
		return;

	neogeo_paletteram16[offset] = newword;

	r = ((newword >> 7) & 0x1e) | ((newword >> 14) & 0x01);
	g = ((newword >> 3) & 0x1e) | ((newword >> 13) & 0x01) ;
	b = ((newword << 1) & 0x1e) | ((newword >> 12) & 0x01) ;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(offset, r, g, b);
}

/******************************************************************************/

static const UINT8 *neogeo_palette(const struct rectangle *clip)
{
	int color,code,pal_base,y,my=0,count,offs,i;
	int colmask[256];
	unsigned int *pen_usage; /* Save some struct derefs */

	int sx =0,sy =0,oy =0,zx = 1, rzy = 1;
	int tileno,tileatr,t1,t2,t3;
	char fullmode=0;
	int ddax=0,dday=0,rzx=15,yskip=0;

	if (Machine->scrbitmap->depth == 16 || !palette_used_colors)
	{
		return palette_recalc();
	}

	palette_init_used_colors();

	/* character foreground */
	pen_usage = Machine->gfx[fix_bank]->pen_usage;
	pal_base = Machine->drv->gfxdecodeinfo[fix_bank].color_codes_start;
	for (color = 0; color < 16; color++)
		colmask[color] = 0;
	for (offs = 0xe000 >> 1; offs < 0xea00 >> 1; offs++)
	{
		code = neogeo_vidram16[offs];
		color = code >> 12;
		colmask[color] |= pen_usage[code&0xfff];
	}
	for (color = 0; color < 16; color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_VISIBLE;
		}
	}

	/* Tiles */
	pen_usage= Machine->gfx[2]->pen_usage;
	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	for (color = 0; color < 256; color++)
		colmask[color] = 0;
	for (count = 0; count < 0x300 >> 1; count++)
	{
		t3 = neogeo_vidram16[(0x10000 >> 1) + count];
		t1 = neogeo_vidram16[(0x10400 >> 1) + count];
		t2 = neogeo_vidram16[(0x10800 >> 1) + count];

		/* If this bit is set this new column is placed next to last one */
		if (t1 & 0x40)
		{
			sx += rzx;
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			/* Get new zoom for this column */
			zx = (t3 >> 8) & 0x0f;
			sy = oy;
		}
		else	/* nope it is a new block */
		{
			/* Sprite scaling */
			zx = (t3 >> 8) & 0x0f;
			rzy = t3 & 0xff;

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			/* Number of tiles in this strip */
			my = t1 & 0x3f;
			if (my == 0x20) fullmode = 1;
			else if (my >= 0x21) fullmode = 2;	/* most games use 0x21, but */
												/* Alpha Mission II uses 0x3f */
			else fullmode = 0;

			sy = 0x200 - (t1 >> 7);
			if (clip->max_y - clip->min_y > 8 ||	/* kludge to improve the ssideki games */
					clip->min_y == Machine->visible_area.min_y)
			{
				if (sy > 0x110) sy -= 0x200;
				if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
				{
					while (sy < 0) sy += 2 * (rzy + 1);
				}
			}
			oy = sy;

			if (rzy < 0xff && my < 0x10 && my)
			{
				my = ((my*16*256)/(rzy+1)+15)/16;
				if (my > 0x10) my = 0x10;
			}
			if (my > 0x20) my=0x20;

			ddax=0; /* =16; NS990110 neodrift fix */		/* setup x zoom */
		}

		/* No point doing anything if tile strip is 0 */
		if (my==0) continue;

		/* Process x zoom */
		if (zx != 15)
		{
			rzx = 0;
			for (i = 0; i < 16; i++)
			{
				ddax -= zx+1;
				if (ddax <= 0)
				{
					ddax += 15+1;
					rzx++;
				}
			}
		}
		else rzx=16;

		if (sx >= 320)
			continue;

		/* Setup y zoom */
		if (rzy == 255)
			yskip=16;
		else
			dday=0; 	/* =256; NS990105 mslug fix */

		offs = count << 6;

		/* my holds the number of tiles in each vertical multisprite block */
		for (y=0; y < my ;y++)
		{
			tileno	= neogeo_vidram16[offs++];
			tileatr = neogeo_vidram16[offs++];

			if (high_tile && tileatr&0x10) tileno+=0x10000;
			if (vhigh_tile && tileatr&0x20) tileno+=0x20000;
			if (vvhigh_tile && tileatr&0x40) tileno+=0x40000;

			if (tileatr&0x8) tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
			else if (tileatr&0x4) tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);

			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				if (sy >= 248) sy -= 2 * (rzy + 1);
			}
			else if (fullmode == 1)
			{
				if (y == 0x10) sy -= 2 * (rzy + 1);
			}
			else if (sy > 0x110) sy -= 0x200;	/* NS990105 mslug2 fix */

			if (rzy != 255)
			{
				yskip = 0;
				for (i = 0; i < 16; i++)
				{
					dday -= rzy+1;
					if (dday <= 0)
					{
						dday += 256;
						yskip++;
					}
				}
			}

			if (sy+yskip-1 >= clip->min_y && sy <= clip->max_y)
			{
				tileatr = tileatr >> 8;
				tileno %= no_of_tiles;
				if (pen_usage[tileno] == 0) /* decode tile if it hasn't been yet */
					decodetile(tileno);
				colmask[tileatr] |= pen_usage[tileno];
			}

			sy +=yskip;

		}  /* for y */
	}  /* for count */

	for (color = 0; color < 256; color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_VISIBLE;
		}
	}

	palette_used_colors[4095] = PALETTE_COLOR_VISIBLE;

	return palette_recalc();
}

/******************************************************************************/

WRITE16_HANDLER( neogeo_vidram16_offset_w )
{
	COMBINE_DATA(&neogeo_vidram16_offset);
}

READ16_HANDLER( neogeo_vidram16_data_r )
{
	return neogeo_vidram16[neogeo_vidram16_offset];
}

WRITE16_HANDLER( neogeo_vidram16_data_w )
{
	COMBINE_DATA(&neogeo_vidram16[neogeo_vidram16_offset]);
	neogeo_vidram16_offset += neogeo_vidram16_modulo;
	neogeo_vidram16_offset &= 0xffff;
}

/* Modulo can become negative , Puzzle Bobble Super Sidekicks and a lot */
/* of other games use this */
WRITE16_HANDLER( neogeo_vidram16_modulo_w )
{
	COMBINE_DATA(&neogeo_vidram16_modulo);
}

READ16_HANDLER( neogeo_vidram16_modulo_r )
{
	return neogeo_vidram16_modulo;
}


WRITE16_HANDLER( neo_board_fix_16_w )
{
	fix_bank = 1;
}

WRITE16_HANDLER( neo_game_fix_16_w )
{
	fix_bank = 0;
}

/******************************************************************************/


void NeoMVSDrawGfx(unsigned char **line,const struct GfxElement *gfx, /* AJP */
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		int zx,int zy,const struct rectangle *clip)
{
	int oy,ey,y,dy;
	unsigned char *bm;
	int col;
	int l; /* Line skipping counter */

	int mydword;

	UINT32 *fspr = (UINT32 *)memory_region(REGION_GFX2);

	char *l_y_skip;


	/* Mish/AJP - Most clipping is done in main loop */
	oy = sy;
	ey = sy + zy -1;	/* Clip for size of zoomed object */

	if (sy < clip->min_y) sy = clip->min_y;
	if (ey >= clip->max_y) ey = clip->max_y;
	if (sx <= -16) return;

	/* Safety feature */
	code %= no_of_tiles;

	if (gfx->pen_usage[code] == 0)	/* decode tile if it hasn't been yet */
		decodetile(code);

	/* Check for total transparency, no need to draw */
	if ((gfx->pen_usage[code] & ~1) == 0)
		return;

	if (zy==16)
		 l_y_skip=full_y_skip;
	else
		 l_y_skip=dda_y_skip;

	if (flipy)	/* Y flip */
	{
		dy = -2;
		fspr+=(code+1)*32 - 2 - (sy-oy)*2;
	}
	else		/* normal */
	{
		dy = 2;
		fspr+=code*32 + (sy-oy)*2;
	}

	{
		const UINT32 *paldata;	/* ASG 980209 */
		paldata = &gfx->colortable[gfx->color_granularity * color];
		if (flipx)	/* X flip */
		{
			l=0;
			if (zx == 16)
			{
				for (y = sy;y <= ey;y++)
				{
					bm	= line[y]+sx;

					fspr+=l_y_skip[l]*dy;

					mydword = fspr[1];
					col = (mydword>> 0)&0xf; if (col) bm[ 0] = paldata[col];
					col = (mydword>> 4)&0xf; if (col) bm[ 1] = paldata[col];
					col = (mydword>> 8)&0xf; if (col) bm[ 2] = paldata[col];
					col = (mydword>>12)&0xf; if (col) bm[ 3] = paldata[col];
					col = (mydword>>16)&0xf; if (col) bm[ 4] = paldata[col];
					col = (mydword>>20)&0xf; if (col) bm[ 5] = paldata[col];
					col = (mydword>>24)&0xf; if (col) bm[ 6] = paldata[col];
					col = (mydword>>28)&0xf; if (col) bm[ 7] = paldata[col];

					mydword = fspr[0];
					col = (mydword>> 0)&0xf; if (col) bm[ 8] = paldata[col];
					col = (mydword>> 4)&0xf; if (col) bm[ 9] = paldata[col];
					col = (mydword>> 8)&0xf; if (col) bm[10] = paldata[col];
					col = (mydword>>12)&0xf; if (col) bm[11] = paldata[col];
					col = (mydword>>16)&0xf; if (col) bm[12] = paldata[col];
					col = (mydword>>20)&0xf; if (col) bm[13] = paldata[col];
					col = (mydword>>24)&0xf; if (col) bm[14] = paldata[col];
					col = (mydword>>28)&0xf; if (col) bm[15] = paldata[col];

					l++;
				}
			}
			else
			{
				for (y = sy;y <= ey;y++)
				{
					bm	= line[y]+sx;
					fspr+=l_y_skip[l]*dy;

					mydword = fspr[1];
					if (dda_x_skip[ 0]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 1]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 2]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 3]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 4]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 5]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 6]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 7]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }

					mydword = fspr[0];
					if (dda_x_skip[ 8]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 9]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[10]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[11]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[12]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[13]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[14]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[15]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }

					l++;
				}
			}
		}
		else		/* normal */
		{
			l=0;
			if (zx == 16)
			{
				for (y = sy ;y <= ey;y++)
				{
					bm	= line[y] + sx;
					fspr+=l_y_skip[l]*dy;

					mydword = fspr[0];
					col = (mydword>>28)&0xf; if (col) bm[ 0] = paldata[col];
					col = (mydword>>24)&0xf; if (col) bm[ 1] = paldata[col];
					col = (mydword>>20)&0xf; if (col) bm[ 2] = paldata[col];
					col = (mydword>>16)&0xf; if (col) bm[ 3] = paldata[col];
					col = (mydword>>12)&0xf; if (col) bm[ 4] = paldata[col];
					col = (mydword>> 8)&0xf; if (col) bm[ 5] = paldata[col];
					col = (mydword>> 4)&0xf; if (col) bm[ 6] = paldata[col];
					col = (mydword>> 0)&0xf; if (col) bm[ 7] = paldata[col];

					mydword = fspr[1];
					col = (mydword>>28)&0xf; if (col) bm[ 8] = paldata[col];
					col = (mydword>>24)&0xf; if (col) bm[ 9] = paldata[col];
					col = (mydword>>20)&0xf; if (col) bm[10] = paldata[col];
					col = (mydword>>16)&0xf; if (col) bm[11] = paldata[col];
					col = (mydword>>12)&0xf; if (col) bm[12] = paldata[col];
					col = (mydword>> 8)&0xf; if (col) bm[13] = paldata[col];
					col = (mydword>> 4)&0xf; if (col) bm[14] = paldata[col];
					col = (mydword>> 0)&0xf; if (col) bm[15] = paldata[col];

					l++;
				}
			}
			else
			{
				for (y = sy ;y <= ey;y++)
				{
					bm	= line[y] + sx;
					fspr+=l_y_skip[l]*dy;

					mydword = fspr[0];
					if (dda_x_skip[ 0]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 1]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 2]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 3]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 4]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 5]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 6]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 7]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }

					mydword = fspr[1];
					if (dda_x_skip[ 8]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 9]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[10]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[11]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[12]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[13]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[14]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[15]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }

					l++;
				}
			}
		}
	}
}

void NeoMVSDrawGfx16(unsigned char **line,const struct GfxElement *gfx, /* AJP */
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		int zx,int zy,const struct rectangle *clip)
{
	int oy,ey,y,dy;
	unsigned short *bm;
	int col;
	int l; /* Line skipping counter */

	int mydword;

	UINT32 *fspr = (UINT32 *)memory_region(REGION_GFX2);

	char *l_y_skip;


	/* Mish/AJP - Most clipping is done in main loop */
	oy = sy;
	ey = sy + zy -1;	/* Clip for size of zoomed object */

	if (sy < clip->min_y) sy = clip->min_y;
	if (ey >= clip->max_y) ey = clip->max_y;
	if (sx <= -16) return;

	/* Safety feature */
	code %= no_of_tiles;

	if (gfx->pen_usage[code] == 0)	/* decode tile if it hasn't been yet */
		decodetile(code);

	/* Check for total transparency, no need to draw */
	if ((gfx->pen_usage[code] & ~1) == 0)
		return;

	if (zy == 16)
		 l_y_skip=full_y_skip;
	else
		 l_y_skip=dda_y_skip;

	if (flipy)	/* Y flip */
	{
		dy = -2;
		fspr+=(code+1)*32 - 2 - (sy-oy)*2;
	}
	else		/* normal */
	{
		dy = 2;
		fspr+=code*32 + (sy-oy)*2;
	}

	{
		const UINT32 *paldata;	/* ASG 980209 */
		paldata = &gfx->colortable[gfx->color_granularity * color];
		if (flipx)	/* X flip */
		{
			l=0;
			if (zx == 16)
			{
				for (y = sy;y <= ey;y++)
				{
					bm	= ((unsigned short *)line[y])+sx;

					fspr+=l_y_skip[l]*dy;

					mydword = fspr[1];
					col = (mydword>> 0)&0xf; if (col) bm[ 0] = paldata[col];
					col = (mydword>> 4)&0xf; if (col) bm[ 1] = paldata[col];
					col = (mydword>> 8)&0xf; if (col) bm[ 2] = paldata[col];
					col = (mydword>>12)&0xf; if (col) bm[ 3] = paldata[col];
					col = (mydword>>16)&0xf; if (col) bm[ 4] = paldata[col];
					col = (mydword>>20)&0xf; if (col) bm[ 5] = paldata[col];
					col = (mydword>>24)&0xf; if (col) bm[ 6] = paldata[col];
					col = (mydword>>28)&0xf; if (col) bm[ 7] = paldata[col];

					mydword = fspr[0];
					col = (mydword>> 0)&0xf; if (col) bm[ 8] = paldata[col];
					col = (mydword>> 4)&0xf; if (col) bm[ 9] = paldata[col];
					col = (mydword>> 8)&0xf; if (col) bm[10] = paldata[col];
					col = (mydword>>12)&0xf; if (col) bm[11] = paldata[col];
					col = (mydword>>16)&0xf; if (col) bm[12] = paldata[col];
					col = (mydword>>20)&0xf; if (col) bm[13] = paldata[col];
					col = (mydword>>24)&0xf; if (col) bm[14] = paldata[col];
					col = (mydword>>28)&0xf; if (col) bm[15] = paldata[col];

					l++;
				}
			}
			else
			{
				for (y = sy;y <= ey;y++)
				{
					bm	= ((unsigned short *)line[y])+sx;
					fspr+=l_y_skip[l]*dy;

					mydword = fspr[1];
					if (dda_x_skip[ 0]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 1]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 2]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 3]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 4]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 5]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 6]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 7]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }

					mydword = fspr[0];
					if (dda_x_skip[ 8]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 9]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[10]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[11]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[12]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[13]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[14]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[15]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }

					l++;
				}
			}
		}
		else		/* normal */
		{
			l=0;
			if (zx == 16)
			{
				for (y = sy ;y <= ey;y++)
				{
					bm	= ((unsigned short *)line[y]) + sx;
					fspr+=l_y_skip[l]*dy;

					mydword = fspr[0];
					col = (mydword>>28)&0xf; if (col) bm[ 0] = paldata[col];
					col = (mydword>>24)&0xf; if (col) bm[ 1] = paldata[col];
					col = (mydword>>20)&0xf; if (col) bm[ 2] = paldata[col];
					col = (mydword>>16)&0xf; if (col) bm[ 3] = paldata[col];
					col = (mydword>>12)&0xf; if (col) bm[ 4] = paldata[col];
					col = (mydword>> 8)&0xf; if (col) bm[ 5] = paldata[col];
					col = (mydword>> 4)&0xf; if (col) bm[ 6] = paldata[col];
					col = (mydword>> 0)&0xf; if (col) bm[ 7] = paldata[col];

					mydword = fspr[1];
					col = (mydword>>28)&0xf; if (col) bm[ 8] = paldata[col];
					col = (mydword>>24)&0xf; if (col) bm[ 9] = paldata[col];
					col = (mydword>>20)&0xf; if (col) bm[10] = paldata[col];
					col = (mydword>>16)&0xf; if (col) bm[11] = paldata[col];
					col = (mydword>>12)&0xf; if (col) bm[12] = paldata[col];
					col = (mydword>> 8)&0xf; if (col) bm[13] = paldata[col];
					col = (mydword>> 4)&0xf; if (col) bm[14] = paldata[col];
					col = (mydword>> 0)&0xf; if (col) bm[15] = paldata[col];

					l++;
				}
			}
			else
			{
				for (y = sy ;y <= ey;y++)
				{
					bm	= ((unsigned short *)line[y]) + sx;
					fspr+=l_y_skip[l]*dy;

					mydword = fspr[0];
					if (dda_x_skip[ 0]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 1]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 2]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 3]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 4]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 5]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 6]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 7]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }

					mydword = fspr[1];
					if (dda_x_skip[ 8]) { col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[ 9]) { col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[10]) { col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[11]) { col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[12]) { col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[13]) { col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[14]) { col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++; }
					if (dda_x_skip[15]) { col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++; }

					l++;
				}
			}
		}
	}
}



/******************************************************************************/

static void screenrefresh(struct osd_bitmap *bitmap,const struct rectangle *clip)
{
	int sx =0,sy =0,oy =0,my =0,zx = 1, rzy = 1;
	int offs,i,count,y,x;
	int tileno,tileatr,t1,t2,t3;
	char fullmode=0;
	int ddax=0,dday=0,rzx=15,yskip=0;
	unsigned char **line=bitmap->line;
	unsigned int *pen_usage;
	struct GfxElement *gfx=Machine->gfx[2]; /* Save constant struct dereference */

	#ifdef NEO_DEBUG
	char buf[80];

	/* debug setting, tile view mode connected to '8' */
	if (keyboard_pressed_memory(KEYCODE_8))
		dotiles ^= 1;

	/* tile view - 0x80, connected to '9' */
	if (keyboard_pressed(KEYCODE_9) && !keyboard_pressed(KEYCODE_LSHIFT))
	{
		if (screen_offs > 0)
			screen_offs -= 0x80;
	}
	if (keyboard_pressed(KEYCODE_9) && keyboard_pressed(KEYCODE_LSHIFT))
	{
		if (screen_yoffs > 0)
			screen_yoffs--;
	}

	/* tile view + 0x80, connected to '0' */
	if (keyboard_pressed(KEYCODE_0) && !keyboard_pressed(KEYCODE_LSHIFT))
	{
		if (screen_offs < 0x10000)
			screen_offs += 0x80;
	}
	if (keyboard_pressed(KEYCODE_0) && keyboard_pressed(KEYCODE_LSHIFT))
	{
		screen_yoffs++;
	}
	#endif

	/* Palette swap occured after last frame but before this one */
	if (palette_swap_pending) swap_palettes();

	/* Do compressed palette stuff */
	neogeo_palette(clip);
	/* no need to check the return code since we redraw everything each frame */

	fillbitmap(bitmap,Machine->pens[4095],clip);

#ifdef NEO_DEBUG
if (!dotiles) { 					/* debug */
#endif

	/* Draw sprites */
	for (count = 0; count < 0x300 >> 1; count++)
	{
		t3 = neogeo_vidram16[(0x10000 >> 1) + count];
		t1 = neogeo_vidram16[(0x10400 >> 1) + count];
		t2 = neogeo_vidram16[(0x10800 >> 1) + count];

		/* If this bit is set this new column is placed next to last one */
		if (t1 & 0x40)
		{
			sx += rzx;
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			/* Get new zoom for this column */
			zx = (t3 >> 8) & 0x0f;
			sy = oy;
		}
		else /* nope it is a new block */
		{
			/* Sprite scaling */
			zx = (t3 >> 8) & 0x0f;
			rzy = t3 & 0xff;

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			/* Number of tiles in this strip */
			my = t1 & 0x3f;
			if (my == 0x20) fullmode = 1;
			else if (my >= 0x21) fullmode = 2;	/* most games use 0x21, but */
												/* Alpha Mission II uses 0x3f */
			else fullmode = 0;

			sy = 0x200 - (t1 >> 7);
			if (clip->max_y - clip->min_y > 8 ||	/* kludge to improve the ssideki games */
					clip->min_y == Machine->visible_area.min_y)
			{
				if (sy > 0x110) sy -= 0x200;
				if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
				{
					while (sy < 0) sy += 2 * (rzy + 1);
				}
			}
			oy = sy;

			if (rzy < 0xff && my < 0x10 && my)
			{
				my = ((my*16*256)/(rzy+1)+15)/16;
				if (my > 0x10) my = 0x10;
			}
			if (my > 0x20) my=0x20;

			ddax=0; /* =16; NS990110 neodrift fix */		/* setup x zoom */
		}

		/* No point doing anything if tile strip is 0 */
		if (my==0) continue;

		/* Process x zoom */
		if (zx != 15)
		{
			rzx = 0;
			for(i = 0; i < 16; i++)
			{
				ddax -= zx+1;
				if (ddax <= 0)
				{
					ddax += 15+1;
					dda_x_skip[i] = 1;
					rzx++;
				}
				else dda_x_skip[i] = 0;
			}
		}
		else rzx=16;

		if (sx >= 320)
			continue;

		/* Setup y zoom */
		if (rzy == 255)
			yskip=16;
		else
			dday=0; /* =256; NS990105 mslug fix */

		offs = count<<6;

		/* my holds the number of tiles in each vertical multisprite block */
		for (y=0; y < my ;y++)
		{
			tileno	= neogeo_vidram16[offs++];
			tileatr = neogeo_vidram16[offs++];

			if (high_tile && tileatr&0x10) tileno+=0x10000;
			if (vhigh_tile && tileatr&0x20) tileno+=0x20000;
			if (vvhigh_tile && tileatr&0x40) tileno+=0x40000;

			if (tileatr&0x8) tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
			else if (tileatr&0x4) tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);

			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				if (sy >= 248) sy -= 2 * (rzy + 1);
			}
			else if (fullmode == 1)
			{
				if (y == 0x10) sy -= 2 * (rzy + 1);
			}
			else if (sy > 0x110) sy -= 0x200;	/* NS990105 mslug2 fix */

			if (rzy != 255)
			{
				yskip=0;
				dda_y_skip[0]=0;
				for(i = 0; i < 16; i++)
				{
					dda_y_skip[i+1] = 0;
					dday -= rzy+1;
					if (dday <= 0)
					{
						dday += 256;
						yskip++;
						dda_y_skip[yskip]++;
					}
					else dda_y_skip[yskip]++;
				}
			}

			if (sy+15 >= clip->min_y && sy <= clip->max_y)
			{
				if (Machine->scrbitmap->depth != 8)
					NeoMVSDrawGfx16(line,
						gfx,
						tileno,
						tileatr >> 8,
						tileatr & 0x01,tileatr & 0x02,
						sx,sy,rzx,yskip,
						clip
					);
				else
					NeoMVSDrawGfx(line,
						gfx,
						tileno,
						tileatr >> 8,
						tileatr & 0x01,tileatr & 0x02,
						sx,sy,rzx,yskip,
						clip
					);
			}

			sy +=yskip;
		}  /* for y */
	}  /* for count */



	/* Save some struct de-refs */
	gfx = Machine->gfx[fix_bank];
	pen_usage=gfx->pen_usage;

	/* Character foreground */
	for (y=clip->min_y / 8; y <= clip->max_y / 8; y++)
	{
		for (x = 0; x < 40; x++)
		{

			int byte1 = neogeo_vidram16[(0xe000 >> 1) + y + 32 * x];
			int byte2 = byte1 >> 12;
			byte1 = byte1 & 0xfff;

			if ((pen_usage[byte1] & ~1) == 0) continue;

			drawgfx(bitmap,gfx,
					byte1,
					byte2,
					0,0,
					x*8,y*8,
					clip,TRANSPARENCY_PEN,0);
		}
	}



#ifdef NEO_DEBUG
	}
	else	/* debug */
	{
		offs = screen_offs;
		for (y = screen_yoffs; y < screen_yoffs+15; y++)
		{
			for (x = 0; x < 20; x++)
			{
				tileno = neogeo_vidram[(offs + 4*y+x*128) >> 1];
				tileatr = neogeo_vidram16[(offs + 4*y+x*128+2) >> 1];

				if (high_tile && tileatr&0x10) tileno+=0x10000;
				if (vhigh_tile && tileatr&0x20) tileno+=0x20000;
				if (vvhigh_tile && tileatr&0x40) tileno+=0x40000;

				if (tileatr&0x8) tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
				else if (tileatr&0x4) tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);

				NeoMVSDrawGfx(line,
					Machine->gfx[2],
					tileno,
					tileatr >> 8,
					tileatr & 0x01,tileatr & 0x02,
					x*16,(y-screen_yoffs+1)*16,16,16,
					&Machine->visible_area
				 );


			}
		}

{
	int j;
	sprintf(buf,"%04X",screen_offs+4*screen_yoffs);
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],UI_COLOR_NORMAL,0,0,3*8+8*j,8*2,0,TRANSPARENCY_NONE,0);
}
if (keyboard_pressed(KEYCODE_D))
{
	FILE *fp;

	fp = fopen("video.dmp","wb");
	if (fp)
	{
		fwrite(neogeo_vidram16, 0x10c00, 1 ,fp);
		fclose(fp);
	}
}
	}	/* debug */
#endif

#ifdef NEO_DEBUG
/* More debug stuff :) */
{



	int j;
	char mybuf[20];
	struct osd_bitmap *mybitmap = Machine->scrbitmap;



  /*
for (i = 0;i < 8 >> 1; i+++)
{
	sprintf(mybuf,"%04X",neogeo_vidram16[(0x100a0 >> 1) + i]));
	for (j = 0;j < 4;j++)
		drawgfx(mybitmap,Machine->uifont,mybuf[j],UI_COLOR_NORMAL,0,0,3*8*i+8*j,8*5,0,TRANSPARENCY_NONE,0);
}


	sprintf(mybuf,"%04X",neogeo_vidram16[0x10002 >> 1]);
	for (j = 0;j < 4;j++)
		drawgfx(mybitmap,Machine->uifont,mybuf[j],UI_COLOR_NORMAL,0,0,8*j+4*8,8*7,0,TRANSPARENCY_NONE,0);
	sprintf(mybuf,"%04X",0x200-(neogeo_vidram16[0x10402 >> 1] >> 7));
	for (j = 0;j < 4;j++)
		drawgfx(mybitmap,Machine->uifont,mybuf[j],UI_COLOR_NORMAL,0,0,8*j+10*8,8*7,0,TRANSPARENCY_NONE,0);
	sprintf(mybuf,"%04X",neogeo_vidram16[0x10802 >> 1] >> 7);
	for (j = 0;j < 4;j++)
		drawgfx(mybitmap,Machine->uifont,mybuf[j],UI_COLOR_NORMAL,0,0,8*j+16*8,8*7,0,TRANSPARENCY_NONE,0);

*/


/*logerror("X: %04x Y: %04x Video: %04x\n",neogeo_vidram16[0x1089c >> 1], neogeo_vidram16[0x1049c >> 1], neogeo_vidram16[0x1009c >> 1]); */

/*logerror("X: %04x Y: %04x Video: %04x\n",neogeo_vidram16[0x10930 >> 1], neogeo_vidram16[0x10530 >> 1], neogeo_vidram16[0x10130 >> 1]); */


}
#endif

}

void neogeo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	screenrefresh(bitmap,&Machine->visible_area);
}

static int next_update_first_line;
extern int neogeo_raster_enable;

void neogeo_vh_raster_partial_refresh(struct osd_bitmap *bitmap,int current_line)
{
	struct rectangle clip;

	if (current_line < next_update_first_line)
		next_update_first_line = 0;

	clip.min_x = Machine->visible_area.min_x;
	clip.max_x = Machine->visible_area.max_x;
	clip.min_y = next_update_first_line;
	clip.max_y = current_line;
	if (clip.min_y < Machine->visible_area.min_y)
		clip.min_y = Machine->visible_area.min_y;
	if (clip.max_y > Machine->visible_area.max_y)
		clip.max_y = Machine->visible_area.max_y;

	if (clip.max_y >= clip.min_y)
	{
/*logerror("refresh %d-%d\n",clip.min_y,clip.max_y); */
		screenrefresh(bitmap,&clip);

		if (neogeo_raster_enable == 2)
		{
			int x;
			for (x = clip.min_x;x <= clip.max_x;x++)
			{
				if (x & 8)
					plot_pixel(bitmap,x,clip.max_y,Machine->uifont->colortable[1]);
			}
		}
	}

	next_update_first_line = current_line + 1;

}

void neogeo_vh_raster_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
}

