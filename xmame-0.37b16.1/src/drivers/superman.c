/***************************************************************************


Taito X-system

driver by Richard Bush, Howie Cohen and Yochizo


Supported games:
----------------------------------------------------
 Name                    Company               Year
  Superman                Taito Corp.           1988
  Daisenpu (Japan)        Taito Corp.           1988
  Balloon Brothers        East Technology Corp. 1990
  Gigandes                East Technology Corp. 1990

Please tell me the games worked on this board.


Memory map:
----------------------------------------------------
  0x000000 - 0x07ffff : ROM
  0x300000   ??
  0x400000   ??
  0x500000 - 0x50000f : Dipswitches a & b, 4 bits to each word
  0x600000   ?? 0, 10, 0x4001, 0x4006
  0x700000   ??
  0x800000 - 0x800003 : sound chip
  0x900000 - 0x900fff : c-chip shared RAM space
  0xb00000 - 0xb00fff : palette RAM, words in the format xRRRRRGGGGGBBBBB
  0xc00000   ??
  0xd00000 - 0xd007ff : video attribute RAM
      0000 - 03ff : sprite y coordinate
      0400 - 07ff : tile x & y scroll
  0xe00000 - 0xe00fff : object RAM bank 1
      0000 - 03ff : sprite number (bit mask 0x3fff)
                    sprite y flip (bit mask 0x4000)
                    sprite x flip (bit mask 0x8000)
      0400 - 07ff : sprite x coordinate (bit mask 0x1ff)
                    sprite color (bit mask 0xf800)
      0800 - 0bff : tile number (bit mask 0x3fff)
                    tile y flip (bit mask 0x4000)
                    tile x flip (bit mask 0x8000)
      0c00 - 0fff : tile color (bit mask 0xf800)
  0xe01000 - 0xe01fff : unused(?) portion of object RAM
  0xe02000 - 0xe02fff : object RAM bank 2
  0xe03000 - 0xe03fff : unused(?) portion of object RAM

Interrupt:
----------------------------------------------------
  IRQ level 6 : Superman
  IRQ level 2 : Daisenpu, Balloon Brothers, Gigandes

Screen resolution:
----------------------------------------------------
  384 x 240   : Superman, Balloon Brothers, Gigandes
  384 x 224   : Daisenpu

Sound chip:
----------------------------------------------------
  YM2610 x 1   : Superman, Balloon Brothers, Gigandes
  YM2151 x 1   : Daisenpu


Gigandes
--------

  - Gigandes has background tile glitches on the demo of cave
    stage (the last one before it does demo of level 1 again).
    It seems to be rapidly switching between two different
    background layers. This may be because the game has put
    different tiles in bank 0 and bank 1 (which alternate each
    frame). The other strangeness is that the background is not
    scrolling. So chances are the graphics chip emulation is
    flawed.

    When this cuts to hiscore screen there is flickering white
    tilemap garbage beneath. These leftover tiles have not been
    cleared from $e00800-bff (but they have been from $e02800).
    So alternate 2 frames display the bank with garbage tiles and
    the one without.

    Probably some control register should be keeping the graphics
    chip in the cleared bank, so the garbage is never visible.
    Separate bank control for sprite / tile layers ?
    Maybe this effect is also desired during the cave stage.


TODO
----

  - Fix known problems
  - Any other games that worked on this board?
  - What is correct date for Ballbros? Two different ones
    appear in this driver!


Dumpers Notes
-------------

Details of custom chip numbers on Superman would be welcome.

Daisenpu
--------

(c)1989 Toaplan/Taito
Taito X System

K1100443A MAIN PCB
J1100188A X SYSTEM
P0-051A

CPU  : 68000 (Toshiba TMP68000N-8)
Sound: Z80 (Sharp LH0080A)
       YM2151+YM3012
OSC  : 16.000MHz

B87-01 (Mask ROMs, read as 27C4200)
B87-02
B87-03
B87-04
(These 4 images could be wrong)

B87-05 (M5M27C101K) - Main PRG
B87-06 (M5M27C101K) - Main PRG
B87-07 (27C256) - Sound PRG

This board uses SETA's custom chips.
X1-001A, X1-002A, X1-004, X1-006, X1-007.
Arkanoid II uses X1-001, X1-002, X1-003, X1-004.

Control: 8-way Joystick + 2-buttons


Gigandes
--------

East Technology 1989

P0-057A

           68000-8    1  2  51832  3  4  51832

6264                        16MHz
5
10                                       8
11       TC0140SYT     X1-001A X1-002A   7
                                         9
YM2610                                   6
        Z80A
                     X1-004          X1-006
                                     X1-007


Balloon Brothers
----------------

East Technology 1992

    68000-8    10A   51832           5A 51832

 6264                  2063
 8D                    2063                 SWB  SWA
 EAST-10                    16MHz
 EAST-11    Taito TC0140SYT
 YM2610                                             3
          Z80A         SETA X1-001A SETA X1-002A

                 SETA X1-004                        2
                                                    1
                                                    0


C-Chip notes
------------

Superman seems to be the only game of the four with a c-chip. Daisenpu
appears to use a simple input device with coin counter and lockout in
its place. The East Technology games on this hardware follow Daisenpu.

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/taitosnd.h"


void superman_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
int superman_vh_start (void);
int ballbros_vh_start (void);
void superman_vh_stop (void);

extern size_t supes_videoram_size;
extern size_t supes_attribram_size;
extern data16_t *supes_videoram;
extern data16_t *supes_attribram;

void cchip1_init_machine(void);
READ16_HANDLER ( cchip1_word_r );
WRITE16_HANDLER( cchip1_word_w );



READ16_HANDLER( superman_dsw_input_r )
{
	switch (offset)
	{
		case 0x00:
			return  readinputport (0) & 0x0f;
		case 0x01:
			return (readinputport (0) & 0xf0) >> 4;
		case 0x02:
			return  readinputport (1) & 0x0f;
		case 0x03:
			return (readinputport (1) & 0xf0) >> 4;
		default:
			logerror("taitox unknown dsw read offset: %04x\n", offset);
			return 0x00;
	}
}

static READ16_HANDLER( daisenpu_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(2);
		case 0x01:
			return readinputport(3);
		case 0x02:
			return readinputport(4);

		default:
			logerror("taitox unknown input read offset: %04x\n", offset);
			return 0x00;
	}
}

static WRITE16_HANDLER( daisenpu_input_w )
{
	switch (offset)
	{
		case 0x04:	/* coin counters and lockout */
			coin_counter_w(0,data & 0x01);
			coin_counter_w(1,data & 0x02);
			coin_lockout_w(0,~data & 0x04);
			coin_lockout_w(1,~data & 0x08);
