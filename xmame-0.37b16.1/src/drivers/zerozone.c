/***************************************************************************

Zero Zone memory map

driver by Brad Oliver

CPU 1 : 68000, uses irq 1

0x000000 - 0x01ffff : ROM
0x080000 - 0x08000f : input ports and dipswitches
0x088000 - 0x0881ff : palette RAM, 256 total colors
0x09ce00 - 0x09d9ff : video ram, 48x32
0x0c0000 - 0x0cffff : RAM
0x0f8000 - 0x0f87ff : RAM (unused?)

TODO:
	* adpcm samples don't seem to be playing at the proper tempo - too fast?
	* There are a lot of unknown dipswitches

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

void zerozone_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int zerozone_vh_start(void);
void zerozone_vh_stop(void);
WRITE16_HANDLER( zerozone_videoram_w );

extern data16_t *zerozone_videoram;

static READ16_HANDLER( zerozone_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(0); /* IN0 */
		case 0x01:
			return (readinputport(1) | (readinputport(2) << 8)); /* IN1 & IN2 */
		case 0x04:
			return (readinputport(4) << 8);
		case 0x05:
			return readinputport(3);
	}

logerror("CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),0x800000+offset);

	return 0x00;
}


WRITE16_HANDLER( zerozone_sound_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(offset,data >> 8);
		cpu_cause_interrupt(1,0xff);
	}
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x080000, 0x08000f, zerozone_input_r },
	{ 0x088000, 0x0881ff, MRA16_RAM },
/*	{ 0x098000, 0x098001, MRA16_RAM },    watchdog?    */
	{ 0x09ce00, 0x09d9ff, MRA16_RAM },
	{ 0x0c0000, 0x0cffff, MRA16_RAM },
	{ 0x0f8000, 0x0f87ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x084000, 0x084001, zerozone_sound_w },
	{ 0x088000, 0x0881ff, paletteram16_BBBBGGGGRRRRxxxx_word_w, &paletteram16 },
	{ 0x09ce00, 0x09d9ff, zerozone_videoram_w, &zerozone_videoram, &videoram_size },
	{ 0x0c0000, 0x0cffff, MWA16_RAM }, /* RAM */
	{ 0x0f8000, 0x0f87ff, MWA16_RAM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

INPUT_PORTS_START( zerozone )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 6C_1C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xc0, "1")
	PORT_DIPSETTING(    0x80, "2")
	PORT_DIPSETTING(    0x40, "3")
	PORT_DIPSETTING(    0x00, "4")

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x0c, "1")
	PORT_DIPSETTING(    0x04, "2")
	PORT_DIPSETTING(    0x08, "3")
	PORT_DIPSETTING(    0x00, "4")
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8+0, 8+4, 16+0, 16+4, 24+0, 24+4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 256 },         /* sprites & playfield */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 8000 },           /* 8000Hz ??? TODO: find out the real frequency */
	{ REGION_SOUND1 },	/* memory region 3 */
	{ 100 }
};

static struct MachineDriver machine_driver_zerozone =
{
	{
		{
			CPU_M68000,
			10000000,	/* 10 MHz */
			readmem,writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1000000,	/* 1 MHz ??? */
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main cpu */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	48*8, 32*8, { 1*8, 47*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
	0,
	zerozone_vh_start,
	zerozone_vh_stop,
	zerozone_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zerozone )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 128k for 68000 code */
	ROM_LOAD16_BYTE( "zz-4.rom", 0x0000, 0x10000, 0x83718b9b )
	ROM_LOAD16_BYTE( "zz-5.rom", 0x0001, 0x10000, 0x18557f41 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* sound cpu */
	ROM_LOAD( "zz-1.rom", 0x00000, 0x08000, 0x223ccce5 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zz-6.rom", 0x00000, 0x80000, 0xc8b906b9 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )      /* ADPCM samples */
	ROM_LOAD( "zz-2.rom", 0x00000, 0x20000, 0xc7551e81 )
	ROM_LOAD( "zz-3.rom", 0x20000, 0x20000, 0xe348ff5e )
ROM_END



GAME( 1993, zerozone, 0, zerozone, zerozone, 0, ROT0, "Comad", "Zero Zone" )
