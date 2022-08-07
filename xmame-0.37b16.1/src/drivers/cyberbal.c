/***************************************************************************

	Atari Cyberball hardware

	driver by Aaron Giles

	Games supported:
		* Cyberball (1988) [2 sets]
		* Tournament Cyberball 2072 (1989)
		* Cyberball 2072, 2-players (1989)

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
 *	Compiler options
 *
 *************************************/

/* better to leave this on; otherwise, you end up playing entire games out of the left speaker */
#define USE_MONO_SOUND			1



/*************************************
 *
 *	Externals
 *
 *************************************/

/* video */
void cyberbal_set_screen(int which);

READ16_HANDLER( cyberbal_paletteram_0_r );
READ16_HANDLER( cyberbal_paletteram_1_r );
WRITE16_HANDLER( cyberbal_paletteram_0_w );
WRITE16_HANDLER( cyberbal_paletteram_1_w );

int cyberbal_vh_start(void);
int cyberb2p_vh_start(void);
void cyberbal_vh_stop(void);
void cyberbal_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void cyberbal_scanline_update(int param);

extern data16_t *cyberbal_paletteram_0;
extern data16_t *cyberbal_paletteram_1;


/* audio */
void cyberbal_sound_reset(void);
int cyberbal_samples_start(const struct MachineSound *msound);
void cyberbal_samples_stop(void);

int cyberbal_sound_68k_irq_gen(void);

READ_HANDLER( cyberbal_special_port3_r );
READ_HANDLER( cyberbal_sound_6502_stat_r );
READ_HANDLER( cyberbal_sound_68k_6502_r );
WRITE_HANDLER( cyberbal_sound_bank_select_w );
WRITE_HANDLER( cyberbal_sound_68k_6502_w );

READ16_HANDLER( cyberbal_sound_68k_r );
WRITE16_HANDLER( cyberbal_io_68k_irq_ack_w );
WRITE16_HANDLER( cyberbal_sound_68k_w );
WRITE16_HANDLER( cyberbal_sound_68k_dac_w );



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate1 = 0;
	int newstate2 = 0;
	int temp;

	if (atarigen_sound_int_state)
		newstate1 |= 1;
	if (atarigen_video_int_state)
		newstate2 |= 1;

	if (newstate1)
		cpu_set_irq_line(0, newstate1, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);

	if (newstate2)
		cpu_set_irq_line(2, newstate2, ASSERT_LINE);
	else
		cpu_set_irq_line(2, 7, CLEAR_LINE);

	/* check for screen swapping */
	temp = readinputport(2);
	if (temp & 1) cyberbal_set_screen(0);
	else if (temp & 2) cyberbal_set_screen(1);
}


static void init_machine(void)
{
	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(cyberbal_scanline_update, 8);
	atarigen_sound_io_reset(1);

	cyberbal_sound_reset();

	/* CPU 2 doesn't run until reset */
	cpu_set_reset_line(2, ASSERT_LINE);

	/* make sure we're pointing to the right screen by default */
	cyberbal_set_screen(0);
}


static void cyberb2p_update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate |= 1;
	if (atarigen_sound_int_state)
		newstate |= 3;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void cyberb2p_init_machine(void)
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(cyberb2p_update_interrupts);
	atarigen_scanline_timer_reset(cyberbal_scanline_update, 8);
	atarijsa_reset();

	/* make sure we're pointing to the only screen */
	cyberbal_set_screen(0);
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

static READ16_HANDLER( special_port0_r )
{
	int temp = readinputport(0);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0080;
	return temp;
}


static READ16_HANDLER( special_port2_r )
{
	int temp = readinputport(2);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x2000;
	return temp;
}


static READ16_HANDLER( sound_state_r )
{
	int temp = 0xffff;
	if (atarigen_cpu_to_sound_ready) temp ^= 0xffff;
	return temp;
}



/*************************************
 *
 *	Extra I/O handlers.
 *
 *************************************/

