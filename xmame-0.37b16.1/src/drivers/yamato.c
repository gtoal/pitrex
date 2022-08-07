/*

TODO:
- This runs on Crazy Climber hardware, should be merged with that driver.
- Two ROMs are not used, I don't know what they are.
- I'm only using the bottom half of the big sprite char ROM - there must
  be a way for the game to address both

*/


#include "driver.h"
#include "vidhrdw/generic.h"

/* in machine/segacrpt.c */
void yamato_decode(void);


extern unsigned char *cclimber_bsvideoram;
extern size_t cclimber_bsvideoram_size;
extern unsigned char *cclimber_bigspriteram;
extern unsigned char *cclimber_column_scroll;
WRITE_HANDLER( cclimber_colorram_w );
WRITE_HANDLER( cclimber_bigsprite_videoram_w );
void cclimber_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int cclimber_vh_start(void);
void cclimber_vh_stop(void);
void cclimber_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


void yamato_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + (offs)])


	/* chars - 12 bits RGB */
	for (i = 0;i < 64;i++)
	{
		int bit0,bit1,bit2,bit3;


		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[64] >> 0) & 0x01;
		bit1 = (color_prom[64] >> 1) & 0x01;
		bit2 = (color_prom[64] >> 2) & 0x01;
		bit3 = (color_prom[64] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
	color_prom += 64;

	/* big sprite - 8 bits RGB */
	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}


	/* character and sprite lookup table */
	/* they use colors 0-63 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		/* pen 0 always uses color 0 (background in River Patrol and Silver Land) */
		if (i % 4 == 0) COLOR(0,i) = 0;
		else COLOR(0,i) = i;
	}

	/* big sprite lookup table */
	/* it uses colors 64-95 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		if (i % 4 == 0) COLOR(2,i) = 0;
		else COLOR(2,i) = i + 64;
	}
}




static int p0,p1;

static WRITE_HANDLER( p0_w )
{
	p0 = data;
}
static WRITE_HANDLER( p1_w )
{
	p1 = data;
}
static READ_HANDLER( p0_r )
{
	return p0;
}
static READ_HANDLER( p1_r )
{
	return p1;
}

static WRITE_HANDLER( flip_screen_x_w )
{
	flip_screen_x_set(data);
}

static WRITE_HANDLER( flip_screen_y_w )
{
	flip_screen_y_set(data);
}


static MEMORY_READ_START( yamato_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_RAM },
	{ 0x7000, 0x7fff, MRA_ROM },
	{ 0x8800, 0x8bff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9800, 0x9bff, MRA_RAM },	/* column scroll registers */
	{ 0x9c00, 0x9fff, MRA_RAM },	/* color RAM */
	{ 0xa000, 0xa000, input_port_0_r },     /* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },     /* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },     /* DSW */
	{ 0xb800, 0xb800, input_port_3_r },     /* IN2 */
	{ 0xba00, 0xba00, input_port_4_r },     /* IN3 (maybe a mirror of b800) */
MEMORY_END

static MEMORY_WRITE_START( yamato_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_RAM },
	{ 0x7000, 0x7fff, MWA_ROM },
	{ 0x8800, 0x88ff, cclimber_bigsprite_videoram_w, &cclimber_bsvideoram, &cclimber_bsvideoram_size },
	{ 0x8900, 0x8bff, MWA_RAM },  /* not used, but initialized */
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
/*{ 0x9400, 0x97ff, videoram_w },    mirror address, used by Crazy Climber to draw windows    */
	/* 9800-9bff and 9c00-9fff share the same RAM, interleaved */
	/* (9800-981f for scroll, 9c20-9c3f for color RAM, and so on) */
	{ 0x9800, 0x981f, MWA_RAM, &cclimber_column_scroll },
	{ 0x9880, 0x989f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x98dc, 0x98df, MWA_RAM, &cclimber_bigspriteram },
	{ 0x9800, 0x9bff, MWA_RAM },  /* not used, but initialized */
	{ 0x9c00, 0x9fff, cclimber_colorram_w, &colorram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa001, 0xa001, flip_screen_x_w },
	{ 0xa002, 0xa002, flip_screen_y_w },
/*{ 0xa004, 0xa004, cclimber_sample_trigger_w }, */
/*{ 0xa800, 0xa800, cclimber_sample_rate_w }, */
/*{ 0xb000, 0xb000, cclimber_sample_volume_w }, */
MEMORY_END

static PORT_WRITE_START( yamato_writeport )
	{ 0x00, 0x00, p0_w },	/* ??? */
	{ 0x01, 0x01, p1_w },	/* ??? */
PORT_END

