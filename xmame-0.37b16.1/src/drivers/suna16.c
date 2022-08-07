/***************************************************************************

							-=  SunA 16 Bit Games =-

					driver by	Luca Elia (l.elia@tin.it)


CPU:	68000   +  Z80 [Music]  +  Z80 x 2 [4 Bit PCM]
Sound:	YM2151  +  DAC x 4


---------------------------------------------------------------------------
Year + Game					By		Hardware
---------------------------------------------------------------------------
96	Back Street Soccer		SunA	68000 + Z80 x 3 + YM2151 + DAC x 4
96	Ultra Balloon			SunA	68000 + Z80 x 2 + YM2151 + DAC x 2
---------------------------------------------------------------------------


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables and functions defined in vidhrdw: */

WRITE16_HANDLER( suna16_flipscreen_w );

READ16_HANDLER ( suna16_paletteram16_r );
WRITE16_HANDLER( suna16_paletteram16_w );

int  suna16_vh_start(void);
void suna16_vh_stop(void);
void suna16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


								Main CPU


***************************************************************************/

WRITE16_HANDLER( suna16_soundlatch_w )
{
	if (ACCESSING_LSB)
	{
		if (Machine->sample_rate != 0)
			soundlatch_w( 0, data & 0xff );
	}
	if (data & ~0xff)	logerror("CPU#0 PC %06X - Sound latch unknown bits: %04X\n", cpu_get_pc(), data);
}


WRITE16_HANDLER( bssoccer_leds_w )
{
	if (ACCESSING_LSB)
	{
		set_led_status(0, data & 0x01);
		set_led_status(1, data & 0x02);
		set_led_status(2, data & 0x04);
		set_led_status(3, data & 0x08);
		coin_counter_w(0, data & 0x10);
	}
	if (data & ~0x1f)	logerror("CPU#0 PC %06X - Leds unknown bits: %04X\n", cpu_get_pc(), data);
}


WRITE16_HANDLER( uballoon_leds_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, data & 0x01);
		set_led_status(0, data & 0x02);
		set_led_status(1, data & 0x04);
	}
	if (data & ~0x07)	logerror("CPU#0 PC %06X - Leds unknown bits: %04X\n", cpu_get_pc(), data);
}


/***************************************************************************
							Back Street Soccer
***************************************************************************/

static MEMORY_READ16_START( bssoccer_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM 				},	/* ROM */
	{ 0x200000, 0x203fff, MRA16_RAM 				},	/* RAM */
	{ 0x400000, 0x4001ff, suna16_paletteram16_r	},	/* Banked Palette */
	{ 0x400200, 0x400fff, MRA16_RAM 				},	/* */
	{ 0x600000, 0x61ffff, MRA16_RAM 				},	/* Sprites */
	{ 0xa00000, 0xa00001, input_port_0_word_r		},	/* P1 (Inputs) */
	{ 0xa00002, 0xa00003, input_port_1_word_r		},	/* P2 */
	{ 0xa00004, 0xa00005, input_port_2_word_r		},	/* P3 */
	{ 0xa00006, 0xa00007, input_port_3_word_r		},	/* P4 */
	{ 0xa00008, 0xa00009, input_port_4_word_r		},	/* DSWs */
	{ 0xa0000a, 0xa0000b, input_port_5_word_r		},	/* Coins */
MEMORY_END

static MEMORY_WRITE16_START( bssoccer_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM 				},	/* ROM */
	{ 0x200000, 0x203fff, MWA16_RAM 				},	/* RAM */
	{ 0x400000, 0x4001ff, suna16_paletteram16_w, &paletteram16 },  /* Banked Palette */
	{ 0x400200, 0x400fff, MWA16_RAM 				},	/* */
	{ 0x600000, 0x61ffff, MWA16_RAM, &spriteram16	},	/* Sprites */
	{ 0xa00000, 0xa00001, suna16_soundlatch_w		},	/* To Sound CPU */
	{ 0xa00002, 0xa00003, suna16_flipscreen_w		},	/* Flip Screen */
	{ 0xa00004, 0xa00005, bssoccer_leds_w			},	/* Leds */
	{ 0xa00006, 0xa00007, MWA16_NOP					},	/* ? IRQ 1 Ack */
	{ 0xa00008, 0xa00009, MWA16_NOP					},	/* ? IRQ 2 Ack */
