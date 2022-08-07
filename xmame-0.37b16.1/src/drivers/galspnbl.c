/***************************************************************************

Hot Pinball
Gals Pinball

driver by Nicola Salmoria

Notes:
- to start a 2 or more players game, press the start button multiple times
- the sprite graphics contain a "(c) Tecmo", and the sprite system is
  indeed similar to other Tecmo games like Ninja Gaiden.


TODO:
- scrolling is wrong.
- sprite/tile priority might be wrong. There is an unknown bit in the fg
  tilemap, marking some tiles that I'm not currently drawing.
- coin insertion is not recognized consistenly.
- almost all dip switches unknown

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


extern data16_t *galspnbl_bgvideoram,*galspnbl_videoram,*galspnbl_colorram;

void galspnbl_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
WRITE16_HANDLER( galspnbl_bgvideoram_w );
WRITE16_HANDLER( galspnbl_scroll_w );
void galspnbl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static WRITE16_HANDLER( soundcommand_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(offset,data & 0xff);
		cpu_cause_interrupt(1,Z80_NMI_INT);
	}
}


static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },
	{ 0x700000, 0x703fff, MRA16_RAM },	/* galspnbl */
	{ 0x708000, 0x70ffff, MRA16_RAM },	/* galspnbl */
	{ 0x800000, 0x803fff, MRA16_RAM },	/* hotpinbl */
	{ 0x808000, 0x80ffff, MRA16_RAM },	/* hotpinbl */
	{ 0x880000, 0x880fff, MRA16_RAM },
	{ 0x900000, 0x900fff, MRA16_RAM },
	{ 0x904000, 0x904fff, MRA16_RAM },
	{ 0x980000, 0x9bffff, MRA16_RAM },
	{ 0xa80000, 0xa80001, input_port_0_word_r },
	{ 0xa80010, 0xa80011, input_port_1_word_r },
	{ 0xa80020, 0xa80021, input_port_2_word_r },
	{ 0xa80030, 0xa80031, input_port_3_word_r },
	{ 0xa80040, 0xa80041, input_port_4_word_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },
	{ 0x700000, 0x703fff, MWA16_RAM },	/* galspnbl work RAM */
	{ 0x708000, 0x70ffff, MWA16_RAM },	/* galspnbl work RAM, bitmaps are decompressed here */
	{ 0x800000, 0x803fff, MWA16_RAM },	/* hotpinbl work RAM */
	{ 0x808000, 0x80ffff, MWA16_RAM },	/* hotpinbl work RAM, bitmaps are decompressed here */
	{ 0x880000, 0x880fff, MWA16_RAM, &spriteram16, &spriteram_size },
{ 0x8ff400, 0x8fffff, MWA16_NOP },	/* ??? */
	{ 0x900000, 0x900fff, MWA16_RAM, &galspnbl_colorram },
{ 0x901000, 0x903fff, MWA16_NOP },	/* ??? */
	{ 0x904000, 0x904fff, MWA16_RAM, &galspnbl_videoram },
{ 0x905000, 0x907fff, MWA16_NOP },	/* ??? */
	{ 0x980000, 0x9bffff, galspnbl_bgvideoram_w, &galspnbl_bgvideoram },
{ 0xa00000, 0xa00fff, MWA16_NOP },	/* more palette ? */
	{ 0xa01000, 0xa017ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
{ 0xa01800, 0xa027ff, MWA16_NOP },	/* more palette ? */
	{ 0xa80010, 0xa80011, soundcommand_w },
	{ 0xa80020, 0xa80021, MWA16_NOP },	/* could be watchdog, but causes resets when picture is shown */
	{ 0xa80030, 0xa80031, MWA16_NOP },	/* irq ack? */
	{ 0xa80050, 0xa80051, galspnbl_scroll_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, OKIM6295_status_0_r },
	{ 0xfc00, 0xfc00, MRA_NOP },	/* irq ack ?? */
	{ 0xfc20, 0xfc20, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, OKIM6295_data_0_w },
	{ 0xf810, 0xf810, YM3812_control_port_0_w },
	{ 0xf811, 0xf811, YM3812_write_port_0_w },
	{ 0xfc00, 0xfc00, MWA_NOP },	/* irq ack ?? */
MEMORY_END




INPUT_PORTS_START( hotpinbl )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Slide Show" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( galspnbl )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Slide Show" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	16,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+1*4, 2*4, 3*4, RGN_FRAC(1,2)+2*4, RGN_FRAC(1,2)+3*4,
			16*8+0*4, 16*8+1*4, 16*8+RGN_FRAC(1,2)+0*4, 16*8+RGN_FRAC(1,2)+1*4, 16*8+2*4, 16*8+3*4, 16*8+RGN_FRAC(1,2)+2*4, 16*8+RGN_FRAC(1,2)+3*4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	32*8
};

