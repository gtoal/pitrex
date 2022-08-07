/***************************************************************************

							  -= Blomby Car =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU    :  68000
Video Chips  :	TI TPC1020AFN-084 ?
Sound Chips  :	OKI M6295

To Do:

- Flip screen unused ?)
- Better driving wheel(s) support

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern data16_t *blmbycar_vram_0, *blmbycar_scroll_0;
extern data16_t *blmbycar_vram_1, *blmbycar_scroll_1;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( blmbycar_palette_w );

WRITE16_HANDLER( blmbycar_vram_0_w );
WRITE16_HANDLER( blmbycar_vram_1_w );

int  blmbycar_vh_start(void);
void blmbycar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


								Sound Banking


***************************************************************************/

/* The top 64k of samples are banked (16 banks total) */

WRITE16_HANDLER( blmbycar_okibank_w )
{
	if (ACCESSING_LSB)
	{
		unsigned char *RAM = memory_region(REGION_SOUND1);
		memcpy(&RAM[0x30000],&RAM[0x40000 + 0x10000*(data & 0xf)],0x10000);
	}
}

/***************************************************************************


								Input Handling


***************************************************************************/

/* Preliminary potentiometric wheel support */

static data8_t pot_wheel = 0;

static WRITE16_HANDLER( blmbycar_pot_wheel_reset_w )
{
	if (ACCESSING_LSB)
		pot_wheel = ~readinputport(2) & 0xff;
}

static WRITE16_HANDLER( blmbycar_pot_wheel_shift_w )
{
	if (ACCESSING_LSB)
	{
		static int old;
		if ( ((old & 0xff) == 0xff) && ((data & 0xff) == 0) )
			pot_wheel <<= 1;
		old = data;
	}
}

static READ16_HANDLER( blmbycar_pot_wheel_r )
{
	return	((pot_wheel & 0x80) ? 0x04 : 0) |
			(rand() & 0x08);
}


/* Preliminary optical wheel support */

static READ16_HANDLER( blmbycar_opt_wheel_r )
{
	return	(~readinputport(2) & 0xff) << 8;
}


/***************************************************************************


								Memory Maps


***************************************************************************/

static MEMORY_READ16_START( blmbycar_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	/* ROM */
	{ 0xfec000, 0xfeffff, MRA16_RAM					},	/* RAM */
	{ 0x200000, 0x203fff, MRA16_RAM					},	/* */
	{ 0x204000, 0x2045ff, MRA16_RAM					},	/* Palette */
	{ 0x204600, 0x207fff, MRA16_RAM					},	/* */
	{ 0x104000, 0x105fff, MRA16_RAM					},	/* Layer 1 */
	{ 0x106000, 0x107fff, MRA16_RAM					},	/* Layer 0 */
	{ 0x440000, 0x441fff, MRA16_RAM					},	/* */
	{ 0x444000, 0x445fff, MRA16_RAM					},	/* Sprites (size?) */
	{ 0x700000, 0x700001, input_port_0_word_r		},	/* 2 x DSW */
	{ 0x700002, 0x700003, input_port_1_word_r		},	/* Joystick + Buttons */
	{ 0x700004, 0x700005, blmbycar_opt_wheel_r		},	/* Wheel (optical) */
	{ 0x700006, 0x700007, input_port_3_word_r		},	/* */
	{ 0x700008, 0x700009, blmbycar_pot_wheel_r		},	/* Wheel (potentiometer) */
	{ 0x70000e, 0x70000f, OKIM6295_status_0_lsb_r	},	/* Sound */
MEMORY_END

static MEMORY_WRITE16_START( blmbycar_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM								},	/* ROM */
	{ 0xfec000, 0xfeffff, MWA16_RAM								},	/* RAM */
	{ 0x100000, 0x103fff, MWA16_RAM								},	/* */
	{ 0x104000, 0x105fff, blmbycar_vram_1_w, &blmbycar_vram_1	},	/* Layer 1 */
	{ 0x106000, 0x107fff, blmbycar_vram_0_w, &blmbycar_vram_0	},	/* Layer 0 */
	{ 0x108000, 0x10bfff, MWA16_RAM								},	/* */
	{ 0x10c000, 0x10c003, MWA16_RAM, &blmbycar_scroll_1			},	/* Scroll 1 */
	{ 0x10c004, 0x10c007, MWA16_RAM, &blmbycar_scroll_0			},	/* Scroll 0 */
	{ 0x200000, 0x203fff, MWA16_RAM								},	/* */
	{ 0x204000, 0x2045ff, blmbycar_palette_w, &paletteram16		},	/* Palette */
	{ 0x204600, 0x207fff, MWA16_RAM								},	/* */
	{ 0x440000, 0x441fff, MWA16_RAM								},	/* */
	{ 0x444000, 0x445fff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites (size?) */
	{ 0x70000a, 0x70000b, MWA16_NOP								},	/* ? Wheel */
	{ 0x70000c, 0x70000d, blmbycar_okibank_w					},	/* Sound */
	{ 0x70000e, 0x70000f, OKIM6295_data_0_lsb_w					},	/* */
	{ 0x70006a, 0x70006b, blmbycar_pot_wheel_reset_w			},	/* Wheel (potentiometer) */
	{ 0x70007a, 0x70007b, blmbycar_pot_wheel_shift_w			},	/* */
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/

INPUT_PORTS_START( blmbycar )

	PORT_START	/* IN0 - $700000.w */
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy"    )
	PORT_DIPSETTING(      0x0003, "Normal"  )
	PORT_DIPSETTING(      0x0001, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, "Joysticks" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPNAME( 0x0018, 0x0018, "Controls" )
	PORT_DIPSETTING(      0x0018, "Joystick" )
/*	PORT_DIPSETTING(      0x0010, "Pot Wheel" )	// Preliminary */
/*	PORT_DIPSETTING(      0x0008, "Opt Wheel" )	// Preliminary */
/*	PORT_DIPSETTING(      0x0000, DEF_STR( Unused ) )	// Time goes to 0 rally fast! */
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Credits To Start" )
	PORT_DIPSETTING(      0x4000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN1 - $700002.w */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	| IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2  )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	| IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* IN2 - $700004.w */
	PORT_ANALOG ( 0x00ff, 0x0080, IPT_AD_STICK_X, 30, 1, 0x00, 0xff )

	PORT_START	/* IN3 - $700006.w */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END



/***************************************************************************


								Graphics Layouts


***************************************************************************/

/* 16x16x4 tiles (made of four 8x8 tiles) */
static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4),RGN_FRAC(2,4),RGN_FRAC(1,4),RGN_FRAC(0,4) },
	{ STEP8(0,1), STEP8(8*8*2,1) },
	{ STEP8(0,8), STEP8(8*8*1,8) },
	16*16
};

