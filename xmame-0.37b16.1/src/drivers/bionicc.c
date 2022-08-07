/********************************************************************

			  Bionic Commando



ToDo:
- finish video driver
	Some attributes are unknown. I don't remember the original game but
	seems there are some problems:
	- misplaced sprites ? ( see beginning of level 1 or 2 for example )
	- sprite / sprite priority ? ( see level 2 the reflectors )
	- sprite / background priority ? ( see level 1: birds walk through
		branches of different trees )
	- see the beginning of level 3: is the background screwed ?

- get rid of input port hack

	Controls appear to be mapped at 0xFE4000, alongside dip switches, but there
	is something strange going on that I can't (yet) figure out.
	Player controls and coin inputs are supposed to magically appear at
	0xFFFFFB (coin/start)
	0xFFFFFD (player 2)
	0xFFFFFF (player 1)
	This is probably done by an MPU on the board (whose ROM is not
	available).

	The MPU also takes care of the commands for the sound CPU, which are stored
	at FFFFF9.

	IRQ4 seems to be control related.
	On each interrupt, it reads 0xFE4000 (coin/start), shift the bits around
	and move the resulting byte into a dword RAM location. The dword RAM location
	is rotated by 8 bits each time this happens.
	This is probably done to be pedantic about coin insertions (might be protection
	related). In fact, currently coin insertions are not consistently recognized.

********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


WRITE16_HANDLER( bionicc_fgvideoram_w );
WRITE16_HANDLER( bionicc_bgvideoram_w );
WRITE16_HANDLER( bionicc_txvideoram_w );
WRITE16_HANDLER( bionicc_paletteram_w );
WRITE16_HANDLER( bionicc_scroll_w );
WRITE16_HANDLER( bionicc_gfxctrl_w );

extern data16_t *bionicc_bgvideoram;
extern data16_t *bionicc_fgvideoram;
extern data16_t *bionicc_txvideoram;

int bionicc_vh_start(void);
void bionicc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void bionicc_eof_callback(void);

void bionicc_readinputs(void);
void bionicc_sound_cmd(int data);



static data16_t bionicc_inp[3];

WRITE16_HANDLER( hacked_controls_w )
{
logerror("%06x: hacked_controls_w %04x %02x\n",cpu_get_pc(),offset,data);
	COMBINE_DATA(&bionicc_inp[offset]);
}

static READ16_HANDLER( hacked_controls_r )
{
logerror("%06x: hacked_controls_r %04x %04x\n",cpu_get_pc(),offset,bionicc_inp[offset]);
	return bionicc_inp[offset];
}

static WRITE16_HANDLER( bionicc_mpu_trigger_w )
{
	data = readinputport(0) >> 12;
	bionicc_inp[0] = data ^ 0x0f;

	data = readinputport(3); /* player 2 controls */
	bionicc_inp[1] = data ^ 0xff;

	data = readinputport(2); /* player 1 controls */
	bionicc_inp[2] = data ^ 0xff;
}


static data16_t soundcommand;

WRITE16_HANDLER( hacked_soundcommand_w )
{
	COMBINE_DATA(&soundcommand);
	soundlatch_w(0,soundcommand & 0xff);
}

static READ16_HANDLER( hacked_soundcommand_r )
{
	return soundcommand;
}


/********************************************************************

  INTERRUPT

  The game runs on 2 interrupts.

  IRQ 2 drives the game
  IRQ 4 processes the input ports

  The game is very picky about timing. The following is the only
  way I have found it to work.

********************************************************************/