static WRITE16_HANDLER( p2_reset_w )
{
	cpu_set_reset_line(2, CLEAR_LINE);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xfc0000, 0xfc03ff, atarigen_eeprom_r },
	{ 0xfc8000, 0xfcffff, atarigen_sound_upper_r },
	{ 0xfe0000, 0xfe0fff, special_port0_r },
	{ 0xfe1000, 0xfe1fff, input_port_1_word_r },
	{ 0xfe8000, 0xfe8fff, cyberbal_paletteram_1_r },
	{ 0xfec000, 0xfecfff, cyberbal_paletteram_0_r },
	{ 0xff0000, 0xff37ff, MRA16_BANK1 },	/* shared */
	{ 0xff3800, 0xff3fff, MRA16_BANK2 },	/* shared */
	{ 0xff4000, 0xff77ff, MRA16_BANK3 },	/* shared */
	{ 0xff7800, 0xff9fff, MRA16_BANK4 },	/* shared */
	{ 0xffa000, 0xffbfff, MRA16_BANK5 },	/* shared */
	{ 0xffc000, 0xffffff, MRA16_BANK6 },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xfc0000, 0xfc03ff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfd0000, 0xfd1fff, atarigen_eeprom_enable_w },
	{ 0xfd2000, 0xfd3fff, atarigen_sound_reset_w },
	{ 0xfd4000, 0xfd5fff, watchdog_reset16_w },
	{ 0xfd6000, 0xfd7fff, p2_reset_w },
	{ 0xfd8000, 0xfd9fff, atarigen_sound_upper_w },
	{ 0xfe8000, 0xfe8fff, cyberbal_paletteram_1_w, &cyberbal_paletteram_1 },
	{ 0xfec000, 0xfecfff, cyberbal_paletteram_0_w, &cyberbal_paletteram_0 },
	{ 0xff0000, 0xff1fff, ataripf_1_simple_w, &ataripf_1_base },
	{ 0xff2000, 0xff2fff, atarian_1_vram_w, &atarian_1_base },
	{ 0xff3000, 0xff37ff, atarimo_1_spriteram_w, &atarimo_1_spriteram },
	{ 0xff3800, 0xff3fff, MWA16_BANK2 },
	{ 0xff4000, 0xff5fff, ataripf_0_simple_w, &ataripf_0_base },
	{ 0xff6000, 0xff6fff, atarian_0_vram_w, &atarian_0_base },
	{ 0xff7000, 0xff77ff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xff7800, 0xff9fff, MWA16_BANK4 },
	{ 0xffa000, 0xffbfff, MWA16_NOP },
	{ 0xffc000, 0xffffff, MWA16_BANK6 },
MEMORY_END



