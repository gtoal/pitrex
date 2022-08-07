/***************************************************************************

Super Cobra and Co. memory map (preliminary)

Main CPU:
--------

There seems to be 2 main board types:

Type 1      Type 2

0000-7fff   0000-7fff	ROM (not all games use the entire range)
8000-87ff   8000-87ff	RAM
8800-8bff   9000-93ff	video RAM
9000-903f   8800-883f	screen attributes
9040-905f   8840-885f	sprites
9060-907f   8860-887f	bullets


read:
b000      	9800		watchdog reset

9800-9803	a000-a00f	PPI8255-0
      					Port A - IN0
	      				Port B - IN1
						Port C - IN2

a000-a003	a800-a80f	PPI8255-1


write:

9800-9803	a000-a00f	PPI8255-0

a000-a003	a800-a80f	PPI8255-1
						Port A - To AY-3-8910 port A (commands for the audio CPU)
						Port B - bit 3 = trigger interrupt on audio CPU

a801      	b004		interrupt enable
a802      	b006		coin counter
a803      	b002		? (POUT1)
a804      	b000		stars on
a805      	b00a		? (POUT2)
a806      	b00e		screen vertical flip
a807      	b00c		screen horizontal flip


Sound CPU:

0000-1fff   ROM
8000-83ff   RAM
9000-9fff   R/C Filter (2 bits for each of the 6 channels)

I/O:

10  		AY8910 #0 control
20			AY8910 #0 data port
40			AY8910 #1 control port
80			AY8910 #1 data port


TODO:
----

- Need correct color PROMs for Super Bond

- Dark Planet background graphics

- Explosion sound in Scramble/Super Cobra repeats


Notes/Tidbits:
-------------

- Moonwar: 8255 Port C bit 4 was originally designed so when bit4=0, 1P spinner
  is selected, and when bit4=1, 2P spinner gets selected.  But they forgot to
  change the 8255 initialization value and Port C was set to input, setting the
  spinner select bit to HI regardless what was written to it. This bug has been
  corrected in the newer set, but, to maintain hardware compatibility with
  older PCB's, they had to reverse to active status of the select bit.  So in the
  newer set, Bit4=1 selects the 1P spinner and Bit4=0 selects the 2P spinner.

- Calipso was apperantly redesigned for two player simultanious play.
  There is code at $298a to flip the screen, but location $8669 has to be
  set to 2. It's set to 1 no matter how many players are playing.
  It's possible that there is a cocktail version of the game.

- Video Hustler and its two bootlegs all have identical code, the only
  differences are the title, copyright removed, different encryptions or
  no encryption, plus hustlerb has a different memory map.

- In Tazmania, when set to Upright mode, player 2 left skips the current
  level

***************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"


extern unsigned char *galaxian_videoram;
extern unsigned char *galaxian_spriteram;
extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern size_t galaxian_spriteram_size;
extern size_t galaxian_bulletsram_size;

void scramble_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void moonwar_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void darkplnt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void rescue_vh_convert_color_prom  (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void minefld_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void stratgyx_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

void init_scobra(void);
void init_stratgyx(void);
void init_moonwar(void);
void init_darkplnt(void);
void init_anteater(void);
void init_rescue(void);
void init_minefld(void);
void init_losttomb(void);
void init_superbon(void);
void init_hustler(void);
void init_billiard(void);

void scramble_init_machine(void);

int  scramble_vh_start(void);
int  theend_vh_start(void);
int  darkplnt_vh_start(void);
int  rescue_vh_start  (void);
int  minefld_vh_start (void);
int  calipso_vh_start (void);
int  stratgyx_vh_start(void);

void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE_HANDLER( galaxian_videoram_w );
READ_HANDLER ( galaxian_videoram_r );
WRITE_HANDLER( galaxian_stars_enable_w );
WRITE_HANDLER( scramble_background_blue_w );
WRITE_HANDLER( galaxian_flip_screen_x_w );
WRITE_HANDLER( galaxian_flip_screen_y_w );
WRITE_HANDLER( scramble_filter_w );
WRITE_HANDLER( frogger_filter_w );

extern struct GfxDecodeInfo galaxian_gfxdecodeinfo[];

extern struct AY8910interface frogger_ay8910_interface;
extern const struct Memory_ReadAddress frogger_sound_readmem[];
extern const struct Memory_WriteAddress frogger_sound_writemem[];
extern const struct IO_ReadPort frogger_sound_readport[];
extern const struct IO_WritePort frogger_sound_writeport[];

READ_HANDLER( scramble_portB_r );

READ_HANDLER(scobra_type2_ppi8255_0_r);
READ_HANDLER(scobra_type2_ppi8255_1_r);
WRITE_HANDLER(scobra_type2_ppi8255_0_w);
WRITE_HANDLER(scobra_type2_ppi8255_1_w);
READ_HANDLER(hustler_ppi8255_0_r);
READ_HANDLER(hustler_ppi8255_1_r);
WRITE_HANDLER(hustler_ppi8255_0_w);
WRITE_HANDLER(hustler_ppi8255_1_w);


static WRITE_HANDLER( type1_coin_counter_w )
{
	coin_counter_w(offset,data);
}

static WRITE_HANDLER( type2_coin_counter_w )
{
	/* bit 1 selects coin counter */
	coin_counter_w(offset >> 1, data);
}


