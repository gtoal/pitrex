/***************************************************************************

Ninja Gaiden memory map (preliminary)

000000-03ffff ROM
060000-063fff RAM
070000-070fff Video RAM (text layer)
072000-075fff VRAM (backgrounds)
076000-077fff Sprite RAM
078000-079fff Palette RAM

07a100-07a1ff Unknown

memory mapped ports:

read:
07a001    IN0
07a002    IN2
07a003    IN1
07a004    DWSB
07a005    DSWA
see the input_ports definition below for details on the input bits

write:
07a104-07a105 text layer Y scroll
07a10c-07a10d text layer X scroll
07a204-07a205 front layer Y scroll
07a20c-07a20d front layer X scroll
07a304-07a305 back layer Y scroll
07a30c-07a30d back layer Xscroll

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"

extern data16_t *gaiden_videoram,*gaiden_videoram2,*gaiden_videoram3;

void gaiden_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

WRITE16_HANDLER( gaiden_videoram_w );
WRITE16_HANDLER( gaiden_videoram2_w );
READ16_HANDLER( gaiden_videoram2_r );
WRITE16_HANDLER( gaiden_videoram3_w );
READ16_HANDLER( gaiden_videoram3_r );

WRITE16_HANDLER( gaiden_txscrollx_w );
WRITE16_HANDLER( gaiden_txscrolly_w );
WRITE16_HANDLER( gaiden_fgscrollx_w );
WRITE16_HANDLER( gaiden_fgscrolly_w );
WRITE16_HANDLER( gaiden_bgscrollx_w );
WRITE16_HANDLER( gaiden_bgscrolly_w );

int gaiden_vh_start(void);
void gaiden_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static WRITE16_HANDLER( gaiden_sound_command_w )
{
	if (ACCESSING_LSB) soundlatch_w(0,data & 0xff);	/* Ninja Gaiden */
	if (ACCESSING_MSB) soundlatch_w(0,data >> 8);	/* Tecmo Knight */
	cpu_cause_interrupt(1,Z80_NMI_INT);
}



/* Tecmo Knight has a simple protection. It writes codes to 0x07a804, and reads */
/* the answer from 0x07a007. The returned values contain the address of a */
/* function to jump to. */

static int prot;

static WRITE16_HANDLER( tknight_protection_w )
{
	if (ACCESSING_MSB)
	{
		static int jumpcode;
		static int jumppoints[] =
		{
			0x0c0c,0x0cac,0x0d42,0x0da2,0x0eea,0x112e,0x1300,0x13fa,
			0x159a,0x1630,0x109a,0x1700,0x1750,0x1806,0x18d6,0x1a44,
			0x1b52
		};

		data >>= 8;

/*logerror("PC %06x: prot = %02x\n",cpu_get_pc(),data); */

		switch (data & 0xf0)
		{
			case 0x00:	/* init */
				prot = 0x00;
				break;
			case 0x10:	/* high 4 bits of jump code */
				jumpcode = (data & 0x0f) << 4;
				prot = 0x10;
				break;
			case 0x20:	/* low 4 bits of jump code */
				jumpcode |= data & 0x0f;
				if (jumpcode > 16)
				{
logerror("unknown jumpcode %02x\n",jumpcode);
					jumpcode = 0;
				}
				prot = 0x20;
				break;
			case 0x30:	/* ask for bits 12-15 of function address */
				prot = 0x40 | ((jumppoints[jumpcode] >> 12) & 0x0f);
				break;
			case 0x40:	/* ask for bits 8-11 of function address */
				prot = 0x50 | ((jumppoints[jumpcode] >> 8) & 0x0f);
				break;
			case 0x50:	/* ask for bits 4-7 of function address */
				prot = 0x60 | ((jumppoints[jumpcode] >> 4) & 0x0f);
				break;
			case 0x60:	/* ask for bits 0-3 of function address */
				prot = 0x70 | ((jumppoints[jumpcode] >> 0) & 0x0f);
				break;
		}
	}
}

