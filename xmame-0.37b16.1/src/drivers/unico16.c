/***************************************************************************

						  -= Unico 16 Bit Games =-

					driver by	Luca Elia (l.elia@tin.it)


CPU			:	M68000
Sound Chips	:	OKI M6295 (AD-65) + YM3812 (K-666)
Video Chips	:	3 x Actel A1020B (Square 84 Pin Socketed)
				MACH211 (Square 44 Pin Socketed) [Or MACH210-15JC]


---------------------------------------------------------------------------
Year + Game			PCB				Notes
---------------------------------------------------------------------------
97	Burglar X		?
98	Zero Point		ZPM1001A/B		Has Light Guns.
---------------------------------------------------------------------------


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables needed by vidhrdw: */

int unico16_has_lightgun;

/* Variables defined in vidhrdw: */

extern data16_t *unico16_vram_0, *unico16_scrollx_0, *unico16_scrolly_0;
extern data16_t *unico16_vram_1, *unico16_scrollx_1, *unico16_scrolly_1;
extern data16_t *unico16_vram_2, *unico16_scrollx_2, *unico16_scrolly_2;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( unico16_vram_0_w );
WRITE16_HANDLER( unico16_vram_1_w );
WRITE16_HANDLER( unico16_vram_2_w );
WRITE16_HANDLER( unico16_palette_w );

int  unico16_vh_start(void);
void unico16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/***************************************************************************


								Memory Maps


***************************************************************************/

READ16_HANDLER ( YM3812_status_port_0_msb_r )	{	return YM3812_status_port_0_r(0) << 8;	}
WRITE16_HANDLER( YM3812_register_port_0_msb_w )	{	if (ACCESSING_MSB)	YM3812_control_port_0_w(0,data >> 8);	}
WRITE16_HANDLER( YM3812_data_port_0_msb_w )		{	if (ACCESSING_MSB)	YM3812_write_port_0_w(0,data >> 8);		}


/*
 Lines starting with an empty comment in the following MemoryReadAddress
 arrays are there for debug (e.g. the game does not read from those ranges
 AFAIK)
*/

/***************************************************************************
								Burglar X
***************************************************************************/

static WRITE16_HANDLER( burglarx_sound_bank_w )
{
	if (ACCESSING_MSB)
	{
		int bank = (data >> 8 ) & 1;
		OKIM6295_set_bank_base(0, 0x40000 * bank );
	}
}

static MEMORY_READ16_START( readmem_burglarx )
	{ 0x000000, 0x0fffff, MRA16_ROM						},	/* ROM */
	{ 0xff0000, 0xffffff, MRA16_RAM						},	/* RAM */
	{ 0x800000, 0x800001, input_port_0_word_r			},	/* P1 + P2 */
	{ 0x800018, 0x800019, input_port_1_word_r			},	/* Buttons */
	{ 0x80001a, 0x80001b, input_port_2_word_r			},	/* DSW */
	{ 0x80001c, 0x80001d, input_port_3_word_r			},	/* DSW */
	{ 0x800188, 0x800189, OKIM6295_status_0_lsb_r		},	/* Sound */
	{ 0x80018c, 0x80018d, YM3812_status_port_0_msb_r	},	/* */
/**/{ 0x904000, 0x907fff, MRA16_RAM						},	/* Layers */
/**/{ 0x908000, 0x90bfff, MRA16_RAM						},	/* */
/**/{ 0x90c000, 0x90ffff, MRA16_RAM						},	/* */
/**/{ 0x920000, 0x923fff, MRA16_RAM						},	/* ? 0 */
/**/{ 0x930000, 0x9307ff, MRA16_RAM						},	/* Sprites */
/**/{ 0x940000, 0x947fff, MRA16_RAM						},	/* Palette */
MEMORY_END

