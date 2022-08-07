/***************************************************************************

  Sony ZN1/ZN2 - Arcade PSX Hardware
  ==================================
  Driver by smf

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/mips/mips.h"
#include "cpu/z80/z80.h"

/*
  Capcom qsound games

  Based on information from:
   The cps1/cps2 qsound driver,
   Miguel Angel Horna
   Amuse.

  The main board is made by sony, capcom included the qsound on the game
  board. None of these have a bios dumped yet so only the music can be
  played for now.

  The qsound hardware is different to cps2 as it uses an i/o port and
  nmi's instead of shared memory. The driver uses 8bit i/o addresses
  but the real ZN1 hardware may use 16bit i/o addresses as the code
  always accesses port 0xf100. The ZN2 code seems to vary the top
  eight bits of the address ( which may or may not be important ).
*/

static int qcode;
static int qcode_last;
static int queue_data;
static int queue_len;

WRITE_HANDLER( qsound_queue_w )
{
	if( cpu_getstatus( 1 ) != 0 )
	{
		queue_data = data;
		queue_len = 2;
	}
}

static int znqs_vh_start( void )
{
	return 0;
}

static void znqs_vh_stop( void )
{
}

static void znqs_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh )
{
	int refresh = full_refresh;

	if( queue_len == 0 )
	{
		if( keyboard_pressed_memory( KEYCODE_UP ) )
		{
			qcode=( qcode & 0xff00 ) | ( ( qcode + 0x0001 ) & 0xff );
		}
		if( keyboard_pressed_memory( KEYCODE_DOWN ) )
		{
			qcode=( qcode & 0xff00 ) | ( ( qcode - 0x0001 ) & 0xff );
		}
		if( keyboard_pressed_memory( KEYCODE_RIGHT ) )
		{
			qcode=( ( qcode + 0x0100 ) & 0xff00 ) | ( qcode & 0xff );
		}
		if( keyboard_pressed_memory( KEYCODE_LEFT ) )
		{
			qcode=( ( qcode - 0x0100 ) & 0xff00 ) | ( qcode & 0xff );
		}
		if( qcode != qcode_last )
		{
			qsound_queue_w( 0, qcode );
			qcode_last = qcode;
			refresh = 1;
		}
	}

	if( refresh )
	{
		struct DisplayText dt[ 4 ];
		char text1[ 256 ];
		char text2[ 256 ];
		char text3[ 256 ];

		strcpy( text1, Machine->gamedrv->description );
		if( strlen( text1 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text1[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		sprintf( text2, "QSOUND CODE=%02x/%02x", qcode >> 8, qcode & 0xff );
		if( strlen( text2 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text2[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		strcpy( text3, "SELECT WITH RIGHT&LEFT/UP&DN" );
		if( strlen( text3 ) > Machine->uiwidth / Machine->uifontwidth )
		{
			text3[ Machine->uiwidth / Machine->uifontwidth ] = 0;
		}
		dt[ 0 ].text = text1;
		dt[ 0 ].color = UI_COLOR_NORMAL;
		dt[ 0 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 0 ].text ) ) / 2;
		dt[ 0 ].y = Machine->uiheight - Machine->uifontheight * 5;
		dt[ 1 ].text = text2;
		dt[ 1 ].color = UI_COLOR_NORMAL;
		dt[ 1 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 1 ].text ) ) / 2;
		dt[ 1 ].y = Machine->uiheight - Machine->uifontheight * 3;
		dt[ 2 ].text = text3;
		dt[ 2 ].color = UI_COLOR_NORMAL;
		dt[ 2 ].x = ( Machine->uiwidth - Machine->uifontwidth * strlen( dt[ 2 ].text ) ) / 2;
		dt[ 2 ].y = Machine->uiheight - Machine->uifontheight * 1;
		dt[ 3 ].text = 0; /* terminate array */
		displaytext( Machine->scrbitmap, dt );
	}
}

static struct QSound_interface qsound_interface =
{
	QSOUND_CLOCK,
	REGION_SOUND1,
	{ 100,100 }
};

static WRITE_HANDLER( qsound_banksw_w )
{
	unsigned char *RAM = memory_region( REGION_CPU2 );
	if( ( data & 0xf0 ) != 0 )
	{
		logerror( "%08lx: qsound_banksw_w( %02x )\n", cpu_get_pc(), data & 0xff );
	}
	cpu_setbank( 1, &RAM[ 0x10000 + ( ( data & 0x0f ) * 0x4000 ) ] );
}

static MEMORY_READ_START( qsound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },	/* banked (contains music data) */
	{ 0xd007, 0xd007, qsound_status_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( qsound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd000, qsound_data_h_w },
	{ 0xd001, 0xd001, qsound_data_l_w },
	{ 0xd002, 0xd002, qsound_cmd_w },
	{ 0xd003, 0xd003, qsound_banksw_w },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( qsound_readport )
	{ 0x00, 0x00, soundlatch_r },
PORT_END

static struct GfxDecodeInfo znqs_gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static MEMORY_READ16_START( znqs_readmem )
	{ 0xbfc00000, 0xbfc7ffff, MRA16_BANK3 },	/* bios */
MEMORY_END

static MEMORY_WRITE16_START( znqs_writemem )
MEMORY_END


static int qsound_interrupt( void )
{
	if( queue_len == 2 )
	{
		soundlatch_w( 0, queue_data >> 8 );
		queue_len--;
		return nmi_interrupt();
	}
	else if( queue_len == 1 )
	{
		soundlatch_w( 0, queue_data & 0xff );
		queue_len--;
		return nmi_interrupt();
	}
	return interrupt();
}

static void znqs_init_machine( void )
{
	/* stop CPU1 as it doesn't do anything useful yet. */
	timer_suspendcpu( 0, 1, SUSPEND_REASON_DISABLE );
	/* but give it some memory so it can reset. */
	cpu_setbank( 3, memory_region( REGION_USER1 ) );

	if( strcmp( Machine->gamedrv->name, "sfex2" ) == 0 ||
		strcmp( Machine->gamedrv->name, "sfex2p" ) == 0 ||
		strcmp( Machine->gamedrv->name, "tgmj" ) == 0 )
	{
		qcode = 0x0400;
	}
	else if( strcmp( Machine->gamedrv->name, "kikaioh" ) == 0 )
	{
		qcode = 0x8000;
	}
	else
	{
		qcode = 0x0000;
	}
	qcode_last = -1;
	queue_len = 0;
	qsound_banksw_w( 0, 0 );
}

static struct MachineDriver machine_driver_znqs =
{
	/* basic machine hardware */
	{
		{
			CPU_PSXCPU,
			33000000, /* 33MHz ?? */
			znqs_readmem, znqs_writemem, 0, 0,
			ignore_interrupt, 1  /* ??? interrupts per frame */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			8000000,  /* 8MHz ?? */
			qsound_readmem, qsound_writemem, qsound_readport, 0,
			qsound_interrupt, 4 /* 4 interrupts per frame ?? */
		}
	},
	60, 0,
	1,
	znqs_init_machine,

	/* video hardware */
	0x30*8+32*2, 0x1c*8+32*3, { 32, 32+0x30*8-1, 32+16, 32+16+0x1c*8-1 },
	znqs_gfxdecodeinfo,
	4096,
	4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	znqs_vh_start,
	znqs_vh_stop,
	znqs_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{ { SOUND_QSOUND, &qsound_interface } }
};

INPUT_PORTS_START( zn )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE	)	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2  )

	PORT_START		/* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START		/* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

ROM_START( rvschool )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x2480000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "jst-04a", 0x0000000, 0x080000, 0x034b1011 )
	ROM_LOAD16_WORD( "jst-05m", 0x0080000, 0x400000, 0x723372b8 )
	ROM_LOAD16_WORD( "jst-06m", 0x0480000, 0x400000, 0x4248988e )
	ROM_LOAD16_WORD( "jst-07m", 0x0880000, 0x400000, 0xc84c5a16 )
	ROM_LOAD16_WORD( "jst-08m", 0x0c80000, 0x400000, 0x791b57f3 )
	ROM_LOAD16_WORD( "jst-09m", 0x1080000, 0x400000, 0x6df42048 )
	ROM_LOAD16_WORD( "jst-10m", 0x1480000, 0x400000, 0xd7e22769 )
	ROM_LOAD16_WORD( "jst-11m", 0x1880000, 0x400000, 0x0a033ac5 )
	ROM_LOAD16_WORD( "jst-12m", 0x1c80000, 0x400000, 0x43bd2ddd )
	ROM_LOAD16_WORD( "jst-13m", 0x2080000, 0x400000, 0x6b443235 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst-02",  0x00000, 0x08000, 0x7809e2c3 )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "jst-03",  0x28000, 0x20000, 0x860ff24d )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "jst-01m", 0x0000000, 0x400000, 0x9a7c98f9 )
ROM_END

ROM_START( jgakuen )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x2480000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "jst-04j", 0x0000000, 0x080000, 0x28b8000a )
	ROM_LOAD16_WORD( "jst-05m", 0x0080000, 0x400000, 0x723372b8 )
	ROM_LOAD16_WORD( "jst-06m", 0x0480000, 0x400000, 0x4248988e )
	ROM_LOAD16_WORD( "jst-07m", 0x0880000, 0x400000, 0xc84c5a16 )
	ROM_LOAD16_WORD( "jst-08m", 0x0c80000, 0x400000, 0x791b57f3 )
	ROM_LOAD16_WORD( "jst-09m", 0x1080000, 0x400000, 0x6df42048 )
	ROM_LOAD16_WORD( "jst-10m", 0x1480000, 0x400000, 0xd7e22769 )
	ROM_LOAD16_WORD( "jst-11m", 0x1880000, 0x400000, 0x0a033ac5 )
	ROM_LOAD16_WORD( "jst-12m", 0x1c80000, 0x400000, 0x43bd2ddd )
	ROM_LOAD16_WORD( "jst-13m", 0x2080000, 0x400000, 0x6b443235 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst-02",  0x00000, 0x08000, 0x7809e2c3 )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "jst-03",  0x28000, 0x20000, 0x860ff24d )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "jst-01m", 0x0000000, 0x400000, 0x9a7c98f9 )
ROM_END

ROM_START( kikaioh )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x3080000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "kioj-04.bin", 0x0000000, 0x080000, 0x3a2a3bc8 )
	ROM_LOAD16_WORD( "kio-05m.bin", 0x0080000, 0x800000, 0x98e9eb24 )
	ROM_LOAD16_WORD( "kio-06m.bin", 0x0880000, 0x800000, 0xbe8d7d73 )
	ROM_LOAD16_WORD( "kio-07m.bin", 0x1080000, 0x800000, 0xffd81f18 )
	ROM_LOAD16_WORD( "kio-08m.bin", 0x1880000, 0x800000, 0x17302226 )
	ROM_LOAD16_WORD( "kio-09m.bin", 0x2080000, 0x800000, 0xa34f2119 )
	ROM_LOAD16_WORD( "kio-10m.bin", 0x2880000, 0x800000, 0x7400037a )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio-02.bin",  0x00000, 0x08000, 0x174309b3 )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "kio-03.bin",  0x28000, 0x20000, 0x0b313ae5 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "kio-01m.bin", 0x0000000, 0x400000, 0x6dc5bd07 )
ROM_END

ROM_START( sfex )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sfe-04a", 0x0000000, 0x080000, 0x08247bd4 )
	ROM_LOAD16_WORD( "sfe-05m", 0x0080000, 0x400000, 0xeab781fe )
	ROM_LOAD16_WORD( "sfe-06m", 0x0480000, 0x400000, 0x999de60c )
	ROM_LOAD16_WORD( "sfe-07m", 0x0880000, 0x400000, 0x76117b0a )
	ROM_LOAD16_WORD( "sfe-08m", 0x0c80000, 0x400000, 0xa36bbec5 )
	ROM_LOAD16_WORD( "sfe-09m", 0x1080000, 0x400000, 0x62c424cc )
	ROM_LOAD16_WORD( "sfe-10m", 0x1480000, 0x400000, 0x83791a8b )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, 0x1908475c )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, 0x95c1e2e0 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, 0xf5afff0d )
ROM_END

ROM_START( sfexj )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sfe-04j", 0x0000000, 0x080000, 0xea100607 )
	ROM_LOAD16_WORD( "sfe-05m", 0x0080000, 0x400000, 0xeab781fe )
	ROM_LOAD16_WORD( "sfe-06m", 0x0480000, 0x400000, 0x999de60c )
	ROM_LOAD16_WORD( "sfe-07m", 0x0880000, 0x400000, 0x76117b0a )
	ROM_LOAD16_WORD( "sfe-08m", 0x0c80000, 0x400000, 0xa36bbec5 )
	ROM_LOAD16_WORD( "sfe-09m", 0x1080000, 0x400000, 0x62c424cc )
	ROM_LOAD16_WORD( "sfe-10m", 0x1480000, 0x400000, 0x83791a8b )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, 0x1908475c )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, 0x95c1e2e0 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, 0xf5afff0d )
ROM_END

ROM_START( sfexp )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sfp-04e", 0x0000000, 0x080000, 0x305e4ec0 )
	ROM_LOAD16_WORD( "sfp-05",  0x0080000, 0x400000, 0xac7dcc5e )
	ROM_LOAD16_WORD( "sfp-06",  0x0480000, 0x400000, 0x1d504758 )
	ROM_LOAD16_WORD( "sfp-07",  0x0880000, 0x400000, 0x0f585f30 )
	ROM_LOAD16_WORD( "sfp-08",  0x0c80000, 0x400000, 0x65eabc61 )
	ROM_LOAD16_WORD( "sfp-09",  0x1080000, 0x400000, 0x15f8b71e )
	ROM_LOAD16_WORD( "sfp-10",  0x1480000, 0x400000, 0xc1ecf652 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, 0x1908475c )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, 0x95c1e2e0 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, 0xf5afff0d )
ROM_END

ROM_START( sfexpj )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x1880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sfp-04j", 0x0000000, 0x080000, 0x18d043f5 )
	ROM_LOAD16_WORD( "sfp-05",  0x0080000, 0x400000, 0xac7dcc5e )
	ROM_LOAD16_WORD( "sfp-06",  0x0480000, 0x400000, 0x1d504758 )
	ROM_LOAD16_WORD( "sfp-07",  0x0880000, 0x400000, 0x0f585f30 )
	ROM_LOAD16_WORD( "sfp-08",  0x0c80000, 0x400000, 0x65eabc61 )
	ROM_LOAD16_WORD( "sfp-09",  0x1080000, 0x400000, 0x15f8b71e )
	ROM_LOAD16_WORD( "sfp-10",  0x1480000, 0x400000, 0xc1ecf652 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe-02",  0x00000, 0x08000, 0x1908475c )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sfe-03",  0x28000, 0x20000, 0x95c1e2e0 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "sfe-01m", 0x0000000, 0x400000, 0xf5afff0d )
ROM_END

ROM_START( sfex2 )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x2480000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "ex2j-04", 0x0000000, 0x080000, 0x5d603586 )
	ROM_LOAD16_WORD( "ex2-05m", 0x0080000, 0x800000, 0x78726b17 )
	ROM_LOAD16_WORD( "ex2-06m", 0x0880000, 0x800000, 0xbe1075ed )
	ROM_LOAD16_WORD( "ex2-07m", 0x1080000, 0x800000, 0x6496c6ed )
	ROM_LOAD16_WORD( "ex2-08m", 0x1880000, 0x800000, 0x3194132e )
	ROM_LOAD16_WORD( "ex2-09m", 0x2080000, 0x400000, 0x075ae585 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2-02",  0x00000, 0x08000, 0x9489875e )
	ROM_CONTINUE(		 0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, 0x14a5bb0e )
ROM_END

ROM_START( sfex2p )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x3080000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "sf2p-04", 0x0000000, 0x080000, 0xc6d0aea3 )
	ROM_LOAD16_WORD( "sf2p-05", 0x0080000, 0x800000, 0x4ee3110f )
	ROM_LOAD16_WORD( "sf2p-06", 0x0880000, 0x800000, 0x4cd53a45 )
	ROM_LOAD16_WORD( "sf2p-07", 0x1080000, 0x800000, 0x11207c2a )
	ROM_LOAD16_WORD( "sf2p-08", 0x1880000, 0x800000, 0x3560c2cc )
	ROM_LOAD16_WORD( "sf2p-09", 0x2080000, 0x800000, 0x344aa227 )
	ROM_LOAD16_WORD( "sf2p-10", 0x2880000, 0x800000, 0x2eef5931 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2p-02", 0x00000, 0x08000, 0x3705de5e )
	ROM_CONTINUE(		 0x10000, 0x18000 )
	ROM_LOAD( "sf2p-03", 0x28000, 0x20000, 0x6ae828f6 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ex2-01m", 0x0000000, 0x400000, 0x14a5bb0e )
ROM_END

ROM_START( shiryu2 )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x2c80000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "hr2j-04.bin", 0x0000000, 0x080000, 0x0824ee5f )
	ROM_LOAD16_WORD( "hr2-05m.bin", 0x0080000, 0x800000, 0x18716fe8 )
	ROM_LOAD16_WORD( "hr2-06m.bin", 0x0880000, 0x800000, 0x6f13b69c )
	ROM_LOAD16_WORD( "hr2-07m.bin", 0x1080000, 0x800000, 0x3925701b )
	ROM_LOAD16_WORD( "hr2-08m.bin", 0x1880000, 0x800000, 0xd844c0dc )
	ROM_LOAD16_WORD( "hr2-09m.bin", 0x2080000, 0x800000, 0xcdd43e6b )
	ROM_LOAD16_WORD( "hr2-10m.bin", 0x2880000, 0x400000, 0xd95b3f37 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2-02.bin",  0x00000, 0x08000, 0xacd8d385 )
	ROM_CONTINUE(		 0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "hr2-01m.bin", 0x0000000, 0x200000, 0x510a16d1 )
	ROM_RELOAD( 0x0200000, 0x200000 )
ROM_END

ROM_START( tgmj )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x0880000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "ate-04j", 0x0000000, 0x080000, 0xbb4bbb96 )
	ROM_LOAD16_WORD( "ate-05",  0x0080000, 0x400000, 0x50977f5a )
	ROM_LOAD16_WORD( "ate-06",  0x0480000, 0x400000, 0x05973f16 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ate-02",  0x00000, 0x08000, 0xf4f6e82f )
	ROM_CONTINUE(		 0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ate-01",  0x0000000, 0x400000, 0xa21c6521 )
ROM_END

ROM_START( ts2j )
	ROM_REGION16_LE( 0x0001000, REGION_USER1, 0 )

	ROM_REGION16_LE( 0x0e80000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "ts2j-04", 0x0000000, 0x080000, 0x4aba8c5e )
	ROM_LOAD16_WORD( "ts2-05",  0x0080000, 0x400000, 0x7f4228e2 )
	ROM_LOAD16_WORD( "ts2-06m", 0x0480000, 0x400000, 0xcd7e0a27 )
	ROM_LOAD16_WORD( "ts2-08m", 0x0880000, 0x400000, 0xb1f7f115 )
	ROM_LOAD16_WORD( "ts2-10",  0x0c80000, 0x200000, 0xad90679a )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2-02",  0x00000, 0x08000, 0x2f45c461 )
	ROM_CONTINUE(		 0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Q Sound Samples */
	ROM_LOAD( "ts2-01",  0x0000000, 0x400000, 0xd7a505e0 )
ROM_END


/*
  Other ZN1 games

  These have had their bios dumped, but they all fail the self test on
  what appears to be protection hardware & the screen turns red
  ( just enough of the psx chipset is emulated to display this ).

  There is an another romset for Gallop Racer 2 with link hw (z80-a, osc 32.0000MHz,
  upd72103agc & lh540202u x2) which has different versions of rom 119 & 120.
*/

#define VRAM_SIZE ( 1024L * 512L * 2L )

static unsigned short *m_p_vram;
static UINT32 m_p_gpu_buffer[ 16 ];
static unsigned char m_n_gpu_buffer_offset;

static void zn_vh_stop( void )
{
	if( m_p_vram != NULL )
	{
		free( m_p_vram );
	}
}

static int zn_vh_start( void )
{
	m_p_vram = malloc( VRAM_SIZE );
	if( m_p_vram == NULL )
	{
		zn_vh_stop();
		return 1;
	}
	return 0;
}

void zn_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	UINT16 n_r;
	UINT16 n_g;
	UINT16 n_b;
	UINT32 n_colour;

	for( n_colour = 0; n_colour < 0x10000; n_colour++ )
	{
		n_r = n_colour & 0x1f;
		n_g = ( n_colour >> 5 ) & 0x1f;
		n_b = ( n_colour >> 10 ) & 0x1f;

		if( Machine->scrbitmap->depth == 16 )
		{
			*( palette++ ) = ( n_r * 0xff ) / 0x1f;
			*( palette++ ) = ( n_g * 0xff ) / 0x1f;
			*( palette++ ) = ( n_b * 0xff ) / 0x1f;
		}
		else
		{
			*( palette++ ) = ( ( n_r & 0x1c ) << 3 ) | ( n_r & 0x1c ) | ( ( n_r & 0x1c ) >> 3 );
			*( palette++ ) = ( ( n_g & 0x1c ) << 3 ) | ( n_g & 0x1c ) | ( ( n_g & 0x1c ) >> 3 );
			*( palette++ ) = ( ( n_b & 0x18 ) << 3 ) | ( ( n_b & 0x18 ) << 1 ) | ( ( n_b & 0x18 ) >> 1 ) | ( ( n_b & 0x18 ) >> 3 );
		}

		colortable[ n_colour ] = n_colour;
	}
}

