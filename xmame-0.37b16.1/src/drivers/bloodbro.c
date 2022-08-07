/**************************************************************************

Blood Bros, West Story.
TAD Corporation 1990
68000 + Z80 + YM3931 + YM3812

driver by Carlos A. Lozano Baides

Coin inputs are handled by the sound CPU, so they don't work with sound
disabled. Use the service switch instead.

TODO:
West Story:
- sound
- some bad sprites, probably bad ROMs.
- tilemap scroll

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

extern void bloodbro_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );
extern void weststry_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );
extern int bloodbro_vh_start(void);

WRITE16_HANDLER( bloodbro_bgvideoram_w );
WRITE16_HANDLER( bloodbro_fgvideoram_w );
WRITE16_HANDLER( bloodbro_txvideoram_w );

extern data16_t *bloodbro_bgvideoram,*bloodbro_fgvideoram;
extern data16_t *bloodbro_txvideoram;
extern data16_t *bloodbro_scroll;

/***************************************************************************/

static MEMORY_READ16_START( readmem_cpu )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x08afff, MRA16_RAM },
	{ 0x08b000, 0x08bfff, MRA16_RAM },
	{ 0x08c000, 0x08c3ff, MRA16_RAM },
	{ 0x08c400, 0x08cfff, MRA16_RAM },
	{ 0x08d000, 0x08d3ff, MRA16_RAM },
	{ 0x08d400, 0x08d7ff, MRA16_RAM },
	{ 0x08d800, 0x08dfff, MRA16_RAM },
	{ 0x08e000, 0x08e7ff, MRA16_RAM },
	{ 0x08e800, 0x08f7ff, MRA16_RAM },
	{ 0x08f800, 0x08ffff, MRA16_RAM },
	{ 0x0a0000, 0x0a000d, seibu_main_word_r },
	{ 0x0c0000, 0x0c007f, MRA16_RAM },
	{ 0x0e0000, 0x0e0001, input_port_1_word_r },
	{ 0x0e0002, 0x0e0003, input_port_2_word_r },
	{ 0x0e0004, 0x0e0005, input_port_3_word_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem_cpu )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x08afff, MWA16_RAM },
	{ 0x08b000, 0x08bfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x08c000, 0x08c3ff, bloodbro_bgvideoram_w, &bloodbro_bgvideoram },
	{ 0x08c400, 0x08cfff, MWA16_RAM },
	{ 0x08d000, 0x08d3ff, bloodbro_fgvideoram_w, &bloodbro_fgvideoram },
	{ 0x08d400, 0x08d7ff, MWA16_RAM },
	{ 0x08d800, 0x08dfff, bloodbro_txvideoram_w, &bloodbro_txvideoram },
	{ 0x08e000, 0x08e7ff, MWA16_RAM },
	{ 0x08e800, 0x08f7ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x08f800, 0x08ffff, MWA16_RAM },
	{ 0x0a0000, 0x0a000d, seibu_main_word_w },
	{ 0x0c0000, 0x0c007f, MWA16_RAM, &bloodbro_scroll },
	{ 0x0c0080, 0x0c0081, MWA16_NOP }, /* IRQ Ack VBL? */
	{ 0x0c00c0, 0x0c00c1, MWA16_NOP }, /* watchdog? */
/*	{ 0x0c0100, 0x0c0100, MWA16_NOP },    ?? Written 1 time    */
MEMORY_END

/**** West Story Memory Map ********************************************/