/*logerror("taitox coin control %04x to offset %04x\n",data,offset); */
			break;

		default:
			logerror("taitox unknown input write %04x to offset %04x\n",data,offset);
	}
}


/**************************************************************************/

static int banknum = -1;

static void reset_sound_region(void)
{
	cpu_setbank( 2, memory_region(REGION_CPU2) + (banknum * 0x4000) + 0x10000 );
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	banknum = (data - 1) & 3;
	reset_sound_region();
}


/**************************************************************************/

static MEMORY_READ16_START( superman_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x500000, 0x500007, superman_dsw_input_r  },
	{ 0x800000, 0x800001, MRA16_NOP },
	{ 0x800002, 0x800003, taitosound_comm16_lsb_r },
	{ 0x900000, 0x900fff, cchip1_word_r } ,
	{ 0xb00000, 0xb00fff, paletteram16_word_r },
	{ 0xd00000, 0xd007ff, MRA16_RAM },	/* video attribute ram */
	{ 0xe00000, 0xe03fff, MRA16_RAM },	/* object ram */
	{ 0xf00000, 0xf03fff, MRA16_RAM },	/* Main RAM */
MEMORY_END

static MEMORY_WRITE16_START( superman_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x300000, 0x300001, MWA16_NOP },	/* written each frame at $3a9c, mostly 0x10 */
	{ 0x400000, 0x400001, MWA16_NOP },	/* written each frame at $3aa2, mostly 0x10 */
	{ 0x600000, 0x600001, MWA16_NOP },	/* written each frame at $3ab0, mostly 0x10 */
	{ 0x800000, 0x800001, taitosound_port16_lsb_w },
	{ 0x800002, 0x800003, taitosound_comm16_lsb_w },
	{ 0x900000, 0x900fff, cchip1_word_w },
	{ 0xb00000, 0xb00fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0xd00000, 0xd007ff, MWA16_RAM, &supes_attribram, &supes_attribram_size },
	{ 0xe00000, 0xe03fff, MWA16_RAM, &supes_videoram, &supes_videoram_size },
	{ 0xf00000, 0xf03fff, MWA16_RAM },			/* Main RAM */