static MEMORY_WRITE16_START( writemem_burglarx )
	{ 0x000000, 0x0fffff, MWA16_ROM							},	/* ROM */
	{ 0xff0000, 0xffffff, MWA16_RAM							},	/* RAM */
	{ 0x800030, 0x800031, MWA16_NOP							},	/* ? 0 */
	{ 0x80010c, 0x80010d, MWA16_RAM, &unico16_scrollx_0		},	/* Scroll */
	{ 0x80010e, 0x80010f, MWA16_RAM, &unico16_scrolly_0		},	/* */
	{ 0x800110, 0x800111, MWA16_RAM, &unico16_scrolly_2		},	/* */
	{ 0x800114, 0x800115, MWA16_RAM, &unico16_scrollx_2		},	/* */
	{ 0x800116, 0x800117, MWA16_RAM, &unico16_scrollx_1		},	/* */
	{ 0x800120, 0x800121, MWA16_RAM, &unico16_scrolly_1		},	/* */
	{ 0x800188, 0x800189, OKIM6295_data_0_lsb_w				},	/* Sound */
	{ 0x80018a, 0x80018b, YM3812_data_port_0_msb_w			},	/* */
	{ 0x80018c, 0x80018d, YM3812_register_port_0_msb_w		},	/* */
	{ 0x80018e, 0x80018f, burglarx_sound_bank_w				},	/* */
	{ 0x8001e0, 0x8001e1, MWA16_RAM							},	/* ? IRQ Ack */
	{ 0x904000, 0x907fff, unico16_vram_1_w, &unico16_vram_1	},	/* Layers */
	{ 0x908000, 0x90bfff, unico16_vram_2_w, &unico16_vram_2	},	/* */
	{ 0x90c000, 0x90ffff, unico16_vram_0_w, &unico16_vram_0	},	/* */
	{ 0x920000, 0x923fff, MWA16_RAM							},	/* ? 0 */
	{ 0x930000, 0x9307ff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites */
	{ 0x940000, 0x947fff, unico16_palette_w, &paletteram16	},	/* Palette */
MEMORY_END



/***************************************************************************
								Zero Point
***************************************************************************/

static WRITE16_HANDLER( zeropnt_sound_bank_w )
{
	if (ACCESSING_MSB)
	{
		/* Banked sound samples. The 3rd quarter of the ROM
		   contains garbage. Indeed, only banks 0&1 are used */

		int bank = (data >> 8 ) & 1;
		unsigned char *dst	= memory_region(REGION_SOUND1);
		unsigned char *src	= dst + 0x80000 + 0x20000 + 0x20000 * bank;
		memcpy(dst + 0x20000, src, 0x20000);

		coin_counter_w(0,data & 0x1000);
		set_led_status(0,data & 0x0800);	/* Start 1 */
		set_led_status(1,data & 0x0400);	/* Start 2 */
	}
}

/* Light Gun - need to wiggle the input slightly otherwise fire doesn't work */
READ16_HANDLER( unico16_gunx_0_msb_r )	{	return ((readinputport(4)&0xff) ^ (cpu_getcurrentframe()&1))<<8;	}
READ16_HANDLER( unico16_guny_0_msb_r )	{	return ((readinputport(3)&0xff) ^ (cpu_getcurrentframe()&1))<<8;	}
READ16_HANDLER( unico16_gunx_1_msb_r )	{	return ((readinputport(6)&0xff) ^ (cpu_getcurrentframe()&1))<<8;	}
READ16_HANDLER( unico16_guny_1_msb_r )	{	return ((readinputport(5)&0xff) ^ (cpu_getcurrentframe()&1))<<8;	}

static MEMORY_READ16_START( readmem_zeropnt )
	{ 0x000000, 0x0fffff, MRA16_ROM						},	/* ROM */
	{ 0xef0000, 0xefffff, MRA16_RAM						},	/* RAM */
	{ 0x800018, 0x800019, input_port_0_word_r			},	/* Buttons */
	{ 0x80001a, 0x80001b, input_port_1_word_r			},	/* DSW */
	{ 0x80001c, 0x80001d, input_port_2_word_r			},	/* DSW */
	{ 0x800170, 0x800171, unico16_guny_0_msb_r			},	/* Light Guns */
	{ 0x800174, 0x800175, unico16_gunx_0_msb_r			},	/* */
	{ 0x800178, 0x800179, unico16_guny_1_msb_r			},	/* */
	{ 0x80017c, 0x80017d, unico16_gunx_1_msb_r			},	/* */
	{ 0x800188, 0x800189, OKIM6295_status_0_lsb_r		},	/* Sound */
	{ 0x80018c, 0x80018d, YM3812_status_port_0_msb_r	},	/* */
/**/{ 0x904000, 0x907fff, MRA16_RAM						},	/* Layers */
/**/{ 0x908000, 0x90bfff, MRA16_RAM						},	/* */
/**/{ 0x90c000, 0x90ffff, MRA16_RAM						},	/* */
/**/{ 0x920000, 0x923fff, MRA16_RAM						},	/* ? 0 */
/**/{ 0x930000, 0x9307ff, MRA16_RAM						},	/* Sprites */
/**/{ 0x940000, 0x947fff, MRA16_RAM						},	/* Palette */
MEMORY_END

static MEMORY_WRITE16_START( writemem_zeropnt )
	{ 0x000000, 0x0fffff, MWA16_ROM							},	/* ROM */
	{ 0xef0000, 0xefffff, MWA16_RAM							},	/* RAM */
	{ 0x800030, 0x800031, MWA16_NOP							},	/* ? 0 */
	{ 0x80010c, 0x80010d, MWA16_RAM, &unico16_scrollx_0		},	/* Scroll */
	{ 0x80010e, 0x80010f, MWA16_RAM, &unico16_scrolly_0		},	/* */
	{ 0x800110, 0x800111, MWA16_RAM, &unico16_scrolly_2		},	/* */
	{ 0x800114, 0x800115, MWA16_RAM, &unico16_scrollx_2		},	/* */
	{ 0x800116, 0x800117, MWA16_RAM, &unico16_scrollx_1		},	/* */
	{ 0x800120, 0x800121, MWA16_RAM, &unico16_scrolly_1		},	/* */
	{ 0x800188, 0x800189, OKIM6295_data_0_lsb_w				},	/* Sound */
	{ 0x80018a, 0x80018b, YM3812_data_port_0_msb_w			},	/* */
	{ 0x80018c, 0x80018d, YM3812_register_port_0_msb_w		},	/* */
	{ 0x80018e, 0x80018f, zeropnt_sound_bank_w				},	/* */
	{ 0x8001e0, 0x8001e1, MWA16_RAM							},	/* ? IRQ Ack */
	{ 0x904000, 0x907fff, unico16_vram_1_w, &unico16_vram_1	},	/* Layers */
	{ 0x908000, 0x90bfff, unico16_vram_2_w, &unico16_vram_2	},	/* */
	{ 0x90c000, 0x90ffff, unico16_vram_0_w, &unico16_vram_0	},	/* */
	{ 0x920000, 0x923fff, MWA16_RAM							},	/* ? 0 */
	{ 0x930000, 0x9307ff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites */
	{ 0x940000, 0x947fff, unico16_palette_w, &paletteram16	},	/* Palette */
MEMORY_END



/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
								Burglar X
***************************************************************************/

INPUT_PORTS_START( burglarx )

	PORT_START	/* IN0 - $800000.w */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - $800019.b */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN2 - $80001a.b */
	PORT_BIT(     0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown 1-4" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_4C ) )

	PORT_START	/* IN3 - $80001c.b */
	PORT_BIT(     0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0200, "None" )
	PORT_DIPSETTING(      0x0300, "A" )
	PORT_DIPSETTING(      0x0100, "B" )
	PORT_DIPSETTING(      0x0000, "C" )
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Energy" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0800, "3" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x8000, "2" )
	PORT_DIPSETTING(      0xc000, "3" )
	PORT_DIPSETTING(      0x4000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )

INPUT_PORTS_END



/***************************************************************************
								Zero Point
***************************************************************************/

INPUT_PORTS_START( zeropnt )

	PORT_START	/* IN0 - $800018.w */
	PORT_BIT(  0x0001, IP_ACTIVE_HIGH, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_HIGH, IPT_COIN2    )
	PORT_BITX( 0x0004, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0008, IP_ACTIVE_HIGH, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_START1   )
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_START2   )
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_SERVICE1 )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $80001a.b */
	PORT_BIT(     0x00ff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0100, 0x0000, "Unknown 1-0" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, "Unknown 1-2" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1000, 0x0000, "Unknown 1-4" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0xe000, 0x0000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_4C ) )

	PORT_START	/* IN2 - $80001c.b */
	PORT_BIT(     0x00ff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0100, 0x0000, "Unknown 2-0" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0000, "Unknown 2-1" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, "Unknown 2-2" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0000, "Unknown 2-3" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1000, "Easy" )
	PORT_DIPSETTING(      0x0000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Hard?" )
	PORT_DIPSETTING(      0x3000, "Hardest?" )
	PORT_DIPNAME( 0xc000, 0x0000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x4000, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x8000, "4" )
	PORT_DIPSETTING(      0xc000, "5" )

	PORT_START	/* IN3 - $800170.b */
	PORT_ANALOG( 0xff, 0x88, IPT_AD_STICK_Y | IPF_PLAYER2, 35, 15, 0x18, 0xf8 )

	PORT_START	/* IN4 - $800174.b */
	PORT_ANALOG( 0x1ff, 0xa4, IPT_AD_STICK_X | IPF_PLAYER2, 35, 15, 0x30, 0x118 )

	PORT_START	/* IN5 - $800178.b */
	PORT_ANALOG( 0xff, 0x88, IPT_AD_STICK_Y | IPF_PLAYER1, 35, 15, 0x18, 0xf8 )

	PORT_START	/* IN6 - $80017c.b */
	PORT_ANALOG( 0x1ff, 0xa4, IPT_AD_STICK_X | IPF_PLAYER1, 35, 15, 0x30, 0x118 )

INPUT_PORTS_END



/***************************************************************************


							Graphics Layouts


***************************************************************************/

/* 16x16x8 */
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,4),
	8,
	{	RGN_FRAC(3,4)+8,	RGN_FRAC(3,4)+0,
		RGN_FRAC(2,4)+8,	RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+8,	RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8,	RGN_FRAC(0,4)+0	},
	{	STEP8(0,1), 		STEP8(16,1)		},
	{	STEP16(0,16*2)						},
	16*16*2
};

