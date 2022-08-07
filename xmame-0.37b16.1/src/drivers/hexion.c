/***************************************************************************

Hexion (GX122) (c) 1992 Konami

driver by Nicola Salmoria

Notes:
- There are probably palette PROMs missing. Palette data doesn't seem to be
  written anywhere in RAM.
- The board has a 052591, which is used for protection in Thunder Cross and
  S.P.Y. IN this game, however, it doesn't seem to do much, except maybe
  clear the screen.
- during startup, some garbage is written to video RAM. This is probably
  supposed to go somewhere else, maybe the 052591. It doesn't look like
  palette data.

***************************************************************************/

#include "driver.h"


void hexion_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int hexion_vh_start(void);
void hexion_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

WRITE_HANDLER( hexion_bankswitch_w );
READ_HANDLER( hexion_bankedram_r );
WRITE_HANDLER( hexion_bankedram_w );
WRITE_HANDLER( hexion_bankctrl_w );
WRITE_HANDLER( hexion_gfxrom_select_w );



static WRITE_HANDLER( coincntr_w )
{
/*logerror("%04x: coincntr_w %02x\n",cpu_get_pc(),data); */

	/* bits 0/1 = coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 5 = flip screen */
	flip_screen_set(data & 0x20);

	/* other bit unknown */
if ((data & 0xdc) != 0x10) usrintf_showmessage("coincntr %02x",data);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xdffe, hexion_bankedram_r },
	{ 0xf400, 0xf400, input_port_0_r },
	{ 0xf401, 0xf401, input_port_1_r },
	{ 0xf402, 0xf402, input_port_3_r },
	{ 0xf403, 0xf403, input_port_4_r },
	{ 0xf440, 0xf440, input_port_2_r },
	{ 0xf441, 0xf441, input_port_5_r },
	{ 0xf540, 0xf540, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xdffe, hexion_bankedram_w },
	{ 0xdfff, 0xdfff, hexion_bankctrl_w },
	{ 0xe800, 0xe87f, K051649_waveform_w },
	{ 0xe880, 0xe889, K051649_frequency_w },
	{ 0xe88a, 0xe88e, K051649_volume_w },
	{ 0xe88f, 0xe88f, K051649_keyonoff_w },
	{ 0xf000, 0xf00f, MWA_NOP },	/* 053252? f00e = IRQ ack, f00f = NMI ack */
	{ 0xf200, 0xf200, OKIM6295_data_0_w },
	{ 0xf480, 0xf480, hexion_bankswitch_w },
	{ 0xf4c0, 0xf4c0, coincntr_w },
	{ 0xf500, 0xf500, hexion_gfxrom_select_w },
MEMORY_END



INPUT_PORTS_START( hexion )
	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
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
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x70, "Easiest" )
	PORT_DIPSETTING(    0x60, "Very Easy" )
	PORT_DIPSETTING(    0x50, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x30, "Medium Hard" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x10, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 052591? game waits for it to be 0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+1*4, 0*4, 1*4, RGN_FRAC(1,2)+2*4, RGN_FRAC(1,2)+3*4, 2*4, 3*4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};

static struct k051649_interface k051649_interface =
{
	24000000/16,	/* Clock */
	100,			/* Volume */
};



static int hexion_interrupt(void)
{
	/* NMI handles start and coin inputs, origin unknown */
	if (cpu_getiloops())
		return nmi_interrupt();
	else
		return interrupt();
}

static struct MachineDriver machine_driver_hexion =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			24000000/4,	/* Z80B 6 MHz */
			readmem,writemem,0,0,
			hexion_interrupt,3	/* both IRQ and NMI are used */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 64*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,256,
	hexion_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	hexion_vh_start,
	0,
	hexion_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		},
		{
			SOUND_K051649,
			&k051649_interface,
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( hexion )
	ROM_REGION( 0x34800, REGION_CPU1, 0 )	/* ROMs + space for additional RAM */
	ROM_LOAD( "122jab01.bin", 0x00000, 0x20000, 0xeabc6dd1 )
	ROM_RELOAD(               0x10000, 0x20000 )	/* banked at 8000-9fff */

	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* addressable by the main CPU */
	ROM_LOAD( "122a07.bin",   0x00000, 0x40000, 0x22ae55e3 )
	ROM_LOAD( "122a06.bin",   0x40000, 0x40000, 0x438f4388 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "122a05.bin",   0x0000, 0x40000, 0xbcc831bf )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "proms",        0x0000, 0x0300, 0x00000000 )
