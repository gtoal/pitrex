/***************************************************************************

Frogger memory map (preliminary)

0000-3fff ROM
8000-87ff RAM
a800-abff Video RAM
b000-b0ff Object RAM
b000-b03f screen attributes
b040-b05f sprites
b060-b0ff unused?

read:
8800	  Watchdog Reset
e000	  IN0
e002	  IN1
e004	  IN2

*
 * IN0 (all bits are inverted)
 * bit 7 : COIN 1
 * bit 6 : COIN 2
 * bit 5 : LEFT player 1
 * bit 4 : RIGHT player 1
 * bit 3 : SHOOT 1 player 1
 * bit 2 : CREDIT
 * bit 1 : SHOOT 2 player 1
 * bit 0 : UP player 2 (TABLE only)
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : START 1
 * bit 6 : START 2
 * bit 5 : LEFT player 2 (TABLE only)
 * bit 4 : RIGHT player 2 (TABLE only)
 * bit 3 : SHOOT 1 player 2 (TABLE only)
 * bit 2 : SHOOT 2 player 2 (TABLE only)
 * bit 1 :\ nr of lives
 * bit 0 :/ 00 = 3	01 = 5	10 = 7	11 = 256
*
 * IN2 (all bits are inverted)
 * bit 7 : unused
 * bit 6 : DOWN player 1
 * bit 5 : unused
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :\ coins per play
 * bit 1 :/
 * bit 0 : DOWN player 2 (TABLE only)
 *

write:
b808	  interrupt enable
b80c	  screen horizontal flip
b810	  screen vertical flip
b818	  coin counter 1
b81c	  coin counter 2
d000	  To AY-3-8910 port A (commands for the second Z80)
d002	  trigger interrupt on sound CPU


SOUND BOARD:
0000-17ff ROM
4000-43ff RAM

I/0 ports:
read:
40		8910 #1  read

write
40		8910 #1  write
80		8910 #1  control

interrupts:
interrupt mode 1 triggered by the main CPU

***************************************************************************/

#include "driver.h"



extern unsigned char *galaxian_videoram;
extern unsigned char *galaxian_spriteram;
extern unsigned char *galaxian_attributesram;
extern size_t galaxian_spriteram_size;

extern struct GfxDecodeInfo galaxian_gfxdecodeinfo[];

void frogger_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

void scramble_init_machine(void);

void init_frogger(void);
void init_froggers(void);

int frogger_vh_start(void);
int froggrmc_vh_start(void);

WRITE_HANDLER( galaxian_flip_screen_x_w );
WRITE_HANDLER( galaxian_flip_screen_y_w );
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE_HANDLER( frogger_filter_w );

READ_HANDLER( frogger_portB_r );
WRITE_HANDLER( froggrmc_sh_irqtrigger_w );

READ_HANDLER(frogger_ppi8255_0_r);
READ_HANDLER(frogger_ppi8255_1_r);
WRITE_HANDLER(frogger_ppi8255_0_w);
WRITE_HANDLER(frogger_ppi8255_1_w);


static WRITE_HANDLER( frogger_coin_counter_0_w )
{
	coin_counter_w (0, data);
}

static WRITE_HANDLER( frogger_coin_counter_1_w )
{
	coin_counter_w (1, data);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, watchdog_reset_r },
	{ 0xa800, 0xabff, MRA_RAM },
	{ 0xb000, 0xb0ff, MRA_RAM },
	{ 0xd000, 0xd007, frogger_ppi8255_1_r },
	{ 0xe000, 0xe007, frogger_ppi8255_0_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa800, 0xabff, MWA_RAM, &galaxian_videoram },
	{ 0xb000, 0xb03f, MWA_RAM, &galaxian_attributesram },
	{ 0xb040, 0xb05f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0xb060, 0xb0ff, MWA_RAM },
	{ 0xb808, 0xb808, interrupt_enable_w },
	{ 0xb80c, 0xb80c, galaxian_flip_screen_y_w },
	{ 0xb810, 0xb810, galaxian_flip_screen_x_w },
	{ 0xb818, 0xb818, frogger_coin_counter_0_w },
	{ 0xb81c, 0xb81c, frogger_coin_counter_1_w },
	{ 0xd000, 0xd007, frogger_ppi8255_1_w },
	{ 0xe000, 0xe007, frogger_ppi8255_0_w },
MEMORY_END


static MEMORY_READ_START( froggrmc_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9800, 0x98ff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( froggrmc_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, MWA_RAM, &galaxian_videoram },
	{ 0x9800, 0x983f, MWA_RAM, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x9860, 0x98ff, MWA_RAM },
	{ 0xa800, 0xa800, soundlatch_w },
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb001, 0xb001, froggrmc_sh_irqtrigger_w },
	{ 0xb006, 0xb006, galaxian_flip_screen_x_w },
	{ 0xb007, 0xb007, galaxian_flip_screen_y_w },
MEMORY_END



