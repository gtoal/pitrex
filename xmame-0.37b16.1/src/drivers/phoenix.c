/***************************************************************************

Phoenix hardware games

driver by Richard Davies

Note:
   pleiads is using another sound driver, sndhrdw\pleiads.c
 Andrew Scott (ascott@utkux.utcc.utk.edu)


To Do:


Survival:

- Protection.  There is a 14 pin part connected to the 8910 Port B D0 labeled DL57S22.
  There is a loop at $2002 that reads the player controls -- the game sits in this
  loop as long as Port B changes.  Also, Port B seems to invert the input bits, and
  the game checks for this at $2f32.  The game also uses the RIM instruction a lot,
  that's purpose is unclear, as the result doesn't seem to be used (even when it's
  stored, the result is never read again.)  I would think that this advances the
  protection chip somehow, but isn't RIM a read only operation?

- Check background visibile area.  When the background scrolls up, it
  currently shows below the top and bottom of the border of the play area.


Pleiads:

- Palette banking.  Controlled by 3 custom chips marked T-X, T-Y and T-Z.
  These chips are reponsible for the protection as well.

***************************************************************************/

#include "driver.h"



READ_HANDLER( phoenix_videoram_r );
WRITE_HANDLER( phoenix_videoram_w );
WRITE_HANDLER( phoenix_videoreg_w );
WRITE_HANDLER( pleiads_videoreg_w );
WRITE_HANDLER( phoenix_scroll_w );
READ_HANDLER( phoenix_input_port_0_r );
READ_HANDLER( pleiads_input_port_0_r );
READ_HANDLER( survival_input_port_0_r );
READ_HANDLER( survival_protection_r );
void phoenix_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void pleiads_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int  phoenix_vh_start(void);
void phoenix_vh_stop(void);
void phoenix_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

WRITE_HANDLER( phoenix_sound_control_a_w );
WRITE_HANDLER( phoenix_sound_control_b_w );
int phoenix_sh_start(const struct MachineSound *msound);
void phoenix_sh_stop(void);
void phoenix_sh_update(void);

WRITE_HANDLER( pleiads_sound_control_a_w );
WRITE_HANDLER( pleiads_sound_control_b_w );
int pleiads_sh_start(const struct MachineSound *msound);
void pleiads_sh_stop(void);
void pleiads_sh_update(void);


static MEMORY_READ_START( phoenix_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_r },		/* 2 pages selected by bit 0 of the video register */
	{ 0x7000, 0x73ff, phoenix_input_port_0_r }, /* IN0 or IN1 */
	{ 0x7800, 0x7bff, input_port_2_r }, 		/* DSW */
MEMORY_END

static MEMORY_READ_START( pleiads_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_r },		/* 2 pages selected by bit 0 of the video register */
	{ 0x7000, 0x73ff, pleiads_input_port_0_r }, /* IN0 or IN1 + protection */
	{ 0x7800, 0x7bff, input_port_2_r }, 		/* DSW */
MEMORY_END

static MEMORY_READ_START( survival_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_r },		/* 2 pages selected by bit 0 of the video register */
	{ 0x6900, 0x69ff, AY8910_read_port_0_r },
	{ 0x7000, 0x73ff, survival_input_port_0_r },/* IN0 or IN1 */
	{ 0x7800, 0x7bff, input_port_2_r },			/* DSW */
MEMORY_END


static MEMORY_WRITE_START( phoenix_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_w },		/* 2 pages selected by bit 0 of the video register */
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5800, 0x5bff, phoenix_scroll_w },
	{ 0x6000, 0x63ff, phoenix_sound_control_a_w },
	{ 0x6800, 0x6bff, phoenix_sound_control_b_w },
MEMORY_END

static MEMORY_WRITE_START( pleiads_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_w },		/* 2 pages selected by bit 0 of the video register */
	{ 0x5000, 0x53ff, pleiads_videoreg_w },
	{ 0x5800, 0x5bff, phoenix_scroll_w },
	{ 0x6000, 0x63ff, pleiads_sound_control_a_w },
	{ 0x6800, 0x6bff, pleiads_sound_control_b_w },
MEMORY_END