int bionicc_interrupt(void)
{
	if (cpu_getiloops() == 0) return 2;
	else return 4;
}
static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },                /* 68000 ROM */
	{ 0xfe0000, 0xfe07ff, MRA16_RAM },                /* RAM? */
	{ 0xfe0800, 0xfe0cff, MRA16_RAM },                /* sprites */
	{ 0xfe0d00, 0xfe3fff, MRA16_RAM },                /* RAM? */
	{ 0xfe4000, 0xfe4001, input_port_0_word_r },
	{ 0xfe4002, 0xfe4003, input_port_1_word_r },
	{ 0xfec000, 0xfecfff, MRA16_RAM },
	{ 0xff0000, 0xff3fff, MRA16_RAM },
	{ 0xff4000, 0xff7fff, MRA16_RAM },
	{ 0xff8000, 0xff87ff, MRA16_RAM },
	{ 0xffc000, 0xfffff7, MRA16_RAM },                /* working RAM */
	{ 0xfffff8, 0xfffff9, hacked_soundcommand_r },      /* hack */
	{ 0xfffffa, 0xffffff, hacked_controls_r },      /* hack */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xfe0000, 0xfe07ff, MWA16_RAM },	/* RAM? */
	{ 0xfe0800, 0xfe0cff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0xfe0d00, 0xfe3fff, MWA16_RAM },              /* RAM? */
	{ 0xfe4000, 0xfe4001, bionicc_gfxctrl_w },	/* + coin counters */
	{ 0xfe8010, 0xfe8017, bionicc_scroll_w },
	{ 0xfe801a, 0xfe801b, bionicc_mpu_trigger_w },	/* ??? not sure, but looks like it */
	{ 0xfec000, 0xfecfff, bionicc_txvideoram_w, &bionicc_txvideoram },
	{ 0xff0000, 0xff3fff, bionicc_fgvideoram_w, &bionicc_fgvideoram },
	{ 0xff4000, 0xff7fff, bionicc_bgvideoram_w, &bionicc_bgvideoram },
	{ 0xff8000, 0xff87ff, bionicc_paletteram_w, &paletteram16 },
	{ 0xffc000, 0xfffff7, MWA16_RAM },	/* working RAM */
	{ 0xfffff8, 0xfffff9, hacked_soundcommand_w },      /* hack */
	{ 0xfffffa, 0xffffff, hacked_controls_w },	/* hack */
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8001, 0x8001, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8000, YM2151_register_port_0_w },
	{ 0x8001, 0x8001, YM2151_data_port_0_w },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END



INPUT_PORTS_START( bionicc )
	PORT_START
	PORT_BIT( 0x0fff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_6C ) )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_DIPSETTING(      0x0000, "7" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x1800, "20K, 40K, every 60K")
	PORT_DIPSETTING(      0x1000, "30K, 50K, every 70K" )
	PORT_DIPSETTING(      0x0800, "20K and 60K only")
	PORT_DIPSETTING(      0x0000, "30K and 70K only" )
	PORT_DIPNAME( 0x6000, 0x4000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x4000, "Easy" )
	PORT_DIPSETTING(      0x6000, "Medium")
	PORT_DIPSETTING(      0x2000, "Hard")
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x8000, 0x8000, "Freeze" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/********************************************************************

  GRAPHICS

********************************************************************/


static struct GfxLayout spritelayout_bionicc=
{
	16,16,  /* 16*16 sprites */
	2048,   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 0x30000*8,0x20000*8,0x10000*8,0 },
	{
		0,1,2,3,4,5,6,7,
		(16*8)+0,(16*8)+1,(16*8)+2,(16*8)+3,
		(16*8)+4,(16*8)+5,(16*8)+6,(16*8)+7
	},
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
	},
	256   /* every sprite takes 256 consecutive bytes */
};

static struct GfxLayout vramlayout_bionicc=
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 character */
	2,      /* 2 bitplanes */
	{ 4,0 },
	{ 0,1,2,3,8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128   /* every character takes 128 consecutive bytes */
};

static struct GfxLayout scroll2layout_bionicc=
{
	8,8,    /* 8*8 tiles */
	2048,   /* 2048 tiles */
	4,      /* 4 bits per pixel */
	{ (0x08000*8)+4,0x08000*8,4,0 },
	{ 0,1,2,3, 8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128   /* every tile takes 128 consecutive bytes */
};

static struct GfxLayout scroll1layout_bionicc=
{
	16,16,  /* 16*16 tiles */
	2048,   /* 2048 tiles */
	4,      /* 4 bits per pixel */
	{ (0x020000*8)+4,0x020000*8,4,0 },
	{
		0,1,2,3, 8,9,10,11,
		(8*4*8)+0,(8*4*8)+1,(8*4*8)+2,(8*4*8)+3,
		(8*4*8)+8,(8*4*8)+9,(8*4*8)+10,(8*4*8)+11
	},
	{
		0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
	},
	512   /* each tile takes 512 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo_bionicc[] =
{
	{ REGION_GFX1, 0, &vramlayout_bionicc,    768, 64 },	/* colors 768-1023 */
	{ REGION_GFX2, 0, &scroll2layout_bionicc,   0,  4 },	/* colors   0-  63 */
	{ REGION_GFX3, 0, &scroll1layout_bionicc, 256,  4 },	/* colors 256- 319 */
	{ REGION_GFX4, 0, &spritelayout_bionicc,  512, 16 },	/* colors 512- 767 */
	{ -1 }
};


static struct YM2151interface ym2151_interface =
{
	1,                      /* 1 chip */
	3579545,                /* 3.579545 MHz ? */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct MachineDriver machine_driver_bionicc =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000, /* ?? MHz ? */
			readmem,writemem,0,0,
			bionicc_interrupt,8
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,  /* 4 MHz ??? TODO: find real FRQ */
			sound_readmem,sound_writemem,0,0,
			nmi_interrupt,4	/* ??? */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo_bionicc,
	1024, 1024,	/* but a lot are not used */
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_BUFFERS_SPRITERAM,
	bionicc_eof_callback,
	bionicc_vh_start,
	0,
	bionicc_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
	}
};



