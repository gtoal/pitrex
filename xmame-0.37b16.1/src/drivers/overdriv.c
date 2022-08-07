/***************************************************************************

Over Drive (GX789) (c) 1990 Konami

driver by Nicola Salmoria

Notes:
- Missing road (two unemulated K053250)
- Visible area and relative placement of sprites and tiles is most likely wrong.
- Test mode doesn't work well with 3 IRQ5 per frame, the ROM check doens't work
  and the coin A setting isn't shown. It's OK with 1 IRQ5 per frame.
- Some flickering sprites, this might be an interrupt/timing issue
- The screen is cluttered with sprites which aren't supposed to be visible,
  increasing the coordinate mask in K053247_sprites_draw() from 0x3ff to 0xfff
  fixes this but breaks other games (e.g. Vendetta).
- The "Continue?" sprites are not visible until you press start
- priorities

***************************************************************************/

#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "machine/eeprom.h"
#include "cpu/m6809/m6809.h"


int overdriv_vh_start(void);
void overdriv_vh_stop(void);
void overdriv_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);




static READ16_HANDLER( K051316_0_msb_r )
{
	return K051316_0_r(offset) << 8;
}

static READ16_HANDLER( K051316_1_msb_r )
{
	return K051316_1_r(offset) << 8;
}

static READ16_HANDLER( K051316_rom_0_msb_r )
{
	return K051316_rom_0_r(offset) << 8;
}

static READ16_HANDLER( K051316_rom_1_msb_r )
{
	return K051316_rom_1_r(offset) << 8;
}

static WRITE16_HANDLER( K051316_0_msb_w )
{
	if (ACCESSING_MSB)
		K051316_0_w(offset,data >> 8);
}

static WRITE16_HANDLER( K051316_1_msb_w )
{
	if (ACCESSING_MSB)
		K051316_1_w(offset,data >> 8);
}

static WRITE16_HANDLER( K051316_ctrl_0_msb_w )
{
	if (ACCESSING_MSB)
		K051316_ctrl_0_w(offset,data >> 8);
}

static WRITE16_HANDLER( K051316_ctrl_1_msb_w )
{
	if (ACCESSING_MSB)
		K051316_ctrl_1_w(offset,data >> 8);
}


/***************************************************************************

  EEPROM

***************************************************************************/

static data8_t default_eeprom[128] =
{
	0x77,0x58,0xFF,0xFF,0x00,0x78,0x90,0x00,0x00,0x78,0x70,0x00,0x00,0x78,0x50,0x00,
	0x54,0x41,0x4B,0x51,0x31,0x36,0x46,0x55,0x4A,0xFF,0x03,0x00,0x02,0x70,0x02,0x50,
	0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,
	0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,
	0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,
	0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,
	0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,
	0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03,0x00,0xB4,0x03
};


static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"011000",		/*  read command */
	"010100",		/* write command */
	0,				/* erase command */
	"010000000000",	/* lock command */
	"010011000000"	/* unlock command */
};

static void nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
			EEPROM_load(file);
		else
			EEPROM_set_data(default_eeprom,sizeof(default_eeprom));
	}
}

static READ16_HANDLER( eeprom_r )
{
	int res;

/*logerror("%06x eeprom_r\n",cpu_get_pc()); */
	/* bit 6 is EEPROM data */
	res = (EEPROM_read_bit() << 6) | input_port_0_word_r(0,0);

	return res;
}

static WRITE16_HANDLER( eeprom_w )
{
/*logerror("%06x: write %04x to eeprom_w\n",cpu_get_pc(),data); */
	if (ACCESSING_LSB)
	{
		/* bit 0 is data */
		/* bit 1 is clock (active high) */
		/* bit 2 is cs (active low) */
		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE);
	}
}





static int cpuA_interrupt(void)
{
	if (cpu_getiloops()) return 5;
	return 4;
}

