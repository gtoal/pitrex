/***************************************************************************

	Atari G1 hardware

	driver by Aaron Giles

	Games supported:
		* Hydra (1990)
		* Pit Fighter (1990)

	Known bugs:
		* slapstic not 100%

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

WRITE16_HANDLER( atarig1_mo_control_w );

int atarig1_vh_start(void);
void atarig1_vh_stop(void);
void atarig1_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void atarig1_scanline_update(int param);

extern UINT8 atarig1_pitfight;



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 which_input;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 1;
	if (atarigen_sound_int_state)
		newstate = 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(atarig1_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

static READ16_HANDLER( special_port0_r )
{
	int temp = readinputport(0);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x1000;
	temp ^= 0x2000;		/* A2DOK always high for now */
	return temp;
}


static WRITE16_HANDLER( a2d_select_w )
{
	which_input = offset;
}


static READ16_HANDLER( a2d_data_r )
{
	/* Pit Fighter has no A2D, just another input port */
	if (atarig1_pitfight)
		return readinputport(1);

	/* otherwise, assume it's hydra */
	if (which_input < 3)
		return readinputport(1 + which_input) << 8;

	return 0;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0xfc0000, 0xfc0001, special_port0_r },
	{ 0xfc8000, 0xfc8001, a2d_data_r },
	{ 0xfd0000, 0xfd0001, atarigen_sound_upper_r },
	{ 0xfd8000, 0xfdffff, atarigen_eeprom_r },
/*	{ 0xfe0000, 0xfe7fff, from_r },*/
	{ 0xfe8000, 0xfe89ff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0xf80000, 0xf80001, watchdog_reset16_w },
	{ 0xf88000, 0xf8ffff, atarigen_eeprom_enable_w },
	{ 0xf90000, 0xf90001, atarigen_sound_upper_w },
	{ 0xf98000, 0xf98001, atarigen_sound_reset_w },
	{ 0xfa0000, 0xfa0001, atarig1_mo_control_w },
	{ 0xfb0000, 0xfb0001, atarigen_video_int_ack_w },
	{ 0xfc8000, 0xfc8007, a2d_select_w },
	{ 0xfd8000, 0xfdffff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfe8000, 0xfe89ff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0xff0000, 0xff0fff, atarirle_0_spriteram_w, &atarirle_0_spriteram },
	{ 0xff1000, 0xff3fff, MWA16_RAM },
	{ 0xff4000, 0xff5fff, ataripf_0_simple_w, &ataripf_0_base },
	{ 0xff6000, 0xff6fff, atarian_0_vram_w, &atarian_0_base },
	{ 0xff7000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( hydra )
	PORT_START		/* fc0000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x0fe0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START		/* ADC 0 @ fc8000 */
	PORT_ANALOG( 0x00ff, 0x0080, IPT_AD_STICK_X, 50, 10, 0, 255 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* ADC 1 @ fc8000 */
	PORT_ANALOG( 0x00ff, 0x0080, IPT_AD_STICK_Y, 70, 10, 0, 255 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* ADC 2 @ fc8000 */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL, 100, 16, 0x00, 0xff )

	JSA_II_PORT		/* audio board port */
INPUT_PORTS_END


INPUT_PORTS_START( pitfight )
	PORT_START		/* fc0000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0f80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START      /* fc8000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* not used */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* not used */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_II_PORT		/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(2,5),
	5,
	{ 0, 0, 1, 2, 3 },
	{ RGN_FRAC(2,5)+0, RGN_FRAC(2,5)+4, 0, 4, RGN_FRAC(2,5)+8, RGN_FRAC(2,5)+12, 8, 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout pftoplayout =
{
	8,8,
	RGN_FRAC(1,5),
	5,
	{ RGN_FRAC(4,5), RGN_FRAC(4,5), RGN_FRAC(4,5), RGN_FRAC(4,5), RGN_FRAC(4,5) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout, 0x300, 8 },
	{ REGION_GFX2, 0, &anlayout, 0x100, 16 },
	{ REGION_GFX1, 0, &pftoplayout, 0x300, 8 },
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_atarig1 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz,
			main_readmem,main_writemem,0,0,
			atarigen_video_int_gen,1
		},
		JSA_II_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1280,1280,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	atarig1_vh_start,
	atarig1_vh_stop,
	atarig1_vh_screenrefresh,

	/* sound hardware */
	JSA_II_MONO(REGION_SOUND1),

	atarigen_nvram_handler
};



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_hydra(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x078000, 116);
	atarijsa_init(1, 4, 0, 0x8000);
	atarigen_init_6502_speedup(1, 0x4159, 0x4171);

	atarig1_pitfight = 0;
}


static void init_hydrap(void)
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 4, 0, 0x8000);
	atarigen_init_6502_speedup(1, 0x4159, 0x4171);

	atarig1_pitfight = 0;
}


