/***************************************************************************

	Atari Gauntlet hardware

	driver by Aaron Giles

	Games supported:
		* Gauntlet (1985) [3 sets]
		* Gauntlet 2-player Version (1985)
		* Gauntlet II (1986)
		* Vindicators Part II (1988)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	MAIN CPU
	========================================================================
	000000-037FFF   R     xxxxxxxx xxxxxxxx   Program ROM
	038000-03FFFF   R     xxxxxxxx xxxxxxxx   Slapstic-protected ROM
	040000-07FFFF   R     xxxxxxxx xxxxxxxx   Program ROM
	800000-801FFF   R/W   xxxxxxxx xxxxxxxx   Program RAM
	802000-802FFF   R/W   -------- xxxxxxxx   EEPROM
	803000          R     -------- xxxxxxxx   Input port 1
	803002          R     -------- xxxxxxxx   Input port 2
	803004          R     -------- xxxxxxxx   Input port 3
	803006          R     -------- xxxxxxxx   Input port 4
	803008          R     -------- -xxxx---   Status port
	                R     -------- -x------      (VBLANK)
	                R     -------- --x-----      (Sound command buffer full)
	                R     -------- ---x----      (Sound response buffer full)
	                R     -------- ----x---      (Self test)
	80300E          R     -------- xxxxxxxx   Sound response read
	803100            W   -------- --------   Watchdog reset
	80312E            W   -------- -------x   Sound CPU reset
	803140            W   -------- --------   VBLANK IRQ acknowledge
	803150            W   -------- --------   EEPROM enable
	803170            W   -------- xxxxxxxx   Sound command write
	900000-901FFF   R/W   xxxxxxxx xxxxxxxx   Playfield RAM (64x64 tiles)
	                R/W   x------- --------      (Horizontal flip)
	                R/W   -xxx---- --------      (Palette select)
	                R/W   ----xxxx xxxxxxxx      (Tile index)
	902000-903FFF   R/W   xxxxxxxx xxxxxxxx   Motion object RAM (1024 entries x 4 words)
	                R/W   -xxxxxxx xxxxxxxx      (0: Tile index)
	                R/W   xxxxxxxx x-------      (1024: X position)
	                R/W   -------- ----xxxx      (1024: Palette select)
	                R/W   xxxxxxxx x-------      (2048: Y position)
	                R/W   -------- -x------      (2048: Horizontal flip)
	                R/W   -------- --xxx---      (2048: Number of X tiles - 1)
	                R/W   -------- -----xxx      (2048: Number of Y tiles - 1)
	                R/W   ------xx xxxxxxxx      (3072: Link to next object)
	904000-904FFF   R/W   xxxxxxxx xxxxxxxx   Spare video RAM
	905000-905FFF   R/W   xxxxxxxx xxxxxxxx   Alphanumerics RAM (64x32 tiles)
	                R/W   x------- --------      (Opaque/transparent)
	                R/W   -xxxxx-- --------      (Palette select)
	                R/W   ------xx xxxxxxxx      (Tile index)
	905F6E          R/W   xxxxxxxx x-----xx   Playfield Y scroll/tile bank select
	                R/W   xxxxxxxx x-------      (Playfield Y scroll)
	                R/W   -------- ------xx      (Playfield tile bank select)
	910000-9101FF   R/W   xxxxxxxx xxxxxxxx   Alphanumercs palette RAM (256 entries)
	                R/W   xxxx---- --------      (Intensity)
	                R/W   ----xxxx --------      (Red)
	                R/W   -------- xxxx----      (Green)
	                R/W   -------- ----xxxx      (Blue)
	910200-9103FF   R/W   xxxxxxxx xxxxxxxx   Motion object palette RAM (256 entries)
	910400-9105FF   R/W   xxxxxxxx xxxxxxxx   Playfield palette RAM (256 entries)
	910600-9107FF   R/W   xxxxxxxx xxxxxxxx   Extra palette RAM (256 entries)
	930000            W   xxxxxxxx x-------   Playfield X scroll
	========================================================================
	Interrupts:
		IRQ4 = VBLANK
		IRQ6 = sound CPU communications
	========================================================================


	========================================================================
	SOUND CPU
	========================================================================
	0000-0FFF   R/W   xxxxxxxx   Program RAM
	1000          W   xxxxxxxx   Sound response write
	1010        R     xxxxxxxx   Sound command read
	1020        R     ----xxxx   Coin inputs
	            R     ----x---      (Coin 1)
	            R     -----x--      (Coin 2)
	            R     ------x-      (Coin 3)
	            R     -------x      (Coin 4)
	1020          W   xxxxxxxx   Mixer control
	              W   xxx-----      (TMS5220 volume)
	              W   ---xx---      (POKEY volume)
	              W   -----xxx      (YM2151 volume)
	1030        R     xxxx----   Sound status read
	            R     x-------      (Sound command buffer full)
	            R     -x------      (Sound response buffer full)
	            R     --x-----      (TMS5220 ready)
	            R     ---x----      (Self test)
	1030          W   x-------   YM2151 reset
	1031          W   x-------   TMS5220 data strobe
	1032          W   x-------   TMS5220 reset
	1033          W   x-------   TMS5220 frequency
	1800-180F   R/W   xxxxxxxx   POKEY communications
	1810-1811   R/W   xxxxxxxx   YM2151 communications
	1820          W   xxxxxxxx   TMS5220 data latch
	1830        R/W   --------   IRQ acknowledge
	4000-FFFF   R     xxxxxxxx   Program ROM
	========================================================================
	Interrupts:
		IRQ = timed interrupt
		NMI = latch on sound command
	========================================================================

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"



/*************************************
 *
 *	Externals
 *
 *************************************/