static int cpuB_interrupt(void)
{
	if (K053246_is_IRQ_enabled()) return 4;
	else return 0;
}


static void overdriv_init(void)
{
	/* start with cpu B halted */
	cpu_set_reset_line(1,ASSERT_LINE);
}

static WRITE16_HANDLER( cpuA_ctrl_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 0 probably enables the second 68000 */
		cpu_set_reset_line(1,(data & 0x01) ? CLEAR_LINE : ASSERT_LINE);

		/* bit 1 is clear during service mode - function unknown */

		set_led_status(0,data & 0x08);
		coin_counter_w(0,data & 0x10);
		coin_counter_w(1,data & 0x20);

/*logerror("%06x: write %04x to cpuA_ctrl_w\n",cpu_get_pc(),data); */
	}
}


static data16_t cpuB_ctrl;

static READ16_HANDLER( cpuB_ctrl_r )
{
	return cpuB_ctrl;
}

static WRITE16_HANDLER( cpuB_ctrl_w )
{
	COMBINE_DATA(&cpuB_ctrl);

	if (ACCESSING_LSB)
	{
		/* bit 0 = enable sprite ROM reading */
		K053246_set_OBJCHA_line((data & 0x01) ? ASSERT_LINE : CLEAR_LINE);

		/* bit 1 used but unknown (irq enable?) */

		/* other bits unused? */
	}
}


static data16_t *sharedram;

static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}



static READ16_HANDLER( overdriv_sound_0_r )
{
	return K053260_0_r(2 + offset);
}

static READ16_HANDLER( overdriv_sound_1_r )
{
	return K053260_1_r(2 + offset);
}

static WRITE16_HANDLER( overdriv_soundirq_w )
{
	cpu_cause_interrupt(2,M6809_INT_IRQ);
}

static WRITE16_HANDLER( overdriv_cpuB_irq5_w )
{
	cpu_cause_interrupt(1,5);
}

static WRITE16_HANDLER( overdriv_cpuB_irq6_w )
{
	cpu_cause_interrupt(1,6);
}




static MEMORY_READ16_START( overdriv_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, MRA16_RAM },
	{ 0x080000, 0x080fff, MRA16_RAM },
	{ 0x0c0000, 0x0c0001, eeprom_r },
	{ 0x0c0002, 0x0c0003, input_port_1_word_r },
	{ 0x180000, 0x180001, input_port_2_word_r },
	{ 0x1d8000, 0x1d8003, overdriv_sound_0_r },	/* K053260 */
	{ 0x1e0000, 0x1e0003, overdriv_sound_1_r },	/* K053260 */
	{ 0x200000, 0x203fff, sharedram_r },
	{ 0x210000, 0x210fff, K051316_0_msb_r },
	{ 0x218000, 0x218fff, K051316_1_msb_r },
	{ 0x220000, 0x220fff, K051316_rom_0_msb_r },
	{ 0x228000, 0x228fff, K051316_rom_1_msb_r },
MEMORY_END

static MEMORY_WRITE16_START( overdriv_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x040000, 0x043fff, MWA16_RAM },	/* work RAM */
	{ 0x080000, 0x080fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x0e0000, 0x0e0001, MWA16_NOP },	/* unknown (always 0x30) */
	{ 0x100000, 0x10001f, MWA16_NOP },	/* 053252? (LSB) */
	{ 0x140000, 0x140001, watchdog_reset16_w },
	{ 0x1c0000, 0x1c001f, K051316_ctrl_0_msb_w },
	{ 0x1c8000, 0x1c801f, K051316_ctrl_1_msb_w },
	{ 0x1d0000, 0x1d001f, K053251_msb_w },
	{ 0x1d8000, 0x1d8003, K053260_0_lsb_w },
	{ 0x1e0000, 0x1e0003, K053260_1_lsb_w },
	{ 0x1e8000, 0x1e8001, overdriv_soundirq_w },
	{ 0x1f0000, 0x1f0001, cpuA_ctrl_w },	/* halt cpu B, coin counter, start lamp, other? */
	{ 0x1f8000, 0x1f8001, eeprom_w },
	{ 0x200000, 0x203fff, sharedram_w, &sharedram },
	{ 0x210000, 0x210fff, K051316_0_msb_w },
	{ 0x218000, 0x218fff, K051316_1_msb_w },
	{ 0x230000, 0x230001, overdriv_cpuB_irq6_w },
	{ 0x238000, 0x238001, overdriv_cpuB_irq5_w },