MEMORY_END


/***************************************************************************
								Ultra Balloon
***************************************************************************/

WRITE16_HANDLER( uballoon_spriteram16_w )
{
	COMBINE_DATA( &spriteram16[offset] );
}

static MEMORY_READ16_START( uballoon_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM 				},	/* ROM */
	{ 0x800000, 0x803fff, MRA16_RAM 				},	/* RAM */
	{ 0x200000, 0x2001ff, suna16_paletteram16_r	},	/* Banked Palette */
	{ 0x200200, 0x200fff, MRA16_RAM 				},	/* */
	{ 0x400000, 0x41ffff, MRA16_RAM 				},	/* Sprites */
	{ 0x600000, 0x600001, input_port_0_word_r		},	/* P1 + Coins(Inputs) */
	{ 0x600002, 0x600003, input_port_1_word_r		},	/* P2 + Coins */
	{ 0x600004, 0x600005, input_port_2_word_r		},	/* DSW 1 */
	{ 0x600006, 0x600007, input_port_3_word_r		},	/* DSW 2 */
	{ 0xa00000, 0xa0ffff, MRA16_NOP					},	/* Protection */
MEMORY_END

static MEMORY_WRITE16_START( uballoon_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM 				},	/* ROM */
	{ 0x800000, 0x803fff, MWA16_RAM 				},	/* RAM */
	{ 0x200000, 0x2001ff, suna16_paletteram16_w, &paletteram16 },  /* Banked Palette */
	{ 0x200200, 0x200fff, MWA16_RAM 				},	/* */
	{ 0x400000, 0x41ffff, MWA16_RAM, &spriteram16	},	/* Sprites */
	{ 0x5c0000, 0x5dffff, uballoon_spriteram16_w	},	/* Sprites (Mirror?) */
	{ 0x600000, 0x600001, suna16_soundlatch_w		},	/* To Sound CPU */
	{ 0x600004, 0x600005, suna16_flipscreen_w		},	/* Flip Screen */
	{ 0x600008, 0x600009, uballoon_leds_w			},	/* Leds */
	{ 0x60000c, 0x60000d, MWA16_NOP					},	/* ? IRQ 1 Ack */
	{ 0x600010, 0x600011, MWA16_NOP					},	/* ? IRQ 1 Ack */
	{ 0xa00000, 0xa0ffff, MWA16_NOP					},	/* Protection */
MEMORY_END



/***************************************************************************


									Z80 #1

		Plays the music (YM2151) and controls the 2 Z80s in charge
		of playing the PCM samples


***************************************************************************/

/***************************************************************************
							Back Street Soccer
***************************************************************************/

static MEMORY_READ_START( bssoccer_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM					},	/* ROM */
	{ 0xf000, 0xf7ff, MRA_RAM					},	/* RAM */
	{ 0xf801, 0xf801, YM2151_status_port_0_r	},	/* YM2151 */
	{ 0xfc00, 0xfc00, soundlatch_r				},	/* From Main CPU */
MEMORY_END

static MEMORY_WRITE_START( bssoccer_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},	/* ROM */
	{ 0xf000, 0xf7ff, MWA_RAM					},	/* RAM */
	{ 0xf800, 0xf800, YM2151_register_port_0_w	},	/* YM2151 */
	{ 0xf801, 0xf801, YM2151_data_port_0_w		},	/* */
	{ 0xfd00, 0xfd00, soundlatch2_w 			},	/* To PCM Z80 #1 */
	{ 0xfe00, 0xfe00, soundlatch3_w 			},	/* To PCM Z80 #2 */