static void init_pitfight(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 111);
	atarijsa_init(1, 4, 0, 0x8000);
	atarigen_init_6502_speedup(1, 0x4159, 0x4171);

	atarig1_pitfight = 1;
}



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( hydra )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "hydr3028.bin", 0x00000, 0x10000, 0x43475f73 )
	ROM_LOAD16_BYTE( "hydr3029.bin", 0x00001, 0x10000, 0x886e1de8 )
	ROM_LOAD16_BYTE( "hydr3034.bin", 0x20000, 0x10000, 0x5115aa36 )
	ROM_LOAD16_BYTE( "hydr3035.bin", 0x20001, 0x10000, 0xa28ba44b )
	ROM_LOAD16_BYTE( "hydr1032.bin", 0x40000, 0x10000, 0xecd1152a )
	ROM_LOAD16_BYTE( "hydr1033.bin", 0x40001, 0x10000, 0x2ebe1939 )
	ROM_LOAD16_BYTE( "hydr1030.bin", 0x60000, 0x10000, 0xb31fd41f )
	ROM_LOAD16_BYTE( "hydr1031.bin", 0x60001, 0x10000, 0x453d076f )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "hydraa0.bin", 0x10000, 0x4000, 0x619d7319 )
	ROM_CONTINUE(            0x04000, 0xc000 )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hydr1017.bin",  0x000000, 0x10000, 0xbd77b747 ) /* playfield, planes 0-3 odd */
	ROM_LOAD( "hydr1018.bin",  0x010000, 0x10000, 0x7c24e637 )
	ROM_LOAD( "hydr1019.bin",  0x020000, 0x10000, 0xaa2fb07b )
	ROM_LOAD( "hydr1020.bin",  0x030000, 0x10000, 0x906ccd98 )
	ROM_LOAD( "hydr1021.bin",  0x040000, 0x10000, 0xf88cdac2 ) /* playfield, planes 0-3 even */
	ROM_LOAD( "hydr1022.bin",  0x050000, 0x10000, 0xa9c612ff )
	ROM_LOAD( "hydr1023.bin",  0x060000, 0x10000, 0xb706aa6e )
	ROM_LOAD( "hydr1024.bin",  0x070000, 0x10000, 0xc49eac53 )
	ROM_LOAD( "hydr1025.bin",  0x080000, 0x10000, 0x98b5b1a1 ) /* playfield plane 4 */
	ROM_LOAD( "hydr1026.bin",  0x090000, 0x10000, 0xd68d44aa )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hydr1027.bin",  0x000000, 0x20000, 0xf9135b9b ) /* alphanumerics */

	ROM_REGION16_BE( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "hydr1001.bin", 0x00001, 0x10000, 0x3f757a53 )
	ROM_LOAD16_BYTE( "hydr1002.bin", 0x00000, 0x10000, 0xa1169469 )
	ROM_LOAD16_BYTE( "hydr1003.bin", 0x20001, 0x10000, 0xaa21ec33 )
	ROM_LOAD16_BYTE( "hydr1004.bin", 0x20000, 0x10000, 0xc0a2be66 )
	ROM_LOAD16_BYTE( "hydr1005.bin", 0x40001, 0x10000, 0x80c285b3 )
	ROM_LOAD16_BYTE( "hydr1006.bin", 0x40000, 0x10000, 0xad831c59 )
	ROM_LOAD16_BYTE( "hydr1007.bin", 0x60001, 0x10000, 0xe0688cc0 )
	ROM_LOAD16_BYTE( "hydr1008.bin", 0x60000, 0x10000, 0xe6827f6b )
	ROM_LOAD16_BYTE( "hydr1009.bin", 0x80001, 0x10000, 0x33624d07 )
	ROM_LOAD16_BYTE( "hydr1010.bin", 0x80000, 0x10000, 0x9de4c689 )
	ROM_LOAD16_BYTE( "hydr1011.bin", 0xa0001, 0x10000, 0xd55c6e49 )
	ROM_LOAD16_BYTE( "hydr1012.bin", 0xa0000, 0x10000, 0x43af45d0 )
	ROM_LOAD16_BYTE( "hydr1013.bin", 0xc0001, 0x10000, 0x2647a82b )
	ROM_LOAD16_BYTE( "hydr1014.bin", 0xc0000, 0x10000, 0x8897d5e9 )
	ROM_LOAD16_BYTE( "hydr1015.bin", 0xe0001, 0x10000, 0xcf7f69fd )
	ROM_LOAD16_BYTE( "hydr1016.bin", 0xe0000, 0x10000, 0x61aaf14f )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 )	/* 192k for ADPCM samples */
	ROM_LOAD( "hydr1037.bin",  0x00000, 0x10000, 0xb974d3d0 )
	ROM_LOAD( "hydr1038.bin",  0x10000, 0x10000, 0xa2eda15b )
	ROM_LOAD( "hydr1039.bin",  0x20000, 0x10000, 0xeb9eaeb7 )
