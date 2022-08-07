/***************************************************************************

	Atari Batman hardware

	driver by Aaron Giles

	Games supported:
		* Batman (1991)

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

int batman_vh_start(void);
void batman_vh_stop(void);
void batman_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void batman_scanline_update(int scanline);



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t latch_data;



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate |= 4;
	if (atarigen_sound_int_state)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarivc_reset(atarivc_eof_data);
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(batman_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O handling
 *
 *************************************/

static READ16_HANDLER( special_port2_r )
{
	int result = readinputport(2);
	if (atarigen_sound_to_cpu_ready) result ^= 0x0010;
	if (atarigen_cpu_to_sound_ready) result ^= 0x0020;
	return result;
}


static WRITE16_HANDLER( latch_w )
{
	int oldword = latch_data;
	COMBINE_DATA(&latch_data);

	/* bit 4 is connected to the /RESET pin on the 6502 */
	if (latch_data & 0x0010)
		cpu_set_reset_line(1, CLEAR_LINE);
	else
		cpu_set_reset_line(1, ASSERT_LINE);

	/* alpha bank is selected by the upper 4 bits */
	if ((oldword ^ latch_data) & 0x7000)
		atarian_set_bankbits(0, ((latch_data >> 12) & 7) << 16);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x120000, 0x120fff, atarigen_eeprom_r },
	{ 0x3e0000, 0x3e0fff, MRA16_RAM },
	{ 0x3effc0, 0x3effff, atarivc_r },
	{ 0x260000, 0x260001, input_port_0_word_r },
	{ 0x260002, 0x260003, input_port_1_word_r },
	{ 0x260010, 0x260011, special_port2_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
	{ 0x3f0000, 0x3fffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x120000, 0x120fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x260040, 0x260041, atarigen_sound_w },
	{ 0x260050, 0x260051, latch_w },
	{ 0x260060, 0x260061, atarigen_eeprom_enable_w },
	{ 0x2a0000, 0x2a0001, watchdog_reset16_w },
	{ 0x3e0000, 0x3e0fff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0x3effc0, 0x3effff, atarivc_w, &atarivc_data },
	{ 0x3f0000, 0x3f1fff, ataripf_1_latched_w, &ataripf_1_base },
	{ 0x3f2000, 0x3f3fff, ataripf_0_latched_w, &ataripf_0_base },
	{ 0x3f4000, 0x3f5fff, ataripf_01_upper_lsb_msb_w, &ataripf_0_upper },
	{ 0x3f6000, 0x3f7fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3f8000, 0x3f8fef, atarian_0_vram_w, &atarian_0_base },
	{ 0x3f8f00, 0x3f8f7f, MWA16_RAM, &atarivc_eof_data },
	{ 0x3f8f80, 0x3f8fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x3f9000, 0x3fffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( batman )
	PORT_START		/* 26000 */
	PORT_BIT( 0x01ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START		/* 26002 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 26010 */
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )	/* Input buffer full (@260030) */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )	/* Output buffer full (@260040) */
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_VBLANK )

	JSA_III_PORT	/* audio board port */
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


