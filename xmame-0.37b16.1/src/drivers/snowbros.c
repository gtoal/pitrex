/***************************************************************************

 Snow Brothers

 driver by Mike Coates

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"

void snowbros_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void wintbob_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static int snowbros_interrupt(void)
{
	return cpu_getiloops() + 2;	/* IRQs 4, 3, and 2 */
}



/* Sound Routines */

READ16_HANDLER( snowbros_68000_sound_r )
{
	int ret;

	/* If the sound CPU is running, read the YM3812 status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0)
	{
		ret = soundlatch_r(offset);
	}
	else
	{
		ret = 3;
	}

	return ret;
}


static WRITE16_HANDLER( snowbros_68000_sound_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(offset,data & 0xff);
		cpu_cause_interrupt(1,Z80_NMI_INT);
	}
}



static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x300000, 0x300001, snowbros_68000_sound_r },
	{ 0x500000, 0x500001, input_port_0_word_r },
	{ 0x500002, 0x500003, input_port_1_word_r },
	{ 0x500004, 0x500005, input_port_2_word_r },
	{ 0x600000, 0x6001ff, MRA16_RAM },
	{ 0x700000, 0x701dff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x200000, 0x200001, watchdog_reset16_w },
	{ 0x300000, 0x300001, snowbros_68000_sound_w },
/*	{ 0x400000, 0x400001, snowbros_interrupt_enable_w }, */
	{ 0x600000, 0x6001ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x700000, 0x701dff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x800001, MWA16_NOP },	/* IRQ 4 acknowledge? */
	{ 0x900000, 0x900001, MWA16_NOP },	/* IRQ 3 acknowledge? */
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* IRQ 2 acknowledge? */
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x02, 0x02, YM3812_status_port_0_r },
	{ 0x04, 0x04, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x02, 0x02, YM3812_control_port_0_w },
	{ 0x03, 0x03, YM3812_write_port_0_w },
	{ 0x04, 0x04, soundlatch_w },	/* goes back to the main CPU, checked during boot */
PORT_END



INPUT_PORTS_START( snowbros )
	PORT_START	/* 500001 */
	PORT_DIPNAME( 0x01, 0x01, "Country (Affects Coinage)" )
	PORT_DIPSETTING(    0x01, "America" )
	PORT_DIPSETTING(    0x00, "Europe" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
/* Better to implement a coin mode 1-2 stuff later */
	PORT_DIPNAME( 0x30, 0x30, "Coin A America/Europe" )
	PORT_DIPSETTING(    0x10, "2C/1C 3C/1C" )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "2C/3C 4C/1C" )
	PORT_DIPSETTING(    0x20, "1C/2C 2C/1C" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B America/Europe" )
	PORT_DIPSETTING(    0x40, "2C/1C 1C/4C" )
	PORT_DIPSETTING(    0xc0, "1C/1C 1C/2C" )
	PORT_DIPSETTING(    0x00, "2C/3C 1C/6C" )
	PORT_DIPSETTING(    0x80, "1C/2C 1C/3C" )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Must be low or game stops! */
                                               /* probably VBlank */

	PORT_START	/* 500003 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "100k and every 200k " )
	PORT_DIPSETTING(    0x0c, "100k Only" )
	PORT_DIPSETTING(    0x08, "200k Only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500005 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ STEP8(0,4), STEP8(8*32,4) },
	{ STEP8(0,32), STEP8(16*32,32) },
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,  0, 16 },
	{ -1 } /* end of array */
};

static struct GfxLayout tilelayout_wb =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ STEP4(3*4,-4), STEP4(7*4,-4), STEP4(11*4,-4), STEP4(15*4,-4) },
	{ STEP16(0,64) },
	16*64
};

static struct GfxDecodeInfo gfxdecodeinfo_wb[] =
{
	{ REGION_GFX1, 0, &tilelayout_wb,  0, 16 },
	{ -1 }
};

/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz ? (hand tuned) */
	{ 100 },	/* volume */
	{ irqhandler },
};



static struct MachineDriver machine_driver_snowbros =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz ????? */
			readmem,writemem,0,0,
			snowbros_interrupt,3
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3600000,	/* 3.6 MHz ??? */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the YM3812 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	snowbros_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