ROM_END


static void init_hexion(void)
{
	int col,i;
	UINT8 *prom = memory_region(REGION_PROMS);

	prom[1+0x000] = 17/16;
	prom[1+0x100] = 37/16;
	prom[1+0x200] = 170/16;
	prom[4+0x000] = 100/16;
	prom[4+0x100] = 100/16;
	prom[4+0x200] = 100/16;
	prom[5+0x000] = 206/16;
	prom[5+0x100] = 16/16;
	prom[5+0x200] = 16/16;
	prom[6+0x000] = 160/16;
	prom[6+0x100] = 16/16;
	prom[6+0x200] = 16/16;
	prom[7+0x000] = 0/16;
	prom[7+0x100] = 216/16;
	prom[7+0x200] = 254/16;
	prom[8+0x000] = 132/16;
	prom[8+0x100] = 237/16;
	prom[8+0x200] = 243/16;
	prom[9+0x000] = 157/16;
	prom[9+0x100] = 255/16;
	prom[9+0x200] = 255/16;
	prom[10+0x000] = 200/16;
	prom[10+0x100] = 30/16;
	prom[10+0x200] = 110/16;
	prom[11+0x000] = 230/16;
	prom[11+0x100] = 80/16;
	prom[11+0x200] = 120/16;
	prom[12+0x000] = 230/16;
	prom[12+0x100] = 104/16;
	prom[12+0x200] = 140/16;
	prom[13+0x000] = 90/16;
	prom[13+0x100] = 104/16;
	prom[13+0x200] = 190/16;
	prom[14+0x000] = 192/16;
	prom[14+0x100] = 222/16;
	prom[14+0x200] = 255/16;
	prom[15+0x000] = 255/16;
	prom[15+0x100] = 255/16;
	prom[15+0x200] = 255/16;
	prom[1*16+1+0x000] = 17/16;
	prom[1*16+1+0x100] = 37/16;
	prom[1*16+1+0x200] = 170/16;
	prom[1*16+8+0x000] = 216/16;
	prom[1*16+8+0x100] = 221/16;
	prom[1*16+8+0x200] = 167/16;
	prom[1*16+14+0x000] = 183/16;
	prom[1*16+14+0x100] = 162/16;
	prom[1*16+14+0x200] = 238/16;
	prom[2*16+1+0x000] = 17/16;
	prom[2*16+1+0x100] = 37/16;
	prom[2*16+1+0x200] = 170/16;
	prom[2*16+14+0x000] = 117/16;
	prom[2*16+14+0x100] = 212/16;
	prom[2*16+14+0x200] = 255/16;

	col = 0x05;
	for (i = 1;i < 16;i++)
	{
		prom[col*16+i+0x000] = (80+i*(255-80)/15)/16;
		prom[col*16+i+0x100] = (150+i*(255-150)/15)/16;
		prom[col*16+i+0x200] = (60+i*(255-60)/15)/16;
	}
	col = 0x0c;
	for (i = 0;i < 16;i++)
	{
		prom[col*16+i+0x000] = i;
		prom[col*16+i+0x100] = i;
		prom[col*16+i+0x200] = i;
	}
	col = 0x0d;
	for (i = 0;i < 16;i++)
	{
		prom[col*16+i+0x000] = i;
		prom[col*16+i+0x100] = 0;
		prom[col*16+i+0x200] = 0;
	}
	col = 0x0e;
	for (i = 0;i < 16;i++)
	{
		prom[col*16+i+0x000] = 0;
		prom[col*16+i+0x100] = i;
		prom[col*16+i+0x200] = 0;
	}
	col = 0x0f;
	for (i = 0;i < 16;i++)
	{
		prom[col*16+i+0x000] = 0;
		prom[col*16+i+0x100] = 0;
		prom[col*16+i+0x200] = i;
	}

	for (col = 0;col < 16;col++)
	{
		prom[col*16+0x000] = 0;
		prom[col*16+0x100] = 0;
		prom[col*16+0x200] = 0;
	}
}


GAMEX( 1992, hexion, 0, hexion, hexion, hexion, ROT0, "Konami", "Hexion (Japan)", GAME_WRONG_COLORS )
