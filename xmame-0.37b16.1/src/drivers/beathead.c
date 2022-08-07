/***************************************************************************

	Atari "Stella on Steroids" hardware

	driver by Aaron Giles

	Games supported:
		* BeatHead

	Known bugs:
		* none known

****************************************************************************

	Memory map

	===================================================================================================
	MAIN CPU
	===================================================================================================
	00000000-0001FFFFF  R/W   xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx   Main RAM
	01800000-01BFFFFFF  R     xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx   Main ROM
	40000000-4000007FF  R/W   -------- -------- -------- xxxxxxxx   EEPROM
	41000000            R     -------- -------- -------- xxxxxxxx   Data from sound board
	41000000              W   -------- -------- -------- xxxxxxxx   Data to sound board
	41000100            R     -------- -------- -------- -----xxx   Interrupt enables
	                          -------- -------- -------- -----x--      (scanline int enable)
	                          -------- -------- -------- ------x-      (unknown int enable)
	                          -------- -------- -------- -------x      (unknown int enable)
	41000100              W   -------- -------- -------- --------   Interrupt acknowledge
	41000104              W   -------- -------- -------- --------   Unknown int disable
	41000108              W   -------- -------- -------- --------   Unknown int disable
	4100010c              W   -------- -------- -------- --------   Scanline int disable
	41000114              W   -------- -------- -------- --------   Unknown int enable
	41000118              W   -------- -------- -------- --------   Unknown int enable
	4100011c              W   -------- -------- -------- --------   Scanline int enable
	41000200            R     -------- -------- xxxx--xx xxxx--xx   Player 2/3 inputs
	                    R     -------- -------- xxxx---- --------      (player 3 joystick UDLR)
	                    R     -------- -------- ------x- --------      (player 3 button 1)
	                    R     -------- -------- -------x --------      (player 3 button 2)
	                    R     -------- -------- -------- xxxx----      (player 2 joystick UDLR)
	                    R     -------- -------- -------- ------x-      (player 2 button 1)
	                    R     -------- -------- -------- -------x      (player 2 button 2)
	41000204            R     -------- -------- xxxx--xx xxxx--xx   Player 1/4 inputs
	                    R     -------- -------- xxxx---- --------      (player 1 joystick UDLR)
	                    R     -------- -------- ------x- --------      (player 1 button 1)
	                    R     -------- -------- -------x --------      (player 1 button 2)
	                    R     -------- -------- -------- xxxx----      (player 4 joystick UDLR)
	                    R     -------- -------- -------- ------x-      (player 4 button 1)
	                    R     -------- -------- -------- -------x      (player 4 button 2)
	41000208              W   -------- -------- -------- --------   Sound /RESET assert
	4100020C              W   -------- -------- -------- --------   Sound /RESET deassert
	41000220              W   -------- -------- -------- --------   Coin counter assert
	41000224              W   -------- -------- -------- --------   Coin counter deassert
	41000300            R     -------- -------- xxxxxxxx -xxx----   DIP switches/additional inputs
	                    R     -------- -------- xxxxxxxx --------      (debug DIP switches)
	                    R     -------- -------- -------- -x------      (service switch)
	                    R     -------- -------- -------- --x-----      (sound output buffer full)
	                    R     -------- -------- -------- ---x----      (sound input buffer full)
	41000304            R     -------- -------- -------- xxxxxxxx   Coin/service inputs
	                    R     -------- -------- -------- xxxx----      (service inputs: R,RC,LC,L)
	                    R     -------- -------- -------- ----xxxx      (coin inputs: R,RC,LC,L)
	41000400              W   -------- -------- -------- -xxxxxxx   Palette select
	41000500              W   -------- -------- -------- --------   EEPROM write enable
	41000600              W   -------- -------- -------- ----xxxx   Finescroll, vertical SYNC flags
	                      W   -------- -------- -------- ----x---      (VBLANK)
	                      W   -------- -------- -------- -----x--      (VSYNC)
	                      W   -------- -------- -------- ------xx      (fine scroll value)
	41000700              W   -------- -------- -------- --------   Watchdog reset
	42000000-4201FFFF   R/W   -------- -------- xxxxxxxx xxxxxxxx   Palette RAM
	                    R/W   -------- -------- x------- --------      (LSB of all three components)
	                    R/W   -------- -------- -xxxxx-- --------      (red component)
	                    R/W   -------- -------- ------xx xxx-----      (green component)
	                    R/W   -------- -------- -------- ---xxxxx      (blue component)
	43000000              W   -------- -------- ----xxxx xxxxxxxx   HSYNC RAM address latch
	                      W   -------- -------- ----x--- --------      (counter enable)
	                      W   -------- -------- -----xxx xxxxxxxx      (RAM address)
	43000004            R/W   -------- -------- -------- xxxxx---   HSYNC RAM data latch
	                    R/W   -------- -------- -------- x-------      (generate IRQ)
	                    R/W   -------- -------- -------- -x------      (VRAM shift enable)
	                    R/W   -------- -------- -------- --x-----      (HBLANK)
	                    R/W   -------- -------- -------- ---x----      (/HSYNC)
	                    R/W   -------- -------- -------- ----x---      (release wait for sync)
	43000008              W   -------- -------- -------- ---x-xx-   HSYNC unknown control
	8DF80000            R     -------- -------- -------- --------   Unknown
	8F380000-8F3FFFFF     W   -------- -------- -------- --------   VRAM latch address
	8F900000-8F97FFFF     W   xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx   VRAM transparent write
	8F980000-8F9FFFFF   R/W   xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx   VRAM standard read/write
	8FB80000-8FBFFFFF     W   ----xxxx ----xxxx ----xxxx ----xxxx   VRAM "bulk" write
	                      W   ----xxxx -------- -------- --------      (enable byte lanes for word 3?)
	                      W   -------- ----xxxx -------- --------      (enable byte lanes for word 2?)
	                      W   -------- -------- ----xxxx --------      (enable byte lanes for word 1?)
	                      W   -------- -------- -------- ----xxxx      (enable byte lanes for word 0?)
	8FFF8000              W   xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx   VRAM "bulk" data latch
	9E280000-9E2FFFFF     W   -------- -------- -------- --------   VRAM copy destination address latch
	===================================================================================================

***************************************************************************/


