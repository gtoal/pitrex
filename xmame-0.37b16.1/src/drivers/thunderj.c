/***************************************************************************

	Atari ThunderJaws hardware

	driver by Aaron Giles

	Games supported:
		* ThunderJaws (1990)

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

int thunderj_vh_start(void);
void thunderj_vh_stop(void);
void thunderj_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void thunderj_scanline_update(int scanline);



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t *rts_address;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;
	int newstate2 = 0;

	if (atarigen_scanline_int_state)
		newstate |= 4, newstate2 |= 4;
	if (atarigen_sound_int_state)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);

	if (newstate2)
		cpu_set_irq_line(1, newstate2, ASSERT_LINE);
	else
		cpu_set_irq_line(1, 7, CLEAR_LINE);
}


static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarivc_reset(atarivc_eof_data);
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(thunderj_scanline_update, 1024);
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

	if (atarigen_sound_to_cpu_ready) result ^= 0x0004;
	if (atarigen_cpu_to_sound_ready) result ^= 0x0008;
	result ^= 0x0010;

	return result;
}


static WRITE16_HANDLER( latch_w )
{
	/* reset extra CPU */
	if (ACCESSING_LSB)
	{
		/* 0 means hold CPU 2's reset low */
		if (data & 1)
			cpu_set_reset_line(1, CLEAR_LINE);
		else
			cpu_set_reset_line(1, ASSERT_LINE);

		/* bits 2-5 are the alpha bank */
		atarian_set_bankbits(0, ((data >> 2) & 7) << 16);
	}
}



/*************************************
 *
 *	Video Controller Hack
 *
 *************************************/

