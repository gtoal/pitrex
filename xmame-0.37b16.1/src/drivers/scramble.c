/***************************************************************************

Scramble memory map (preliminary)

MAIN BOARD:
0000-3fff ROM
4000-47ff RAM
4800-4bff Video RAM
5000-50ff Object RAM
5000-503f  screen attributes
5040-505f  sprites
5060-507f  bullets
5080-50ff  unused?

read:
7000      Watchdog Reset (Scramble)
7800      Watchdog Reset (Battle of Atlantis)
8100      IN0
8101      IN1
8102      IN2 (bits 5 and 7 used for protection check in Scramble)

write:
6801      interrupt enable
6802      coin counter
6803      ? (POUT1)
6804      stars on
6805      ? (POUT2)
6806      screen vertical flip
6807      screen horizontal flip
8200      To AY-3-8910 port A (commands for the audio CPU)
8201      bit 3 = interrupt trigger on audio CPU  bit 4 = AMPM (?)
8202      protection check control?


SOUND BOARD:
0000-1fff ROM
8000-83ff RAM

I/0 ports:
read:
20      8910 #2  read
80      8910 #1  read

write
10      8910 #2  control20      8910 #2  write
40      8910 #1  control
80      8910 #1  write

interrupts:
interrupt mode 1 triggered by the main CPU


Interesting tidbit:

There is a bug in Amidars and Triple Punch. Look at the loop at 0x2715.
It expects DE to be saved during the call to 0x2726, but it can be destroyed,
causing the loop to read all kinds of bogus memory locations.


To Do:

- Mariner has discrete sound circuits connected to the 8910's output ports


Notes:

- While Atlantis has a cabinet switch, it doesn't use the 2nd player controls
  in cocktail mode.

***************************************************************************/

#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "machine/8255ppi.h"


extern unsigned char *galaxian_videoram;
extern unsigned char *galaxian_spriteram;
extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern size_t galaxian_spriteram_size;
extern size_t galaxian_bulletsram_size;

extern struct AY8910interface scobra_ay8910_interface;
extern struct AY8910interface frogger_ay8910_interface;
extern const struct Memory_ReadAddress scobra_sound_readmem[];
extern const struct Memory_ReadAddress frogger_sound_readmem[];
extern const struct Memory_WriteAddress scobra_sound_writemem[];
extern const struct Memory_WriteAddress frogger_sound_writemem[];
extern const struct IO_ReadPort scobra_sound_readport[];
extern const struct IO_ReadPort frogger_sound_readport[];
extern const struct IO_WritePort scobra_sound_writeport[];
extern const struct IO_WritePort frogger_sound_writeport[];

extern struct GfxDecodeInfo galaxian_gfxdecodeinfo[];

void scramble_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mariner_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void frogger_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
WRITE_HANDLER( galaxian_videoram_w );
READ_HANDLER( galaxian_videoram_r );
WRITE_HANDLER( galaxian_stars_enable_w );
WRITE_HANDLER( scramble_background_blue_w );

void init_scobra(void);
void init_scramble(void);
void init_scrambls(void);
void init_theend(void);
void init_ckongs(void);
void init_mariner(void);
void init_froggers(void);
void init_mars(void);
void init_hotshock(void);
void init_cavelon(void);

void scramble_init_machine(void);

int scramble_vh_start(void);
int theend_vh_start(void);
int mariner_vh_start(void);
int ckongs_vh_start(void);
int pisces_vh_start(void);
int froggers_vh_start(void);

void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

WRITE_HANDLER( pisces_gfxbank_w );

int hunchbks_vh_interrupt(void);
WRITE_HANDLER( galaxian_flip_screen_x_w );
WRITE_HANDLER( galaxian_flip_screen_y_w );

WRITE_HANDLER( hotshock_sh_irqtrigger_w );

READ_HANDLER(mars_ppi8255_0_r);
READ_HANDLER(mars_ppi8255_1_r);
WRITE_HANDLER(mars_ppi8255_0_w);
WRITE_HANDLER(mars_ppi8255_1_w);

/* protection stuff. in machine\scramble.c */
READ_HANDLER( triplep_pip_r );
READ_HANDLER( triplep_pap_r );


static WRITE_HANDLER( scramble_coin_counter_1_w )
{
	coin_counter_w(0, data);
}

static WRITE_HANDLER( scramble_coin_counter_2_w )
{
	coin_counter_w(1, data);
}

static WRITE_HANDLER( scramble_coin_counter_3_w )
{
	coin_counter_w(2, data);
}

static WRITE_HANDLER( flip_screen_w )
{
	flip_screen_set(data);
}


READ_HANDLER( hunchbks_mirror_r )
{
	return cpu_readmem16(0x1000+offset);
}

WRITE_HANDLER( hunchbks_mirror_w )
{
	cpu_writemem16(0x1000+offset,data);
}


static MEMORY_READ_START( scramble_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },	/* mirror */
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x7800, 0x7800, watchdog_reset_r },
	{ 0x8100, 0x8103, ppi8255_0_r },
	{ 0x8110, 0x8113, ppi8255_0_r },  /* mirror for Frog */
	{ 0x8200, 0x8203, ppi8255_1_r },
MEMORY_END