static struct MachineDriver machine_driver_wintbob =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10MHz .. Needed to compensate for less capable gfx hardware? otherwise game runs too slow */
			readmem,writemem,0,0,
			snowbros_interrupt,3
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3600000,	/* 3.6 MHz ??? */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the YM3812 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo_wb,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	wintbob_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( snowbros )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "sn6.bin",  0x00000, 0x20000, 0x4899ddcf )
	ROM_LOAD16_BYTE( "sn5.bin",  0x00001, 0x20000, 0xad310d3f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, 0xe6eab4e4 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch0",          0x00000, 0x20000, 0x36d84dfe )
	ROM_LOAD( "ch1",          0x20000, 0x20000, 0x76347256 )
	ROM_LOAD( "ch2",          0x40000, 0x20000, 0xfdaa634c )
	ROM_LOAD( "ch3",          0x60000, 0x20000, 0x34024aef )
ROM_END

ROM_START( snowbroa )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "snowbros.3a",  0x00000, 0x20000, 0x10cb37e1 )
	ROM_LOAD16_BYTE( "snowbros.2a",  0x00001, 0x20000, 0xab91cc1e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, 0xe6eab4e4 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch0",          0x00000, 0x20000, 0x36d84dfe )
	ROM_LOAD( "ch1",          0x20000, 0x20000, 0x76347256 )
	ROM_LOAD( "ch2",          0x40000, 0x20000, 0xfdaa634c )
	ROM_LOAD( "ch3",          0x60000, 0x20000, 0x34024aef )
ROM_END

ROM_START( snowbrob )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "sbros3-a",     0x00000, 0x20000, 0x301627d6 )
	ROM_LOAD16_BYTE( "sbros2-a",     0x00001, 0x20000, 0xf6689f41 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, 0xe6eab4e4 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch0",          0x00000, 0x20000, 0x36d84dfe )
	ROM_LOAD( "ch1",          0x20000, 0x20000, 0x76347256 )
	ROM_LOAD( "ch2",          0x40000, 0x20000, 0xfdaa634c )
	ROM_LOAD( "ch3",          0x60000, 0x20000, 0x34024aef )
ROM_END

ROM_START( snowbroj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "snowbros.3",   0x00000, 0x20000, 0x3f504f9e )
	ROM_LOAD16_BYTE( "snowbros.2",   0x00001, 0x20000, 0x854b02bc )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, 0xe6eab4e4 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	/* The gfx ROM (snowbros.1) was bad, I'm using the ones from the other sets. */
	ROM_LOAD( "ch0",          0x00000, 0x20000, 0x36d84dfe )
	ROM_LOAD( "ch1",          0x20000, 0x20000, 0x76347256 )
	ROM_LOAD( "ch2",          0x40000, 0x20000, 0xfdaa634c )
	ROM_LOAD( "ch3",          0x60000, 0x20000, 0x34024aef )
ROM_END

ROM_START( wintbob )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "wb03.bin", 0x00000, 0x10000, 0xdf56e168 )
	ROM_LOAD16_BYTE( "wb01.bin", 0x00001, 0x10000, 0x05722f17 )
	ROM_LOAD16_BYTE( "wb04.bin", 0x20000, 0x10000, 0x53be758d )
	ROM_LOAD16_BYTE( "wb02.bin", 0x20001, 0x10000, 0xfc8e292e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "wb05.bin",     0x0000, 0x10000, 0x53fe59df )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	/* probably identical data to Snow Bros, in a different format */
	ROM_LOAD16_BYTE( "wb13.bin",     0x00000, 0x10000, 0x426921de )
	ROM_LOAD16_BYTE( "wb06.bin",     0x00001, 0x10000, 0x68204937 )
	ROM_LOAD16_BYTE( "wb12.bin",     0x20000, 0x10000, 0xef4e04c7 )
	ROM_LOAD16_BYTE( "wb07.bin",     0x20001, 0x10000, 0x53f40978 )
	ROM_LOAD16_BYTE( "wb11.bin",     0x40000, 0x10000, 0x41cb4563 )
	ROM_LOAD16_BYTE( "wb08.bin",     0x40001, 0x10000, 0x9497b88c )
	ROM_LOAD16_BYTE( "wb10.bin",     0x60000, 0x10000, 0x5fa22b1e )
	ROM_LOAD16_BYTE( "wb09.bin",     0x60001, 0x10000, 0x9be718ca )
ROM_END


GAME( 1990, snowbros, 0,        snowbros, snowbros, 0, ROT0, "Toaplan (Romstar license)", "Snow Bros. - Nick & Tom (set 1)" )
GAME( 1990, snowbroa, snowbros, snowbros, snowbros, 0, ROT0, "Toaplan (Romstar license)", "Snow Bros. - Nick & Tom (set 2)" )
GAME( 1990, snowbrob, snowbros, snowbros, snowbros, 0, ROT0, "Toaplan (Romstar license)", "Snow Bros. - Nick & Tom (set 3)" )
GAME( 1990, snowbroj, snowbros, snowbros, snowbros, 0, ROT0, "Toaplan (Romstar license)", "Snow Bros. - Nick & Tom (Japan)" )
GAME( 1990, wintbob,  snowbros, wintbob,  snowbros, 0, ROT0, "bootleg", "The Winter Bobble" )