static void zn_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh )
{
	UINT16 n_x;
	UINT16 n_y;
	UINT32 *pens = Machine->pens;

	if( palette_recalc() || full_refresh )
	{
		if( bitmap->depth == 16 )
		{
			UINT16 *p_line;
			for( n_y = 0; n_y < bitmap->height; n_y++ )
			{
				p_line = (UINT16 *)bitmap->line[ n_y ];
				for( n_x = 0; n_x < bitmap->width; n_x++ )
				{
					*( p_line++ ) = pens[ m_p_vram[ ( n_y * 1024 ) + n_x ] ];
				}
			}
		}
		else
		{
			UINT8 *p_line;
			for( n_y = 0; n_y < bitmap->height; n_y++ )
			{
				p_line = (UINT8 *)bitmap->line[ n_y ];
				for( n_x = 0; n_x < bitmap->width; n_x++ )
				{
					*( p_line++ ) = pens[ m_p_vram[ ( n_y * 1024 ) + n_x ] ];
				}
			}
		}
	}
}

static void triangle( UINT32 n_x1, UINT32 n_y1, UINT32 n_x2, UINT32 n_y2, UINT32 n_x3, UINT32 n_y3, UINT32 n_colour )
{
	UINT32 n_x;
	UINT32 n_y;
	UINT32 n_mx;
	UINT32 n_my;
	UINT32 n_cx1;
	UINT32 n_cx2;
	INT32 n_dx1;
	INT32 n_dx2;

	n_colour = ( ( n_colour >> 3 ) & 0x1f ) |
		( ( ( n_colour >> 11 ) & 0x1f ) << 5 ) |
		( ( ( n_colour >> 19 ) & 0x1f ) << 10 );

	n_cx1 = n_x1 << 16;
	n_cx2 = n_x1 << 16;
	if( n_y1 < n_y3 )
	{
		n_dx1 = (INT32)( ( n_x3 << 16 ) - n_cx1 ) / (INT32)( n_y3 - n_y1 );
	}
	else
	{
		n_dx1 = 0;
	}
	if( n_y1 < n_y2 )
	{
		n_dx2 = (INT32)( ( n_x2 << 16 ) - n_cx2 ) / (INT32)( n_y2 - n_y1 );
	}
	else
	{
		n_dx2 = 0;
	}

	n_my = n_y2;
	if( n_my < n_y3 )
	{
		n_my = n_y3;
	}
	if( n_my > 512 )
	{
		n_my = 512;
	}
	n_y = n_y1;
	while( n_y < n_my )
	{
		if( n_y == n_y2 )
		{
			n_cx2 = n_x2 << 16;
			n_dx2 = (INT32)( n_cx2 - ( n_x3 << 16 ) ) / (INT32)( n_y2 - n_y3 );
		}
		if( n_y == n_y3 )
		{
			n_cx1 = n_x3 << 16;
			n_dx1 = (INT32)( n_cx1 - ( n_x2 << 16 ) ) / (INT32)( n_y3 - n_y2 );
		}
		n_mx = ( n_cx2 >> 16 );
		if( n_mx > 1024 )
		{
			n_mx = 1024;
		}
		n_x = ( n_cx1 >> 16 );
		while( n_x < n_mx )
		{
			m_p_vram[ n_y * 1024 + n_x ] = n_colour;
			n_x++;
		}
		n_cx1 += n_dx1;
		n_cx2 += n_dx2;
		n_y++;
	}
	schedule_full_refresh();
}

