/*******************************************************************************
 WWF Wrestlefest (C) 1991 Technos Japan  (drivers/wwfwfest.c)
********************************************************************************
 driver by David Haywood

 Special Thanks to:

 Richard Bush & the Rest of the Raine Team - Raine's WWF Wrestlefest driver on
 which some of this driver has been based.

********************************************************************************

 Hardware:

 Primary CPU : 68000 - 12mhz

 Sound CPUs : Z80 - ?mhz

 Sound Chips : YM2151, M6295

 4 Layers from now on if mentioned will be refered to as

 BG0 - Background Layer 0
 BG1 - Background Layer 1
 SPR - Sprites
 FG0 - Foreground / Text Layer

 Priorities of BG0, BG1 and SPR can be changed

********************************************************************************

 Change Log:
 20 Jun 2001 | Did Pretty Much everything else, the game is now playable.
 19 Jun 2001 | Started the driver, based on Raine, the WWF Superstars driver,
			 | and the Double Dragon 3 Driver, got most of the basics done,
			 | the game will boot showing some graphics.

********************************************************************************

 Notes:

- Sound Volumes, probably not right
- DSW's are mapped weirdly, i'll verify my code here, and try to work out some
  of the others, Raine's DSW's don't seem to be correct
- Improve Sprite Code, I think it may clip at the bottom sometimes when it
  shouldn't
- Improve way priority is handled, which bits of the control register are
  significant etc.
- Demolition don't seem to have a logo in Tag mode? is this normal? its just a
  block of Junk but i don't see any other way it could be.
- Palette Code could probably be improved, lot of unused colours at the moment
  because of the odd way palette ram is used

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"

data16_t *fg0_videoram, *bg0_videoram, *bg1_videoram;

/*- in this file -*/
static READ16_HANDLER( wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_r );
static WRITE16_HANDLER( wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_w );
static WRITE16_HANDLER( wwfwfest_1410_write ); /* priority write */
static WRITE16_HANDLER( wwfwfest_scroll_write ); /* scrolling write */
static READ16_HANDLER( wwfwfest_inputs_read );
static WRITE_HANDLER( oki_bankswitch_w );
static WRITE16_HANDLER ( wwfwfest_soundwrite );

/*- vidhrdw/wwfwfest.c -*/
int wwfwfest_vh_start(void);
void wwfwfest_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE16_HANDLER( wwfwfest_fg0_videoram_w );
WRITE16_HANDLER( wwfwfest_bg0_videoram_w );
WRITE16_HANDLER( wwfwfest_bg1_videoram_w );

/*******************************************************************************
 Memory Maps
********************************************************************************
 Pretty Straightforward

 still some unknown writes however, sound cpu memory map is the same as dd3
*******************************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },	/* Rom */
	{ 0x0c0000, 0x0c1fff, MRA16_RAM },	/* FG0 Ram */
	{ 0x0c2000, 0x0c3fff, MRA16_RAM },	/* SPR Ram */
	{ 0x080000, 0x080fff, MRA16_RAM },	/* BG0 Ram */
	{ 0x082000, 0x082fff, MRA16_RAM },	/* BG1 Ram */
	{ 0x140020, 0x140027, wwfwfest_inputs_read },	/* Inputs */
	{ 0x180000, 0x18ffff, wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_r },	/* BG0 Ram */
	{ 0x1c0000, 0x1c3fff, MRA16_RAM },	/* Work Ram */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },	/* Rom */
	{ 0x0c0000, 0x0c1fff, wwfwfest_fg0_videoram_w, &fg0_videoram },	/* FG0 Ram - 4 bytes per tile */
	{ 0x0c2000, 0x0c3fff, MWA16_RAM, &spriteram16 },				/* SPR Ram */
	{ 0x080000, 0x080fff, wwfwfest_bg0_videoram_w, &bg0_videoram },	/* BG0 Ram - 4 bytes per tile */
	{ 0x082000, 0x082fff, wwfwfest_bg1_videoram_w, &bg1_videoram },	/* BG1 Ram - 2 bytes per tile */
	{ 0x100000, 0x100007, wwfwfest_scroll_write },
	{ 0x14000C, 0x14000D, wwfwfest_soundwrite },
	{ 0x140010, 0x140011, wwfwfest_1410_write },
	{ 0x180000, 0x18ffff, wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x1c0000, 0x1c3fff, MWA16_RAM },	/* Work Ram */
MEMORY_END

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc801, 0xc801, YM2151_status_port_0_r },
	{ 0xd800, 0xd800, OKIM6295_status_0_r },
	{ 0xe000, 0xe000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc800, YM2151_register_port_0_w },
	{ 0xc801, 0xc801, YM2151_data_port_0_w },
	{ 0xd800, 0xd800, OKIM6295_data_0_w },
	{ 0xe800, 0xe800, oki_bankswitch_w },