static struct GfxDecodeInfo unico16_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8, 0x0, 0x20 }, /* [0] Sprites */
	{ REGION_GFX2, 0, &layout_16x16x8, 0x0, 0x20 }, /* [1] Layers */
	{ -1 }
};



/***************************************************************************


								Machine Drivers


***************************************************************************/

void init_machine_unico16(void)
{
	unico16_has_lightgun = 0;
}

static struct YM3812interface unico16_ym3812_intf =
{
	1,
	4000000,		/* ? */
	{ 20 },
	{ 0 },	/* IRQ Line */
};

static struct OKIM6295interface unico16_m6295_intf =
{
	1,
	{ 8000 },		/* ? */
	{ REGION_SOUND1 },
	{ 80 }
};


/***************************************************************************
								Burglar X
***************************************************************************/

static struct MachineDriver machine_driver_burglarx =
{
	{
		{
			CPU_M68000,
			16000000,
			readmem_burglarx, writemem_burglarx,0,0,
			m68_level2_irq, 1
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_unico16,

	/* video hardware */
	0x180, 0xe0, { 0, 0x180-1, 0, 0xe0-1 },
	unico16_gfxdecodeinfo,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	unico16_vh_start,
	0,
	unico16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM3812,	&unico16_ym3812_intf	},
		{	SOUND_OKIM6295, &unico16_m6295_intf		}
	},
};



