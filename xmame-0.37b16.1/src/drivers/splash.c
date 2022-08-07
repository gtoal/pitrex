/***************************************************************************

Splash! (c) 1992 Gaelco

Driver by Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/m68000.h"


extern data16_t *splash_vregs;
extern data16_t *splash_videoram;
extern data16_t *splash_spriteram;
extern data16_t *splash_pixelram;

/* from vidhrdw/gaelco.c */
READ16_HANDLER( splash_vram_r );
READ16_HANDLER( splash_pixelram_r );
WRITE16_HANDLER( splash_vram_w );
WRITE16_HANDLER( splash_pixelram_w );
int splash_vh_start( void );
void splash_vh_screenrefresh( struct osd_bitmap *bitmap,int full_refresh );


static WRITE16_HANDLER( splash_sh_irqtrigger_w )
{
	if (ACCESSING_LSB){
		soundlatch_w(0,data & 0xff);
		cpu_cause_interrupt(1,Z80_IRQ_INT);
	}
}

static MEMORY_READ16_START( splash_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },			/* ROM */
	{ 0x800000, 0x83ffff, splash_pixelram_r },	/* Pixel Layer */
	{ 0x840000, 0x840001, input_port_0_word_r },/* DIPSW #1 */
	{ 0x840002, 0x840003, input_port_1_word_r },/* DIPSW #2 */
	{ 0x840004, 0x840005, input_port_2_word_r },/* INPUT #1 */
	{ 0x840006, 0x840007, input_port_3_word_r },/* INPUT #2 */
	{ 0x880000, 0x8817ff, splash_vram_r },		/* Video RAM */
	{ 0x881800, 0x881803, MRA16_RAM },			/* Scroll registers */
	{ 0x881804, 0x881fff, MRA16_RAM },			/* Work RAM */
	{ 0x8c0000, 0x8c0fff, MRA16_RAM },			/* Palette */
	{ 0x900000, 0x900fff, MRA16_RAM },			/* Sprite RAM */
	{ 0xffc000, 0xffffff, MRA16_RAM },			/* Work RAM */
MEMORY_END

WRITE16_HANDLER( splash_coin_w )
{
	if (ACCESSING_MSB){
		switch ((offset >> 3)){
			case 0x00:	/* Coin Lockouts */
			case 0x01:
				coin_lockout_w( (offset >> 3) & 0x01, (data & 0x0400) >> 8);
				break;
			case 0x02:	/* Coin Counters */
			case 0x03:
				coin_counter_w( (offset >> 3) & 0x01, (data & 0x0100) >> 8);
				break;
		}
	}
}

static MEMORY_WRITE16_START( splash_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },										/* ROM */
	{ 0x800000, 0x83ffff, splash_pixelram_w, &splash_pixelram },			/* Pixel Layer */
	{ 0x84000e, 0x84000f, splash_sh_irqtrigger_w },							/* Sound command */
	{ 0x84000a, 0x84003b, splash_coin_w },									/* Coin Counters + Coin Lockout */
	{ 0x880000, 0x8817ff, splash_vram_w, &splash_videoram },				/* Video RAM */
	{ 0x881800, 0x881803, MWA16_RAM, &splash_vregs },						/* Scroll registers */
	{ 0x881804, 0x881fff, MWA16_RAM },										/* Work RAM */
	{ 0x8c0000, 0x8c0fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },/* Palette is xRRRRxGGGGxBBBBx */
	{ 0x900000, 0x900fff, MWA16_RAM, &splash_spriteram },					/* Sprite RAM */
	{ 0xffc000, 0xffffff, MWA16_RAM },										/* Work RAM */
MEMORY_END


static MEMORY_READ_START( splash_readmem_sound )
	{ 0x0000, 0xd7ff, MRA_ROM },					/* ROM */
	{ 0xe800, 0xe800, soundlatch_r },				/* Sound latch */
	{ 0xf000, 0xf000, YM3812_status_port_0_r },		/* YM3812 */
	{ 0xf800, 0xffff, MRA_RAM },					/* RAM */
MEMORY_END

static int adpcm_data;

static WRITE_HANDLER( splash_adpcm_data_w ){
	adpcm_data = data;
}

static void splash_msm5205_int(int data)
{
	MSM5205_data_w(0,adpcm_data >> 4);
	adpcm_data = (adpcm_data << 4) & 0xf0;
}


