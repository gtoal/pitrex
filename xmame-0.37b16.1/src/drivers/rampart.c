/***************************************************************************

	Atari Rampart hardware

	driver by Aaron Giles

	Games supported:
		* Rampart (1990) [3 sets]

	Known bugs:
		* slapstic emulation currently imcomplete

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

WRITE16_HANDLER( rampart_bitmap_w );

int rampart_vh_start(void);
void rampart_vh_stop(void);
void rampart_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern data16_t *rampart_bitmap;



/*************************************
 *
 *	Externals
 *
 *************************************/

static data16_t *slapstic_base;
static UINT32 current_bank;



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


static void scanline_update(int scanline)
{
	/* generate 32V signals */
	if ((scanline & 32) == 0)
		atarigen_scanline_int_gen();
}



/*************************************
 *
 *	Slapstic fun & joy
 *
 *************************************/

static UINT32 bank_list[] = { 0x2000, 0x3000, 0x0000, 0x1000 };

static READ16_HANDLER( slapstic_bank_r )
{
	int opcode_pc = cpu_getpreviouspc();
	int result;

	/* if the previous PC was 1400E6, then we will be passing through 1400E8 as
	   we decode this instruction. 1400E8 -> 0074 which represents a significant
	   location on the Rampart slapstic; best to tweak it here */
	if (opcode_pc == 0x1400e6)
	{
		current_bank = bank_list[slapstic_tweak(0x00e6 / 2)];
		current_bank = bank_list[slapstic_tweak(0x00e8 / 2)];
		current_bank = bank_list[slapstic_tweak(0x00ea / 2)];
	}

	/* tweak the slapstic and adjust the bank */
	current_bank = bank_list[slapstic_tweak(offset)];
	result = slapstic_base[current_bank + (offset & 0xfff)];

	/* if we did the special hack above, then also tweak for the following
	   instruction fetch, which will force the bank switch to occur */
	if (opcode_pc == 0x1400e6)
		current_bank = bank_list[slapstic_tweak(0x00ec / 2)];

	/* adjust the bank and return the result */
	return result;
}


static WRITE16_HANDLER( slapstic_bank_w )
{
}


static OPBASE_HANDLER( opbase_override )
{
	int oldpc = cpu_getpreviouspc();

	/* tweak the slapstic at the source PC */
	if (oldpc >= 0x140000 && oldpc < 0x148000)
		slapstic_bank_r(oldpc - 0x140000,0);

	/* tweak the slapstic at the destination PC */
	if (address >= 0x140000 && address < 0x148000)
	{
		current_bank = bank_list[slapstic_tweak((address - 0x140000) / 2)];

		/* use a bogus ophw so that we will be called again on the next jump/ret */
		catch_nextBranch();

		/* compute the new ROM base */
		OP_RAM = OP_ROM = (UINT8 *)&slapstic_base[current_bank] - 0x140000;

		/* return -1 so that the standard routine doesn't do anything more */
		address = -1;

		logerror("Slapstic op override at %06X\n", address);
	}

	return address;
}



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void init_machine(void)
{
	atarigen_eeprom_reset();
	slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(scanline_update, 32);
}



/*************************************
 *
 *	MSM5295 I/O
 *
 *************************************/

static READ16_HANDLER( adpcm_r )
{
	return (OKIM6295_status_0_r(offset) << 8) | 0x00ff;
}


static WRITE16_HANDLER( adpcm_w )
{
	if (ACCESSING_MSB)
		OKIM6295_data_0_w(offset, (data >> 8) & 0xff);
}



/*************************************
 *
 *	YM2413 I/O
 *
 *************************************/

static WRITE16_HANDLER( ym2413_w )
{
	if (ACCESSING_MSB)
	{
		if (offset & 2)
			YM2413_data_port_0_w(0, (data >> 8) & 0xff);
		else
			YM2413_register_port_0_w(0, (data >> 8) & 0xff);
	}
}



/*************************************
 *
 *	Latch write
 *
 *************************************/

