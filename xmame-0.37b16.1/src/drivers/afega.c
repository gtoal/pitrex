/***************************************************************************

							  -= Afega Games =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU	:	M68000
Video Chips	:	AFEGA AFI-GFSK  (68 Pin PLCC)
				AFEGA AFI-GFLK (208 Pin PQFP)

Sound CPU	:	Z80
Sound Chips	:	M6295 (AD-65)  +  YM2151 (BS901)  +  YM3014 (BS90?)

---------------------------------------------------------------------------
Year + Game						Notes
---------------------------------------------------------------------------
98 Sen Jin - Guardian Storm		Some text missing (protection, see service mode)
98 Stagger I
---------------------------------------------------------------------------

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern data16_t *afega_vram_0, *afega_scroll_0;
extern data16_t *afega_vram_1, *afega_scroll_1;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( afega_vram_0_w );
WRITE16_HANDLER( afega_vram_1_w );
WRITE16_HANDLER( afega_palette_w );

void grdnstrm_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

int  afega_vh_start(void);
void afega_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

WRITE16_HANDLER( afega_soundlatch_w )
{
	if (ACCESSING_LSB && Machine->sample_rate)
		soundlatch_w(0,data&0xff);
}

/*
 Lines starting with an empty comment in the following MemoryReadAddress
 arrays are there for debug (e.g. the game does not read from those ranges
 AFAIK)
*/

static MEMORY_READ16_START( afega_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM */
	{ 0x080000, 0x080001, input_port_0_word_r		},	/* Buttons */
	{ 0x080002, 0x080003, input_port_1_word_r		},	/* P1 + P2 */
	{ 0x080004, 0x080005, input_port_2_word_r		},	/* 2 x DSW */
/**/{ 0x088000, 0x0885ff, MRA16_RAM					},	/* Palette */
/**/{ 0x08c000, 0x08c003, MRA16_RAM					},	/* Scroll */
/**/{ 0x08c004, 0x08c007, MRA16_RAM					},	/* */
/**/{ 0x090000, 0x091fff, MRA16_RAM					},	/* Layer 0 */
/**/{ 0x092000, 0x093fff, MRA16_RAM					},	/* ? */
/**/{ 0x09c000, 0x09c7ff, MRA16_RAM					},	/* Layer 1 */
	{ 0x3c0000, 0x3c7fff, MRA16_RAM					},	/* RAM */
	{ 0x3c8000, 0x3c8fff, MRA16_RAM					},	/* Sprites */
	{ 0x3c9000, 0x3cffff, MRA16_RAM					},	/* RAM */
MEMORY_END

static MEMORY_WRITE16_START( afega_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM */
	{ 0x080000, 0x08001d, MWA16_RAM						},	/* */
	{ 0x08001e, 0x08001f, afega_soundlatch_w			},	/* To Sound CPU */
	{ 0x080020, 0x087fff, MWA16_RAM						},	/* */
	{ 0x088000, 0x0885ff, afega_palette_w, &paletteram16},	/* Palette */
	{ 0x088600, 0x08bfff, MWA16_RAM						},	/* */
	{ 0x08c000, 0x08c003, MWA16_RAM, &afega_scroll_0	},	/* Scroll */
	{ 0x08c004, 0x08c007, MWA16_RAM, &afega_scroll_1	},	/* */
	{ 0x08c008, 0x08ffff, MWA16_RAM						},	/* */
	{ 0x090000, 0x091fff, afega_vram_0_w, &afega_vram_0	},	/* Layer 0 */
	{ 0x092000, 0x093fff, MWA16_RAM						},	/* ? */
	{ 0x09c000, 0x09c7ff, afega_vram_1_w, &afega_vram_1	},	/* Layer 1 */
	{ 0x3c0000, 0x3c7fff, MWA16_RAM						},	/* RAM */
	{ 0x3c8000, 0x3c8fff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites */
	{ 0x3c9000, 0x3cffff, MWA16_RAM						},	/* RAM */
MEMORY_END


/***************************************************************************


							Memory Maps - Sound CPU


***************************************************************************/

static MEMORY_READ_START( afega_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM					},	/* ROM */
	{ 0xf000, 0xf7ff, MRA_RAM					},	/* RAM */
	{ 0xf800, 0xf800, soundlatch_r				},	/* From Main CPU */
	{ 0xf809, 0xf809, YM2151_status_port_0_r	},	/* YM2151 */
	{ 0xf80a, 0xf80a, OKIM6295_status_0_r		},	/* M6295 */
MEMORY_END

static MEMORY_WRITE_START( afega_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM					},	/* ROM */
	{ 0xf000, 0xf7ff, MWA_RAM					},	/* RAM */
	{ 0xf808, 0xf808, YM2151_register_port_0_w	},	/* YM2151 */
	{ 0xf809, 0xf809, YM2151_data_port_0_w		},	/* */
	{ 0xf80a, 0xf80a, OKIM6295_data_0_w			},	/* M6295 */
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
							Sen Jin - Guardian Storm
***************************************************************************/

INPUT_PORTS_START( grdnstrm )
	PORT_START	/* IN0 - $800000.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $800002.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $800004.w */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Bombs" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPSETTING(      0x0200, "Horizontally" )
	PORT_DIPSETTING(      0x0100, "Vertically" )
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x1800, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END


/***************************************************************************
								Stagger I
***************************************************************************/

INPUT_PORTS_START( stagger1 )
	PORT_START	/* IN0 - $800000.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $800002.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $800004.w */
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPSETTING(      0x0200, "Horizontally" )
	PORT_DIPSETTING(      0x0100, "Vertically" )
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x1800, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END


/***************************************************************************


							Graphics Layouts


***************************************************************************/

static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4)	},
	{ STEP8(0,8*4)	},
	8*8*4
};

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};