MEMORY_END


static MEMORY_READ16_START( overdriv_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x083fff, MRA16_RAM },
{ 0x0c0000, 0x0c1fff, MRA16_RAM },
{ 0x100000, 0x10000f, MRA16_NOP },	/* K053250 #0 */
{ 0x108000, 0x10800f, MRA16_NOP },	/* K053250 #1 */
	{ 0x118000, 0x118fff, K053247_word_r },
	{ 0x120000, 0x120001, K053246_word_r },
	{ 0x128000, 0x128001, cpuB_ctrl_r },
	{ 0x200000, 0x203fff, sharedram_r },
{ 0x208000, 0x20bfff, MRA16_RAM },

{ 0x218000, 0x219fff, MRA16_NOP },	/* K053250 #0 gfx ROM read (LSB) */
{ 0x220000, 0x221fff, MRA16_NOP },	/* K053250 #1 gfx ROM read (LSB) */
MEMORY_END

static MEMORY_WRITE16_START( overdriv_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x083fff, MWA16_RAM },	/* work RAM */
{ 0x0c0000, 0x0c1fff, MWA16_RAM },
{ 0x100000, 0x10000f, MWA16_NOP },	/* K053250 #0 */
{ 0x108000, 0x10800f, MWA16_NOP },	/* K053250 #1 */
	{ 0x118000, 0x118fff, K053247_word_w },
	{ 0x128000, 0x128001, cpuB_ctrl_w },	/* enable K053247 ROM reading, plus something else */
	{ 0x130000, 0x130007, K053246_word_w },
	{ 0x200000, 0x203fff, sharedram_w },
{ 0x208000, 0x20bfff, MWA16_RAM },
MEMORY_END


static MEMORY_READ_START( overdriv_s_readmem )
	{ 0x0201, 0x0201, YM2151_status_port_0_r },
	{ 0x0400, 0x042f, K053260_0_r },
	{ 0x0600, 0x062f, K053260_1_r },
	{ 0x0800, 0x0fff, MRA_RAM },
	{ 0x1000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( overdriv_s_writemem )
	{ 0x0200, 0x0200, YM2151_register_port_0_w },
	{ 0x0201, 0x0201, YM2151_data_port_0_w },
	{ 0x0400, 0x042f, K053260_0_w },
	{ 0x0600, 0x062f, K053260_1_w },
	{ 0x0800, 0x0fff, MWA_RAM },
	{ 0x1000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( overdriv )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* ? */

	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_DIAL | IPF_CENTER, 100, 50, 0, 0 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ STEP8(0,4) },
	{ STEP8(7*8*4,-8*4) },
	8*8*4
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX4, 0, &charlayout, 0, 0x80 },
	{ REGION_GFX5, 0, &charlayout, 0, 0x80 },
	{ -1 }
};



static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct K053260_interface k053260_interface =
{
	2,
	{ 3579545, 3579545 },
	{ REGION_SOUND1, REGION_SOUND1 }, /* memory region */
	{ { MIXER(70,MIXER_PAN_LEFT), MIXER(70,MIXER_PAN_RIGHT) }, { MIXER(70,MIXER_PAN_LEFT), MIXER(70,MIXER_PAN_RIGHT) } },
	{ 0, 0 }
};