static MEMORY_READ_START( type1_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8bff, MRA_RAM },
	{ 0x8c00, 0x8fff, galaxian_videoram_r },	/* mirror */
	{ 0x9000, 0x90ff, MRA_RAM },
	{ 0x9800, 0x9803, ppi8255_0_r },
	{ 0xa000, 0xa003, ppi8255_1_r },
	{ 0xb000, 0xb000, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( type1_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, MWA_RAM, &galaxian_videoram },
	{ 0x8c00, 0x8fff, galaxian_videoram_w },	/* mirror */
	{ 0x9000, 0x903f, MWA_RAM, &galaxian_attributesram },
	{ 0x9040, 0x905f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x9060, 0x907f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x9080, 0x90ff, MWA_RAM },
	{ 0x9800, 0x9803, ppi8255_0_w },
	{ 0xa000, 0xa003, ppi8255_1_w },
	{ 0xa801, 0xa801, interrupt_enable_w },
	{ 0xa802, 0xa802, type1_coin_counter_w },
	{ 0xa803, 0xa803, scramble_background_blue_w },
	{ 0xa804, 0xa804, galaxian_stars_enable_w },
	{ 0xa806, 0xa806, galaxian_flip_screen_x_w },
	{ 0xa807, 0xa807, galaxian_flip_screen_y_w },
MEMORY_END


static MEMORY_READ_START( type2_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x88ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9400, 0x97ff, galaxian_videoram_r },	/* mirror */
	{ 0x9800, 0x9800, watchdog_reset_r},
	{ 0xa000, 0xa00f, scobra_type2_ppi8255_0_r },
	{ 0xa800, 0xa80f, scobra_type2_ppi8255_1_r },
MEMORY_END

static MEMORY_WRITE_START( type2_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x883f, MWA_RAM, &galaxian_attributesram },
	{ 0x8840, 0x885f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x8860, 0x887f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x8880, 0x88ff, MWA_RAM },
	{ 0x9000, 0x93ff, MWA_RAM, &galaxian_videoram },
	{ 0x9400, 0x97ff, galaxian_videoram_w },	/* mirror */
	{ 0xa000, 0xa00f, scobra_type2_ppi8255_0_w },
	{ 0xa800, 0xa80f, scobra_type2_ppi8255_1_w },
	{ 0xb000, 0xb000, galaxian_stars_enable_w },
	{ 0xb002, 0xb002, scramble_background_blue_w },
	{ 0xb004, 0xb004, interrupt_enable_w },
	{ 0xb006, 0xb008, type2_coin_counter_w },
	{ 0xb00c, 0xb00c, galaxian_flip_screen_y_w },
	{ 0xb00e, 0xb00e, galaxian_flip_screen_x_w },
MEMORY_END


static MEMORY_READ_START( hustler_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8bff, MRA_RAM },
	{ 0x9000, 0x90ff, MRA_RAM },
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ 0xd000, 0xd01f, hustler_ppi8255_0_r },
	{ 0xe000, 0xe01f, hustler_ppi8255_1_r },
MEMORY_END

static MEMORY_WRITE_START( hustler_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, MWA_RAM, &galaxian_videoram },
	{ 0x9000, 0x903f, MWA_RAM, &galaxian_attributesram },
	{ 0x9040, 0x905f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x9060, 0x907f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x9080, 0x90ff, MWA_RAM },
	{ 0xa802, 0xa802, galaxian_flip_screen_x_w },
	{ 0xa804, 0xa804, interrupt_enable_w },
	{ 0xa806, 0xa806, galaxian_flip_screen_y_w },
	{ 0xa80e, 0xa80e, MWA_NOP },	/* coin counters */
	{ 0xd000, 0xd01f, hustler_ppi8255_0_w },
	{ 0xe000, 0xe01f, hustler_ppi8255_1_w },
MEMORY_END


static MEMORY_READ_START( hustlerb_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8bff, MRA_RAM },
	{ 0x9000, 0x90ff, MRA_RAM },
	{ 0xb000, 0xb000, watchdog_reset_r },
	{ 0xc100, 0xc103, ppi8255_0_r },
	{ 0xc200, 0xc203, ppi8255_1_r },
MEMORY_END

static MEMORY_WRITE_START( hustlerb_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, MWA_RAM, &galaxian_videoram },
	{ 0x9000, 0x903f, MWA_RAM, &galaxian_attributesram },
	{ 0x9040, 0x905f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x9060, 0x907f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x9080, 0x90ff, MWA_RAM },
	{ 0xa801, 0xa801, interrupt_enable_w },
	{ 0xa802, 0xa802, MWA_NOP },	/* coin counters */
	{ 0xa806, 0xa806, galaxian_flip_screen_y_w },
	{ 0xa807, 0xa807, galaxian_flip_screen_x_w },
	{ 0xc100, 0xc103, ppi8255_0_w },
	{ 0xc200, 0xc203, ppi8255_1_w },
MEMORY_END


static UINT8 *scobra_soundram;

static READ_HANDLER(scobra_soundram_r)
{
	return scobra_soundram[offset & 0x03ff];
}

static WRITE_HANDLER(scobra_soundram_w)
{
	scobra_soundram[offset & 0x03ff] = data;
}

MEMORY_READ_START( scobra_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0x8fff, scobra_soundram_r },
MEMORY_END

MEMORY_WRITE_START( scobra_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0x8fff, scobra_soundram_w },
	{ 0x8000, 0x83ff, MWA_NOP, &scobra_soundram },  /* only here to initialize pointer */
	{ 0x9000, 0x9fff, scramble_filter_w },
MEMORY_END