static MEMORY_WRITE_START( survival_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, phoenix_videoram_w },		/* 2 pages selected by bit 0 of the video register */
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5800, 0x5bff, phoenix_scroll_w },
	{ 0x6800, 0x68ff, AY8910_control_port_0_w },
	{ 0x6900, 0x69ff, AY8910_write_port_0_w },
MEMORY_END



INPUT_PORTS_START( phoenix )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( phoenixa )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	/* Coinage is backwards from phoenix (Amstar) */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( phoenixt )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( phoenix3 )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( pleiads )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL )	   /* Protection. See 0x0552 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "3K 30K" )
	PORT_DIPSETTING(	0x04, "4K 40K" )
	PORT_DIPSETTING(	0x08, "5K 50K" )
	PORT_DIPSETTING(	0x0c, "6K 60K" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( pleiadce )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL )	   /* Protection. See 0x0552 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL  )

	PORT_START		/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "7K 70K" )
	PORT_DIPSETTING(	0x04, "8K 80K" )
	PORT_DIPSETTING(	0x08, "9K 90K" )
  /*PORT_DIPSETTING(	0x0c, "INVALID" )   Sets bonus to A000 */
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( survival )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL  )

    PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x03, "2" )
	PORT_DIPSETTING(	0x02, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x0c, "25000" )
	PORT_DIPSETTING(	0x08, "35000" )
	PORT_DIPSETTING(	0x04, "45000" )
	PORT_DIPSETTING(	0x00, "55000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x60, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for non-memory mapped dip switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 256*8*8, 0 }, /* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 }, /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo phoenix_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	  0, 16 },
	{ REGION_GFX2, 0, &charlayout, 16*4, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo pleiads_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	  0, 32 },
	{ REGION_GFX2, 0, &charlayout, 32*4, 32 },
	{ -1 } /* end of array */
};


static struct TMS36XXinterface phoenix_tms36xx_interface =
{
	1,
	{ 50 }, 		/* mixing levels */
	{ MM6221AA },	/* TMS36xx subtype(s) */
	{ 372  },		/* base frequency */
	{ {0.50,0,0,1.05,0,0} }, /* decay times of voices */
    { 0.21 },       /* tune speed (time between beats) */
};

static struct CustomSound_interface phoenix_custom_interface =
{
	phoenix_sh_start,
	phoenix_sh_stop,
	phoenix_sh_update
};

static struct TMS36XXinterface pleiads_tms36xx_interface =
{
	1,
	{ 75		},	/* mixing levels */
	{ TMS3615	},	/* TMS36xx subtype(s) */
	{ 247		},	/* base frequencies (one octave below A) */
	/*
	 * Decay times of the voices; NOTE: it's unknown if
	 * the the TMS3615 mixes more than one voice internally.
	 * A wav taken from Pop Flamer sounds like there
	 * are at least no 'odd' harmonics (5 1/3' and 2 2/3')
     */
	{ {0.33,0.33,0,0.33,0,0.33} }
};

static struct CustomSound_interface pleiads_custom_interface =
{
	pleiads_sh_start,
	pleiads_sh_stop,
	pleiads_sh_update
};

static struct AY8910interface survival_ay8910_interface =
{
	1,	/* 1 chip */
	11000000/4,
	{ 50 },
	{ 0 },
	{ survival_protection_r },
	{ 0 },
	{ 0 }
};



#define MACHINE_DRIVER(NAME, PENS)									\
																	\
static struct MachineDriver machine_driver_##NAME = 				\
{																	\
	/* basic machine hardware */									\
	{																\
		{															\
			CPU_8085A,												\
			11000000/4,	/* 2.75 MHz */								\
			NAME##_readmem,NAME##_writemem,0,0,						\
			ignore_interrupt,1										\
		}															\
	},																\
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */	\
	1,	/* single CPU, no need for interleaving */					\
	0,																\
																	\
	/* video hardware */											\
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 26*8-1 },						\
	NAME##_gfxdecodeinfo,											\
	256,PENS*4+PENS*4,												\
	NAME##_vh_convert_color_prom,									\
																	\
	VIDEO_TYPE_RASTER,												\
	0,																\
	phoenix_vh_start,												\
	phoenix_vh_stop,												\
	phoenix_vh_screenrefresh,										\
																	\
	/* sound hardware */											\
	0,0,0,0,														\
	{																\
		{															\
			SOUND_TMS36XX,											\
			&NAME##_tms36xx_interface								\
		},															\
		{															\
			SOUND_CUSTOM,											\
			&NAME##_custom_interface								\
		}															\
	}																\
};