static MEMORY_WRITE_START( scramble_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, MWA_RAM, &galaxian_videoram },
	{ 0x4c00, 0x4fff, galaxian_videoram_w },	/* mirror address */
	{ 0x5000, 0x503f, MWA_RAM, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6801, 0x6801, interrupt_enable_w },
	{ 0x6802, 0x6802, scramble_coin_counter_1_w },
	{ 0x6803, 0x6803, scramble_background_blue_w },
	{ 0x6804, 0x6804, galaxian_stars_enable_w },
	{ 0x6806, 0x6806, galaxian_flip_screen_x_w },
	{ 0x6807, 0x6807, galaxian_flip_screen_y_w },
	{ 0x8100, 0x8103, ppi8255_0_w },
	{ 0x8200, 0x8203, ppi8255_1_w },
MEMORY_END


static MEMORY_READ_START( ckongs_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6bff, MRA_RAM },
	{ 0x7000, 0x7003, ppi8255_0_r },
	{ 0x7800, 0x7803, ppi8255_1_r },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9800, 0x98ff, MRA_RAM },
	{ 0xb000, 0xb000, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( ckongs_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6bff, MWA_RAM },
	{ 0x7000, 0x7003, ppi8255_0_w },
	{ 0x7800, 0x7803, ppi8255_1_w },
	{ 0x9000, 0x93ff, MWA_RAM, &galaxian_videoram },
	{ 0x9800, 0x983f, MWA_RAM, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x9860, 0x987f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x9880, 0x98ff, MWA_RAM },
	{ 0xa801, 0xa801, interrupt_enable_w },
	{ 0xa802, 0xa802, scramble_coin_counter_1_w },
	{ 0xa806, 0xa806, galaxian_flip_screen_x_w },
	{ 0xa807, 0xa807, galaxian_flip_screen_y_w },
MEMORY_END


static MEMORY_READ_START( mars_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8100, 0x810f, mars_ppi8255_0_r },
	{ 0x8200, 0x820f, mars_ppi8255_1_r },
MEMORY_END

static MEMORY_WRITE_START( mars_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, MWA_RAM, &galaxian_videoram },
	{ 0x5000, 0x503f, MWA_RAM, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6800, 0x6800, scramble_coin_counter_2_w },
	{ 0x6801, 0x6801, galaxian_stars_enable_w },
	{ 0x6802, 0x6802, interrupt_enable_w },
	{ 0x6808, 0x6808, scramble_coin_counter_1_w },
	{ 0x6809, 0x6809, galaxian_flip_screen_x_w },
	{ 0x680b, 0x680b, galaxian_flip_screen_y_w },
	{ 0x8100, 0x810f, mars_ppi8255_0_w },
	{ 0x8200, 0x820f, mars_ppi8255_1_w },
MEMORY_END


static MEMORY_READ_START( newsin7_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8200, 0x820f, mars_ppi8255_1_r },
	{ 0xa000, 0xafff, MRA_ROM },
	{ 0xc100, 0xc10f, mars_ppi8255_0_r },
MEMORY_END

static MEMORY_WRITE_START( newsin7_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, MWA_RAM, &galaxian_videoram },
	{ 0x5000, 0x503f, MWA_RAM, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6800, 0x6800, scramble_coin_counter_2_w },
	{ 0x6801, 0x6801, galaxian_stars_enable_w },
	{ 0x6802, 0x6802, interrupt_enable_w },
	{ 0x6808, 0x6808, scramble_coin_counter_1_w },
	{ 0x6809, 0x6809, galaxian_flip_screen_x_w },
	{ 0x680b, 0x680b, galaxian_flip_screen_y_w },
	{ 0x8200, 0x820f, mars_ppi8255_1_w },
	{ 0xa000, 0xafff, MWA_ROM },
	{ 0xc100, 0xc10f, mars_ppi8255_0_w },
MEMORY_END


static MEMORY_READ_START( hotshock_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4bff, MRA_RAM },
	{ 0x4c00, 0x4fff, galaxian_videoram_r },
	{ 0x5000, 0x50ff, MRA_RAM },
	{ 0x8000, 0x8000, input_port_0_r },
	{ 0x8001, 0x8001, input_port_1_r },
	{ 0x8002, 0x8002, input_port_2_r },
	{ 0x8003, 0x8003, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( hotshock_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, MWA_RAM, &galaxian_videoram },
	{ 0x5000, 0x503f, MWA_RAM, &galaxian_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x5060, 0x507f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x5080, 0x50ff, MWA_RAM },
	{ 0x6000, 0x6000, scramble_coin_counter_3_w },
	{ 0x6002, 0x6002, scramble_coin_counter_2_w },
	{ 0x6004, 0x6004, flip_screen_w },
	{ 0x6005, 0x6005, scramble_coin_counter_1_w },
	{ 0x6006, 0x6006, pisces_gfxbank_w },
	{ 0x6801, 0x6801, interrupt_enable_w },
	{ 0x7000, 0x7000, watchdog_reset_w },
	{ 0x8000, 0x8000, soundlatch_w },
	{ 0x9000, 0x9000, hotshock_sh_irqtrigger_w },
MEMORY_END


static MEMORY_READ_START( hunchbks_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x1210, 0x1213, ppi8255_1_r },
	{ 0x1400, 0x14ff, MRA_RAM },
	{ 0x1500, 0x1503, ppi8255_0_r },
	{ 0x1680, 0x1680, watchdog_reset_r },
	{ 0x1800, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, MRA_ROM },
	{ 0x3000, 0x3fff, hunchbks_mirror_r },
	{ 0x4000, 0x4fff, MRA_ROM },
	{ 0x5000, 0x5fff, hunchbks_mirror_r },
	{ 0x6000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, hunchbks_mirror_r },
MEMORY_END

static MEMORY_WRITE_START( hunchbks_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x1210, 0x1213, ppi8255_1_w },
	{ 0x1400, 0x143f, MWA_RAM, &galaxian_attributesram },
	{ 0x1440, 0x145f, MWA_RAM, &galaxian_spriteram, &galaxian_spriteram_size },
	{ 0x1460, 0x147f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x1480, 0x14ff, MWA_RAM },
	{ 0x1500, 0x1503, ppi8255_0_w },
	{ 0x1606, 0x1606, galaxian_flip_screen_x_w },
	{ 0x1607, 0x1607, galaxian_flip_screen_y_w },
	{ 0x1800, 0x1bff, MWA_RAM, &galaxian_videoram },
	{ 0x1c00, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2fff, MWA_ROM },
	{ 0x3000, 0x3fff, hunchbks_mirror_w },
	{ 0x4000, 0x4fff, MWA_ROM },
	{ 0x5000, 0x5fff, hunchbks_mirror_w },
	{ 0x6000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, hunchbks_mirror_w },
MEMORY_END


static PORT_READ_START( triplep_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x02, 0x02, triplep_pip_r },
	{ 0x03, 0x03, triplep_pap_r },
PORT_END

static PORT_WRITE_START( triplep_writeport )
	{ 0x00, 0x00, AY8910_write_port_0_w },
	{ 0x01, 0x01, AY8910_control_port_0_w },
PORT_END


static PORT_READ_START( hotshock_sound_readport )
	{ 0x20, 0x20, AY8910_read_port_0_r },
	{ 0x40, 0x40, AY8910_read_port_1_r },
PORT_END

static PORT_WRITE_START( hotshock_sound_writeport )
	{ 0x10, 0x10, AY8910_control_port_0_w },
	{ 0x20, 0x20, AY8910_write_port_0_w },
	{ 0x40, 0x40, AY8910_write_port_1_w },
	{ 0x80, 0x80, AY8910_control_port_1_w },
PORT_END


static PORT_READ_START( hunchbks_readport )
    { S2650_SENSE_PORT, S2650_SENSE_PORT, input_port_3_r },
PORT_END



INPUT_PORTS_START( scramble )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
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

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1  B 2/1  C 1/1" )
	PORT_DIPSETTING(    0x02, "A 1/2  B 1/1  C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3  B 3/1  C 1/3" )
	PORT_DIPSETTING(    0x06, "A 1/4  B 4/1  C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	/* protection bit */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	/* protection bit */
INPUT_PORTS_END

INPUT_PORTS_START( atlantis )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x0e, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 1/3  B 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/6  B 1/1" )
	PORT_DIPSETTING(    0x04, "A 1/99 B 1/99")
	/* all the other combos give 99 credits */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( theend )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )		/* output bits */
INPUT_PORTS_END

INPUT_PORTS_START( froggers )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* read - function unknown */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot2 - unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 2P shoot1 - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 2/1 C 2/1" )
	PORT_DIPSETTING(    0x04, "A 2/1 B 1/3 C 2/1" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1 C 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 1/6 C 1/1" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( amidars )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* 1P shoot2 - unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( triplep )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 1/2 B 1/1 C 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/3 B 3/1 C 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 2/1 C 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/4 B 4/1 C 1/4" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ckongs )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	/* the coinage dip switch is spread across bits 0/1 of port 1 and bit 3 of port 2. */
	/* To handle that, we swap bits 0/1 of port 1 and bits 1/2 of port 2 - this is handled */
	/* by ckongs_input_port_N_r() */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	/* the coinage dip switch is spread across bits 0/1 of port 1 and bit 3 of port 2. */
	/* To handle that, we swap bits 0/1 of port 1 and bits 1/2 of port 2 - this is handled */
	/* by ckongs_input_port_N_r() */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
INPUT_PORTS_END

INPUT_PORTS_START( mars )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP     | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT   | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY | IPF_PLAYER2 )  /* this also control cocktail mode */
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP     | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )

	PORT_START	/* IN3 */
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( devilfsh )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x01, "15000" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( newsin7 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, " A 1C/1C  B 2C/1C" )
	PORT_DIPSETTING(    0x01, " A 1C/3C  B 3C/1C" )
	PORT_DIPSETTING(    0x02, " A 1C/2C  B 1C/1C" )
	PORT_DIPSETTING(    0x00, " A 1C/4C  B 4C/1C" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )  /* difficulty? */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hotshock )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* pressing this disables the coins */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START	/* IN3 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Language" )
	PORT_DIPSETTING(    0x04, "English" )
	PORT_DIPSETTING(    0x00, "Italian" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "75000" )
	PORT_DIPSETTING(    0x08, "150000" )
	PORT_DIPSETTING(    0x10, "200000" )
	PORT_DIPSETTING(    0x18, "None" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( hunchbks )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/5" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x06, "80000" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */

    PORT_START /* Sense */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( cavelon )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* force UR controls in CK mode? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection check? */
