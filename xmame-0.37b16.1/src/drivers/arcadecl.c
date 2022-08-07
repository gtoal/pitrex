/***************************************************************************

	Atari Arcade Classics hardware (prototypes)

    driver by Aaron Giles

	Games supported:
		* Arcade Classics (1992)
		* Sparkz (1982)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	MAIN CPU
	========================================================================
	000000-0FFFFF   R     xxxxxxxx xxxxxxxx   Program ROM
	200000-21FFFF   R/W   xxxxxxxx xxxxxxxx   Playfield RAM (512x256 pixels)
	                R/W   xxxxxxxx --------      (Left pixel)
	                R/W   -------- xxxxxxxx      (Right pixel)
	3C0000-3C01FF   R/W   xxxxxxxx xxxxxxxx   Playfield palette RAM (256 entries)
	                R/W   x------- --------      (RGB 1 LSB)
	                R/W   -xxxxx-- --------      (Red 5 MSB)
	                R/W   ------xx xxx-----      (Green 5 MSB)
	                R/W   -------- ---xxxxx      (Blue 5 MSB)
	3C0200-3C03FF   R/W   xxxxxxxx xxxxxxxx   Motion object palette RAM (256 entries)
	3C0400-3C07FF   R/W   xxxxxxxx xxxxxxxx   Extra palette RAM (512 entries)
	3E0000-3E07FF   R/W   xxxxxxxx xxxxxxxx   Motion object RAM (256 entries x 4 words)
	                R/W   -------- xxxxxxxx      (0: Link to next object)
	                R/W   x------- --------      (1: Horizontal flip)
	                R/W   -xxxxxxx xxxxxxxx      (1: Tile index)
	                R/W   xxxxxxxx x-------      (2: X position)
	                R/W   -------- ----xxxx      (2: Palette select)
	                R/W   xxxxxxxx x-------      (3: Y position)
	                R/W   -------- -xxx----      (3: Number of X tiles - 1)
	                R/W   -------- -----xxx      (3: Number of Y tiles - 1)
	3E0800-3EFFFF   R/W   xxxxxxxx xxxxxxxx   Extra sprite RAM
	640000          R     xxxxxxxx --------   Input port 1
	640002          R     xxxxxxxx --------   Input port 2
	640010          R     -------- xx------   Status port
	                R     -------- x-------      (VBLANK)
	                R     -------- -x------      (Self test)
	640012          R     -------- --xx--xx   Coin inputs
	                R     -------- --xx----      (Service coins)
	                R     -------- ------xx      (Coin switches)
	640020-640027   R     -------- xxxxxxxx   Analog inputs
	640040            W   -------- x--xxxxx   Sound control
	                  W   -------- x-------      (ADPCM bank select)
	                  W   -------- ---xxxxx      (Volume)
	640060            W   -------- --------   EEPROM enable
	641000-641FFF   R/W   -------- xxxxxxxx   EEPROM
	642000-642001   R/W   xxxxxxxx --------   MSM6295 communications
	646000            W   -------- --------   32V IRQ acknowledge
	647000            W   -------- --------   Watchdog reset
	========================================================================
	Interrupts:
		IRQ4 = 32V
	========================================================================

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Externals
 *
 *************************************/

WRITE16_HANDLER( rampart_bitmap_w );

int arcadecl_vh_start(void);
void arcadecl_vh_stop(void);
void arcadecl_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern data16_t *rampart_bitmap;



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
 *	Initialization
 *
 *************************************/

static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(scanline_update, 32);
}



/*************************************
 *
 *	MSM6295 I/O
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
 *	Latch write
 *
 *************************************/