/***************************************************************************
								Zero Point
***************************************************************************/

void init_machine_zeropt(void)
{
	init_machine_unico16();
	unico16_has_lightgun = 1;
}

static struct MachineDriver machine_driver_zeropnt =
{
	{
		{
			CPU_M68000,
			16000000,
			readmem_zeropnt, writemem_zeropnt,0,0,
			m68_level2_irq, 1
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_zeropt,

	/* video hardware */
	0x180, 0xe0, { 0, 0x180-1, 0, 0xe0-1 },
	unico16_gfxdecodeinfo,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	unico16_vh_start,
	0,
	unico16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM3812,	&unico16_ym3812_intf	},
		{	SOUND_OKIM6295, &unico16_m6295_intf		}
	},
};



/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

								Zero Point

(C) 1998 Unico

PCB Number: ZPM1001A
CPU: 68HC000P16
SND: AD-65, K-666
OSC: 14.31818MHz, 32.000MHz
RAM: 62256 x 5, 6116 x 8, 84256 x 2
DIPS: 2 x 8 position

Other Chips: 3 x Actel A1020B (square 84 pin socketed, Same video chip as Power Instinct and Blomby Car)
             MACH211 (square 44 pin socketed)

There is a small gun interface board (Number ZPT1001B) located near the 68000 which contains
another Actel A1020B chip, a 74HC14 TTL chip and a 4.9152MHz OSC.

ROMS:
ZERO1.BIN  \
ZERO2.BIN  / Main Program 4M Mask ROMs
ZERO3.BIN  -- Sound MX27C4000
ZEROMSK1.BIN -\
ZEROMSK2.BIN   \
ZEROMSK3.BIN    \
ZEROMSK4.BIN     \
ZEROMSK5.BIN      - GFX,16M Mask ROMs
ZEROMSK6.BIN     /
ZEROMSK7.BIN    /
ZEROMSK8.BIN  -/

***************************************************************************/