MEMORY_END

/***************************************************************************
								Ultra Balloon
***************************************************************************/

static MEMORY_READ_START( uballoon_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM					},	/* ROM */
	{ 0xf000, 0xf7ff, MRA_RAM					},	/* RAM */
	{ 0xf801, 0xf801, YM2151_status_port_0_r	},	/* YM2151 */
	{ 0xfc00, 0xfc00, soundlatch_r				},	/* From Main CPU */
MEMORY_END

static MEMORY_WRITE_START( uballoon_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM					},	/* ROM */
	{ 0xf000, 0xf7ff, MWA_RAM					},	/* RAM */
	{ 0xf800, 0xf800, YM2151_register_port_0_w	},	/* YM2151 */
	{ 0xf801, 0xf801, YM2151_data_port_0_w		},	/* */
	{ 0xfc00, 0xfc00, soundlatch2_w				},	/* To PCM Z80 */
MEMORY_END



/***************************************************************************


								Z80 #2 & #3

		Dumb PCM samples players (e.g they don't even have RAM!)


***************************************************************************/

/***************************************************************************
							Back Street Soccer
***************************************************************************/

/* Bank Switching */

static WRITE_HANDLER( bssoccer_pcm_1_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	int bank = data & 7;
	if (bank & ~7)	logerror("CPU#2 PC %06X - ROM bank unknown bits: %02X\n", cpu_get_pc(), data);
	cpu_setbank(1, &RAM[bank * 0x10000 + 0x1000]);
}

static WRITE_HANDLER( bssoccer_pcm_2_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU4);
	int bank = data & 7;
	if (bank & ~7)	logerror("CPU#3 PC %06X - ROM bank unknown bits: %02X\n", cpu_get_pc(), data);
	cpu_setbank(2, &RAM[bank * 0x10000 + 0x1000]);
}



/* Memory maps: Yes, *no* RAM */

static MEMORY_READ_START( bssoccer_pcm_1_readmem )
	{ 0x0000, 0x0fff, MRA_ROM			},	/* ROM */
	{ 0x1000, 0xffff, MRA_BANK1 		},	/* Banked ROM */
MEMORY_END
static MEMORY_WRITE_START( bssoccer_pcm_1_writemem )
	{ 0x0000, 0xffff, MWA_ROM			},	/* ROM */
MEMORY_END


static MEMORY_READ_START( bssoccer_pcm_2_readmem )
	{ 0x0000, 0x0fff, MRA_ROM			},	/* ROM */
	{ 0x1000, 0xffff, MRA_BANK2 		},	/* Banked ROM */
MEMORY_END
static MEMORY_WRITE_START( bssoccer_pcm_2_writemem )
	{ 0x0000, 0xffff, MWA_ROM			},	/* ROM */
MEMORY_END



/* 2 DACs per CPU - 4 bits per sample */

static WRITE_HANDLER( bssoccer_DAC_1_w )
{
	DAC_data_w( 0 + (offset & 1), (data & 0xf) * 0x11 );
}

static WRITE_HANDLER( bssoccer_DAC_2_w )
{
	DAC_data_w( 2 + (offset & 1), (data & 0xf) * 0x11 );
}



static PORT_READ_START( bssoccer_pcm_1_readport )
	{ 0x00, 0x00, soundlatch2_r 				},	/* From The Sound Z80 */
PORT_END
static PORT_WRITE_START( bssoccer_pcm_1_writeport )
	{ 0x00, 0x01, bssoccer_DAC_1_w				},	/* 2 x DAC */
	{ 0x03, 0x03, bssoccer_pcm_1_bankswitch_w	},	/* Rom Bank */
PORT_END

static PORT_READ_START( bssoccer_pcm_2_readport )
	{ 0x00, 0x00, soundlatch3_r 				},	/* From The Sound Z80 */
