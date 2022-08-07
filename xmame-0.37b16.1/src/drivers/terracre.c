/******************************************************************
Terra Cresta (preliminary)
Nichibutsu 1985
68000 + Z80

driver by Carlos A. Lozano

TODO: I'm playing samples with a DAC, but they could be ADPCM

Carlos A. Lozano (calb@gsyc.inf.uc3m.es)

MEMORY MAP
0x000000 - 0x01ffff   ROM
0x020000 - 0x02006f   VRAM (Sprites)???
0x020070 - 0x021fff   RAM
0x022000 - 0x022fff   VRAM (Background)???
0x024000 - 0x24000f   Input Ports
0x026000 - 0x26000f   Output Ports
0x028000 - 0x287fff   VRAM (Tiles)

VRAM(Background)
0x22000 - 32 bytes (16 tiles)
0x22040 - 32 bytes (16 tiles)
0x22080 - 32 bytes (16 tiles)
0x220c0 - 32 bytes (16 tiles)
0x22100 - 32 bytes (16 tiles)
...
0x22fc0 - 32 bytes (16 tiles)

VRAM(Tiles)
0x28000-0x287ff (1024 tiles 8x8 tiles, 2 bytes every tile)

VRAM(Sprites)
0x20000-0x201ff

******************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"


void terrac_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void terracre_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
/*void terracre_vh_screenrefresh(struct osd_bitmap *bitmap); */
int terrac_vh_start(void);
void terrac_vh_stop(void);
WRITE16_HANDLER( terrac_videoram2_w );
READ16_HANDLER( terrac_videoram2_r );

extern data16_t *terrac_videoram2;
extern size_t terrac_videoram2_size;
extern data16_t *terrac_scrolly;


WRITE16_HANDLER( terracre_soundcmd_w )
{
	soundlatch_w(0,((data & 0x7f) << 1) | 1);
}

static READ_HANDLER( soundlatch_clear_r )
{
	soundlatch_clear_w(0,0);
	return 0;
}



static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x020000, 0x0201ff, MRA16_RAM },
	{ 0x020200, 0x021fff, MRA16_RAM },
	{ 0x022000, 0x022fff, terrac_videoram2_r },
	{ 0x023000, 0x023fff, MRA16_RAM },
	{ 0x024000, 0x024001, input_port_0_word_r },
	{ 0x024002, 0x024003, input_port_1_word_r },
	{ 0x024004, 0x024005, input_port_2_word_r },
	{ 0x024006, 0x024007, input_port_3_word_r },
	{ 0x028000, 0x0287ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x020000, 0x0201ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x020200, 0x021fff, MWA16_RAM },
	{ 0x022000, 0x022fff, terrac_videoram2_w, &terrac_videoram2, &terrac_videoram2_size },
	{ 0x023000, 0x023fff, MWA16_RAM },
	{ 0x026002, 0x026003, MWA16_RAM, &terrac_scrolly },
	{ 0x02600c, 0x02600d, terracre_soundcmd_w },
	{ 0x028000, 0x0287ff, MWA16_RAM, &videoram16, &videoram_size },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x04, 0x04, soundlatch_clear_r },
	{ 0x06, 0x06, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport_3526 )
	{ 0x00, 0x00, YM3526_control_port_0_w },
	{ 0x01, 0x01, YM3526_write_port_0_w },
	{ 0x02, 0x02, DAC_0_signed_data_w },
	{ 0x03, 0x03, DAC_1_signed_data_w },
PORT_END

static PORT_WRITE_START( sound_writeport_2203 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x02, 0x02, DAC_0_signed_data_w },
	{ 0x03, 0x03, DAC_1_signed_data_w },
PORT_END


INPUT_PORTS_START( terracre )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x2000, IP_ACTIVE_LOW )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPSETTING(      0x0001, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x000c, "20000 60000" )
	PORT_DIPSETTING(      0x0008, "30000 70000" )
	PORT_DIPSETTING(      0x0004, "40000 80000" )
	PORT_DIPSETTING(      0x0000, "50000 90000" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x1000, "Easy" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x4000, 0x4000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Complete Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Base Ship Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout backlayout =
{
	16,16,	/* 16*16 chars */
	512,	/* 512 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* plane offset */
	{ 4, 0, 12, 8, 20, 16, 28, 24,
		32+4, 32+0, 32+12, 32+8, 32+20, 32+16, 32+28, 32+24, },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8   /* every char takes 128 consecutive bytes  */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 characters */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 4+0x8000*8, 0+0x8000*8, 12, 8, 12+0x8000*8, 8+0x8000*8,
		20, 16, 20+0x8000*8, 16+0x8000*8, 28, 24, 28+0x8000*8, 24+0x8000*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
          8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8	/* every char takes 64 consecutive bytes  */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,            0,   1 },
	{ REGION_GFX2, 0, &backlayout,         1*16,  16 },
	{ REGION_GFX3, 0, &spritelayout, 1*16+16*16, 256 },
	{ -1 } /* end of array */
};