MACHINE_DRIVER(phoenix,16)
MACHINE_DRIVER(pleiads,32)


/* Same as Phoenix, but uses an AY8910 and an extra visible line (column) */

static struct MachineDriver machine_driver_survival =
{
	/* basic machine hardware */
	{
		{
			CPU_8085A,
			11000000/4,	/* 2.75 MHz */
			survival_readmem,survival_writemem,0,0,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 26*8-1 },
	phoenix_gfxdecodeinfo,
	256,16*4+16*4,
	phoenix_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	phoenix_vh_start,
	phoenix_vh_stop,
	phoenix_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&survival_ay8910_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( phoenix )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic45",         0x0000, 0x0800, 0x9f68086b )
	ROM_LOAD( "ic46",         0x0800, 0x0800, 0x273a4a82 )
	ROM_LOAD( "ic47",         0x1000, 0x0800, 0x3d4284b9 )
	ROM_LOAD( "ic48",         0x1800, 0x0800, 0xcb5d9915 )
	ROM_LOAD( "ic49",         0x2000, 0x0800, 0xa105e4e7 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "ic52",         0x3800, 0x0800, 0xaff8e9c5 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39",         0x0000, 0x0800, 0x53413e8f )
	ROM_LOAD( "ic40",         0x0800, 0x0800, 0x0be2ba91 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )  /* palette high bits */
ROM_END

ROM_START( phoenixa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic45.k1",      0x0000, 0x0800, 0xc7a9b499 )
	ROM_LOAD( "ic46.k2",      0x0800, 0x0800, 0xd0e6ae1b )
	ROM_LOAD( "ic47.k3",      0x1000, 0x0800, 0x64bf463a )
	ROM_LOAD( "ic48.k4",      0x1800, 0x0800, 0x1b20fe62 )
	ROM_LOAD( "phoenixc.49",  0x2000, 0x0800, 0x1a1ce0d0 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "ic52",         0x3800, 0x0800, 0xaff8e9c5 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "phoenixc.39",  0x0000, 0x0800, 0xbb0525ed )
	ROM_LOAD( "phoenixc.40",  0x0800, 0x0800, 0x4178aa4f )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )  /* palette high bits */
ROM_END

ROM_START( phoenixt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "phoenix.45",   0x0000, 0x0800, 0x5b8c55a8 )
	ROM_LOAD( "phoenix.46",   0x0800, 0x0800, 0xdbc942fa )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, 0xcbbb8839 )
	ROM_LOAD( "phoenix.48",   0x1800, 0x0800, 0xcb65eff8 )
	ROM_LOAD( "phoenix.49",   0x2000, 0x0800, 0xc8a5d6d6 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "phoenix.52",   0x3800, 0x0800, 0xb9915263 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39",         0x0000, 0x0800, 0x53413e8f )
	ROM_LOAD( "ic40",         0x0800, 0x0800, 0x0be2ba91 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )  /* palette high bits */
ROM_END

ROM_START( phoenix3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "phoenix3.45",  0x0000, 0x0800, 0xa362cda0 )
	ROM_LOAD( "phoenix3.46",  0x0800, 0x0800, 0x5748f486 )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, 0xcbbb8839 )
	ROM_LOAD( "phoenix3.48",  0x1800, 0x0800, 0xb5d97a4d )
	ROM_LOAD( "ic49",         0x2000, 0x0800, 0xa105e4e7 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "phoenix3.52",  0x3800, 0x0800, 0xd2c5c984 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39",         0x0000, 0x0800, 0x53413e8f )
	ROM_LOAD( "ic40",         0x0800, 0x0800, 0x0be2ba91 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )  /* palette high bits */
ROM_END

ROM_START( phoenixc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "phoenix.45",   0x0000, 0x0800, 0x5b8c55a8 )
	ROM_LOAD( "phoenix.46",   0x0800, 0x0800, 0xdbc942fa )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, 0xcbbb8839 )
	ROM_LOAD( "phoenixc.48",  0x1800, 0x0800, 0x5ae0b215 )
	ROM_LOAD( "phoenixc.49",  0x2000, 0x0800, 0x1a1ce0d0 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "phoenixc.52",  0x3800, 0x0800, 0x8424d7c4 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "phoenixc.39",  0x0000, 0x0800, 0xbb0525ed )
	ROM_LOAD( "phoenixc.40",  0x0800, 0x0800, 0x4178aa4f )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )  /* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )  /* palette high bits */
ROM_END

ROM_START( pleiads )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic47.r1",      0x0000, 0x0800, 0x960212c8 )
	ROM_LOAD( "ic48.r2",      0x0800, 0x0800, 0xb254217c )
	ROM_LOAD( "ic47.bin",     0x1000, 0x0800, 0x87e700bb ) /* IC 49 on real board */
	ROM_LOAD( "ic48.bin",     0x1800, 0x0800, 0x2d5198d0 ) /* IC 50 on real board */
	ROM_LOAD( "ic51.r5",      0x2000, 0x0800, 0x49c629bc )
	ROM_LOAD( "ic50.bin",     0x2800, 0x0800, 0xf1a8a00d ) /* IC 52 on real board */
	ROM_LOAD( "ic53.r7",      0x3000, 0x0800, 0xb5f07fbc )
	ROM_LOAD( "ic52.bin",     0x3800, 0x0800, 0xb1b5a8a6 ) /* IC 54 on real board */

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23.bin",     0x0000, 0x0800, 0x4e30f9e7 ) /* IC 45 on real board */
	ROM_LOAD( "ic24.bin",     0x0800, 0x0800, 0x5188fc29 ) /* IC 44 on real board */

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39.bin",     0x0000, 0x0800, 0x85866607 ) /* IC 27 on real board */
	ROM_LOAD( "ic40.bin",     0x0800, 0x0800, 0xa841d511 ) /* IC 26 on real board */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "7611-5.26",    0x0000, 0x0100, 0x7a1bcb1e )   /* palette low bits */
	ROM_LOAD( "7611-5.33",    0x0100, 0x0100, 0xe38eeb83 )   /* palette high bits */