INPUT_PORTS_END


static struct GfxLayout devilfsh_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 2*256*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout devilfsh_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 2*64*16*16 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout newsin7_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 0, 2*256*8*8, 2*2*256*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout newsin7_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 2*64*16*16, 2*2*64*16*16 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo devilfsh_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &devilfsh_charlayout,   0, 8 },
	{ REGION_GFX1, 0x0800, &devilfsh_spritelayout, 0, 8 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo newsin7_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &newsin7_charlayout,   0, 4 },
	{ REGION_GFX1, 0x0800, &newsin7_spritelayout, 0, 4 },
	{ -1 } /* end of array */
};



static struct AY8910interface triplep_ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 MHz */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


#define hotshock_sound_readmem	scobra_sound_readmem
#define hotshock_sound_writemem	scobra_sound_writemem

#define hotshock_ay8910_interface	scobra_ay8910_interface


#define DRIVER_2CPU(NAME, GFXDECODE, MAINMEM, SOUND, VHSTART, COLORPROM)	\
static struct MachineDriver machine_driver_##NAME =		\
{																\
	/* basic machine hardware */								\
	{															\
		{														\
			CPU_Z80,											\
			18432000/6,	/* 3.072 MHz */							\
			MAINMEM##_readmem,MAINMEM##_writemem,0,0,			\
			nmi_interrupt,1										\
		},														\
		{														\
			CPU_Z80 | CPU_AUDIO_CPU,							\
			14318000/8,	/* 1.78975 MHz */						\
			SOUND##_sound_readmem,SOUND##_sound_writemem,SOUND##_sound_readport,SOUND##_sound_writeport,	\
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */								\
		}														\
	},															\
	16000.0/132/2, 2500,	/* frames per second, vblank duration */	\
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */				\
	scramble_init_machine,										\
																\
	/* video hardware */										\
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },					\
	GFXDECODE##_gfxdecodeinfo,									\
	32+64+2+2,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 2 for background */	\
	COLORPROM##_vh_convert_color_prom,							\
																\
	VIDEO_TYPE_RASTER,											\
	0,															\
	VHSTART##_vh_start,											\
	0,															\
	galaxian_vh_screenrefresh,									\
																\
	/* sound hardware */										\
	0,0,0,0,													\
	{															\
		{														\
			SOUND_AY8910,										\
			&SOUND##_ay8910_interface							\
		}														\
	}															\
}

