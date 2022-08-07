/***************************************************************************

						  -= Yun Sung 16 Bit Games =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU    :  MC68000
Sound CPU    :  Z80 [Optional]
Video Chips  :	?
Sound Chips  :	OKI M6295 + YM3812 [Optional]


---------------------------------------------------------------------------
Year + Game         Board#
---------------------------------------------------------------------------
97 Shocking         ?
?? Magic Bubble     YS102
---------------------------------------------------------------------------

- In shocking, service mode just shows the menu, with mangled graphics
  (sprites, but the charset they used is in the tiles ROMs!).
  In magicbub they used color 0 for tiles (all blacks, so you can't see
  most of it!). Again, color 0 for sprites would be ok. Some kind
  of sprites-tiles swapping, or unfinished leftovers ?

- Screen flipping: not used!?

- Are priorities correct ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern data16_t *yunsun16_vram_0,   *yunsun16_vram_1;
extern data16_t *yunsun16_scroll_0, *yunsun16_scroll_1;
extern data16_t *yunsun16_priority;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( yunsun16_vram_0_w );
WRITE16_HANDLER( yunsun16_vram_1_w );

int  yunsun16_vh_start(void);
void yunsun16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

static WRITE16_HANDLER( yunsun16_sound_bank_w )
{
	if (ACCESSING_LSB)
	{
		int bank = data & 3;
		unsigned char *dst	= memory_region(REGION_SOUND1);
		unsigned char *src	= dst + 0x80000 + 0x20000 * bank;
		memcpy(dst + 0x20000, src, 0x20000);
	}
}

static MEMORY_READ16_START( yunsun16_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM */
	{ 0xff0000, 0xffffff, MRA16_RAM					},	/* RAM */
	{ 0x800000, 0x800001, input_port_0_word_r		},	/* P1 + P2 */
	{ 0x800018, 0x800019, input_port_1_word_r		},	/* Coins */
	{ 0x80001a, 0x80001b, input_port_2_word_r		},	/* DSW1 */
	{ 0x80001c, 0x80001d, input_port_3_word_r		},	/* DSW2 */
	{ 0x800188, 0x800189, OKIM6295_status_0_lsb_r	},	/* Sound */
	{ 0x900000, 0x903fff, MRA16_RAM					},	/* Palette */
	{ 0x908000, 0x90bfff, MRA16_RAM					},	/* Layer 1 */
	{ 0x90c000, 0x90ffff, MRA16_RAM					},	/* Layer 0 */
	{ 0x910000, 0x910fff, MRA16_RAM					},	/* Sprites */
MEMORY_END

