/***************************************************************************

Dooyong games

driver by Nicola Salmoria

The Last Day  Z80     Z80 2xYM2203
Pollux        Z80     Z80 2xYM2203
Blue Hawk     Z80     Z80 YM2151 OKI6295
Sadari        Z80     Z80 YM2151 OKI6295
Primella      Z80     Z80 YM2151 OKI6295
R-Shark       68000   Z80 YM2151 OKI6295

These games all run on different but similar hardware. A common thing that they
all have is tilemaps hardcoded in ROM.

TODO:
- video driver is not optimized at all; should do palette marking and remove 16BIT
- port A of both of the YM2203 is constantly read and stored in memory -
  function unknown
- some of the sound programs often write to the ROM area - is this just a bug, or
  is there something connected there?
Last Day:
- sprite/fg priority is not understood (tanks, boats should pass below bridges)
- when you insert a coin, the demo sprites continue to move in the background.
  Maybe the whole background and sprites are supposed to be disabled.
Blue Hawk:
- sprite/fg priority is not understood
Primella:
- does the game really support cocktail mode as service mode suggests?
- are buttons 2 and 3 used as service mode suggests?
R-Shark:
- sprite/fg priority is not understood

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *lastday_txvideoram;
extern unsigned char *lastday_bgscroll,*lastday_fgscroll,*bluehawk_fg2scroll;
extern data16_t *rshark_scroll1,*rshark_scroll2,*rshark_scroll3,*rshark_scroll4;

WRITE_HANDLER( lastday_ctrl_w );
WRITE_HANDLER( pollux_ctrl_w );
WRITE_HANDLER( primella_ctrl_w );
WRITE16_HANDLER( rshark_ctrl_w );
void lastday_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void pollux_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void bluehawk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void primella_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void rshark_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void dooyong_eof_callback(void);
void rshark_eof_callback(void);



static WRITE_HANDLER( lastday_bankswitch_w )
{
 	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + (data & 0x07) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

if (data & 0xf8) usrintf_showmessage("bankswitch %02x",data);
}

static WRITE_HANDLER( flip_screen_w )
{
	flip_screen_set(data);
}



static MEMORY_READ_START( lastday_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc010, 0xc010, input_port_0_r },
	{ 0xc011, 0xc011, input_port_1_r },
	{ 0xc012, 0xc012, input_port_2_r },
	{ 0xc013, 0xc013, input_port_3_r },	/* DSWA */
	{ 0xc014, 0xc014, input_port_4_r },	/* DSWB */
	{ 0xc800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( lastday_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc004, MWA_RAM, &lastday_bgscroll },
	{ 0xc008, 0xc00c, MWA_RAM, &lastday_fgscroll },
	{ 0xc010, 0xc010, lastday_ctrl_w },	/* coin counter, flip screen */
	{ 0xc011, 0xc011, lastday_bankswitch_w },
	{ 0xc012, 0xc012, soundlatch_w },
	{ 0xc800, 0xcfff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0xd000, 0xdfff, MWA_RAM, &lastday_txvideoram },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END

static MEMORY_READ_START( pollux_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf000, input_port_0_r },
	{ 0xf001, 0xf001, input_port_1_r },
	{ 0xf002, 0xf002, input_port_2_r },
	{ 0xf003, 0xf003, input_port_3_r },
	{ 0xf004, 0xf004, input_port_4_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( pollux_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xdfff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe000, 0xefff, MWA_RAM, &lastday_txvideoram },
	{ 0xf000, 0xf000, lastday_bankswitch_w },
	{ 0xf008, 0xf008, pollux_ctrl_w },	/* coin counter, flip screen */
	{ 0xf010, 0xf010, soundlatch_w },
	{ 0xf018, 0xf01c, MWA_RAM, &lastday_bgscroll },
	{ 0xf020, 0xf024, MWA_RAM, &lastday_fgscroll },
	{ 0xf800, 0xffff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
MEMORY_END

static MEMORY_READ_START( bluehawk_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xc800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( bluehawk_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, flip_screen_w },
	{ 0xc008, 0xc008, lastday_bankswitch_w },
	{ 0xc010, 0xc010, soundlatch_w },
	{ 0xc018, 0xc01c, MWA_RAM, &bluehawk_fg2scroll },
	{ 0xc040, 0xc044, MWA_RAM, &lastday_bgscroll },
	{ 0xc048, 0xc04c, MWA_RAM, &lastday_fgscroll },
	{ 0xc800, 0xcfff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
	{ 0xd000, 0xdfff, MWA_RAM, &lastday_txvideoram },
	{ 0xe000, 0xefff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( primella_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf800, 0xf800, input_port_0_r },
	{ 0xf810, 0xf810, input_port_1_r },
	{ 0xf820, 0xf820, input_port_2_r },
	{ 0xf830, 0xf830, input_port_3_r },
	{ 0xf840, 0xf840, input_port_4_r },
MEMORY_END

static MEMORY_WRITE_START( primella_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd3ff, MWA_RAM },	/* what is this? looks like a palette? scratchpad RAM maybe? */
	{ 0xe000, 0xefff, MWA_RAM, &lastday_txvideoram },
	{ 0xf000, 0xf7ff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
	{ 0xf800, 0xf800, primella_ctrl_w },	/* bank switch, flip screen etc */
	{ 0xf810, 0xf810, soundlatch_w },
	{ 0xfc00, 0xfc04, MWA_RAM, &lastday_bgscroll },
	{ 0xfc08, 0xfc0c, MWA_RAM, &lastday_fgscroll },
MEMORY_END

static MEMORY_READ16_START( rshark_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x340000, 0x34cfff, MRA16_RAM },
	{ 0x34d000, 0x34dfff, MRA16_RAM },
	{ 0x34e000, 0x34ffff, MRA16_RAM },
	{ 0x3c0002, 0x3c0003, input_port_0_word_r },
	{ 0x3c0004, 0x3c0005, input_port_1_word_r },
	{ 0x3c0006, 0x3c0007, input_port_2_word_r },
MEMORY_END

static MEMORY_WRITE16_START( rshark_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x340000, 0x34cfff, MWA16_RAM },
	{ 0x34d000, 0x34dfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x34e000, 0x34ffff, MWA16_RAM },
	{ 0x3c4000, 0x3c4009, MWA16_RAM, &rshark_scroll4 },
	{ 0x3c4010, 0x3c4019, MWA16_RAM, &rshark_scroll3 },
	{ 0x3c8000, 0x3c8fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0x3c0012, 0x3c0013, soundlatch_word_w },
	{ 0x3c0014, 0x3c0015, rshark_ctrl_w },	/* flip screen + unknown stuff */
	{ 0x3cc000, 0x3cc009, MWA16_RAM, &rshark_scroll2 },
	{ 0x3cc010, 0x3cc019, MWA16_RAM, &rshark_scroll1 },
MEMORY_END

static MEMORY_READ_START( lastday_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ 0xf001, 0xf001, YM2203_read_port_0_r },
	{ 0xf002, 0xf002, YM2203_status_port_1_r },
	{ 0xf003, 0xf003, YM2203_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( lastday_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
MEMORY_END

static MEMORY_READ_START( pollux_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, soundlatch_r },
	{ 0xf802, 0xf802, YM2203_status_port_0_r },
	{ 0xf803, 0xf803, YM2203_read_port_0_r },
	{ 0xf804, 0xf804, YM2203_status_port_1_r },
	{ 0xf805, 0xf805, YM2203_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( pollux_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf802, 0xf802, YM2203_control_port_0_w },
	{ 0xf803, 0xf803, YM2203_write_port_0_w },
	{ 0xf804, 0xf804, YM2203_control_port_1_w },
	{ 0xf805, 0xf805, YM2203_write_port_1_w },
MEMORY_END

static MEMORY_READ_START( bluehawk_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, soundlatch_r },
	{ 0xf809, 0xf809, YM2151_status_port_0_r },
	{ 0xf80a, 0xf80a, OKIM6295_status_0_r },
MEMORY_END

static MEMORY_WRITE_START( bluehawk_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf808, 0xf808, YM2151_register_port_0_w },
	{ 0xf809, 0xf809, YM2151_data_port_0_w },
	{ 0xf80a, 0xf80a, OKIM6295_data_0_w },
MEMORY_END



INPUT_PORTS_START( lastday )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_TILT )	/* maybe, but I'm not sure */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc2, 0xc2, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x42, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) ) */
	PORT_DIPSETTING(    0xc2, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x82, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "Every 200000" )
	PORT_DIPSETTING(    0x20, "Every 240000" )
	PORT_DIPSETTING(    0x10, "280000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Speed" )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x40, "High" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( pollux )
	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc2, 0xc2, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x42, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) ) */
	PORT_DIPSETTING(    0xc2, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x82, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( bluehawk )
	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc2, 0xc2, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x42, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) ) */
	PORT_DIPSETTING(    0xc2, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x82, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( primella )
	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc2, 0xc2, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x42, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) ) */
	PORT_DIPSETTING(    0xc2, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x82, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, "Show Girl" )
	PORT_DIPSETTING(    0x00, "Skip Skip Skip" )
	PORT_DIPSETTING(    0x03, "Dress Dress Dress" )
	PORT_DIPSETTING(    0x02, "Dress Half Half" )
	PORT_DIPSETTING(    0x01, "Dress Half Nake" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( rshark )
	PORT_START
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c2, 0x00c2, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0042, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ) ) */
	PORT_DIPSETTING(      0x00c2, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0082, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout lastday_charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout bluehawk_charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tilelayout =
{
	32,32,
	RGN_FRAC(1,1),
	4,
	{ 0*4, 1*4, 2*4, 3*4 },
	{ 0, 1, 2, 3, 16+0, 16+1, 16+2, 16+3,
			32*32+0, 32*32+1, 32*32+2, 32*32+3, 32*32+16+0, 32*32+16+1, 32*32+16+2, 32*32+16+3,
			2*32*32+0, 2*32*32+1, 2*32*32+2, 2*32*32+3, 2*32*32+16+0, 2*32*32+16+1, 2*32*32+16+2, 2*32*32+16+3,
			3*32*32+0, 3*32*32+1, 3*32*32+2, 3*32*32+3, 3*32*32+16+0, 3*32*32+16+1, 3*32*32+16+2, 3*32*32+16+3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32,
			24*32, 25*32, 26*32, 27*32, 28*32, 29*32, 30*32, 31*32 },
	512*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0*4, 1*4, 2*4, 3*4 },
	{ 0, 1, 2, 3, 16+0, 16+1, 16+2, 16+3,
			16*32+0, 16*32+1, 16*32+2, 16*32+3, 16*32+16+0, 16*32+16+1, 16*32+16+2, 16*32+16+3 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout rshark_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};


static struct GfxDecodeInfo lastday_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &lastday_charlayout,   0, 16 },
	{ REGION_GFX2, 0, &spritelayout,       256, 16 },
	{ REGION_GFX3, 0, &tilelayout,         768, 16 },
	{ REGION_GFX4, 0, &tilelayout,         512, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo bluehawk_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bluehawk_charlayout,  0, 16 },
	{ REGION_GFX2, 0, &spritelayout,       256, 16 },
	{ REGION_GFX3, 0, &tilelayout,         768, 16 },
	{ REGION_GFX4, 0, &tilelayout,         512, 16 },
	{ REGION_GFX5, 0, &tilelayout,           0, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo primella_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bluehawk_charlayout,  0, 16 },
	/* no sprites */
	{ REGION_GFX2, 0, &tilelayout,         768, 16 },
	{ REGION_GFX3, 0, &tilelayout,         512, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rshark_gfxdecodeinfo[] =
{
	/* no chars */
	{ REGION_GFX1, 0, &rshark_spritelayout,  0, 16 },
	{ REGION_GFX2, 0, &spritelayout,       256, 16 },
	{ REGION_GFX3, 0, &spritelayout,       512, 16 },
	{ REGION_GFX4, 0, &spritelayout,       768, 16 },
	{ REGION_GFX5, 0, &spritelayout,      1024, 16 },
	{ -1 } /* end of array */
};



static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

READ_HANDLER( unk_r )
{
	return 0;
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	4000000,	/* 4 MHz ? */
	{ YM2203_VOL(40,40), YM2203_VOL(40,40) },
	{ unk_r, unk_r },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct YM2151interface bluehawk_ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irqhandler },
	{ 0 }
};

static struct YM2151interface primella_ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(40,MIXER_PAN_CENTER,40,MIXER_PAN_CENTER) },
	{ irqhandler },
	{ 0 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* 8000Hz frequency? */
	{ REGION_SOUND1 },	/* memory region */
	{ 60 }
};



static struct MachineDriver machine_driver_lastday =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			8000000,	/* ??? */
			lastday_readmem,lastday_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ??? */
			lastday_sound_readmem,lastday_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2203 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 1*8, 31*8-1 },
	lastday_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	dooyong_eof_callback,
	0,
	0,
	lastday_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static struct MachineDriver machine_driver_pollux =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			8000000,	/* ??? */
			pollux_readmem,pollux_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ??? */
			pollux_sound_readmem,pollux_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2203 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 1*8, 31*8-1 },
	lastday_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	dooyong_eof_callback,
	0,
	0,
	pollux_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static struct MachineDriver machine_driver_bluehawk =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			8000000,	/* ??? */
			bluehawk_readmem,bluehawk_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ??? */
			bluehawk_sound_readmem,bluehawk_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 1*8, 31*8-1 },
	bluehawk_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	dooyong_eof_callback,
	0,
	0,
	bluehawk_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&bluehawk_ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_primella =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			8000000,	/* ??? */
			primella_readmem,primella_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ??? */
			bluehawk_sound_readmem,bluehawk_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 0*8, 32*8-1 },
	primella_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	dooyong_eof_callback,
	0,
	0,
	primella_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&primella_ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static int rshark_interrupt(void)
{
	if (cpu_getiloops() == 0) return 5;
	else return 6;
}