ROM_END

ROM_START( pleiadbl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic45.bin",     0x0000, 0x0800, 0x93fc2958 )
	ROM_LOAD( "ic46.bin",     0x0800, 0x0800, 0xe2b5b8cd )
	ROM_LOAD( "ic47.bin",     0x1000, 0x0800, 0x87e700bb )
	ROM_LOAD( "ic48.bin",     0x1800, 0x0800, 0x2d5198d0 )
	ROM_LOAD( "ic49.bin",     0x2000, 0x0800, 0x9dc73e63 )
	ROM_LOAD( "ic50.bin",     0x2800, 0x0800, 0xf1a8a00d )
	ROM_LOAD( "ic51.bin",     0x3000, 0x0800, 0x6f56f317 )
	ROM_LOAD( "ic52.bin",     0x3800, 0x0800, 0xb1b5a8a6 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic23.bin",     0x0000, 0x0800, 0x4e30f9e7 )
	ROM_LOAD( "ic24.bin",     0x0800, 0x0800, 0x5188fc29 )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39.bin",     0x0000, 0x0800, 0x85866607 )
	ROM_LOAD( "ic40.bin",     0x0800, 0x0800, 0xa841d511 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "7611-5.26",    0x0000, 0x0100, 0x7a1bcb1e )   /* palette low bits */
	ROM_LOAD( "7611-5.33",    0x0100, 0x0100, 0xe38eeb83 )   /* palette high bits */
ROM_END

ROM_START( pleiadce )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pleiades.47",  0x0000, 0x0800, 0x711e2ba0 )
	ROM_LOAD( "pleiades.48",  0x0800, 0x0800, 0x93a36943 )
	ROM_LOAD( "ic47.bin",     0x1000, 0x0800, 0x87e700bb )
	ROM_LOAD( "pleiades.50",  0x1800, 0x0800, 0x5a9beba0 )
	ROM_LOAD( "pleiades.51",  0x2000, 0x0800, 0x1d828719 )
	ROM_LOAD( "ic50.bin",     0x2800, 0x0800, 0xf1a8a00d )
	ROM_LOAD( "pleiades.53",  0x3000, 0x0800, 0x037b319c )
	ROM_LOAD( "pleiades.54",  0x3800, 0x0800, 0xca264c7c )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pleiades.45",  0x0000, 0x0800, 0x8dbd3785 )
	ROM_LOAD( "pleiades.44",  0x0800, 0x0800, 0x0db3e436 )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic39.bin",     0x0000, 0x0800, 0x85866607 )
	ROM_LOAD( "ic40.bin",     0x0800, 0x0800, 0xa841d511 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "7611-5.26",    0x0000, 0x0100, 0x7a1bcb1e )   /* palette low bits */
	ROM_LOAD( "7611-5.33",    0x0100, 0x0100, 0xe38eeb83 )   /* palette high bits */
ROM_END

ROM_START( survival )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "g959-32a.u45", 0x0000, 0x0800, 0x0bc53541 )
	ROM_LOAD( "g959-33a.u46", 0x0800, 0x0800, 0x726e9428 )
	ROM_LOAD( "g959-34a.u47", 0x1000, 0x0800, 0x78f166ff )
	ROM_LOAD( "g959-35a.u48", 0x1800, 0x0800, 0x59dbe099 )
	ROM_LOAD( "g959-36a.u49", 0x2000, 0x0800, 0xbd5e586e )
	ROM_LOAD( "g959-37a.u50", 0x2800, 0x0800, 0xb2de1094 )
	ROM_LOAD( "g959-38a.u51", 0x3000, 0x0800, 0x131c4440 )
	ROM_LOAD( "g959-39a.u52", 0x3800, 0x0800, 0x213bc910 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g959-42.u23",  0x0000, 0x0800, 0x3d1ce38d )
	ROM_LOAD( "g959-43.u24",  0x0800, 0x0800, 0xcd150da9 )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "g959-40.u39",  0x0000, 0x0800, 0x41dee996 )
	ROM_LOAD( "g959-41.u40",  0x0800, 0x0800, 0xa255d6dc )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "clr.u40",      0x0000, 0x0100, 0xb3e20669 )   /* palette low bits */
	ROM_LOAD( "clr.u41",      0x0100, 0x0100, 0xabddf69a )   /* palette high bits */