static struct GfxLayout layout_16x16x4_2 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(0,2),1), STEP4(RGN_FRAC(1,2),1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};


static struct GfxDecodeInfo grdnstrm_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 256*1, 16 }, /* [0] Sprites */
	{ REGION_GFX2, 0, &layout_16x16x8, 256*3, 16 }, /* [1] Layer 0 */
	{ REGION_GFX3, 0, &layout_8x8x4,   256*2, 16 }, /* [2] Layer 1 */
	{ -1 }
};

static struct GfxDecodeInfo stagger1_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4_2, 256*1, 16 }, /* [0] Sprites */
	{ REGION_GFX2, 0, &layout_16x16x4,   256*0, 16 }, /* [1] Layer 0 */
	{ REGION_GFX3, 0, &layout_8x8x4,     256*2, 16 }, /* [2] Layer 1 */
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface afega_ym2151_intf =
{
	1,
	4000000,	/* ? */
	{ YM3012_VOL(30,MIXER_PAN_LEFT,30,MIXER_PAN_RIGHT) },
	{ irq_handler }
};

static struct OKIM6295interface afega_m6295_intf =
{
	1,
	{ 8000 },	/* ? */
	{ REGION_SOUND1 },
	{ 70 }
};

int interrupt_afega(void)
{
	switch ( cpu_getiloops() )
	{
		case 0:		return 2;
		case 1:		return 4;
		default:	return ignore_interrupt();
	}
}

static struct MachineDriver machine_driver_grdnstrm =
{
	{
		{
			CPU_M68000,
			10000000,
			afega_readmem, afega_writemem,0,0,
			interrupt_afega, 2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* ? */
			afega_sound_readmem, afega_sound_writemem,0,0,
			ignore_interrupt, 1	/* IRQ by YM2151, no NMI */
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1 },
	grdnstrm_gfxdecodeinfo,
	256 * 3, 256 * 3 + 16*256,
	grdnstrm_vh_init_palette,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	afega_vh_start,
	0,
	afega_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM2151,	&afega_ym2151_intf	},
		{	SOUND_OKIM6295,	&afega_m6295_intf	}
	},
};

static struct MachineDriver machine_driver_stagger1 =
{
	{
		{
			CPU_M68000,
			10000000,
			afega_readmem, afega_writemem,0,0,
			interrupt_afega, 2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* ? */
			afega_sound_readmem, afega_sound_writemem,0,0,
			ignore_interrupt, 1	/* IRQ by YM2151, no NMI */
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1 },
	stagger1_gfxdecodeinfo,
	256 * 3, 256 * 3,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	afega_vh_start,
	0,
	afega_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM2151,	&afega_ym2151_intf	},
		{	SOUND_OKIM6295,	&afega_m6295_intf	}
	},
};


/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

									Stagger I
(AFEGA 1998)

Parts:

1 MC68HC000P10
1 Z80
2 Lattice ispLSI 1032E

***************************************************************************/