#include "driver.h"
#include "cpu/asap/asap.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "vidhrdw/generic.h"



/*************************************
 *
 *	Externals
 *
 *************************************/

extern data32_t *	beathead_vram_bulk_latch;
extern data32_t *	beathead_palette_select;

int beathead_vh_start(void);
void beathead_vh_stop(void);
void beathead_scanline_update(int scanline);
void beathead_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

WRITE32_HANDLER( beathead_vram_transparent_w );
WRITE32_HANDLER( beathead_vram_bulk_w );
WRITE32_HANDLER( beathead_vram_latch_w );
WRITE32_HANDLER( beathead_vram_copy_w );
WRITE32_HANDLER( beathead_finescroll_w );
WRITE32_HANDLER( beathead_palette_w );
READ32_HANDLER( beathead_hsync_ram_r );
WRITE32_HANDLER( beathead_hsync_ram_w );



/*************************************
 *
 *	Statics
 *
 *************************************/

static data32_t *	ram_base;
static data32_t *	rom_base;

static double		hblank_offset;

static UINT8		irq_line_state;
static UINT8		irq_enable[3];
static UINT8		irq_state[3];

static UINT8		eeprom_enabled;
static data32_t *	eeprom_data;


#define MAX_SCANLINES	262



/*************************************
 *
 *	Machine init
 *
 *************************************/

static void update_interrupts(void);

static void scanline_callback(int scanline)
{
	/* update the video */
	beathead_scanline_update(scanline);

	/* on scanline zero, clear any halt condition */
	if (scanline == 0)
		cpu_set_halt_line(0, CLEAR_LINE);

	/* wrap around at 262 */
	scanline++;
	if (scanline >= MAX_SCANLINES)
		scanline = 0;

	/* set the scanline IRQ */
	irq_state[2] = 1;
	update_interrupts();

	/* set the timer for the next one */
	timer_set(cpu_getscanlinetime(scanline) - hblank_offset, scanline, scanline_callback);
}


static void init_machine(void)
{
	/* reset the common subsystems */
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarijsa_reset();

	/* the code is temporarily mapped at 0 at startup */
	/* just copying the first 0x40 bytes is sufficient */
	memcpy(ram_base, rom_base, 0x40);

	/* compute the timing of the HBLANK interrupt and set the first timer */
	hblank_offset = cpu_getscanlineperiod() * ((455. - 336. - 25.) / 455.);
	timer_set(cpu_getscanlinetime(0) - hblank_offset, 0, scanline_callback);

	/* reset IRQs */
	irq_line_state = CLEAR_LINE;
	irq_state[0] = irq_state[1] = irq_state[2] = 0;
	irq_enable[0] = irq_enable[1] = irq_enable[2] = 0;
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static void update_interrupts(void)
{
	int gen_int;

	/* compute the combined interrupt signal */
	gen_int  = irq_state[0] & irq_enable[0];
	gen_int |= irq_state[1] & irq_enable[1];
	gen_int |= irq_state[2] & irq_enable[2];
	gen_int  = gen_int ? ASSERT_LINE : CLEAR_LINE;

	/* if it's changed since the last time, call through */
	if (irq_line_state != gen_int)
	{
		irq_line_state = gen_int;
		if (irq_line_state != CLEAR_LINE)
			cpu_set_irq_line(0, ASAP_IRQ0, irq_line_state);
		else
			asap_set_irq_line(ASAP_IRQ0, irq_line_state);
	}
}


static WRITE32_HANDLER( interrupt_control_w )
{
	int irq = offset & 3;
	int control = (offset >> 2) & 1;

	/* offsets 1-3 seem to be the enable latches for the IRQs */
	if (irq != 0)
		irq_enable[irq - 1] = control;

	/* offset 0 seems to be the interrupt ack */
	else
		irq_state[0] = irq_state[1] = irq_state[2] = 0;

	/* update the current state */
	update_interrupts();
}


static READ32_HANDLER( interrupt_control_r )
{
	/* return the enables as a bitfield */
	return (irq_enable[0]) | (irq_enable[1] << 1) | (irq_enable[2] << 2);
}



/*************************************
 *
 *	EEPROM handling
 *
 *************************************/

static void nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file, eeprom_data, 0x800);
	else if (file)
		osd_fread(file, eeprom_data, 0x800);
	else
		memset(eeprom_data, 0xff, 0x800);
}