MEMORY_READ_START( frogger_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( frogger_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
    { 0x6000, 0x6fff, frogger_filter_w },
MEMORY_END



PORT_READ_START( frogger_sound_readport )
	{ 0x40, 0x40, AY8910_read_port_0_r },
PORT_END

PORT_WRITE_START( frogger_sound_writeport )
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ 0x80, 0x80, AY8910_control_port_0_w },
PORT_END



INPUT_PORTS_START( frogger )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x02, "7" )
	PORT_BITX( 0,		0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot2 - unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, "A 2/1 B 2/1 C 2/1" )
	PORT_DIPSETTING(	0x04, "A 2/1 B 1/3 C 2/1" )
	PORT_DIPSETTING(	0x00, "A 1/1 B 1/1 C 1/1" )
	PORT_DIPSETTING(	0x06, "A 1/1 B 1/6 C 1/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( froggrmc )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0xc0, "3" )
	PORT_DIPSETTING(	0x80, "5" )
	PORT_DIPSETTING(	0x40, "7" )
	PORT_BITX( 0,		0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, "A 2/1 B 2/1 C 2/1" )
	PORT_DIPSETTING(	0x04, "A 2/1 B 1/3 C 2/1" )
	PORT_DIPSETTING(	0x06, "A 1/1 B 1/1 C 1/1" )
	PORT_DIPSETTING(	0x00, "A 1/1 B 1/6 C 1/1" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



struct AY8910interface frogger_ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 MHz */
	{ MIXERG(80,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ soundlatch_r },
	{ frogger_portB_r },
	{ 0 },
	{ 0 }
};


static struct MachineDriver machine_driver_frogger =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6, /* 3.072 MHz */
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8, /* 1.78975 MHz */
			frogger_sound_readmem,frogger_sound_writemem,frogger_sound_readport,frogger_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	frogger_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	frogger_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&frogger_ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_froggrmc =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6, /* 3.072 MHz */
			froggrmc_readmem,froggrmc_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8, /* 1.78975 MHz */
			frogger_sound_readmem,frogger_sound_writemem,frogger_sound_readport,frogger_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	frogger_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	froggrmc_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&frogger_ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( frogger )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "frogger.26",   0x0000, 0x1000, 0x597696d6 )
	ROM_LOAD( "frogger.27",   0x1000, 0x1000, 0xb6e6fcc3 )
	ROM_LOAD( "frsm3.7",      0x2000, 0x1000, 0xaca22ae0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, 0xe8ab0256 )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, 0x7380a48f )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, 0x31d7eb27 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, 0x05f7d883 )
	ROM_LOAD( "frogger.606",  0x0800, 0x0800, 0xf524ee30 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, 0x413703bf )
ROM_END

ROM_START( frogseg1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "frogger.26",   0x0000, 0x1000, 0x597696d6 )
	ROM_LOAD( "frogger.27",   0x1000, 0x1000, 0xb6e6fcc3 )
	ROM_LOAD( "frogger.34",   0x2000, 0x1000, 0xed866bab )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, 0xe8ab0256 )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, 0x7380a48f )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, 0x31d7eb27 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, 0x05f7d883 )
	ROM_LOAD( "frogger.606",  0x0800, 0x0800, 0xf524ee30 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, 0x413703bf )
ROM_END

ROM_START( frogseg2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "frogger.ic5",  0x0000, 0x1000, 0xefab0c79 )
	ROM_LOAD( "frogger.ic6",  0x1000, 0x1000, 0xaeca9c13 )
	ROM_LOAD( "frogger.ic7",  0x2000, 0x1000, 0xdd251066 )
	ROM_LOAD( "frogger.ic8",  0x3000, 0x1000, 0xbf293a02 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, 0xe8ab0256 )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, 0x7380a48f )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, 0x31d7eb27 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, 0x05f7d883 )
	ROM_LOAD( "frogger.606",  0x0800, 0x0800, 0xf524ee30 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, 0x413703bf )
ROM_END

ROM_START( froggrmc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr-1031.15",  0x0000, 0x1000, 0x4b7c8d11 )
	ROM_LOAD( "epr-1032.16",  0x1000, 0x1000, 0xac00b9d9 )
	ROM_LOAD( "epr-1033.33",  0x2000, 0x1000, 0xbc1d6fbc )
	ROM_LOAD( "epr-1034.34",  0x3000, 0x1000, 0x9efe7399 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "epr-1082.42",  0x0000, 0x1000, 0x802843c2 )
	ROM_LOAD( "epr-1035.43",  0x1000, 0x0800, 0x14e74148 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "frogger.607",  0x0000, 0x0800, 0x05f7d883 )
	ROM_LOAD( "epr-1036.1k",  0x0800, 0x0800, 0x658745f8 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91.6l",     0x0000, 0x0020, 0x413703bf )
ROM_END



GAME( 1981, frogger,  0,	   frogger,  frogger,  frogger,  ROT90, "Konami", "Frogger" )
GAME( 1981, frogseg1, frogger, frogger,  frogger,  frogger,  ROT90, "[Konami] (Sega license)", "Frogger (Sega set 1)" )
GAME( 1981, frogseg2, frogger, frogger,  frogger,  frogger,  ROT90, "[Konami] (Sega license)", "Frogger (Sega set 2)" )
GAME( 1981, froggrmc, frogger, froggrmc, froggrmc, froggers, ROT90, "bootleg?", "Frogger (modified Moon Cresta hardware)" )