static READ16_HANDLER( tknight_protection_r )
{
/*logerror("PC %06x: read prot %02x\n",cpu_get_pc(),prot); */
	return prot;
}



static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x060000, 0x063fff, MRA16_RAM },
	{ 0x070000, 0x070fff, MRA16_RAM },
	{ 0x072000, 0x073fff, gaiden_videoram2_r },
	{ 0x074000, 0x075fff, gaiden_videoram3_r },
	{ 0x076000, 0x077fff, MRA16_RAM },
	{ 0x078000, 0x0787ff, MRA16_RAM },
	{ 0x078800, 0x079fff, MRA16_NOP },   /* extra portion of palette RAM, not really used */
	{ 0x07a000, 0x07a001, input_port_0_word_r },
	{ 0x07a002, 0x07a003, input_port_1_word_r },
	{ 0x07a004, 0x07a005, input_port_2_word_r },
	{ 0x07a006, 0x07a007, tknight_protection_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x060000, 0x063fff, MWA16_RAM },
	{ 0x070000, 0x070fff, gaiden_videoram_w, &gaiden_videoram },
	{ 0x072000, 0x073fff, gaiden_videoram2_w, &gaiden_videoram2 },
	{ 0x074000, 0x075fff, gaiden_videoram3_w, &gaiden_videoram3 },
	{ 0x076000, 0x077fff, MWA16_RAM, &spriteram16 },
	{ 0x078000, 0x0787ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x078800, 0x079fff, MWA16_NOP },   /* extra portion of palette RAM, not really used */
	{ 0x07a104, 0x07a105, gaiden_txscrolly_w },
	{ 0x07a10c, 0x07a10d, gaiden_txscrollx_w },
	{ 0x07a204, 0x07a205, gaiden_fgscrolly_w },
	{ 0x07a20c, 0x07a20d, gaiden_fgscrollx_w },
	{ 0x07a304, 0x07a305, gaiden_bgscrolly_w },
	{ 0x07a30c, 0x07a30d, gaiden_bgscrollx_w },
	{ 0x07a800, 0x07a801, MWA16_NOP },
	{ 0x07a802, 0x07a803, gaiden_sound_command_w },
	{ 0x07a804, 0x07a805, tknight_protection_w },
	{ 0x07a806, 0x07a807, MWA16_NOP },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, OKIM6295_status_0_r },
	{ 0xfc00, 0xfc00, MRA_NOP },	/* ?? */
	{ 0xfc20, 0xfc20, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, OKIM6295_data_0_w },
	{ 0xf810, 0xf810, YM2203_control_port_0_w },
	{ 0xf811, 0xf811, YM2203_write_port_0_w },
	{ 0xf820, 0xf820, YM2203_control_port_1_w },
	{ 0xf821, 0xf821, YM2203_write_port_1_w },
	{ 0xfc00, 0xfc00, MWA_NOP },	/* ?? */
MEMORY_END



INPUT_PORTS_START( gaiden )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* PLAYER */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, "Energy" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x1000, "4" )
	PORT_DIPSETTING(      0x2000, "5" )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0xc000, "2" )
	PORT_DIPSETTING(      0x4000, "3" )
	PORT_DIPSETTING(      0x8000, "4" )
INPUT_PORTS_END

INPUT_PORTS_START( tknight )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* PLAYER */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x8000, "1" )
	PORT_DIPSETTING(      0xc000, "2" )
	PORT_DIPSETTING(      0x4000, "3" )
/*	PORT_DIPSETTING(      0x0000, "2" ) */
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	8,8,	/* tile size */
	RGN_FRAC(1,1),	/* number of tiles */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* offset to next tile */
};

static struct GfxLayout tile2layout =
{
	16,16,	/* tile size */
	RGN_FRAC(1,1),	/* number of tiles */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
	  32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4,
	  32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4,},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32},
	128*8	/* offset to next tile */
};

