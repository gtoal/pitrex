/***************************************************************************

	Atari Blasteroids hardware

	driver by Aaron Giles

	Games supported:
		* Blasteroids (1987) [3 sets]

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

int blstroid_vh_start(void);
void blstroid_vh_stop(void);
void blstroid_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void blstroid_scanline_update(int scanline);

extern data16_t *blstroid_priorityram;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate = 1;
	if (atarigen_video_int_state)
		newstate = 2;
	if (atarigen_sound_int_state)
		newstate = 4;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(blstroid_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O read dispatch
 *
 *************************************/

static READ16_HANDLER( special_port2_r )
{
	int temp = readinputport(2);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0040;
	if (atarigen_get_hblank()) temp ^= 0x0010;
	return temp;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xff9400, 0xff9401, atarigen_sound_r },
	{ 0xff9800, 0xff9801, input_port_0_word_r },
	{ 0xff9804, 0xff9805, input_port_1_word_r },
	{ 0xff9c00, 0xff9c01, special_port2_r },
	{ 0xff9c02, 0xff9c03, input_port_3_word_r },
	{ 0xffa000, 0xffa3ff, MRA16_RAM },
	{ 0xffb000, 0xffb3ff, atarigen_eeprom_r },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xff8000, 0xff8001, watchdog_reset16_w },
	{ 0xff8200, 0xff8201, atarigen_scanline_int_ack_w },
	{ 0xff8400, 0xff8401, atarigen_video_int_ack_w },
	{ 0xff8600, 0xff8601, atarigen_eeprom_enable_w },
	{ 0xff8800, 0xff89ff, MWA16_RAM, &blstroid_priorityram },
	{ 0xff8a00, 0xff8a01, atarigen_sound_w },
	{ 0xff8c00, 0xff8c01, atarigen_sound_reset_w },
	{ 0xff8e00, 0xff8e01, atarigen_halt_until_hblank_0_w },
	{ 0xffa000, 0xffa3ff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
	{ 0xffb000, 0xffb3ff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xffc000, 0xffcfff, ataripf_0_simple_w, &ataripf_0_base },
	{ 0xffd000, 0xffdfff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xffe000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( blstroid )
	PORT_START      /* ff9800 */
	PORT_ANALOG( 0x00ff, 0, IPT_DIAL | IPF_PLAYER1, 60, 10, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* ff9804 */
	PORT_ANALOG( 0x00ff, 0, IPT_DIAL | IPF_PLAYER2, 60, 10, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* ff9c00 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* ff9c02 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_I_PORT	/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	16,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0,0, 4,4, 8,8, 12,12, 16,16, 20,20, 24,24, 28,28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};


static struct GfxLayout molayout =
{
	16,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4, RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+12, 8, 12,
			RGN_FRAC(1,2)+16, RGN_FRAC(1,2)+20, 16, 20, RGN_FRAC(1,2)+24, RGN_FRAC(1,2)+28, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout,  256, 16 },
	{ REGION_GFX2, 0, &molayout,    0, 16 },
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_blstroid =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz/2,
			main_readmem,main_writemem,0,0,
			atarigen_video_int_gen,1
		},
		JSA_I_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	init_machine,

	/* video hardware */
	40*16, 30*8, { 0*8, 40*16-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	512,512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK |
			VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	blstroid_vh_start,
	blstroid_vh_stop,
	blstroid_vh_screenrefresh,

	/* sound hardware */
	JSA_I_STEREO,

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( blstroid )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "057-4123",  0x00000, 0x10000, 0xd14badc4 )
	ROM_LOAD16_BYTE( "057-4121",  0x00001, 0x10000, 0xae3e93e8 )
	ROM_LOAD16_BYTE( "057-4124",  0x20000, 0x10000, 0xfd2365df )
	ROM_LOAD16_BYTE( "057-4122",  0x20001, 0x10000, 0xc364706e )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "blstroid.snd", 0x10000, 0x4000, 0xbaa8b5fe )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x040000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "blstroid.1l",  0x000000, 0x10000, 0x3c2daa5b ) /* playfield */
	ROM_LOAD( "blstroid.1m",  0x010000, 0x10000, 0xf84f0b97 ) /* playfield */
	ROM_LOAD( "blstroid.3l",  0x020000, 0x10000, 0xae5274f0 ) /* playfield */
	ROM_LOAD( "blstroid.3m",  0x030000, 0x10000, 0x4bb72060 ) /* playfield */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "blstroid.5m",  0x000000, 0x10000, 0x50e0823f ) /* mo */
	ROM_LOAD( "blstroid.6m",  0x010000, 0x10000, 0x729de7a9 ) /* mo */
	ROM_LOAD( "blstroid.8m",  0x020000, 0x10000, 0x090e42ab ) /* mo */
	ROM_LOAD( "blstroid.10m", 0x030000, 0x10000, 0x1ff79e67 ) /* mo */
	ROM_LOAD( "blstroid.11m", 0x040000, 0x10000, 0x4be1d504 ) /* mo */
	ROM_LOAD( "blstroid.13m", 0x050000, 0x10000, 0xe4409310 ) /* mo */
	ROM_LOAD( "blstroid.14m", 0x060000, 0x10000, 0x7aaca15e ) /* mo */
	ROM_LOAD( "blstroid.16m", 0x070000, 0x10000, 0x33690379 ) /* mo */
	ROM_LOAD( "blstroid.5n",  0x080000, 0x10000, 0x2720ee71 ) /* mo */
	ROM_LOAD( "blstroid.6n",  0x090000, 0x10000, 0x2faecd15 ) /* mo */
	ROM_LOAD( "blstroid.8n",  0x0a0000, 0x10000, 0xf10e59ed ) /* mo */
	ROM_LOAD( "blstroid.10n", 0x0b0000, 0x10000, 0x4d5fc284 ) /* mo */
	ROM_LOAD( "blstroid.11n", 0x0c0000, 0x10000, 0xa70fc6e6 ) /* mo */
	ROM_LOAD( "blstroid.13n", 0x0d0000, 0x10000, 0xf423b4f8 ) /* mo */
	ROM_LOAD( "blstroid.14n", 0x0e0000, 0x10000, 0x56fa3d16 ) /* mo */
	ROM_LOAD( "blstroid.16n", 0x0f0000, 0x10000, 0xf257f738 ) /* mo */
