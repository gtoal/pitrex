/***************************************************************************

1942

driver by Paul Leaman


MAIN CPU:

0000-bfff ROM (8000-bfff banked)
cc00-cc7f Sprites
d000-d3ff Video RAM
d400-d7ff Color RAM
d800-dbff Background RAM (groups of 32 bytes, 16 code, 16 color/attribute)
e000-efff RAM

read:
c000      IN0
c001      IN1
c002      IN2
c003      DSW0
c004      DSW1

write:
c800      command for the audio CPU
c802-c803 background scroll
c804      bit 7: flip screen
          bit 4: cpu B reset
		  bit 0: coin counter
c805      background palette bank selector
c806      bit 0-1 ROM bank selector 00=1-N5.BIN
                                    01=1-N6.BIN
                                    10=1-N7.BIN



SOUND CPU:

0000-3fff ROM
4000-47ff RAM
6000      command from the main CPU
8000      8910 #1 control
8001      8910 #1 write
c000      8910 #2 control
c001      8910 #2 write



Game runs in interrupt mode 0 (the devices supply the interrupt number).

Two interrupts must be triggered per refresh for the game to function
correctly.

0x10 is the video retrace. This controls the speed of the game and generally
     drives the code. This must be triggerd for each video retrace.
0x08 is the sound card service interupt. The game uses this to throw sounds
     at the sound CPU.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *c1942_fgvideoram;
extern unsigned char *c1942_bgvideoram;


int c1942_vh_start(void);
void c1942_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
WRITE_HANDLER( c1942_fgvideoram_w );
WRITE_HANDLER( c1942_bgvideoram_w );
WRITE_HANDLER( c1942_scroll_w );
WRITE_HANDLER( c1942_c804_w );
WRITE_HANDLER( c1942_palette_bank_w );
void c1942_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static WRITE_HANDLER( c1942_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);


	bankaddress = 0x10000 + (data & 0x03) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}



static int c1942_interrupt(void)
{
	if (cpu_getiloops() != 0) return 0x00cf;	/* RST 08h */
	else return 0x00d7;	/* RST 10h - vblank */
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_0_r },	/* IN0 */
	{ 0xc001, 0xc001, input_port_1_r },	/* IN1 */
	{ 0xc002, 0xc002, input_port_2_r },	/* IN2 */
	{ 0xc003, 0xc003, input_port_3_r },	/* DSW0 */
	{ 0xc004, 0xc004, input_port_4_r },	/* DSW1 */
	{ 0xd000, 0xdbff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc802, 0xc803, c1942_scroll_w },
	{ 0xc804, 0xc804, c1942_c804_w },
	{ 0xc805, 0xc805, c1942_palette_bank_w },
	{ 0xc806, 0xc806, c1942_bankswitch_w },
	{ 0xcc00, 0xcc7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xd7ff, c1942_fgvideoram_w, &c1942_fgvideoram },
	{ 0xd800, 0xdbff, c1942_bgvideoram_w, &c1942_bgvideoram },
	{ 0xe000, 0xefff, MWA_RAM },
MEMORY_END



static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0xc000, 0xc000, AY8910_control_port_1_w },
	{ 0xc001, 0xc001, AY8910_write_port_1_w },
MEMORY_END