static struct GfxLayout spritelayout =
{
	8,8,	/* sprites size */
	RGN_FRAC(1,2),	/* number of sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0,4,RGN_FRAC(1,2),4+RGN_FRAC(1,2),8,12,8+RGN_FRAC(1,2),12+RGN_FRAC(1,2) },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* offset to next sprite */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,        256, 16 },	/* tiles 8x8 */
	{ REGION_GFX2, 0, &tile2layout,       768, 16 },	/* tiles 16x16 */
	{ REGION_GFX3, 0, &tile2layout,       512, 16 },	/* tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout,        0, 16 },	/* sprites 8x8 */
	{ -1 } /* end of array */
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz ? (hand tuned) */
	{ YM2203_VOL(60,15), YM2203_VOL(60,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 7576 },			/* 7576Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 20 }
};


static struct MachineDriver machine_driver_gaiden =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz */
			readmem,writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* NMIs are triggered by the main CPU */
								/* IRQs are triggered by the YM2203 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 30*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	gaiden_vh_start,
	0,
	gaiden_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
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

ROM_START( gaiden )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "gaiden.1",     0x00000, 0x20000, 0xe037ff7c )
	ROM_LOAD16_BYTE( "gaiden.2",     0x00001, 0x20000, 0x454f7314 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "gaiden.3",     0x0000, 0x10000, 0x75fd3e6a )   /* Audio CPU is a Z80  */

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gaiden.5",     0x000000, 0x10000, 0x8d4035f7 )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "14.bin",       0x000000, 0x20000, 0x1ecfddaa )
	ROM_LOAD( "15.bin",       0x020000, 0x20000, 0x1291a696 )
	ROM_LOAD( "16.bin",       0x040000, 0x20000, 0x140b47ca )
	ROM_LOAD( "17.bin",       0x060000, 0x20000, 0x7638cccb )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "18.bin",       0x000000, 0x20000, 0x3fadafd6 )
	ROM_LOAD( "19.bin",       0x020000, 0x20000, 0xddae9d5b )
	ROM_LOAD( "20.bin",       0x040000, 0x20000, 0x08cf7a93 )
	ROM_LOAD( "21.bin",       0x060000, 0x20000, 0x1ac892f5 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "gaiden.6",     0x000000, 0x20000, 0xe7ccdf9f )	/* sprites A1 */
	ROM_LOAD( "gaiden.8",     0x020000, 0x20000, 0x7ef7f880 )	/* sprites B1 */
	ROM_LOAD( "gaiden.10",    0x040000, 0x20000, 0xa6451dec )	/* sprites C1 */
	ROM_LOAD( "gaiden.12",    0x060000, 0x20000, 0x90f1e13a )	/* sprites D1 */
	ROM_LOAD( "gaiden.7",     0x080000, 0x20000, 0x016bec95 )	/* sprites A2 */
	ROM_LOAD( "gaiden.9",     0x0a0000, 0x20000, 0x6e9b7fd3 )	/* sprites B2 */
	ROM_LOAD( "gaiden.11",    0x0c0000, 0x20000, 0x7fbfdf5e )	/* sprites C2 */
	ROM_LOAD( "gaiden.13",    0x0e0000, 0x20000, 0x7d9f5c5e )	/* sprites D2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "gaiden.4",     0x0000, 0x20000, 0xb0e0faf9 ) /* samples */
ROM_END