static MEMORY_WRITE_START( hustlerb_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x6000, 0x6fff, frogger_filter_w },
	{ 0x8000, 0x83ff, MWA_RAM },
MEMORY_END


PORT_READ_START( scobra_sound_readport )
	{ 0x20, 0x20, AY8910_read_port_0_r },
	{ 0x80, 0x80, AY8910_read_port_1_r },
PORT_END

PORT_WRITE_START( scobra_sound_writeport )
	{ 0x10, 0x10, AY8910_control_port_0_w },
	{ 0x20, 0x20, AY8910_write_port_0_w },
	{ 0x40, 0x40, AY8910_control_port_1_w },
	{ 0x80, 0x80, AY8910_write_port_1_w },
PORT_END


static PORT_READ_START( hustlerb_sound_readport )
	{ 0x80, 0x80, AY8910_read_port_0_r },
PORT_END

static PORT_WRITE_START( hustlerb_sound_writeport )
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_0_w },
PORT_END


INPUT_PORTS_START( scobra )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/99 Credits" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* identical to scobra apart from the number of lives */
INPUT_PORTS_START( scobras )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/99 Credits" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( stratgyx )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_SPECIAL ) /* lower 2 coinage DIPs */
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )

	PORT_START      /* IN3 */
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_SPECIAL ) /* upper 2 coinage DIPs */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* none of these appear to be used */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN4 - fake coinage DIPs.  read via IN2 and IN3 */
	PORT_DIPNAME( 0x0f, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0a, "A 2/1  B 1/3" )
  /*PORT_DIPSETTING(    0x02, "A 2/1  B 1/3" )*/
	PORT_DIPSETTING(    0x09, "A 2/1  B 1/6" )
	PORT_DIPSETTING(    0x03, "A 4/3  B 1/3" )
	PORT_DIPSETTING(    0x0c, "A 1/1  B 2/1" )
	PORT_DIPSETTING(    0x07, "A 1/1  B 1/2" )
	PORT_DIPSETTING(    0x01, "A 1/1  B 1/3" )
	PORT_DIPSETTING(    0x06, "A 1/1  B 1/5" )
  /*PORT_DIPSETTING(    0x0b, "A 1/1  B 1/5" )*/
	PORT_DIPSETTING(    0x05, "A 1/1  B 1/7" )
	PORT_DIPSETTING(    0x0d, "A 1/2  B 1/1" )
	PORT_DIPSETTING(    0x0e, "A 1/3  B 3/1" )
	PORT_DIPSETTING(    0x0f, "A 1/4  B 4/1" )
	PORT_DIPSETTING(    0x04, "A 1/99  B 2/1" )
  /*PORT_DIPSETTING(    0x08, "A 1/99  B 2/1" )*/
	PORT_DIPSETTING(    0x00, "A 1/99  B 1/3" )
INPUT_PORTS_END

INPUT_PORTS_START( armorcar )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( moonwar )
	PORT_START      /* IN0 */
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_SPECIAL ) /* the spinner */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2  | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1  | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )	/* output bits */

	PORT_START      /* IN3/4 - dummy ports for the dial */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER, 25, 10, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER | IPF_COCKTAIL, 25, 10, 0, 0 )
INPUT_PORTS_END

/* same as above, but coinage is different */
INPUT_PORTS_START( moonwara )
	PORT_START      /* IN0 */
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_SPECIAL ) /* the spinner */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2  | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1  | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )	/* output bits */

	PORT_START      /* IN3/4 - dummy ports for the dial */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER, 25, 10, 0, 0 )

	PORT_START		/* doesn't actually work due to bug in game code */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER | IPF_COCKTAIL, 25, 10, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( spdcoin )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Freeze" )   /* Dip Sw #2 */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR ( Unknown ) ) /* Dip Sw #1 */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )    /* Dip Sw #5 */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )    /* Dip Sw #4 */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )    /* Dip Sw #3 */
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* cocktail mode is N/A */
INPUT_PORTS_START( darkplnt )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Bonus Occurrence" )
	PORT_DIPSETTING(    0x01, "Once" )
	PORT_DIPSETTING(    0x00, "Every" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_ANALOG( 0xfc, 0x00, IPT_DIAL, 25, 10, 0, 0 )	/* scrambled dial */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "100k" )
	PORT_DIPSETTING(    0x08, "200k" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tazmania )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* cocktail mode is N/A */