static struct MachineDriver machine_driver_overdriv =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			24000000/2,	/* 12 MHz */
			overdriv_readmem,overdriv_writemem,0,0,
			cpuA_interrupt,4	/* ??? IRQ 4 is vblank, IRQ 5 of unknown origin */
		},
		{
			CPU_M68000,
			24000000/2,	/* 12 MHz */
			overdriv_readmem2,overdriv_writemem2,0,0,
			cpuB_interrupt,1	/* IRQ 5 and 6 are generated by the main CPU. */
								/* IRQ 5 is used only in test mode, to request the checksums of the gfx ROMs. */
		},
		{
			CPU_M6809,
			3579545/2,	/* 1.789 MHz?? This might be the right speed, but ROM testing */
						/* takes a little too much (the counter wraps from 0000 to 9999). */
						/* This might just mean that the video refresh rate is less than */
						/* 60 fps, that's how I fixed it for now. */
			overdriv_s_readmem,overdriv_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
		}
	},
	59, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	200, /* CPU slices */
	overdriv_init,

	/* video hardware */
	64*8, 32*8, { 13*8, (64-13)*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,	/* gfx decoded by konamiic.c */
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	overdriv_vh_start,
	overdriv_vh_stop,
	overdriv_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_K053260,
			&k053260_interface,
		}
	},

	nvram_handler
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( overdriv )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "789.2",        0x00000, 0x20000, 0x77f18f3f )
	ROM_LOAD16_BYTE( "789.1",        0x00001, 0x20000, 0x4f44e6ad )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "789.4",        0x00000, 0x20000, 0x46fb7e88 )
	ROM_LOAD16_BYTE( "789.3",        0x00001, 0x20000, 0x24427195 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "789.5",        0x00000, 0x10000, 0x1085f069 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* graphics (addressable by the CPU) */
	ROM_LOAD( "e12.r1",       0x000000, 0x100000, 0x14a10fb2 )	/* sprites */
	ROM_LOAD( "e13.r4",       0x100000, 0x100000, 0x6314a628 )
	ROM_LOAD( "e14.r10",      0x200000, 0x100000, 0xb5eca14b )
	ROM_LOAD( "e15.r15",      0x300000, 0x100000, 0x5d93e0c3 )

	ROM_REGION( 0x020000, REGION_GFX2, 0 )	/* graphics (addressable by the CPU) */
	ROM_LOAD( "e06.a21",      0x000000, 0x020000, 0x14a085e6 )	/* zoom/rotate */

	ROM_REGION( 0x020000, REGION_GFX3, 0 )	/* graphics (addressable by the CPU) */
	ROM_LOAD( "e07.c23",      0x000000, 0x020000, 0x8a6ceab9 )	/* zoom/rotate */

	ROM_REGION( 0x0c0000, REGION_GFX4, 0 )	/* graphics (addressable by the CPU) */
	ROM_LOAD( "e18.p22",      0x000000, 0x040000, 0x985a4a75 )	/* 053250 #0 */
	ROM_LOAD( "e19.r22",      0x040000, 0x040000, 0x15c54ea2 )
	ROM_LOAD( "e20.s22",      0x080000, 0x040000, 0xea204acd )

	ROM_REGION( 0x080000, REGION_GFX5, 0 )	/* unknown (053250?) */
	ROM_LOAD( "e16.p12",      0x000000, 0x040000, 0x9348dee1 )	/* 053250 #1 */
	ROM_LOAD( "e17.p17",      0x040000, 0x040000, 0x04c07248 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* 053260 samples */
	ROM_LOAD( "e03.j1",       0x000000, 0x100000, 0x51ebfebe )
	ROM_LOAD( "e02.f1",       0x100000, 0x100000, 0xbdd3b5c6 )
ROM_END



static void init_overdriv(void)
{
	konami_rom_deinterleave_4(REGION_GFX1);
}



GAMEX( 1990, overdriv, 0, overdriv, overdriv, overdriv, ROT90_16BIT, "Konami", "Over Drive", GAME_IMPERFECT_GRAPHICS )