static WRITE32_HANDLER( eeprom_data_w )
{
	if (eeprom_enabled)
	{
		mem_mask |= 0xffffff00;
		COMBINE_DATA(&eeprom_data[offset]);
		eeprom_enabled = 0;
	}
}


static WRITE32_HANDLER( eeprom_enable_w )
{
	eeprom_enabled = 1;
}



/*************************************
 *
 *	Input handling
 *
 *************************************/

static READ32_HANDLER( input_0_r )
{
	return readinputport(0);
}


static READ32_HANDLER( input_1_r )
{
	return readinputport(1);
}


static READ32_HANDLER( input_2_r )
{
	int result = readinputport(2);
	if (atarigen_sound_to_cpu_ready) result ^= 0x10;
	if (atarigen_cpu_to_sound_ready) result ^= 0x20;
	return result;
}


static READ32_HANDLER( input_3_r )
{
	return readinputport(3);
}



/*************************************
 *
 *	Sound communication
 *
 *************************************/

static READ32_HANDLER( sound_data_r )
{
	return atarigen_sound_r(offset,0);
}


static WRITE32_HANDLER( sound_data_w )
{
	if (ACCESSING_LSB32)
		atarigen_sound_w(offset, data, mem_mask);
}


static WRITE32_HANDLER( sound_reset_w )
{
	logerror("Sound reset = %d\n", !offset);
	cpu_set_reset_line(1, offset ? CLEAR_LINE : ASSERT_LINE);
}



/*************************************
 *
 *	Misc other I/O
 *
 *************************************/

static WRITE32_HANDLER( watchdog_reset32_w )
{
	watchdog_reset_w(0,0);
}