void gpu32_w( UINT32 offset, UINT32 data )
{
	switch( offset )
	{
	case 0x00:
		m_p_gpu_buffer[ m_n_gpu_buffer_offset ] = data;
		switch( m_p_gpu_buffer[ 0 ] >> 24 )
		{
		case 0x28:
			if( m_n_gpu_buffer_offset < 4 )
			{
				m_n_gpu_buffer_offset++;
			}
			else
			{
				triangle( m_p_gpu_buffer[ 1 ] & 0xffff, m_p_gpu_buffer[ 1 ] >> 16, m_p_gpu_buffer[ 2 ] & 0xffff, m_p_gpu_buffer[ 2 ] >> 16, m_p_gpu_buffer[ 3 ] & 0xffff, m_p_gpu_buffer[ 3 ] >> 16, m_p_gpu_buffer[ 0 ] & 0xffffff );
				triangle( m_p_gpu_buffer[ 2 ] & 0xffff, m_p_gpu_buffer[ 2 ] >> 16, m_p_gpu_buffer[ 4 ] & 0xffff, m_p_gpu_buffer[ 4 ] >> 16, m_p_gpu_buffer[ 3 ] & 0xffff, m_p_gpu_buffer[ 3 ] >> 16, m_p_gpu_buffer[ 0 ] & 0xffffff );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		}
		break;
	}
}

UINT32 gpu32_r( UINT32 offset )
{
	switch( offset )
	{
	case 0x04:
		return 0x14802000;
	}
	return 0;
}

/*

Simple 16 to 32bit bridge.

Only one register can be written to / read per instruction.
Writes occur after the program counter has been incremented.
Transfers are always 32bits no matter what the cpu reads / writes.

*/

static UINT32 m_n_bridge_data;
void *m_p_bridge_timer;
void ( *m_p_bridge32_w )( UINT32 offset, UINT32 data );

static void bridge_w_flush( int offset )
{
	m_p_bridge32_w( offset, m_n_bridge_data );
	m_p_bridge_timer = NULL;
}

void bridge_w( void ( *p_bridge32_w )( UINT32 offset, UINT32 data ), UINT32 offset, UINT32 data )
{
	if( ( offset % 4 ) != 0 )
	{
		m_n_bridge_data = ( m_n_bridge_data & 0x0000ffff ) | ( data << 16 );
	}
	else
	{
		m_n_bridge_data = ( m_n_bridge_data & 0xffff0000 ) | ( data & 0xffff );
	}
	if( m_p_bridge_timer == NULL )
	{
		m_p_bridge32_w = p_bridge32_w;
		m_p_bridge_timer = timer_set( TIME_NOW, offset & ~3, bridge_w_flush );
	}
}

static void bridge_r_flush( int offset )
{
	m_p_bridge_timer = NULL;
}

UINT16 bridge_r( UINT32 ( *p_bridge32_r )( UINT32 offset ), UINT32 offset )
{
	if( m_p_bridge_timer == NULL )
	{
		m_p_bridge_timer = timer_set( TIME_NOW, 0, bridge_r_flush );
		m_n_bridge_data = p_bridge32_r( offset & ~3 );
	}
	if( ( offset % 4 ) != 0 )
	{
		return m_n_bridge_data >> 16;
	}
	return m_n_bridge_data & 0xffff;
}

WRITE16_HANDLER( gpu_w )
{
	bridge_w( gpu32_w, offset<<1, data );
}

READ16_HANDLER( gpu_r )
{
	return bridge_r( gpu32_r, offset<<1 );
}

static MEMORY_READ16_START( zn_readmem )
	{ 0x00000000, 0x001fffff, MRA16_RAM },	/* ram */
	{ 0x1f801810, 0x1f801817, gpu_r },
	{ 0xa0000000, 0xa01fffff, MRA16_BANK1 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MRA16_BANK2 },	/* bios */
MEMORY_END

static MEMORY_WRITE16_START( zn_writemem )
	{ 0x00000000, 0x001fffff, MWA16_RAM },	/* ram */
	{ 0x1f801810, 0x1f801817, gpu_w },
	{ 0xa0000000, 0xa01fffff, MWA16_BANK1 },	/* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MWA16_ROM },	/* bios */
MEMORY_END

static void zn_init_machine( void )
{
	cpu_setbank( 1, memory_region( REGION_CPU1 ) );
	cpu_setbank( 2, memory_region( REGION_USER1 ) );

	m_n_gpu_buffer_offset = 0;
	memset( m_p_vram, 0x00, VRAM_SIZE );

	m_p_bridge_timer = NULL;
}

static struct MachineDriver machine_driver_zn =
{
	/* basic machine hardware */
	{
		{
			CPU_PSXCPU,
			33868800, /* 33MHz ?? */
			zn_readmem, zn_writemem, 0, 0,
			ignore_interrupt,1 /* ??? interrupts per frame */
		},
	},
	60, 0,
	1,
	zn_init_machine,

	/* video hardware */
	256, 240, { 0, 255, 0, 239 },
	0,
	65536,65536,
	zn_init_palette,

	VIDEO_TYPE_RASTER,
	0,
	zn_vh_start,
	zn_vh_stop,
	zn_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
};

ROM_START( doapp )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION16_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "mg-bios.bin",  0x0000000, 0x080000, 0x69ffbcb4 )

	ROM_REGION16_LE( 0x01a00000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "doapp119.bin", 0x0000001, 0x100000, 0xbbe04cef )
	ROM_LOAD16_BYTE( "doapp120.bin", 0x0000000, 0x100000, 0xb614d7e6 )
	ROM_LOAD16_WORD( "doapp-0.216",  0x0200000, 0x400000, 0xacc6c539 )
	ROM_LOAD16_WORD( "doapp-1.217",  0x0600000, 0x400000, 0x14b961c4 )
	ROM_LOAD16_WORD( "doapp-2.218",  0x0a00000, 0x400000, 0x134f698f )
	ROM_LOAD16_WORD( "doapp-3.219",  0x0e00000, 0x400000, 0x1c6540f3 )
	ROM_LOAD16_WORD( "doapp-4.220",  0x1200000, 0x400000, 0xf83bacf7 )
	ROM_LOAD16_WORD( "doapp-5.221",  0x1600000, 0x400000, 0xe11e8b71 )