static WRITE16_HANDLER( latch_w )
{
	/* bit layout in this register:

		0x0080 == ADPCM bank
		0x001F == volume
	*/

	/* lower byte being modified? */
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, (data & 0x80) ? 0x40000 : 0x00000);
		atarigen_set_oki6295_vol((data & 0x001f) * 100 / 0x1f);
	}
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x200000, 0x21ffff, MRA16_RAM },
	{ 0x3c0000, 0x3c07ff, MRA16_RAM },
	{ 0x3e0000, 0x3effff, MRA16_RAM },
	{ 0x640000, 0x640001, input_port_0_word_r },
	{ 0x640002, 0x640003, input_port_1_word_r },
	{ 0x640010, 0x640011, input_port_2_word_r },
	{ 0x640012, 0x640013, input_port_3_word_r },
	{ 0x640020, 0x640021, input_port_4_word_r },
	{ 0x640022, 0x640023, input_port_5_word_r },
	{ 0x640024, 0x640025, input_port_6_word_r },
	{ 0x640026, 0x640027, input_port_7_word_r },
	{ 0x641000, 0x641fff, atarigen_eeprom_r },
	{ 0x642000, 0x642001, adpcm_r },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x200000, 0x21ffff, rampart_bitmap_w, &rampart_bitmap },
	{ 0x3c0000, 0x3c07ff, atarigen_expanded_666_paletteram_w, &paletteram16 },
	{ 0x3e0000, 0x3e07ff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3e0800, 0x3effbf, MWA16_RAM },
	{ 0x3effc0, 0x3effff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x640040, 0x64004f, latch_w },
	{ 0x640060, 0x64006f, atarigen_eeprom_enable_w },
	{ 0x641000, 0x641fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x642000, 0x642001, adpcm_w },
	{ 0x646000, 0x646fff, atarigen_scanline_int_ack_w },
	{ 0x647000, 0x647fff, watchdog_reset16_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( arcadecl )
	PORT_START	/* 640000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640002 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640010 */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640012 */
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(  0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT(  0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640020 */
    PORT_ANALOGX( 0x00ff, 0, IPT_TRACKBALL_X | IPF_REVERSE | IPF_PLAYER2, 50, 32, 0, 0, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640022 */
	PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_Y | IPF_PLAYER2, 50, 32, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640024 */
    PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_X | IPF_REVERSE | IPF_PLAYER1, 50, 32, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640026 */
    PORT_ANALOG( 0x00ff, 0, IPT_TRACKBALL_Y | IPF_PLAYER1, 50, 32, 0, 0 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( sparkz )
	PORT_START	/* 640000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START	/* 640002 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START	/* 640010 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640012 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640020 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640022 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640024 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 640026 */
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
	{ ATARI_CLOCK_14MHz/4/3/165 },	/* not verified -- assumed from Rampart */
	{ REGION_SOUND1 },
	{ 100 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_arcadecl =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz,
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
	arcadecl_vh_start,
	arcadecl_vh_stop,
	arcadecl_vh_screenrefresh,

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

ROM_START( arcadecl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "pgm0",  0x00000, 0x80000, 0xb5b93623 )
	ROM_LOAD16_BYTE( "prog1", 0x00001, 0x80000, 0xe7efef85 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "atcl_mob",   0x00000, 0x80000, 0x0e9b3930 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "adpcm",      0x00000, 0x80000, 0x03ca7f03 )
ROM_END


ROM_START( sparkz )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sparkzpg.0", 0x00000, 0x80000, 0xa75c331c )
	ROM_LOAD16_BYTE( "sparkzpg.1", 0x00001, 0x80000, 0x1af1fc04 )

	ROM_REGION( 0x20, REGION_GFX1, ROMREGION_DISPOSE )
	/* empty */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "sparkzsn",      0x00000, 0x80000, 0x87097ce2 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_arcadecl(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_invert_region(REGION_GFX1);
}


static void init_sparkz(void)
{
	atarigen_eeprom_default = NULL;
	memset(memory_region(REGION_GFX1), 0, memory_region_length(REGION_GFX1));
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1992, arcadecl, 0, arcadecl, arcadecl, arcadecl, ROT0_16BIT, "Atari Games", "Arcade Classics (prototype)" )
GAME( 1992, sparkz,   0, arcadecl, sparkz,   sparkz,   ROT0,       "Atari Games", "Sparkz (prototype)" )