static MEMORY_READ_START( yamato_sound_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x5000, 0x53ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( yamato_sound_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x5000, 0x53ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( yamato_sound_readport )
	{ 0x04, 0x04, p0_r },	/* ??? */
	{ 0x08, 0x08, p1_r },	/* ??? */
PORT_END

static PORT_WRITE_START( yamato_sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x02, 0x02, AY8910_control_port_1_w },
	{ 0x03, 0x03, AY8910_write_port_1_w },
PORT_END



INPUT_PORTS_START( yamato )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Speed" )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x40, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )	/* set 1 only */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )	/* set 1 only */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters (256 in Crazy Climber) */
	2,      /* 2 bits per pixel */
	{ 0, 512*8*8 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },     /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};
static struct GfxLayout bscharlayout =
{
	8,8,    /* 8*8 characters */
	512,/*256,       256 characters    */
	2,      /* 2 bits per pixel */
	{ 0, 512*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },     /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	128,    /* 128 sprites (64 in Crazy Climber) */
	2,      /* 2 bits per pixel */
	{ 0, 128*16*16 },       /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,       /* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,      0, 16 }, /* char set #1 */
	{ REGION_GFX1, 0x2000, &charlayout,      0, 16 }, /* char set #2 */
	{ REGION_GFX2, 0x0000, &bscharlayout, 16*4,  8 }, /* big sprite char set */
	{ REGION_GFX1, 0x0000, &spritelayout,    0, 16 }, /* sprite set #1 */
	{ REGION_GFX1, 0x2000, &spritelayout,    0, 16 }, /* sprite set #2 */
	{ -1 } /* end of array */
};



static struct AY8910interface yamato_ay8910_interface =
{
	2,      /* 2 chips */
	1536000,	/* 1.536 MHz??? */
	{ 25, 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver_yamato =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 MHz ? */
			yamato_readmem,yamato_writemem,0,yamato_writeport,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 MHz ? */
			yamato_sound_readmem,yamato_sound_writemem,yamato_sound_readport,yamato_sound_writeport,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	96,16*4+8*4,
	yamato_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	cclimber_vh_start,
	cclimber_vh_stop,
	cclimber_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&yamato_ay8910_interface
		}
	}
};



ROM_START( yamato )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "2.5de",        0x0000, 0x2000, 0x20895096 )
	ROM_LOAD( "3.5f",         0x2000, 0x2000, 0x57a696f9 )
	ROM_LOAD( "4.5jh",        0x4000, 0x2000, 0x59a468e8 )
	/* hole at 6000-6fff */
	ROM_LOAD( "11.5a",        0x7000, 0x1000, 0x35987485 )

	/* I don't know what the following ROMs are! */
	ROM_LOAD( "5.5lm",        0xf000, 0x1000, 0x7761ad24 )	/* ?? */
	ROM_LOAD( "6.5n",         0xf000, 0x1000, 0xda48444c )	/* ?? */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "1.5v",         0x0000, 0x0800, 0x3aad9e3c )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "10.11k",       0x0000, 0x1000, 0x161121f5 )
	ROM_CONTINUE(             0x2000, 0x1000 )
	ROM_LOAD( "9.11h",        0x1000, 0x1000, 0x56e84cc4 )
	ROM_CONTINUE(             0x3000, 0x1000 )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	/* TODO: I'm swapping the two halves of the ROMs to use only the bottom */
	/* 256 chars. There must be a way for the game to address both halves */
	ROM_LOAD( "8.11c",        0x0800, 0x0800, 0x28024d9a )
	ROM_CONTINUE(             0x0000, 0x0800 )
	ROM_LOAD( "7.11a",        0x1800, 0x0800, 0x4a179790 )
	ROM_CONTINUE(             0x1000, 0x0800 )

	ROM_REGION( 0x00a0, REGION_PROMS, 0 )
	ROM_LOAD( "1.bpr",        0x0000, 0x0020, 0xef2053ab )
	ROM_LOAD( "2.bpr",        0x0020, 0x0020, 0x2281d39f )
	ROM_LOAD( "3.bpr",        0x0040, 0x0020, 0x9e6341e3 )
	ROM_LOAD( "4.bpr",        0x0060, 0x0020, 0x1c97dc0b )
	ROM_LOAD( "5.bpr",        0x0080, 0x0020, 0xedd6c05f )
ROM_END

ROM_START( yamato2 )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "2-2.5de",      0x0000, 0x2000, 0x93da1d52 )
	ROM_LOAD( "3-2.5f",       0x2000, 0x2000, 0x31e73821 )
	ROM_LOAD( "4-2.5jh",      0x4000, 0x2000, 0xfd7bcfc3 )
	/* hole at 6000-6fff */
	/* 7000-7fff not present here */

	/* I don't know what the following ROMs are! */
	ROM_LOAD( "5.5lm",        0xf000, 0x1000, 0x7761ad24 )	/* ?? */
	ROM_LOAD( "6.5n",         0xf000, 0x1000, 0xda48444c )	/* ?? */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "1.5v",         0x0000, 0x0800, 0x3aad9e3c )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "10.11k",       0x0000, 0x1000, 0x161121f5 )
	ROM_CONTINUE(             0x2000, 0x1000 )
	ROM_LOAD( "9.11h",        0x1000, 0x1000, 0x56e84cc4 )
	ROM_CONTINUE(             0x3000, 0x1000 )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	/* TODO: I'm swapping the two halves of the ROMs to use only the bottom */
	/* 256 chars. There must be a way for the game to address both halves */
	ROM_LOAD( "8.11c",        0x0800, 0x0800, 0x28024d9a )
	ROM_CONTINUE(             0x0000, 0x0800 )
	ROM_LOAD( "7.11a",        0x1800, 0x0800, 0x4a179790 )
	ROM_CONTINUE(             0x1000, 0x0800 )

	ROM_REGION( 0x00a0, REGION_PROMS, 0 )
	ROM_LOAD( "1.bpr",        0x0000, 0x0020, 0xef2053ab )
	ROM_LOAD( "2.bpr",        0x0020, 0x0020, 0x2281d39f )
	ROM_LOAD( "3.bpr",        0x0040, 0x0020, 0x9e6341e3 )
	ROM_LOAD( "4.bpr",        0x0060, 0x0020, 0x1c97dc0b )
	ROM_LOAD( "5.bpr",        0x0080, 0x0020, 0xedd6c05f )
ROM_END


static void init_yamato(void)
{
	yamato_decode();
}


GAME( 1983, yamato,  0,      yamato, yamato, yamato, ROT90, "Sega", "Yamato (set 1)" )
GAME( 1983, yamato2, yamato, yamato, yamato, yamato, ROT90, "Sega", "Yamato (set 2)" )