ROM_START( bionicc )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "tsu_02b.rom",  0x00000, 0x10000, 0xcf965a0a ) /* 68000 code */
	ROM_LOAD16_BYTE( "tsu_04b.rom",  0x00001, 0x10000, 0xc9884bfb ) /* 68000 code */
	ROM_LOAD16_BYTE( "tsu_03b.rom",  0x20000, 0x10000, 0x4e157ae2 ) /* 68000 code */
	ROM_LOAD16_BYTE( "tsu_05b.rom",  0x20001, 0x10000, 0xe66ca0f9 ) /* 68000 code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "tsu_01b.rom",  0x00000, 0x8000, 0xa9a6cafa )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_08.rom",   0x00000, 0x8000, 0x9bf0b7a2 )	/* VIDEORAM (text layer) tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_07.rom",   0x00000, 0x8000, 0x9469efa4 )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "tsu_06.rom",   0x08000, 0x8000, 0x40bf0eb4 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_12.rom",    0x00000, 0x8000, 0xe4b4619e )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "ts_11.rom",    0x08000, 0x8000, 0xab30237a )
	ROM_LOAD( "ts_17.rom",    0x10000, 0x8000, 0xdeb657e4 )
	ROM_LOAD( "ts_16.rom",    0x18000, 0x8000, 0xd363b5f9 )
	ROM_LOAD( "ts_13.rom",    0x20000, 0x8000, 0xa8f5a004 )
	ROM_LOAD( "ts_18.rom",    0x28000, 0x8000, 0x3b36948c )
	ROM_LOAD( "ts_23.rom",    0x30000, 0x8000, 0xbbfbe58a )
	ROM_LOAD( "ts_24.rom",    0x38000, 0x8000, 0xf156e564 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_10.rom",   0x00000, 0x8000, 0xf1180d02 )	/* Sprites */
	ROM_LOAD( "tsu_09.rom",   0x08000, 0x8000, 0x6a049292 )
	ROM_LOAD( "tsu_15.rom",   0x10000, 0x8000, 0xea912701 )
	ROM_LOAD( "tsu_14.rom",   0x18000, 0x8000, 0x46b2ad83 )
	ROM_LOAD( "tsu_20.rom",   0x20000, 0x8000, 0x17857ad2 )
	ROM_LOAD( "tsu_19.rom",   0x28000, 0x8000, 0xb5c82722 )
	ROM_LOAD( "tsu_22.rom",   0x30000, 0x8000, 0x5ee1ae6a )
	ROM_LOAD( "tsu_21.rom",   0x38000, 0x8000, 0x98777006 )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.18f",   0x0000, 0x0100, 0xb58d0023 )	/* priority (not used) */
ROM_END

