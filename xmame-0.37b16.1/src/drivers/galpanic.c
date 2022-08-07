/***************************************************************************

Gals Panic       1990 Kaneko
Fantasia         1994 Comad
New Fantasia     1995 Comad
Miss World '96   1996 Comad

driver by Nicola Salmoria

The Comad games run on hardware similar to Gals Panic, with a different
sprite system. They are not ROM swaps because the addresses of work RAM
and of the OKI chip change from one to the other, however everything else
is pretty much identical.


TODO:
- There is a vector for IRQ4. The function does nothing in galpanic but is
  more complicated in the Comad ones. However I'm not triggering it, and
  they seems to work anyway...
- Four unknown ROMs in fantasia. The game seems to work fine without them.
- There was a ROM in the newfant set, obj2_14.rom, which was identical to
  Terminator 2's t2.107. I can only assume this was a mistake of the dumper.
- lots of unknown reads and writes, also in galpanic but particularly in
  the Comad ones.
- fantasia and newfant have a service mode but they doesn't work well (text
  is missing or replaced by garbage). This might be just how the games are.
- Is there a background enable register? Or a background clear. fantasia and
  newfant certainly look ugly as they are.

Notes about Gals Panic:
-----------------------
The current ROM set is strange because two ROMs overlap two others replacing
the program.

It's definitely a Kaneko boardset, but it could very well be they converted
some other game to run Gals Panic, because there's some ROMs piggybacked
on top of each other and some ROMs on a daughterboard plugged into smaller
sized ROM sockets. It's not a pirate version. The piggybacked ROMs even have
Kaneko stickers. The silkscreen on the board says PAMERA-4.

There is at least another version of the Gals Panic board. It's single board,
so no daughterboard. There are only 4 IC's socketed, the rest is soldered to
the board, and no piggybacked ROMs. Board number is MDK 321 V-0    EXPRO-02

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern data16_t *galpanic_bgvideoram,*galpanic_fgvideoram;
extern size_t galpanic_fgvideoram_size;

void galpanic_init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom);
WRITE16_HANDLER( galpanic_bgvideoram_w );
WRITE16_HANDLER( galpanic_paletteram_w );
void galpanic_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void comad_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);




static int galpanic_interrupt(void)
{
	/* IRQ 3 drives the game, IRQ 5 updates the palette */
	if (cpu_getiloops() != 0) return 5;
	else return 3;
}

static WRITE16_HANDLER( galpanic_6295_bankswitch_w )
{
	if (ACCESSING_MSB)
	{
		UINT8 *rom = memory_region(REGION_SOUND1);

		memcpy(&rom[0x30000],&rom[0x40000 + ((data >> 8) & 0x0f) * 0x10000],0x10000);
	}
}



static MEMORY_READ16_START( galpanic_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_status_0_lsb_r },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x7047ff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
MEMORY_END

static MEMORY_WRITE16_START( galpanic_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },
	{ 0x400000, 0x400001, OKIM6295_data_0_lsb_w },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x7047ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* ??? */
	{ 0xb00000, 0xb00001, MWA16_NOP },	/* ??? */
	{ 0xc00000, 0xc00001, MWA16_NOP },	/* ??? */
MEMORY_END

static READ16_HANDLER( kludge )
{
	return rand() & 0x0700;
}

static MEMORY_READ16_START( comad_readmem )
	{ 0x000000, 0x4fffff, MRA16_ROM },
	{ 0x500000, 0x51ffff, MRA16_RAM },
	{ 0x520000, 0x53ffff, MRA16_RAM },
	{ 0x600000, 0x6007ff, MRA16_RAM },
	{ 0x700000, 0x700fff, MRA16_RAM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x800004, 0x800005, input_port_2_word_r },
/*	{ 0x800006, 0x800007,  },	?? */
	{ 0x80000a, 0x80000b, kludge },	/* bits 8-a = timer? palette update code waits for them to be 111 */
	{ 0x80000c, 0x80000d, kludge },	/* missw96 bits 8-a = timer? palette update code waits for them to be 111 */
	{ 0xc00000, 0xc0ffff, MRA16_RAM },	/* missw96 */
	{ 0xc80000, 0xc8ffff, MRA16_RAM },	/* fantasia, newfant */
	{ 0xf00000, 0xf00001, OKIM6295_status_0_msb_r },	/* fantasia, missw96 */
	{ 0xf80000, 0xf80001, OKIM6295_status_0_msb_r },	/* newfant */
MEMORY_END

static MEMORY_WRITE16_START( comad_writemem )
	{ 0x000000, 0x4fffff, MWA16_ROM },
	{ 0x500000, 0x51ffff, MWA16_RAM, &galpanic_fgvideoram, &galpanic_fgvideoram_size },
	{ 0x520000, 0x53ffff, galpanic_bgvideoram_w, &galpanic_bgvideoram },	/* + work RAM */
	{ 0x600000, 0x6007ff, galpanic_paletteram_w, &paletteram16 },	/* 1024 colors, but only 512 seem to be used */
	{ 0x700000, 0x700fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x900000, 0x900001, galpanic_6295_bankswitch_w },	/* not sure */
	{ 0xc00000, 0xc0ffff, MWA16_RAM },	/* missw96 */
	{ 0xc80000, 0xc8ffff, MWA16_RAM },	/* fantasia, newfant */
	{ 0xf00000, 0xf00001, OKIM6295_data_0_msb_w },	/* fantasia, missw96 */
	{ 0xf80000, 0xf80001, OKIM6295_data_0_msb_w },	/* newfant */