PORT_END
static PORT_WRITE_START( bssoccer_pcm_2_writeport )
	{ 0x00, 0x01, bssoccer_DAC_2_w				},	/* 2 x DAC */
	{ 0x03, 0x03, bssoccer_pcm_2_bankswitch_w	},	/* Rom Bank */
PORT_END



/***************************************************************************
								Ultra Balloon
***************************************************************************/

/* Bank Switching */

static WRITE_HANDLER( uballoon_pcm_1_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	int bank = data & 1;
	if (bank & ~1)	logerror("CPU#2 PC %06X - ROM bank unknown bits: %02X\n", cpu_get_pc(), data);
	cpu_setbank(1, &RAM[bank * 0x10000 + 0x400]);
}

/* Memory maps: Yes, *no* RAM */

static MEMORY_READ_START( uballoon_pcm_1_readmem )
	{ 0x0000, 0x03ff, MRA_ROM			},	/* ROM */
	{ 0x0400, 0xffff, MRA_BANK1 		},	/* Banked ROM */
MEMORY_END
static MEMORY_WRITE_START( uballoon_pcm_1_writemem )
	{ 0x0000, 0xffff, MWA_ROM			},	/* ROM */
MEMORY_END


static PORT_READ_START( uballoon_pcm_1_readport )
	{ 0x00, 0x00, soundlatch2_r 				},	/* From The Sound Z80 */
PORT_END
static PORT_WRITE_START( uballoon_pcm_1_writeport )
	{ 0x00, 0x01, bssoccer_DAC_1_w				},	/* 2 x DAC */
	{ 0x03, 0x03, uballoon_pcm_1_bankswitch_w	},	/* Rom Bank */
PORT_END

/***************************************************************************


								Input Ports


***************************************************************************/

#define JOY(_n_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  |  IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN   |  IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   |  IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  |  IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1		  |  IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2		  |  IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3		  |  IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START##_n_						 )


/***************************************************************************
							Back Street Soccer
***************************************************************************/

INPUT_PORTS_START( bssoccer )

	PORT_START	/* IN0 - $a00001.b - Player 1 */
	JOY(1)

	PORT_START	/* IN1 - $a00003.b - Player 2 */
	JOY(2)

	PORT_START	/* IN2 - $a00005.b - Player 3 */
	JOY(3)

	PORT_START	/* IN3 - $a00007.b - Player 4 */
	JOY(4)

	PORT_START	/* IN4 - $a00008.w - DSW x 2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	  0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	  0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	  0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	  0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	  0x0010, "Easy"     )
	PORT_DIPSETTING(	  0x0018, "Normal"   )
	PORT_DIPSETTING(	  0x0008, "Hard"     )
	PORT_DIPSETTING(	  0x0000, "Hardest?" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, "Play Time P1" )
	PORT_DIPSETTING(	  0x0300, "1:30" )
	PORT_DIPSETTING(	  0x0200, "1:45" )
	PORT_DIPSETTING(	  0x0100, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Play Time P2" )
	PORT_DIPSETTING(	  0x0c00, "1:30" )
	PORT_DIPSETTING(	  0x0800, "1:45" )
	PORT_DIPSETTING(	  0x0400, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0x3000, 0x3000, "Play Time P3" )
	PORT_DIPSETTING(	  0x3000, "1:30" )
	PORT_DIPSETTING(	  0x2000, "1:45" )
	PORT_DIPSETTING(	  0x1000, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )
	PORT_DIPNAME( 0xc000, 0xc000, "Play Time P4" )
	PORT_DIPSETTING(	  0xc000, "1:30" )
	PORT_DIPSETTING(	  0x8000, "1:45" )
	PORT_DIPSETTING(	  0x4000, "2:00" )
	PORT_DIPSETTING(	  0x0000, "2:15" )

	PORT_START	/* IN5 - $a0000b.b - Coins */
	PORT_DIPNAME( 0x0001, 0x0001, "Copyright" )         /* these 4 are shown in test mode */
	PORT_DIPSETTING(	  0x0001, "Distributer Unico" )
	PORT_DIPSETTING(	  0x0000, "All Rights Reserved" )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* used! */
	PORT_DIPSETTING(	  0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW,  IPT_COIN3   )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW,  IPT_COIN4   )