ROM_START( bionicc2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "02",      0x00000, 0x10000, 0xf2528f08 ) /* 68000 code */
	ROM_LOAD16_BYTE( "04",      0x00001, 0x10000, 0x38b1c7e4 ) /* 68000 code */
	ROM_LOAD16_BYTE( "03",      0x20000, 0x10000, 0x72c3b76f ) /* 68000 code */
	ROM_LOAD16_BYTE( "05",      0x20001, 0x10000, 0x70621f83 ) /* 68000 code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "tsu_01b.rom",  0x00000, 0x8000, 0xa9a6cafa )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_08.rom",   0x00000, 0x8000, 0x9bf0b7a2 )	/* VIDEORAM (text layer) tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_07.rom",   0x00000, 0x8000, 0x9469efa4 )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "tsu_06.rom",   0x08000, 0x8000, 0x40bf0eb4 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_12.rom",    0x00000, 0x8000, 0xe4b4619e )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "ts_11.rom",    0x08000, 0x8000, 0xab30237a )
	ROM_LOAD( "ts_17.rom",    0x10000, 0x8000, 0xdeb657e4 )
	ROM_LOAD( "ts_16.rom",    0x18000, 0x8000, 0xd363b5f9 )
	ROM_LOAD( "ts_13.rom",    0x20000, 0x8000, 0xa8f5a004 )
	ROM_LOAD( "ts_18.rom",    0x28000, 0x8000, 0x3b36948c )
	ROM_LOAD( "ts_23.rom",    0x30000, 0x8000, 0xbbfbe58a )
	ROM_LOAD( "ts_24.rom",    0x38000, 0x8000, 0xf156e564 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tsu_10.rom",   0x00000, 0x8000, 0xf1180d02 )	/* Sprites */
	ROM_LOAD( "tsu_09.rom",   0x08000, 0x8000, 0x6a049292 )
	ROM_LOAD( "tsu_15.rom",   0x10000, 0x8000, 0xea912701 )
	ROM_LOAD( "tsu_14.rom",   0x18000, 0x8000, 0x46b2ad83 )
	ROM_LOAD( "tsu_20.rom",   0x20000, 0x8000, 0x17857ad2 )
	ROM_LOAD( "tsu_19.rom",   0x28000, 0x8000, 0xb5c82722 )
	ROM_LOAD( "tsu_22.rom",   0x30000, 0x8000, 0x5ee1ae6a )
	ROM_LOAD( "tsu_21.rom",   0x38000, 0x8000, 0x98777006 )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.18f",   0x0000, 0x0100, 0xb58d0023 )	/* priority (not used) */
ROM_END

ROM_START( topsecrt )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "ts_02.rom",  0x00000, 0x10000, 0xb2fe1ddb ) /* 68000 code */
	ROM_LOAD16_BYTE( "ts_04.rom",  0x00001, 0x10000, 0x427a003d ) /* 68000 code */
	ROM_LOAD16_BYTE( "ts_03.rom",  0x20000, 0x10000, 0x27f04bb6 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ts_05.rom",  0x20001, 0x10000, 0xc01547b1 ) /* 68000 code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "ts_01.rom",    0x00000, 0x8000, 0x8ea07917 )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_08.rom",    0x00000, 0x8000, 0x96ad379e )	/* VIDEORAM (text layer) tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_07.rom",    0x00000, 0x8000, 0x25cdf8b2 )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "ts_06.rom",    0x08000, 0x8000, 0x314fb12d )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_12.rom",    0x00000, 0x8000, 0xe4b4619e )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "ts_11.rom",    0x08000, 0x8000, 0xab30237a )
	ROM_LOAD( "ts_17.rom",    0x10000, 0x8000, 0xdeb657e4 )
	ROM_LOAD( "ts_16.rom",    0x18000, 0x8000, 0xd363b5f9 )
	ROM_LOAD( "ts_13.rom",    0x20000, 0x8000, 0xa8f5a004 )
	ROM_LOAD( "ts_18.rom",    0x28000, 0x8000, 0x3b36948c )
	ROM_LOAD( "ts_23.rom",    0x30000, 0x8000, 0xbbfbe58a )
	ROM_LOAD( "ts_24.rom",    0x38000, 0x8000, 0xf156e564 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "ts_10.rom",    0x00000, 0x8000, 0xc3587d05 )	/* Sprites */
	ROM_LOAD( "ts_09.rom",    0x08000, 0x8000, 0x6b63eef2 )
	ROM_LOAD( "ts_15.rom",    0x10000, 0x8000, 0xdb8cebb0 )
	ROM_LOAD( "ts_14.rom",    0x18000, 0x8000, 0xe2e41abf )
	ROM_LOAD( "ts_20.rom",    0x20000, 0x8000, 0xbfd1a695 )
	ROM_LOAD( "ts_19.rom",    0x28000, 0x8000, 0x928b669e )
	ROM_LOAD( "ts_22.rom",    0x30000, 0x8000, 0x3fe05d9a )
	ROM_LOAD( "ts_21.rom",    0x38000, 0x8000, 0x27a9bb7c )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.18f",   0x0000, 0x0100, 0xb58d0023 )	/* priority (not used) */
ROM_END



GAME( 1987, bionicc,  0,       bionicc, bionicc, 0, ROT0, "Capcom", "Bionic Commando (US set 1)" )
GAME( 1987, bionicc2, bionicc, bionicc, bionicc, 0, ROT0, "Capcom", "Bionic Commando (US set 2)" )
GAME( 1987, topsecrt, bionicc, bionicc, bionicc, 0, ROT0, "Capcom", "Top Secret (Japan)" )