static struct MachineDriver machine_driver_rshark =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* ??? */
			rshark_readmem,rshark_writemem,0,0,
			rshark_interrupt,2	/* 5 and 6 */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ??? */
			bluehawk_sound_readmem,bluehawk_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 1*8, 31*8-1 },
	rshark_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	rshark_eof_callback,
	0,
	0,
	rshark_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&primella_ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};



ROM_START( lastday )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for banks */
	ROM_LOAD( "lday3.bin",    0x00000, 0x10000, 0xa06dfb1e )
	ROM_RELOAD(               0x10000, 0x10000 )				/* banked at 0x8000-0xbfff */
	ROM_LOAD( "lday4.bin",    0x20000, 0x10000, 0x70961ea6 )	/* banked at 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "lday1.bin",    0x0000, 0x8000, 0xdd4316fd )	/* empty */
	ROM_CONTINUE(             0x0000, 0x8000 )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "lday2.bin",    0x0000, 0x8000, 0x83eb572c )	/* empty */
	ROM_CONTINUE(             0x0000, 0x8000 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_BYTE( "lday16.bin",   0x00000, 0x20000, 0xdf503504 )
	ROM_LOAD16_BYTE( "lday15.bin",   0x00001, 0x20000, 0xcd990442 )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD16_BYTE( "lday6.bin",    0x00000, 0x20000, 0x1054361d )
	ROM_LOAD16_BYTE( "lday9.bin",    0x00001, 0x20000, 0x6952ef4d )
	ROM_LOAD16_BYTE( "lday7.bin",    0x40000, 0x20000, 0x6e57a888 )
	ROM_LOAD16_BYTE( "lday10.bin",   0x40001, 0x20000, 0xa5548dca )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD16_BYTE( "lday12.bin",   0x00000, 0x20000, 0x992bc4af )
	ROM_LOAD16_BYTE( "lday14.bin",   0x00001, 0x20000, 0xa79abc85 )

	ROM_REGION( 0x20000, REGION_GFX5, 0 )	/* background tilemaps */
	ROM_LOAD16_BYTE( "lday5.bin",    0x00000, 0x10000, 0x4789bae8 )
	ROM_LOAD16_BYTE( "lday8.bin",    0x00001, 0x10000, 0x92402b9a )

	ROM_REGION( 0x20000, REGION_GFX6, 0 )	/* fg tilemaps */
	ROM_LOAD16_BYTE( "lday11.bin",   0x00000, 0x10000, 0x04b961de )
	ROM_LOAD16_BYTE( "lday13.bin",   0x00001, 0x10000, 0x6bdbd887 )
ROM_END

ROM_START( lastdaya )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for banks */
	ROM_LOAD( "lday3.bin",    0x00000, 0x10000, 0xa06dfb1e )
	ROM_RELOAD(               0x10000, 0x10000 )				/* banked at 0x8000-0xbfff */
	ROM_LOAD( "lday4.bin",    0x20000, 0x10000, 0x70961ea6 )	/* banked at 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "e1",           0x0000, 0x8000, 0xce96e106 )	/* empty */
	ROM_CONTINUE(             0x0000, 0x8000 )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "lday2.bin",    0x0000, 0x8000, 0x83eb572c )	/* empty */
	ROM_CONTINUE(             0x0000, 0x8000 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_BYTE( "lday16.bin",   0x00000, 0x20000, 0xdf503504 )
	ROM_LOAD16_BYTE( "lday15.bin",   0x00001, 0x20000, 0xcd990442 )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD16_BYTE( "e6",           0x00000, 0x20000, 0x7623c443 )
	ROM_LOAD16_BYTE( "e9",           0x00001, 0x20000, 0x717f6a0e )
	ROM_LOAD16_BYTE( "lday7.bin",    0x40000, 0x20000, 0x6e57a888 )
	ROM_LOAD16_BYTE( "lday10.bin",   0x40001, 0x20000, 0xa5548dca )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD16_BYTE( "lday12.bin",   0x00000, 0x20000, 0x992bc4af )
	ROM_LOAD16_BYTE( "lday14.bin",   0x00001, 0x20000, 0xa79abc85 )

	ROM_REGION( 0x20000, REGION_GFX5, 0 )	/* bg tilemaps */
	ROM_LOAD16_BYTE( "e5",           0x00000, 0x10000, 0x5f801410 )
	ROM_LOAD16_BYTE( "e8",           0x00001, 0x10000, 0xa7b8250b )

	ROM_REGION( 0x20000, REGION_GFX6, 0 )	/* fg tilemaps */
	ROM_LOAD16_BYTE( "lday11.bin",   0x00000, 0x10000, 0x04b961de )
	ROM_LOAD16_BYTE( "lday13.bin",   0x00001, 0x10000, 0x6bdbd887 )
ROM_END

ROM_START( pollux )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for banks */
	ROM_LOAD( "pollux2.bin",  0x00000, 0x10000, 0x45e10d4e )
	ROM_RELOAD(               0x10000, 0x10000 )	/* banked at 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "pollux3.bin",  0x00000, 0x10000, 0x85a9dc98 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "pollux1.bin",  0x08000, 0x08000, 0x7f7135da )
	ROM_CONTINUE(             0x00000, 0x08000 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_WORD_SWAP( "polluxm2.bin", 0x00000, 0x80000, 0xbdea6f7d )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD16_WORD_SWAP( "polluxm1.bin", 0x00000, 0x80000, 0x1d2dedd2 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD16_BYTE( "pollux6.bin",  0x00000, 0x20000, 0xb0391db5 )
	ROM_LOAD16_BYTE( "pollux7.bin",  0x00001, 0x20000, 0x632f6e10 )
	ROM_FILL(                        0x40000, 0x40000, 0xff )

	ROM_REGION( 0x20000, REGION_GFX5, 0 )	/* bg tilemaps */
	ROM_LOAD16_BYTE( "pollux9.bin",  0x00000, 0x10000, 0x378d8914 )
	ROM_LOAD16_BYTE( "pollux8.bin",  0x00001, 0x10000, 0x8859fa70 )

	ROM_REGION( 0x20000, REGION_GFX6, 0 )	/* fg tilemaps */
	ROM_LOAD16_BYTE( "pollux5.bin",  0x00000, 0x10000, 0xac090d34 )
	ROM_LOAD16_BYTE( "pollux4.bin",  0x00001, 0x10000, 0x2c6bd3be )
ROM_END

ROM_START( bluehawk )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for banks */
	ROM_LOAD( "rom19",        0x00000, 0x20000, 0x24149246 )
	ROM_RELOAD(               0x10000, 0x20000 )	/* banked at 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "rom1",         0x00000, 0x10000, 0xeef22920 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "rom3",         0x00000, 0x10000, 0xc192683f )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD16_WORD_SWAP( "dy-bh-m3",     0x00000, 0x80000, 0x8809d157 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_WORD_SWAP( "dy-bh-m1",     0x00000, 0x80000, 0x51816b2c )

	ROM_REGION( 0x80000, REGION_GFX4, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_WORD_SWAP( "dy-bh-m2",     0x00000, 0x80000, 0xf9daace6 )

	ROM_REGION( 0x40000, REGION_GFX5, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "rom6",         0x00000, 0x20000, 0xe6bd9daa )
	ROM_LOAD16_BYTE( "rom5",         0x00001, 0x20000, 0x5c654dc6 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* OKI6295 samples */
	ROM_LOAD( "rom4",         0x00000, 0x20000, 0xf7318919 )
ROM_END

ROM_START( sadari )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for banks */
	ROM_LOAD( "1.3d",         0x00000, 0x20000, 0xbd953217 )
	ROM_RELOAD(               0x10000, 0x20000 )				/* banked at 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "3.6r",         0x0000, 0x10000, 0x4786fca6 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "2.4c",         0x0000, 0x20000, 0xb2a3f1c6 )

	/* no sprites */

	ROM_REGION( 0x80000, REGION_GFX2, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "10.10l",       0x00000, 0x20000, 0x70269ab1 )
	ROM_LOAD16_BYTE( "5.8l",         0x00001, 0x20000, 0xceceb4c3 )
	ROM_LOAD16_BYTE( "9.10n",        0x40000, 0x20000, 0x21bd1bda )
	ROM_LOAD16_BYTE( "4.8n",         0x40001, 0x20000, 0xcd318ae5 )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "11.10j",       0x00000, 0x20000, 0x62a1d580 )
	ROM_LOAD16_BYTE( "6.8j",         0x00001, 0x20000, 0xc4b13ed7 )
	ROM_LOAD16_BYTE( "12.10g",       0x40000, 0x20000, 0x547b7645 )
	ROM_LOAD16_BYTE( "7.8g",         0x40001, 0x20000, 0x14f20fa3 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* OKI6295 samples */
	ROM_LOAD( "8.10r",        0x00000, 0x20000, 0x9c29a093 )
ROM_END

ROM_START( primella )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for banks */
	ROM_LOAD( "1_d3.bin",     0x00000, 0x20000, 0x82fea4e0 )
	ROM_RELOAD(               0x10000, 0x20000 )				/* banked at 0x8000-0xbfff */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "3_r6.bin",     0x0000, 0x10000, 0xea41c4ad )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "2_c5.bin",     0x0000, 0x20000, 0x8575e64b )

	/* no sprites */

	ROM_REGION( 0x40000, REGION_GFX2, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "7_n9.bin",     0x00000, 0x20000, 0x20b6a574 )
	ROM_LOAD16_BYTE( "4_n7.bin",     0x00001, 0x20000, 0xfe593666 )

	ROM_REGION( 0x40000, REGION_GFX3, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "8_g9.bin",     0x00000, 0x20000, 0x542ecb83 )
	ROM_LOAD16_BYTE( "5_g7.bin",     0x00001, 0x20000, 0x058ecac6 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* OKI6295 samples */
	ROM_LOAD( "6_r9.bin",     0x00000, 0x20000, 0xf92e5803 )
ROM_END

ROM_START( rshark )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 64k for code + 128k for banks */
	ROM_LOAD16_BYTE( "rspl00.bin",   0x00000, 0x20000, 0x40356b9d )
	ROM_LOAD16_BYTE( "rspu00.bin",   0x00001, 0x20000, 0x6635c668 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "rse3.bin",     0x0000, 0x10000, 0x03c8fd17 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprite */
	ROM_LOAD16_BYTE( "rse4.bin",     0x000000, 0x80000, 0xb857e411 )
	ROM_LOAD16_BYTE( "rse5.bin",     0x000001, 0x80000, 0x7822d77a )
	ROM_LOAD16_BYTE( "rse6.bin",     0x100000, 0x80000, 0x80215c52 )
	ROM_LOAD16_BYTE( "rse7.bin",     0x100001, 0x80000, 0xbd28bbdc )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "rse11.bin",    0x00000, 0x80000, 0x8a0c572f )
	ROM_LOAD16_BYTE( "rse10.bin",    0x00001, 0x80000, 0x139d5947 )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "rse15.bin",    0x00000, 0x80000, 0xd188134d )
	ROM_LOAD16_BYTE( "rse14.bin",    0x00001, 0x80000, 0x0ef637a7 )

	ROM_REGION( 0x100000, REGION_GFX4, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "rse17.bin",    0x00000, 0x80000, 0x7ff0f3c7 )
	ROM_LOAD16_BYTE( "rse16.bin",    0x00001, 0x80000, 0xc176c8bc )

	ROM_REGION( 0x100000, REGION_GFX5, 0 )	/* tiles + tilemaps (together!) */
	ROM_LOAD16_BYTE( "rse21.bin",    0x00000, 0x80000, 0x2ea665af )
	ROM_LOAD16_BYTE( "rse20.bin",    0x00001, 0x80000, 0xef93e3ac )

	ROM_REGION( 0x80000, REGION_GFX6, 0 )	/* top 4 bits of tilemaps */
	ROM_LOAD( "rse12.bin",    0x00000, 0x20000, 0xfadbf947 )
	ROM_LOAD( "rse13.bin",    0x20000, 0x20000, 0x323d4df6 )
	ROM_LOAD( "rse18.bin",    0x40000, 0x20000, 0xe00c9171 )
	ROM_LOAD( "rse19.bin",    0x60000, 0x20000, 0xd214d1d0 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKI6295 samples */
	ROM_LOAD( "rse1.bin",     0x00000, 0x20000, 0x0291166f )
	ROM_LOAD( "rse2.bin",     0x20000, 0x20000, 0x5a26ee72 )
ROM_END



/* The differences between the two lastday sets are only in the sound program
   and graphics. The main program is the same. */
GAME( 1990, lastday,  0,       lastday,  lastday,  0, ROT270_16BIT, "Dooyong", "The Last Day (set 1)" )
GAME( 1990, lastdaya, lastday, lastday,  lastday,  0, ROT270_16BIT, "Dooyong", "The Last Day (set 2)" )
GAME( 1991, pollux,   0,       pollux,   pollux,   0, ROT270_16BIT, "Dooyong", "Pollux" )
GAME( 1993, bluehawk, 0,       bluehawk, bluehawk, 0, ROT270_16BIT, "Dooyong", "Blue Hawk" )
GAME( 1993, sadari,   0,       primella, primella, 0, ROT0_16BIT,   "[Dooyong] (NTC license)", "Sadari" )
GAME( 1994, primella, 0,       primella, primella, 0, ROT0_16BIT,   "[Dooyong] (NTC license)", "Primella" )
GAME( 1995, rshark,   0,       rshark,   rshark,   0, ROT270_16BIT, "Dooyong", "R-Shark" )