ROM_START( shadoww )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "shadoww.1",    0x00000, 0x20000, 0xfefba387 )
	ROM_LOAD16_BYTE( "shadoww.2",    0x00001, 0x20000, 0x9b9d6b18 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "gaiden.3",     0x0000, 0x10000, 0x75fd3e6a )   /* Audio CPU is a Z80  */

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gaiden.5",     0x000000, 0x10000, 0x8d4035f7 )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "14.bin",       0x000000, 0x20000, 0x1ecfddaa )
	ROM_LOAD( "15.bin",       0x020000, 0x20000, 0x1291a696 )
	ROM_LOAD( "16.bin",       0x040000, 0x20000, 0x140b47ca )
	ROM_LOAD( "17.bin",       0x060000, 0x20000, 0x7638cccb )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "18.bin",       0x000000, 0x20000, 0x3fadafd6 )
	ROM_LOAD( "19.bin",       0x020000, 0x20000, 0xddae9d5b )
	ROM_LOAD( "20.bin",       0x040000, 0x20000, 0x08cf7a93 )
	ROM_LOAD( "21.bin",       0x060000, 0x20000, 0x1ac892f5 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "gaiden.6",     0x000000, 0x20000, 0xe7ccdf9f )	/* sprites A1 */
	ROM_LOAD( "gaiden.8",     0x020000, 0x20000, 0x7ef7f880 )	/* sprites B1 */
	ROM_LOAD( "gaiden.10",    0x040000, 0x20000, 0xa6451dec )	/* sprites C1 */
	ROM_LOAD( "shadoww.12a",  0x060000, 0x10000, 0x9bb07731 )	/* sprites D1 */
	ROM_LOAD( "shadoww.12b",  0x070000, 0x10000, 0xa4a950a2 )	/* sprites D1 */
	ROM_LOAD( "gaiden.7",     0x080000, 0x20000, 0x016bec95 )	/* sprites A2 */
	ROM_LOAD( "gaiden.9",     0x0a0000, 0x20000, 0x6e9b7fd3 )	/* sprites B2 */
	ROM_LOAD( "gaiden.11",    0x0c0000, 0x20000, 0x7fbfdf5e )	/* sprites C2 */
	ROM_LOAD( "shadoww.13a",  0x0e0000, 0x10000, 0x996d2fa5 )	/* sprites D2 */
	ROM_LOAD( "shadoww.13b",  0x0f0000, 0x10000, 0xb8df8a34 )	/* sprites D2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "gaiden.4",     0x0000, 0x20000, 0xb0e0faf9 ) /* samples */
ROM_END

ROM_START( ryukendn )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "ryukendn.1",  0x00000, 0x20000, 0x6203a5e2 )
	ROM_LOAD16_BYTE( "ryukendn.2",  0x00001, 0x20000, 0x9e99f522 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ryukendn.3",   0x0000, 0x10000, 0x6b686b69 )   /* Audio CPU is a Z80  */

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ryukendn.5",   0x000000, 0x10000, 0x765e7baa )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "14.bin",       0x000000, 0x20000, 0x1ecfddaa )
	ROM_LOAD( "15.bin",       0x020000, 0x20000, 0x1291a696 )
	ROM_LOAD( "16.bin",       0x040000, 0x20000, 0x140b47ca )
	ROM_LOAD( "17.bin",       0x060000, 0x20000, 0x7638cccb )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "18.bin",       0x000000, 0x20000, 0x3fadafd6 )
	ROM_LOAD( "19.bin",       0x020000, 0x20000, 0xddae9d5b )
	ROM_LOAD( "20.bin",       0x040000, 0x20000, 0x08cf7a93 )
	ROM_LOAD( "21.bin",       0x060000, 0x20000, 0x1ac892f5 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "gaiden.6",     0x000000, 0x20000, 0xe7ccdf9f )	/* sprites A1 */
	ROM_LOAD( "gaiden.8",     0x020000, 0x20000, 0x7ef7f880 )	/* sprites B1 */
	ROM_LOAD( "gaiden.10",    0x040000, 0x20000, 0xa6451dec )	/* sprites C1 */
	ROM_LOAD( "shadoww.12a",  0x060000, 0x10000, 0x9bb07731 )	/* sprites D1 */
	ROM_LOAD( "ryukendn.12b", 0x070000, 0x10000, 0x1773628a )	/* sprites D1 */
	ROM_LOAD( "gaiden.7",     0x080000, 0x20000, 0x016bec95 )	/* sprites A2 */
	ROM_LOAD( "ryukendn.9a",  0x0a0000, 0x10000, 0xc821e200 )	/* sprites B2 */
	ROM_LOAD( "ryukendn.9b",  0x0b0000, 0x10000, 0x6a6233b3 )	/* sprites B2 */
	ROM_LOAD( "gaiden.11",    0x0c0000, 0x20000, 0x7fbfdf5e )	/* sprites C2 */
	ROM_LOAD( "shadoww.13a",  0x0e0000, 0x10000, 0x996d2fa5 )	/* sprites D2 */
	ROM_LOAD( "ryukendn.13b", 0x0f0000, 0x10000, 0x1f43c507 )	/* sprites D2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "gaiden.4",     0x0000, 0x20000, 0xb0e0faf9 ) /* samples */