static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	4000000,	/* 4 MHz ? (hand tuned) */
	{ 50 }		/* volume */
};

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ???? */
	{ YM2203_VOL(40,20), YM2203_VOL(40,20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	2,	/* 2 channels */
	{ 50, 50 }
};


static struct MachineDriver machine_driver_ym3526 =
{
	{
		{
			CPU_M68000,
			8000000, /* 8 MHz?? */
			readmem,writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz???? */
			sound_readmem,sound_writemem,sound_readport,sound_writeport_3526,
			interrupt,128	/* ??? */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 1*16+16*16+16*256,
	terrac_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	terrac_vh_start,
	terrac_vh_stop,
	terracre_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
		   SOUND_YM3526,
		   &ym3526_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver machine_driver_ym2203 =
{
	{
		{
			CPU_M68000,
			8000000, /* 8 MHz?? */
			readmem,writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz???? */
			sound_readmem,sound_writemem,sound_readport,sound_writeport_2203,
			interrupt,128	/* ??? */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 1*16+16*16+16*256,
	terrac_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	terrac_vh_start,
	terrac_vh_stop,
	terracre_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
		   SOUND_YM2203,
		   &ym2203_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



ROM_START( terracre )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128K for 68000 code */
	ROM_LOAD16_BYTE( "1a_4b.rom",    0x00001, 0x4000, 0x76f17479 )
	ROM_LOAD16_BYTE( "1a_4d.rom",    0x00000, 0x4000, 0x8119f06e )
	ROM_LOAD16_BYTE( "1a_6b.rom",    0x08001, 0x4000, 0xba4b5822 )
	ROM_LOAD16_BYTE( "1a_6d.rom",    0x08000, 0x4000, 0xca4852f6 )
	ROM_LOAD16_BYTE( "1a_7b.rom",    0x10001, 0x4000, 0xd0771bba )
	ROM_LOAD16_BYTE( "1a_7d.rom",    0x10000, 0x4000, 0x029d59d9 )
	ROM_LOAD16_BYTE( "1a_9b.rom",    0x18001, 0x4000, 0x69227b56 )
	ROM_LOAD16_BYTE( "1a_9d.rom",    0x18000, 0x4000, 0x5a672942 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "2a_15b.rom",   0x0000, 0x4000, 0x604c3b11 )
	ROM_LOAD( "2a_17b.rom",   0x4000, 0x4000, 0xaffc898d )
	ROM_LOAD( "2a_18b.rom",   0x8000, 0x4000, 0x302dc0ab )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_16b.rom",   0x00000, 0x2000, 0x591a3804 ) /* tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1a_15f.rom",   0x00000, 0x8000, 0x984a597f ) /* Background */
	ROM_LOAD( "1a_17f.rom",   0x08000, 0x8000, 0x30e297ff )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_6e.rom",    0x00000, 0x4000, 0xbcf7740b ) /* Sprites */
	ROM_LOAD( "2a_7e.rom",    0x04000, 0x4000, 0xa70b565c )
	ROM_LOAD( "2a_6g.rom",    0x08000, 0x4000, 0x4a9ec3e6 )
	ROM_LOAD( "2a_7g.rom",    0x0c000, 0x4000, 0x450749fc )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "tc1a_10f.bin", 0x0000, 0x0100, 0xce07c544 )	/* red component */
	ROM_LOAD( "tc1a_11f.bin", 0x0100, 0x0100, 0x566d323a )	/* green component */
	ROM_LOAD( "tc1a_12f.bin", 0x0200, 0x0100, 0x7ea63946 )	/* blue component */
	ROM_LOAD( "tc2a_2g.bin",  0x0300, 0x0100, 0x08609bad )	/* sprite lookup table */
	ROM_LOAD( "tc2a_4e.bin",  0x0400, 0x0100, 0x2c43991f )	/* sprite palette bank */
ROM_END

/**********************************************************/
/* Notes: All the roms are the same except the SOUND ROMs */
/**********************************************************/

ROM_START( terracrb )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128K for 68000 code */
	ROM_LOAD16_BYTE( "1a_4b.rom",    0x00001, 0x4000, 0x76f17479 )
	ROM_LOAD16_BYTE( "1a_4d.rom",    0x00000, 0x4000, 0x8119f06e )
	ROM_LOAD16_BYTE( "1a_6b.rom",    0x08001, 0x4000, 0xba4b5822 )
	ROM_LOAD16_BYTE( "1a_6d.rom",    0x08000, 0x4000, 0xca4852f6 )
	ROM_LOAD16_BYTE( "1a_7b.rom",    0x10001, 0x4000, 0xd0771bba )
	ROM_LOAD16_BYTE( "1a_7d.rom",    0x10000, 0x4000, 0x029d59d9 )
	ROM_LOAD16_BYTE( "1a_9b.rom",    0x18001, 0x4000, 0x69227b56 )
	ROM_LOAD16_BYTE( "1a_9d.rom",    0x18000, 0x4000, 0x5a672942 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "2a_15b.rom",   0x0000, 0x4000, 0x604c3b11 )
	ROM_LOAD( "dg.12",        0x4000, 0x4000, 0x9e9b3808 )
	ROM_LOAD( "2a_18b.rom",   0x8000, 0x4000, 0x302dc0ab )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_16b.rom",   0x00000, 0x2000, 0x591a3804 ) /* tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1a_15f.rom",   0x00000, 0x8000, 0x984a597f ) /* Background */
	ROM_LOAD( "1a_17f.rom",   0x08000, 0x8000, 0x30e297ff )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_6e.rom",    0x00000, 0x4000, 0xbcf7740b ) /* Sprites */
	ROM_LOAD( "2a_7e.rom",    0x04000, 0x4000, 0xa70b565c )
	ROM_LOAD( "2a_6g.rom",    0x08000, 0x4000, 0x4a9ec3e6 )
	ROM_LOAD( "2a_7g.rom",    0x0c000, 0x4000, 0x450749fc )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "tc1a_10f.bin", 0x0000, 0x0100, 0xce07c544 )	/* red component */
	ROM_LOAD( "tc1a_11f.bin", 0x0100, 0x0100, 0x566d323a )	/* green component */
	ROM_LOAD( "tc1a_12f.bin", 0x0200, 0x0100, 0x7ea63946 )	/* blue component */
	ROM_LOAD( "tc2a_2g.bin",  0x0300, 0x0100, 0x08609bad )	/* sprite lookup table */
	ROM_LOAD( "tc2a_4e.bin",  0x0400, 0x0100, 0x2c43991f )	/* sprite palette bank */
ROM_END

/**********************************************************/
/* Notes: All the roms are the same except the SOUND ROMs */
/**********************************************************/

ROM_START( terracra )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128K for 68000 code */
	ROM_LOAD16_BYTE( "1a_4b.rom",    0x00001, 0x4000, 0x76f17479 )
	ROM_LOAD16_BYTE( "1a_4d.rom",    0x00000, 0x4000, 0x8119f06e )
	ROM_LOAD16_BYTE( "1a_6b.rom",    0x08001, 0x4000, 0xba4b5822 )
	ROM_LOAD16_BYTE( "1a_6d.rom",    0x08000, 0x4000, 0xca4852f6 )
	ROM_LOAD16_BYTE( "1a_7b.rom",    0x10001, 0x4000, 0xd0771bba )
	ROM_LOAD16_BYTE( "1a_7d.rom",    0x10000, 0x4000, 0x029d59d9 )
	ROM_LOAD16_BYTE( "1a_9b.rom",    0x18001, 0x4000, 0x69227b56 )
	ROM_LOAD16_BYTE( "1a_9d.rom",    0x18000, 0x4000, 0x5a672942 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k to sound cpu */
	ROM_LOAD( "tc2a_15b.bin", 0x0000, 0x4000, 0x790ddfa9 )
	ROM_LOAD( "tc2a_17b.bin", 0x4000, 0x4000, 0xd4531113 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_16b.rom",   0x00000, 0x2000, 0x591a3804 ) /* tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1a_15f.rom",   0x00000, 0x8000, 0x984a597f ) /* Background */
	ROM_LOAD( "1a_17f.rom",   0x08000, 0x8000, 0x30e297ff )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "2a_6e.rom",    0x00000, 0x4000, 0xbcf7740b ) /* Sprites */
	ROM_LOAD( "2a_7e.rom",    0x04000, 0x4000, 0xa70b565c )
	ROM_LOAD( "2a_6g.rom",    0x08000, 0x4000, 0x4a9ec3e6 )
	ROM_LOAD( "2a_7g.rom",    0x0c000, 0x4000, 0x450749fc )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "tc1a_10f.bin", 0x0000, 0x0100, 0xce07c544 )	/* red component */
	ROM_LOAD( "tc1a_11f.bin", 0x0100, 0x0100, 0x566d323a )	/* green component */
	ROM_LOAD( "tc1a_12f.bin", 0x0200, 0x0100, 0x7ea63946 )	/* blue component */
	ROM_LOAD( "tc2a_2g.bin",  0x0300, 0x0100, 0x08609bad )	/* sprite lookup table */
	ROM_LOAD( "tc2a_4e.bin",  0x0400, 0x0100, 0x2c43991f )	/* sprite palette bank */
ROM_END



GAME( 1985, terracre, 0,        ym3526, terracre, 0, ROT270, "Nichibutsu", "Terra Cresta (YM3526 set 1)" )
GAME( 1985, terracrb, terracre, ym3526, terracre, 0, ROT270, "Nichibutsu", "Terra Cresta (YM3526 set 2)" )
GAME( 1985, terracra, terracre, ym2203, terracre, 0, ROT270, "Nichibutsu", "Terra Cresta (YM2203)" )