WRITE16_HANDLER( gauntlet_xscroll_w );
WRITE16_HANDLER( gauntlet_yscroll_w );

int gauntlet_vh_start(void);
void gauntlet_vh_stop(void);
void gauntlet_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern UINT8 vindctr2_screen_refresh;
extern data16_t *gauntlet_yscroll;



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t *speed_check;
static UINT32 last_speed_check;

static UINT8 speech_val;
static UINT8 last_speech_write;
static UINT8 speech_squeak;

static data16_t sound_reset_val;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate |= 4;
	if (atarigen_sound_int_state)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void scanline_update(int scanline)
{
	/* sound IRQ is on 32V */
	if (scanline & 32)
		atarigen_6502_irq_gen();
	else
		atarigen_6502_irq_ack_r(0);
}


static void init_machine(void)
{
	last_speed_check = 0;
	last_speech_write = 0x80;
	sound_reset_val = 1;
	speech_squeak = 0;

	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(scanline_update, 32);
	atarigen_sound_io_reset(1);
}



/*************************************
 *
 *	Controller reads
 *
 *************************************/

static int fake_inputs(int real_port, int fake_port)
{
	int result = readinputport(real_port);
	int fake = readinputport(fake_port);

	if (fake & 0x01)			/* up */
	{
		if (fake & 0x04)		/* up and left */
			result &= ~0x20;
		else if (fake & 0x08)	/* up and right */
			result &= ~0x10;
		else					/* up only */
			result &= ~0x30;
	}
	else if (fake & 0x02)		/* down */
	{
		if (fake & 0x04)		/* down and left */
			result &= ~0x80;
		else if (fake & 0x08)	/* down and right */
			result &= ~0x40;
		else					/* down only */
			result &= ~0xc0;
	}
	else if (fake & 0x04)		/* left only */
		result &= ~0x60;
	else if (fake & 0x08)		/* right only */
		result &= ~0x90;

	return result;
}


static READ16_HANDLER( port0_r )
{
	return vindctr2_screen_refresh ?
				fake_inputs(0, 6) :
				readinputport(readinputport(6));
}


static READ16_HANDLER( port1_r )
{
	return vindctr2_screen_refresh ?
				fake_inputs(1, 7) :
				readinputport((readinputport(6) != 1) ? 1 : 0);
}


static READ16_HANDLER( port2_r )
{
	return vindctr2_screen_refresh ?
				readinputport(2) :
				readinputport((readinputport(6) != 2) ? 2 : 0);
}