ROM_START( stagger1 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2.bin", 0x000000, 0x020000, 0x8555929b )
	ROM_LOAD16_BYTE( "3.bin", 0x000001, 0x020000, 0x5b0b63ac )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1.bin", 0x00000, 0x10000, 0x5d8cf28e )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "7.bin", 0x00000, 0x80000, 0x048f7683 )
	ROM_LOAD16_BYTE( "6.bin", 0x00001, 0x80000, 0x051d4a77 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x4 */
	ROM_LOAD( "4.bin", 0x00000, 0x80000, 0x46463d36 )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	/* Unused */

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "5.bin", 0x00000, 0x40000, 0x525eec4a )	/* FIRST AND SECOND HALF IDENTICAL */
	ROM_CONTINUE(      0x00000, 0x40000             )
ROM_END

void init_stagger1(void)
{
	data16_t *RAM = (data16_t*)memory_region( REGION_CPU1 );

	/* Is this a 68k emulation bug ? */
	/* Patch movem.w A5, -(A7)	to	movem.l A5, -(A7) */
	RAM[0x1b6a0/2] = 0x48e7;
	RAM[0x1b6c0/2] = 0x48e7;
	RAM[0x1d15e/2] = 0x48e7;
	RAM[0x1d17e/2] = 0x48e7;

	/* Patch movem.w (A7)+, A5	to	movem.l (A7)+, A5 */
	RAM[0x1b6b8/2] = 0x4cdf;
	RAM[0x1b6d8/2] = 0x4cdf;
	RAM[0x1d176/2] = 0x4cdf;
	RAM[0x1d196/2] = 0x4cdf;
}


/***************************************************************************

							Sen Jin - Guardian Storm

(C) Afega 1998

CPU: 68HC000FN10 (68000, 68 pin PLCC)
SND: Z84C000FEC (Z80, 44 pin PQFP), AD-65 (44 pin PQFP, Probably OKI M6295),
     BS901 (Probably YM2151 or YM3812, 24 pin DIP), BS901 (possibly YM3014 or similar? 16 pin DIP)
OSC: 12.000MHz (near 68000), 4.000MHz (Near Z84000)
RAM: LH52B256 x 8, 6116 x 7
DIPS: 2 x 8 position

Other Chips: AFEGA AFI-GFSK (68 pin PLCC, located next to 68000)
             AFEGA AFI-GFLK (208 pin PQFP)

ROMS:
GST-01.U92   27C512, \
GST-02.U95   27C2000  > Sound Related, all located near Z80
GST-03.U4    27C512  /

GST-04.112   27C2000 \
GST-05.107   27C2000  /Main Program

GST-06.C13   read as 27C160  label = AF1-SP (Sprites?)
GST-07.C08   read as 27C160  label = AF1=B2 (Backgrounds?)
GST-08.C03   read as 27C160  label = AF1=B1 (Backgrounds?)

***************************************************************************/

ROM_START( grdnstrm )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "gst-04.112", 0x000000, 0x040000, 0x922c931a )
	ROM_LOAD16_BYTE( "gst-05.107", 0x000001, 0x040000, 0xd22ca2dc )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "gst-01.u92", 0x00000, 0x10000, 0x5d8cf28e )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD( "gst-06.c13", 0x000000, 0x200000, 0x7d4d4985 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "gst-07.c08", 0x000000, 0x200000, 0xd68588c2 )
	ROM_LOAD( "gst-08.c03", 0x200000, 0x200000, 0xf8b200a8 )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "gst-03.u4",  0x00000, 0x10000, 0xa1347297 )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "gst-02.u95", 0x00000, 0x40000, 0xe911ce33 )
ROM_END

/* Address lines scrambling + Protection */
void init_grdnstrm(void)
{
	data8_t *RAM = memory_region       ( REGION_CPU1 );
	size_t  size = memory_region_length( REGION_CPU1 );
	int i;

#define SWAP( b1, b2 ) \
{	data8_t t; \
	int i1 = ((i&(1<<b1))?(1<<b2):0)|((i&(1<<b2))?(1<<b1):0)|(i&~((1<<b1)|(1<<b2))); \
	if (i1>i)	continue; \
	t = RAM[i];	RAM[i] = RAM[i1];	RAM[i1] = t;	}

	for (i = 0; i < size; i++)	SWAP(0x10,0x11)
	for (i = 0; i < size; i++)	SWAP(0x0e,0x0f)

	/* Patch Protection (that supplies some 68k code seen in the
	   2760-29cf range */
	((data16_t*)RAM)[0x0027a/2] = 0x4e71;
	((data16_t*)RAM)[0x02760/2] = 0x4e75;

	/* Is this a 68k emulation bug ? */
	/* Patch movem.w A5, -(A7)	to	movem.l A5, -(A7) */
	((data16_t*)RAM)[0x2402a/2] = 0x48e7;
	((data16_t*)RAM)[0x2404a/2] = 0x48e7;
	((data16_t*)RAM)[0x24892/2] = 0x48e7;
	((data16_t*)RAM)[0x248b2/2] = 0x48e7;

	/* Patch movem.w (A7)+, A5	to	movem.l (A7)+, A5 */
	((data16_t*)RAM)[0x24042/2] = 0x4cdf;
	((data16_t*)RAM)[0x24062/2] = 0x4cdf;
	((data16_t*)RAM)[0x248aa/2] = 0x4cdf;
	((data16_t*)RAM)[0x248ca/2] = 0x4cdf;
}


/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 1998, stagger1, 0, stagger1, stagger1, stagger1, ROT270_16BIT, "Afega", "Stagger I (Japan)",                GAME_NOT_WORKING )
GAMEX( 1998, grdnstrm, 0, grdnstrm, grdnstrm, grdnstrm, ROT270_16BIT, "Afega", "Sen Jin - Guardian Storm (Korea)", GAME_NOT_WORKING )