/*			NAME      GFXDECODE  MAINMEM   SOUND     VHSTART   CONV_COLORPROM */
DRIVER_2CPU(scramble, galaxian,  scramble, scobra,   scramble, scramble);
DRIVER_2CPU(theend,   galaxian,  scramble, scobra,   theend,   scramble);
DRIVER_2CPU(froggers, galaxian,  scramble, frogger,  froggers, frogger);
DRIVER_2CPU(mars,     galaxian,  mars,     scobra,   scramble, scramble);
DRIVER_2CPU(devilfsh, devilfsh,  mars,     scobra,   scramble, scramble);
DRIVER_2CPU(newsin7,  newsin7,   newsin7,  scobra,   scramble, scramble);
DRIVER_2CPU(ckongs,   galaxian,  ckongs,   scobra,   ckongs,   scramble);
DRIVER_2CPU(hotshock, galaxian,  hotshock, hotshock, pisces,   scramble);
DRIVER_2CPU(cavelon,  galaxian,  scramble, scobra,   ckongs,   scramble);

/* Triple Punch and Mariner are different - only one CPU, one 8910 */
static struct MachineDriver machine_driver_triplep =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			scramble_readmem,scramble_writemem,triplep_readport,triplep_writeport,
			nmi_interrupt,1
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
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
			&triplep_ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_mariner =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 MHz */
			scramble_readmem,scramble_writemem,triplep_readport,triplep_writeport,
			nmi_interrupt,1
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	scramble_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	galaxian_gfxdecodeinfo,
	32+64+2+16,8*4,	/* 32 for characters, 64 for stars, 2 for bullets, 16 for background */
	mariner_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	mariner_vh_start,
	0,
	galaxian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&triplep_ay8910_interface
		}
	}
};

/**********************************************************/
/* hunchbks is *very* different, as it uses an S2650 CPU */
/*  epoxied in a plastic case labelled Century Playpack   */
/**********************************************************/