ROM_END

ROM_START( glpracr2 )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION16_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "mg-bios.bin",  0x0000000, 0x080000, 0x69ffbcb4 )

	ROM_REGION16_LE( 0x02200000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "1.119",        0x0000001, 0x100000, 0x0fe2d2df )
	ROM_LOAD16_BYTE( "1.120",        0x0000000, 0x100000, 0x8e3fb1c0 )
	ROM_LOAD16_WORD( "gra2-0.217",   0x0200000, 0x400000, 0xa077ffa3 )
	ROM_LOAD16_WORD( "gra2-1.218",   0x0600000, 0x400000, 0x28ce033c )
	ROM_LOAD16_WORD( "gra2-2.219",   0x0a00000, 0x400000, 0x0c9cb7da )
	ROM_LOAD16_WORD( "gra2-3.220",   0x0e00000, 0x400000, 0x264e3a0c )
	ROM_LOAD16_WORD( "gra2-4.221",   0x1200000, 0x400000, 0x2b070307 )
	ROM_LOAD16_WORD( "gra2-5.222",   0x1600000, 0x400000, 0x94a363c1 )
	ROM_LOAD16_WORD( "gra2-6.223",   0x1a00000, 0x400000, 0x8c6b4c4c )
	ROM_LOAD16_WORD( "gra2-7.323",   0x1e00000, 0x400000, 0x7dfb6c54 )