static MEMORY_WRITE_START( splash_writemem_sound )
	{ 0x0000, 0xd7ff, MWA_ROM },					/* ROM */
	{ 0xd800, 0xd800, splash_adpcm_data_w },		/* ADPCM data for the MSM5205 chip */
/*	{ 0xe000, 0xe000, MWA_NOP },					   ???    */
	{ 0xf000, 0xf000, YM3812_control_port_0_w },	/* YM3812 */
	{ 0xf001, 0xf001, YM3812_write_port_0_w },		/* YM3812 */
	{ 0xf800, 0xffff, MWA_RAM },					/* RAM */
MEMORY_END


INPUT_PORTS_START( splash )
	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1C/1C or Free Play (if Coin B too)" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1C/1C or Free Play (if Coin A too)" )

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	/* 	according to the manual, Lives = 0x00 is NOT used */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Girls" )
	PORT_DIPSETTING(    0x00, "Light" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Paint Mode" )
	PORT_DIPSETTING(    0x00, "Paint again" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* 1P INPUTS & COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* 2P INPUTS & STARTSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END


static struct GfxLayout tilelayout8 =
{
	8,8,									/* 8x8 tiles */
	0x20000/8,								/* number of tiles */
	4,										/* bitplanes */
	{ 0*0x20000*8, 1*0x20000*8, 2*0x20000*8, 3*0x20000*8 }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static struct GfxLayout tilelayout16 =
{
	16,16,									/* 16x16 tiles */
	0x20000/32,								/* number of tiles */
	4,										/* bitplanes */
	{ 0*0x20000*8, 1*0x20000*8, 2*0x20000*8, 3*0x20000*8 }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7, 16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tilelayout8 ,0,128 },
	{ REGION_GFX1, 0x000000, &tilelayout16,0,128 },
	{ -1 }
};

static struct YM3812interface splash_ym3812_interface =
{
	1,						/* 1 chip */
	3000000,				/* 3 MHz? */
	{ 40 },					/* volume */
	{ 0 }					/* IRQ handler */
};

static struct MSM5205interface splash_msm5205_interface =
{
	1,						/* 1 chip */
	384000,					/* 384KHz */
	{ splash_msm5205_int },	/* IRQ handler */
	{ MSM5205_S48_4B },		/* 8KHz */
	{ 80 }					/* volume */
};


static struct MachineDriver machine_driver_splash =
{
	{
		{
			CPU_M68000,
			24000000/2,			/* 12 MHz */
			splash_readmem,splash_writemem,0,0,
			m68_level6_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			30000000/8,			/* 3.75 MHz? */
			splash_readmem_sound, splash_writemem_sound,0,0,
			nmi_interrupt,64	/* needed for the msm5205 to play the samples */
		}
	},
	60,DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	64*8, 64*8, { 2*8, 49*8-1, 2*8, 32*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	splash_vh_start,
	0,
	splash_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&splash_ym3812_interface
		},
		{
			SOUND_MSM5205,
			&splash_msm5205_interface
	    }
	}
};


ROM_START( splash )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code + gfx */
	ROM_LOAD16_BYTE(	"4g",	0x000000, 0x020000, 0xb38fda40 )
	ROM_LOAD16_BYTE(	"4i",	0x000001, 0x020000, 0x02359c47 )
	ROM_LOAD16_BYTE(	"5g",	0x100000, 0x080000, 0xa4e8ed18 )
	ROM_LOAD16_BYTE(	"5i",	0x100001, 0x080000, 0x73e1154d )
	ROM_LOAD16_BYTE(	"6g",	0x200000, 0x080000, 0xffd56771 )
	ROM_LOAD16_BYTE(	"6i",	0x200001, 0x080000, 0x16e9170c )
	ROM_LOAD16_BYTE(	"8g",	0x300000, 0x080000, 0xdc3a3172 )
	ROM_LOAD16_BYTE(	"8i",	0x300001, 0x080000, 0x2e23e6c3 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )	/* Z80 code + sound data */
	ROM_LOAD( "5c",	0x00000, 0x10000, 0x0ed7ebc9 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "18i",	0x000000, 0x020000, 0x028a4a68 )
	ROM_LOAD( "15i",	0x020000, 0x020000, 0x2a8cb830 )
	ROM_LOAD( "16i",	0x040000, 0x020000, 0x21aeff2c )
	ROM_LOAD( "13i",	0x060000, 0x020000, 0xfebb9893 )
ROM_END


GAME( 1992, splash,   0, splash,   splash,   0, ROT0_16BIT, "Gaelco", "Splash!" )