static MEMORY_WRITE16_START( yunsun16_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM					},	/* ROM */
	{ 0xff0000, 0xffffff, MWA16_RAM					},	/* RAM */
	{ 0x800030, 0x800031, MWA16_NOP					},	/* ? (value: don't care) */
	{ 0x800100, 0x800101, MWA16_NOP					},	/* ? $9100 */
	{ 0x800102, 0x800103, MWA16_NOP					},	/* ? $9080 */
	{ 0x800104, 0x800105, MWA16_NOP					},	/* ? $90c0 */
	{ 0x80010a, 0x80010b, MWA16_NOP					},	/* ? $9000 */
	{ 0x80010c, 0x80010f, MWA16_RAM, &yunsun16_scroll_1	},	/* Scrolling */
	{ 0x800114, 0x800117, MWA16_RAM, &yunsun16_scroll_0	},	/* */
	{ 0x800154, 0x800155, MWA16_RAM, &yunsun16_priority	},	/* Priority */
	{ 0x800180, 0x800181, yunsun16_sound_bank_w		},	/* Sound */
	{ 0x800188, 0x800189, OKIM6295_data_0_lsb_w		},	/* */
	{ 0x8001fe, 0x8001ff, MWA16_NOP					},	/* ? 0 (during int) */
	{ 0x900000, 0x903fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	/* Palette */
	{ 0x908000, 0x90bfff, yunsun16_vram_1_w, &yunsun16_vram_1		},	/* Layer 1 */
	{ 0x90c000, 0x90ffff, yunsun16_vram_0_w, &yunsun16_vram_0		},	/* Layer 0 */
	{ 0x910000, 0x910fff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites */
MEMORY_END


static WRITE16_HANDLER( magicbub_sound_command_w )
{
	if (ACCESSING_LSB)
	{
/*
HACK: the game continuously sends this. It'll play the oki sample
number 0 on each voice. That sample is 00000-00000.
*/
		if ((data&0xff)!=0x3a)
		{
		soundlatch_w(0,data & 0xff);
		cpu_set_nmi_line(1,PULSE_LINE);
		}
	}
}

void init_magicbub(void)
{
/*	remove_mem_write16_handler (0, 0x800180, 0x800181 ); */
	install_mem_write16_handler(0, 0x800188, 0x800189, magicbub_sound_command_w);
}

/***************************************************************************


							Memory Maps - Sound CPU


***************************************************************************/

static MEMORY_READ_START( yunsun16_sound_readmem )
	{ 0x0000, 0xdfff, MRA_ROM		},	/* ROM */
	{ 0xe000, 0xe7ff, MRA_RAM		},	/* RAM */
MEMORY_END

static MEMORY_WRITE_START( yunsun16_sound_writemem )
	{ 0x0000, 0xdfff, MWA_ROM		},	/* ROM */
	{ 0xe000, 0xe7ff, MWA_RAM		},	/* RAM */
MEMORY_END

static PORT_READ_START( yunsun16_sound_readport )
	{ 0x10, 0x10, YM3812_status_port_0_r	},	/* YM3812 */
	{ 0x18, 0x18, soundlatch_r				},	/* From Main CPU */
	{ 0x1c, 0x1c, OKIM6295_status_0_r		},	/* M6295 */
PORT_END

static PORT_WRITE_START( yunsun16_sound_writeport )
	{ 0x10, 0x10, YM3812_control_port_0_w	},	/* YM3812 */
	{ 0x11, 0x11, YM3812_write_port_0_w		},
	{ 0x1c, 0x1c, OKIM6295_data_0_w			},	/* M6295 */
PORT_END


/***************************************************************************


								Input Ports


***************************************************************************/


/***************************************************************************
								Magic Bubble
***************************************************************************/

INPUT_PORTS_START( magicbub )

	PORT_START	/* IN0 - $800000.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - $800019.b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $80001b.b */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0018, 0x0010, "Unknown 1-4&5" )
	PORT_DIPSETTING(      0x0010, "0" )
	PORT_DIPSETTING(      0x0018, "2" )
	PORT_DIPSETTING(      0x0008, "4" )
	PORT_DIPSETTING(      0x0000, "8" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN3 - $80001d.b */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown 2-0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown 2-1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, "Lives (Vs Mode)" )
	PORT_DIPSETTING(      0x0008, "1" )
	PORT_DIPSETTING(      0x000c, "2" )
	PORT_DIPSETTING(      0x0004, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 2-4*" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 2-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 2-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 2-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
								Shocking
***************************************************************************/

INPUT_PORTS_START( shocking )

	PORT_START	/* IN0 - $800000.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - $800019.b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $80001b.b */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 1-3" )		/* rest unused */
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $80001d.b */
	PORT_DIPNAME( 0x0007, 0x0007, "Unknown 2-0&1&2" )
	PORT_DIPSETTING(      0x0004, "0" )
	PORT_DIPSETTING(      0x0005, "1" )
	PORT_DIPSETTING(      0x0006, "2" )
	PORT_DIPSETTING(      0x0007, "3" )
	PORT_DIPSETTING(      0x0003, "4" )
	PORT_DIPSETTING(      0x0002, "5" )
	PORT_DIPSETTING(      0x0001, "6" )
	PORT_DIPSETTING(      0x0000, "7" )
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

INPUT_PORTS_END




/***************************************************************************


							Graphics Layouts


***************************************************************************/


/* 16x16x4 */
static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(0,4) },
	{ STEP16(0,1) },
	{ STEP16(0,16) },
	16*16
};

/* 16x16x8 */
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 6*8,4*8, 2*8,0*8, 7*8,5*8, 3*8,1*8 },
	{ STEP8(0,1),STEP8(8*8,1) },
	{ STEP16(0,16*8) },
	16*16*8
};


static struct GfxDecodeInfo yunsun16_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8, 0x1000, 0x10 }, /* [0] Layers */
	{ REGION_GFX2, 0, &layout_16x16x4, 0x0000, 0x20 }, /* [1] Sprites */
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

/***************************************************************************
								Magic Bubble
***************************************************************************/

static void soundirq(int state)
{
	cpu_set_irq_line(1, 0, state);
}

static struct YM3812interface magicbub_ym3812_intf =
{
	1,
	4000000,		/* ? */
	{ 20 },
	{ soundirq },	/* IRQ Line */
};

static struct OKIM6295interface magicbub_okim6295_intf =
{
	1,
	{ 8000 },		/* ? */
	{ REGION_SOUND1 },
	{ 80 }
};

