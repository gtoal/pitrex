/***************************************************************************

	Atari Escape hardware

	driver by Aaron Giles

	Games supported:
		* Escape From The Planet Of The Robot Monsters (1989) [2 sets]

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

int eprom_vh_start(void);
void eprom_vh_stop(void);
void eprom_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void eprom_scanline_update(int scanline);



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t *sync_data;



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;
	int newstate2 = 0;

	if (atarigen_video_int_state)
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
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(eprom_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O handling
 *
 *************************************/

static READ16_HANDLER( special_port1_r )
{
	int result = readinputport(1);

	if (atarigen_sound_to_cpu_ready) result ^= 0x0004;
	if (atarigen_cpu_to_sound_ready) result ^= 0x0008;
	result ^= 0x0010;

	return result;
}


static READ16_HANDLER( adc_r )
{
	static int last_offset;
	int result = readinputport(2 + (last_offset & 3));
	last_offset = offset;
	return result;
}



/*************************************
 *
 *	Latch write handler
 *
 *************************************/

static WRITE16_HANDLER( eprom_latch_w )
{
	/* reset extra CPU */
	if (ACCESSING_LSB)
	{
		if (data & 1)
			cpu_set_reset_line(1, CLEAR_LINE);
		else
			cpu_set_reset_line(1, ASSERT_LINE);
	}
}



/*************************************
 *
 *	Synchronization
 *
 *************************************/

static READ16_HANDLER( sync_r )
{
	return sync_data[offset];
}


