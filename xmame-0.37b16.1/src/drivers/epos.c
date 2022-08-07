/***************************************************************************

  Epos games

  driver by Zsolt Vasvari


  Notes:

  - To walk in IGMO, hold down button 2.
  - Super Glob seems like a later revision of The Glob, the most obvious
    difference being an updated service mode.
  - These games don't have cocktail mode.
  - The divisor 4 was derived using the timing loop used to split the screen
    in the middle.  This loop takes roughly 24200 cycles, giving
    2500 + (24200 - 2500) * 2 * 60 = 2754000 = 2.75Mhz for the CPU speed,
    assuming 60 fps and a 2500 cycle VBLANK period.  Since the schematics
    are available, so this should be easy to check.
  - I think theglob2 is earlier than theglob.  They only differ in one routine,
    but it appears to be a bug fix.  Also, theglob3 appears to be even older.

  To Do:

  - During the color test, Super Blob uses a busy loop to split the screen
    between the two palettes.  This effect is not emulated.  See $039c.
    The Glob has a different color test, not using the busy loop.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void epos_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
WRITE_HANDLER( epos_videoram_w );
WRITE_HANDLER( epos_port_1_w );
void epos_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, epos_videoram_w, &videoram, &videoram_size },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, watchdog_reset_w },
	{ 0x01, 0x01, epos_port_1_w },
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x06, 0x06, AY8910_control_port_0_w },
PORT_END


/* I think the upper two bits of port 1 are used as a simple form of protection,
   so that ROMs couldn't be simply swapped.  Each game checks these bits and halts
   the processor if an unexpected value is read. */

INPUT_PORTS_START( suprglob )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x26, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x22, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x24, "7" )
	PORT_DIPSETTING(    0x26, "8" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000 + Difficulty * 10000" )
	PORT_DIPSETTING(    0x08, "90000 + Difficulty * 10000" )
	PORT_DIPNAME( 0x50, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x50, "6" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* this has to be LO */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SPECIAL )   /* this has to be HI */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( igmo )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x22, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x02, "40000" )
	PORT_DIPSETTING(    0x20, "60000" )
	PORT_DIPSETTING(    0x22, "80000" )
	PORT_DIPNAME( 0x8c, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x84, "6" )
	PORT_DIPSETTING(    0x88, "7" )
	PORT_DIPSETTING(    0x8c, "8" )
	PORT_DIPNAME( 0x50, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x50, "6" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BITX(0x10, IP_ACTIVE_LOW,  IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_SPECIAL )	/* this has to be HI */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_SPECIAL )   /* this has to be HI */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


static struct AY8910interface ay8912_interface =
{
	1,	/* 1 chip */
	11000000/4,	/* 2.75 MHz */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct MachineDriver machine_driver_epos =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			11000000/4,		/* 2.75 MHz (see notes) */
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60, 2500,  /* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	272, 241, { 0, 271, 0, 235 },	/* an unusual resolution */
	0,
	32, 0,
	epos_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_bitmapped_vh_start,
	generic_bitmapped_vh_stop,
	epos_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8912_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( suprglob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "u10",			0x0000, 0x1000, 0xc0141324 )
	ROM_LOAD( "u9",				0x1000, 0x1000, 0x58be8128 )
	ROM_LOAD( "u8",				0x2000, 0x1000, 0x6d088c16 )
	ROM_LOAD( "u7",				0x3000, 0x1000, 0xb2768203 )
	ROM_LOAD( "u6",				0x4000, 0x1000, 0x976c8f46 )
	ROM_LOAD( "u5",				0x5000, 0x1000, 0x340f5290 )
	ROM_LOAD( "u4",				0x6000, 0x1000, 0x173bd589 )
	ROM_LOAD( "u11",			0x7000, 0x0800, 0xd45b740d )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, 0xf4f6ddc5 )
ROM_END