MEMORY_END

/*******************************************************************************
 Read / Write Handlers
********************************************************************************
 as used by the above memory map
*******************************************************************************/

/*- Palette Reads/Writes -*/

static READ16_HANDLER( wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_r )
{
	offset = (offset & 0x000f) | (offset & 0x7fc0) >> 2;
	return paletteram16[offset];
}

static WRITE16_HANDLER( wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_w )
{
	offset = (offset & 0x000f) | (offset & 0x7fc0) >> 2;
	paletteram16_xxxxBBBBGGGGRRRR_word_w (offset, data, mem_mask);
}

/*- Priority Control -*/

int pri;

static WRITE16_HANDLER( wwfwfest_1410_write )
{
	pri = data;
}

/*- Scroll Control -*/

int bg0_scrollx, bg0_scrolly, bg1_scrollx, bg1_scrolly;

static WRITE16_HANDLER( wwfwfest_scroll_write )
{
	switch (offset) {
		case 0x00:
			bg0_scrollx = data;
			break;
		case 0x01:
			bg0_scrolly = data;
			break;
		case 0x02:
			bg1_scrollx = data;
			break;
		case 0x03:
			bg1_scrolly = data;
			break;
	}
}

/*- Inputs -*/
/* todo: verify this */

static READ16_HANDLER( wwfwfest_inputs_read )
{
	int tmp = 0;

	switch (offset)
	{
	case 0x00:
	tmp = readinputport(0) | readinputport(4) << 8;
	tmp &= 0xcfff;
	tmp |= ((readinputport(7) & 0xc0) << 6);
	break;
	case 0x01:
	tmp = readinputport(1);
	tmp &= 0xc0ff;
	tmp |= ((readinputport(7) & 0x3f)<<8);
	break;
	case 0x02:
	tmp = readinputport(2);
	tmp &= 0xc0ff;
	tmp |= ((readinputport(6) & 0x3f)<<8);
	break;
	case 0x03:
	tmp = (readinputport(3) | readinputport(5) << 8) ;
	tmp &= 0xfcff;
	tmp |= ((readinputport(6) & 0xc0) << 2);
	break;
	}

	return tmp;
}

/*- Sound Related (from dd3) -*/