static struct MachineDriver machine_driver_magicbub =
{
	{
		{
			CPU_M68000,
			16000000,
			yunsun16_readmem, yunsun16_writemem,0,0,
			m68_level2_irq, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* ? */
			yunsun16_sound_readmem,  yunsun16_sound_writemem,
			yunsun16_sound_readport, yunsun16_sound_writeport,
			ignore_interrupt, 1	/* IRQ by YM3812; NMI by main CPU */
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	0x180, 0xe0, { 0+0x20, 0x180-1-0x20, 0, 0xe0-1 },
	yunsun16_gfxdecodeinfo,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	yunsun16_vh_start,
	0,
	yunsun16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ SOUND_YM3812,   &magicbub_ym3812_intf   },
		{ SOUND_OKIM6295, &magicbub_okim6295_intf }
	},
};


/***************************************************************************
								Shocking
***************************************************************************/

static struct OKIM6295interface shocking_okim6295_intf =
{
	1,
	{ 8000 },		/* ? */
	{ REGION_SOUND1 },
	{ 100 }
};

static struct MachineDriver machine_driver_shocking =
{
	{
		{
			CPU_M68000,
			16000000,
			yunsun16_readmem, yunsun16_writemem,0,0,
			m68_level2_irq, 1
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	0x180, 0xe0, { 0, 0x180-1-4, 0, 0xe0-1 },
	yunsun16_gfxdecodeinfo,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	yunsun16_vh_start,
	0,
	yunsun16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ SOUND_OKIM6295, &shocking_okim6295_intf }
	},
};



/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

								Magic Bubble

by Yung Sung YS102

U143 ---27c512
U23, 21, 22, 20, 131 ----27c010
U67, 68, 69, 70 --------27c040
U32, 33   ------------27c020

U143, 131  .........most likely sound
U32, 33 .............most likely program
U20-23........most likely sprites
U67-70 ..........most likely BG

A1020b is close to U67-70

68HC000 p16 is close to  U32,33

16.000000 Mhz

Sound section uses "KS8001" and "KS8002" and SMD --> Z80
and SMD "AD-65"

***************************************************************************/

ROM_START( magicbub )

	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "magbuble.u33", 0x000000, 0x040000, 0x18fdd582 )
	ROM_LOAD16_BYTE( "magbuble.u32", 0x000001, 0x040000, 0xf6ea7004 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "magbuble.143", 0x00000, 0x10000, 0x04192753 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16x8 */
	ROMX_LOAD( "magbuble.u67", 0x000000, 0x080000, 0x6355e57d, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "magbuble.u68", 0x000002, 0x080000, 0x53ae6c2b, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "magbuble.u69", 0x000004, 0x080000, 0xb892e64c, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "magbuble.u70", 0x000006, 0x080000, 0x37794837, ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 */
	ROM_LOAD( "magbuble.u20", 0x000000, 0x020000, 0xf70e3b8c )
	ROM_LOAD( "magbuble.u21", 0x020000, 0x020000, 0xad082cf3 )
	ROM_LOAD( "magbuble.u22", 0x040000, 0x020000, 0x7c68df7a )
	ROM_LOAD( "magbuble.u23", 0x060000, 0x020000, 0xc7763fc1 )

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "magbuble.131", 0x000000, 0x020000, 0x03e04e89 )

ROM_END


/***************************************************************************

								Shocking

***************************************************************************/

ROM_START( shocking )

	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "yunsun16.u33", 0x000000, 0x040000, 0x8a155521 )
	ROM_LOAD16_BYTE( "yunsun16.u32", 0x000001, 0x040000, 0xc4998c10 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16x8 */
	ROMX_LOAD( "yunsun16.u67", 0x000000, 0x080000, 0xe30fb2c4, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "yunsun16.u68", 0x000002, 0x080000, 0x7d702538, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "yunsun16.u69", 0x000004, 0x080000, 0x97447fec, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "yunsun16.u70", 0x000006, 0x080000, 0x1b1f7895, ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 */
	ROM_LOAD( "yunsun16.u20", 0x000000, 0x040000, 0x124699d0 )
	ROM_LOAD( "yunsun16.u21", 0x040000, 0x040000, 0x4eea29a2 )
	ROM_LOAD( "yunsun16.u22", 0x080000, 0x040000, 0xd6db0388 )
	ROM_LOAD( "yunsun16.u23", 0x0c0000, 0x040000, 0x1fa33b2e )

	ROM_REGION( 0x080000 * 2, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "yunsun16.131", 0x000000, 0x080000, 0xd0a1bb8c )
	ROM_RELOAD(               0x080000, 0x080000             )

ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 19??, magicbub, 0, magicbub, magicbub, magicbub, ROT0_16BIT, "Yun Sung", "Magic Bubble", GAME_NO_COCKTAIL )
GAMEX( 1997, shocking, 0, shocking, shocking, 0,        ROT0_16BIT, "Yun Sung", "Shocking",     GAME_NO_COCKTAIL )
