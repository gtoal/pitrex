/***************************************************************************

	Atari Shuuz hardware

	driver by Aaron Giles

	Games supported:
		* Shuuz (1990) [2 sets]

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Externals
 *
 *************************************/

int shuuz_vh_start(void);
void shuuz_vh_stop(void);
void shuuz_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void shuuz_scanline_update(int scanline);



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate = 4;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarivc_reset(atarivc_eof_data);
	atarigen_interrupt_reset(update_interrupts);
}


static WRITE16_HANDLER( latch_w )
{
}



/*************************************
 *
 *	LETA I/O
 *
 *************************************/

static READ16_HANDLER( leta_r )
{
	/* trackball -- rotated 45 degrees? */
	static int cur[2];
	int which = offset & 1;

	/* when reading the even ports, do a real analog port update */
	if (which == 0)
	{
		int dx = (INT8)readinputport(2);
		int dy = (INT8)readinputport(3);

		cur[0] = dx + dy;
		cur[1] = dx - dy;
	}

	/* clip the result to -0x3f to +0x3f to remove directional ambiguities */
	return cur[which];
}



/*************************************
 *
 *	MSM5295 I/O
 *
 *************************************/

static READ16_HANDLER( adpcm_r )
{
	return OKIM6295_status_0_r(offset) | 0xff00;
}


static WRITE16_HANDLER( adpcm_w )
{
	if (ACCESSING_LSB)
		OKIM6295_data_0_w(offset, data & 0xff);
}



/*************************************
 *
 *	Additional I/O
 *
 *************************************/