static MEMORY_READ16_START( weststry_readmem_cpu )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x08afff, MRA16_RAM },
	{ 0x08b000, 0x08bfff, MRA16_RAM },
	{ 0x08c000, 0x08c3ff, MRA16_RAM },
	{ 0x08c400, 0x08cfff, MRA16_RAM },
	{ 0x08d000, 0x08d3ff, MRA16_RAM },
	{ 0x08d400, 0x08dfff, MRA16_RAM },
	{ 0x08d800, 0x08dfff, MRA16_RAM },
	{ 0x08e000, 0x08ffff, MRA16_RAM },
	{ 0x0c1000, 0x0c1001, input_port_0_word_r },
	{ 0x0c1002, 0x0c1003, input_port_1_word_r },
	{ 0x0c1004, 0x0c1005, input_port_2_word_r },
	{ 0x0c1000, 0x0c17ff, MRA16_RAM },
	{ 0x128000, 0x1287ff, MRA16_RAM },
	{ 0x120000, 0x128fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( weststry_writemem_cpu )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x08afff, MWA16_RAM },
	{ 0x08b000, 0x08bfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x08c000, 0x08c3ff, bloodbro_bgvideoram_w, &bloodbro_bgvideoram },
	{ 0x08c400, 0x08cfff, MWA16_RAM },
	{ 0x08d000, 0x08d3ff, bloodbro_fgvideoram_w, &bloodbro_fgvideoram },
	{ 0x08d400, 0x08d7ff, MWA16_RAM },
	{ 0x08d800, 0x08dfff, bloodbro_txvideoram_w, &bloodbro_txvideoram },
	{ 0x08e000, 0x08ffff, MWA16_RAM },
	{ 0x0c1000, 0x0c17ff, MWA16_RAM },
	{ 0x128000, 0x1287ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x120000, 0x128fff, MWA16_RAM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( bloodbro )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Mode" )
	PORT_DIPSETTING(      0x0001, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
/* Coin Mode 1, todo Mode 2 */
	PORT_DIPNAME( 0x001e, 0x001e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0016, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0012, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
/* mode 2
	PORT_DIPNAME( 0x0006, 0x0006, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
*/
	PORT_DIPNAME( 0x0020, 0x0020, "Starting Coin" )
	PORT_DIPSETTING(      0x0020, "Normal" )
	PORT_DIPSETTING(      0x0000, "x2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "300K 500K" )
	PORT_DIPSETTING(      0x0800, "500K 500K" )
	PORT_DIPSETTING(      0x0400, "500K" )
	PORT_DIPSETTING(      0x0000, "None" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x000e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0e00, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( weststry )
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
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/**** Blood Bros gfx decode ********************************************/

static struct GfxLayout textlayout = {
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 4, 0x10000*8, 0x10000*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout backlayout = {
	16,16,	/* 16*16 sprites  */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
             3+32*16, 2+32*16, 1+32*16, 0+32*16, 16+3+32*16, 16+2+32*16, 16+1+32*16, 16+0+32*16 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout = {
	16,16,	/* 16*16 sprites  */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
             3+32*16, 2+32*16, 1+32*16, 0+32*16, 16+3+32*16, 16+2+32*16, 16+1+32*16, 16+0+32*16 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo bloodbro_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &textlayout,   0x70*16,  0x10 }, /* Text */
	{ REGION_GFX2, 0x00000, &backlayout,   0x40*16,  0x10 }, /* Background */
	{ REGION_GFX2, 0x80000, &backlayout,   0x50*16,  0x10 }, /* Foreground */
	{ REGION_GFX3, 0x00000, &spritelayout, 0x00*16,  0x10 }, /* Sprites */
	{ -1 }
};

/**** West Story gfx decode *********************************************/

static struct GfxLayout weststry_textlayout = {
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 0x8000*8, 2*0x8000*8, 3*0x8000*8 },
        { 0, 1, 2, 3, 4, 5, 6, 7 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout weststry_backlayout = {
	16,16,	/* 16*16 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0*0x20000*8, 1*0x20000*8, 2*0x20000*8, 3*0x20000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
         	16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout weststry_spritelayout = {
	16,16,	/* 16*16 sprites */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 0*0x40000*8, 1*0x40000*8, 2*0x40000*8, 3*0x40000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
         	16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo weststry_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &weststry_textlayout,     16*16,  0x10 },
	{ REGION_GFX2, 0x00000, &weststry_backlayout,     48*16,  0x10 },
	{ REGION_GFX2, 0x80000, &weststry_backlayout,     32*16,  0x10 },
	{ REGION_GFX3, 0x00000, &weststry_spritelayout,    0*16,  0x10 },
	{ -1 }
};

/**** Blood Bros Interrupt & Driver Machine  ****************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);

static struct MachineDriver machine_driver_bloodbro =
{
	{
		{
			CPU_M68000,
			10000000, /* 10 MHz */
			readmem_cpu,writemem_cpu,0,0,
			m68_level4_irq,1
		},
		{
			SEIBU_SOUND_SYSTEM_CPU(14318180/4)
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,	/* CPU slices per frame */
	seibu_sound_init_1, /* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	bloodbro_gfxdecodeinfo,
	2048,2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	bloodbro_vh_start,
	0,
	bloodbro_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
	}
};

static struct MachineDriver machine_driver_weststry =
{
	{
		{
			CPU_M68000,
			10000000, /* 10 MHz */
			weststry_readmem_cpu,weststry_writemem_cpu,0,0,
			m68_level6_irq,1
		},
		{
			SEIBU_SOUND_SYSTEM_CPU(14318180/4)
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,	/* CPU slices per frame */
	seibu_sound_init_1, /* init machine */

	/* video hardware */
	256, 256, { 0, 255, 16, 239 },
	weststry_gfxdecodeinfo,
	1024,1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	bloodbro_vh_start,
	0,
	weststry_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
	}
};



ROM_START( bloodbro )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bb_02.bin",    0x00001, 0x20000, 0xc0fdc3e4 )
	ROM_LOAD16_BYTE( "bb_01.bin",    0x00000, 0x20000, 0x2d7e0fdf )
	ROM_LOAD16_BYTE( "bb_04.bin",    0x40001, 0x20000, 0xfd951c2c )
	ROM_LOAD16_BYTE( "bb_03.bin",    0x40000, 0x20000, 0x18d3c460 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "bb_07.bin",    0x000000, 0x08000, 0x411b94e8 )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bb_05.bin",    0x00000, 0x10000, 0x04ba6d19 )	/* characters */
	ROM_LOAD( "bb_06.bin",    0x10000, 0x10000, 0x7092e35b )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bloodb.bk",   0x00000, 0x100000, 0x1aa87ee6 )	/* Background+Foreground */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "bloodb.obj",   0x00000, 0x100000, 0xd27c3952 )	/* sprites */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "bb_08.bin",    0x00000, 0x20000, 0xdeb1b975 )
ROM_END

ROM_START( weststry )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "ws13.bin",    0x00001, 0x20000, 0x158e302a )
	ROM_LOAD16_BYTE( "ws15.bin",    0x00000, 0x20000, 0x672e9027 )
	ROM_LOAD16_BYTE( "bb_04.bin",   0x40001, 0x20000, 0xfd951c2c )
	ROM_LOAD16_BYTE( "bb_03.bin",   0x40000, 0x20000, 0x18d3c460 )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "ws17.bin",    0x000000, 0x08000, 0xe00a8f09 )
	ROM_CONTINUE(            0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ws09.bin",    0x00000, 0x08000, 0xf05b2b3e )	/* characters */
	ROM_CONTINUE(            0x00000, 0x8000 )
	ROM_LOAD( "ws11.bin",    0x08000, 0x08000, 0x2b10e3d2 )
	ROM_CONTINUE(            0x08000, 0x8000 )
	ROM_LOAD( "ws10.bin",    0x10000, 0x08000, 0xefdf7c82 )
	ROM_CONTINUE(            0x10000, 0x8000 )
	ROM_LOAD( "ws12.bin",    0x18000, 0x08000, 0xaf993578 )
	ROM_CONTINUE(            0x18000, 0x8000 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ws05.bin",    0x00000, 0x20000, 0x007c8dc0 )	/* Background */
	ROM_LOAD( "ws07.bin",    0x20000, 0x20000, 0x0f0c8d9a )
	ROM_LOAD( "ws06.bin",    0x40000, 0x20000, 0x459d075e )
	ROM_LOAD( "ws08.bin",    0x60000, 0x20000, 0x4d6783b3 )
	ROM_LOAD( "ws01.bin",    0x80000, 0x20000, 0x32bda4bc )	/* Foreground */
	ROM_LOAD( "ws03.bin",    0xa0000, 0x20000, 0x046b51f8 )
	ROM_LOAD( "ws02.bin",    0xc0000, 0x20000, 0xed9d682e )
	ROM_LOAD( "ws04.bin",    0xe0000, 0x20000, 0x75f082e5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ws25.bin",    0x00000, 0x20000, 0x8092e8e9 )	/* sprites */
	ROM_LOAD( "ws26.bin",    0x20000, 0x20000, 0xf6a1f42c )
	ROM_LOAD( "ws23.bin",    0x40000, 0x20000, 0x43d58e24 )
	ROM_LOAD( "ws24.bin",    0x60000, 0x20000, 0x20a867ea )
	ROM_LOAD( "ws21.bin",    0x80000, 0x20000, 0xe23d7296 )
	ROM_LOAD( "ws22.bin",    0xa0000, 0x20000, 0x7150a060 )
	ROM_LOAD( "ws19.bin",    0xc0000, 0x20000, 0xc5dd0a96 )
	ROM_LOAD( "ws20.bin",    0xe0000, 0x20000, 0xf1245c16 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "bb_08.bin",    0x00000, 0x20000, 0xdeb1b975 )
ROM_END

/***************************************************************************/

static void init_weststry(void)
{
	UINT8 *gfx = memory_region(REGION_GFX3);
	int i;

	/* invert sprite data */
	for (i = 0;i < memory_region_length(REGION_GFX3);i++)
		gfx[i] = ~gfx[i];
}

/***************************************************************************/

GAMEX( 1990, bloodbro, 0,        bloodbro, bloodbro, 0,        ROT0, "Tad", "Blood Bros.", GAME_NO_COCKTAIL )
GAMEX( 1990, weststry, bloodbro, weststry, weststry, weststry, ROT0, "bootleg", "West Story", GAME_NO_COCKTAIL | GAME_NO_SOUND )