INPUT_PORTS_END


/***************************************************************************
								Ultra Balloon
***************************************************************************/

INPUT_PORTS_START( uballoon )

	PORT_START	/* IN0 - $600000.w - Player 1 */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP     | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN   | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1         | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2         | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_COIN1    )

	PORT_START	/* IN1 - $600002.w - Player 2 */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP     | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN   | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1         | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2         | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_DIPNAME( 0x3000, 0x3000, "Copyright" )	/* Jumpers */
	PORT_DIPSETTING(	  0x3000, "Distributer Unico" )
	PORT_DIPSETTING(	  0x2000, "All Rights Reserved" )
/*	PORT_DIPSETTING(	  0x1000, "Distributer Unico" ) */
/*	PORT_DIPSETTING(	  0x0000, "All Rights Reserved" ) */
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_START	/* IN2 - $600005.b - DSW 1 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	  0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	  0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	  0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	  0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	  0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Lives ) )
	PORT_DIPSETTING(	  0x0010, "2" )
	PORT_DIPSETTING(	  0x0018, "3" )
	PORT_DIPSETTING(	  0x0008, "4" )
	PORT_DIPSETTING(	  0x0000, "5" )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	  0x0040, "Easy"    )
	PORT_DIPSETTING(	  0x0060, "Normal"  )
	PORT_DIPSETTING(	  0x0020, "Hard"    )
	PORT_DIPSETTING(	  0x0000, "Hardest" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN3 - $600007.b - DSW 2 */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	  0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	  0x0002, DEF_STR( Upright ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	  0x001c, "200K" )
	PORT_DIPSETTING(	  0x0010, "300K, 1000K" )
	PORT_DIPSETTING(	  0x0018, "400K" )
	PORT_DIPSETTING(	  0x000c, "500K, 1500K" )
	PORT_DIPSETTING(	  0x0008, "500K, 2000K" )
	PORT_DIPSETTING(	  0x0004, "500K, 3000K" )
	PORT_DIPSETTING(	  0x0014, "600K" )
	PORT_DIPSETTING(	  0x0000, "None" )
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown 1-5*" )
	PORT_DIPSETTING(	  0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6*" )
	PORT_DIPSETTING(	  0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	  0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************


								Graphics Layouts


***************************************************************************/

/* Tiles are 8x8x4 but the minimum sprite size is 2x2 tiles */

static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+4,	0,4 },
	{ 3,2,1,0, 11,10,9,8 },
	{ STEP8(0,16) },
	8*8*4/2
};

static struct GfxDecodeInfo suna16_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4, 0, 16*2 }, /* [0] Sprites */
	{ -1 }
};




/***************************************************************************


								Machine drivers


***************************************************************************/


/***************************************************************************
							Back Street Soccer
***************************************************************************/

static struct YM2151interface bssoccer_ym2151_interface =
{
	1,
	3579545,	/* ? */
	{ YM3012_VOL(20,MIXER_PAN_LEFT, 20,MIXER_PAN_RIGHT) },
	{ 0 },		/* irq handler */
	{ 0 }		/* port write handler */
};

static struct DACinterface bssoccer_dac_interface =
{
	4,
	{	MIXER(40,MIXER_PAN_LEFT), MIXER(40,MIXER_PAN_RIGHT),
		MIXER(40,MIXER_PAN_LEFT), MIXER(40,MIXER_PAN_RIGHT)	}
};