MEMORY_END



INPUT_PORTS_START( galpanic )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )	/* flip screen? */
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )	/* might affect coinage according to manual, */
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )		/* but settings below don't match */
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )/* BUTTON2 | IPF_PLAYER1 ) used in test mode */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, "Difficulty?" )
	PORT_DIPSETTING(      0x0002, "Easy?" )
	PORT_DIPSETTING(      0x0003, "Normal?" )
	PORT_DIPSETTING(      0x0001, "Hard?" )
	PORT_DIPSETTING(      0x0000, "Hardest?" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0010, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* manual says demo sounds but has no effect */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Test Mode" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )/*BUTTON2 | IPF_PLAYER2 ) */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( newfant )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* in service mode, but doesn't add credits */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( missw96 )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )	/* MAME crashes when pressed */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			64*4, 65*4, 66*4, 67*4, 68*4, 69*4, 70*4, 71*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout,  256, 16 },
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 12000 },          /* 12000Hz frequency */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }
};


#define MACHINEDRIVER(NAME,CLOCK,SCREENREFRESH)								\
static struct MachineDriver machine_driver_##NAME =							\
{																			\
	/* basic machine hardware */											\
	{																		\
		{																	\
			CPU_M68000,														\
			CLOCK,															\
			NAME##_readmem,NAME##_writemem,0,0,								\
			galpanic_interrupt,2											\
		}																	\
	},																		\
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */	\
	1,	/* single CPU, no need for interleaving */							\
	0,																		\
																			\
	/* video hardware */													\
	256, 256, { 0, 256-1, 0, 224-1 },										\
	gfxdecodeinfo,															\
	1024 + 32768, 1024,														\
	galpanic_init_palette,													\
																			\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,								\
	0,																		\
	generic_bitmapped_vh_start,												\
	generic_bitmapped_vh_stop,												\
	SCREENREFRESH##_vh_screenrefresh,										\
																			\
	/* sound hardware */													\
	0,0,0,0,																\
	{																		\
		{																	\
			SOUND_OKIM6295,													\
			&okim6295_interface												\
		}																	\
	}																		\
};

MACHINEDRIVER(galpanic, 8000000,galpanic)	/*  8 MHz ??? */
MACHINEDRIVER(comad,   10000000,comad)		/* 10 MHz ??? */



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( galpanic )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "pm110.4m2",    0x000000, 0x080000, 0xae6b17a8 )
	ROM_LOAD16_BYTE( "pm109.4m1",    0x000001, 0x080000, 0xb85d792d )
	/* The above two ROMs contain valid 68000 code, but the game doesn't */
	/* work. I think there might be a protection (addressed at e00000). */
	/* The two following ROMs replace the code with a working version. */
	ROM_LOAD16_BYTE( "pm112.6",      0x000000, 0x20000, 0x7b972b58 )
	ROM_LOAD16_BYTE( "pm111.5",      0x000001, 0x20000, 0x4eb7298d )
	ROM_LOAD16_BYTE( "pm004e.8",     0x100001, 0x80000, 0xd3af52bc )
	ROM_LOAD16_BYTE( "pm005e.7",     0x100000, 0x80000, 0xd7ec650c )
	ROM_LOAD16_BYTE( "pm000e.15",    0x200001, 0x80000, 0x5d220f3f )
	ROM_LOAD16_BYTE( "pm001e.14",    0x200000, 0x80000, 0x90433eb1 )
	ROM_LOAD16_BYTE( "pm002e.17",    0x300001, 0x80000, 0x713ee898 )
	ROM_LOAD16_BYTE( "pm003e.16",    0x300000, 0x80000, 0x6bb060fd )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "pm006e.67",    0x000000, 0x100000, 0x57aec037 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "pm008e.l",     0x00000, 0x80000, 0xd9379ba8 )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "pm007e.u",     0xc0000, 0x80000, 0xc7ed7950 )
ROM_END