ROM_END


ROM_START( blstroi2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "blstroid.6c",  0x00000, 0x10000, 0x5a092513 )
	ROM_LOAD16_BYTE( "blstroid.6b",  0x00001, 0x10000, 0x486aac51 )
	ROM_LOAD16_BYTE( "blstroid.4c",  0x20000, 0x10000, 0xd0fa38fe )
	ROM_LOAD16_BYTE( "blstroid.4b",  0x20001, 0x10000, 0x744bf921 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "blstroid.snd", 0x10000, 0x4000, 0xbaa8b5fe )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x040000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "blstroid.1l",  0x000000, 0x10000, 0x3c2daa5b ) /* playfield */
	ROM_LOAD( "blstroid.1m",  0x010000, 0x10000, 0xf84f0b97 ) /* playfield */
	ROM_LOAD( "blstroid.3l",  0x020000, 0x10000, 0xae5274f0 ) /* playfield */
	ROM_LOAD( "blstroid.3m",  0x030000, 0x10000, 0x4bb72060 ) /* playfield */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "blstroid.5m",  0x000000, 0x10000, 0x50e0823f ) /* mo */
	ROM_LOAD( "blstroid.6m",  0x010000, 0x10000, 0x729de7a9 ) /* mo */
	ROM_LOAD( "blstroid.8m",  0x020000, 0x10000, 0x090e42ab ) /* mo */
	ROM_LOAD( "blstroid.10m", 0x030000, 0x10000, 0x1ff79e67 ) /* mo */
	ROM_LOAD( "blstroid.11m", 0x040000, 0x10000, 0x4be1d504 ) /* mo */
	ROM_LOAD( "blstroid.13m", 0x050000, 0x10000, 0xe4409310 ) /* mo */
	ROM_LOAD( "blstroid.14m", 0x060000, 0x10000, 0x7aaca15e ) /* mo */
	ROM_LOAD( "blstroid.16m", 0x070000, 0x10000, 0x33690379 ) /* mo */
	ROM_LOAD( "blstroid.5n",  0x080000, 0x10000, 0x2720ee71 ) /* mo */
	ROM_LOAD( "blstroid.6n",  0x090000, 0x10000, 0x2faecd15 ) /* mo */
	ROM_LOAD( "blstroid.8n",  0x0a0000, 0x10000, 0xf10e59ed ) /* mo */
	ROM_LOAD( "blstroid.10n", 0x0b0000, 0x10000, 0x4d5fc284 ) /* mo */
	ROM_LOAD( "blstroid.11n", 0x0c0000, 0x10000, 0xa70fc6e6 ) /* mo */
	ROM_LOAD( "blstroid.13n", 0x0d0000, 0x10000, 0xf423b4f8 ) /* mo */
	ROM_LOAD( "blstroid.14n", 0x0e0000, 0x10000, 0x56fa3d16 ) /* mo */
	ROM_LOAD( "blstroid.16n", 0x0f0000, 0x10000, 0xf257f738 ) /* mo */
