/***************************************************************************

Son Son memory map (preliminary)

driver by Mirko Buffoni


MAIN CPU:

0000-0fff RAM
1000-13ff Video RAM
1400-17ff Color RAM
2020-207f Sprites
4000-ffff ROM

read:
3002      IN0
3003      IN1
3004      IN2
3005      DSW0
3006      DSW1

write:
3000      horizontal scroll
3008      ? one of these two should be
3018      ? the watchdog reset
3010      command for the audio CPU
3019      trigger FIRQ on audio CPU


SOUND CPU:
0000-07ff RAM
e000-ffff ROM

read:
a000      command from the main CPU

write:
2000      8910 #1 control
2001      8910 #1 write
4000      8910 #2 control
4001      8910 #2 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"



extern unsigned char *sonson_scrollx;

void sonson_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void sonson_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



WRITE_HANDLER( sonson_sh_irqtrigger_w )
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,M6809_INT_FIRQ);
	}

	last = data;
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x17ff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_ROM },
	{ 0x3002, 0x3002, input_port_0_r },	/* IN0 */
	{ 0x3003, 0x3003, input_port_1_r },	/* IN1 */
	{ 0x3004, 0x3004, input_port_2_r },	/* IN2 */
	{ 0x3005, 0x3005, input_port_3_r },	/* DSW0 */
	{ 0x3006, 0x3006, input_port_4_r },	/* DSW1 */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
	{ 0x1400, 0x17ff, colorram_w, &colorram },
	{ 0x2020, 0x207f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3000, 0x3000, MWA_RAM, &sonson_scrollx },
{ 0x3008, 0x3008, MWA_NOP },
	{ 0x3010, 0x3010, soundlatch_w },
{ 0x3018, 0x3018, MWA_NOP },
	{ 0x3019, 0x3019, sonson_sh_irqtrigger_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_control_port_0_w },
	{ 0x2001, 0x2001, AY8910_write_port_0_w },
	{ 0x4000, 0x4000, AY8910_control_port_1_w },
	{ 0x4001, 0x4001, AY8910_write_port_1_w },
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( sonson )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage affects" )
	PORT_DIPSETTING(    0x10, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Coin_B ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* maybe flip screen */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "2 Players Game" )
	PORT_DIPSETTING(    0x04, "1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Credits" )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20000 80000 100000" )
	PORT_DIPSETTING(    0x00, "30000 90000 120000" )
	PORT_DIPSETTING(    0x18, "20000" )
	PORT_DIPSETTING(    0x10, "30000" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(1,2), RGN_FRAC(0,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 8*16+7, 8*16+6, 8*16+5, 8*16+4, 8*16+3, 8*16+2, 8*16+1, 8*16+0,
			7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 64 },
	{ REGION_GFX2, 0, &spritelayout, 64*4, 32 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	12000000/8,	/* 1.5 MHz ? */
	{ 30, 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver_sonson =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			12000000/6,	/* 2 MHz ??? */
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			12000000/6,	/* 2 MHz ??? */
			sound_readmem,sound_writemem,0,0,
			interrupt,4	/* FIRQs are triggered by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 1*8, 31*8-1 },
	gfxdecodeinfo,
	32,64*4+32*8,
	sonson_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	sonson_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sonson )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "ss.01e",       0x4000, 0x4000, 0xcd40cc54 )
	ROM_LOAD( "ss.02e",       0x8000, 0x4000, 0xc3476527 )
	ROM_LOAD( "ss.03e",       0xc000, 0x4000, 0x1fd0e729 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ss_6.c11",     0xe000, 0x2000, 0x1135c48a )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ss_7.b6",      0x00000, 0x2000, 0x990890b1 )	/* characters */
	ROM_LOAD( "ss_8.b5",      0x02000, 0x2000, 0x9388ff82 )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ss_9.m5",      0x00000, 0x2000, 0x8cb1cacf )	/* sprites */
	ROM_LOAD( "ss_10.m6",     0x02000, 0x2000, 0xf802815e )
	ROM_LOAD( "ss_11.m3",     0x04000, 0x2000, 0x4dbad88a )
	ROM_LOAD( "ss_12.m4",     0x06000, 0x2000, 0xaa05e687 )
	ROM_LOAD( "ss_13.m1",     0x08000, 0x2000, 0x66119bfa )
	ROM_LOAD( "ss_14.m2",     0x0a000, 0x2000, 0xe14ef54e )

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "ssb4.b2",      0x0000, 0x0020, 0xc8eaf234 )	/* red/green component */
	ROM_LOAD( "ssb5.b1",      0x0020, 0x0020, 0x0e434add )	/* blue component */
	ROM_LOAD( "ssb2.c4",      0x0040, 0x0100, 0xc53321c6 )	/* character lookup table */
	ROM_LOAD( "ssb3.h7",      0x0140, 0x0100, 0x7d2c324a )	/* sprite lookup table */
	ROM_LOAD( "ssb1.k11",     0x0240, 0x0100, 0xa04b0cfe )	/* unknown (not used) */
ROM_END

ROM_START( sonsonj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "ss_0.l9",      0x4000, 0x2000, 0x705c168f )
	ROM_LOAD( "ss_1.j9",      0x6000, 0x2000, 0x0f03b57d )
	ROM_LOAD( "ss_2.l8",      0x8000, 0x2000, 0xa243a15d )
	ROM_LOAD( "ss_3.j8",      0xa000, 0x2000, 0xcb64681a )
	ROM_LOAD( "ss_4.l7",      0xc000, 0x2000, 0x4c3e9441 )
	ROM_LOAD( "ss_5.j7",      0xe000, 0x2000, 0x847f660c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ss_6.c11",     0xe000, 0x2000, 0x1135c48a )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ss_7.b6",      0x00000, 0x2000, 0x990890b1 )	/* characters */
	ROM_LOAD( "ss_8.b5",      0x02000, 0x2000, 0x9388ff82 )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ss_9.m5",      0x00000, 0x2000, 0x8cb1cacf )	/* sprites */
	ROM_LOAD( "ss_10.m6",     0x02000, 0x2000, 0xf802815e )
	ROM_LOAD( "ss_11.m3",     0x04000, 0x2000, 0x4dbad88a )
	ROM_LOAD( "ss_12.m4",     0x06000, 0x2000, 0xaa05e687 )
	ROM_LOAD( "ss_13.m1",     0x08000, 0x2000, 0x66119bfa )
	ROM_LOAD( "ss_14.m2",     0x0a000, 0x2000, 0xe14ef54e )

	ROM_REGION( 0x0340, REGION_PROMS, 0 )
	ROM_LOAD( "ssb4.b2",      0x0000, 0x0020, 0xc8eaf234 )	/* red/green component */
	ROM_LOAD( "ssb5.b1",      0x0020, 0x0020, 0x0e434add )	/* blue component */
	ROM_LOAD( "ssb2.c4",      0x0040, 0x0100, 0xc53321c6 )	/* character lookup table */
	ROM_LOAD( "ssb3.h7",      0x0140, 0x0100, 0x7d2c324a )	/* sprite lookup table */
	ROM_LOAD( "ssb1.k11",     0x0240, 0x0100, 0xa04b0cfe )	/* unknown (not used) */
ROM_END


GAMEX( 1984, sonson,  0,      sonson, sonson, 0, ROT0, "Capcom", "Son Son", GAME_NO_COCKTAIL )
GAMEX( 1984, sonsonj, sonson, sonson, sonson, 0, ROT0, "Capcom", "Son Son (Japan)", GAME_NO_COCKTAIL )