ROM_END


ROM_START( hydrap )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "hydhi0.bin", 0x00000, 0x10000, 0xdab2e8a2 )
	ROM_LOAD16_BYTE( "hydlo0.bin", 0x00001, 0x10000, 0xc18d4f16 )
	ROM_LOAD16_BYTE( "hydhi1.bin", 0x20000, 0x10000, 0x50c12bb9 )
	ROM_LOAD16_BYTE( "hydlo1.bin", 0x20001, 0x10000, 0x5ee0a846 )
	ROM_LOAD16_BYTE( "hydhi2.bin", 0x40000, 0x10000, 0x436a6d81 )
	ROM_LOAD16_BYTE( "hydlo2.bin", 0x40001, 0x10000, 0x182bfd6a )
	ROM_LOAD16_BYTE( "hydhi3.bin", 0x60000, 0x10000, 0x29e9e03e )
	ROM_LOAD16_BYTE( "hydlo3.bin", 0x60001, 0x10000, 0x7b5047f0 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "hydraa0.bin", 0x10000, 0x4000, BADCRC(0x619d7319) )
	ROM_CONTINUE(            0x04000, 0xc000 )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hydr1017.bin",  0x000000, 0x10000, 0xbd77b747 ) /* playfield, planes 0-3 odd */
	ROM_LOAD( "hydr1018.bin",  0x010000, 0x10000, 0x7c24e637 )
	ROM_LOAD( "hydr1019.bin",  0x020000, 0x10000, 0xaa2fb07b )
	ROM_LOAD( "hydpl03.bin",   0x030000, 0x10000, 0x1f0dfe60 )
	ROM_LOAD( "hydr1021.bin",  0x040000, 0x10000, 0xf88cdac2 ) /* playfield, planes 0-3 even */
	ROM_LOAD( "hydr1022.bin",  0x050000, 0x10000, 0xa9c612ff )
	ROM_LOAD( "hydr1023.bin",  0x060000, 0x10000, 0xb706aa6e )
	ROM_LOAD( "hydphi3.bin",   0x070000, 0x10000, 0x917e250c )
	ROM_LOAD( "hydr1025.bin",  0x080000, 0x10000, 0x98b5b1a1 ) /* playfield plane 4 */
	ROM_LOAD( "hydpl41.bin",   0x090000, 0x10000, 0x85f9afa6 )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hydalph.bin",   0x000000, 0x20000, 0x7dd2b062 ) /* alphanumerics */

	ROM_REGION16_BE( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "hydmhi0.bin", 0x00001, 0x10000, 0x3c83b42d )
	ROM_LOAD16_BYTE( "hydmlo0.bin", 0x00000, 0x10000, 0x6d49650c )
	ROM_LOAD16_BYTE( "hydmhi1.bin", 0x20001, 0x10000, 0x689b3376 )
	ROM_LOAD16_BYTE( "hydmlo1.bin", 0x20000, 0x10000, 0xc81a4e88 )
	ROM_LOAD16_BYTE( "hydmhi2.bin", 0x40001, 0x10000, 0x77098e14 )
	ROM_LOAD16_BYTE( "hydmlo2.bin", 0x40000, 0x10000, 0x40015d9d )
	ROM_LOAD16_BYTE( "hydmhi3.bin", 0x60001, 0x10000, 0xdfebdcbd )
	ROM_LOAD16_BYTE( "hydmlo3.bin", 0x60000, 0x10000, 0x213c407c )
	ROM_LOAD16_BYTE( "hydmhi4.bin", 0x80001, 0x10000, 0x2897765f )
	ROM_LOAD16_BYTE( "hydmlo4.bin", 0x80000, 0x10000, 0x730157f3 )
	ROM_LOAD16_BYTE( "hydmhi5.bin", 0xa0001, 0x10000, 0xecd061ae )
	ROM_LOAD16_BYTE( "hydmlo5.bin", 0xa0000, 0x10000, 0xa5a08c53 )
	ROM_LOAD16_BYTE( "hydmhi6.bin", 0xc0001, 0x10000, 0xaa3f2903 )
	ROM_LOAD16_BYTE( "hydmlo6.bin", 0xc0000, 0x10000, 0xdb8ea56f )
	ROM_LOAD16_BYTE( "hydmhi7.bin", 0xe0001, 0x10000, 0x71fc3e43 )
	ROM_LOAD16_BYTE( "hydmlo7.bin", 0xe0000, 0x10000, 0x7960b0c2 )

	ROM_REGION( 0x30000, REGION_SOUND1, 0 )	/* 192k for ADPCM samples */
	ROM_LOAD( "hydr1037.bin",  0x00000, 0x10000, BADCRC(0xb974d3d0) )
	ROM_LOAD( "hydr1038.bin",  0x10000, 0x10000, BADCRC(0xa2eda15b) )
	ROM_LOAD( "hydr1039.bin",  0x20000, 0x10000, BADCRC(0xeb9eaeb7) )
