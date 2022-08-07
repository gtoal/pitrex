/***************************************************************************

							  -= Tetris Plus 2 =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU    :  TMP68HC000P-12

Video Chips  :	SS91022-03 9428XX001
				GS91022-04 9721PD008
				SS91022-05 9347EX002
				GS91022-05 048 9726HX002

Sound Chips  :	Yamaha YMZ280B-F

Other        :  XILINX XC5210 PQ240C X68710M AKJ9544
				XC7336 PC44ACK9633 A63458A
				NVRAM


To Do:

-	There is a 3rd unimplemented layer capable of rotation (not used by
	the game, can be tested in service mode).
-	Priority RAM is not taken into account.

Notes:

-	The Japan set doesn't seem to have (or use) NVRAM. I can't enter
	a test mode or use the service coin either !?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern data16_t *tetrisp2_vram_0, *tetrisp2_scroll_0;
extern data16_t *tetrisp2_vram_1, *tetrisp2_scroll_1;

extern data16_t *tetrisp2_priority;
extern data16_t *tetrisp2_vregs;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( tetrisp2_palette_w );
WRITE16_HANDLER( tetrisp2_priority_w );

WRITE16_HANDLER( tetrisp2_vram_0_w );
WRITE16_HANDLER( tetrisp2_vram_1_w );

int  tetrisp2_vh_start(void);
void tetrisp2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


									Sound


***************************************************************************/

static READ16_HANDLER( tetrisp2_sound_r )
{
	return YMZ280B_status_0_r(offset);
}

static WRITE16_HANDLER( tetrisp2_sound_w )
{
	if (ACCESSING_LSB)
	{
		if (offset)	YMZ280B_data_0_w     (offset, data & 0xff);
		else		YMZ280B_register_0_w (offset, data & 0xff);
	}
}

/***************************************************************************


								Protection


***************************************************************************/

static READ16_HANDLER( tetrisp2_ip_1_word_r )
{
	return	( readinputport(1) &  0xfcff ) |
			(           rand() & ~0xfcff ) |
			(      1 << (8 + (rand()&1)) );
}


/***************************************************************************


									NVRAM


***************************************************************************/

static data16_t *tetrisp2_nvram;
static size_t tetrisp2_nvram_size;

void tetrisp2_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file,tetrisp2_nvram,tetrisp2_nvram_size);
	else
	{
		if (file)
			osd_fread(file,tetrisp2_nvram,tetrisp2_nvram_size);
		else
		{
			/* fill in the default values */
			memset(tetrisp2_nvram,0,tetrisp2_nvram_size);
		}
	}
}


/* The game only ever writes even bytes and reads odd bytes */
READ16_HANDLER( tetrisp2_nvram_r )
{
	return	( (tetrisp2_nvram[offset] >> 8) & 0x00ff ) |
			( (tetrisp2_nvram[offset] << 8) & 0xff00 ) ;
}

WRITE16_HANDLER( tetrisp2_nvram_w )
{
	COMBINE_DATA(&tetrisp2_nvram[offset]);
}


/***************************************************************************


								Memory Map


***************************************************************************/