INPUT_PORTS_START( calipso )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x08, 0x08, "Cabinet (Not Supported)" )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* cocktail mode not working due to bug in game code */
INPUT_PORTS_START( anteater )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* cocktail mode is N/A */
INPUT_PORTS_START( rescue )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x02, 0x02, "Starting Level" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPSETTING(    0x04, "Coin A 1/3 Coin B 3/1" )
	PORT_DIPSETTING(    0x06, "Coin A 1/4 Coin B 4/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* cocktail mode is N/A */
INPUT_PORTS_START( minefld )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x02, 0x02, "Starting Level" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "Coin A 1/2 Coin B 2/1" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* cocktail mode is N/A */
INPUT_PORTS_START( losttomb )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "A 1/2 B 2/1" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* cocktail mode is N/A */
INPUT_PORTS_START( superbon )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, "A 1/2 B 2/1" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( hustler )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_BITX(    0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



struct AY8910interface scobra_ay8910_interface =
{
	2,	/* 2 chips */
	14318000/8,	/* 1.78975 MHz */
	/* Ant Eater clips if the volume is set higher than this */
	{ MIXERG(16,MIXER_GAIN_2x,MIXER_PAN_CENTER), MIXERG(16,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ 0, soundlatch_r },
	{ 0, scramble_portB_r },
	{ 0, 0 },
	{ 0, 0 }
};


static struct MachineDriver machine_driver_type1 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type1_readmem,type1_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	scramble_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

/* same as regular type 1, the only difference that it has long bullets */
static struct MachineDriver machine_driver_armorcar =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type1_readmem,type1_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	scramble_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	theend_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

/* same as regular type 1, the only difference is that the bullets are less yellow */
static struct MachineDriver machine_driver_moonwar =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type1_readmem,type1_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	moonwar_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

/* Rescue, Minefield and Strategy X have extra colors, and custom video initialise */
/* routines to set up the graduated color backgound they use */
static struct MachineDriver machine_driver_rescue =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type1_readmem,type1_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+64,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 64 for background */	\
	rescue_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	rescue_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_minefld =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type1_readmem,type1_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+128,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 128 for background */	\
	minefld_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	minefld_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_stratgyx =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type2_readmem,type2_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+8,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 8 for background */	\
	stratgyx_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	stratgyx_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_type2 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type2_readmem,type2_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	scramble_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_darkplnt =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type2_readmem,type2_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+2+1,8*4+128*1,
	darkplnt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	darkplnt_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_hustler =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			hustler_readmem,hustler_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			frogger_sound_readmem,frogger_sound_writemem,frogger_sound_readport,frogger_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	scramble_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&frogger_ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_hustlerb =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			hustlerb_readmem,hustlerb_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,hustlerb_sound_writemem,hustlerb_sound_readport,hustlerb_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	scramble_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	scramble_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&frogger_ay8910_interface
		}
	}
};

/* same as the others, but no sprite flipping, but instead, the bits are used
   as extra sprite code bits, giving 256 sprite images */
static struct MachineDriver machine_driver_calipso =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			type1_readmem,type1_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	scramble_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	calipso_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&scobra_ay8910_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scobra )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x1000, 0xa0744b3f )
	ROM_LOAD( "2e",           0x1000, 0x1000, 0x8e7245cd )
	ROM_LOAD( "2f",           0x2000, 0x1000, 0x47a4e6fb )
	ROM_LOAD( "2h",           0x3000, 0x1000, 0x7244f21c )
	ROM_LOAD( "2j",           0x4000, 0x1000, 0xe1f8a801 )
	ROM_LOAD( "2l",           0x5000, 0x1000, 0xd52affde )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xd4346959 )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0xcc025d95 )
	ROM_LOAD( "5e",           0x1000, 0x0800, 0x1628c53f )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x64d113b4 )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0xa96316d3 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x9b87f90d )
ROM_END

ROM_START( scobras )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "scobra2c.bin", 0x0000, 0x1000, 0xe15ade38 )
	ROM_LOAD( "scobra2e.bin", 0x1000, 0x1000, 0xa270e44d )
	ROM_LOAD( "scobra2f.bin", 0x2000, 0x1000, 0xbdd70346 )
	ROM_LOAD( "scobra2h.bin", 0x3000, 0x1000, 0xdca5ec31 )
	ROM_LOAD( "scobra2j.bin", 0x4000, 0x1000, 0x0d8f6b6e )
	ROM_LOAD( "scobra2l.bin", 0x5000, 0x1000, 0x6f80f3a9 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "snd_5c.bin",   0x0000, 0x0800, 0xdeeb0dd3 )
	ROM_LOAD( "snd_5d.bin",   0x0800, 0x0800, 0x872c1a74 )
	ROM_LOAD( "snd_5e.bin",   0x1000, 0x0800, 0xccd7a110 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x64d113b4 )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0xa96316d3 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x9b87f90d )
ROM_END

ROM_START( scobrab )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "vid_2c.bin",   0x0000, 0x0800, 0xaeddf391 )
	ROM_LOAD( "vid_2e.bin",   0x0800, 0x0800, 0x72b57eb7 )
	ROM_LOAD( "scobra2e.bin", 0x1000, 0x1000, 0xa270e44d )
	ROM_LOAD( "scobra2f.bin", 0x2000, 0x1000, 0xbdd70346 )
	ROM_LOAD( "scobra2h.bin", 0x3000, 0x1000, 0xdca5ec31 )
	ROM_LOAD( "scobra2j.bin", 0x4000, 0x1000, 0x0d8f6b6e )
	ROM_LOAD( "scobra2l.bin", 0x5000, 0x1000, 0x6f80f3a9 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "snd_5c.bin",   0x0000, 0x0800, 0xdeeb0dd3 )
	ROM_LOAD( "snd_5d.bin",   0x0800, 0x0800, 0x872c1a74 )
	ROM_LOAD( "snd_5e.bin",   0x1000, 0x0800, 0xccd7a110 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x64d113b4 )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0xa96316d3 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x9b87f90d )
ROM_END