/*************************************
 *
 *	Extra CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( extra_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xfe0000, 0xfe0fff, special_port0_r },
	{ 0xfe1000, 0xfe1fff, input_port_1_word_r },
	{ 0xfe8000, 0xfe8fff, cyberbal_paletteram_1_r },
	{ 0xfec000, 0xfecfff, cyberbal_paletteram_0_r },
	{ 0xff0000, 0xff37ff, MRA16_BANK1 },
	{ 0xff3800, 0xff3fff, MRA16_BANK2 },
	{ 0xff4000, 0xff77ff, MRA16_BANK3 },
	{ 0xff7800, 0xff9fff, MRA16_BANK4 },
	{ 0xffa000, 0xffbfff, MRA16_BANK5 },
	{ 0xffc000, 0xffffff, MRA16_BANK6 },
MEMORY_END


static MEMORY_WRITE16_START( extra_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xfc0000, 0xfdffff, atarigen_video_int_ack_w },
	{ 0xfe8000, 0xfe8fff, cyberbal_paletteram_1_w },
	{ 0xfec000, 0xfecfff, cyberbal_paletteram_0_w },
	{ 0xff0000, 0xff1fff, ataripf_1_simple_w },
	{ 0xff2000, 0xff2fff, atarian_1_vram_w },
	{ 0xff3000, 0xff37ff, atarimo_1_spriteram_w },
	{ 0xff3800, 0xff3fff, MWA16_BANK2 },
	{ 0xff4000, 0xff5fff, ataripf_0_simple_w },
	{ 0xff6000, 0xff6fff, atarian_0_vram_w },
	{ 0xff7000, 0xff77ff, atarimo_0_spriteram_w },
	{ 0xff7800, 0xff9fff, MWA16_BANK4 },
	{ 0xffa000, 0xffbfff, MWA16_BANK5 },
	{ 0xffc000, 0xffffff, MWA16_NOP },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2802, 0x2803, atarigen_6502_irq_ack_r },
	{ 0x2c00, 0x2c01, atarigen_6502_sound_r },
	{ 0x2c02, 0x2c03, cyberbal_special_port3_r },
	{ 0x2c04, 0x2c05, cyberbal_sound_68k_6502_r },
	{ 0x2c06, 0x2c07, cyberbal_sound_6502_stat_r },
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2801, cyberbal_sound_68k_6502_w },
	{ 0x2802, 0x2803, atarigen_6502_irq_ack_w },
	{ 0x2804, 0x2805, atarigen_6502_sound_w },
	{ 0x2806, 0x2807, cyberbal_sound_bank_select_w },
	{ 0x3000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	68000 Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( sound_68k_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xff8000, 0xff87ff, cyberbal_sound_68k_r },
	{ 0xfff000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( sound_68k_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xff8800, 0xff8fff, cyberbal_sound_68k_w },
	{ 0xff9000, 0xff97ff, cyberbal_io_68k_irq_ack_w },
	{ 0xff9800, 0xff9fff, cyberbal_sound_68k_dac_w },
	{ 0xfff000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	2-player main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( cyberb2p_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0xfc0000, 0xfc0003, input_port_0_word_r },
	{ 0xfc2000, 0xfc2003, input_port_1_word_r },
	{ 0xfc4000, 0xfc4003, special_port2_r },
	{ 0xfc6000, 0xfc6003, atarigen_sound_upper_r },
	{ 0xfc8000, 0xfc8fff, atarigen_eeprom_r },
	{ 0xfca000, 0xfcafff, MRA16_RAM },
	{ 0xfe0000, 0xfe0003, sound_state_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( cyberb2p_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0xfc8000, 0xfc8fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfca000, 0xfcafff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0xfd0000, 0xfd0003, atarigen_eeprom_enable_w },
	{ 0xfd2000, 0xfd2003, atarigen_sound_reset_w },
	{ 0xfd4000, 0xfd4003, watchdog_reset16_w },
	{ 0xfd6000, 0xfd6003, atarigen_video_int_ack_w },
	{ 0xfd8000, 0xfd8003, atarigen_sound_upper_w },
	{ 0xff0000, 0xff1fff, ataripf_0_simple_w, &ataripf_0_base },
	{ 0xff2000, 0xff2fff, atarian_0_vram_w, &atarian_0_base },
	{ 0xff3000, 0xff37ff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xff3800, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( cyberbal )
	PORT_START      /* fe0000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START      /* fe1000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for screen switching */
	PORT_BITX(  0x0001, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select Left Screen", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX(  0x0002, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select Right Screen", KEYCODE_0, IP_JOY_NONE )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* audio board port */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* output buffer full */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* input buffer full */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* self test */
INPUT_PORTS_END