ROM_END

ROM_START( tknight )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "tkni1.bin",    0x00000, 0x20000, 0x9121daa8 )
	ROM_LOAD16_BYTE( "tkni2.bin",    0x00001, 0x20000, 0x6669cd87 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "tkni3.bin",    0x0000, 0x10000, 0x15623ec7 )   /* Audio CPU is a Z80  */

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tkni5.bin",    0x000000, 0x10000, 0x5ed15896 )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tkni7.bin",    0x000000, 0x80000, 0x4b4d4286 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tkni6.bin",    0x000000, 0x80000, 0xf68fafb1 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tkni9.bin",    0x000000, 0x80000, 0xd22f4239 )	/* sprites */
	ROM_LOAD( "tkni8.bin",    0x080000, 0x80000, 0x4931b184 )	/* sprites */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "tkni4.bin",    0x0000, 0x20000, 0xa7a1dbcf ) /* samples */
ROM_END

ROM_START( wildfang )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k for 68000 code */
	ROM_LOAD16_BYTE( "1.3st",    0x00000, 0x20000, 0xab876c9b )
	ROM_LOAD16_BYTE( "2.5st",    0x00001, 0x20000, 0x1dc74b3b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "tkni3.bin",    0x0000, 0x10000, 0x15623ec7 )   /* Audio CPU is a Z80  */

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tkni5.bin",    0x000000, 0x10000, 0x5ed15896 )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "14.3a",        0x000000, 0x20000, 0x0d20c10c )
	ROM_LOAD( "15.3b",        0x020000, 0x20000, 0x3f40a6b4 )
	ROM_LOAD( "16.1a",        0x040000, 0x20000, 0x0f31639e )
	ROM_LOAD( "17.1b",        0x060000, 0x20000, 0xf32c158e )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tkni6.bin",    0x000000, 0x80000, 0xf68fafb1 )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tkni9.bin",    0x000000, 0x80000, 0xd22f4239 )	/* sprites */
	ROM_LOAD( "tkni8.bin",    0x080000, 0x80000, 0x4931b184 )	/* sprites */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "tkni4.bin",    0x0000, 0x20000, 0xa7a1dbcf ) /* samples */
ROM_END



GAMEX( 1988, gaiden,   0,       gaiden, gaiden,  0, ROT0, "Tecmo", "Ninja Gaiden (World)", GAME_NO_COCKTAIL )
GAMEX( 1988, shadoww,  gaiden,  gaiden, gaiden,  0, ROT0, "Tecmo", "Shadow Warriors (US)", GAME_NO_COCKTAIL )
GAMEX( 1989, ryukendn, gaiden,  gaiden, gaiden,  0, ROT0, "Tecmo", "Ninja Ryukenden (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1989, tknight,  0,       gaiden, tknight, 0, ROT0, "Tecmo", "Tecmo Knight", GAME_NO_COCKTAIL )
GAMEX( 1989, wildfang, tknight, gaiden, tknight, 0, ROT0, "Tecmo", "Wild Fang", GAME_NO_COCKTAIL )