static WRITE16_HANDLER( sync_w )
{
	int oldword = sync_data[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	sync_data[offset] = newword;
	if ((oldword & 0xff00) != (newword & 0xff00))
		cpu_yield();
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x09ffff, MRA16_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_r },
	{ 0x16cc00, 0x16cc01, sync_r },
	{ 0x160000, 0x16ffff, MRA16_BANK1 },
	{ 0x260000, 0x26000f, input_port_0_word_r },
	{ 0x260010, 0x26001f, special_port1_r },
	{ 0x260020, 0x26002f, adc_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
	{ 0x3e0000, 0x3e0fff, MRA16_RAM },
	{ 0x3f0000, 0x3f9fff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x09ffff, MWA16_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x16cc00, 0x16cc01, sync_w, &sync_data },
	{ 0x160000, 0x16ffff, MWA16_BANK1 },	/* shared */
	{ 0x1f0000, 0x1fffff, atarigen_eeprom_enable_w },
	{ 0x2e0000, 0x2e0001, watchdog_reset16_w },
	{ 0x360000, 0x360001, atarigen_video_int_ack_w },
	{ 0x360010, 0x360011, eprom_latch_w },
	{ 0x360020, 0x360021, atarigen_sound_reset_w },
	{ 0x360030, 0x360031, atarigen_sound_w },
	{ 0x3e0000, 0x3e0fff, paletteram16_IIIIRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x3f0000, 0x3f1fff, ataripf_0_simple_w, &ataripf_0_base },
	{ 0x3f2000, 0x3f3fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3f4000, 0x3f4f7f, atarian_0_vram_w, &atarian_0_base },
	{ 0x3f4f80, 0x3f4fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x3f5000, 0x3f7fff, MWA16_RAM },
	{ 0x3f8000, 0x3f9fff, ataripf_0_upper_msb_w, &ataripf_0_upper },
MEMORY_END



/*************************************
 *
 *	Extra CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( extra_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x16cc00, 0x16cc01, sync_r },
	{ 0x160000, 0x16ffff, MRA16_BANK1 },
	{ 0x260000, 0x26000f, input_port_0_word_r },
	{ 0x260010, 0x26001f, special_port1_r },
	{ 0x260020, 0x26002f, adc_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
MEMORY_END


static MEMORY_WRITE16_START( extra_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x16cc00, 0x16cc01, sync_w, &sync_data },
	{ 0x160000, 0x16ffff, MWA16_BANK1 },	/* shared */
	{ 0x360000, 0x360001, atarigen_video_int_ack_w },
	{ 0x360010, 0x360011, eprom_latch_w },
	{ 0x360020, 0x360021, atarigen_sound_reset_w },
	{ 0x360030, 0x360031, atarigen_sound_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( eprom )
	PORT_START		/* 26000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 26010 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* Input buffer full (@260030) */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED ) /* Output buffer full (@360030) */
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED ) /* ADEOC, end of conversion */
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC0 @ 0x260020 */
	PORT_ANALOG( 0x00ff, 0x0080, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0x10, 0xf0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC1 @ 0x260022 */
	PORT_ANALOG( 0x00ff, 0x0080, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 100, 10, 0x10, 0xf0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC0 @ 0x260024 */
	PORT_ANALOG( 0x00ff, 0x0080, IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x10, 0xf0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC1 @ 0x260026 */
	PORT_ANALOG( 0x00ff, 0x0080, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 100, 10, 0x10, 0xf0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

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
	{ REGION_GFX1, 0, &pfmolayout,  256, 32 },	/* sprites & playfield */
	{ REGION_GFX2, 0, &anlayout,      0, 64 },	/* characters 8x8 */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_eprom =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			ATARI_CLOCK_14MHz/2,
			main_readmem,main_writemem,0,0,
			atarigen_video_int_gen,1
		},
		{
			CPU_M68000,
			ATARI_CLOCK_14MHz/2,
			extra_readmem,extra_writemem,0,0,
			ignore_interrupt,1
		},
		JSA_I_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	10,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	eprom_vh_start,
	eprom_vh_stop,
	eprom_vh_screenrefresh,

	/* sound hardware */
	JSA_I_MONO_WITH_SPEECH,

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( eprom )
	ROM_REGION( 0xa0000, REGION_CPU1, 0 )	/* 10*64k for 68000 code */
	ROM_LOAD16_BYTE( "136069.50a",   0x00000, 0x10000, 0x08888dec )
	ROM_LOAD16_BYTE( "136069.40a",   0x00001, 0x10000, 0x29cb1e97 )
	ROM_LOAD16_BYTE( "136069.50b",   0x20000, 0x10000, 0x702241c9 )
	ROM_LOAD16_BYTE( "136069.40b",   0x20001, 0x10000, 0xfecbf9e2 )
	ROM_LOAD16_BYTE( "136069.50d",   0x40000, 0x10000, 0x0f2f1502 )
	ROM_LOAD16_BYTE( "136069.40d",   0x40001, 0x10000, 0xbc6f6ae8 )
	ROM_LOAD16_BYTE( "136069.40k",   0x60000, 0x10000, 0x130650f6 )
	ROM_LOAD16_BYTE( "136069.50k",   0x60001, 0x10000, 0x1da21ed8 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "136069.10s",   0x00000, 0x10000, 0xdeff6469 )
	ROM_LOAD16_BYTE( "136069.10u",   0x00001, 0x10000, 0x5d7afca2 )
	ROM_COPY( REGION_CPU1, 0x60000,  0x60000, 0x20000 )

	ROM_REGION( 0x14000, REGION_CPU3, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "136069.7b",    0x10000, 0x4000, 0x86e93695 )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "136069.47s",   0x00000, 0x10000, 0x0de9d98d )
	ROM_LOAD( "136069.43s",   0x10000, 0x10000, 0x8eb106ad )
	ROM_LOAD( "136069.38s",   0x20000, 0x10000, 0xbf3d0e18 )
	ROM_LOAD( "136069.32s",   0x30000, 0x10000, 0x48fb2e42 )
	ROM_LOAD( "136069.76s",   0x40000, 0x10000, 0x602d939d )
	ROM_LOAD( "136069.70s",   0x50000, 0x10000, 0xf6c973af )
	ROM_LOAD( "136069.64s",   0x60000, 0x10000, 0x9cd52e30 )
	ROM_LOAD( "136069.57s",   0x70000, 0x10000, 0x4e2c2e7e )
	ROM_LOAD( "136069.47u",   0x80000, 0x10000, 0xe7edcced )
	ROM_LOAD( "136069.43u",   0x90000, 0x10000, 0x9d3e144d )
	ROM_LOAD( "136069.38u",   0xa0000, 0x10000, 0x23f40437 )
	ROM_LOAD( "136069.32u",   0xb0000, 0x10000, 0x2a47ff7b )
	ROM_LOAD( "136069.76u",   0xc0000, 0x10000, 0xb0cead58 )
	ROM_LOAD( "136069.70u",   0xd0000, 0x10000, 0xfbc3934b )
	ROM_LOAD( "136069.64u",   0xe0000, 0x10000, 0x0e07493b )
	ROM_LOAD( "136069.57u",   0xf0000, 0x10000, 0x34f8f0ed )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1360691.25d",  0x00000, 0x04000, 0x409d818e )
ROM_END

ROM_START( eprom2 )
	ROM_REGION( 0xa0000, REGION_CPU1, 0 )	/* 10*64k for 68000 code */
	ROM_LOAD16_BYTE( "1025.50a",   0x00000, 0x10000, 0xb0c9a476 )
	ROM_LOAD16_BYTE( "1024.40a",   0x00001, 0x10000, 0x4cc2c50c )
	ROM_LOAD16_BYTE( "1027.50b",   0x20000, 0x10000, 0x84f533ea )
	ROM_LOAD16_BYTE( "1026.40b",   0x20001, 0x10000, 0x506396ce )
	ROM_LOAD16_BYTE( "1029.50d",   0x40000, 0x10000, 0x99810b9b )
	ROM_LOAD16_BYTE( "1028.40d",   0x40001, 0x10000, 0x08ab41f2 )
	ROM_LOAD16_BYTE( "1033.40k",   0x60000, 0x10000, 0x395fc203 )
	ROM_LOAD16_BYTE( "1032.50k",   0x60001, 0x10000, 0xa19c8acb )
	ROM_LOAD16_BYTE( "1037.50e",   0x80000, 0x10000, 0xad39a3dd )
	ROM_LOAD16_BYTE( "1036.40e",   0x80001, 0x10000, 0x34fc8895 )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1035.10s",    0x00000, 0x10000, 0xffeb5647 )
	ROM_LOAD16_BYTE( "1034.10u",    0x00001, 0x10000, 0xc68f58dd )
	ROM_COPY( REGION_CPU1, 0x60000, 0x60000, 0x20000 )

	ROM_REGION( 0x14000, REGION_CPU3, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "136069.7b",    0x10000, 0x4000, 0x86e93695 )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "136069.47s",   0x00000, 0x10000, 0x0de9d98d )
	ROM_LOAD( "136069.43s",   0x10000, 0x10000, 0x8eb106ad )
	ROM_LOAD( "136069.38s",   0x20000, 0x10000, 0xbf3d0e18 )
	ROM_LOAD( "136069.32s",   0x30000, 0x10000, 0x48fb2e42 )
	ROM_LOAD( "136069.76s",   0x40000, 0x10000, 0x602d939d )
	ROM_LOAD( "136069.70s",   0x50000, 0x10000, 0xf6c973af )
	ROM_LOAD( "136069.64s",   0x60000, 0x10000, 0x9cd52e30 )
	ROM_LOAD( "136069.57s",   0x70000, 0x10000, 0x4e2c2e7e )
	ROM_LOAD( "136069.47u",   0x80000, 0x10000, 0xe7edcced )
	ROM_LOAD( "136069.43u",   0x90000, 0x10000, 0x9d3e144d )
	ROM_LOAD( "136069.38u",   0xa0000, 0x10000, 0x23f40437 )
	ROM_LOAD( "136069.32u",   0xb0000, 0x10000, 0x2a47ff7b )
	ROM_LOAD( "136069.76u",   0xc0000, 0x10000, 0xb0cead58 )
	ROM_LOAD( "136069.70u",   0xd0000, 0x10000, 0xfbc3934b )
	ROM_LOAD( "136069.64u",   0xe0000, 0x10000, 0x0e07493b )
	ROM_LOAD( "136069.57u",   0xf0000, 0x10000, 0x34f8f0ed )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1360691.25d",  0x00000, 0x04000, 0x409d818e )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_eprom(void)
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(2, 6, 1, 0x0002);
	atarigen_init_6502_speedup(2, 0x4158, 0x4170);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1989, eprom,  0,     eprom, eprom, eprom, ROT0, "Atari Games", "Escape from the Planet of the Robot Monsters (set 1)" )
GAME( 1989, eprom2, eprom, eprom, eprom, eprom, ROT0, "Atari Games", "Escape from the Planet of the Robot Monsters (set 2)" )
