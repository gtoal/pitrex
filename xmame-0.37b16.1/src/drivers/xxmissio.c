/*****************************************************************************

XX Mission (c) 1986 UPL

	Driver by Uki

	31/Mar/2001 -

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void xxmissio_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern UINT8 *xxmissio_fgram;
extern size_t xxmissio_fgram_size;

static UINT8 *shared_workram;

static UINT8 xxmissio_status;


WRITE_HANDLER( xxmissio_scroll_x_w );
WRITE_HANDLER( xxmissio_scroll_y_w );
WRITE_HANDLER( xxmissio_flipscreen_w );

READ_HANDLER( xxmissio_videoram_r );
WRITE_HANDLER( xxmissio_videoram_w );
READ_HANDLER( xxmissio_fgram_r );
WRITE_HANDLER( xxmissio_fgram_w );

WRITE_HANDLER( xxmissio_paletteram_w );

WRITE_HANDLER( shared_workram_w )
{
	shared_workram[offset ^ 0x1000] = data;
}

READ_HANDLER( shared_workram_r )
{
	return shared_workram[offset ^ 0x1000];
}

WRITE_HANDLER( xxmissio_bank_sel_w )
{
	UINT8 *BANK = memory_region(REGION_USER1);
	UINT32 bank_address = (data & 0x07) * 0x4000;
	cpu_setbank(1, &BANK[bank_address]);
}

READ_HANDLER( xxmissio_status_r )
{
	xxmissio_status = (xxmissio_status | 2) & ( readinputport(4) | 0xfd );
	return xxmissio_status;
}

WRITE_HANDLER ( xxmissio_status_m_w )
{
	switch (data)
	{
		case 0x00:
			xxmissio_status |= 0x20;
			break;

		case 0x40:
			xxmissio_status &= ~0x08;
			cpu_cause_interrupt(1,0x10);
			break;

		case 0x80:
			xxmissio_status |= 0x04;
			break;
	}
}

WRITE_HANDLER ( xxmissio_status_s_w )
{
	switch (data)
	{
		case 0x00:
			xxmissio_status |= 0x10;
			break;

		case 0x40:
			xxmissio_status |= 0x08;
			break;

		case 0x80:
			xxmissio_status &= ~0x04;
			cpu_cause_interrupt(0,0x10);
			break;
	}
}

int xxmissio_interrupt_m(void)
{
	xxmissio_status &= ~0x20;
	return interrupt();
}

int xxmissio_interrupt_s(void)
{
	xxmissio_status &= ~0x10;
	return interrupt();
}

/****************************************************************************/

static MEMORY_READ_START( readmem1 )
	{ 0x0000, 0x7fff, MRA_ROM },

	{ 0x8000, 0x8000, YM2203_status_port_0_r },
	{ 0x8001, 0x8001, YM2203_read_port_0_r },
	{ 0x8002, 0x8002, YM2203_status_port_1_r },
	{ 0x8003, 0x8003, YM2203_read_port_1_r },

	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa001, 0xa001, input_port_1_r },

	{ 0xa002, 0xa002, xxmissio_status_r },

	{ 0xc000, 0xc7ff, xxmissio_fgram_r },
	{ 0xc800, 0xcfff, xxmissio_videoram_r },
	{ 0xd000, 0xd7ff, spriteram_r },

	{ 0xd800, 0xdaff, paletteram_r },

	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem1 )
	{ 0x0000, 0x7fff, MWA_ROM },

	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0x8002, 0x8002, YM2203_control_port_1_w },
	{ 0x8003, 0x8003, YM2203_write_port_1_w },

	{ 0xa002, 0xa002, xxmissio_status_m_w },

	{ 0xa003, 0xa003, xxmissio_flipscreen_w },

	{ 0xc000, 0xc7ff, xxmissio_fgram_w, &xxmissio_fgram, &xxmissio_fgram_size },
	{ 0xc800, 0xcfff, xxmissio_videoram_w, &videoram, &videoram_size },
	{ 0xd000, 0xd7ff, MWA_RAM, &spriteram, &spriteram_size },

	{ 0xd800, 0xdaff, xxmissio_paletteram_w, &paletteram },

	{ 0xe000, 0xffff, MWA_RAM, &shared_workram },
MEMORY_END

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },

	{ 0x8000, 0x8000, YM2203_status_port_0_r },
	{ 0x8001, 0x8001, YM2203_read_port_0_r },
	{ 0x8002, 0x8002, YM2203_status_port_1_r },
	{ 0x8003, 0x8003, YM2203_read_port_1_r },

	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa001, 0xa001, input_port_1_r },

	{ 0xa002, 0xa002, xxmissio_status_r },

	{ 0xc000, 0xc7ff, xxmissio_fgram_r },
	{ 0xc800, 0xcfff, xxmissio_videoram_r },
	{ 0xd000, 0xd7ff, spriteram_r },

	{ 0xd800, 0xdaff, paletteram_r },

	{ 0xe000, 0xffff, shared_workram_r },

MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_BANK1 },

	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0x8002, 0x8002, YM2203_control_port_1_w },
	{ 0x8003, 0x8003, YM2203_write_port_1_w },

	{ 0x8006, 0x8006, xxmissio_bank_sel_w },

	{ 0xa002, 0xa002, xxmissio_status_s_w },

	{ 0xa003, 0xa003, xxmissio_flipscreen_w },

	{ 0xc000, 0xc7ff, xxmissio_fgram_w },
	{ 0xc800, 0xcfff, xxmissio_videoram_w },
	{ 0xd000, 0xd7ff, spriteram_w },

	{ 0xd800, 0xdaff, xxmissio_paletteram_w },

	{ 0xe000, 0xffff, shared_workram_w },
MEMORY_END

/****************************************************************************/

INPUT_PORTS_START( xxmissio )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-5" ) /* difficulty?? */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Endless Game", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "First Bonus" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x18, 0x08, "Bonus Every" )
	PORT_DIPSETTING(    0x18, "50000" )
	PORT_DIPSETTING(    0x08, "70000" )
	PORT_DIPSETTING(    0x10, "90000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END

/****************************************************************************/

static struct GfxLayout charlayout =
{
	16,8,   /* 16*8 characters */
	2048,   /* 2048 characters */
	4,      /* 4 bits per pixel */
	{0,1,2,3},
	{0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60},
	{64*0, 64*1, 64*2, 64*3, 64*4, 64*5, 64*6, 64*7},
	64*8
};

static struct GfxLayout spritelayout =
{
	32,16,    /* 32*16 characters */
	512,	  /* 512 sprites */
	4,        /* 4 bits per pixel */
	{0,1,2,3},
	{0,4,8,12,16,20,24,28,
	 32,36,40,44,48,52,56,60,
	 8*64+0,8*64+4,8*64+8,8*64+12,8*64+16,8*64+20,8*64+24,8*64+28,
	 8*64+32,8*64+36,8*64+40,8*64+44,8*64+48,8*64+52,8*64+56,8*64+60},
	{64*0, 64*1, 64*2, 64*3, 64*4, 64*5, 64*6, 64*7,
	 64*16, 64*17, 64*18, 64*19, 64*20, 64*21, 64*22, 64*23},
	64*8*4
};

static struct GfxLayout bglayout =
{
	16,8,   /* 16*8 characters */
	1024,   /* 1024 characters */
	4,      /* 4 bits per pixel */
	{0,1,2,3},
	{0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60},
	{64*0, 64*1, 64*2, 64*3, 64*4, 64*5, 64*6, 64*7},
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,   256,  8 }, /* FG */
	{ REGION_GFX1, 0x0000, &spritelayout,   0,  8 }, /* sprite */
	{ REGION_GFX2, 0x0000, &bglayout,     512, 16 }, /* BG */
	{ -1 } /* end of array */
};

/****************************************************************************/

static struct YM2203interface ym2203_interface =
{
	2,          /* 2 chips */
	12000000/8,    /* 1.5 MHz */
	{ YM2203_VOL(40,15), YM2203_VOL(40,15) },
	{ input_port_2_r,0 },
	{ input_port_3_r,0 },
	{ 0,xxmissio_scroll_x_w },
	{ 0,xxmissio_scroll_y_w }
};

static struct MachineDriver machine_driver_xxmissio =
{
	{
		{
			CPU_Z80,
			12000000/4,	/* 3.0MHz */
			readmem1,writemem1,0,0,
			xxmissio_interrupt_m,1
		},
		{
			CPU_Z80,
			12000000/4,	/* 3.0MHz */
			readmem2,writemem2,0,0,
			xxmissio_interrupt_s,2
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,
	0,

	64*8, 32*8, { 0*8, 64*8-1, 4*8, 28*8-1 },

	gfxdecodeinfo,
	768,768,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	generic_vh_start,
	generic_vh_stop,
	xxmissio_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

/****************************************************************************/

ROM_START( xxmissio )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* CPU1 */
	ROM_LOAD( "xx1.4l", 0x0000,  0x8000, 0x86e07709 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* CPU2 */
	ROM_LOAD( "xx2.4b", 0x0000,  0x4000, 0x13fa7049 )

	ROM_REGION( 0x18000, REGION_USER1, 0 ) /* BANK */
	ROM_LOAD( "xx3.6a", 0x00000,  0x8000, 0x16fdacab )
	ROM_LOAD( "xx4.6b", 0x08000,  0x8000, 0x274bd4d2)
	ROM_LOAD( "xx5.6d", 0x10000,  0x8000, 0xc5f35535 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* FG/sprites */
	ROM_LOAD16_BYTE( "xx6.8j", 0x00001, 0x8000, 0xdc954d01 )
	ROM_LOAD16_BYTE( "xx8.8f", 0x00000, 0x8000, 0xa9587cc6 )
	ROM_LOAD16_BYTE( "xx7.8h", 0x10001, 0x8000, 0xabe9cd68 )
	ROM_LOAD16_BYTE( "xx9.8e", 0x10000, 0x8000, 0x854e0e5f )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE ) /* BG */
	ROM_LOAD16_BYTE( "xx10.4c", 0x0000,  0x8000, 0xd27d7834 )
	ROM_LOAD16_BYTE( "xx11.4b", 0x0001,  0x8000, 0xd9dd827c )
ROM_END

GAME( 1986, xxmissio, 0, xxmissio, xxmissio, 0, ROT90, "UPL", "XX Mission" )
