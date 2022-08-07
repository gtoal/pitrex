/***************************************************************************

  Super Burger Time     (c) 1990 Data East Corporation (DE-0343)
  China Town            (c) 1991 Data East Corporation (DE-0343)

  Sound:  Ym2151, Oki adpcm - NOTE!  The sound program writes to the address
of a YM2203 and a 2nd Oki chip but the board does _not_ have them.  The sound
program is simply the 'generic' Data East sound program unmodified for this cut
down hardware (it doesn't write any good sound data btw, mostly zeros).

  Super Burgertime has a few bugs:

  Some sprites clip at the edges of the screen.
  Some burgers (from crushing an enemy) appear with wrong colour.
  Colour cycle on title screen doesn't work first time around.

  These are NOT driver bugs!  They all exist in the original game.

  Same hardware as Tumblepop, the two drivers can be joined at a later date.

  Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"

int  supbtime_vh_start(void);
void supbtime_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void chinatwn_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

WRITE16_HANDLER( supbtime_pf2_data_w );
WRITE16_HANDLER( supbtime_pf1_data_w );
WRITE16_HANDLER( supbtime_control_0_w );

extern data16_t *supbtime_pf2_data,*supbtime_pf1_data,*supbtime_pf1_row;

static READ16_HANDLER( supbtime_pf1_data_r ) { return supbtime_pf1_data[offset]; }
static READ16_HANDLER( supbtime_pf2_data_r ) { return supbtime_pf2_data[offset]; }

/******************************************************************************/

static READ16_HANDLER( supbtime_controls_r )
{
 	switch (offset<<1)
	{
		case 0: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));
		case 2: /* Dips */
			return (readinputport(3) + (readinputport(4) << 8));
		case 8: /* Credits */
			return readinputport(2);
		case 10: /* ?  Not used for anything */
		case 12:
			return 0;
	}

	logerror("CPU #0 PC %06x: warning - read unmapped control address %06x\n",cpu_get_pc(),offset);
	return ~0;
}

static WRITE16_HANDLER( sound_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_cause_interrupt(1,H6280_INT_IRQ1);
}

/******************************************************************************/

static MEMORY_READ16_START( supbtime_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x120000, 0x1207ff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x180000, 0x18000f, supbtime_controls_r },
	{ 0x320000, 0x321fff, supbtime_pf1_data_r },
	{ 0x322000, 0x323fff, supbtime_pf2_data_r },
	{ 0x340000, 0x3401ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( supbtime_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x104000, 0x11ffff, MWA16_NOP }, /* Nothing there */
	{ 0x120000, 0x1207ff, MWA16_RAM, &spriteram16 },
	{ 0x120800, 0x13ffff, MWA16_NOP }, /* Nothing there */
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x18000a, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a0001, sound_w },

	{ 0x300000, 0x30000f, supbtime_control_0_w },
	{ 0x320000, 0x321fff, supbtime_pf1_data_w, &supbtime_pf1_data },
	{ 0x322000, 0x323fff, supbtime_pf2_data_w, &supbtime_pf2_data },

	{ 0x340000, 0x3401ff, MWA16_RAM, &supbtime_pf1_row },
	{ 0x340400, 0x3405ff, MWA16_NOP },/* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },/* Unused row scroll */
	{ 0x342400, 0x3425ff, MWA16_NOP },/* Unused col scroll */
MEMORY_END

static MEMORY_READ16_START( chinatwn_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x120000, 0x1207ff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x180000, 0x18000f, supbtime_controls_r },
	{ 0x1a0000, 0x1a3fff, MRA16_RAM },
	{ 0x320000, 0x321fff, supbtime_pf1_data_r },
	{ 0x322000, 0x323fff, supbtime_pf2_data_r },
	{ 0x340000, 0x3401ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( chinatwn_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x100001, sound_w },
	{ 0x120000, 0x1207ff, MWA16_RAM, &spriteram16 },
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x18000a, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a3fff, MWA16_RAM },

	{ 0x300000, 0x30000f, supbtime_control_0_w },
	{ 0x320000, 0x321fff, supbtime_pf1_data_w, &supbtime_pf1_data },
	{ 0x322000, 0x323fff, supbtime_pf2_data_w, &supbtime_pf2_data },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM2151_w )
{
	switch (offset) {
	case 0:
		YM2151_register_port_0_w(0,data);
		break;
	case 1:
		YM2151_data_port_0_w(0,data);
		break;
	}
}