ROM_START( theglob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "globu10.bin",	0x0000, 0x1000, 0x08fdb495 )
	ROM_LOAD( "globu9.bin",		0x1000, 0x1000, 0x827cd56c )
	ROM_LOAD( "globu8.bin",		0x2000, 0x1000, 0xd1219966 )
	ROM_LOAD( "globu7.bin",		0x3000, 0x1000, 0xb1649da7 )
	ROM_LOAD( "globu6.bin",		0x4000, 0x1000, 0xb3457e67 )
	ROM_LOAD( "globu5.bin",		0x5000, 0x1000, 0x89d582cd )
	ROM_LOAD( "globu4.bin",		0x6000, 0x1000, 0x7ee9fdeb )
	ROM_LOAD( "globu11.bin",	0x7000, 0x0800, 0x9e05dee3 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, 0xf4f6ddc5 )
ROM_END

ROM_START( theglob2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "611293.u10",		0x0000, 0x1000, 0x870af7ce )
	ROM_LOAD( "611293.u9",		0x1000, 0x1000, 0xa3679782 )
	ROM_LOAD( "611293.u8",		0x2000, 0x1000, 0x67499d1a )
	ROM_LOAD( "611293.u7",		0x3000, 0x1000, 0x55e53aac )
	ROM_LOAD( "611293.u6",		0x4000, 0x1000, 0xc64ad743 )
	ROM_LOAD( "611293.u5",		0x5000, 0x1000, 0xf93c3203 )
	ROM_LOAD( "611293.u4",		0x6000, 0x1000, 0xceea0018 )
	ROM_LOAD( "611293.u11",		0x7000, 0x0800, 0x6ac83f9b )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, 0xf4f6ddc5 )
ROM_END

ROM_START( theglob3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "theglob3.u10",	0x0000, 0x1000, 0x969cfaf6 )
	ROM_LOAD( "theglob3.u9",	0x1000, 0x1000, 0x00000000 )	/* bad, all 0xff's */
	ROM_LOAD( "theglob3.u8",	0x2000, 0x1000, 0x1c1ca5c8 )
	ROM_LOAD( "theglob3.u7",	0x3000, 0x1000, 0xa54b9d22 )
	ROM_LOAD( "theglob3.u6",	0x4000, 0x1000, 0x5a6f82a9 )
	ROM_LOAD( "theglob3.u5",	0x5000, 0x1000, 0x72f935db )
	ROM_LOAD( "theglob3.u4",	0x6000, 0x1000, 0x81db53ad )
	ROM_LOAD( "theglob3.u11",	0x7000, 0x0800, 0x0e2e6359 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, 0xf4f6ddc5 )
ROM_END

ROM_START( igmo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "igmo-u10.732",	0x0000, 0x1000, 0xa9f691a4 )
	ROM_LOAD( "igmo-u9.732",	0x1000, 0x1000, 0x3c133c97 )
	ROM_LOAD( "igmo-u8.732",	0x2000, 0x1000, 0x5692f8d8 )
	ROM_LOAD( "igmo-u7.732",	0x3000, 0x1000, 0x630ae2ed )
	ROM_LOAD( "igmo-u6.732",	0x4000, 0x1000, 0xd3f20e1d )
	ROM_LOAD( "igmo-u5.732",	0x5000, 0x1000, 0xe26bb391 )
	ROM_LOAD( "igmo-u4.732",	0x6000, 0x1000, 0x762a4417 )
	ROM_LOAD( "igmo-u11.716",	0x7000, 0x0800, 0x8c675837 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.u66",		0x0000, 0x0020, 0x00000000 )	/* missing */
ROM_END



GAME ( 1983, suprglob, 0,        epos, suprglob, 0, ROT270, "Epos Corporation", "Super Glob" )
GAME ( 1983, theglob,  suprglob, epos, suprglob, 0, ROT270, "Epos Corporation", "The Glob" )
GAME ( 1983, theglob2, suprglob, epos, suprglob, 0, ROT270, "Epos Corporation", "The Glob (earlier)" )
GAMEX( 1983, theglob3, suprglob, epos, suprglob, 0, ROT270, "Epos Corporation", "The Glob (set 3)", GAME_NOT_WORKING )
GAMEX( 1984, igmo,     0,        epos, igmo,     0, ROT270, "Epos Corporation", "IGMO", GAME_WRONG_COLORS )