INPUT_PORTS_START( cyberb2p )
	PORT_START      /* fc0000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* fc2000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* fc4000 */
	PORT_BIT( 0x1fff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	JSA_II_PORT		/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pfanlayout =
{
	16,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0,0, 4,4, 8,8, 12,12, 16,16, 20,20, 24,24, 28,28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};

static struct GfxLayout pfanlayout_interleaved =
{
	16,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4,RGN_FRAC(1,2)+4, 0,0, 4,4, RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+12,RGN_FRAC(1,2)+12, 8,8, 12,12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout molayout =
{
	16,8,
	RGN_FRAC(1,4),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+4, RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+4, 0, 4,
	  RGN_FRAC(3,4)+8, RGN_FRAC(3,4)+12, RGN_FRAC(2,4)+8, RGN_FRAC(2,4)+12, RGN_FRAC(1,4)+8, RGN_FRAC(1,4)+12, 8, 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &pfanlayout,     0, 128 },
	{ REGION_GFX1, 0, &molayout,   0x600, 16 },
	{ REGION_GFX3, 0, &pfanlayout, 0x780, 8 },
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_interleaved[] =
{
	{ REGION_GFX2, 0, &pfanlayout_interleaved,     0, 128 },
	{ REGION_GFX1, 0, &molayout,               0x600, 16 },
	{ REGION_GFX3, 0, &pfanlayout_interleaved, 0x780, 8 },
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
#if USE_MONO_SOUND
	{ YM3012_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
#else
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
#endif
	{ atarigen_ym2151_irq_gen }
};


static struct DACinterface dac_interface =
{
	2,
#if USE_MONO_SOUND
	{ MIXER(50,MIXER_PAN_CENTER), MIXER(50,MIXER_PAN_CENTER) }
#else
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) }
#endif
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static struct MachineDriver machine_driver_cyberbal =
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
			CPU_M6502,
			ATARI_CLOCK_14MHz/8,
			sound_readmem,sound_writemem,0,0,
			0,0,
			atarigen_6502_irq_gen,(UINT32)(1000000000.0/((double)ATARI_CLOCK_14MHz/4/4/16/16/14))
		},
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz/2,
			extra_readmem,extra_writemem,0,0,
			atarigen_video_int_gen,1
		},
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz/2,
			sound_68k_readmem,sound_68k_writemem,0,0,
			0,0,
			cyberbal_sound_68k_irq_gen,10000
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	10,
	init_machine,

	/* video hardware */
	42*16, 30*8, { 0*16, 42*16-1, 0*8, 30*8-1 },
	gfxdecodeinfo_interleaved,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK |
			VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	cyberbal_vh_start,
	cyberbal_vh_stop,
	cyberbal_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	},

	atarigen_nvram_handler
};