/* Layers both use the first $20 color codes. Sprites the next $10 */
static struct GfxDecodeInfo blmbycar_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0x0, 0x30 }, /* [0] Layers + Sprites */
	{ -1 }
};



/***************************************************************************


								Machine Drivers


***************************************************************************/

static struct OKIM6295interface blmbycar_okim6295_interface =
{
	1,
	{ 8000 },		/* ? */
	{ REGION_SOUND1 },
	{ 100 }
};

static struct MachineDriver machine_driver_blmbycar =
{
	{
		{
			CPU_M68000,
			10000000,	/* ? */
			blmbycar_readmem, blmbycar_writemem,0,0,
			m68_level1_irq, 1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	0x180, 0x100, { 0, 0x180-1, 0, 0x100-1 },
	blmbycar_gfxdecodeinfo,
	0x300, 0x300,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	blmbycar_vh_start,
	0,
	blmbycar_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_OKIM6295,	&blmbycar_okim6295_interface	}
	}
};




/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

								Blomby Car
Abm & Gecas, 1990.

CPU : 68000
SND : Oki M6295 (samples only)
OSC : 30.000 + 24.000
DSW : 2 x 8
GFX : TI TPC1020AFN-084

***************************************************************************/

ROM_START( blmbycar )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "bc_rom4", 0x000000, 0x080000, 0x76f054a2 )
	ROM_LOAD16_BYTE( "bc_rom6", 0x000001, 0x080000, 0x2570b4c5 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "bc_rom7",   0x000000, 0x080000, 0xe55ca79b )
	ROM_LOAD( "bc_rom8",   0x080000, 0x080000, 0xcdf38c96 )
	ROM_LOAD( "bc_rom9",   0x100000, 0x080000, 0x0337ab3d )
	ROM_LOAD( "bc_rom10",  0x180000, 0x080000, 0x5458917e )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* 8 bit adpcm (banked) */
	ROM_LOAD( "bc_rom1",     0x040000, 0x080000, 0xac6f8ba1 )
	ROM_LOAD( "bc_rom2",     0x0c0000, 0x080000, 0xa4bc31bf )
	ROM_COPY( REGION_SOUND1, 0x040000, 0x000000,   0x040000 )
ROM_END



/***************************************************************************


								Game Drivers


***************************************************************************/

GAME( 1994, blmbycar, 0, blmbycar, blmbycar, 0, ROT0, "ABM & Gecas", "Blomby Car" )