static READ16_HANDLER( port3_r )
{
	return vindctr2_screen_refresh ?
				readinputport(3) :
				readinputport((readinputport(6) != 3) ? 3 : 0);
}


static READ16_HANDLER( port4_r )
{
	int temp = readinputport(4);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0020;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x0010;
	return temp;
}



/*************************************
 *
 *	Sound reset
 *
 *************************************/

static WRITE16_HANDLER( sound_reset_w )
{
	if (ACCESSING_LSB)
	{
		int oldword = sound_reset_val;
		COMBINE_DATA(&sound_reset_val);

		if ((oldword ^ sound_reset_val) & 1)
		{
			cpu_set_reset_line(1, (sound_reset_val & 1) ? CLEAR_LINE : ASSERT_LINE);
			atarigen_sound_reset();
		}
	}
}



/*************************************
 *
 *	Speed cheats
 *
 *************************************/

static READ16_HANDLER( speedup_68010_r )
{
	int result = speed_check[offset];
	int time = cpu_gettotalcycles();
	int delta = time - last_speed_check;

	last_speed_check = time;
	if (delta <= 100 && result == 0 && delta >= 0)
		cpu_spin();

	return result;
}


static WRITE16_HANDLER( speedup_68010_w )
{
	last_speed_check -= 1000;
	COMBINE_DATA(&speed_check[offset]);
}



/*************************************
 *
 *	Sound I/O inputs
 *
 *************************************/

static READ_HANDLER( switch_6502_r )
{
	int temp = 0x30;

	if (atarigen_cpu_to_sound_ready) temp ^= 0x80;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x40;
	if (tms5220_ready_r()) temp ^= 0x20;
	if (!(readinputport(4) & 0x0008)) temp ^= 0x10;

	return temp;
}



/*************************************
 *
 *	Sound TMS5220 write
 *
 *************************************/

static WRITE_HANDLER( tms5220_w )
{
	speech_val = data;
}



/*************************************
 *
 *	Sound control write
 *
 *************************************/

static WRITE_HANDLER( sound_ctl_w )
{
	switch (offset & 7)
	{
		case 0:	/* music reset, bit D7, low reset */
			break;

		case 1:	/* speech write, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_data_w(0, speech_val);
			last_speech_write = data;
			break;

		case 2:	/* speech reset, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_reset();
			break;

		case 3:	/* speech squeak, bit D7 */
			data = 5 | ((data >> 6) & 2);
			tms5220_set_frequency(ATARI_CLOCK_14MHz/2 / (16 - data));
			break;
	}
}



/*************************************
 *
 *	Sound mixer write
 *
 *************************************/