static WRITE16_HANDLER( latch_w )
{
	/* bit layout in this register:

		0x8000 == VCR ???
		0x2000 == LETAMODE1 (controls right trackball)
		0x1000 == CBANK (color bank -- is it ever set to non-zero?)
		0x0800 == LETAMODE0 (controls center and left trackballs)
		0x0400 == LETARES (reset LETA analog control reader)

		0x0020 == PMIX0 (ADPCM mixer level)
		0x0010 == /PCMRES (ADPCM reset)
		0x000E == YMIX2-0 (YM2413 mixer level)
		0x0001 == /YAMRES (YM2413 reset)
	*/

	/* upper byte being modified? */
	if (ACCESSING_MSB)
	{
		if (data & 0x1000)
			logerror("Color bank set to 1!\n");
	}

	/* lower byte being modified? */
	if (ACCESSING_LSB)
	{
		atarigen_set_ym2413_vol(((data >> 1) & 7) * 100 / 7);
		atarigen_set_oki6295_vol((data & 0x0020) ? 100 : 0);
	}
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x140000, 0x147fff, slapstic_bank_r },
	{ 0x200000, 0x21ffff, MRA16_RAM },
	{ 0x3c0000, 0x3c07ff, MRA16_RAM },
	{ 0x3e0000, 0x3effff, MRA16_RAM },
	{ 0x460000, 0x460001, adpcm_r },
	{ 0x500000, 0x500fff, atarigen_eeprom_r },
	{ 0x640000, 0x640001, input_port_0_word_r },
	{ 0x640002, 0x640003, input_port_1_word_r },
	{ 0x6c0000, 0x6c0001, input_port_2_word_r },
	{ 0x6c0002, 0x6c0003, input_port_3_word_r },
	{ 0x6c0004, 0x6c0005, input_port_4_word_r },
	{ 0x6c0006, 0x6c0007, input_port_5_word_r },
	{ 0x6c0008, 0x6c0009, input_port_6_word_r },
	{ 0x6c000a, 0x6c000b, input_port_7_word_r },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x140000, 0x147fff, slapstic_bank_w, &slapstic_base },	/* here only to initialize the pointer */
	{ 0x200000, 0x21ffff, rampart_bitmap_w, &rampart_bitmap },
	{ 0x220000, 0x3bffff, MWA16_NOP },	/* the code blasts right through this when initializing */
	{ 0x3c0000, 0x3c07ff, atarigen_expanded_666_paletteram_w, &paletteram16 },
	{ 0x3c0800, 0x3dffff, MWA16_NOP },	/* the code blasts right through this when initializing */
	{ 0x3e0000, 0x3e07ff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3e0800, 0x3e3f3f, MWA16_RAM },
	{ 0x3e3f40, 0x3e3f7f, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x3e3f80, 0x3effff, MWA16_RAM },
	{ 0x460000, 0x460001, adpcm_w },
	{ 0x480000, 0x480003, ym2413_w },
	{ 0x500000, 0x500fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x5a0000, 0x5affff, atarigen_eeprom_enable_w },
	{ 0x640000, 0x640001, latch_w },
	{ 0x720000, 0x72ffff, watchdog_reset16_w },
	{ 0x7e0000, 0x7effff, atarigen_scanline_int_ack_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( rampart )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BITX(  0x0004, 0x0004, IPT_DIPSWITCH_NAME, "Number of Players", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x0000, "2-player Version")
	PORT_DIPSETTING(    0x0004, "3-player Version")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0800, IP_ACTIVE_LOW )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER2, 100, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_X | IPF_REVERSE | IPF_PLAYER2, 100, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER1, 100, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_X | IPF_REVERSE | IPF_PLAYER1, 100, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER3, 100, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
    PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_X | IPF_REVERSE | IPF_PLAYER3, 100, 30, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( ramprt2p )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BITX(  0x0004, 0x0000, IPT_DIPSWITCH_NAME, "Number of Players", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x0000, "2-player Version")
	PORT_DIPSETTING(    0x0004, "3-player Version")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0800, IP_ACTIVE_LOW )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0xf000, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout molayout =
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
	{ REGION_GFX1, 0, &molayout,  256, 16 },
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
	{ ATARI_CLOCK_14MHz/4/3/165 },
	{ REGION_SOUND1 },
	{ 100 }
};


static struct YM2413interface ym2413_interface =
{
	1,
	ATARI_CLOCK_14MHz/4,
	{ 75 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_rampart =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz/2,
			main_readmem,main_writemem,0,0,
			atarigen_video_int_gen,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	init_machine,

	/* video hardware */
	43*8, 30*8, { 0*8+4, 43*8-1-4, 0*8, 30*8-1 },
	gfxdecodeinfo,
	512,512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	rampart_vh_start,
	rampart_vh_stop,
	rampart_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		},
		{
			SOUND_YM2413,
			&ym2413_interface
		}
	},

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( rampart )
	ROM_REGION( 0x148000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "082-1033.13l", 0x00000, 0x80000, 0x5c36795f )
	ROM_LOAD16_BYTE( "082-1032.13j", 0x00001, 0x80000, 0xec7bc38c )
	ROM_LOAD16_BYTE( "082-2031.13l", 0x00000, 0x10000, 0x07650c7e )
	ROM_LOAD16_BYTE( "082-2030.13h", 0x00001, 0x10000, 0xe2bf2a26 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "082-1009.2n",   0x000000, 0x20000, 0x23b95f59 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM data */
	ROM_LOAD( "082-1007.2d", 0x00000, 0x20000, 0xc96a0fc3 )
	ROM_LOAD( "082-1008.1d", 0x20000, 0x20000, 0x518218d9 )
ROM_END


ROM_START( ramprt2p )
	ROM_REGION( 0x148000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "082-1033.13l", 0x00000, 0x80000, 0x5c36795f )
	ROM_LOAD16_BYTE( "082-1032.13j", 0x00001, 0x80000, 0xec7bc38c )
	ROM_LOAD16_BYTE( "205113kl.rom", 0x00000, 0x20000, 0xd4e26d0f )
	ROM_LOAD16_BYTE( "205013h.rom",  0x00001, 0x20000, 0xed2a49bd )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "10192n.rom",   0x000000, 0x20000, 0xefa38bef )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM data */
	ROM_LOAD( "082-1007.2d", 0x00000, 0x20000, 0xc96a0fc3 )
	ROM_LOAD( "082-1008.1d", 0x20000, 0x20000, 0x518218d9 )