static struct MachineDriver machine_driver_hunchbks =
{
	/* basic machine hardware */
	{
		{
			CPU_S2650,
			18432000/6,
			hunchbks_readmem,hunchbks_writemem,hunchbks_readport,0,
			hunchbks_vh_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,	/* 1.78975 MHz */
			scobra_sound_readmem,scobra_sound_writemem,scobra_sound_readport,scobra_sound_writeport,
			ignore_interrupt,1
		}
	},
	16000.0/132/2, 2500,	/* frames per second, vblank duration */
	1,
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

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scramble )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2d.k",         0x0000, 0x0800, 0xea35ccaa )
	ROM_LOAD( "2e.k",         0x0800, 0x0800, 0xe7bba1b3 )
	ROM_LOAD( "2f.k",         0x1000, 0x0800, 0x12d7fc3e )
	ROM_LOAD( "2h.k",         0x1800, 0x0800, 0xb59360eb )
	ROM_LOAD( "2j.k",         0x2000, 0x0800, 0x4919a91c )
	ROM_LOAD( "2l.k",         0x2800, 0x0800, 0x26a4547b )
	ROM_LOAD( "2m.k",         0x3000, 0x0800, 0x0bb49470 )
	ROM_LOAD( "2p.k",         0x3800, 0x0800, 0x6a5740e5 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xbcd297f0 )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0xde7912da )
	ROM_LOAD( "5e",           0x1000, 0x0800, 0xba2fa933 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f.k",         0x0000, 0x0800, 0x4708845b )
	ROM_LOAD( "5h.k",         0x0800, 0x0800, 0x11fd2887 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( scrambls )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2d",           0x0000, 0x0800, 0xb89207a1 )
	ROM_LOAD( "2e",           0x0800, 0x0800, 0xe9b4b9eb )
	ROM_LOAD( "2f",           0x1000, 0x0800, 0xa1f14f4c )
	ROM_LOAD( "2h",           0x1800, 0x0800, 0x591bc0d9 )
	ROM_LOAD( "2j",           0x2000, 0x0800, 0x22f11b6b )
	ROM_LOAD( "2l",           0x2800, 0x0800, 0x705ffe49 )
	ROM_LOAD( "2m",           0x3000, 0x0800, 0xea26c35c )
	ROM_LOAD( "2p",           0x3800, 0x0800, 0x94d8f5e3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xbcd297f0 )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0xde7912da )
	ROM_LOAD( "5e",           0x1000, 0x0800, 0xba2fa933 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x5f30311a )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0x516e029e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( atlantis )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c",           0x0000, 0x0800, 0x0e485b9a )
	ROM_LOAD( "2e",           0x0800, 0x0800, 0xc1640513 )
	ROM_LOAD( "2f",           0x1000, 0x0800, 0xeec265ee )
	ROM_LOAD( "2h",           0x1800, 0x0800, 0xa5d2e442 )
	ROM_LOAD( "2j",           0x2000, 0x0800, 0x45f7cf34 )
	ROM_LOAD( "2l",           0x2800, 0x0800, 0xf335b96b )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xbcd297f0 )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0xde7912da )
	ROM_LOAD( "5e",           0x1000, 0x0800, 0xba2fa933 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f",           0x0000, 0x0800, 0x57f9c6b9 )
	ROM_LOAD( "5h",           0x0800, 0x0800, 0xe989f325 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( atlants2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "rom1",         0x0000, 0x0800, 0xad348089 )
	ROM_LOAD( "rom2",         0x0800, 0x0800, 0xcaa705d1 )
	ROM_LOAD( "rom3",         0x1000, 0x0800, 0xe420641d )
	ROM_LOAD( "rom4",         0x1800, 0x0800, 0x04792d90 )
	ROM_LOAD( "rom5",         0x2000, 0x0800, 0x6eaf510d )
	ROM_LOAD( "rom6",         0x2800, 0x0800, 0xb297bd4b )
	ROM_LOAD( "rom7",         0x3000, 0x0800, 0xa50bf8d5 )
	ROM_LOAD( "rom8",         0x3800, 0x0800, 0xd2c5c984 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "5c",           0x0000, 0x0800, 0xbcd297f0 )
	ROM_LOAD( "5d",           0x0800, 0x0800, 0xde7912da )
	ROM_LOAD( "5e",           0x1000, 0x0800, 0xba2fa933 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom9",         0x0000, 0x0800, 0x55cd5acd )
	ROM_LOAD( "rom10",        0x0800, 0x0800, 0x72e773b8 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( theend )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic13_1t.bin",  0x0000, 0x0800, 0x93e555ba )
	ROM_LOAD( "ic14_2t.bin",  0x0800, 0x0800, 0x2de7ad27 )
	ROM_LOAD( "ic15_3t.bin",  0x1000, 0x0800, 0x035f750b )
	ROM_LOAD( "ic16_4t.bin",  0x1800, 0x0800, 0x61286b5c )
	ROM_LOAD( "ic17_5t.bin",  0x2000, 0x0800, 0x434a8f68 )
	ROM_LOAD( "ic18_6t.bin",  0x2800, 0x0800, 0xdc4cc786 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic56_1.bin",   0x0000, 0x0800, 0x7a141f29 )
	ROM_LOAD( "ic55_2.bin",   0x0800, 0x0800, 0x218497c1 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic30_2c.bin",  0x0000, 0x0800, 0x68ccf7bf )
	ROM_LOAD( "ic31_1c.bin",  0x0800, 0x0800, 0x4a48c999 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, 0x24652bc4 )
ROM_END

ROM_START( theends )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic13",         0x0000, 0x0800, 0x90e5ab14 )
	ROM_LOAD( "ic14",         0x0800, 0x0800, 0x950f0a07 )
	ROM_LOAD( "ic15",         0x1000, 0x0800, 0x6786bcf5 )
	ROM_LOAD( "ic16",         0x1800, 0x0800, 0x380a0017 )
	ROM_LOAD( "ic17",         0x2000, 0x0800, 0xaf067b7f )
	ROM_LOAD( "ic18",         0x2800, 0x0800, 0xa0411b93 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic56",         0x0000, 0x0800, 0x3b2c2f70 )
	ROM_LOAD( "ic55",         0x0800, 0x0800, 0xe0429e50 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic30",         0x0000, 0x0800, 0x527fd384 )
	ROM_LOAD( "ic31",         0x0800, 0x0800, 0xaf6d09b6 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6331-1j.86",   0x0000, 0x0020, 0x24652bc4 )
ROM_END

ROM_START( froggers )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "vid_d2.bin",   0x0000, 0x0800, 0xc103066e )
	ROM_LOAD( "vid_e2.bin",   0x0800, 0x0800, 0xf08bc094 )
	ROM_LOAD( "vid_f2.bin",   0x1000, 0x0800, 0x637a2ff8 )
	ROM_LOAD( "vid_h2.bin",   0x1800, 0x0800, 0x04c027a5 )
	ROM_LOAD( "vid_j2.bin",   0x2000, 0x0800, 0xfbdfbe74 )
	ROM_LOAD( "vid_l2.bin",   0x2800, 0x0800, 0x8a4389e1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "frogger.608",  0x0000, 0x0800, 0xe8ab0256 )
	ROM_LOAD( "frogger.609",  0x0800, 0x0800, 0x7380a48f )
	ROM_LOAD( "frogger.610",  0x1000, 0x0800, 0x31d7eb27 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-1036.1k",  0x0000, 0x0800, 0x658745f8 )
	ROM_LOAD( "frogger.607",  0x0800, 0x0800, 0x05f7d883 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "vid_e6.bin",   0x0000, 0x0020, 0x0b878b54 )
ROM_END

ROM_START( amidars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "am2d",         0x0000, 0x0800, 0x24b79547 )
	ROM_LOAD( "am2e",         0x0800, 0x0800, 0x4c64161e )
	ROM_LOAD( "am2f",         0x1000, 0x0800, 0xb3987a72 )
	ROM_LOAD( "am2h",         0x1800, 0x0800, 0x29873461 )
	ROM_LOAD( "am2j",         0x2000, 0x0800, 0x0fdd54d8 )
	ROM_LOAD( "am2l",         0x2800, 0x0800, 0x5382f7ed )
	ROM_LOAD( "am2m",         0x3000, 0x0800, 0x1d7109e9 )
	ROM_LOAD( "am2p",         0x3800, 0x0800, 0xc9163ac6 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "amidarus.5c",  0x0000, 0x1000, 0x8ca7b750 )
	ROM_LOAD( "amidarus.5d",  0x1000, 0x1000, 0x9b5bdc0a )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2716.a6",      0x0000, 0x0800, 0x2082ad0a )   /* Same graphics ROMs as Amigo */
	ROM_LOAD( "2716.a5",      0x0800, 0x0800, 0x3029f94f )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "amidar.clr",   0x0000, 0x0020, 0xf940dcc3 )
ROM_END

ROM_START( triplep )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "triplep.2g",   0x0000, 0x1000, 0xc583a93d )
	ROM_LOAD( "triplep.2h",   0x1000, 0x1000, 0xc03ddc49 )
	ROM_LOAD( "triplep.2k",   0x2000, 0x1000, 0xe83ca6b5 )
	ROM_LOAD( "triplep.2l",   0x3000, 0x1000, 0x982cc3b9 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "triplep.5f",   0x0000, 0x0800, 0xd51cbd6f )
	ROM_LOAD( "triplep.5h",   0x0800, 0x0800, 0xf21c0059 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "tripprom.6e",  0x0000, 0x0020, 0x624f75df )
ROM_END

ROM_START( knockout )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "knockout.2h",  0x0000, 0x1000, 0xeaaa848e )
	ROM_LOAD( "knockout.2k",  0x1000, 0x1000, 0xbc26d2c0 )
	ROM_LOAD( "knockout.2l",  0x2000, 0x1000, 0x02025c10 )
	ROM_LOAD( "knockout.2m",  0x3000, 0x1000, 0xe9abc42b )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "triplep.5f",   0x0000, 0x0800, 0xd51cbd6f )
	ROM_LOAD( "triplep.5h",   0x0800, 0x0800, 0xf21c0059 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "tripprom.6e",  0x0000, 0x0020, 0x624f75df )
ROM_END

ROM_START( mariner )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "tp1.2h",       0x0000, 0x1000, 0xdac1dfd0 )
	ROM_LOAD( "tm2.2k",       0x1000, 0x1000, 0xefe7ca28 )
	ROM_LOAD( "tm3.2l",       0x2000, 0x1000, 0x027881a6 )
	ROM_LOAD( "tm4.2m",       0x3000, 0x1000, 0xa0fde7dc )
	ROM_LOAD( "tm5.2p",       0x6000, 0x0800, 0xd7ebcb8e )
	ROM_CONTINUE(             0x5800, 0x0800             )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm8.5f",       0x0000, 0x1000, 0x70ae611f )
	ROM_LOAD( "tm9.5h",       0x1000, 0x1000, 0x8e4e999e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t4.6e",        0x0000, 0x0020, 0xca42b6dd )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "t6.6p",        0x0000, 0x0100, 0xad208ccc )	/* background color prom */

	ROM_REGION( 0x0020, REGION_USER2, 0 )
	ROM_LOAD( "t5.7p",        0x0000, 0x0020, 0x1bd88cff )	/* char banking and star placement */
ROM_END

ROM_START( 800fath )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "tu1.2h",       0x0000, 0x1000, 0x5dd3d42f )
	ROM_LOAD( "tm2.2k",       0x1000, 0x1000, 0xefe7ca28 )
	ROM_LOAD( "tm3.2l",       0x2000, 0x1000, 0x027881a6 )
	ROM_LOAD( "tm4.2m",       0x3000, 0x1000, 0xa0fde7dc )
	ROM_LOAD( "tu5.2p",       0x6000, 0x0800, 0xf864a8a6 )
	ROM_CONTINUE(             0x5800, 0x0800             )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tm8.5f",       0x0000, 0x1000, 0x70ae611f )
	ROM_LOAD( "tm9.5h",       0x1000, 0x1000, 0x8e4e999e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "t4.6e",        0x0000, 0x0020, 0xca42b6dd )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "t6.6p",        0x0000, 0x0100, 0xad208ccc )	/* background color prom */

	ROM_REGION( 0x0020, REGION_USER2, 0 )
	ROM_LOAD( "t5.7p",        0x0000, 0x0020, 0x1bd88cff )	/* char banking and star placement */
ROM_END

ROM_START( ckongs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "vid_2c.bin",   0x0000, 0x1000, 0x49a8c234 )
	ROM_LOAD( "vid_2e.bin",   0x1000, 0x1000, 0xf1b667f1 )
	ROM_LOAD( "vid_2f.bin",   0x2000, 0x1000, 0xb194b75d )
	ROM_LOAD( "vid_2h.bin",   0x3000, 0x1000, 0x2052ba8a )
	ROM_LOAD( "vid_2j.bin",   0x4000, 0x1000, 0xb377afd0 )
	ROM_LOAD( "vid_2l.bin",   0x5000, 0x1000, 0xfe65e691 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "turt_snd.5c",  0x0000, 0x1000, 0xf0c30f9a )
	ROM_LOAD( "snd_5d.bin",   0x1000, 0x1000, 0x892c9547 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vid_5f.bin",   0x0000, 0x1000, 0x7866d2cb )
	ROM_LOAD( "vid_5h.bin",   0x1000, 0x1000, 0x7311a101 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "vid_6e.bin",   0x0000, 0x0020, 0x5039af97 )
ROM_END

ROM_START( mars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u26.3",        0x0000, 0x0800, 0x2f88892c )
	ROM_LOAD( "u56.4",        0x0800, 0x0800, 0x9e6bcbf7 )
	ROM_LOAD( "u69.5",        0x1000, 0x0800, 0xdf496e6e )
	ROM_LOAD( "u98.6",        0x1800, 0x0800, 0x75f274bb )
	ROM_LOAD( "u114.7",       0x2000, 0x0800, 0x497fd8d0 )
	ROM_LOAD( "u133.8",       0x2800, 0x0800, 0x3d4cd59f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "u39.9",        0x0000, 0x0800, 0xbb5968b9 )
	ROM_LOAD( "u51.10",       0x0800, 0x0800, 0x75fd7720 )
	ROM_LOAD( "u78.11",       0x1000, 0x0800, 0x72a492da )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u72.1",        0x0000, 0x0800, 0x279789d0 )
	ROM_LOAD( "u101.2",       0x0800, 0x0800, 0xc5dc627f )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( devilfsh )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "u26.1",        0x0000, 0x0800, 0xec047d71 )
	ROM_LOAD( "u56.2",        0x0800, 0x0800, 0x0138ade9 )
	ROM_LOAD( "u69.3",        0x1000, 0x0800, 0x5dd0b3fc )
	ROM_LOAD( "u98.4",        0x1800, 0x0800, 0xded0b745 )
	ROM_LOAD( "u114.5",       0x2000, 0x0800, 0x5fd40176 )
	ROM_LOAD( "u133.6",       0x2800, 0x0800, 0x03538336 )
	ROM_LOAD( "u143.7",       0x3000, 0x0800, 0x64676081 )
	ROM_LOAD( "u163.8",       0x3800, 0x0800, 0xbc3d6770 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "u39.9",        0x0000, 0x0800, 0x09987e2e )
	ROM_LOAD( "u51.10",       0x0800, 0x0800, 0x1e2b1471 )
	ROM_LOAD( "u78.11",       0x1000, 0x0800, 0x45279aaa )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u72.12",       0x0000, 0x1000, 0x5406508e )
	ROM_LOAD( "u101.13",      0x1000, 0x1000, 0x8c4018b6 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( newsin7 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "newsin.1",     0x0000, 0x1000, 0xe6c23fe0 )
	ROM_LOAD( "newsin.2",     0x1000, 0x1000, 0x3d477b5f )
	ROM_LOAD( "newsin.3",     0x2000, 0x1000, 0x7dfa9af0 )
	ROM_LOAD( "newsin.4",     0x3000, 0x1000, 0xd1b0ba19 )
	ROM_LOAD( "newsin.5",     0xa000, 0x1000, 0x06275d59 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "newsin.13",    0x0000, 0x0800, 0xd88489a2 )
	ROM_LOAD( "newsin.12",    0x0800, 0x0800, 0xb154a7af )
	ROM_LOAD( "newsin.11",    0x1000, 0x0800, 0x7ade709b )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "newsin.7",     0x2000, 0x1000, 0x6bc5d64f )
	ROM_LOAD( "newsin.8",     0x1000, 0x1000, 0x0c5b895a )
	ROM_LOAD( "newsin.9",     0x0000, 0x1000, 0x6b87adff )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "newsin.6",     0x0000, 0x0020, 0x5cf2cd8d )
ROM_END

ROM_START( hotshock )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "hotshock.l10", 0x0000, 0x1000, 0x401078f7 )
	ROM_LOAD( "hotshock.l9",  0x1000, 0x1000, 0xaf76c237 )
	ROM_LOAD( "hotshock.l8",  0x2000, 0x1000, 0x30486031 )
	ROM_LOAD( "hotshock.l7",  0x3000, 0x1000, 0x5bde9312 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "hotshock.b3",  0x0000, 0x1000, 0x0092f0e2 )
	ROM_LOAD( "hotshock.b4",  0x1000, 0x1000, 0xc2135a44 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hotshock.h4",  0x0000, 0x1000, 0x60bdaea9 )
	ROM_LOAD( "hotshock.h5",  0x1000, 0x1000, 0x4ef17453 )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.6e",    0x0000, 0x0020, 0x4e3caeab )
ROM_END

ROM_START( hunchbks )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2c_hb01.bin",  0x0000, 0x0800, 0x8bebd834 )
	ROM_LOAD( "2e_hb02.bin",  0x0800, 0x0800, 0x07de4229 )
	ROM_LOAD( "2f_hb03.bin",  0x2000, 0x0800, 0xb75a0dfc )
	ROM_LOAD( "2h_hb04.bin",  0x2800, 0x0800, 0xf3206264 )
	ROM_LOAD( "2j_hb05.bin",  0x4000, 0x0800, 0x1bb78728 )
	ROM_LOAD( "2l_hb06.bin",  0x4800, 0x0800, 0xf25ed680 )
	ROM_LOAD( "2m_hb07.bin",  0x6000, 0x0800, 0xc72e0e17 )
	ROM_LOAD( "2p_hb08.bin",  0x6800, 0x0800, 0x412087b0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "11d_snd.bin",  0x0000, 0x0800, 0x88226086 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5f_hb09.bin",  0x0000, 0x0800, 0xdb489c3d )
	ROM_LOAD( "5h_hb10.bin",  0x0800, 0x0800, 0x3977650e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "6e_prom.bin",  0x0000, 0x0020, 0x01004d3f )
ROM_END

ROM_START( cavelon )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k + 16K banked for code */
	ROM_LOAD( "2.bin",		 0x00000, 0x2000, 0xa3b353ac )
	ROM_LOAD( "1.bin",		 0x02000, 0x2000, 0x3f62efd6 )
	ROM_RELOAD(				 0x12000, 0x2000)
	ROM_LOAD( "3.bin",		 0x10000, 0x2000, 0x39d74e4e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1c_snd.bin",	  0x0000, 0x0800, 0xf58dcf55 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "h.bin",		  0x0000, 0x1000, 0xd44fcd6f )
	ROM_LOAD( "k.bin",		  0x1000, 0x1000, 0x59bc7f9e )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "cavelon.clr",  0x0000, 0x0020, 0xd133356b )
ROM_END


GAME( 1981, scramble, 0,        scramble, scramble, scramble, ROT90, "Konami", "Scramble" )
GAME( 1981, scrambls, scramble, scramble, scramble, scrambls, ROT90, "[Konami] (Stern license)", "Scramble (Stern)" )
GAME( 1981, atlantis, 0,        scramble, atlantis, scobra,   ROT90, "Comsoft", "Battle of Atlantis (set 1)" )
GAME( 1981, atlants2, atlantis, scramble, atlantis, scobra,   ROT90, "Comsoft", "Battle of Atlantis (set 2)" )
GAME( 1980, theend,   0,        theend,   theend,   theend,   ROT90, "Konami", "The End" )
GAME( 1980, theends,  theend,   theend,   theend,   theend,   ROT90, "[Konami] (Stern license)", "The End (Stern)" )
GAME( 1981, froggers, frogger,  froggers, froggers, froggers, ROT90, "bootleg", "Frog" )
GAME( 1982, amidars,  amidar,   scramble, amidars,  scobra,   ROT90, "Konami", "Amidar (Scramble hardware)" )
GAME( 1982, triplep,  0,        triplep,  triplep,  scobra,   ROT90, "KKI", "Triple Punch" )
GAME( 1982, knockout, triplep,  triplep,  triplep,  scobra,   ROT90, "KKK", "Knock Out!!" )
GAME( 1981, mariner,  0,        mariner,  scramble, mariner,  ROT90, "Amenip", "Mariner" )
GAME( 1981, 800fath,  mariner,  mariner,  scramble, mariner,  ROT90, "Amenip (US Billiards Inc. license)", "800 Fathoms" )
GAME( 1981, ckongs,   ckong,    ckongs,   ckongs,   ckongs,   ROT90, "bootleg", "Crazy Kong (Scramble hardware)" )
GAME( 1981, mars,     0,        mars,     mars,     mars,     ROT90, "Artic", "Mars" )
GAME( 1982, devilfsh, 0,        devilfsh, devilfsh, mars,     ROT90, "Artic", "Devil Fish" )
GAMEX(1983, newsin7,  0,        newsin7,  newsin7,  mars,     ROT90, "ATW USA, Inc.", "New Sinbad 7", GAME_IMPERFECT_COLORS )
GAME( 1982, hotshock, 0,        hotshock, hotshock, hotshock, ROT90, "E.G. Felaco", "Hot Shocker" )
GAME( 1983, hunchbks, hunchbak, hunchbks, hunchbks, scobra,   ROT90, "Century", "Hunchback (Scramble hardware)" )
GAME( 1983, cavelon,  0,        cavelon,  cavelon,  cavelon,  ROT90, "Jetsoft", "Cavelon" )