READ16_HANDLER( thunderj_video_control_r )
{
	/* Sigh. CPU #1 reads the video controller register twice per frame, once at
	   the beginning of interrupt and once near the end. It stores these values in a
	   table starting at $163484. CPU #2 periodically looks at this table to make
	   sure that it is getting interrupts at the appropriate times, and that the
	   VBLANK bit is set appropriately. Unfortunately, due to all the cpu_yield()
	   calls we make to synchronize the two CPUs, we occasionally get out of time
	   and generate the interrupt outside of the tight tolerances CPU #2 expects.

	   So we fake it. Returning scanlines $f5 and $f7 alternately provides the
	   correct answer that causes CPU #2 to be happy and not aggressively trash
	   memory (which is what it does if this interrupt test fails -- see the code
	   at $1E56 to see!) */

	/* Use these lines to detect when things go south:

	if (cpu_readmem24bew_word(0x163482) > 0xfff)
		printf("You're screwed!");*/

	return atarivc_r(offset,0);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x09ffff, MRA16_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_r },
	{ 0x160000, 0x16ffff, MRA16_BANK1 },
	{ 0x260000, 0x26000f, input_port_0_word_r },
	{ 0x260010, 0x260011, input_port_1_word_r },
	{ 0x260012, 0x260013, special_port2_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
	{ 0x3e0000, 0x3e0fff, MRA16_RAM },
	{ 0x3effc0, 0x3effff, thunderj_video_control_r },
	{ 0x3f0000, 0x3fffff, MRA16_RAM },
	{ 0x800000, 0x800001, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x09ffff, MWA16_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x160000, 0x16ffff, MWA16_BANK1 },	/* shared */
	{ 0x1f0000, 0x1fffff, atarigen_eeprom_enable_w },
	{ 0x2e0000, 0x2e0001, watchdog_reset16_w },
	{ 0x360010, 0x360011, latch_w },
	{ 0x360020, 0x360021, atarigen_sound_reset_w },
	{ 0x360030, 0x360031, atarigen_sound_w },
	{ 0x3e0000, 0x3e0fff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0x3effc0, 0x3effff, atarivc_w, &atarivc_data },
	{ 0x3f0000, 0x3f1fff, ataripf_1_latched_w, &ataripf_1_base },
	{ 0x3f2000, 0x3f3fff, ataripf_0_latched_w, &ataripf_0_base },
	{ 0x3f4000, 0x3f5fff, ataripf_01_upper_lsb_msb_w, &ataripf_0_upper },
	{ 0x3f6000, 0x3f7fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3f8000, 0x3f8eff, atarian_0_vram_w, &atarian_0_base },
	{ 0x3f8f00, 0x3f8f7f, MWA16_RAM, &atarivc_eof_data },
	{ 0x3f8f80, 0x3f8fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x3f9000, 0x3fffff, MWA16_RAM },
	{ 0x800000, 0x800001, MWA16_RAM, &rts_address },
MEMORY_END



/*************************************
 *
 *	Extra CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( extra_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x060000, 0x07ffff, MRA16_ROM },
	{ 0x160000, 0x16ffff, MRA16_BANK1 },
	{ 0x260000, 0x26000f, input_port_0_word_r },
	{ 0x260010, 0x260011, input_port_1_word_r },
	{ 0x260012, 0x260013, special_port2_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
MEMORY_END


static MEMORY_WRITE16_START( extra_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x060000, 0x07ffff, MWA16_ROM },
	{ 0x160000, 0x16ffff, MWA16_BANK1 },
	{ 0x360000, 0x360001, atarigen_video_int_ack_w },
	{ 0x360010, 0x360011, latch_w },
	{ 0x360020, 0x360021, atarigen_sound_reset_w },
	{ 0x360030, 0x360031, atarigen_sound_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( thunderj )
	PORT_START		/* 260000 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 260010 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0c00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START		/* 260012 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* Input buffer full (@260030) */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )	/* Output buffer full (@360030) */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0c00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	JSA_II_PORT		/* audio board port */
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
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pfmolayout,  512,  96 },	/* sprites & playfield */
	{ REGION_GFX2, 0, &pfmolayout,  256, 112 },	/* sprites & playfield */
	{ REGION_GFX3, 0, &anlayout,      0, 512 },	/* characters 8x8 */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_thunderj =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz/2,
			main_readmem,main_writemem,0,0,
			ignore_interrupt,1
		},
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz/2,
			extra_readmem,extra_writemem,0,0,
			ignore_interrupt,1
		},
		JSA_II_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	100,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	thunderj_vh_start,
	thunderj_vh_stop,
	thunderj_vh_screenrefresh,

	/* sound hardware */
	JSA_II_MONO(REGION_SOUND1),

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( thunderj )
	ROM_REGION( 0xa0000, REGION_CPU1, 0 )	/* 10*64k for 68000 code */
	ROM_LOAD16_BYTE( "2001.14e",   0x00000, 0x10000, 0xf6a71532 )
	ROM_LOAD16_BYTE( "2002.14c",   0x00001, 0x10000, 0x173ec10d )
	ROM_LOAD16_BYTE( "2003.15e",   0x20000, 0x10000, 0x6e155469 )
	ROM_LOAD16_BYTE( "2004.15c",   0x20001, 0x10000, 0xe9ff1e42 )
	ROM_LOAD16_BYTE( "2005.16e",   0x40000, 0x10000, 0xa40242e7 )
	ROM_LOAD16_BYTE( "2006.16c",   0x40001, 0x10000, 0xaa18b94c )
	ROM_LOAD16_BYTE( "1005.15h",   0x60000, 0x10000, 0x05474ebb )
	ROM_LOAD16_BYTE( "1010.16h",   0x60001, 0x10000, 0xccff21c8 )
	ROM_LOAD16_BYTE( "1007.17e",   0x80000, 0x10000, 0x9c2a8aba )
	ROM_LOAD16_BYTE( "1008.17c",   0x80001, 0x10000, 0x22109d16 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1011.17l",    0x00000, 0x10000, 0xbbbbca45 )
	ROM_LOAD16_BYTE( "1012.17n",    0x00001, 0x10000, 0x53e5e638 )
	ROM_COPY( REGION_CPU1, 0x60000, 0x60000, 0x20000 )

	ROM_REGION( 0x14000, REGION_CPU3, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "tjw65snd.bin",  0x10000, 0x4000, 0xd8feb7fb )
	ROM_CONTINUE(              0x04000, 0xc000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1021.5s",   0x000000, 0x10000, 0xd8432766 )	/* graphics, plane 0 */
	ROM_LOAD( "1025.5r",   0x010000, 0x10000, 0x839feed5 )
	ROM_LOAD( "1029.3p",   0x020000, 0x10000, 0xfa887662 )
	ROM_LOAD( "1033.6p",   0x030000, 0x10000, 0x2addda79 )
	ROM_LOAD( "1022.9s",   0x040000, 0x10000, 0xdcf50371 )	/* graphics, plane 1 */
	ROM_LOAD( "1026.9r",   0x050000, 0x10000, 0x216e72c8 )
	ROM_LOAD( "1030.10s",  0x060000, 0x10000, 0xdc51f606 )
	ROM_LOAD( "1034.10r",  0x070000, 0x10000, 0xf8e35516 )
	ROM_LOAD( "1023.13s",  0x080000, 0x10000, 0xb6dc3f13 )	/* graphics, plane 2 */
	ROM_LOAD( "1027.13r",  0x090000, 0x10000, 0x621cc2ce )
	ROM_LOAD( "1031.14s",  0x0a0000, 0x10000, 0x4682ceb5 )
	ROM_LOAD( "1035.14r",  0x0b0000, 0x10000, 0x7a0e1b9e )
	ROM_LOAD( "1024.17s",  0x0c0000, 0x10000, 0xd84452b5 )	/* graphics, plane 3 */
	ROM_LOAD( "1028.17r",  0x0d0000, 0x10000, 0x0cc20245 )
	ROM_LOAD( "1032.14p",  0x0e0000, 0x10000, 0xf639161a )
	ROM_LOAD( "1036.16p",  0x0f0000, 0x10000, 0xb342443d )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1037.2s",   0x000000, 0x10000, 0x07addba6 )
	ROM_LOAD( "1041.2r",   0x010000, 0x10000, 0x1e9c29e4 )
	ROM_LOAD( "1045.34s",  0x020000, 0x10000, 0xe7235876 )
	ROM_LOAD( "1049.34r",  0x030000, 0x10000, 0xa6eb8265 )
	ROM_LOAD( "1038.6s",   0x040000, 0x10000, 0x2ea543f9 )
	ROM_LOAD( "1042.6r",   0x050000, 0x10000, 0xefabdc2b )
	ROM_LOAD( "1046.7s",   0x060000, 0x10000, 0x6692151f )
	ROM_LOAD( "1050.7r",   0x070000, 0x10000, 0xad7bb5f3 )
	ROM_LOAD( "1039.11s",  0x080000, 0x10000, 0xcb563a40 )
	ROM_LOAD( "1043.11r",  0x090000, 0x10000, 0xb7565eee )
	ROM_LOAD( "1047.12s",  0x0a0000, 0x10000, 0x60877136 )
	ROM_LOAD( "1051.12r",  0x0b0000, 0x10000, 0xd4715ff0 )
	ROM_LOAD( "1040.15s",  0x0c0000, 0x10000, 0x6e910fc2 )
	ROM_LOAD( "1044.15r",  0x0d0000, 0x10000, 0xff67a17a )
	ROM_LOAD( "1048.16s",  0x0e0000, 0x10000, 0x200d45b3 )
	ROM_LOAD( "1052.16r",  0x0f0000, 0x10000, 0x74711ef1 )

	ROM_REGION( 0x010000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1020.4m",   0x000000, 0x10000, 0x65470354 )	/* alphanumerics */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* 256k for ADPCM */
	ROM_LOAD( "tj1016.bin",  0x00000, 0x10000, 0xc10bdf73 )
	ROM_LOAD( "tj1017.bin",  0x10000, 0x10000, 0x4e5e25e8 )
	ROM_LOAD( "tj1018.bin",  0x20000, 0x10000, 0xec81895d )
	ROM_LOAD( "tj1019.bin",  0x30000, 0x10000, 0xa4009037 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_thunderj(void)
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(2, 3, 2, 0x0002);
	atarigen_init_6502_speedup(2, 0x4159, 0x4171);

	/* it looks like they jsr to $800000 as some kind of protection */
	/* put an RTS there so we don't die */
	*rts_address = 0x4e75;
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1990, thunderj, 0, thunderj, thunderj, thunderj, ROT0, "Atari Games", "ThunderJaws" )