static WRITE_HANDLER( mixer_w )
{
	atarigen_set_ym2151_vol((data & 7) * 100 / 7);
	atarigen_set_pokey_vol(((data >> 3) & 3) * 100 / 3);
	atarigen_set_tms5220_vol(((data >> 5) & 7) * 100 / 7);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x800000, 0x801fff, MRA16_RAM },
	{ 0x802000, 0x802fff, atarigen_eeprom_r },
	{ 0x803000, 0x803001, port0_r },
	{ 0x803002, 0x803003, port1_r },
	{ 0x803004, 0x803005, port2_r },
	{ 0x803006, 0x803007, port3_r },
	{ 0x803008, 0x803009, port4_r },
	{ 0x80300e, 0x80300f, atarigen_sound_r },
	{ 0x900000, 0x905fff, MRA16_RAM },
	{ 0x910000, 0x9107ff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x800000, 0x801fff, MWA16_RAM },
	{ 0x802000, 0x802fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x803100, 0x803101, watchdog_reset16_w },
	{ 0x80312e, 0x80312f, sound_reset_w },
	{ 0x803140, 0x803141, atarigen_video_int_ack_w },
	{ 0x803150, 0x803151, atarigen_eeprom_enable_w },
	{ 0x803170, 0x803171, atarigen_sound_w },
	{ 0x900000, 0x901fff, ataripf_0_simple_w, &ataripf_0_base },
	{ 0x902000, 0x903fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x904000, 0x904fff, MWA16_RAM },
	{ 0x905f6e, 0x905f6f, gauntlet_yscroll_w, &gauntlet_yscroll },
	{ 0x905000, 0x905f7f, atarian_0_vram_w, &atarian_0_base },
	{ 0x905f80, 0x905fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x910000, 0x9107ff, paletteram16_IIIIRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x930000, 0x930001, gauntlet_xscroll_w },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1010, 0x101f, atarigen_6502_sound_r },
	{ 0x1020, 0x102f, input_port_5_r },
	{ 0x1030, 0x103f, switch_6502_r },
	{ 0x1800, 0x180f, pokey1_r },
	{ 0x1811, 0x1811, YM2151_status_port_0_r },
	{ 0x1830, 0x183f, atarigen_6502_irq_ack_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x100f, atarigen_6502_sound_w },
	{ 0x1020, 0x102f, mixer_w },
	{ 0x1030, 0x103f, sound_ctl_w },
	{ 0x1800, 0x180f, pokey1_w },
	{ 0x1810, 0x1810, YM2151_register_port_0_w },
	{ 0x1811, 0x1811, YM2151_data_port_0_w },
	{ 0x1820, 0x182f, tms5220_w },
	{ 0x1830, 0x183f, atarigen_6502_irq_ack_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( gauntlet )
	PORT_START	/* 803000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803004 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803006 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803008 */
	PORT_BIT( 0x0007, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0030, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 1020 (sound) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Fake! */
	PORT_DIPNAME( 0x0003, 0x0000, "Player 1 Plays" )
	PORT_DIPSETTING(      0x0000, "Red/Warrior" )
	PORT_DIPSETTING(      0x0001, "Blue/Valkyrie" )
	PORT_DIPSETTING(      0x0002, "Yellow/Wizard" )
	PORT_DIPSETTING(      0x0003, "Green/Elf" )
INPUT_PORTS_END


INPUT_PORTS_START( vindctr2 )
	PORT_START	/* 803000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803004 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803006 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803008 */
	PORT_BIT( 0x0007, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0030, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 1020 (sound) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* single joystick */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )

	PORT_START	/* single joystick */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
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
	{ REGION_GFX2, 0, &pfmolayout,  256, 32 },
	{ REGION_GFX1, 0, &anlayout,      0, 64 },
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct YM2151interface ym2151_interface =
{
	1,
	ATARI_CLOCK_14MHz/4,
	{ YM3012_VOL(48,MIXER_PAN_LEFT,48,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct POKEYinterface pokey_interface =
{
	1,
	ATARI_CLOCK_14MHz/8,
	{ 32 },
};


static struct TMS5220interface tms5220_interface =
{
	ATARI_CLOCK_14MHz/2/11,	/* potentially ATARI_CLOCK_14MHz/2/9 as well */
	80,
	0
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_gauntlet =
{
	/* basic machine hardware */
	{
		{
			CPU_M68010,		/* verified */
			ATARI_CLOCK_14MHz/2,
			main_readmem,main_writemem,0,0,
			atarigen_video_int_gen,1
		},
		{
			CPU_M6502,
			ATARI_CLOCK_14MHz/8,
			sound_readmem,sound_writemem,0,0,
			0,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1024,1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	gauntlet_vh_start,
	gauntlet_vh_stop,
	gauntlet_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_POKEY,
			&pokey_interface
		},
		{
			SOUND_TMS5220,
			&tms5220_interface
		}
	},

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( gauntlet )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gauntlt1.9a",  0x00000, 0x08000, 0x46fe8743 )
	ROM_LOAD16_BYTE( "gauntlt1.9b",  0x00001, 0x08000, 0x276e15c4 )
	ROM_LOAD16_BYTE( "gauntlt1.10a", 0x38000, 0x04000, 0x6d99ed51 )
	ROM_LOAD16_BYTE( "gauntlt1.10b", 0x38001, 0x04000, 0x545ead91 )
	ROM_LOAD16_BYTE( "gauntlt1.7a",  0x40000, 0x08000, 0x6fb8419c )
	ROM_LOAD16_BYTE( "gauntlt1.7b",  0x40001, 0x08000, 0x931bd2a0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt1.16r", 0x4000, 0x4000, 0x6ee7f3cc )
	ROM_LOAD( "gauntlt1.16s", 0x8000, 0x8000, 0xfa19861f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.6p",  0x00000, 0x04000, 0x6c276a1d )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.1a",  0x00000, 0x08000, 0x91700f33 )
	ROM_LOAD( "gauntlt1.1b",  0x08000, 0x08000, 0x869330be )

	ROM_LOAD( "gauntlt1.1l",  0x10000, 0x08000, 0xd497d0a8 )
	ROM_LOAD( "gauntlt1.1mn", 0x18000, 0x08000, 0x29ef9882 )

	ROM_LOAD( "gauntlt1.2a",  0x20000, 0x08000, 0x9510b898 )
	ROM_LOAD( "gauntlt1.2b",  0x28000, 0x08000, 0x11e0ac5b )

	ROM_LOAD( "gauntlt1.2l",  0x30000, 0x08000, 0x29a5db41 )
	ROM_LOAD( "gauntlt1.2mn", 0x38000, 0x08000, 0x8bf3b263 )
ROM_END


ROM_START( gauntir1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gaun1ir1.9a",  0x00000, 0x08000, 0xfd871f81 )
	ROM_LOAD16_BYTE( "gaun1ir1.9b",  0x00001, 0x08000, 0xbcb2fb1d )
	ROM_LOAD16_BYTE( "gaun1ir1.10a", 0x38000, 0x04000, 0x4642cd95 )
	ROM_LOAD16_BYTE( "gaun1ir1.10b", 0x38001, 0x04000, 0xc8df945e )
	ROM_LOAD16_BYTE( "gaun1ir1.7a",  0x40000, 0x08000, 0xc57377b3 )
	ROM_LOAD16_BYTE( "gaun1ir1.7b",  0x40001, 0x08000, 0x1cac2071 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt1.16r", 0x4000, 0x4000, 0x6ee7f3cc )
	ROM_LOAD( "gauntlt1.16s", 0x8000, 0x8000, 0xfa19861f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.6p",  0x00000, 0x04000, 0x6c276a1d )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.1a",  0x00000, 0x08000, 0x91700f33 )
	ROM_LOAD( "gauntlt1.1b",  0x08000, 0x08000, 0x869330be )

	ROM_LOAD( "gauntlt1.1l",  0x10000, 0x08000, 0xd497d0a8 )
	ROM_LOAD( "gauntlt1.1mn", 0x18000, 0x08000, 0x29ef9882 )

	ROM_LOAD( "gauntlt1.2a",  0x20000, 0x08000, 0x9510b898 )
	ROM_LOAD( "gauntlt1.2b",  0x28000, 0x08000, 0x11e0ac5b )

	ROM_LOAD( "gauntlt1.2l",  0x30000, 0x08000, 0x29a5db41 )
	ROM_LOAD( "gauntlt1.2mn", 0x38000, 0x08000, 0x8bf3b263 )
ROM_END


ROM_START( gauntir2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gaun1ir1.9a",  0x00000, 0x08000, 0xfd871f81 )
	ROM_LOAD16_BYTE( "gaun1ir1.9b",  0x00001, 0x08000, 0xbcb2fb1d )
	ROM_LOAD16_BYTE( "gaun1ir1.10a", 0x38000, 0x04000, 0x4642cd95 )
	ROM_LOAD16_BYTE( "gaun1ir1.10b", 0x38001, 0x04000, 0xc8df945e )
	ROM_LOAD16_BYTE( "gaun1ir2.7a",  0x40000, 0x08000, 0x73e1ad79 )
	ROM_LOAD16_BYTE( "gaun1ir2.7b",  0x40001, 0x08000, 0xfd248cea )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt1.16r", 0x4000, 0x4000, 0x6ee7f3cc )
	ROM_LOAD( "gauntlt1.16s", 0x8000, 0x8000, 0xfa19861f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.6p",  0x00000, 0x04000, 0x6c276a1d )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.1a",  0x00000, 0x08000, 0x91700f33 )
	ROM_LOAD( "gauntlt1.1b",  0x08000, 0x08000, 0x869330be )

	ROM_LOAD( "gauntlt1.1l",  0x10000, 0x08000, 0xd497d0a8 )
	ROM_LOAD( "gauntlt1.1mn", 0x18000, 0x08000, 0x29ef9882 )

	ROM_LOAD( "gauntlt1.2a",  0x20000, 0x08000, 0x9510b898 )
	ROM_LOAD( "gauntlt1.2b",  0x28000, 0x08000, 0x11e0ac5b )

	ROM_LOAD( "gauntlt1.2l",  0x30000, 0x08000, 0x29a5db41 )
	ROM_LOAD( "gauntlt1.2mn", 0x38000, 0x08000, 0x8bf3b263 )
ROM_END


ROM_START( gaunt2p )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gaunt2p.9a",   0x00000, 0x08000, 0x8784133f )
	ROM_LOAD16_BYTE( "gaunt2p.9b",   0x00001, 0x08000, 0x2843bde3 )
	ROM_LOAD16_BYTE( "gauntlt1.10a", 0x38000, 0x04000, 0x6d99ed51 )
	ROM_LOAD16_BYTE( "gauntlt1.10b", 0x38001, 0x04000, 0x545ead91 )
	ROM_LOAD16_BYTE( "gaunt2p.7a",   0x40000, 0x08000, 0x5b4ee415 )
	ROM_LOAD16_BYTE( "gaunt2p.7b",   0x40001, 0x08000, 0x41f5c9e2 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt1.16r", 0x4000, 0x4000, 0x6ee7f3cc )
	ROM_LOAD( "gauntlt1.16s", 0x8000, 0x8000, 0xfa19861f )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.6p",  0x00000, 0x04000, 0x6c276a1d )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.1a",  0x00000, 0x08000, 0x91700f33 )
	ROM_LOAD( "gauntlt1.1b",  0x08000, 0x08000, 0x869330be )

	ROM_LOAD( "gauntlt1.1l",  0x10000, 0x08000, 0xd497d0a8 )
	ROM_LOAD( "gauntlt1.1mn", 0x18000, 0x08000, 0x29ef9882 )

	ROM_LOAD( "gauntlt1.2a",  0x20000, 0x08000, 0x9510b898 )
	ROM_LOAD( "gauntlt1.2b",  0x28000, 0x08000, 0x11e0ac5b )

	ROM_LOAD( "gauntlt1.2l",  0x30000, 0x08000, 0x29a5db41 )
	ROM_LOAD( "gauntlt1.2mn", 0x38000, 0x08000, 0x8bf3b263 )
ROM_END


ROM_START( gaunt2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gauntlt2.9a",  0x00000, 0x08000, 0x46fe8743 )
	ROM_LOAD16_BYTE( "gauntlt2.9b",  0x00001, 0x08000, 0x276e15c4 )
	ROM_LOAD16_BYTE( "gauntlt2.10a", 0x38000, 0x04000, 0x45dfda47 )
	ROM_LOAD16_BYTE( "gauntlt2.10b", 0x38001, 0x04000, 0x343c029c )
	ROM_LOAD16_BYTE( "gauntlt2.7a",  0x40000, 0x08000, 0x58a0a9a3 )
	ROM_LOAD16_BYTE( "gauntlt2.7b",  0x40001, 0x08000, 0x658f0da8 )
	ROM_LOAD16_BYTE( "gauntlt2.6a",  0x50000, 0x08000, 0xae301bba )
	ROM_LOAD16_BYTE( "gauntlt2.6b",  0x50001, 0x08000, 0xe94aaa8a )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt2.16r", 0x4000, 0x4000, 0x5c731006 )
	ROM_LOAD( "gauntlt2.16s", 0x8000, 0x8000, 0xdc3591e7 )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt2.6p",  0x00000, 0x04000, 0xd101905d )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt2.1a",  0x00000, 0x08000, 0x09df6e23 )
	ROM_LOAD( "gauntlt2.1b",  0x08000, 0x08000, 0x869330be )
	ROM_LOAD( "gauntlt2.1c",  0x10000, 0x04000, 0xe4c98f01 )
	ROM_RELOAD(               0x14000, 0x04000 )

	ROM_LOAD( "gauntlt2.1l",  0x18000, 0x08000, 0x33cb476e )
	ROM_LOAD( "gauntlt2.1mn", 0x20000, 0x08000, 0x29ef9882 )
	ROM_LOAD( "gauntlt2.1p",  0x28000, 0x04000, 0xc4857879 )
	ROM_RELOAD(               0x2c000, 0x04000 )

	ROM_LOAD( "gauntlt2.2a",  0x30000, 0x08000, 0xf71e2503 )
	ROM_LOAD( "gauntlt2.2b",  0x38000, 0x08000, 0x11e0ac5b )
	ROM_LOAD( "gauntlt2.2c",  0x40000, 0x04000, 0xd9c2c2d1 )
	ROM_RELOAD(               0x44000, 0x04000 )

	ROM_LOAD( "gauntlt2.2l",  0x48000, 0x08000, 0x9e30b2e9 )
	ROM_LOAD( "gauntlt2.2mn", 0x50000, 0x08000, 0x8bf3b263 )
	ROM_LOAD( "gauntlt2.2p",  0x58000, 0x04000, 0xa32c732a )
	ROM_RELOAD(               0x5c000, 0x04000 )
ROM_END


ROM_START( vindctr2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1186", 0x00000, 0x08000, 0xaf138263 )
	ROM_LOAD16_BYTE( "1187", 0x00001, 0x08000, 0x44baff64 )
	ROM_LOAD16_BYTE( "1196", 0x38000, 0x04000, 0xc92bf6dd )
	ROM_LOAD16_BYTE( "1197", 0x38001, 0x04000, 0xd7ace347 )
	ROM_LOAD16_BYTE( "3188", 0x40000, 0x08000, 0x10f558d2 )
	ROM_LOAD16_BYTE( "3189", 0x40001, 0x08000, 0x302e24b6 )
	ROM_LOAD16_BYTE( "2190", 0x50000, 0x08000, 0xe7dc2b74 )
	ROM_LOAD16_BYTE( "2191", 0x50001, 0x08000, 0xed8ed86e )
	ROM_LOAD16_BYTE( "2192", 0x60000, 0x08000, 0xeec2c93d )
	ROM_LOAD16_BYTE( "2193", 0x60001, 0x08000, 0x3fbee9aa )
	ROM_LOAD16_BYTE( "1194", 0x70000, 0x08000, 0xe6bcf458 )
	ROM_LOAD16_BYTE( "1195", 0x70001, 0x08000, 0xb9bf245d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1160", 0x4000, 0x4000, 0xeef0a003 )
	ROM_LOAD( "1161", 0x8000, 0x8000, 0x68c74337 )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1198",  0x00000, 0x04000, 0xf99b631a )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1101", 0x00000, 0x08000, 0xdd3833ad )
	ROM_LOAD( "1166", 0x08000, 0x08000, 0xe2db50a0 )
	ROM_LOAD( "1170", 0x10000, 0x08000, 0xf050ab43 )
	ROM_LOAD( "1174", 0x18000, 0x08000, 0xb6704bd1 )
	ROM_LOAD( "1178", 0x20000, 0x08000, 0xd3006f05 )
	ROM_LOAD( "1182", 0x28000, 0x08000, 0x9046e985 )

	ROM_LOAD( "1102", 0x30000, 0x08000, 0xd505b04a )
	ROM_LOAD( "1167", 0x38000, 0x08000, 0x1869c76d )
	ROM_LOAD( "1171", 0x40000, 0x08000, 0x1b229c2b )
	ROM_LOAD( "1175", 0x48000, 0x08000, 0x73c41aca )
	ROM_LOAD( "1179", 0x50000, 0x08000, 0x9b7cb0ef )
	ROM_LOAD( "1183", 0x58000, 0x08000, 0x393bba42 )

	ROM_LOAD( "1103", 0x60000, 0x08000, 0x50e76162 )
	ROM_LOAD( "1168", 0x68000, 0x08000, 0x35c78469 )
	ROM_LOAD( "1172", 0x70000, 0x08000, 0x314ac268 )
	ROM_LOAD( "1176", 0x78000, 0x08000, 0x061d79db )
	ROM_LOAD( "1180", 0x80000, 0x08000, 0x89c1fe16 )
	ROM_LOAD( "1184", 0x88000, 0x08000, 0x541209d3 )

	ROM_LOAD( "1104", 0x90000, 0x08000, 0x9484ba65 )
	ROM_LOAD( "1169", 0x98000, 0x08000, 0x132d3337 )
	ROM_LOAD( "1173", 0xa0000, 0x08000, 0x98de2426 )
	ROM_LOAD( "1177", 0xa8000, 0x08000, 0x9d0824f8 )
	ROM_LOAD( "1181", 0xb0000, 0x08000, 0x9e62b27c )
	ROM_LOAD( "1185", 0xb8000, 0x08000, 0x9d62f6b7 )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void init_gauntlet(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 104);
	atarigen_init_6502_speedup(1, 0x410f, 0x4127);
	atarigen_invert_region(REGION_GFX2);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);

	/* indicate that we're not vindicators 2 */
	vindctr2_screen_refresh = 0;

	/* speed up the 68010 */
	speed_check = install_mem_write16_handler(0, 0x904002, 0x904003, speedup_68010_w);
	install_mem_read16_handler(0, 0x904002, 0x904003, speedup_68010_r);
}


static void init_gaunt2p(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 107);
	atarigen_init_6502_speedup(1, 0x410f, 0x4127);
	atarigen_invert_region(REGION_GFX2);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);

	/* indicate that we're not vindicators 2 */
	vindctr2_screen_refresh = 0;

	/* speed up the 68010 */
	speed_check = install_mem_write16_handler(0, 0x904002, 0x904003, speedup_68010_w);
	install_mem_read16_handler(0, 0x904002, 0x904003, speedup_68010_r);
}


static void init_gauntlet2(void)
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 106);
	atarigen_init_6502_speedup(1, 0x410f, 0x4127);
	atarigen_invert_region(REGION_GFX2);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x050000, memory_region(REGION_CPU1) + 0x058000, 0x8000);

	/* indicate that we're not vindicators 2 */
	vindctr2_screen_refresh = 0;

	/* speed up the 68010 */
	speed_check = install_mem_write16_handler(0, 0x904002, 0x904003, speedup_68010_w);
	install_mem_read16_handler(0, 0x904002, 0x904003, speedup_68010_r);
}


static void init_vindctr2(void)
{
	UINT8 *gfx2_base = memory_region(REGION_GFX2);
	UINT8 *data;
	int i;

	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 118);
	atarigen_init_6502_speedup(1, 0x40ff, 0x4117);
	atarigen_invert_region(REGION_GFX2);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x050000, memory_region(REGION_CPU1) + 0x058000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x060000, memory_region(REGION_CPU1) + 0x068000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x070000, memory_region(REGION_CPU1) + 0x078000, 0x8000);

	/* highly strange -- the address bits on the chip at 2J (and only that
	   chip) are scrambled -- this is verified on the schematics! */
	data = malloc(0x8000);
	if (data)
	{
		memcpy(data, &gfx2_base[0x88000], 0x8000);
		for (i = 0; i < 0x8000; i++)
		{
			int srcoffs = (i & 0x4000) | ((i << 11) & 0x3800) | ((i >> 3) & 0x07ff);
			gfx2_base[0x88000 + i] = data[srcoffs];
		}
		free(data);
	}

	/* indicate that we are vindicators 2 */
	vindctr2_screen_refresh = 1;
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1985, gauntlet, 0,        gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet" )
GAME( 1985, gauntir1, gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (Intermediate Release 1)" )
GAME( 1985, gauntir2, gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (Intermediate Release 2)" )
GAME( 1985, gaunt2p,  gauntlet, gauntlet, gauntlet, gaunt2p,   ROT0, "Atari Games", "Gauntlet (2 Players)" )
GAME( 1986, gaunt2,   0,        gauntlet, gauntlet, gauntlet2, ROT0, "Atari Games", "Gauntlet II" )
GAME( 1988, vindctr2, 0,        gauntlet, vindctr2, vindctr2,  ROT0, "Atari Games", "Vindicators Part II" )