MEMORY_END

static MEMORY_READ16_START( daisenpu_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x500000, 0x50000f, superman_dsw_input_r },
	{ 0x800000, 0x800001, MRA16_NOP },
	{ 0x800002, 0x800003, taitosound_comm16_lsb_r },
	{ 0x900000, 0x90000f, daisenpu_input_r },
	{ 0xb00000, 0xb00fff, paletteram16_word_r },
	{ 0xd00000, 0xd00fff, MRA16_RAM },	/* video attribute ram */
	{ 0xe00000, 0xe03fff, MRA16_RAM },	/* object ram */
	{ 0xf00000, 0xf03fff, MRA16_RAM },	/* Main RAM */
MEMORY_END

static MEMORY_WRITE16_START( daisenpu_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
/*	{ 0x400000, 0x400001, MWA16_NOP },	   written each frame at $2ac, values change    */
/*	{ 0x600000, 0x600001, MWA16_NOP },	   written each frame at $2a2, values change    */
	{ 0x800000, 0x800001, taitosound_port16_lsb_w },
	{ 0x800002, 0x800003, taitosound_comm16_lsb_w },
	{ 0x900000, 0x90000f, daisenpu_input_w },
	{ 0xb00000, 0xb00fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0xd00000, 0xd00fff, MWA16_RAM, &supes_attribram, &supes_attribram_size },
	{ 0xe00000, 0xe03fff, MWA16_RAM, &supes_videoram, &supes_videoram_size },
	{ 0xf00000, 0xf03fff, MWA16_RAM },			/* Main RAM */
MEMORY_END

static MEMORY_READ16_START( gigandes_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x500000, 0x500007, superman_dsw_input_r },
	{ 0x800000, 0x800001, MRA16_NOP },
	{ 0x800002, 0x800003, taitosound_comm16_lsb_r },
	{ 0x900000, 0x90000f, daisenpu_input_r },
	{ 0xb00000, 0xb00fff, paletteram16_word_r },
	{ 0xd00000, 0xd007ff, MRA16_RAM },	/* video attribute ram */
	{ 0xe00000, 0xe03fff, MRA16_RAM },	/* object ram */
	{ 0xf00000, 0xf03fff, MRA16_RAM },	/* Main RAM */
MEMORY_END

static MEMORY_WRITE16_START( gigandes_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x400000, 0x400001, MWA16_NOP },	/* 0x1 written each frame at $d42, watchdog? */
	{ 0x600000, 0x600001, MWA16_NOP },	/* 0x1 written each frame at $d3c, watchdog? */
	{ 0x800000, 0x800001, taitosound_port16_lsb_w },
	{ 0x800002, 0x800003, taitosound_comm16_lsb_w },
	{ 0x900000, 0x90000f, daisenpu_input_w },
	{ 0xb00000, 0xb00fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0xd00000, 0xd007ff, MWA16_RAM, &supes_attribram, &supes_attribram_size },
	{ 0xe00000, 0xe03fff, MWA16_RAM, &supes_videoram, &supes_videoram_size },
	{ 0xf00000, 0xf03fff, MWA16_RAM },			/* Main RAM */
MEMORY_END

static MEMORY_READ16_START( ballbros_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x500000, 0x50000f, superman_dsw_input_r },
	{ 0x800000, 0x800001, MRA16_NOP },
	{ 0x800002, 0x800003, taitosound_comm16_lsb_r },
	{ 0x900000, 0x90000f, daisenpu_input_r },
	{ 0xb00000, 0xb00fff, paletteram16_word_r },
	{ 0xd00000, 0xd007ff, MRA16_RAM },	/* video attribute ram */
	{ 0xe00000, 0xe03fff, MRA16_RAM },	/* object ram */
	{ 0xf00000, 0xf03fff, MRA16_RAM },	/* Main RAM */
MEMORY_END

static MEMORY_WRITE16_START( ballbros_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x400000, 0x400001, MWA16_NOP },	/* 0x1 written each frame at $c56, watchdog? */
	{ 0x600000, 0x600001, MWA16_NOP },	/* 0x1 written each frame at $c4e, watchdog? */
	{ 0x800000, 0x800001, taitosound_port16_lsb_w },
	{ 0x800002, 0x800003, taitosound_comm16_lsb_w },
	{ 0x900000, 0x90000f, daisenpu_input_w },
	{ 0xb00000, 0xb00fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0xd00000, 0xd007ff, MWA16_RAM, &supes_attribram, &supes_attribram_size },
	{ 0xe00000, 0xe03fff, MWA16_RAM, &supes_videoram, &supes_videoram_size },
	{ 0xf00000, 0xf03fff, MWA16_RAM },			/* Main RAM */
MEMORY_END


/**************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w }, /* bankswitch ? */
MEMORY_END

static MEMORY_READ_START( daisenpu_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe001, YM2151_status_port_0_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( daisenpu_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END

static MEMORY_READ_START( ballbros_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe003, 0xe003, YM2610_read_port_0_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( ballbros_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w }, /* bankswitch ? */
MEMORY_END


/**************************************************************************/

INPUT_PORTS_START( superman )
	PORT_START /* DSW A / DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW C / DSW D */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "50k and every 150k" )
	PORT_DIPSETTING(    0x04, "Bonus 2??" )
	PORT_DIPSETTING(    0x08, "Bonus 3??" )
	PORT_DIPSETTING(    0x00, "Bonus 4??" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( daisenpu )
	PORT_START /* DSW A / DSW B */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW C / DSW D */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "70k, 270k and 470k" )
	PORT_DIPSETTING(    0x08, "50k, 200k and 350k" )
	PORT_DIPSETTING(    0x04, "100k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( gigandes )
	PORT_START /* DSW A / DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* DSW C / DSW D */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ballbros )
	PORT_START /* DSW A / DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* DSW C / DSW D */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "Japanese" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/**************************************************************************/

#define NUM_TILES 16384
static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 sprites */
	NUM_TILES,	/* 16384 of them */
	4,	       /* 4 bits per pixel */
	{ 64*8*NUM_TILES + 8, 64*8*NUM_TILES + 0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*16, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },

	64*8	/* every sprite takes 64 consecutive bytes */
};
#undef NUM_TILES

static struct GfxLayout ballbros_tilelayout =
{
	16,16,  /* 16*16 sprites */
	4096,	/* 4096 of them */
	4,	       /* 4 bits per pixel */
	{ 0x20000*3*8, 0x20000*2*8, 0x20000*1*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*8, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },

	32*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo superman_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tilelayout,    0, 256 },	 /* sprites & playfield */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ballbros_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &ballbros_tilelayout,    0, 256 },	 /* sprites & playfield */
	{ -1 } /* end of array */
};