INPUT_PORTS_START( 1942 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "20000 80000" )
	PORT_DIPSETTING(    0x20, "20000 100000" )
	PORT_DIPSETTING(    0x10, "30000 80000" )
	PORT_DIPSETTING(    0x00, "30000 100000" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+8+0, 16*16+8+1, 16*16+8+2, 16*16+8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,             0, 64 },
	{ REGION_GFX2, 0, &tilelayout,          64*4, 4*32 },
	{ REGION_GFX3, 0, &spritelayout, 64*4+4*32*8, 16 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 25, 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver_1942 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 MHz (?) */
			readmem,writemem,0,0,
			c1942_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 MHz ??? */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 64*4+4*32*8+16*16,
	c1942_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	c1942_vh_start,
	0,
	c1942_vh_screenrefresh,

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

ROM_START( 1942 )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "1-n3a.bin",    0x00000, 0x4000, 0x40201bab )
	ROM_LOAD( "1-n4.bin",     0x04000, 0x4000, 0xa60ac644 )
	ROM_LOAD( "1-n5.bin",     0x10000, 0x4000, 0x835f7b24 )
	ROM_LOAD( "1-n6.bin",     0x14000, 0x2000, 0x821c6481 )
	ROM_LOAD( "1-n7.bin",     0x18000, 0x4000, 0x5df525e1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1-c11.bin",    0x0000, 0x4000, 0xbd87f06b )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-f2.bin",     0x0000, 0x2000, 0x6ebca191 )	/* characters */

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2-a1.bin",     0x0000, 0x2000, 0x3884d9eb )	/* tiles */
	ROM_LOAD( "2-a2.bin",     0x2000, 0x2000, 0x999cf6e0 )
	ROM_LOAD( "2-a3.bin",     0x4000, 0x2000, 0x8edb273a )
	ROM_LOAD( "2-a4.bin",     0x6000, 0x2000, 0x3a2726c3 )
	ROM_LOAD( "2-a5.bin",     0x8000, 0x2000, 0x1bd3d8bb )
	ROM_LOAD( "2-a6.bin",     0xa000, 0x2000, 0x658f02c4 )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2-l1.bin",     0x00000, 0x4000, 0x2528bec6 )	/* sprites */
	ROM_LOAD( "2-l2.bin",     0x04000, 0x4000, 0xf89287aa )
	ROM_LOAD( "2-n1.bin",     0x08000, 0x4000, 0x024418f8 )
	ROM_LOAD( "2-n2.bin",     0x0c000, 0x4000, 0xe2c7e489 )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "08e_sb-5.bin", 0x0000, 0x0100, 0x93ab8153 )	/* red component */
	ROM_LOAD( "09e_sb-6.bin", 0x0100, 0x0100, 0x8ab44f7d )	/* green component */
	ROM_LOAD( "10e_sb-7.bin", 0x0200, 0x0100, 0xf4ade9a4 )	/* blue component */
	ROM_LOAD( "f01_sb-0.bin", 0x0300, 0x0100, 0x6047d91b )	/* char lookup table */
	ROM_LOAD( "06d_sb-4.bin", 0x0400, 0x0100, 0x4858968d )	/* tile lookup table */
	ROM_LOAD( "03k_sb-8.bin", 0x0500, 0x0100, 0xf6fad943 )	/* sprite lookup table */
	ROM_LOAD( "01d_sb-2.bin", 0x0600, 0x0100, 0x8bb8b3df )	/* tile palette selector? (not used) */
	ROM_LOAD( "02d_sb-3.bin", 0x0700, 0x0100, 0x3b0c99af )	/* tile palette selector? (not used) */
	ROM_LOAD( "k06_sb-1.bin", 0x0800, 0x0100, 0x712ac508 )	/* interrupt timing (not used) */
	ROM_LOAD( "01m_sb-9.bin", 0x0900, 0x0100, 0x4921635c )	/* video timing? (not used) */
ROM_END

ROM_START( 1942a )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "1-n3.bin",     0x00000, 0x4000, 0x612975f2 )
	ROM_LOAD( "1-n4.bin",     0x04000, 0x4000, 0xa60ac644 )
	ROM_LOAD( "1-n5.bin",     0x10000, 0x4000, 0x835f7b24 )
	ROM_LOAD( "1-n6.bin",     0x14000, 0x2000, 0x821c6481 )
	ROM_LOAD( "1-n7.bin",     0x18000, 0x4000, 0x5df525e1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1-c11.bin",    0x0000, 0x4000, 0xbd87f06b )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-f2.bin",     0x0000, 0x2000, 0x6ebca191 )	/* characters */

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2-a1.bin",     0x0000, 0x2000, 0x3884d9eb )	/* tiles */
	ROM_LOAD( "2-a2.bin",     0x2000, 0x2000, 0x999cf6e0 )
	ROM_LOAD( "2-a3.bin",     0x4000, 0x2000, 0x8edb273a )
	ROM_LOAD( "2-a4.bin",     0x6000, 0x2000, 0x3a2726c3 )
	ROM_LOAD( "2-a5.bin",     0x8000, 0x2000, 0x1bd3d8bb )
	ROM_LOAD( "2-a6.bin",     0xa000, 0x2000, 0x658f02c4 )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2-l1.bin",     0x00000, 0x4000, 0x2528bec6 )	/* sprites */
	ROM_LOAD( "2-l2.bin",     0x04000, 0x4000, 0xf89287aa )
	ROM_LOAD( "2-n1.bin",     0x08000, 0x4000, 0x024418f8 )
	ROM_LOAD( "2-n2.bin",     0x0c000, 0x4000, 0xe2c7e489 )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "08e_sb-5.bin", 0x0000, 0x0100, 0x93ab8153 )	/* red component */
	ROM_LOAD( "09e_sb-6.bin", 0x0100, 0x0100, 0x8ab44f7d )	/* green component */
	ROM_LOAD( "10e_sb-7.bin", 0x0200, 0x0100, 0xf4ade9a4 )	/* blue component */
	ROM_LOAD( "f01_sb-0.bin", 0x0300, 0x0100, 0x6047d91b )	/* char lookup table */
	ROM_LOAD( "06d_sb-4.bin", 0x0400, 0x0100, 0x4858968d )	/* tile lookup table */
	ROM_LOAD( "03k_sb-8.bin", 0x0500, 0x0100, 0xf6fad943 )	/* sprite lookup table */
	ROM_LOAD( "01d_sb-2.bin", 0x0600, 0x0100, 0x8bb8b3df )	/* tile palette selector? (not used) */
	ROM_LOAD( "02d_sb-3.bin", 0x0700, 0x0100, 0x3b0c99af )	/* tile palette selector? (not used) */
	ROM_LOAD( "k06_sb-1.bin", 0x0800, 0x0100, 0x712ac508 )	/* interrupt timing (not used) */
	ROM_LOAD( "01m_sb-9.bin", 0x0900, 0x0100, 0x4921635c )	/* video timing? (not used) */