int bssoccer_interrupt(void)
{
	switch (cpu_getiloops())
	{
		case 0: 	return 1;
		case 1: 	return 2;
		default:	return ignore_interrupt();
	}
}

static struct MachineDriver machine_driver_bssoccer =
{
	{
		{
			CPU_M68000,
			8000000,	/* ? */
			bssoccer_readmem, bssoccer_writemem,0,0,
			bssoccer_interrupt, 2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,	/* Z80B */
			3579545,	/* ? */
			bssoccer_sound_readmem,  bssoccer_sound_writemem,
			0,0,
			ignore_interrupt, 1 	/* No interrupts! */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,	/* Z80B */
			5000000,	/* ? */
			bssoccer_pcm_1_readmem,  bssoccer_pcm_1_writemem,
			bssoccer_pcm_1_readport, bssoccer_pcm_1_writeport,
			ignore_interrupt, 1 	/* No interrupts! */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,	/* Z80B */
			5000000,	/* ? */
			bssoccer_pcm_2_readmem,  bssoccer_pcm_2_writemem,
			bssoccer_pcm_2_readport, bssoccer_pcm_2_writeport,
			ignore_interrupt, 1 	/* No interrupts! */
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	100,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1 },
	suna16_gfxdecodeinfo,
	512, 512,		/* 2 banks of 256 colors */
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	suna16_vh_start,
	suna16_vh_stop,
	suna16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM2151,	&bssoccer_ym2151_interface	},
		{	SOUND_DAC,		&bssoccer_dac_interface		},
	}
};



/***************************************************************************
								Ultra Balloon
***************************************************************************/

static struct YM2151interface uballoon_ym2151_interface =
{
	1,
	3579545,	/* ? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 },		/* irq handler */
	{ 0 }		/* port write handler */
};

static struct DACinterface uballoon_dac_interface =
{
	2,
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) }
};

static struct MachineDriver machine_driver_uballoon =
{
	{
		{
			CPU_M68000,
			8000000,
			uballoon_readmem, uballoon_writemem,0,0,
			m68_level1_irq, 1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* ? */
			uballoon_sound_readmem,  uballoon_sound_writemem,
			0,0,
			ignore_interrupt, 1 	/* No interrupts! */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			5000000,	/* ? */
			uballoon_pcm_1_readmem,  uballoon_pcm_1_writemem,
			uballoon_pcm_1_readport, uballoon_pcm_1_writeport,
			ignore_interrupt, 1 	/* No interrupts! */
		},

		/* 2nd PCM Z80 missing */

	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	100,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0+16, 256-16-1 },
	suna16_gfxdecodeinfo,
	512, 512,		/* 2 banks of 256 colors */
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	suna16_vh_start,
	suna16_vh_stop,
	suna16_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{	SOUND_YM2151,	&uballoon_ym2151_interface	},
		{	SOUND_DAC,		&uballoon_dac_interface		},
	}
};

/***************************************************************************


								ROMs Loading


***************************************************************************/


/***************************************************************************

							[ Back Street Soccer ]

  68000-10	32MHz
			14.318MHz
  01   02					 12
  03   04					Z80B
  6264 6264 	  YM2151
				  6116
				   11	   13
  62256 		  Z80B	  Z80B
  62256
  62256   05 06 				 SW2
		  07 08 				 SW1
		  09 10 		 6116-45
									 6116-45
						 6116-45	 6116-45

***************************************************************************/