/**************************************************************************/

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz ?????? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

static struct YM2610interface ballbros_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz ?????? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

static struct YM2151interface ym2151_interface =
{
	1,	/* 1 chip */
	4000000,	/* 4 MHz ?????? */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ irqhandler },
	{ 0 }
};


/**************************************************************************/

static struct MachineDriver machine_driver_superman =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz? */
			superman_readmem,superman_writemem,0,0,
			m68_level6_irq,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	52*8, 32*8, { 2*8, 50*8-1, 2*8, 32*8-1 },

	superman_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	superman_vh_start,
	superman_vh_stop,
	superman_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_daisenpu =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz? */
			daisenpu_readmem,daisenpu_writemem,0,0,
			m68_level2_irq,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			daisenpu_sound_readmem, daisenpu_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	52*8, 32*8, { 2*8, 50*8-1, 3*8, 31*8-1 },

	superman_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	superman_vh_start,
	superman_vh_stop,
	superman_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

static struct MachineDriver machine_driver_gigandes =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz? */
			gigandes_readmem,gigandes_writemem,0,0,
			m68_level2_irq,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			ballbros_sound_readmem, ballbros_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	52*8, 32*8, { 2*8, 50*8-1, 2*8, 32*8-1 },

	superman_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	superman_vh_start,
	superman_vh_stop,
	superman_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ballbros_ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_ballbros =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz? */
			ballbros_readmem,ballbros_writemem,0,0,
			m68_level2_irq,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			ballbros_sound_readmem, ballbros_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	cchip1_init_machine,

	/* video hardware */
	52*8, 32*8, { 2*8, 50*8-1, 2*8, 32*8-1 },

	ballbros_gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	ballbros_vh_start,
	superman_vh_stop,
	superman_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ballbros_ym2610_interface
		}
	}
};