static MEMORY_READ16_START( tetrisp2_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	/* ROM */
	{ 0x100000, 0x103fff, MRA16_RAM				},	/* Object RAM */
	{ 0x104000, 0x107fff, MRA16_RAM				},	/* Spare Object RAM */
	{ 0x108000, 0x10ffff, MRA16_RAM				},	/* Work RAM */
	{ 0x200000, 0x27ffff, MRA16_RAM				},	/* Priority */
	{ 0x300000, 0x31ffff, MRA16_RAM				},	/* Palette */
	{ 0x400000, 0x403fff, MRA16_RAM				},	/* Foreground */
	{ 0x404000, 0x407fff, MRA16_RAM				},	/* Background */
	{ 0x500000, 0x507fff, MRA16_RAM				},	/* Rotation Data ? */
	{ 0x600000, 0x607fff, MRA16_RAM				},	/* Rotation Layer ? */
	{ 0x608000, 0x60bfff, MRA16_RAM				},	/* */
	{ 0x902000, 0x903fff, tetrisp2_nvram_r		},	/* NVRAM */
	{ 0x904000, 0x904005, tetrisp2_nvram_r		},	/* NVRAM (mirror) */
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	/* ? */
	{ 0xbe0002, 0xbe0003, input_port_0_word_r	},	/* Inputs */
	{ 0xbe0004, 0xbe0005, tetrisp2_ip_1_word_r	},	/* */
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	/* */
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	/* Watchdog */
MEMORY_END

static MEMORY_WRITE16_START( tetrisp2_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM							},	/* ROM */
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Object RAM */
	{ 0x104000, 0x107fff, MWA16_RAM							},	/* Spare Object RAM */
	{ 0x108000, 0x10ffff, MWA16_RAM							},	/* Work RAM */
	{ 0x200000, 0x27ffff, tetrisp2_priority_w, &tetrisp2_priority	},	/* Priority */
	{ 0x300000, 0x31ffff, tetrisp2_palette_w,  &paletteram16		},	/* Palette */
	{ 0x400000, 0x403fff, tetrisp2_vram_1_w,   &tetrisp2_vram_1		},	/* Foreground */
	{ 0x404000, 0x407fff, tetrisp2_vram_0_w,   &tetrisp2_vram_0		},	/* Background */
	{ 0x500000, 0x507fff, MWA16_RAM							},	/* Rotation Data ? */
	{ 0x600000, 0x607fff, MWA16_RAM							},	/* Rotation Layer ? */
	{ 0x608000, 0x60bfff, MWA16_RAM							},	/* */
	{ 0x800000, 0x800003, tetrisp2_sound_w					},	/* Sound */
	{ 0x902000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	/* NVRAM */
	{ 0x904000, 0x904005, tetrisp2_nvram_w					},	/* NVRAM (mirror) */
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_1		},	/* Scrolling */
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_0		},	/* */
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_vregs		},	/* Video Registers */
	{ 0xba001a, 0xba001b, MWA16_NOP							},	/* ? 0 (Lev 4 irq ack?) */
	{ 0xba001e, 0xba001f, MWA16_NOP							},	/* ? 0 (Lev 2 irq ack?) */
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
							Tetris Plus 2 (World)
***************************************************************************/

INPUT_PORTS_START( tetrisp2 )

	PORT_START	/* IN0 - $be0002.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused button */

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused button */

	PORT_START	/* IN1 - $be0004.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? */
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? */
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN2 - $be0008.w */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0200, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Vs Mode Rounds" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPNAME( 0x0800, 0x0000, "Language" )
	PORT_DIPSETTING(      0x0800, "Japanese" )
	PORT_DIPSETTING(      0x0000, "English" )
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown 2-4" )	/* F.B.I. Logo (in the USA set?) */
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
							Tetris Plus 2 (Japan)
***************************************************************************/


INPUT_PORTS_START( teplus2j )

	PORT_START	/* IN0 - $be0002.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused button */

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused button */

	PORT_START	/* IN1 - $be0004.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )	/* no service coin? */
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? */
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SPECIAL  )	/* ? */
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

/*
	The code for checking the "service mode" and "free play" DSWs
	is (deliberately?) bugged in this set
*/
	PORT_START	/* IN2 - $be0008.w */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0200, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Vs Mode Rounds" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPNAME( 0x0800, 0x0800, "Language" )
	PORT_DIPSETTING(      0x0800, "Japanese" )
/*	PORT_DIPSETTING(      0x0000, "English" )	// it does not work properly with "english" */
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown 2-4" )	/* F.B.I. Logo (in the USA set?) */
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************


							Graphics Layouts


***************************************************************************/


/* 8x8x8 tiles */
static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	{ STEP8(0,8*8) },
	8*8*8
};

/* 16x16x8 tiles */
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP16(0,8) },
	{ STEP16(0,16*8) },
	16*16*8
};


static struct GfxDecodeInfo tetrisp2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8,   0x0000, 0x10 }, /* [0] Sprites */
	{ REGION_GFX2, 0, &layout_16x16x8, 0x1000, 0x10 }, /* [1] Background */
	{ REGION_GFX3, 0, &layout_8x8x8,   0x6000, 0x10 }, /* [2] Foreground */
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static struct YMZ280Binterface ymz280b_intf =
{
	1,
	{ 16934400 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }	/* irq */
};

int tetrisp2_interrupt(void)
{
	switch ( cpu_getiloops() )
	{
		case 0:	return 2;	/* vblank? */
		case 1:	return 4;
		case 2:	return 1;
	}
	return ignore_interrupt();
}