static WRITE_HANDLER( oki_bankswitch_w )
{
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static WRITE16_HANDLER ( wwfwfest_soundwrite )
{
	soundlatch_w(1,data & 0xff);
	cpu_cause_interrupt( 1, Z80_NMI_INT );
}

/*******************************************************************************
 Input Ports
********************************************************************************
 There are 4 players, 2 sets of dipswitches and 2 misc
*******************************************************************************/

INPUT_PORTS_START( wwfwfest )
	PORT_START	/* Player 1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START /* Player 2 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START /* Player 3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START /* Player 4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START /* Misc 1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* Misc 2 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* Dips 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x00,  DEF_STR( 3C_1C )  )
	PORT_DIPSETTING(	0x01,  DEF_STR( 2C_1C )  )
	PORT_DIPSETTING(	0x03,  DEF_STR( 1C_1C )  )
	PORT_DIPSETTING(	0x02,  DEF_STR( 1C_2C )  )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown )  )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "FBI Logo" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown )  )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* Dips 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Championship Game" )
	PORT_DIPSETTING(	0x00, "4th" )
	PORT_DIPSETTING(	0x80, "5th" )
INPUT_PORTS_END

/*******************************************************************************
 Graphic Decoding
*******************************************************************************/
static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 2, 4, 6 },
	{ 1, 0, 8*8+1, 8*8+0, 16*8+1, 16*8+0, 24*8+1, 24*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout tile_layout =
{
	16,16,	/* 16*16 tiles */
	4096,	/* 8192 tiles */
	4,	/* 4 bits per pixel */
	{ 8, 0, 0x40000*8+8 , 0x40000*8+0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*8, 16*9, 16*10, 16*11, 16*12, 16*13, 16*14, 16*15 },
	64*8	/* every tile takes 64 consecutive bytes */
};

static struct GfxLayout sprite_layout = {
	16,16,	/* 16*16 tiles */
	RGN_FRAC(1,4),
	4,	/* 4 bits per pixel */
	{ 0, 0x200000*8, 2*0x200000*8 , 3*0x200000*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0x0000, 16 },
	{ REGION_GFX2, 0, &sprite_layout,   0x0400, 16 },
	{ REGION_GFX3, 0, &tile_layout,     0x1000, 16 },
	{ REGION_GFX3, 0, &tile_layout,     0x0c00, 16 },
	{ -1 }
};

/*******************************************************************************
 Interrupt Function
*******************************************************************************/

static int wwfwfest_interrupt(void) {
	if( cpu_getiloops() == 0 )
		return MC68000_IRQ_3;

	 return MC68000_IRQ_2;
}

/*******************************************************************************
 Sound Stuff..
********************************************************************************
 Straight from Ddragon 3 with some adjusted volumes
*******************************************************************************/

static void dd3_ymirq_handler(int irq)
{
	cpu_set_irq_line( 1, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* Guess */
	{ YM3012_VOL(10,MIXER_PAN_LEFT,10,MIXER_PAN_RIGHT) },
	{ dd3_ymirq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 90 }
};

/*******************************************************************************
 Machine Driver(s)
*******************************************************************************/

static struct MachineDriver machine_driver_wwfwfest =
{
	/* basic machine hardware */
	{

		{
			CPU_M68000,
			12000000,	/* 24 crystal, 12 rated chip */
			readmem,writemem,0,0,
			wwfwfest_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* Guess */
			readmem_sound,writemem_sound,0,0,
			ignore_interrupt,0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 256, { 0, 319, 0, 255 },
	gfxdecodeinfo,
	0x2000, 0x2000,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	wwfwfest_vh_start,
	0,
	wwfwfest_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
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

/*******************************************************************************
 Rom Loaders / Game Drivers
********************************************************************************
 2 sets supported,
 wwfwfest - US? Set (Tecmo License / Distribution?)
 wwfwfstj - Japan? Set

 readme / info files below

--------------------------------------------------------------------------------
 wwfwfest: readthis.doc
--------------------------------------------------------------------------------
 Here are the proms for Technos Wrestlefest
 It runs on a MC 68000. It also has one Z80 CPU
 And one YM 2151 for the sound!
 IC-73 on the board which I dumped as WF_73a + b, could
 be a 4 Mbit or a 2 Mbit prom so I dumped it 2 times.

 If you need more info or if this package doesn't
 Work, mail me.

 ..............CaBBe!.............Romlist@hotmail.com

--------------------------------------------------------------------------------
 wwfwfstj: README.TXT
--------------------------------------------------------------------------------
 Wrestlefest
 Technos 1991

 TA-0031
                                                         68000-12
  31J0_IC1  6264 6264 31A14-2 31A13-2 6264 6264 31A12-0  24MHz
  31J1_IC2
                   TJ-002          TJ-004
                                   6264                   SW1
                   28MHz                                  SW2
                                                          SW3
                                             61C16-35
                        61C16-35             61C16-35
  31J2_IC8
  31J3_IC9
  31J4_IC10
  31J5_IC11
  31J6_IC12
  31J7_IC13
  31J8_IC14    TJ-003                31A11-2  M6295   31J10_IC73
  31J9_IC15    61C16-35 61C16-35     Z80      YM2151

*******************************************************************************/
ROM_START( wwfwfest )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Main CPU  (68000) */
	ROM_LOAD16_BYTE( "31a13-2.19", 0x00001, 0x40000, 0x7175bca7 )
	ROM_LOAD16_BYTE( "31a14-2.18", 0x00000, 0x40000, 0x5d06bfd1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU (Z80)  */
	ROM_LOAD( "31a11-2.42",    0x00000, 0x10000, 0x5ddebfea )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "wf_73a.rom",    0x00000, 0x80000, 0x6c522edb )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* FG0 Tiles (8x8) */
	ROM_LOAD( "31a12-0.33",    0x00000, 0x20000, 0xd0803e20 )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPR Tiles (16x16) */
	ROM_LOAD( "wf_09.rom",    0x000000, 0x100000, 0xe395cf1d ) /* Tiles 0 */
	ROM_LOAD( "wf_08.rom",    0x100000, 0x100000, 0xb5a97465 ) /* Tiles 1 */
	ROM_LOAD( "wf_11.rom",    0x200000, 0x100000, 0x2ce545e8 ) /* Tiles 0 */
	ROM_LOAD( "wf_10.rom",    0x300000, 0x100000, 0x00edb66a ) /* Tiles 1 */
	ROM_LOAD( "wf_12.rom",    0x400000, 0x100000, 0x79956cf8 ) /* Tiles 0 */
	ROM_LOAD( "wf_13.rom",    0x500000, 0x100000, 0x74d774c3 ) /* Tiles 1 */
	ROM_LOAD( "wf_15.rom",    0x600000, 0x100000, 0xdd387289 ) /* Tiles 0 */
	ROM_LOAD( "wf_14.rom",    0x700000, 0x100000, 0x44abe127 ) /* Tiles 1 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG0 / BG1 Tiles (16x16) */
	ROM_LOAD( "wf_01.rom",    0x40000, 0x40000, 0x8a12b450 ) /* 0,1 */
	ROM_LOAD( "wf_02.rom",    0x00000, 0x40000, 0x82ed7155 ) /* 2,3 */
ROM_END

ROM_START( wwfwfsta )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Main CPU  (68000) */
	ROM_LOAD16_BYTE( "wf_18.rom", 0x00000, 0x40000, 0x933ea1a0 )
	ROM_LOAD16_BYTE( "wf_19.rom", 0x00001, 0x40000, 0xbd02e3c4 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU (Z80)  */
	ROM_LOAD( "31a11-2.42",    0x00000, 0x10000, 0x5ddebfea )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "wf_73a.rom",    0x00000, 0x80000, 0x6c522edb )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* FG0 Tiles (8x8) */
	ROM_LOAD( "wf_33.rom",    0x00000, 0x20000, 0x06f22615 )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPR Tiles (16x16) */
	ROM_LOAD( "wf_09.rom",    0x000000, 0x100000, 0xe395cf1d ) /* Tiles 0 */
	ROM_LOAD( "wf_08.rom",    0x100000, 0x100000, 0xb5a97465 ) /* Tiles 1 */
	ROM_LOAD( "wf_11.rom",    0x200000, 0x100000, 0x2ce545e8 ) /* Tiles 0 */
	ROM_LOAD( "wf_10.rom",    0x300000, 0x100000, 0x00edb66a ) /* Tiles 1 */
	ROM_LOAD( "wf_12.rom",    0x400000, 0x100000, 0x79956cf8 ) /* Tiles 0 */
	ROM_LOAD( "wf_13.rom",    0x500000, 0x100000, 0x74d774c3 ) /* Tiles 1 */
	ROM_LOAD( "wf_15.rom",    0x600000, 0x100000, 0xdd387289 ) /* Tiles 0 */
	ROM_LOAD( "wf_14.rom",    0x700000, 0x100000, 0x44abe127 ) /* Tiles 1 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG0 / BG1 Tiles (16x16) */
	ROM_LOAD( "wf_01.rom",    0x40000, 0x40000, 0x8a12b450 ) /* 0,1 */
	ROM_LOAD( "wf_02.rom",    0x00000, 0x40000, 0x82ed7155 ) /* 2,3 */
ROM_END

ROM_START( wwfwfstj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* Main CPU  (68000) */
	ROM_LOAD16_BYTE( "31j13-0.bin", 0x00001, 0x40000, 0x2147780d )
	ROM_LOAD16_BYTE( "31j14-0.bin", 0x00000, 0x40000, 0xd76fc747 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU (Z80)  */
	ROM_LOAD( "31a11-2.42",    0x00000, 0x10000, 0x5ddebfea )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "wf_73a.rom",    0x00000, 0x80000, 0x6c522edb )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* FG0 Tiles (8x8) */
	ROM_LOAD( "31j12-0.bin",    0x00000, 0x20000, 0xf4821fe0 )

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPR Tiles (16x16) */
	ROM_LOAD( "wf_09.rom",    0x000000, 0x100000, 0xe395cf1d ) /* Tiles 0 */
	ROM_LOAD( "wf_08.rom",    0x100000, 0x100000, 0xb5a97465 ) /* Tiles 1 */
	ROM_LOAD( "wf_11.rom",    0x200000, 0x100000, 0x2ce545e8 ) /* Tiles 0 */
	ROM_LOAD( "wf_10.rom",    0x300000, 0x100000, 0x00edb66a ) /* Tiles 1 */
	ROM_LOAD( "wf_12.rom",    0x400000, 0x100000, 0x79956cf8 ) /* Tiles 0 */
	ROM_LOAD( "wf_13.rom",    0x500000, 0x100000, 0x74d774c3 ) /* Tiles 1 */
	ROM_LOAD( "wf_15.rom",    0x600000, 0x100000, 0xdd387289 ) /* Tiles 0 */
	ROM_LOAD( "wf_14.rom",    0x700000, 0x100000, 0x44abe127 ) /* Tiles 1 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG0 / BG1 Tiles (16x16) */
	ROM_LOAD( "wf_01.rom",    0x40000, 0x40000, 0x8a12b450 ) /* 0,1 */
	ROM_LOAD( "wf_02.rom",    0x00000, 0x40000, 0x82ed7155 ) /* 2,3 */
ROM_END


GAME( 1991, wwfwfest, 0,        wwfwfest, wwfwfest, 0, ROT0, "Technos Japan", "WWF WrestleFest (US)" )
GAME( 1991, wwfwfsta, wwfwfest, wwfwfest, wwfwfest, 0, ROT0, "Technos Japan (Tecmo license)", "WWF WrestleFest (US Tecmo)" )
GAME( 1991, wwfwfstj, wwfwfest, wwfwfest, wwfwfest, 0, ROT0, "Technos Japan", "WWF WrestleFest (Japan)" )