ROM_END


ROM_START( rampartj )
	ROM_REGION( 0x148000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "3451.bin",  0x00000, 0x20000, 0xc6596d32 )
	ROM_LOAD16_BYTE( "3450.bin",  0x00001, 0x20000, 0x563b33cc )
	ROM_LOAD16_BYTE( "1463.bin",  0x40000, 0x20000, 0x65fe3491 )
	ROM_LOAD16_BYTE( "1462.bin",  0x40001, 0x20000, 0xba731652 )
	ROM_LOAD16_BYTE( "1465.bin",  0x80000, 0x20000, 0x9cb87d1b )
	ROM_LOAD16_BYTE( "1464.bin",  0x80001, 0x20000, 0x2ff75c40 )
	ROM_LOAD16_BYTE( "1467.bin",  0xc0000, 0x20000, 0xe0cfcda5 )
	ROM_LOAD16_BYTE( "1466.bin",  0xc0001, 0x20000, 0xa7a5a951 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2419.bin",   0x000000, 0x20000, 0x456a8aae )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM data */
	ROM_LOAD( "082-1007.2d", 0x00000, 0x20000, 0xc96a0fc3 )
	ROM_LOAD( "082-1008.1d", 0x20000, 0x20000, 0x518218d9 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_rampart(void)
{
	static const UINT16 compressed_default_eeprom[] =
	{
		0x0001,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0150,0x0101,
		0x0100,0x0151,0x0300,0x0151,0x0400,0x0150,0x0101,0x01FB,
		0x021E,0x0104,0x011A,0x0200,0x011A,0x0700,0x01FF,0x0E00,
		0x01FF,0x0E00,0x01FF,0x0150,0x0101,0x0100,0x0151,0x0300,
		0x0151,0x0400,0x0150,0x0101,0x01FB,0x021E,0x0104,0x011A,
		0x0200,0x011A,0x0700,0x01AD,0x0150,0x0129,0x0187,0x01CD,
		0x0113,0x0100,0x0172,0x0179,0x0140,0x0186,0x0113,0x0100,
		0x01E5,0x0149,0x01F8,0x012A,0x019F,0x0185,0x01E7,0x0113,
		0x0100,0x01C3,0x01B5,0x0115,0x0184,0x0113,0x0100,0x0179,
		0x014E,0x01B7,0x012F,0x016D,0x01B7,0x01D5,0x010B,0x0100,
		0x0163,0x0242,0x01B6,0x010B,0x0100,0x01B9,0x0104,0x01B7,
		0x01F0,0x01DD,0x01B5,0x0119,0x010B,0x0100,0x01C2,0x012D,
		0x0142,0x01B4,0x010B,0x0100,0x01C5,0x0115,0x01BB,0x016F,
		0x01A2,0x01CF,0x01D3,0x0107,0x0100,0x0192,0x01CD,0x0142,
		0x01CE,0x0107,0x0100,0x0170,0x0136,0x01B1,0x0140,0x017B,
		0x01CD,0x01FB,0x0107,0x0100,0x0144,0x013B,0x0148,0x01CC,
		0x0107,0x0100,0x0181,0x0139,0x01FF,0x0E00,0x01FF,0x0E00,
		0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,
		0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,
		0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,
		0x0000
	};

	atarigen_eeprom_default = compressed_default_eeprom;
	memcpy(&memory_region(REGION_CPU1)[0x140000], &memory_region(REGION_CPU1)[0x40000], 0x8000);
	atarigen_invert_region(REGION_GFX1);
	slapstic_init(118);

	/* set up some hacks to handle the slapstic accesses */
	memory_set_opbase_handler(0, opbase_override);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1990, rampart,  0,       rampart, rampart,  rampart, ROT0, "Atari Games", "Rampart (3-player Trackball)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1990, ramprt2p, rampart, rampart, ramprt2p, rampart, ROT0, "Atari Games", "Rampart (2-player Joystick)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1990, rampartj, rampart, rampart, ramprt2p, rampart, ROT0, "Atari Games", "Rampart (Japan, 2-player Joystick)", GAME_UNEMULATED_PROTECTION )