ROM_END

ROM_START( 1942b )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "srb-03.n3",    0x00000, 0x4000, 0xd9dafcc3 )
	ROM_LOAD( "srb-04.n4",    0x04000, 0x4000, 0xda0cf924 )
	ROM_LOAD( "srb-05.n5",    0x10000, 0x4000, 0xd102911c )
	ROM_LOAD( "srb-06.n6",    0x14000, 0x2000, 0x466f8248 )
	ROM_LOAD( "srb-07.n7",    0x18000, 0x4000, 0x0d31038c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1-c11.bin",    0x0000, 0x4000, 0xbd87f06b )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1-f2.bin",     0x0000, 0x2000, 0x6ebca191 )	/* characters */

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "2-a1.bin",     0x0000, 0x2000, 0x3884d9eb )	/* tiles */
	ROM_LOAD( "2-a2.bin",     0x2000, 0x2000, 0x999cf6e0 )
	ROM_LOAD( "2-a3.bin",     0x4000, 0x2000, 0x8edb273a )
	ROM_LOAD( "2-a4.bin",     0x6000, 0x2000, 0x3a2726c3 )
	ROM_LOAD( "2-a5.bin",     0x8000, 0x2000, 0x1bd3d8bb )
	ROM_LOAD( "2-a6.bin",     0xa000, 0x2000, 0x658f02c4 )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2-l1.bin",     0x00000, 0x4000, 0x2528bec6 )	/* sprites */
	ROM_LOAD( "2-l2.bin",     0x04000, 0x4000, 0xf89287aa )
	ROM_LOAD( "2-n1.bin",     0x08000, 0x4000, 0x024418f8 )
	ROM_LOAD( "2-n2.bin",     0x0c000, 0x4000, 0xe2c7e489 )

	ROM_REGION( 0x0a00, REGION_PROMS, 0 )
	ROM_LOAD( "08e_sb-5.bin", 0x0000, 0x0100, 0x93ab8153 )	/* red component */
	ROM_LOAD( "09e_sb-6.bin", 0x0100, 0x0100, 0x8ab44f7d )	/* green component */
	ROM_LOAD( "10e_sb-7.bin", 0x0200, 0x0100, 0xf4ade9a4 )	/* blue component */
	ROM_LOAD( "f01_sb-0.bin", 0x0300, 0x0100, 0x6047d91b )	/* char lookup table */
	ROM_LOAD( "06d_sb-4.bin", 0x0400, 0x0100, 0x4858968d )	/* tile lookup table */
	ROM_LOAD( "03k_sb-8.bin", 0x0500, 0x0100, 0xf6fad943 )	/* sprite lookup table */
	ROM_LOAD( "01d_sb-2.bin", 0x0600, 0x0100, 0x8bb8b3df )	/* tile palette selector? (not used) */
	ROM_LOAD( "02d_sb-3.bin", 0x0700, 0x0100, 0x3b0c99af )	/* tile palette selector? (not used) */
	ROM_LOAD( "k06_sb-1.bin", 0x0800, 0x0100, 0x712ac508 )	/* interrupt timing (not used) */
	ROM_LOAD( "01m_sb-9.bin", 0x0900, 0x0100, 0x4921635c )	/* video timing? (not used) */
ROM_END



GAME( 1984, 1942,  0,    1942, 1942, 0, ROT270, "Capcom", "1942 (set 1)" )
GAME( 1984, 1942a, 1942, 1942, 1942, 0, ROT270, "Capcom", "1942 (set 2)" )
GAME( 1984, 1942b, 1942, 1942, 1942, 0, ROT270, "Capcom", "1942 (set 3)" )
