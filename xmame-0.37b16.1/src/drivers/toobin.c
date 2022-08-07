/***************************************************************************

	Atari Toobin' hardware

	driver by Aaron Giles

	Games supported:
		* Toobin' (1988) [3 sets]

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"



/*************************************
 *
 *	Externals
 *
 *************************************/

WRITE16_HANDLER( toobin_paletteram_w );
WRITE16_HANDLER( toobin_hscroll_w );
WRITE16_HANDLER( toobin_vscroll_w );

int toobin_vh_start(void);
void toobin_vh_stop(void);
void toobin_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern data16_t *toobin_intensity;



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t *interrupt_scan;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate |= 1;
	if (atarigen_sound_int_state)
		newstate |= 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarijsa_reset();
}



/*************************************
 *
 *	Interrupt handlers
 *
 *************************************/

static WRITE16_HANDLER( interrupt_scan_w )
{
	int oldword = interrupt_scan[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	/* if something changed, update the word in memory */
	if (oldword != newword)
	{
		interrupt_scan[offset] = newword;
		atarigen_scanline_int_set(newword & 0x1ff);
	}
}



/*************************************
 *
 *	I/O read dispatch
 *
 *************************************/

static READ16_HANDLER( special_port1_r )
{
	int result = readinputport(1);
	if (atarigen_get_hblank()) result ^= 0x8000;
	if (atarigen_cpu_to_sound_ready) result ^= 0x2000;
	return result;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0xc00000, 0xc09fff, MRA16_RAM },
	{ 0xc10000, 0xc107ff, MRA16_RAM },
	{ 0xff6000, 0xff6001, MRA16_NOP },		/* who knows? read at controls time */
	{ 0xff8800, 0xff8801, input_port_0_word_r },
	{ 0xff9000, 0xff9001, special_port1_r },
	{ 0xff9800, 0xff9801, atarigen_sound_r },
	{ 0xffa000, 0xffafff, atarigen_eeprom_r },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0xc00000, 0xc07fff, ataripf_0_large_w, &ataripf_0_base },
	{ 0xc08000, 0xc097ff, atarian_0_vram_w, &atarian_0_base },
	{ 0xc09800, 0xc09fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xc10000, 0xc107ff, toobin_paletteram_w, &paletteram16 },
	{ 0xff8000, 0xff8001, watchdog_reset16_w },
	{ 0xff8100, 0xff8101, atarigen_sound_w },
	{ 0xff8300, 0xff8301, MWA16_RAM, &toobin_intensity },
	{ 0xff8340, 0xff8341, interrupt_scan_w, &interrupt_scan },
	{ 0xff8380, 0xff8381, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0xff83c0, 0xff83c1, atarigen_scanline_int_ack_w },
	{ 0xff8400, 0xff8401, atarigen_sound_reset_w },
	{ 0xff8500, 0xff8501, atarigen_eeprom_enable_w },
	{ 0xff8600, 0xff8601, toobin_hscroll_w },
	{ 0xff8700, 0xff8701, toobin_vscroll_w },
	{ 0xffa000, 0xffafff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xffc000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( toobin )
	PORT_START	/* ff8800 */
	PORT_BITX(0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2, "P2 R Paddle Forward", KEYCODE_L, IP_JOY_DEFAULT )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2, "P2 L Paddle Forward", KEYCODE_J, IP_JOY_DEFAULT )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2, "P2 L Paddle Backward", KEYCODE_U, IP_JOY_DEFAULT )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "P2 R Paddle Backward", KEYCODE_O, IP_JOY_DEFAULT )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1, "P1 R Paddle Forward", KEYCODE_D, IP_JOY_DEFAULT )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1, "P1 L Paddle Forward", KEYCODE_A, IP_JOY_DEFAULT )
	PORT_BITX(0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1, "P1 L Paddle Backward", KEYCODE_Q, IP_JOY_DEFAULT )
	PORT_BITX(0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1, "P1 R Paddle Backward", KEYCODE_E, IP_JOY_DEFAULT )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Throw", KEYCODE_LCONTROL, IP_JOY_DEFAULT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Throw", KEYCODE_RCONTROL, IP_JOY_DEFAULT )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ff9000 */
	PORT_BIT( 0x03ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x1000, IP_ACTIVE_LOW )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_I_PORT	/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static struct GfxLayout molayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	8*64
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout,     0, 16 },
	{ REGION_GFX2, 0, &molayout,   256, 16 },
	{ REGION_GFX3, 0, &anlayout,   512, 64 },
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_toobin =
{
	/* basic machine hardware */
	{
		{
			CPU_M68010,		/* verified */
			ATARI_CLOCK_32MHz/4,
			main_readmem,main_writemem,0,0,
			ignore_interrupt,1
		},
		JSA_I_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	init_machine,

	/* video hardware */
	64*8, 48*8, { 0*8, 64*8-1, 0*8, 48*8-1 },
	gfxdecodeinfo,
	1024,1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	toobin_vh_start,
	toobin_vh_stop,
	toobin_vh_screenrefresh,

	/* sound hardware */
	JSA_I_STEREO_WITH_POKEY,

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( toobin )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "061-3133.bin", 0x00000, 0x10000, 0x79a92d02 )
	ROM_LOAD16_BYTE( "061-3137.bin", 0x00001, 0x10000, 0xe389ef60 )
	ROM_LOAD16_BYTE( "061-3134.bin", 0x20000, 0x10000, 0x3dbe9a48 )
	ROM_LOAD16_BYTE( "061-3138.bin", 0x20001, 0x10000, 0xa17fb16c )
	ROM_LOAD16_BYTE( "061-3135.bin", 0x40000, 0x10000, 0xdc90b45c )
	ROM_LOAD16_BYTE( "061-3139.bin", 0x40001, 0x10000, 0x6f8a719a )
	ROM_LOAD16_BYTE( "061-1136.bin", 0x60000, 0x10000, 0x5ae3eeac )
	ROM_LOAD16_BYTE( "061-1140.bin", 0x60001, 0x10000, 0xdacbbd94 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "061-1114.bin", 0x10000, 0x4000, 0xc0dcce1a )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1101.bin", 0x000000, 0x10000, 0x02696f15 )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1102.bin", 0x010000, 0x10000, 0x4bed4262 )
	ROM_LOAD( "061-1103.bin", 0x020000, 0x10000, 0xe62b037f )
	ROM_LOAD( "061-1104.bin", 0x030000, 0x10000, 0xfa05aee6 )
	ROM_LOAD( "061-1105.bin", 0x040000, 0x10000, 0xab1c5578 )
	ROM_LOAD( "061-1106.bin", 0x050000, 0x10000, 0x4020468e )
	ROM_LOAD( "061-1107.bin", 0x060000, 0x10000, 0xfe6f6aed )
	ROM_LOAD( "061-1108.bin", 0x070000, 0x10000, 0x26fe71e1 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1143.bin", 0x000000, 0x20000, 0x211c1049 )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1144.bin", 0x020000, 0x20000, 0xef62ed2c )
	ROM_LOAD( "061-1145.bin", 0x040000, 0x20000, 0x067ecb8a )
	ROM_LOAD( "061-1146.bin", 0x060000, 0x20000, 0xfea6bc92 )
	ROM_LOAD( "061-1125.bin", 0x080000, 0x10000, 0xc37f24ac )
	ROM_RELOAD(               0x0c0000, 0x10000 )
	ROM_LOAD( "061-1126.bin", 0x090000, 0x10000, 0x015257f0 )
	ROM_RELOAD(               0x0d0000, 0x10000 )
	ROM_LOAD( "061-1127.bin", 0x0a0000, 0x10000, 0xd05417cb )
	ROM_RELOAD(               0x0e0000, 0x10000 )
	ROM_LOAD( "061-1128.bin", 0x0b0000, 0x10000, 0xfba3e203 )
	ROM_RELOAD(               0x0f0000, 0x10000 )
	ROM_LOAD( "061-1147.bin", 0x100000, 0x20000, 0xca4308cf )
	ROM_LOAD( "061-1148.bin", 0x120000, 0x20000, 0x23ddd45c )
	ROM_LOAD( "061-1149.bin", 0x140000, 0x20000, 0xd77cd1d0 )
	ROM_LOAD( "061-1150.bin", 0x160000, 0x20000, 0xa37157b8 )
	ROM_LOAD( "061-1129.bin", 0x180000, 0x10000, 0x294aaa02 )
	ROM_RELOAD(               0x1c0000, 0x10000 )
	ROM_LOAD( "061-1130.bin", 0x190000, 0x10000, 0xdd610817 )
	ROM_RELOAD(               0x1d0000, 0x10000 )
	ROM_LOAD( "061-1131.bin", 0x1a0000, 0x10000, 0xe8e2f919 )
	ROM_RELOAD(               0x1e0000, 0x10000 )
	ROM_LOAD( "061-1132.bin", 0x1b0000, 0x10000, 0xc79f8ffc )
	ROM_RELOAD(               0x1f0000, 0x10000 )

	ROM_REGION( 0x004000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1142.bin", 0x000000, 0x04000, 0xa6ab551f )  /* alpha font */
ROM_END


ROM_START( toobin2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "061-2133.1j",  0x00000, 0x10000, 0x2c3382e4 )
	ROM_LOAD16_BYTE( "061-2137.1f",  0x00001, 0x10000, 0x891c74b1 )
	ROM_LOAD16_BYTE( "061-2134.2j",  0x20000, 0x10000, 0x2b8164c8 )
	ROM_LOAD16_BYTE( "061-2138.2f",  0x20001, 0x10000, 0xc09cadbd )
	ROM_LOAD16_BYTE( "061-2135.4j",  0x40000, 0x10000, 0x90477c4a )
	ROM_LOAD16_BYTE( "061-2139.4f",  0x40001, 0x10000, 0x47936958 )
	ROM_LOAD16_BYTE( "061-1136.bin", 0x60000, 0x10000, 0x5ae3eeac )
	ROM_LOAD16_BYTE( "061-1140.bin", 0x60001, 0x10000, 0xdacbbd94 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "061-1114.bin", 0x10000, 0x4000, 0xc0dcce1a )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1101.bin", 0x000000, 0x10000, 0x02696f15 )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1102.bin", 0x010000, 0x10000, 0x4bed4262 )
	ROM_LOAD( "061-1103.bin", 0x020000, 0x10000, 0xe62b037f )
	ROM_LOAD( "061-1104.bin", 0x030000, 0x10000, 0xfa05aee6 )
	ROM_LOAD( "061-1105.bin", 0x040000, 0x10000, 0xab1c5578 )
	ROM_LOAD( "061-1106.bin", 0x050000, 0x10000, 0x4020468e )
	ROM_LOAD( "061-1107.bin", 0x060000, 0x10000, 0xfe6f6aed )
	ROM_LOAD( "061-1108.bin", 0x070000, 0x10000, 0x26fe71e1 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1143.bin", 0x000000, 0x20000, 0x211c1049 )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1144.bin", 0x020000, 0x20000, 0xef62ed2c )
	ROM_LOAD( "061-1145.bin", 0x040000, 0x20000, 0x067ecb8a )
	ROM_LOAD( "061-1146.bin", 0x060000, 0x20000, 0xfea6bc92 )
	ROM_LOAD( "061-1125.bin", 0x080000, 0x10000, 0xc37f24ac )
	ROM_RELOAD(               0x0c0000, 0x10000 )
	ROM_LOAD( "061-1126.bin", 0x090000, 0x10000, 0x015257f0 )
	ROM_RELOAD(               0x0d0000, 0x10000 )
	ROM_LOAD( "061-1127.bin", 0x0a0000, 0x10000, 0xd05417cb )
	ROM_RELOAD(               0x0e0000, 0x10000 )
	ROM_LOAD( "061-1128.bin", 0x0b0000, 0x10000, 0xfba3e203 )
	ROM_RELOAD(               0x0f0000, 0x10000 )
	ROM_LOAD( "061-1147.bin", 0x100000, 0x20000, 0xca4308cf )
	ROM_LOAD( "061-1148.bin", 0x120000, 0x20000, 0x23ddd45c )
	ROM_LOAD( "061-1149.bin", 0x140000, 0x20000, 0xd77cd1d0 )
	ROM_LOAD( "061-1150.bin", 0x160000, 0x20000, 0xa37157b8 )
	ROM_LOAD( "061-1129.bin", 0x180000, 0x10000, 0x294aaa02 )
	ROM_RELOAD(               0x1c0000, 0x10000 )
	ROM_LOAD( "061-1130.bin", 0x190000, 0x10000, 0xdd610817 )
	ROM_RELOAD(               0x1d0000, 0x10000 )
	ROM_LOAD( "061-1131.bin", 0x1a0000, 0x10000, 0xe8e2f919 )
	ROM_RELOAD(               0x1e0000, 0x10000 )
	ROM_LOAD( "061-1132.bin", 0x1b0000, 0x10000, 0xc79f8ffc )
	ROM_RELOAD(               0x1f0000, 0x10000 )

	ROM_REGION( 0x004000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1142.bin", 0x000000, 0x04000, 0xa6ab551f )  /* alpha font */
ROM_END


ROM_START( toobinp )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "pg-0-up.1j",   0x00000, 0x10000, 0xcaeb5d1b )
	ROM_LOAD16_BYTE( "pg-0-lo.1f",   0x00001, 0x10000, 0x9713d9d3 )
	ROM_LOAD16_BYTE( "pg-20-up.2j",  0x20000, 0x10000, 0x119f5d7b )
	ROM_LOAD16_BYTE( "pg-20-lo.2f",  0x20001, 0x10000, 0x89664841 )
	ROM_LOAD16_BYTE( "061-2135.4j",  0x40000, 0x10000, 0x90477c4a )
	ROM_LOAD16_BYTE( "pg-40-lo.4f",  0x40001, 0x10000, 0xa9f082a9 )
	ROM_LOAD16_BYTE( "061-1136.bin", 0x60000, 0x10000, 0x5ae3eeac )
	ROM_LOAD16_BYTE( "061-1140.bin", 0x60001, 0x10000, 0xdacbbd94 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "061-1114.bin", 0x10000, 0x4000, 0xc0dcce1a )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1101.bin", 0x000000, 0x10000, 0x02696f15 )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1102.bin", 0x010000, 0x10000, 0x4bed4262 )
	ROM_LOAD( "061-1103.bin", 0x020000, 0x10000, 0xe62b037f )
	ROM_LOAD( "061-1104.bin", 0x030000, 0x10000, 0xfa05aee6 )
	ROM_LOAD( "061-1105.bin", 0x040000, 0x10000, 0xab1c5578 )
	ROM_LOAD( "061-1106.bin", 0x050000, 0x10000, 0x4020468e )
	ROM_LOAD( "061-1107.bin", 0x060000, 0x10000, 0xfe6f6aed )
	ROM_LOAD( "061-1108.bin", 0x070000, 0x10000, 0x26fe71e1 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1143.bin", 0x000000, 0x20000, 0x211c1049 )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1144.bin", 0x020000, 0x20000, 0xef62ed2c )
	ROM_LOAD( "061-1145.bin", 0x040000, 0x20000, 0x067ecb8a )
	ROM_LOAD( "061-1146.bin", 0x060000, 0x20000, 0xfea6bc92 )
	ROM_LOAD( "061-1125.bin", 0x080000, 0x10000, 0xc37f24ac )
	ROM_RELOAD(               0x0c0000, 0x10000 )
	ROM_LOAD( "061-1126.bin", 0x090000, 0x10000, 0x015257f0 )
	ROM_RELOAD(               0x0d0000, 0x10000 )
	ROM_LOAD( "061-1127.bin", 0x0a0000, 0x10000, 0xd05417cb )
	ROM_RELOAD(               0x0e0000, 0x10000 )
	ROM_LOAD( "061-1128.bin", 0x0b0000, 0x10000, 0xfba3e203 )
	ROM_RELOAD(               0x0f0000, 0x10000 )
	ROM_LOAD( "061-1147.bin", 0x100000, 0x20000, 0xca4308cf )
	ROM_LOAD( "061-1148.bin", 0x120000, 0x20000, 0x23ddd45c )
	ROM_LOAD( "061-1149.bin", 0x140000, 0x20000, 0xd77cd1d0 )
	ROM_LOAD( "061-1150.bin", 0x160000, 0x20000, 0xa37157b8 )
	ROM_LOAD( "061-1129.bin", 0x180000, 0x10000, 0x294aaa02 )
	ROM_RELOAD(               0x1c0000, 0x10000 )
	ROM_LOAD( "061-1130.bin", 0x190000, 0x10000, 0xdd610817 )
	ROM_RELOAD(               0x1d0000, 0x10000 )
	ROM_LOAD( "061-1131.bin", 0x1a0000, 0x10000, 0xe8e2f919 )
	ROM_RELOAD(               0x1e0000, 0x10000 )
	ROM_LOAD( "061-1132.bin", 0x1b0000, 0x10000, 0xc79f8ffc )
	ROM_RELOAD(               0x1f0000, 0x10000 )

	ROM_REGION( 0x004000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1142.bin", 0x000000, 0x04000, 0xa6ab551f )  /* alpha font */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_toobin(void)
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 2, 1, 0x1000);
	atarigen_init_6502_speedup(1, 0x414e, 0x4166);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1988, toobin,  0,      toobin, toobin, toobin, ROT270, "Atari Games", "Toobin' (version 3)" )
GAME( 1988, toobin2, toobin, toobin, toobin, toobin, ROT270, "Atari Games", "Toobin' (version 2)" )
GAME( 1988, toobinp, toobin, toobin, toobin, toobin, ROT270, "Atari Games", "Toobin' (prototype)" )