ROM_START( zeropnt )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "zero1.bin", 0x000000, 0x080000, 0x1e599509 )
	ROM_LOAD16_BYTE( "zero2.bin", 0x000001, 0x080000, 0x588aeef7 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_INVERT | ROMREGION_DISPOSE )	/* 16x16x8 Sprites */
	ROM_LOAD( "zeromsk1.bin", 0x000000, 0x200000, 0x1f2768a3 )
	ROM_LOAD( "zeromsk2.bin", 0x200000, 0x200000, 0xde34f33a )
	ROM_LOAD( "zeromsk3.bin", 0x400000, 0x200000, 0xd7a657f7 )
	ROM_LOAD( "zeromsk4.bin", 0x600000, 0x200000, 0x3aec2f8d )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_INVERT | ROMREGION_DISPOSE )	/* 16x16x8 Layers */
	ROM_LOAD( "zeromsk6.bin", 0x000000, 0x200000, 0xe1e53cf0 )
	ROM_LOAD( "zeromsk5.bin", 0x200000, 0x200000, 0x0d7d4850 )
	ROM_LOAD( "zeromsk7.bin", 0x400000, 0x200000, 0xbb178f32 )
	ROM_LOAD( "zeromsk8.bin", 0x600000, 0x200000, 0x672f02e5 )

	ROM_REGION( 0x80000 * 2, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "zero3.bin", 0x000000, 0x080000, 0xfd2384fa )
	ROM_RELOAD(            0x080000, 0x080000             )

ROM_END



/***************************************************************************

								Burglar X

by Unico

68000-16mhz , MACH210-15JC, 3 x A1020B
14.31818 Mhz, 32.000 Mhz

***************************************************************************/

ROM_START( burglarx )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "bx-rom2.pgm", 0x000000, 0x080000, 0xf81120c8 )
	ROM_LOAD16_BYTE( "bx-rom3.pgm", 0x000001, 0x080000, 0x080b4e82 )

	/* Notice the weird ROMs order? Pretty much bit scrambling */
	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_INVERT | ROMREGION_DISPOSE )	/* 16x16x8 Sprites */
	ROM_LOAD16_BYTE( "bx-rom4",  0x000000, 0x080000, 0xf74ce31f )
	ROM_LOAD16_BYTE( "bx-rom10", 0x000001, 0x080000, 0x6f56ca23 )
	ROM_LOAD16_BYTE( "bx-rom9",  0x100000, 0x080000, 0x33f29d79 )
	ROM_LOAD16_BYTE( "bx-rom8",  0x100001, 0x080000, 0x24367092 )
	ROM_LOAD16_BYTE( "bx-rom7",  0x200000, 0x080000, 0xaff6bdea )
	ROM_LOAD16_BYTE( "bx-rom6",  0x200001, 0x080000, 0x246afed2 )
	ROM_LOAD16_BYTE( "bx-rom11", 0x300000, 0x080000, 0x898d176a )
	ROM_LOAD16_BYTE( "bx-rom5",  0x300001, 0x080000, 0xfdee1423 )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_INVERT | ROMREGION_DISPOSE )	/* 16x16x8 Layers */
	ROM_LOAD16_BYTE( "bx-rom14", 0x000000, 0x080000, 0x30413373 )
	ROM_LOAD16_BYTE( "bx-rom18", 0x000001, 0x080000, 0x8e7fc99f )
	ROM_LOAD16_BYTE( "bx-rom19", 0x100000, 0x080000, 0xd40eabcd )
	ROM_LOAD16_BYTE( "bx-rom15", 0x100001, 0x080000, 0x78833c75 )
	ROM_LOAD16_BYTE( "bx-rom17", 0x200000, 0x080000, 0xf169633f )
	ROM_LOAD16_BYTE( "bx-rom12", 0x200001, 0x080000, 0x71eb160f )
	ROM_LOAD16_BYTE( "bx-rom13", 0x300000, 0x080000, 0xda34bbb5 )
	ROM_LOAD16_BYTE( "bx-rom16", 0x300001, 0x080000, 0x55b28ef9 )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "bx-rom1.snd", 0x000000, 0x080000, 0x8ae67138 )	/* 2 x 40000 */

ROM_END



/***************************************************************************


								Game Drivers


***************************************************************************/

GAME( 1997, burglarx, 0, burglarx, burglarx, 0, ROT0_16BIT, "Unico Electronics", "Burglar X"  )
GAME( 1998, zeropnt,  0, zeropnt,  zeropnt,  0, ROT0_16BIT, "Unico Electronics", "Zero Point" )