ROM_START( stratgyx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c_1.bin",     0x0000, 0x1000, 0xeec01237 )
	ROM_LOAD( "2e_2.bin",     0x1000, 0x1000, 0x926cb2d5 )
	ROM_LOAD( "2f_3.bin",     0x2000, 0x1000, 0x849e2504 )
	ROM_LOAD( "2h_4.bin",     0x3000, 0x1000, 0x8a64069b )
	ROM_LOAD( "2j_5.bin",     0x4000, 0x1000, 0x78b9b898 )
	ROM_LOAD( "2l_6.bin",     0x5000, 0x1000, 0x20bae414 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound code */
	ROM_LOAD( "s1.bin",       0x0000, 0x1000, 0x713a5db8 )
	ROM_LOAD( "s2.bin",       0x1000, 0x1000, 0x46079411 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f_c2.bin",    0x0000, 0x0800, 0x7121b679 )
	ROM_LOAD( "5h_c1.bin",    0x0800, 0x0800, 0xd105ad91 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "strategy.6e",  0x0000, 0x0020, 0x51a629e1 )

	ROM_REGION( 0x0020, REGION_USER1, 0 )
	ROM_LOAD( "strategy.10k", 0x0000, 0x0020, 0xd95c0318 )	/* background color map */
ROM_END

ROM_START( stratgys )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c.cpu",       0x0000, 0x1000, 0xf2aaaf2b )
	ROM_LOAD( "2e.cpu",       0x1000, 0x1000, 0x5873fdc8 )
	ROM_LOAD( "2f.cpu",       0x2000, 0x1000, 0x532d604f )
	ROM_LOAD( "2h.cpu",       0x3000, 0x1000, 0x82b1d95e )
	ROM_LOAD( "2j.cpu",       0x4000, 0x1000, 0x66e84cde )
	ROM_LOAD( "2l.cpu",       0x5000, 0x1000, 0x62b032d0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound code */
	ROM_LOAD( "s1.bin",       0x0000, 0x1000, 0x713a5db8 )
	ROM_LOAD( "s2.bin",       0x1000, 0x1000, 0x46079411 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f.cpu",       0x0000, 0x0800, 0xf4aa5ddd )
	ROM_LOAD( "5h.cpu",       0x0800, 0x0800, 0x548e4635 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "strategy.6e",  0x0000, 0x0020, 0x51a629e1 )

	ROM_REGION( 0x0020, REGION_USER1, 0 )
	ROM_LOAD( "strategy.10k", 0x0000, 0x0020, 0xd95c0318 )
ROM_END

ROM_START( armorcar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu.2c",       0x0000, 0x1000, 0x0d7bfdfb )
	ROM_LOAD( "cpu.2e",       0x1000, 0x1000, 0x76463213 )
	ROM_LOAD( "cpu.2f",       0x2000, 0x1000, 0x2cc6d5f0 )
	ROM_LOAD( "cpu.2h",       0x3000, 0x1000, 0x61278dbb )
	ROM_LOAD( "cpu.2j",       0x4000, 0x1000, 0xfb158d8c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "sound.5c",     0x0000, 0x0800, 0x54ee7753 )
	ROM_LOAD( "sound.5d",     0x0800, 0x0800, 0x5218fec0 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu.5f",       0x0000, 0x0800, 0x8a3da4d1 )
	ROM_LOAD( "cpu.5h",       0x0800, 0x0800, 0x85bdb113 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x9b87f90d )
ROM_END

ROM_START( armorca2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x1000, 0xe393bd2f )
	ROM_LOAD( "2e",           0x1000, 0x1000, 0xb7d443af )
	ROM_LOAD( "2g",           0x2000, 0x1000, 0xe67380a4 )
	ROM_LOAD( "2h",           0x3000, 0x1000, 0x72af7b37 )
	ROM_LOAD( "2j",           0x4000, 0x1000, 0xe6b0dd7f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "sound.5c",     0x0000, 0x0800, 0x54ee7753 )
	ROM_LOAD( "sound.5d",     0x0800, 0x0800, 0x5218fec0 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cpu.5f",       0x0000, 0x0800, 0x8a3da4d1 )
	ROM_LOAD( "cpu.5h",       0x0800, 0x0800, 0x85bdb113 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x9b87f90d )
ROM_END

ROM_START( moonwar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "mw2.2c",       0x0000, 0x1000, 0x7c11b4d9 )
	ROM_LOAD( "mw2.2e",       0x1000, 0x1000, 0x1b6362be )
	ROM_LOAD( "mw2.2f",       0x2000, 0x1000, 0x4fd8ba4b )
	ROM_LOAD( "mw2.2h",       0x3000, 0x1000, 0x56879f0d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "mw2.5c",       0x0000, 0x0800, 0xc26231eb )
	ROM_LOAD( "mw2.5d",       0x0800, 0x0800, 0xbb48a646 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mw2.5f",       0x0000, 0x0800, 0xc5fa1aa0 )
	ROM_LOAD( "mw2.5h",       0x0800, 0x0800, 0xa6ccc652 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "mw2.clr",      0x0000, 0x0020, 0x99614c6c )
ROM_END

ROM_START( moonwara )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x1000, 0xbc20b734 )
	ROM_LOAD( "2e",           0x1000, 0x1000, 0xdb6ffec2 )
	ROM_LOAD( "2f",           0x2000, 0x1000, 0x378931b8 )
	ROM_LOAD( "2h",           0x3000, 0x1000, 0x031dbc2c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "mw2.5c",       0x0000, 0x0800, 0xc26231eb )
	ROM_LOAD( "mw2.5d",       0x0800, 0x0800, 0xbb48a646 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mw2.5f",       0x0000, 0x0800, 0xc5fa1aa0 )
	ROM_LOAD( "mw2.5h",       0x0800, 0x0800, 0xa6ccc652 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "moonwara.clr", 0x0000, 0x0020, 0xf58d4f58 )	/* olive, instead of white */
ROM_END

ROM_START( spdcoin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "spdcoin.2c",   0x0000, 0x1000, 0x65cf1e49 )
	ROM_LOAD( "spdcoin.2e",   0x1000, 0x1000, 0x1ee59232 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "spdcoin.5c",   0x0000, 0x0800, 0xb4cf64b7 )
	ROM_LOAD( "spdcoin.5d",   0x0800, 0x0800, 0x92304df0 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spdcoin.5f",   0x0000, 0x0800, 0xdd5f1dbc )
	ROM_LOAD( "spdcoin.5h",   0x0800, 0x0800, 0xab1fe81b )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "spdcoin.clr",  0x0000, 0x0020, 0x1a2ccc56 )
ROM_END

ROM_START( darkplnt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "drkplt2c.dat", 0x0000, 0x1000, 0x5a0ca559 )
	ROM_LOAD( "drkplt2e.dat", 0x1000, 0x1000, 0x52e2117d )
	ROM_LOAD( "drkplt2g.dat", 0x2000, 0x1000, 0x4093219c )
	ROM_LOAD( "drkplt2j.dat", 0x3000, 0x1000, 0xb974c78d )
	ROM_LOAD( "drkplt2k.dat", 0x4000, 0x1000, 0x71a37385 )
	ROM_LOAD( "drkplt2l.dat", 0x5000, 0x1000, 0x5ad25154 )
	ROM_LOAD( "drkplt2m.dat", 0x6000, 0x1000, 0x8d2f0122 )
	ROM_LOAD( "drkplt2p.dat", 0x7000, 0x1000, 0x2d66253b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c.snd",       0x0000, 0x1000, 0x672b9454 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "drkplt5f.dat", 0x0000, 0x0800, 0x2af0ee66 )
	ROM_LOAD( "drkplt5h.dat", 0x0800, 0x0800, 0x66ef3225 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6e.cpu",       0x0000, 0x0020, 0x86b6e124 )
ROM_END

ROM_START( tazmania )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c.cpu",       0x0000, 0x1000, 0x932c5a06 )
	ROM_LOAD( "2e.cpu",       0x1000, 0x1000, 0xef17ce65 )
	ROM_LOAD( "2f.cpu",       0x2000, 0x1000, 0x43c7c39d )
	ROM_LOAD( "2h.cpu",       0x3000, 0x1000, 0xbe829694 )
	ROM_LOAD( "2j.cpu",       0x4000, 0x1000, 0x6e197271 )
	ROM_LOAD( "2k.cpu",       0x5000, 0x1000, 0xa1eb453b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "rom0.snd",     0x0000, 0x0800, 0xb8d741f1 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f.cpu",       0x0000, 0x0800, 0x2c5b612b )
	ROM_LOAD( "5h.cpu",       0x0800, 0x0800, 0x3f5ff3ac )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "colr6f.cpu",   0x0000, 0x0020, 0xfce333c7 )
ROM_END

ROM_START( tazmani2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2ck.cpu",      0x0000, 0x1000, 0xbf0492bf )
	ROM_LOAD( "2ek.cpu",      0x1000, 0x1000, 0x6636c4d0 )
	ROM_LOAD( "2fk.cpu",      0x2000, 0x1000, 0xce59a57b )
	ROM_LOAD( "2hk.cpu",      0x3000, 0x1000, 0x8bda3380 )
	ROM_LOAD( "2jk.cpu",      0x4000, 0x1000, 0xa4095e35 )
	ROM_LOAD( "2kk.cpu",      0x5000, 0x1000, 0xf308ca36 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "rom0.snd",     0x0000, 0x0800, 0xb8d741f1 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f.cpu",       0x0000, 0x0800, 0x2c5b612b )
	ROM_LOAD( "5h.cpu",       0x0800, 0x0800, 0x3f5ff3ac )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "colr6f.cpu",   0x0000, 0x0020, 0xfce333c7 )
ROM_END

ROM_START( calipso )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "calipso.2c",   0x0000, 0x1000, 0x0fcb703c )
	ROM_LOAD( "calipso.2e",   0x1000, 0x1000, 0xc6622f14 )
	ROM_LOAD( "calipso.2f",   0x2000, 0x1000, 0x7bacbaba )
	ROM_LOAD( "calipso.2h",   0x3000, 0x1000, 0xa3a8111b )
	ROM_LOAD( "calipso.2j",   0x4000, 0x1000, 0xfcbd7b9e )
	ROM_LOAD( "calipso.2l",   0x5000, 0x1000, 0xf7630cab )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound code */
	ROM_LOAD( "calipso.5c",   0x0000, 0x0800, 0x9cbc65ab )
	ROM_LOAD( "calipso.5d",   0x0800, 0x0800, 0xa225ee3b )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "calipso.5f",   0x0000, 0x2000, 0xfd4252e9 )
	ROM_LOAD( "calipso.5h",   0x2000, 0x2000, 0x1663a73a )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "calipso.clr",  0x0000, 0x0020, 0x01165832 )
ROM_END

ROM_START( anteater )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ra1-2c",       0x0000, 0x1000, 0x58bc9393 )
	ROM_LOAD( "ra1-2e",       0x1000, 0x1000, 0x574fc6f6 )
	ROM_LOAD( "ra1-2f",       0x2000, 0x1000, 0x2f7c1fe5 )
	ROM_LOAD( "ra1-2h",       0x3000, 0x1000, 0xae8a5da3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ra4-5c",       0x0000, 0x0800, 0x87300b4f )
	ROM_LOAD( "ra4-5d",       0x0800, 0x0800, 0xaf4e5ffe )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ra6-5f",       0x0000, 0x0800, 0x4c3f8a08 )
	ROM_LOAD( "ra6-5h",       0x0800, 0x0800, 0xb30c7c9f )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "colr6f.cpu",   0x0000, 0x0020, 0xfce333c7 )
ROM_END

ROM_START( rescue )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "rb15acpu.bin", 0x0000, 0x1000, 0xd7e654ba )
	ROM_LOAD( "rb15bcpu.bin", 0x1000, 0x1000, 0xa93ea158 )
	ROM_LOAD( "rb15ccpu.bin", 0x2000, 0x1000, 0x058cd3d0 )
	ROM_LOAD( "rb15dcpu.bin", 0x3000, 0x1000, 0xd6505742 )
	ROM_LOAD( "rb15ecpu.bin", 0x4000, 0x1000, 0x604df3a4 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "rb15csnd.bin", 0x0000, 0x0800, 0x8b24bf17 )
	ROM_LOAD( "rb15dsnd.bin", 0x0800, 0x0800, 0xd96e4fb3 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rb15fcpu.bin", 0x0000, 0x0800, 0x4489d20c )
	ROM_LOAD( "rb15hcpu.bin", 0x0800, 0x0800, 0x5512c547 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "rescue.clr",   0x0000, 0x0020, 0x40c6bcbd )
ROM_END

ROM_START( minefld )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ma22c",        0x0000, 0x1000, 0x1367a035 )
	ROM_LOAD( "ma22e",        0x1000, 0x1000, 0x68946d21 )
	ROM_LOAD( "ma22f",        0x2000, 0x1000, 0x7663aee5 )
	ROM_LOAD( "ma22h",        0x3000, 0x1000, 0x9787475d )
	ROM_LOAD( "ma22j",        0x4000, 0x1000, 0x2ceceb54 )
	ROM_LOAD( "ma22l",        0x5000, 0x1000, 0x85138fc9 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ma15c",        0x0000, 0x0800, 0x8bef736b )
	ROM_LOAD( "ma15d",        0x0800, 0x0800, 0xf67b3f97 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ma15f",        0x0000, 0x0800, 0x9f703006 )
	ROM_LOAD( "ma15h",        0x0800, 0x0800, 0xed0dccb1 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "minefld.clr",  0x0000, 0x0020, 0x1877368e )
ROM_END

ROM_START( losttomb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x1000, 0xd6176d2c )
	ROM_LOAD( "2e",           0x1000, 0x1000, 0xa5f55f4a )
	ROM_LOAD( "2f",           0x2000, 0x1000, 0x0169fa3c )
	ROM_LOAD( "2h-easy",      0x3000, 0x1000, 0x054481b6 )
	ROM_LOAD( "2j",           0x4000, 0x1000, 0x249ee040 )
	ROM_LOAD( "2l",           0x5000, 0x1000, 0xc7d2e608 )
	ROM_LOAD( "2m",           0x6000, 0x1000, 0xbc4bc5b1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xb899be2a )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0x6907af31 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x61f137e7 )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0x5581de5f )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ltprom",       0x0000, 0x0020, 0x1108b816 )
ROM_END

ROM_START( losttmbh )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x1000, 0xd6176d2c )
	ROM_LOAD( "2e",           0x1000, 0x1000, 0xa5f55f4a )
	ROM_LOAD( "2f",           0x2000, 0x1000, 0x0169fa3c )
	ROM_LOAD( "lthard",       0x3000, 0x1000, 0xe32cbf0e )
	ROM_LOAD( "2j",           0x4000, 0x1000, 0x249ee040 )
	ROM_LOAD( "2l",           0x5000, 0x1000, 0xc7d2e608 )
	ROM_LOAD( "2m",           0x6000, 0x1000, 0xbc4bc5b1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xb899be2a )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0x6907af31 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x61f137e7 )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0x5581de5f )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ltprom",       0x0000, 0x0020, 0x1108b816 )
ROM_END

ROM_START( superbon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2d.cpu",       0x0000, 0x1000, 0x60c0ba18 )
	ROM_LOAD( "2e.cpu",       0x1000, 0x1000, 0xddcf44bf )
	ROM_LOAD( "2f.cpu",       0x2000, 0x1000, 0xbb66c2d5 )
	ROM_LOAD( "2h.cpu",       0x3000, 0x1000, 0x74f4f04d )
	ROM_LOAD( "2j.cpu",       0x4000, 0x1000, 0x78effb08 )
	ROM_LOAD( "2l.cpu",       0x5000, 0x1000, 0xe9dcecbd )
	ROM_LOAD( "2m.cpu",       0x6000, 0x1000, 0x3ed0337e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",  	      0x0000, 0x0800, 0xb899be2a )
	ROM_LOAD( "5d.snd",       0x0800, 0x0800, 0x80640a04 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f.cpu",       0x0000, 0x0800, 0x5b9d4686 )
	ROM_LOAD( "5h.cpu",       0x0800, 0x0800, 0x58c29927 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "superbon.clr", 0x0000, 0x0020, 0x00000000 )
ROM_END

ROM_START( hustler )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hustler.1",    0x0000, 0x1000, 0x94479a3e )
	ROM_LOAD( "hustler.2",    0x1000, 0x1000, 0x3cc67bcc )
	ROM_LOAD( "hustler.3",    0x2000, 0x1000, 0x9422226a )
	/* 3000-3fff space for diagnostics ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "hustler.6",    0x0000, 0x0800, 0x7a946544 )
	ROM_LOAD( "hustler.7",    0x0800, 0x0800, 0x3db57351 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hustler.5f",   0x0000, 0x0800, 0x0bdfad0e )
	ROM_LOAD( "hustler.5h",   0x0800, 0x0800, 0x8e062177 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "hustler.clr",  0x0000, 0x0020, 0xaa1f7f5e )
ROM_END

ROM_START( billiard )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a",            0x0000, 0x1000, 0xb7eb50c0 )
	ROM_LOAD( "b",            0x1000, 0x1000, 0x988fe1c5 )
	ROM_LOAD( "c",            0x2000, 0x1000, 0x7b8de793 )
	/* 3000-3fff space for diagnostics ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "hustler.6",    0x0000, 0x0800, 0x7a946544 )
	ROM_LOAD( "hustler.7",    0x0800, 0x0800, 0x3db57351 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hustler.5f",   0x0000, 0x0800, 0x0bdfad0e )
	ROM_LOAD( "hustler.5h",   0x0800, 0x0800, 0x8e062177 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "hustler.clr",  0x0000, 0x0020, 0xaa1f7f5e )
ROM_END

/* this is identical to billiard, but with a different memory map */
ROM_START( hustlerb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hustler.2c",   0x0000, 0x1000, 0x3a1ac6a9 )
	ROM_LOAD( "hustler.2f",   0x1000, 0x1000, 0xdc6752ec )
	ROM_LOAD( "hustler.2j",   0x2000, 0x1000, 0x27c1e0f8 )
	/* 3000-3fff space for diagnostics ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "hustler.11d",  0x0000, 0x0800, 0xb559bfde )
	ROM_LOAD( "hustler.10d",  0x0800, 0x0800, 0x6ef96cfb )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hustler.5f",   0x0000, 0x0800, 0x0bdfad0e )
	ROM_LOAD( "hustler.5h",   0x0800, 0x0800, 0x8e062177 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "hustler.clr",  0x0000, 0x0020, 0xaa1f7f5e )
ROM_END



GAME( 1981, scobra,   0,        type1,    scobra,   scobra,	  ROT90,  "Konami", "Super Cobra" )
GAME( 1981, scobras,  scobra,   type1,    scobras,  scobra,	  ROT90,  "[Konami] (Stern license)", "Super Cobra (Stern)" )
GAME( 1981, scobrab,  scobra,   type1,    scobras,  scobra,	  ROT90,  "bootleg", "Super Cobra (bootleg)" )
GAME( 1981, stratgyx, 0,        stratgyx, stratgyx, stratgyx, ROT0,   "Konami", "Strategy X" )
GAME( 1981, stratgys, stratgyx, stratgyx, stratgyx, stratgyx, ROT0,   "[Konami] (Stern license)", "Strategy X (Stern)" )
GAME( 1981, armorcar, 0,        stratgyx, armorcar, scobra,	  ROT90,  "Stern", "Armored Car (set 1)" )
GAME( 1981, armorca2, armorcar, armorcar, armorcar, scobra,	  ROT90,  "Stern", "Armored Car (set 2)" )
GAME( 1981, moonwar,  0,        moonwar,  moonwar,  moonwar,  ROT90,  "Stern", "Moonwar" )
GAME( 1981, moonwara, moonwar,  moonwar,  moonwara, moonwar,  ROT90,  "Stern", "Moonwar (older)" )
GAME( 1984, spdcoin,  0,        type1,    spdcoin,  scobra,	  ROT90,  "Stern", "Speed Coin (prototype)" )
GAME( 1982, darkplnt, 0,        darkplnt, darkplnt, darkplnt, ROT180, "Stern", "Dark Planet" )
GAME( 1982, tazmania, 0,        type1,    tazmania, scobra,	  ROT90,  "Stern", "Tazz-Mania" )
GAME( 1982, tazmani2, tazmania, type2,    tazmania, scobra,	  ROT90,  "Stern", "Tazz-Mania (set 2)" )
GAME( 1982, calipso,  0,        calipso,  calipso,  scobra,	  ROT90,  "[Stern] (Tago license)", "Calipso" )
GAME( 1982, anteater, 0,        type1,    anteater, anteater, ROT90,  "[Stern] (Tago license)", "Anteater" )
GAME( 1982, rescue,   0,        rescue,   rescue,   rescue,	  ROT90,  "Stern", "Rescue" )
GAME( 1983, minefld,  0,        minefld,  minefld,  minefld,  ROT90,  "Stern", "Minefield" )
GAME( 1982, losttomb, 0,        type1,    losttomb, losttomb, ROT90,  "Stern", "Lost Tomb (easy)" )
GAME( 1982, losttmbh, losttomb, type1,    losttomb, losttomb, ROT90,  "Stern", "Lost Tomb (hard)" )
GAMEX(1982?,superbon, 0,        type1,    superbon, superbon, ROT90,  "bootleg", "Super Bond", GAME_WRONG_COLORS )
GAME( 1981, hustler,  0,        hustler,  hustler,  hustler,  ROT90,  "Konami", "Video Hustler" )
GAME( 1981, billiard, hustler,  hustler,  hustler,  billiard, ROT90,  "bootleg", "The Billiards" )
GAME( 1981, hustlerb, hustler,  hustlerb, hustler,  scobra,   ROT90,  "bootleg", "Video Hustler (bootleg)" )