ROM_END


ROM_START( blsthead )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "eheadh0.c6",  0x00000, 0x10000, 0x061f0898 )
	ROM_LOAD16_BYTE( "eheadl0.b6",  0x00001, 0x10000, 0xae8df7cb )
	ROM_LOAD16_BYTE( "eheadh1.c5",  0x20000, 0x10000, 0x0b7a3cb6 )
	ROM_LOAD16_BYTE( "eheadl1.b5",  0x20001, 0x10000, 0x43971694 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "blstroid.snd", 0x10000, 0x4000, 0xbaa8b5fe )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x040000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "blstroid.1l",  0x000000, 0x10000, 0x3c2daa5b ) /* playfield */
	ROM_LOAD( "blstroid.1m",  0x010000, 0x10000, 0xf84f0b97 ) /* playfield */
	ROM_LOAD( "blstroid.3l",  0x020000, 0x10000, 0xae5274f0 ) /* playfield */
	ROM_LOAD( "blstroid.3m",  0x030000, 0x10000, 0x4bb72060 ) /* playfield */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "blstroid.5m",  0x000000, 0x10000, 0x50e0823f ) /* mo */
	ROM_LOAD( "blstroid.6m",  0x010000, 0x10000, 0x729de7a9 ) /* mo */
	ROM_LOAD( "blstroid.8m",  0x020000, 0x10000, 0x090e42ab ) /* mo */
	ROM_LOAD( "blstroid.10m", 0x030000, 0x10000, 0x1ff79e67 ) /* mo */
	ROM_LOAD( "mol4.m12",     0x040000, 0x10000, 0x571139ea ) /* mo */
	ROM_LOAD( "blstroid.13m", 0x050000, 0x10000, 0xe4409310 ) /* mo */
	ROM_LOAD( "blstroid.14m", 0x060000, 0x10000, 0x7aaca15e ) /* mo */
	ROM_LOAD( "mol7.m16",     0x070000, 0x10000, 0xd27b2d91 ) /* mo */
	ROM_LOAD( "blstroid.5n",  0x080000, 0x10000, 0x2720ee71 ) /* mo */
	ROM_LOAD( "blstroid.6n",  0x090000, 0x10000, 0x2faecd15 ) /* mo */
	ROM_LOAD( "moh2.n8",      0x0a0000, 0x10000, 0xa15e79e1 ) /* mo */
	ROM_LOAD( "blstroid.10n", 0x0b0000, 0x10000, 0x4d5fc284 ) /* mo */
	ROM_LOAD( "moh4.n12",     0x0c0000, 0x10000, 0x1a74e960 ) /* mo */
	ROM_LOAD( "blstroid.13n", 0x0d0000, 0x10000, 0xf423b4f8 ) /* mo */
	ROM_LOAD( "blstroid.14n", 0x0e0000, 0x10000, 0x56fa3d16 ) /* mo */
	ROM_LOAD( "moh7.n16",     0x0f0000, 0x10000, 0xa93cbbe7 ) /* mo */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_blstroid(void)
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 4, 2, 0x80);
	atarigen_init_6502_speedup(1, 0x4157, 0x416f);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1987, blstroid, 0,        blstroid, blstroid, blstroid, ROT0, "Atari Games", "Blasteroids (version 4)" )
GAME( 1987, blstroi2, blstroid, blstroid, blstroid, blstroid, ROT0, "Atari Games", "Blasteroids (version 2)" )
GAME( 1987, blsthead, blstroid, blstroid, blstroid, blstroid, ROT0, "Atari Games", "Blasteroids (with heads)" )