ROM_END


static void init_survival(void)
{
	unsigned char *rom = memory_region(REGION_CPU1);

	rom[0x0157] = 0x21;	/* ROM check */
	rom[0x02e8] = 0x21; /* crash due to protection, it still locks up somewhere else */
}



GAME ( 1980, phoenix,  0,       phoenix,  phoenix,  0,        ROT90, "Amstar", "Phoenix (Amstar)" )
GAME ( 1980, phoenixa, phoenix, phoenix,  phoenixa, 0,        ROT90, "Amstar (Centuri license)", "Phoenix (Centuri)" )
GAME ( 1980, phoenixt, phoenix, phoenix,  phoenixt, 0,        ROT90, "Taito", "Phoenix (Taito)" )
GAME ( 1980, phoenix3, phoenix, phoenix,  phoenix3, 0,        ROT90, "bootleg", "Phoenix (T.P.N.)" )
GAME ( 1981, phoenixc, phoenix, phoenix,  phoenixt, 0,        ROT90, "bootleg?", "Phoenix (IRECSA, G.G.I Corp)" )
GAMEX( 1981, pleiads,  0,       pleiads,  pleiads,  0,        ROT90, "Tehkan", "Pleiads (Tehkan)", GAME_IMPERFECT_COLORS )
GAMEX( 1981, pleiadbl, pleiads, pleiads,  pleiads,  0,        ROT90, "bootleg", "Pleiads (bootleg)", GAME_IMPERFECT_COLORS )
GAMEX( 1981, pleiadce, pleiads, pleiads,  pleiadce, 0,        ROT90, "Tehkan (Centuri license)", "Pleiads (Centuri)", GAME_IMPERFECT_COLORS )
GAMEX( 1982, survival, 0,       survival, survival, survival, ROT90, "Rock-ola", "Survival", GAME_UNEMULATED_PROTECTION )