static struct MachineDriver machine_driver_tetrisp2 =
{
	{
		{
			CPU_M68000,
			12000000,
			tetrisp2_readmem, tetrisp2_writemem,0,0,
			tetrisp2_interrupt, 3
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	0x140, 0xe0, { 0, 0x140-1, 0, 0xe0-1 },
	tetrisp2_gfxdecodeinfo,
	0x8000, 0x8000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	tetrisp2_vh_start,
	0,
	tetrisp2_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ SOUND_YMZ280B, &ymz280b_intf }
	},

	tetrisp2_nvram_handler
};



/***************************************************************************


								ROMs Loading


***************************************************************************/


/***************************************************************************

								Tetris Plus 2

(C) Jaleco 1996

TP-97222
96019 EB-00-20117-0
MDK332V-0

BRIEF HARDWARE OVERVIEW

Toshiba TMP68HC000P-12
Yamaha YMZ280B-F
OSC: 12.000Mhz, 48.000Mhz, 16.9344Mhz

Listing of custom chips. (Some on scan are hard to read).

IC38	JALECO SS91022-03 9428XX001
IC31	JALECO SS91022-05 9347EX002
IC32	JALECO GS91022-05    048  9726HX002
IC30	JALECO GS91022-04 9721PD008
IC39	XILINX XC5210 PQ240C X68710M AKJ9544
IC49	XILINX XC7336 PC44ACK9633 A63458A

***************************************************************************/

ROM_START( tetrisp2 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "t2p_04.rom", 0x000000, 0x080000, 0xe67f9c51 )
	ROM_LOAD16_BYTE( "t2p_01.rom", 0x000001, 0x080000, 0x5020a4ed )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "96019-01.9", 0x000000, 0x400000, 0x06f7dc64 )
	ROM_LOAD32_WORD( "96019-02.8", 0x000002, 0x400000, 0x3e613bed )
	/* If t2p_m01&2 from this board were correctly read, since they
	   hold the same data of the above but with swapped halves, it
	   means they had to invert the top bit of the "page select"
	   register in the sprite's hardware on this board! */

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD( "96019-06.13", 0x000000, 0x400000, 0x16f7093c )
	ROM_LOAD( "96019-04.6",  0x400000, 0x100000, 0xb849dec9 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "tetp2-10.bin", 0x000000, 0x080000, 0x34dd1bad )	/* 11111xxxxxxxxxxxxxx = 0xFF */

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, 0xa8a61954 )

ROM_END


/***************************************************************************

							Tetris Plus 2 (Japan)

(c)1997 Jaleco / The Tetris Company

TP-97222
96019 EB-00-20117-0

CPU:	68000-12
Sound:	YMZ280B-F
OSC:	12.000MHz
		48.0000MHz
		16.9344MHz

Custom:	SS91022-03
		GS91022-04
		GS91022-05
		SS91022-05

***************************************************************************/

ROM_START( teplus2j )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tet2-4v2.2", 0x000000, 0x080000, 0x5bfa32c8 )	/* v2.2 */
	ROM_LOAD16_BYTE( "tet2-1v2.2", 0x000001, 0x080000, 0x919116d0 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "96019-01.9", 0x000000, 0x400000, 0x06f7dc64 )
	ROM_LOAD32_WORD( "96019-02.8", 0x000002, 0x400000, 0x3e613bed )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD( "96019-06.13", 0x000000, 0x400000, 0x16f7093c )
	ROM_LOAD( "96019-04.6",  0x400000, 0x100000, 0xb849dec9 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "tetp2-10.bin", 0x000000, 0x080000, 0x34dd1bad )	/* 11111xxxxxxxxxxxxxx = 0xFF */

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, 0xa8a61954 )

ROM_END

/***************************************************************************


								Game Drivers


***************************************************************************/

/* Better leave the Japanese set as parent as long as it's the only good dump */
GAMEX( 1997, tetrisp2, 0,        tetrisp2, tetrisp2, 0, ROT0_16BIT, "Jaleco / The Tetris Company", "Tetris Plus 2 (World?)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, teplus2j, tetrisp2, tetrisp2, teplus2j, 0, ROT0_16BIT, "Jaleco / The Tetris Company", "Tetris Plus 2 (Japan)", GAME_IMPERFECT_GRAPHICS )