ROM_END


ROM_START( pitfight )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "4028", 0x00000, 0x10000, 0xf7cb1a4b )
	ROM_LOAD16_BYTE( "4029", 0x00001, 0x10000, 0x13ae0d4f )
	ROM_LOAD16_BYTE( "3030", 0x20000, 0x10000, 0xb053e779 )
	ROM_LOAD16_BYTE( "3031", 0x20001, 0x10000, 0x2b8c4d13 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1060", 0x10000, 0x4000, 0x231d71d7 )
	ROM_CONTINUE(     0x04000, 0xc000 )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1017",  0x000000, 0x10000, 0xad3cfea5 ) /* playfield, planes 0-3 odd */
	ROM_LOAD( "1018",  0x010000, 0x10000, 0x1a0f8bcf )
	ROM_LOAD( "1021",  0x040000, 0x10000, 0x777efee3 ) /* playfield, planes 0-3 even */
	ROM_LOAD( "1022",  0x050000, 0x10000, 0x524319d0 )
	ROM_LOAD( "1025",  0x080000, 0x10000, 0xfc41691a ) /* playfield plane 4 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1027",  0x000000, 0x10000, 0xa59f381d ) /* alphanumerics */

	ROM_REGION16_BE( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "1001", 0x000001, 0x20000, 0x3af31444 )
	ROM_LOAD16_BYTE( "1002", 0x000000, 0x20000, 0xf1d76a4c )
	ROM_LOAD16_BYTE( "1003", 0x040001, 0x20000, 0x28c41c2a )
	ROM_LOAD16_BYTE( "1004", 0x040000, 0x20000, 0x977744da )
	ROM_LOAD16_BYTE( "1005", 0x080001, 0x20000, 0xae59aef2 )
	ROM_LOAD16_BYTE( "1006", 0x080000, 0x20000, 0xb6ccd77e )
	ROM_LOAD16_BYTE( "1007", 0x0c0001, 0x20000, 0xba33b0c0 )
	ROM_LOAD16_BYTE( "1008", 0x0c0000, 0x20000, 0x09bd047c )
	ROM_LOAD16_BYTE( "1009", 0x100001, 0x20000, 0xab85b00b )
	ROM_LOAD16_BYTE( "1010", 0x100000, 0x20000, 0xeca94bdc )
	ROM_LOAD16_BYTE( "1011", 0x140001, 0x20000, 0xa86582fd )
	ROM_LOAD16_BYTE( "1012", 0x140000, 0x20000, 0xefd1152d )
	ROM_LOAD16_BYTE( "1013", 0x180001, 0x20000, 0xa141379e )
	ROM_LOAD16_BYTE( "1014", 0x180000, 0x20000, 0x93bfcc15 )
	ROM_LOAD16_BYTE( "1015", 0x1c0001, 0x20000, 0x9378ad0b )
	ROM_LOAD16_BYTE( "1016", 0x1c0000, 0x20000, 0x19c3fbe0 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "1061",  0x00000, 0x10000, 0x5b0468c6 )
	ROM_LOAD( "1062",  0x10000, 0x10000, 0xf73fe3cb )
	ROM_LOAD( "1063",  0x20000, 0x10000, 0xaa93421d )
	ROM_LOAD( "1064",  0x30000, 0x10000, 0x33f045d5 )
ROM_END


ROM_START( pitfigh3 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "3028", 0x00000, 0x10000, 0x99530da4 )
	ROM_LOAD16_BYTE( "3029", 0x00001, 0x10000, 0x78c7afbf )
	ROM_LOAD16_BYTE( "3030", 0x20000, 0x10000, 0xb053e779 )
	ROM_LOAD16_BYTE( "3031", 0x20001, 0x10000, 0x2b8c4d13 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1060", 0x10000, 0x4000, 0x231d71d7 )
	ROM_CONTINUE(     0x04000, 0xc000 )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1017",  0x000000, 0x10000, 0xad3cfea5 ) /* playfield, planes 0-3 odd */
	ROM_LOAD( "1018",  0x010000, 0x10000, 0x1a0f8bcf )
	ROM_LOAD( "1021",  0x040000, 0x10000, 0x777efee3 ) /* playfield, planes 0-3 even */
	ROM_LOAD( "1022",  0x050000, 0x10000, 0x524319d0 )
	ROM_LOAD( "1025",  0x080000, 0x10000, 0xfc41691a ) /* playfield plane 4 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1027",  0x000000, 0x10000, 0xa59f381d ) /* alphanumerics */

	ROM_REGION16_BE( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "1001", 0x000001, 0x20000, 0x3af31444 )
	ROM_LOAD16_BYTE( "1002", 0x000000, 0x20000, 0xf1d76a4c )
	ROM_LOAD16_BYTE( "1003", 0x040001, 0x20000, 0x28c41c2a )
	ROM_LOAD16_BYTE( "1004", 0x040000, 0x20000, 0x977744da )
	ROM_LOAD16_BYTE( "1005", 0x080001, 0x20000, 0xae59aef2 )
	ROM_LOAD16_BYTE( "1006", 0x080000, 0x20000, 0xb6ccd77e )
	ROM_LOAD16_BYTE( "1007", 0x0c0001, 0x20000, 0xba33b0c0 )
	ROM_LOAD16_BYTE( "1008", 0x0c0000, 0x20000, 0x09bd047c )
	ROM_LOAD16_BYTE( "1009", 0x100001, 0x20000, 0xab85b00b )
	ROM_LOAD16_BYTE( "1010", 0x100000, 0x20000, 0xeca94bdc )
	ROM_LOAD16_BYTE( "1011", 0x140001, 0x20000, 0xa86582fd )
	ROM_LOAD16_BYTE( "1012", 0x140000, 0x20000, 0xefd1152d )
	ROM_LOAD16_BYTE( "1013", 0x180001, 0x20000, 0xa141379e )
	ROM_LOAD16_BYTE( "1014", 0x180000, 0x20000, 0x93bfcc15 )
	ROM_LOAD16_BYTE( "1015", 0x1c0001, 0x20000, 0x9378ad0b )
	ROM_LOAD16_BYTE( "1016", 0x1c0000, 0x20000, 0x19c3fbe0 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "1061",  0x00000, 0x10000, 0x5b0468c6 )
	ROM_LOAD( "1062",  0x10000, 0x10000, 0xf73fe3cb )
	ROM_LOAD( "1063",  0x20000, 0x10000, 0xaa93421d )
	ROM_LOAD( "1064",  0x30000, 0x10000, 0x33f045d5 )
ROM_END



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME ( 1990, hydra,    0,        atarig1, hydra,    hydra,    ROT0, "Atari Games", "Hydra" )
GAME ( 1990, hydrap,   hydra,    atarig1, hydra,    hydrap,   ROT0, "Atari Games", "Hydra (prototype)" )
GAMEX( 1990, pitfight, 0,        atarig1, pitfight, pitfight, ROT0, "Atari Games", "Pit Fighter (version 4)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1990, pitfigh3, pitfight, atarig1, pitfight, pitfight, ROT0, "Atari Games", "Pit Fighter (version 3)", GAME_UNEMULATED_PROTECTION )