static READ16_HANDLER( special_port0_r )
{
	int result = readinputport(0);

	if ((result & 0x0800) && atarigen_get_hblank())
		result &= ~0x0800;

	return result;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x100fff, atarigen_eeprom_r },
	{ 0x103000, 0x103003, leta_r },
	{ 0x105000, 0x105001, special_port0_r },
	{ 0x105002, 0x105003, input_port_1_word_r },
	{ 0x106000, 0x106001, adpcm_r },
	{ 0x107000, 0x107007, MRA16_NOP },
	{ 0x3e0000, 0x3e087f, MRA16_RAM },
	{ 0x3effc0, 0x3effff, atarivc_r },
	{ 0x3f4000, 0x3fffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x100fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x101000, 0x101fff, atarigen_eeprom_enable_w },
	{ 0x102000, 0x102001, watchdog_reset16_w },
	{ 0x105000, 0x105001, latch_w },
	{ 0x106000, 0x106001, adpcm_w },
	{ 0x107000, 0x107007, MWA16_NOP },
	{ 0x3e0000, 0x3e087f, atarigen_666_paletteram_w, &paletteram16 },
	{ 0x3effc0, 0x3effff, atarivc_w, &atarivc_data },
	{ 0x3f4000, 0x3f5eff, ataripf_0_latched_w, &ataripf_0_base },
	{ 0x3f5f00, 0x3f5f7f, MWA16_RAM, &atarivc_eof_data },
	{ 0x3f5f80, 0x3f5fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x3f6000, 0x3f7fff, ataripf_0_upper_msb_w, &ataripf_0_upper },
	{ 0x3f8000, 0x3fcfff, MWA16_RAM },
	{ 0x3fd000, 0x3fd3ff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3fd400, 0x3fffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( shuuz )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x07fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x07fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0800, IP_ACTIVE_LOW )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_X | IPF_PLAYER1, 50, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER1, 50, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( shuuz2 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(  0x0100, IP_ACTIVE_LOW, 0, "Step Debug SW", KEYCODE_S, IP_JOY_NONE )
	PORT_BIT( 0x0600, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BITX(  0x1000, IP_ACTIVE_LOW, 0, "Playfield Debug SW", KEYCODE_Y, IP_JOY_NONE )
	PORT_BITX(  0x2000, IP_ACTIVE_LOW, 0, "Reset Debug SW", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(  0x4000, IP_ACTIVE_LOW, 0, "Crosshair Debug SW", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(  0x8000, IP_ACTIVE_LOW, 0, "Freeze Debug SW", KEYCODE_F, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x00fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(  0x0100, IP_ACTIVE_LOW, 0, "Replay Debug SW", KEYCODE_R, IP_JOY_NONE )
	PORT_BIT( 0x0600, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0800, IP_ACTIVE_LOW )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START
    PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_X | IPF_PLAYER1, 50, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0xff, 0, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER1, 50, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pfmolayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, 0+RGN_FRAC(1,2), 4+RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pfmolayout,  256, 16 },		/* sprites & playfield */
	{ REGION_GFX2, 0, &pfmolayout,    0, 16 },		/* sprites & playfield */
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ ATARI_CLOCK_14MHz/16/132 },
	{ REGION_SOUND1 },
	{ 100 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_shuuz =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz/2,
			main_readmem,main_writemem,0,0,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	shuuz_vh_start,
	shuuz_vh_stop,
	shuuz_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	},

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( shuuz )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "4010.23p",     0x00000, 0x20000, 0x1c2459f8 )
	ROM_LOAD16_BYTE( "4011.13p",     0x00001, 0x20000, 0x6db53a85 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2030.43x", 0x000000, 0x20000, 0x8ecf1ed8 )
	ROM_LOAD( "2032.20x", 0x020000, 0x20000, 0x5af184e6 )
	ROM_LOAD( "2031.87x", 0x040000, 0x20000, 0x72e9db63 )
	ROM_LOAD( "2033.65x", 0x060000, 0x20000, 0x8f552498 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1020.43u", 0x000000, 0x20000, 0xd21ad039 )
	ROM_LOAD( "1022.20u", 0x020000, 0x20000, 0x0c10bc90 )
	ROM_LOAD( "1024.43m", 0x040000, 0x20000, 0xadb09347 )
	ROM_LOAD( "1026.20m", 0x060000, 0x20000, 0x9b20e13d )
	ROM_LOAD( "1021.87u", 0x080000, 0x20000, 0x8388910c )
	ROM_LOAD( "1023.65u", 0x0a0000, 0x20000, 0x71353112 )
	ROM_LOAD( "1025.87m", 0x0c0000, 0x20000, 0xf7b20a64 )
	ROM_LOAD( "1027.65m", 0x0e0000, 0x20000, 0x55d54952 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM data */
	ROM_LOAD( "1040.75b", 0x00000, 0x20000, 0x0896702b )
	ROM_LOAD( "1041.65b", 0x20000, 0x20000, 0xb3b07ce9 )
ROM_END


ROM_START( shuuz2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "23p.rom",     0x00000, 0x20000, 0x98aec4e7 )
	ROM_LOAD16_BYTE( "13p.rom",     0x00001, 0x20000, 0xdd9d5d5c )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2030.43x", 0x000000, 0x20000, 0x8ecf1ed8 )
	ROM_LOAD( "2032.20x", 0x020000, 0x20000, 0x5af184e6 )
	ROM_LOAD( "2031.87x", 0x040000, 0x20000, 0x72e9db63 )
	ROM_LOAD( "2033.65x", 0x060000, 0x20000, 0x8f552498 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1020.43u", 0x000000, 0x20000, 0xd21ad039 )
	ROM_LOAD( "1022.20u", 0x020000, 0x20000, 0x0c10bc90 )
	ROM_LOAD( "1024.43m", 0x040000, 0x20000, 0xadb09347 )
	ROM_LOAD( "1026.20m", 0x060000, 0x20000, 0x9b20e13d )
	ROM_LOAD( "1021.87u", 0x080000, 0x20000, 0x8388910c )
	ROM_LOAD( "1023.65u", 0x0a0000, 0x20000, 0x71353112 )
	ROM_LOAD( "1025.87m", 0x0c0000, 0x20000, 0xf7b20a64 )
	ROM_LOAD( "1027.65m", 0x0e0000, 0x20000, 0x55d54952 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM data */
	ROM_LOAD( "1040.75b", 0x00000, 0x20000, 0x0896702b )
	ROM_LOAD( "1041.65b", 0x20000, 0x20000, 0xb3b07ce9 )
ROM_END



/*************************************
 *
 *	Driver init
 *
 *************************************/

static void init_shuuz(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_invert_region(REGION_GFX1);
	atarigen_invert_region(REGION_GFX2);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1990, shuuz,  0,     shuuz, shuuz,  shuuz, ROT0, "Atari Games", "Shuuz (version 8.0)" )
GAME( 1990, shuuz2, shuuz, shuuz, shuuz2, shuuz, ROT0, "Atari Games", "Shuuz (version 7.1)" )