ROM_START( fantasia )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2_16.rom", 0x000000, 0x80000, 0xe27c6c57 )
	ROM_LOAD16_BYTE( "prog1_13.rom", 0x000001, 0x80000, 0x68d27413 )
	ROM_LOAD16_BYTE( "iscr6_09.rom", 0x100000, 0x80000, 0x2a588393 )
	ROM_LOAD16_BYTE( "iscr5_05.rom", 0x100001, 0x80000, 0x6160e0f0 )
	ROM_LOAD16_BYTE( "iscr4_08.rom", 0x200000, 0x80000, 0xf776b743 )
	ROM_LOAD16_BYTE( "iscr3_04.rom", 0x200001, 0x80000, 0x5df0dff2 )
	ROM_LOAD16_BYTE( "iscr2_07.rom", 0x300000, 0x80000, 0x5707d861 )
	ROM_LOAD16_BYTE( "iscr1_03.rom", 0x300001, 0x80000, 0x36cb811a )
	ROM_LOAD16_BYTE( "imag2_10.rom", 0x400000, 0x80000, 0x1f14a395 )
	ROM_LOAD16_BYTE( "imag1_06.rom", 0x400001, 0x80000, 0xfaf870e4 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1_17.rom",  0x00000, 0x80000, 0xaadb6eb7 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "mus-1_01.rom", 0x00000, 0x80000, 0x22955efb )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "mus-2_02.rom", 0xc0000, 0x80000, 0x4cd4d6c3 )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )	/* unknown */
	ROM_LOAD16_BYTE( "gscr2_15.rom", 0x000000, 0x80000, 0x46666768 )
	ROM_LOAD16_BYTE( "gscr1_12.rom", 0x000001, 0x80000, 0x4bd25be6 )
	ROM_LOAD16_BYTE( "gscr4_14.rom", 0x100000, 0x80000, 0x4e7e6ed4 )
	ROM_LOAD16_BYTE( "gscr3_11.rom", 0x100001, 0x80000, 0x6d00a4c5 )
ROM_END

ROM_START( newfant )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "prog2_12.rom", 0x000000, 0x80000, 0xde43a457 )
	ROM_LOAD16_BYTE( "prog1_07.rom", 0x000001, 0x80000, 0x370b45be )
	ROM_LOAD16_BYTE( "iscr2_10.rom", 0x100000, 0x80000, 0x4f2da2eb )
	ROM_LOAD16_BYTE( "iscr1_05.rom", 0x100001, 0x80000, 0x63c6894f )
	ROM_LOAD16_BYTE( "iscr4_09.rom", 0x200000, 0x80000, 0x725741ec )
	ROM_LOAD16_BYTE( "iscr3_04.rom", 0x200001, 0x80000, 0x51d6b362 )
	ROM_LOAD16_BYTE( "iscr6_08.rom", 0x300000, 0x80000, 0x178b2ef3 )
	ROM_LOAD16_BYTE( "iscr5_03.rom", 0x300001, 0x80000, 0xd2b5c5fa )
	ROM_LOAD16_BYTE( "iscr8_11.rom", 0x400000, 0x80000, 0xf4148528 )
	ROM_LOAD16_BYTE( "iscr7_06.rom", 0x400001, 0x80000, 0x2dee0c31 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "obj1_13.rom",  0x00000, 0x80000, 0xe6d1bc71 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "musc1_01.rom", 0x00000, 0x80000, 0x10347fce )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "musc2_02.rom", 0xc0000, 0x80000, 0xb9646a8c )
ROM_END

ROM_START( missw96 )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "mw96_10.bin",  0x000000, 0x80000, 0xb1309bb1 )
	ROM_LOAD16_BYTE( "mw96_06.bin",  0x000001, 0x80000, 0xa5892bb3 )
	ROM_LOAD16_BYTE( "mw96_09.bin",  0x100000, 0x80000, 0x7032dfdf )
	ROM_LOAD16_BYTE( "mw96_05.bin",  0x100001, 0x80000, 0x91de5ab5 )
	ROM_LOAD16_BYTE( "mw96_08.bin",  0x200000, 0x80000, 0xb8e66fb5 )
	ROM_LOAD16_BYTE( "mw96_04.bin",  0x200001, 0x80000, 0xe77a04f8 )
	ROM_LOAD16_BYTE( "mw96_07.bin",  0x300000, 0x80000, 0x26112ed3 )
	ROM_LOAD16_BYTE( "mw96_03.bin",  0x300001, 0x80000, 0xe9374a46 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "mw96_11.bin",  0x00000, 0x80000, 0x3983152f )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	/* 00000-2ffff is fixed, 30000-3ffff is bank switched from all the ROMs */
	ROM_LOAD( "mw96_01.bin",  0x00000, 0x80000, 0xe78a659e )
	ROM_RELOAD(               0x40000, 0x80000 )
	ROM_LOAD( "mw96_02.bin",  0xc0000, 0x80000, 0x60fa0c00 )
ROM_END


GAMEX( 1990, galpanic, 0, galpanic, galpanic, 0, ROT90_16BIT, "Kaneko", "Gals Panic", GAME_NO_COCKTAIL )
GAMEX( 1994, fantasia, 0, comad,    newfant,  0, ROT90_16BIT, "Comad & New Japan System", "Fantasia", GAME_NO_COCKTAIL )
GAMEX( 1995, newfant,  0, comad,    newfant,  0, ROT90_16BIT, "Comad & New Japan System", "New Fantasia", GAME_NO_COCKTAIL )
GAMEX( 1996, missw96,  0, comad,    missw96,  0, ROT0_16BIT,  "Comad", "Miss World '96 Nude", GAME_NO_COCKTAIL )