static struct GfxLayout spritelayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 8+0, 8+4, 8+RGN_FRAC(1,2)+0, 8+RGN_FRAC(1,2)+4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   512, 16 },
	{ REGION_GFX2, 0, &spritelayout,   0, 16 },
	{ -1 } /* end of array */
};



static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz ? */
	{ 100 },	/* volume */
	{ irqhandler },
};

static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
	{ 8000 },			/* 8000Hz frequency? */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};



static struct MachineDriver machine_driver_hotpinbl =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 MHz ??? */
			readmem,writemem,0,0,
			m68_level3_irq,1	/* also has vector for 6, but it does nothing */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ is caused by the YM3812 */
								/* NMI is caused by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	512, 256, { 0, 512-1, 16, 240-1 },
	gfxdecodeinfo,
	1024 + 32768, 1024,
	galspnbl_init_palette,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	generic_bitmapped_vh_start,
	generic_bitmapped_vh_stop,
	galspnbl_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( galspnbl )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "7.rom",        0x000000, 0x80000, 0xce0189bf )
	ROM_LOAD16_BYTE( "3.rom",        0x000001, 0x80000, 0x9b0a8744 )
	ROM_LOAD16_BYTE( "8.rom",        0x100000, 0x80000, 0xeee2f087 )
	ROM_LOAD16_BYTE( "4.rom",        0x100001, 0x80000, 0x56298489 )
	ROM_LOAD16_BYTE( "9.rom",        0x200000, 0x80000, 0xd9e4964c )
	ROM_LOAD16_BYTE( "5.rom",        0x200001, 0x80000, 0xa5e71ee4 )
	ROM_LOAD16_BYTE( "10.rom",       0x300000, 0x80000, 0x3a20e1e5 )
	ROM_LOAD16_BYTE( "6.rom",        0x300001, 0x80000, 0x94927d20 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "2.rom",        0x0000, 0x10000, 0xfae688a7 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "17.rom",       0x00000, 0x40000, 0x7d435701 )
	ROM_LOAD( "18.rom",       0x40000, 0x40000, 0x136adaac )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "15.rom",       0x00000, 0x20000, 0x4beb840d )
	ROM_LOAD( "16.rom",       0x20000, 0x20000, 0x93d3c610 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "1.rom",        0x00000, 0x40000, 0x93c06d3d )
ROM_END

ROM_START( hotpinbl )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "hp_07.bin",    0x000000, 0x80000, 0x978cc13e )
	ROM_LOAD16_BYTE( "hp_03.bin",    0x000001, 0x80000, 0x68388726 )
	ROM_LOAD16_BYTE( "hp_08.bin",    0x100000, 0x80000, 0xbd16be12 )
	ROM_LOAD16_BYTE( "hp_04.bin",    0x100001, 0x80000, 0x655b0cf0 )
	ROM_LOAD16_BYTE( "hp_09.bin",    0x200000, 0x80000, 0xa6368624 )
	ROM_LOAD16_BYTE( "hp_05.bin",    0x200001, 0x80000, 0x48efd028 )
	ROM_LOAD16_BYTE( "hp_10.bin",    0x300000, 0x80000, 0xa5c63e34 )
	ROM_LOAD16_BYTE( "hp_06.bin",    0x300001, 0x80000, 0x513eda91 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "hp_02.bin",    0x0000, 0x10000, 0x82698269 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hp_13.bin",    0x00000, 0x40000, 0xd53b64b9 )
	ROM_LOAD( "hp_14.bin",    0x40000, 0x40000, 0x2fe3fcee )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hp_11.bin",    0x00000, 0x20000, 0xdeecd7f1 )
	ROM_LOAD( "hp_12.bin",    0x20000, 0x20000, 0x5fd603c2 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "hp_01.bin",    0x00000, 0x40000, 0x93c06d3d )
ROM_END



GAMEX( 1995, hotpinbl, 0, hotpinbl, hotpinbl, 0, ROT90_16BIT, "Comad & New Japan System", "Hot Pinball", GAME_NO_COCKTAIL )
GAMEX( 1996, galspnbl, 0, hotpinbl, galspnbl, 0, ROT90_16BIT, "Comad", "Gals Pinball", GAME_NO_COCKTAIL )