/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( superman )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "a10_09.bin", 0x00000, 0x20000, 0x640f1d58 )
	ROM_LOAD16_BYTE( "a05_07.bin", 0x00001, 0x20000, 0xfddb9953 )
	ROM_LOAD16_BYTE( "a08_08.bin", 0x40000, 0x20000, 0x79fc028e )
	ROM_LOAD16_BYTE( "a03_13.bin", 0x40001, 0x20000, 0x9f446a44 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "d18_10.bin", 0x00000, 0x4000, 0x6efe79e8 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "f01_14.bin", 0x000000, 0x80000, 0x89368c3e ) /* Plane 0, 1 */
	ROM_LOAD( "h01_15.bin", 0x080000, 0x80000, 0x910cc4f9 )
	ROM_LOAD( "j01_16.bin", 0x100000, 0x80000, 0x3622ed2f ) /* Plane 2, 3 */
	ROM_LOAD( "k01_17.bin", 0x180000, 0x80000, 0xc34f27e0 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "e18_01.bin", 0x00000, 0x80000, 0x3cf99786 )
ROM_END

ROM_START( daisenpu )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "b87-06", 0x00000, 0x20000, 0xcf236100 )
	ROM_LOAD16_BYTE( "b87-05", 0x00001, 0x20000, 0x7f15edc7 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )     /* 32k for Z80 code */
	ROM_LOAD( "b87-07", 0x00000, 0x4000, 0xe2e0efa0)
	ROM_CONTINUE(       0x10000, 0x4000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b87-02", 0x000000, 0x80000, 0x89ad43a0 ) /* Plane 0, 1 */
	ROM_LOAD( "b87-01", 0x080000, 0x80000, 0x81e82ae1)
	ROM_LOAD( "b87-04", 0x100000, 0x80000, 0x958434b6 ) /* Plane 2, 3 */
	ROM_LOAD( "b87-03", 0x180000, 0x80000, 0xce155ae0 )
ROM_END

ROM_START( gigandes )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "1", 0x00000, 0x20000, 0x290c50e0 )
	ROM_LOAD16_BYTE( "3", 0x00001, 0x20000, 0x9cef82af )
	ROM_LOAD16_BYTE( "2", 0x40000, 0x20000, 0xdd94b4d0 )
	ROM_LOAD16_BYTE( "4", 0x40001, 0x20000, 0xa647310a )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "5", 0x00000, 0x4000, 0xb24ab5f4 )
	ROM_CONTINUE(           0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6", 0x000000, 0x80000, 0x75eece28 ) /* Plane 0, 1 */
	ROM_LOAD( "7", 0x080000, 0x80000, 0xb179a76a )
	ROM_LOAD( "9", 0x100000, 0x80000, 0x5c5e6898 ) /* Plane 2, 3 */
	ROM_LOAD( "8", 0x180000, 0x80000, 0x52db30e9 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "11", 0x00000, 0x80000, 0x92111f96 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "10", 0x00000, 0x80000, 0xca0ac419 )
ROM_END

ROM_START( ballbros )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )     /* 256k for 68000 code */
	ROM_LOAD16_BYTE( "10a", 0x00000, 0x20000, 0x4af0e858 )
	ROM_LOAD16_BYTE( "5a",  0x00001, 0x20000, 0x0b983a69 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "8d", 0x00000, 0x4000, 0xd1c515af)
	ROM_CONTINUE(   0x10000, 0xc000 ) /* banked stuff */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "3", 0x000000, 0x20000, 0xec3e0537 ) /* Plane 0, 1 */
	ROM_LOAD( "2", 0x020000, 0x20000, 0xbb441717 )
	ROM_LOAD( "1", 0x040000, 0x20000, 0x8196d624 )
	ROM_LOAD( "0", 0x060000, 0x20000, 0x1cc584e5 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "east-11", 0x00000, 0x80000, 0x92111f96 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Delta-T samples */
	ROM_LOAD( "east-10", 0x00000, 0x80000, 0xca0ac419 )
ROM_END


void init_taitox( void)
{
	state_save_register_int("taitof2", 0, "sound region", &banknum);
	state_save_register_func_postload(reset_sound_region);
}


GAME( 1988, superman, 0, superman, superman, taitox,   ROT0,   "Taito Corporation", "Superman" )
GAME( 1989, daisenpu, 0, daisenpu, daisenpu, taitox,   ROT270, "Taito Corporation", "Daisenpu (Japan)" )
GAME( 1989, gigandes, 0, gigandes, gigandes, taitox,   ROT0,   "East Technology", "Gigandes" )
GAME( 1992, ballbros, 0, ballbros, ballbros, taitox,   ROT0,   "East Technology", "Balloon Brothers" )