ROM_START( bssoccer )

	ROM_REGION( 0x200000, REGION_CPU1, 0 ) 	/* 68000 Code */
	ROM_LOAD16_BYTE( "02", 0x000000, 0x080000, 0x32871005 )
	ROM_LOAD16_BYTE( "01", 0x000001, 0x080000, 0xace00db6 )
	ROM_LOAD16_BYTE( "04", 0x100000, 0x080000, 0x25ee404d )
	ROM_LOAD16_BYTE( "03", 0x100001, 0x080000, 0x1a131014 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) 	/* Z80 #1 - Music */
	ROM_LOAD( "11", 0x000000, 0x010000, 0xdf7ae9bc ) /* 1xxxxxxxxxxxxxxx = 0xFF */

	ROM_REGION( 0x080000, REGION_CPU3, 0 ) 	/* Z80 #2 - PCM */
	ROM_LOAD( "13", 0x000000, 0x080000, 0x2b273dca )

	ROM_REGION( 0x080000, REGION_CPU4, 0 ) 	/* Z80 #3 - PCM */
	ROM_LOAD( "12", 0x000000, 0x080000, 0x6b73b87b )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* Sprites */
	ROM_LOAD( "05", 0x000000, 0x080000, 0xa5245bd4 )
	ROM_LOAD( "07", 0x080000, 0x080000, 0xfdb765c2 )
	ROM_LOAD( "09", 0x100000, 0x080000, 0x0e82277f )
	ROM_LOAD( "06", 0x180000, 0x080000, 0xd42ce84b )
	ROM_LOAD( "08", 0x200000, 0x080000, 0x96cd2136 )
	ROM_LOAD( "10", 0x280000, 0x080000, 0x1ca94d21 )

ROM_END



/***************************************************************************

							[ Ultra Ballon ]

the gameplay on this game a like bubble bobble in many ways,it uses a
68k@8mhz as the main cpu,2 z80's and a ym2151,the names of the rom files
are just my guess.

prg1.rom      27c040
prg2.rom      27c040
gfx1.rom	  27c040
gfx2.rom	  27c040
gfx3.rom	  27c040
gfx4.rom	  27c040
audio1.rom	  27c512
audio2.rom	  27c010

***************************************************************************/

ROM_START( uballoon )

	ROM_REGION( 0x100000, REGION_CPU1, 0 ) 	/* 68000 Code */
	ROM_LOAD16_BYTE( "prg2.rom", 0x000000, 0x080000, 0x72ab80ea )
	ROM_LOAD16_BYTE( "prg1.rom", 0x000001, 0x080000, 0x27a04f55 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) 	/* Z80 #1 - Music */
	ROM_LOAD( "audio1.rom", 0x000000, 0x010000, 0xc771f2b4 )

	ROM_REGION( 0x020000, REGION_CPU3, 0 ) 	/* Z80 #2 - PCM */
	ROM_LOAD( "audio2.rom", 0x000000, 0x020000, 0xc7f75347 )

	/* There's no Z80 #3 - PCM */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )	/* Sprites */
	ROM_LOAD( "gfx1.rom", 0x000000, 0x080000, 0xfd2ec297 )
	ROM_LOAD( "gfx2.rom", 0x080000, 0x080000, 0x6307aa60 )
	ROM_LOAD( "gfx3.rom", 0x100000, 0x080000, 0x718f3150 )
	ROM_LOAD( "gfx4.rom", 0x180000, 0x080000, 0xaf7e057e )

ROM_END


void init_uballoon(void)
{
	data16_t *RAM = (data16_t *) memory_region(REGION_CPU1);

	/* Patch out the protection checks */
	RAM[0x0113c/2] = 0x4e71;	/* bne $646 */
	RAM[0x0113e/2] = 0x4e71;	/* "" */
	RAM[0x01784/2] = 0x600c;	/* beq $1792 */
	RAM[0x018e2/2] = 0x600c;	/* beq $18f0 */
	RAM[0x03c54/2] = 0x600C;	/* beq $3c62 */
	RAM[0x126a0/2] = 0x4e71;	/* bne $1267a (ROM test) */
}



/***************************************************************************


								Games Drivers


***************************************************************************/

GAME( 1996, bssoccer, 0, bssoccer, bssoccer, 0,        ROT0, "SunA", "Back Street Soccer" )
GAME( 1996, uballoon, 0, uballoon, uballoon, uballoon, ROT0, "SunA", "Ultra Balloon" )
