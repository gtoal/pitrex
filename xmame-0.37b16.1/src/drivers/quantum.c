/***************************************************************************

	Atari Food Fight hardware

	driver by Paul Forgey, with some help from Aaron Giles

	Games supported:
		* Quantum

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	QUANTUM MEMORY MAP (per schem):

	000000-003FFF	ROM0
	004000-004FFF	ROM1
	008000-00BFFF	ROM2
	00C000-00FFFF	ROM3
	010000-013FFF	ROM4

	018000-01BFFF	RAM0
	01C000-01CFFF	RAM1

	940000			TRACKBALL
	948000			SWITCHES
	950000			COLORRAM
	958000			CONTROL (LED and coin control)
	960000-970000	RECALL (nvram read)
	968000			VGRST (vector reset)
	970000			VGGO (vector go)
	978000			WDCLR (watchdog)
	900000			NVRAM (nvram write)
	840000			I/OS (sound and dip switches)
	800000-801FFF	VMEM (vector display list)
	940000			I/O (shematic label really - covered above)
	900000			DTACK1

***************************************************************************/


#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t *nvram;



/*************************************
 *
 *	NVRAM handler
 *
 *************************************/

static void nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file, nvram, 512);
	else if (file)
		osd_fread(file, nvram, 512);
	else
		memset(nvram, 0xff, 512);
}



/*************************************
 *
 *	Interrupts
 *
 *************************************/

static int interrupt_gen(void)
{
	return 1; /* ipl0' == ivector 1 */
}



/*************************************
 *
 *	Inputs
 *
 *************************************/

static READ16_HANDLER( switches_r )
{
	return (readinputport(0) | (avgdvg_done() ? 1 : 0));
}


static READ16_HANDLER( trackball_r )
{
	return (readinputport(4) << 4) | readinputport(3);
}


static READ_HANDLER( input_1_r )
{
	return (readinputport(1) << (7 - (offset - POT0_C))) & 0x80;
}


static READ_HANDLER( input_2_r )
{
	return (readinputport(2) << (7 - (offset - POT0_C))) & 0x80;
}



/*************************************
 *
 *	LEDs/coin counters
 *
 *************************************/

static WRITE16_HANDLER( led_w )
{
	if (ACCESSING_LSB)
	{
		/* bits 0 and 1 are coin counters */
		coin_counter_w(0, data & 2);
		coin_counter_w(1, data & 1);

		/* bit 3 = select second trackball for cocktail mode? */

		/* bits 4 and 5 are LED controls */
		set_led_status(0, data & 0x10);
		set_led_status(1, data & 0x20);

		/* bits 6 and 7 flip screen */
		vector_set_flip_x (data & 0x40);
		vector_set_flip_y (data & 0x80);
		vector_set_swap_xy (1);	/* vertical game */
	}
}



/*************************************
 *
 *	POKEY I/O
 *
 *************************************/

static WRITE16_HANDLER( pokey_word_w )
{
	if (offset & 0x10) /* A5 selects chip */
		pokey2_w(offset & 0x0f, data);
	else
		pokey1_w(offset & 0x0f, data);
}


static READ16_HANDLER( pokey_word_r )
{
	if (offset & 0x10)
		return pokey2_r(offset & 0x0f);
	else
		return pokey1_r(offset & 0x0f);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

MEMORY_READ16_START( readmem )
	{ 0x000000, 0x013fff, MRA16_ROM },
	{ 0x018000, 0x01cfff, MRA16_RAM },
	{ 0x800000, 0x801fff, MRA16_RAM },
	{ 0x840000, 0x84003f, pokey_word_r },
	{ 0x900000, 0x9001ff, MRA16_RAM },
	{ 0x940000, 0x940001, trackball_r }, /* trackball */
	{ 0x948000, 0x948001, switches_r },
	{ 0x978000, 0x978001, MRA16_NOP },	/* ??? */
MEMORY_END


MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x013fff, MWA16_ROM },
	{ 0x018000, 0x01cfff, MWA16_RAM },
	{ 0x800000, 0x801fff, MWA16_RAM, (data16_t **)&vectorram, &vectorram_size },
	{ 0x840000, 0x84003f, pokey_word_w },
	{ 0x900000, 0x9001ff, MWA16_RAM, &nvram },
	{ 0x950000, 0x95001f, quantum_colorram_w },
	{ 0x958000, 0x958001, led_w },
	{ 0x960000, 0x960001, MWA16_NOP },	/* enable NVRAM? */
	{ 0x968000, 0x968001, avgdvg_reset_word_w },
/*	{ 0x970000, 0x970001, avgdvg_go_w }, */
/*	{ 0x978000, 0x978001, watchdog_reset_w }, */
	/* the following is wrong, but it's the only way I found to fix the service mode */
	{ 0x978000, 0x978001, avgdvg_go_word_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( quantum )
	PORT_START		/* IN0 */
	/* YHALT here MUST BE ALWAYS 0  */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,IPT_SPECIAL )	/* vg YHALT */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

/* first POKEY is SW2, second is SW1 -- more confusion! */
	PORT_START 		/* DSW0 */
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x30, 0x00, "Right Coin" )
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x20, "*4" )
	PORT_DIPSETTING(    0x10, "*5" )
	PORT_DIPSETTING(    0x30, "*6" )
	PORT_DIPNAME( 0x08, 0x00, "Left Coin" )
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x08, "*2" )
	PORT_DIPNAME( 0x07, 0x00, "Bonus Coins" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x01, "1 each 5" )
	PORT_DIPSETTING(    0x02, "1 each 4" )
	PORT_DIPSETTING(    0x05, "1 each 3" )
	PORT_DIPSETTING(    0x06, "2 each 4" )

	PORT_START		/* DSW1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_ANALOG( 0x0f, 0, IPT_TRACKBALL_Y | IPF_REVERSE, 10, 10, 0,0)

	PORT_START      /* IN3 */
	PORT_ANALOG( 0x0f, 0, IPT_TRACKBALL_X, 10, 10, 0, 0 )