ROM_END

ROM_START( sncwgltd )
	ROM_REGION( 0x200400, REGION_CPU1, 0 )

	ROM_REGION16_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "kn-bios.bin",  0x0000000, 0x080000, 0x5ff165f3 )

	ROM_REGION16_LE( 0x01a80000, REGION_USER2, 0 )
	ROM_LOAD16_WORD( "ic5.bin",      0x0000000, 0x080000, 0x458f14aa )
	ROM_LOAD16_WORD( "ic6.bin",      0x0080000, 0x080000, 0x8233dd1e )
	ROM_LOAD16_WORD( "ic7.bin",      0x0100000, 0x080000, 0xdf5ba2f7 )
	ROM_LOAD16_WORD( "ic8.bin",      0x0180000, 0x080000, 0xe8145f2b )
	ROM_LOAD16_WORD( "ic9.bin",      0x0200000, 0x080000, 0x605c9370 )
	ROM_LOAD16_WORD( "ic11.bin",     0x0280000, 0x400000, 0xa93f6fee )
	ROM_LOAD16_WORD( "ic12.bin",     0x0680000, 0x400000, 0x9f584ef7 )
	ROM_LOAD16_WORD( "ic13.bin",     0x0a80000, 0x400000, 0x652e9c78 )
	ROM_LOAD16_WORD( "ic14.bin",     0x0e80000, 0x400000, 0xc4ef1424 )
	ROM_LOAD16_WORD( "ic15.bin",     0x1280000, 0x400000, 0x2551d816 )
	ROM_LOAD16_WORD( "ic16.bin",     0x1680000, 0x400000, 0x21b401bc )