static struct GfxLayout pfmolayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX3, 0, &pfmolayout,  512, 64 },		/* sprites & playfield */
	{ REGION_GFX2, 0, &pfmolayout,  256, 64 },		/* sprites & playfield */
	{ REGION_GFX1, 0, &anlayout,      0, 64 },		/* characters 8x8 */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_batman =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			ATARI_CLOCK_14MHz,
			main_readmem,main_writemem,0,0,
			ignore_interrupt,1
		},
		JSA_III_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	batman_vh_start,
	batman_vh_stop,
	batman_vh_screenrefresh,

	/* sound hardware */
	JSA_III_MONO(REGION_SOUND1),

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( batman )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )	/* 6*128k for 68000 code */
	ROM_LOAD16_BYTE( "085-2030.10r",  0x00000, 0x20000, 0x7cf4e5bf )
	ROM_LOAD16_BYTE( "085-2031.7r",   0x00001, 0x20000, 0x7d7f3fc4 )
	ROM_LOAD16_BYTE( "085-2032.91r",  0x40000, 0x20000, 0xd80afb20 )
	ROM_LOAD16_BYTE( "085-2033.6r",   0x40001, 0x20000, 0x97efa2b8 )
	ROM_LOAD16_BYTE( "085-2034.9r",   0x80000, 0x20000, 0x05388c62 )
	ROM_LOAD16_BYTE( "085-2035.5r",   0x80001, 0x20000, 0xe77c92dd )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "085-1040.12c",  0x10000, 0x4000, 0x080db83c )
	ROM_CONTINUE(              0x04000, 0xc000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "085-2009.10m",  0x00000, 0x20000, 0xa82d4923 )	/* alphanumerics */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "085-1010.13r",  0x000000, 0x20000, 0x466e1365 )	/* graphics, plane 0 */
	ROM_LOAD( "085-1014.14r",  0x020000, 0x20000, 0xef53475a )
	ROM_LOAD( "085-1011.13m",  0x040000, 0x20000, 0x8cda5efc )	/* graphics, plane 1 */
	ROM_LOAD( "085-1015.14m",  0x060000, 0x20000, 0x043e7f8b )
	ROM_LOAD( "085-1012.13f",  0x080000, 0x20000, 0xb017f2c3 )	/* graphics, plane 2 */
	ROM_LOAD( "085-1016.14f",  0x0a0000, 0x20000, 0x70aa2360 )
	ROM_LOAD( "085-1013.13c",  0x0c0000, 0x20000, 0x68b64975 )	/* graphics, plane 3 */
	ROM_LOAD( "085-1017.14c",  0x0e0000, 0x20000, 0xe4af157b )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "085-1018.15r",  0x000000, 0x20000, 0x4c14f1e5 )
	ROM_LOAD( "085-1022.16r",  0x020000, 0x20000, 0x7476a15d )
	ROM_LOAD( "085-1019.15m",  0x040000, 0x20000, 0x2046d9ec )
	ROM_LOAD( "085-1023.16m",  0x060000, 0x20000, 0x75cac686 )
	ROM_LOAD( "085-1020.15f",  0x080000, 0x20000, 0xcc4f4b94 )
	ROM_LOAD( "085-1024.16f",  0x0a0000, 0x20000, 0xd60d35e0 )
	ROM_LOAD( "085-1021.15c",  0x0c0000, 0x20000, 0x9c8ef9ba )
	ROM_LOAD( "085-1025.16c",  0x0e0000, 0x20000, 0x5d30bcd1 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM */
	ROM_LOAD( "085-1041.19e",  0x80000, 0x20000, 0xd97d5dbb )
	ROM_LOAD( "085-1042.17e",  0xa0000, 0x20000, 0x8c496986 )
	ROM_LOAD( "085-1043.15e",  0xc0000, 0x20000, 0x51812d3b )
	ROM_LOAD( "085-1044.12e",  0xe0000, 0x20000, 0x5e2d7f31 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_batman(void)
{
	static const data16_t default_eeprom[] =
	{
		0x0001,0x01F1,0x0154,0x01C5,0x0100,0x0113,0x0300,0x0173,
		0x0700,0x0154,0x0200,0x0107,0x0100,0x0120,0x0300,0x0165,
		0x0125,0x0100,0x0149,0x019D,0x016C,0x018B,0x01F1,0x0154,
		0x01C5,0x0100,0x0113,0x0300,0x0173,0x0700,0x0154,0x0200,
		0x0107,0x0100,0x0120,0x0300,0x0165,0x0125,0x0100,0x0149,
		0x019D,0x016C,0x018B,0x6800,0x0134,0x0113,0x0148,0x0100,
		0x019A,0x0105,0x01DC,0x01A2,0x013A,0x0139,0x0100,0x0105,
		0x01AB,0x016A,0x0149,0x0100,0x01ED,0x0105,0x0185,0x01B2,
		0x0134,0x0100,0x0105,0x0160,0x01AA,0x0149,0x0100,0x0105,
		0x012A,0x0152,0x0110,0x0100,0x0168,0x0105,0x0113,0x012E,
		0x0150,0x0218,0x01D0,0x0100,0x01D0,0x0300,0x01D0,0x0600,
		0x01D0,0x02C8,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarigen_init_6502_speedup(1, 0x4163, 0x417b);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1991, batman, 0, batman, batman, batman, ROT0, "Atari Games", "Batman" )