INPUT_PORTS_END



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	2,	/* 2 chips */
	600000,        /* .6 MHz? (hand tuned) */
	{ 50, 50 },
	/* The 8 pot handlers */
	{ input_1_r, input_2_r },
	{ input_1_r, input_2_r },
	{ input_1_r, input_2_r },
	{ input_1_r, input_2_r },
	{ input_1_r, input_2_r },
	{ input_1_r, input_2_r },
	{ input_1_r, input_2_r },
	{ input_1_r, input_2_r },
	/* The allpot handler */
	{ 0, 0 },
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_quantum =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			6000000,			/* 6MHz */
			readmem,writemem,0,0,
			interrupt_gen,3		/* IRQ rate = 750kHz/4096 */
		}
	},
	60, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,
	0,

	/* video hardware */
	400, 300, { 0, 600, 0, 900 },
	0,
	256, 0,
	avg_init_palette_multi,

	VIDEO_TYPE_VECTOR | VIDEO_SUPPORTS_DIRTY,
	0,
	avg_start_quantum,
	avg_stop,
	vector_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
	},

	nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( quantum )
	ROM_REGION( 0x014000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE( "136016.201",   0x000000, 0x002000, 0x7e7be63a )
    ROM_LOAD16_BYTE( "136016.206",   0x000001, 0x002000, 0x2d8f5759 )
    ROM_LOAD16_BYTE( "136016.102",   0x004000, 0x002000, 0x408d34f4 )
    ROM_LOAD16_BYTE( "136016.107",   0x004001, 0x002000, 0x63154484 )
    ROM_LOAD16_BYTE( "136016.203",   0x008000, 0x002000, 0xbdc52fad )
    ROM_LOAD16_BYTE( "136016.208",   0x008001, 0x002000, 0xdab4066b )
    ROM_LOAD16_BYTE( "136016.104",   0x00C000, 0x002000, 0xbf271e5c )
    ROM_LOAD16_BYTE( "136016.109",   0x00C001, 0x002000, 0xd2894424 )
    ROM_LOAD16_BYTE( "136016.105",   0x010000, 0x002000, 0x13ec512c )
    ROM_LOAD16_BYTE( "136016.110",   0x010001, 0x002000, 0xacb50363 )
ROM_END


ROM_START( quantum1 )
	ROM_REGION( 0x014000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE( "136016.101",   0x000000, 0x002000, 0x5af0bd5b )
    ROM_LOAD16_BYTE( "136016.106",   0x000001, 0x002000, 0xf9724666 )
    ROM_LOAD16_BYTE( "136016.102",   0x004000, 0x002000, 0x408d34f4 )
    ROM_LOAD16_BYTE( "136016.107",   0x004001, 0x002000, 0x63154484 )
    ROM_LOAD16_BYTE( "136016.103",   0x008000, 0x002000, 0x948f228b )
    ROM_LOAD16_BYTE( "136016.108",   0x008001, 0x002000, 0xe4c48e4e )
    ROM_LOAD16_BYTE( "136016.104",   0x00C000, 0x002000, 0xbf271e5c )
    ROM_LOAD16_BYTE( "136016.109",   0x00C001, 0x002000, 0xd2894424 )
    ROM_LOAD16_BYTE( "136016.105",   0x010000, 0x002000, 0x13ec512c )
    ROM_LOAD16_BYTE( "136016.110",   0x010001, 0x002000, 0xacb50363 )
ROM_END


ROM_START( quantump )
	ROM_REGION( 0x014000, REGION_CPU1, 0 )
    ROM_LOAD16_BYTE( "quantump.2e",  0x000000, 0x002000, 0x176d73d3 )
    ROM_LOAD16_BYTE( "quantump.3e",  0x000001, 0x002000, 0x12fc631f )
    ROM_LOAD16_BYTE( "quantump.2f",  0x004000, 0x002000, 0xb64fab48 )
    ROM_LOAD16_BYTE( "quantump.3f",  0x004001, 0x002000, 0xa52a9433 )
    ROM_LOAD16_BYTE( "quantump.2h",  0x008000, 0x002000, 0x5b29cba3 )
    ROM_LOAD16_BYTE( "quantump.3h",  0x008001, 0x002000, 0xc64fc03a )
    ROM_LOAD16_BYTE( "quantump.2k",  0x00C000, 0x002000, 0x854f9c09 )
    ROM_LOAD16_BYTE( "quantump.3k",  0x00C001, 0x002000, 0x1aac576c )
    ROM_LOAD16_BYTE( "quantump.2l",  0x010000, 0x002000, 0x1285b5e7 )
    ROM_LOAD16_BYTE( "quantump.3l",  0x010001, 0x002000, 0xe19de844 )
ROM_END



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1982, quantum,  0,       quantum, quantum, 0, ROT270, "Atari", "Quantum (rev 2)", GAME_WRONG_COLORS )
GAMEX( 1982, quantum1, quantum, quantum, quantum, 0, ROT270, "Atari", "Quantum (rev 1)", GAME_WRONG_COLORS )
GAMEX( 1982, quantump, quantum, quantum, quantum, 0, ROT270, "Atari", "Quantum (prototype)", GAME_WRONG_COLORS )