/* Physical memory map (21 bits) */
static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, MRA_NOP }, /* This board only has 1 oki chip */
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP }, /* YM2203 - this board doesn't have one */
	{ 0x110000, 0x110001, YM2151_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, MWA_NOP },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( supbtime )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 - inverted with respect to other Deco games */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
  	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( chinatwn )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* Used? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )  /* Used? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 - inverted with respect to other Deco games */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, "Time" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x80, "2000" )
	PORT_DIPSETTING(    0xc0, "2500" )
	PORT_DIPSETTING(    0x40, "3000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
  	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 0x40000*8+8, 0x40000*8, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tile_layout =
{
	16,16,
	4096,
	4,
	{ 0x40000*8+8, 0x40000*8, 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout sprite_layout =
{
	16,16,
	4096*2,
	4,
	{ 8, 0, 0x80000*8+8, 0x80000*8 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   256, 16 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tile_layout,  512, 16 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &sprite_layout,  0, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* May not be correct, there is another crystal near the ym2151 */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static struct MachineDriver machine_driver_supbtime =
{
	/* basic machine hardware */
	{
	 	{
			CPU_M68000,
			14000000,
			supbtime_readmem,supbtime_writemem,0,0,
			m68_level6_irq,1
		},
		{
			CPU_H6280 | CPU_AUDIO_CPU, /* Custom chip 45 */
			32220000/8, /* Audio section crystal is 32.220 MHz */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529,
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	supbtime_vh_start,
	0,
	supbtime_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
  	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_chinatwn =
{
	/* basic machine hardware */
	{
	 	{
			CPU_M68000,
			14000000,
			chinatwn_readmem,chinatwn_writemem,0,0,
			m68_level6_irq,1
		},
		{
			CPU_H6280 | CPU_AUDIO_CPU, /* Custom chip 45 */
			32220000/8, /* Audio section crystal is 32.220 MHz */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529,
	1,
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	supbtime_vh_start,
	0,
	chinatwn_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
  	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/******************************************************************************/

ROM_START( supbtime )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gk03", 0x00000, 0x20000, 0xaeaeed61 )
	ROM_LOAD16_BYTE( "gk04", 0x00001, 0x20000, 0x2bc5a4eb )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "gc06.bin",    0x00000, 0x10000, 0xe0e6c0f4 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mae02.bin", 0x000000, 0x80000, 0xa715cca0 ) /* chars */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
  	ROM_LOAD( "mae00.bin", 0x000000, 0x80000, 0x30043094 ) /* sprites */
	ROM_LOAD( "mae01.bin", 0x080000, 0x80000, 0x434af3fb )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
  	ROM_LOAD( "gc05.bin",    0x00000, 0x20000, 0x2f2246ff )
ROM_END

ROM_START( supbtimj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gc03.bin", 0x00000, 0x20000, 0xb5621f6a )
	ROM_LOAD16_BYTE( "gc04.bin", 0x00001, 0x20000, 0x551b2a0c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "gc06.bin",    0x00000, 0x10000, 0xe0e6c0f4 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mae02.bin", 0x000000, 0x80000, 0xa715cca0 ) /* chars */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
  	ROM_LOAD( "mae00.bin", 0x000000, 0x80000, 0x30043094 ) /* sprites */
	ROM_LOAD( "mae01.bin", 0x080000, 0x80000, 0x434af3fb )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
  	ROM_LOAD( "gc05.bin",    0x00000, 0x20000, 0x2f2246ff )
ROM_END

ROM_START( chinatwn )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gv_00-.f11", 0x00000, 0x20000, 0x2ea7ea5d )
	ROM_LOAD16_BYTE( "gv_01-.f13", 0x00001, 0x20000, 0xbcab03c7 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "gv_02-.f16",    0x00000, 0x10000, 0x95151d84 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mak-02.h2", 0x000000, 0x80000, 0x745b2c50 ) /* chars */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
  	ROM_LOAD( "mak-00.a2", 0x000000, 0x80000, 0x18e8cc1b ) /* sprites */
	ROM_LOAD( "mak-01.a4", 0x080000, 0x80000, 0xd88ebda8 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
  	ROM_LOAD( "gv_03-.j14",    0x00000, 0x20000, 0x948faf92 )
ROM_END

/******************************************************************************/

GAME( 1990, supbtime, 0,        supbtime, supbtime, 0, ROT0, "Data East Corporation", "Super Burger Time (World)" )
GAME( 1990, supbtimj, supbtime, supbtime, supbtime, 0, ROT0, "Data East Corporation", "Super Burger Time (Japan)" )
GAME( 1991, chinatwn, 0,        chinatwn, chinatwn, 0, ROT0, "Data East Corporation", "China Town (Japan)" )