static struct MachineDriver machine_driver_cyberb2p =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,		/* verified */
			ATARI_CLOCK_14MHz/2,
			cyberb2p_readmem,cyberb2p_writemem,0,0,
			atarigen_video_int_gen,1
		},
		JSA_II_CPU
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	cyberb2p_init_machine,

	/* video hardware */
	42*16, 30*8, { 0*16, 42*16-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK |
			VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	cyberb2p_vh_start,
	cyberbal_vh_stop,
	cyberbal_vh_screenrefresh,

	/* sound hardware */
	JSA_II_MONO(REGION_SOUND1),

	atarigen_nvram_handler
};



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( cyberbal )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "4123.1m", 0x00000, 0x10000, 0xfb872740 )
	ROM_LOAD16_BYTE( "4124.1k", 0x00001, 0x10000, 0x87babad9 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "2131-snd.2f",  0x10000, 0x4000, 0xbd7e3d84 )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "2127.3c", 0x00000, 0x10000, 0x3e5feb1f )
	ROM_LOAD16_BYTE( "2128.1b", 0x00001, 0x10000, 0x4e642cc3 )
	ROM_LOAD16_BYTE( "2129.1c", 0x20000, 0x10000, 0xdb11d2f0 )
	ROM_LOAD16_BYTE( "2130.3b", 0x20001, 0x10000, 0xfd86b8aa )

	ROM_REGION16_BE( 0x40000, REGION_CPU4, 0 )	/* 256k for 68000 sound code */
	ROM_LOAD16_BYTE( "1132-snd.5c",  0x00000, 0x10000, 0xca5ce8d8 )
	ROM_LOAD16_BYTE( "1133-snd.7c",  0x00001, 0x10000, 0xffeb8746 )
	ROM_LOAD16_BYTE( "1134-snd.5a",  0x20000, 0x10000, 0xbcbd4c00 )
	ROM_LOAD16_BYTE( "1135-snd.7a",  0x20001, 0x10000, 0xd520f560 )

	ROM_REGION( 0x140000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1150.15a",  0x000000, 0x10000, 0xe770eb3e ) /* MO */
	ROM_LOAD( "1154.16a",  0x010000, 0x10000, 0x40db00da ) /* MO */
	ROM_LOAD( "2158.17a",  0x020000, 0x10000, 0x52bb08fb ) /* MO */
	ROM_LOAD( "1162.19a",  0x030000, 0x10000, 0x0a11d877 ) /* MO */

	ROM_LOAD( "1151.11a",  0x050000, 0x10000, 0x6f53c7c1 ) /* MO */
	ROM_LOAD( "1155.12a",  0x060000, 0x10000, 0x5de609e5 ) /* MO */
	ROM_LOAD( "2159.13a",  0x070000, 0x10000, 0xe6f95010 ) /* MO */
	ROM_LOAD( "1163.14a",  0x080000, 0x10000, 0x47f56ced ) /* MO */

	ROM_LOAD( "1152.15c",  0x0a0000, 0x10000, 0xc8f1f7ff ) /* MO */
	ROM_LOAD( "1156.16c",  0x0b0000, 0x10000, 0x6bf0bf98 ) /* MO */
	ROM_LOAD( "2160.17c",  0x0c0000, 0x10000, 0xc3168603 ) /* MO */
	ROM_LOAD( "1164.19c",  0x0d0000, 0x10000, 0x7ff29d09 ) /* MO */

	ROM_LOAD( "1153.11c",  0x0f0000, 0x10000, 0x99629412 ) /* MO */
	ROM_LOAD( "1157.12c",  0x100000, 0x10000, 0xaa198cb7 ) /* MO */
	ROM_LOAD( "2161.13c",  0x110000, 0x10000, 0x6cf79a67 ) /* MO */
	ROM_LOAD( "1165.14c",  0x120000, 0x10000, 0x40bdf767 ) /* MO */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1146.9l",   0x000000, 0x10000, 0xa64b4da8 ) /* playfield */
	ROM_LOAD( "1147.8l",   0x010000, 0x10000, 0xca91ec1b ) /* playfield */
	ROM_LOAD( "1148.11l",  0x020000, 0x10000, 0xee29d1d1 ) /* playfield */
	ROM_LOAD( "1149.10l",  0x030000, 0x10000, 0x882649f8 ) /* playfield */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1166.14n",  0x000000, 0x10000, 0x0ca1e3b3 ) /* alphanumerics */
	ROM_LOAD( "1167.16n",  0x010000, 0x10000, 0x882f4e1c ) /* alphanumerics */
ROM_END


ROM_START( cyberba2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "2123.1m", 0x00000, 0x10000, 0x502676e8 )
	ROM_LOAD16_BYTE( "2124.1k", 0x00001, 0x10000, 0x30f55915 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "2131-snd.2f",  0x10000, 0x4000, 0xbd7e3d84 )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "2127.3c", 0x00000, 0x10000, 0x3e5feb1f )
	ROM_LOAD16_BYTE( "2128.1b", 0x00001, 0x10000, 0x4e642cc3 )
	ROM_LOAD16_BYTE( "2129.1c", 0x20000, 0x10000, 0xdb11d2f0 )
	ROM_LOAD16_BYTE( "2130.3b", 0x20001, 0x10000, 0xfd86b8aa )

	ROM_REGION16_BE( 0x40000, REGION_CPU4, 0 )	/* 256k for 68000 sound code */
	ROM_LOAD16_BYTE( "1132-snd.5c",  0x00000, 0x10000, 0xca5ce8d8 )
	ROM_LOAD16_BYTE( "1133-snd.7c",  0x00001, 0x10000, 0xffeb8746 )
	ROM_LOAD16_BYTE( "1134-snd.5a",  0x20000, 0x10000, 0xbcbd4c00 )
	ROM_LOAD16_BYTE( "1135-snd.7a",  0x20001, 0x10000, 0xd520f560 )

	ROM_REGION( 0x140000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1150.15a",  0x000000, 0x10000, 0xe770eb3e ) /* MO */
	ROM_LOAD( "1154.16a",  0x010000, 0x10000, 0x40db00da ) /* MO */
	ROM_LOAD( "2158.17a",  0x020000, 0x10000, 0x52bb08fb ) /* MO */
	ROM_LOAD( "1162.19a",  0x030000, 0x10000, 0x0a11d877 ) /* MO */

	ROM_LOAD( "1151.11a",  0x050000, 0x10000, 0x6f53c7c1 ) /* MO */
	ROM_LOAD( "1155.12a",  0x060000, 0x10000, 0x5de609e5 ) /* MO */
	ROM_LOAD( "2159.13a",  0x070000, 0x10000, 0xe6f95010 ) /* MO */
	ROM_LOAD( "1163.14a",  0x080000, 0x10000, 0x47f56ced ) /* MO */

	ROM_LOAD( "1152.15c",  0x0a0000, 0x10000, 0xc8f1f7ff ) /* MO */
	ROM_LOAD( "1156.16c",  0x0b0000, 0x10000, 0x6bf0bf98 ) /* MO */
	ROM_LOAD( "2160.17c",  0x0c0000, 0x10000, 0xc3168603 ) /* MO */
	ROM_LOAD( "1164.19c",  0x0d0000, 0x10000, 0x7ff29d09 ) /* MO */

	ROM_LOAD( "1153.11c",  0x0f0000, 0x10000, 0x99629412 ) /* MO */
	ROM_LOAD( "1157.12c",  0x100000, 0x10000, 0xaa198cb7 ) /* MO */
	ROM_LOAD( "2161.13c",  0x110000, 0x10000, 0x6cf79a67 ) /* MO */
	ROM_LOAD( "1165.14c",  0x120000, 0x10000, 0x40bdf767 ) /* MO */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1146.9l",   0x000000, 0x10000, 0xa64b4da8 ) /* playfield */
	ROM_LOAD( "1147.8l",   0x010000, 0x10000, 0xca91ec1b ) /* playfield */
	ROM_LOAD( "1148.11l",  0x020000, 0x10000, 0xee29d1d1 ) /* playfield */
	ROM_LOAD( "1149.10l",  0x030000, 0x10000, 0x882649f8 ) /* playfield */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1166.14n",  0x000000, 0x10000, 0x0ca1e3b3 ) /* alphanumerics */
	ROM_LOAD( "1167.16n",  0x010000, 0x10000, 0x882f4e1c ) /* alphanumerics */
ROM_END


ROM_START( cyberbt )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "cyb1007.bin", 0x00000, 0x10000, 0xd434b2d7 )
	ROM_LOAD16_BYTE( "cyb1008.bin", 0x00001, 0x10000, 0x7d6c4163 )
	ROM_LOAD16_BYTE( "cyb1009.bin", 0x20000, 0x10000, 0x3933e089 )
	ROM_LOAD16_BYTE( "cyb1010.bin", 0x20001, 0x10000, 0xe7a7cae8 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "cyb1029.bin",  0x10000, 0x4000, 0xafee87e1 )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "cyb1011.bin", 0x00000, 0x10000, 0x22d3e09c )
	ROM_LOAD16_BYTE( "cyb1012.bin", 0x00001, 0x10000, 0xa8eeed8c )
	ROM_LOAD16_BYTE( "cyb1013.bin", 0x20000, 0x10000, 0x11d287c9 )
	ROM_LOAD16_BYTE( "cyb1014.bin", 0x20001, 0x10000, 0xbe15db42 )

	ROM_REGION16_BE( 0x40000, REGION_CPU4, 0 )	/* 256k for 68000 sound code */
	ROM_LOAD16_BYTE( "1132-snd.5c",  0x00000, 0x10000, 0xca5ce8d8 )
	ROM_LOAD16_BYTE( "1133-snd.7c",  0x00001, 0x10000, 0xffeb8746 )
	ROM_LOAD16_BYTE( "1134-snd.5a",  0x20000, 0x10000, 0xbcbd4c00 )
	ROM_LOAD16_BYTE( "1135-snd.7a",  0x20001, 0x10000, 0xd520f560 )

	ROM_REGION( 0x140000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1001.bin",  0x000000, 0x20000, 0x586ba107 ) /* MO */
	ROM_LOAD( "1005.bin",  0x020000, 0x20000, 0xa53e6248 ) /* MO */
	ROM_LOAD( "1032.bin",  0x040000, 0x10000, 0x131f52a0 ) /* MO */

	ROM_LOAD( "1002.bin",  0x050000, 0x20000, 0x0f71f86c ) /* MO */
	ROM_LOAD( "1006.bin",  0x070000, 0x20000, 0xdf0ab373 ) /* MO */
	ROM_LOAD( "1033.bin",  0x090000, 0x10000, 0xb6270943 ) /* MO */

	ROM_LOAD( "1003.bin",  0x0a0000, 0x20000, 0x1cf373a2 ) /* MO */
	ROM_LOAD( "1007.bin",  0x0c0000, 0x20000, 0xf2ffab24 ) /* MO */
	ROM_LOAD( "1034.bin",  0x0e0000, 0x10000, 0x6514f0bd ) /* MO */

	ROM_LOAD( "1004.bin",  0x0f0000, 0x20000, 0x537f6de3 ) /* MO */
	ROM_LOAD( "1008.bin",  0x110000, 0x20000, 0x78525bbb ) /* MO */
	ROM_LOAD( "1035.bin",  0x130000, 0x10000, 0x1be3e5c8 ) /* MO */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cyb1015.bin",  0x000000, 0x10000, 0xdbbad153 ) /* playfield */
	ROM_LOAD( "cyb1016.bin",  0x010000, 0x10000, 0x76e0d008 ) /* playfield */
	ROM_LOAD( "cyb1017.bin",  0x020000, 0x10000, 0xddca9ca2 ) /* playfield */
	ROM_LOAD( "cyb1018.bin",  0x030000, 0x10000, 0xaa495b6f ) /* playfield */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cyb1019.bin",  0x000000, 0x10000, 0x833b4768 ) /* alphanumerics */
	ROM_LOAD( "cyb1020.bin",  0x010000, 0x10000, 0x4976cffd ) /* alphanumerics */
ROM_END


ROM_START( cyberb2p )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "3019.bin", 0x00000, 0x10000, 0x029f8cb6 )
	ROM_LOAD16_BYTE( "3020.bin", 0x00001, 0x10000, 0x1871b344 )
	ROM_LOAD16_BYTE( "3021.bin", 0x20000, 0x10000, 0xfd7ebead )
	ROM_LOAD16_BYTE( "3022.bin", 0x20001, 0x10000, 0x173ccad4 )
	ROM_LOAD16_BYTE( "2023.bin", 0x40000, 0x10000, 0xe541b08f )
	ROM_LOAD16_BYTE( "2024.bin", 0x40001, 0x10000, 0x5a77ee95 )
	ROM_LOAD16_BYTE( "1025.bin", 0x60000, 0x10000, 0x95ff68c6 )
	ROM_LOAD16_BYTE( "1026.bin", 0x60001, 0x10000, 0xf61c4898 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1042.bin",  0x10000, 0x4000, 0xe63cf125 )
	ROM_CONTINUE(          0x04000, 0xc000 )

	ROM_REGION( 0x140000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1001.bin",  0x000000, 0x20000, 0x586ba107 ) /* MO */
	ROM_LOAD( "1005.bin",  0x020000, 0x20000, 0xa53e6248 ) /* MO */
	ROM_LOAD( "1032.bin",  0x040000, 0x10000, 0x131f52a0 ) /* MO */

	ROM_LOAD( "1002.bin",  0x050000, 0x20000, 0x0f71f86c ) /* MO */
	ROM_LOAD( "1006.bin",  0x070000, 0x20000, 0xdf0ab373 ) /* MO */
	ROM_LOAD( "1033.bin",  0x090000, 0x10000, 0xb6270943 ) /* MO */

	ROM_LOAD( "1003.bin",  0x0a0000, 0x20000, 0x1cf373a2 ) /* MO */
	ROM_LOAD( "1007.bin",  0x0c0000, 0x20000, 0xf2ffab24 ) /* MO */
	ROM_LOAD( "1034.bin",  0x0e0000, 0x10000, 0x6514f0bd ) /* MO */

	ROM_LOAD( "1004.bin",  0x0f0000, 0x20000, 0x537f6de3 ) /* MO */
	ROM_LOAD( "1008.bin",  0x110000, 0x20000, 0x78525bbb ) /* MO */
	ROM_LOAD( "1035.bin",  0x130000, 0x10000, 0x1be3e5c8 ) /* MO */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1036.bin",  0x000000, 0x10000, 0xcdf6e3d6 ) /* playfield */
	ROM_LOAD( "1037.bin",  0x010000, 0x10000, 0xec2fef3e ) /* playfield */
	ROM_LOAD( "1038.bin",  0x020000, 0x10000, 0xe866848f ) /* playfield */
	ROM_LOAD( "1039.bin",  0x030000, 0x10000, 0x9b9a393c ) /* playfield */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1040.bin",  0x000000, 0x10000, 0xa4c116f9 ) /* alphanumerics */
	ROM_LOAD( "1041.bin",  0x010000, 0x10000, 0xe25d7847 ) /* alphanumerics */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "1049.bin",  0x00000, 0x10000, 0x94f24575 )
	ROM_LOAD( "1050.bin",  0x10000, 0x10000, 0x87208e1e )
	ROM_LOAD( "1051.bin",  0x20000, 0x10000, 0xf82558b9 )
	ROM_LOAD( "1052.bin",  0x30000, 0x10000, 0xd96437ad )
ROM_END



/*************************************
 *
 *	Machine initialization
 *
 *************************************/

static const data16_t default_eeprom[] =
{
	0x0001,0x01FF,0x0F00,0x011A,0x014A,0x0100,0x01A1,0x0200,
	0x010E,0x01AF,0x0300,0x01FF,0x0114,0x0144,0x01FF,0x0F00,
	0x011A,0x014A,0x0100,0x01A1,0x0200,0x010E,0x01AF,0x0300,
	0x01FF,0x0114,0x0144,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,
	0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,
	0x0E00,0x01A8,0x0131,0x010B,0x0100,0x014C,0x0A00,0x01FF,
	0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0xB5FF,0x0E00,0x01FF,
	0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,
	0x0E00,0x01FF,0x0E00,0x0000
};


static void init_cyberbal(void)
{
	atarigen_eeprom_default = default_eeprom;
	atarigen_slapstic_init(0, 0x018000, 0);
	atarigen_init_6502_speedup(1, 0x4191, 0x41A9);

	/* make sure the banks are pointing to the correct location */
	cpu_setbank(1, ataripf_1_base);
	cpu_setbank(3, ataripf_0_base);
}


static void init_cyberbt(void)
{
	atarigen_eeprom_default = default_eeprom;
	atarigen_slapstic_init(0, 0x018000, 116);
	atarigen_init_6502_speedup(1, 0x4191, 0x41A9);

	/* make sure the banks are pointing to the correct location */
	cpu_setbank(1, ataripf_1_base);
	cpu_setbank(3, ataripf_0_base);
}


static void init_cyberb2p(void)
{
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x8000);
	atarigen_init_6502_speedup(1, 0x4159, 0x4171);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1988, cyberbal, 0,        cyberbal, cyberbal, cyberbal, ROT0_16BIT, "Atari Games", "Cyberball (Version 4)" )
GAME( 1988, cyberba2, cyberbal, cyberbal, cyberbal, cyberbal, ROT0_16BIT, "Atari Games", "Cyberball (Version 2)" )
GAME( 1989, cyberbt,  cyberbal, cyberbal, cyberbal, cyberbt,  ROT0_16BIT, "Atari Games", "Tournament Cyberball 2072" )
GAME( 1989, cyberb2p, cyberbal, cyberb2p, cyberb2p, cyberb2p, ROT0_16BIT, "Atari Games", "Cyberball 2072 (2 player)" )