ROM_END




GAME( 1995, ts2j,	  0,		znqs, zn, 0, ROT0, "Capcom/Takara", "Battle Arena Toshinden 2 (JAPAN 951124)" )
GAME( 1996, sfex,	  0,		znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX (ASIA 961219)" )
GAME( 1996, sfexj,	  sfex, 	znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX (JAPAN 961130)" )
GAME( 1997, sfexp,	  0,		znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX Plus (USA 970311)" )
GAME( 1997, sfexpj,   sfexp,	znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX Plus (JAPAN 970311)" )
GAME( 1997, rvschool, 0,		znqs, zn, 0, ROT0, "Capcom", "Rival Schools (ASIA 971117)" )
GAME( 1997, jgakuen,  rvschool, znqs, zn, 0, ROT0, "Capcom", "Justice Gakuen (JAPAN 971117)" )
GAME( 1998, sfex2,	  0,		znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX 2 (JAPAN 980312)" )
GAME( 1998, tgmj,	  0,		znqs, zn, 0, ROT0, "Capcom/Akira", "Tetris The Grand Master (JAPAN 980710)" )
GAME( 1998, kikaioh,  0,		znqs, zn, 0, ROT0, "Capcom", "Kikaioh (JAPAN 980914)" )
GAME( 1999, sfex2p,   0,		znqs, zn, 0, ROT0, "Capcom/Arika", "Street Fighter EX 2 Plus (JAPAN 990611)" )
GAME( 1999, shiryu2,  0,		znqs, zn, 0, ROT0, "Capcom", "Strider Hiryu 2 (JAPAN 991213)" )

GAME( 1996, sncwgltd,	0,	  zn, zn, 0, ROT0_16BIT, "Video System Co.", "Sonic Wings Limited (JAPAN)" )
GAME( 1997, glpracr2,	0,	  zn, zn, 0, ROT0_16BIT, "Tecmo", "Gallop Racer 2 (JAPAN)" )
GAME( 1998, doapp,		0,	  zn, zn, 0, ROT0_16BIT, "Tecmo", "Dead Or Alive ++ (JAPAN)" )