static WRITE32_HANDLER( coin_count_w )
{
	coin_counter_w(0, !offset);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ32_START( readmem )
	{ 0x00000000, 0x0001ffff, MRA32_RAM },
	{ 0x01800000, 0x01bfffff, MRA32_ROM },
	{ 0x40000000, 0x400007ff, MRA32_RAM },
	{ 0x41000000, 0x41000003, sound_data_r },
	{ 0x41000100, 0x41000103, interrupt_control_r },
	{ 0x41000200, 0x41000203, input_0_r },
	{ 0x41000204, 0x41000207, input_1_r },
	{ 0x41000300, 0x41000303, input_2_r },
	{ 0x41000304, 0x41000307, input_3_r },
	{ 0x42000000, 0x4201ffff, MRA32_RAM },
	{ 0x43000000, 0x43000007, beathead_hsync_ram_r },
	{ 0x8df80000, 0x8df80003, MRA32_NOP },	/* noisy x4 during scanline int */
	{ 0x8f980000, 0x8f9fffff, MRA32_RAM },
MEMORY_END


static MEMORY_WRITE32_START( writemem )
	{ 0x00000000, 0x0001ffff, MWA32_RAM, &ram_base },
	{ 0x01800000, 0x01bfffff, MWA32_ROM, &rom_base },
	{ 0x40000000, 0x400007ff, eeprom_data_w, &eeprom_data },
	{ 0x41000000, 0x41000003, sound_data_w },
	{ 0x41000100, 0x4100011f, interrupt_control_w },
	{ 0x41000208, 0x4100020f, sound_reset_w },
	{ 0x41000220, 0x41000227, coin_count_w },
	{ 0x41000400, 0x41000403, MWA32_RAM, &beathead_palette_select },
	{ 0x41000500, 0x41000503, eeprom_enable_w },
	{ 0x41000600, 0x41000603, beathead_finescroll_w },
	{ 0x41000700, 0x41000703, watchdog_reset32_w },
	{ 0x42000000, 0x4201ffff, beathead_palette_w, &paletteram32 },
	{ 0x43000000, 0x43000007, beathead_hsync_ram_w },
	{ 0x8f380000, 0x8f3fffff, beathead_vram_latch_w },
	{ 0x8f900000, 0x8f97ffff, beathead_vram_transparent_w },
	{ 0x8f980000, 0x8f9fffff, MWA32_RAM, &videoram32 },
	{ 0x8fb80000, 0x8fbfffff, beathead_vram_bulk_w },
	{ 0x8fff8000, 0x8fff8003, MWA32_RAM, &beathead_vram_bulk_latch },
	{ 0x9e280000, 0x9e2fffff, beathead_vram_copy_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( beathead )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0006, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_III_PORT	/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_beathead =
{
	/* basic machine hardware */
	{
		{
			CPU_ASAP,			/* verified */
			ATARI_CLOCK_14MHz,
			readmem,writemem,0,0,
			ignore_interrupt,1
		},
		JSA_III_CPU
	},
	60, (int)(((262. - 240.) / 262.) * 1000000. / 60.),
	1,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	0,
	0x8000,0x8000,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	beathead_vh_start,
	beathead_vh_stop,
	beathead_vh_screenrefresh,

	/* sound hardware */
	JSA_III_MONO(REGION_SOUND1),
	nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( beathead )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )			/* dummy ASAP region */

	ROM_REGION( 0x14000, REGION_CPU2, 0 )			/* 64k + 16k for 6502 code */
	ROM_LOAD( "bhsnd.bin",  0x10000, 0x4000, 0xdfd33f02 )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION32_LE( 0x400000, REGION_USER1, ROMREGION_DISPOSE )	/* 4MB for ASAP code */
	ROM_LOAD32_BYTE( "bhprog0.bin", 0x000000, 0x80000, 0x87975721 )
	ROM_LOAD32_BYTE( "bhprog1.bin", 0x000001, 0x80000, 0x25d89743 )
	ROM_LOAD32_BYTE( "bhprog2.bin", 0x000002, 0x80000, 0x87722609 )
	ROM_LOAD32_BYTE( "bhprog3.bin", 0x000003, 0x80000, 0xa795d616 )
	ROM_LOAD32_BYTE( "bhpics0.bin", 0x200000, 0x80000, 0x926bf65d )
	ROM_LOAD32_BYTE( "bhpics1.bin", 0x200001, 0x80000, 0xa8f12e41 )
	ROM_LOAD32_BYTE( "bhpics2.bin", 0x200002, 0x80000, 0x00b96481 )
	ROM_LOAD32_BYTE( "bhpics3.bin", 0x200003, 0x80000, 0x99c4f1db )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )		/* 1MB for ADPCM */
	ROM_LOAD( "bhpcm0.bin",  0x80000, 0x20000, 0x609ca626 )
	ROM_LOAD( "bhpcm1.bin",  0xa0000, 0x20000, 0x35511509 )
	ROM_LOAD( "bhpcm2.bin",  0xc0000, 0x20000, 0xf71a840a )
	ROM_LOAD( "bhpcm3.bin",  0xe0000, 0x20000, 0xfedd4936 )
ROM_END



/*************************************
 *
 *	Driver speedups
 *
 *************************************/

/*
	In-game hotspot @ 0180F8D8
*/


static data32_t *speedup_data;
static READ32_HANDLER( speedup_r )
{
	int result = *speedup_data;
	if ((cpu_getpreviouspc() & 0xfffff) == 0x006f0 && result == cpu_get_reg(ASAP_R3))
		cpu_spinuntil_int();
	return result;
}


static data32_t *movie_speedup_data;
static READ32_HANDLER( movie_speedup_r )
{
	int result = *movie_speedup_data;
	if ((cpu_getpreviouspc() & 0xfffff) == 0x00a88 && (cpu_get_reg(ASAP_R28) & 0xfffff) == 0x397c0 &&
		movie_speedup_data[4] == cpu_get_reg(ASAP_R1))
	{
		UINT32 temp = (INT16)result + movie_speedup_data[4] * 262;
		if (temp - (UINT32)cpu_get_reg(ASAP_R15) < (UINT32)cpu_get_reg(ASAP_R23))
			cpu_spinuntil_int();
	}
	return result;
}



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_beathead(void)
{
	/* initialize the common systems */
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 4, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarigen_init_6502_speedup(1, 0x4321, 0x4339);

	/* copy the ROM data */
	memcpy(rom_base, memory_region(REGION_USER1), memory_region_length(REGION_USER1));

	/* prepare the speedups */
	speedup_data = install_mem_read32_handler(0, 0x00000ae8, 0x00000aeb, speedup_r);
	movie_speedup_data = install_mem_read32_handler(0, 0x00000804, 0x00000807, movie_speedup_r);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1993, beathead, 0, beathead, beathead, beathead, ROT0_16BIT, "Atari Games", "BeatHead (prototype)" )
