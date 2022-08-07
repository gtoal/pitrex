/***************************************************************************

  Data East 16 bit games - Bryan McPhail, mish@tendril.co.uk

  This file contains drivers for:

    * Heavy Barrel                            (USA set)
    * Heavy Barrel                            (World set)
	* Bad Dudes vs. Dragonninja               (USA set)
    * Dragonninja                             (Japanese version of above)
	* Birdy Try                               (Japanese set)
    * Robocop                                 (World bootleg rom set)
    * Robocop                                 (World rev 3)
    * Robocop                                 (USA rev 1)
    * Robocop                                 (USA rev 0)
    * Hippodrome                              (USA set)
    * Fighting Fantasy                        (Japanese version of above)
    * Sly Spy                                 (USA rev 3)
    * Sly Spy                                 (USA rev 2)
    * Secret Agent                            (World set)
    * Midnight Resistance                     (World set)
	* Midnight Resistance                     (USA set)
    * Midnight Resistance                     (Japanese set)
	* Boulderdash                             (World set)

	Heavy Barrel, Bad Dudes, Robocop, Birdy Try & Hippodrome use the 'MEC-M1'
motherboard and varying game boards.  Sly Spy, Midnight Resistance and
Boulderdash use the same graphics chips but are different pcbs.

	There are Secret Agent (bootleg) and Robocop (bootleg) sets to add.

  Thanks to Gouky & Richard Bush for information along the way, especially
  Gouky's patch for Bad Dudes & YM3812 information!
	Thanks to JC Alexander for fix to Robocop ending!

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/h6280/h6280.h"

/* Video emulation definitions */
int  dec0_vh_start(void);
void dec0_vh_stop(void);
int  dec0_nodma_vh_start(void);
void dec0_nodma_vh_stop(void);
void hbarrel_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void baddudes_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void birdtry_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void robocop_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void hippodrm_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void slyspy_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void midres_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern data16_t *dec0_pf1_rowscroll,*dec0_pf2_rowscroll,*dec0_pf3_rowscroll;
extern data16_t *dec0_pf1_colscroll,*dec0_pf2_colscroll,*dec0_pf3_colscroll;
extern data16_t *dec0_pf1_data,*dec0_pf2_data,*dec0_pf3_data;

WRITE16_HANDLER( dec0_pf1_control_0_w );
WRITE16_HANDLER( dec0_pf1_control_1_w );
WRITE16_HANDLER( dec0_pf1_data_w );
READ16_HANDLER( dec0_pf1_data_r );
WRITE16_HANDLER( dec0_pf2_control_0_w );
WRITE16_HANDLER( dec0_pf2_control_1_w );
WRITE16_HANDLER( dec0_pf2_data_w );
READ16_HANDLER( dec0_pf2_data_r );
WRITE16_HANDLER( dec0_pf3_control_0_w );
WRITE16_HANDLER( dec0_pf3_control_1_w );
WRITE16_HANDLER( dec0_pf3_data_w );
READ16_HANDLER( dec0_pf3_data_r );
WRITE16_HANDLER( dec0_priority_w );
WRITE16_HANDLER( dec0_update_sprites_w );

WRITE16_HANDLER( dec0_paletteram_rg_w );
WRITE16_HANDLER( dec0_paletteram_b_w );

READ_HANDLER( dec0_pf3_data_8bit_r );
WRITE_HANDLER( dec0_pf3_data_8bit_w );
WRITE_HANDLER( dec0_pf3_control_8bit_w );

/* System prototypes - from machine/dec0.c */
READ16_HANDLER( dec0_controls_r );
READ16_HANDLER( dec0_rotary_r );
READ16_HANDLER( midres_controls_r );
READ16_HANDLER( slyspy_controls_r );
READ16_HANDLER( slyspy_protection_r );
WRITE16_HANDLER( slyspy_state_w );
READ16_HANDLER( slyspy_state_r );
WRITE16_HANDLER( slyspy_240000_w );
WRITE16_HANDLER( slyspy_242000_w );
WRITE16_HANDLER( slyspy_246000_w );
WRITE16_HANDLER( slyspy_248000_w );
WRITE16_HANDLER( slyspy_24c000_w );
WRITE16_HANDLER( slyspy_24e000_w );

void init_slyspy(void);
void init_hippodrm(void);
void init_robocop(void);
void init_baddudes(void);
void init_hbarrel(void);
void init_hbarrelw(void);
void init_birdtry(void);

extern void dec0_i8751_write(int data);
extern void dec0_i8751_reset(void);
READ_HANDLER( hippodrm_prot_r );
WRITE_HANDLER( hippodrm_prot_w );
READ_HANDLER( hippodrm_shared_r );
WRITE_HANDLER( hippodrm_shared_w );

data16_t *dec0_ram;

/******************************************************************************/

static WRITE16_HANDLER( dec0_control_w )
{
	switch (offset<<1)
	{
		case 0: /* Playfield & Sprite priority */
			dec0_priority_w(0,data,mem_mask);
			break;

		case 2: /* DMA flag */
			dec0_update_sprites_w(0,0,mem_mask);
			break;

		case 4: /* 6502 sound cpu */
			if (ACCESSING_LSB)
			{
				soundlatch_w(0,data & 0xff);
				cpu_cause_interrupt(1,M6502_INT_NMI);
			}
			break;

		case 6: /* Intel 8751 microcontroller - Bad Dudes, Heavy Barrel, Birdy Try only */
			dec0_i8751_write(data);
			break;

		case 8: /* Interrupt ack (VBL - IRQ 6) */
			break;

		case 0xa: /* Mix Psel(?) */
 			logerror("CPU #0 PC %06x: warning - write %02x to unmapped memory address %06x\n",cpu_get_pc(),data,0x30c010+(offset<<1));
			break;

/*		case 0xc:    Cblk - coin blockout?  Seems to be unused by the games    */
/*			break; */

		case 0xe: /* Reset Intel 8751? - not sure, all the games write here at startup */
			dec0_i8751_reset();
 			logerror("CPU #0 PC %06x: warning - write %02x to unmapped memory address %06x\n",cpu_get_pc(),data,0x30c010+(offset<<1));
			break;

		default:
			logerror("CPU #0 PC %06x: warning - write %02x to unmapped memory address %06x\n",cpu_get_pc(),data,0x30c010+(offset<<1));
			break;
	}
}

static WRITE16_HANDLER( slyspy_control_w )
{
    switch (offset<<1) {
    	case 0:
			if (ACCESSING_LSB)
			{
				soundlatch_w(0,data & 0xff);
				cpu_cause_interrupt(1,H6280_INT_NMI);
			}
			break;
		case 2:
			dec0_priority_w(0,data,mem_mask);
			break;
    }
}

static WRITE16_HANDLER( midres_sound_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		cpu_cause_interrupt(1,H6280_INT_NMI);
	}
}

/******************************************************************************/

static MEMORY_READ16_START( dec0_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x242800, 0x243fff, MRA16_RAM }, /* Robocop only */
	{ 0x244000, 0x245fff, dec0_pf1_data_r },
	{ 0x24a000, 0x24a7ff, dec0_pf2_data_r },
	{ 0x24c800, 0x24c87f, MRA16_RAM },
	{ 0x24d000, 0x24d7ff, dec0_pf3_data_r },
	{ 0x300000, 0x30001f, dec0_rotary_r },
	{ 0x30c000, 0x30c00b, dec0_controls_r },
	{ 0x310000, 0x3107ff, MRA16_RAM },
	{ 0x314000, 0x3147ff, MRA16_RAM },
	{ 0xff8000, 0xffbfff, MRA16_RAM }, /* Main ram */
	{ 0xffc000, 0xffc7ff, MRA16_RAM }, /* Sprites */
MEMORY_END

static MEMORY_WRITE16_START( dec0_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x240000, 0x240007, dec0_pf1_control_0_w },	/* text layer */
	{ 0x240010, 0x240017, dec0_pf1_control_1_w },
 	{ 0x242000, 0x24207f, MWA16_RAM, &dec0_pf1_colscroll },
	{ 0x242400, 0x2427ff, MWA16_RAM, &dec0_pf1_rowscroll },
	{ 0x242800, 0x243fff, MWA16_RAM }, /* Robocop only */
	{ 0x244000, 0x245fff, dec0_pf1_data_w, &dec0_pf1_data },

	{ 0x246000, 0x246007, dec0_pf2_control_0_w },	/* first tile layer */
	{ 0x246010, 0x246017, dec0_pf2_control_1_w },
	{ 0x248000, 0x24807f, MWA16_RAM, &dec0_pf2_colscroll },
	{ 0x248400, 0x2487ff, MWA16_RAM, &dec0_pf2_rowscroll },
	{ 0x24a000, 0x24a7ff, dec0_pf2_data_w, &dec0_pf2_data },

	{ 0x24c000, 0x24c007, dec0_pf3_control_0_w },	/* second tile layer */
	{ 0x24c010, 0x24c017, dec0_pf3_control_1_w },
	{ 0x24c800, 0x24c87f, MWA16_RAM, &dec0_pf3_colscroll },
	{ 0x24cc00, 0x24cfff, MWA16_RAM, &dec0_pf3_rowscroll },
	{ 0x24d000, 0x24d7ff, dec0_pf3_data_w, &dec0_pf3_data },

	{ 0x30c010, 0x30c01f, dec0_control_w },	/* Priority, sound, etc. */
	{ 0x310000, 0x3107ff, dec0_paletteram_rg_w, &paletteram16 },	/* Red & Green bits */
	{ 0x314000, 0x3147ff, dec0_paletteram_b_w, &paletteram16_2 },	/* Blue bits */
	{ 0xff8000, 0xffbfff, MWA16_RAM, &dec0_ram },
	{ 0xffc000, 0xffc7ff, MWA16_RAM, &spriteram16 },
MEMORY_END

static MEMORY_READ_START( robocop_sub_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 }, /* Main ram */
MEMORY_END

static MEMORY_WRITE_START( robocop_sub_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 }, /* Main ram */
MEMORY_END

static MEMORY_READ_START( hippodrm_sub_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x180000, 0x1800ff, hippodrm_shared_r },
	{ 0x1a1000, 0x1a17ff, dec0_pf3_data_8bit_r },
	{ 0x1d0000, 0x1d00ff, hippodrm_prot_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 }, /* Main ram */
	{ 0x1ff402, 0x1ff403, input_port_5_r }, /* VBL */
MEMORY_END

static MEMORY_WRITE_START( hippodrm_sub_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x180000, 0x1800ff, hippodrm_shared_w },
	{ 0x1a0000, 0x1a001f, dec0_pf3_control_8bit_w },
	{ 0x1a1000, 0x1a17ff, dec0_pf3_data_8bit_w },
	{ 0x1d0000, 0x1d00ff, hippodrm_prot_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 }, /* Main ram */
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

static MEMORY_READ16_START( slyspy_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x244000, 0x244001, slyspy_state_r }, /* protection */
	{ 0x304000, 0x307fff, MRA16_RAM }, /* Sly spy main ram */
	{ 0x308000, 0x3087ff, MRA16_RAM }, /* Sprites */
	{ 0x310000, 0x3107ff, MRA16_RAM },
	{ 0x314008, 0x31400f, slyspy_controls_r },
	{ 0x31c000, 0x31c00f, slyspy_protection_r },
MEMORY_END

static MEMORY_WRITE16_START( slyspy_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },

	/* These locations aren't real!  They are just there so memory is allocated */
	{ 0x232000, 0x23207f, MWA16_NOP, &dec0_pf2_colscroll },
	{ 0x232400, 0x2327ff, MWA16_NOP, &dec0_pf2_rowscroll },
	{ 0x23c000, 0x23c07f, MWA16_NOP, &dec0_pf1_colscroll },
	{ 0x23c400, 0x23c7ff, MWA16_NOP, &dec0_pf1_rowscroll },
	{ 0x200000, 0x2007ff, MWA16_NOP, &dec0_pf2_data },
	{ 0x202000, 0x203fff, MWA16_NOP, &dec0_pf1_data },

	{ 0x244000, 0x244001, MWA16_NOP }, /* Extra protection? */

	/* The location of p1 & pf2 can change between these according to protection */
	{ 0x240000, 0x241fff, slyspy_240000_w },
	{ 0x242000, 0x243fff, slyspy_242000_w },
	{ 0x246000, 0x247fff, slyspy_246000_w },
	{ 0x248000, 0x249fff, slyspy_248000_w },
	{ 0x24c000, 0x24dfff, slyspy_24c000_w },
	{ 0x24e000, 0x24ffff, slyspy_24e000_w },

	{ 0x24a000, 0x24a001, slyspy_state_w }, /* Protection */

	/* Pf3 is unaffected by protection */
	{ 0x300000, 0x300007, dec0_pf3_control_0_w },
	{ 0x300010, 0x300017, dec0_pf3_control_1_w },
	{ 0x300800, 0x30087f, MWA16_RAM, &dec0_pf3_colscroll },
	{ 0x300c00, 0x300fff, MWA16_RAM, &dec0_pf3_rowscroll },
	{ 0x301000, 0x3017ff, dec0_pf3_data_w, &dec0_pf3_data },

	{ 0x304000, 0x307fff, MWA16_RAM, &dec0_ram }, /* Sly spy main ram */
	{ 0x308000, 0x3087ff, MWA16_RAM, &spriteram16 },
	{ 0x310000, 0x3107ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x314000, 0x314003, slyspy_control_w },
	{ 0x31c000, 0x31c00f, MWA16_NOP },
MEMORY_END

static MEMORY_READ16_START( midres_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x120000, 0x1207ff, MRA16_RAM },
	{ 0x180000, 0x18000f, midres_controls_r },
	{ 0x320000, 0x321fff, dec0_pf1_data_r },
MEMORY_END

static MEMORY_WRITE16_START( midres_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM, &dec0_ram },
	{ 0x120000, 0x1207ff, MWA16_RAM, &spriteram16 },
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x160000, 0x160001, dec0_priority_w },
	{ 0x180008, 0x18000f, MWA16_NOP }, /* ?? watchdog ?? */
	{ 0x1a0000, 0x1a0001, midres_sound_w },

	{ 0x200000, 0x200007, dec0_pf2_control_0_w },
	{ 0x200010, 0x200017, dec0_pf2_control_1_w },
	{ 0x220000, 0x2207ff, dec0_pf2_data_w, &dec0_pf2_data },
	{ 0x220800, 0x220fff, dec0_pf2_data_w },	/* mirror address used in end sequence */
	{ 0x240000, 0x24007f, MWA16_RAM, &dec0_pf2_colscroll },
	{ 0x240400, 0x2407ff, MWA16_RAM, &dec0_pf2_rowscroll },

	{ 0x280000, 0x280007, dec0_pf3_control_0_w },
	{ 0x280010, 0x280017, dec0_pf3_control_1_w },
	{ 0x2a0000, 0x2a07ff, dec0_pf3_data_w, &dec0_pf3_data },
	{ 0x2c0000, 0x2c007f, MWA16_RAM, &dec0_pf3_colscroll },
	{ 0x2c0400, 0x2c07ff, MWA16_RAM, &dec0_pf3_rowscroll },

	{ 0x300000, 0x300007, dec0_pf1_control_0_w },
	{ 0x300010, 0x300017, dec0_pf1_control_1_w },
	{ 0x320000, 0x321fff, dec0_pf1_data_w, &dec0_pf1_data },
	{ 0x340000, 0x34007f, MWA16_RAM, &dec0_pf1_colscroll },
	{ 0x340400, 0x3407ff, MWA16_RAM, &dec0_pf1_rowscroll },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM3812_w )
{
	switch (offset) {
	case 0:
		YM3812_control_port_0_w(0,data);
		break;
	case 1:
		YM3812_write_port_0_w(0,data);
		break;
	}
}

static WRITE_HANDLER( YM2203_w )
{
	switch (offset) {
	case 0:
		YM2203_control_port_0_w(0,data);
		break;
	case 1:
		YM2203_write_port_0_w(0,data);
		break;
	}
}

static MEMORY_READ_START( dec0_s_readmem )
	{ 0x0000, 0x05ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x3800, 0x3800, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( dec0_s_writemem )
	{ 0x0000, 0x05ff, MWA_RAM },
	{ 0x0800, 0x0801, YM2203_w },
	{ 0x1000, 0x1001, YM3812_w },
	{ 0x3800, 0x3800, OKIM6295_data_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/* Physical memory map (21 bits) */
static MEMORY_READ_START( slyspy_s_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x0a0000, 0x0a0001, MRA_NOP }, /* Protection counter */
	{ 0x0e0000, 0x0e0001, OKIM6295_status_0_r },
	{ 0x0f0000, 0x0f0001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( slyspy_s_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x090000, 0x090001, YM3812_w },
	{ 0x0b0000, 0x0b0001, YM2203_w},
	{ 0x0e0000, 0x0e0001, OKIM6295_data_0_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 }, /* Main ram */
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

static MEMORY_READ_START( midres_s_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x130000, 0x130001, OKIM6295_status_0_r },
	{ 0x138000, 0x138001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( midres_s_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x108000, 0x108001, YM3812_w },
	{ 0x118000, 0x118001, YM2203_w },
	{ 0x130000, 0x130001, OKIM6295_data_0_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 }, /* Main ram */
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/******************************************************************************/

#define DEC0_PLAYER1_CONTROL \
	PORT_START /* Player 1 controls */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* Button 3 - unused */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* Button 4 - unused */

#define DEC0_PLAYER2_CONTROL \
	PORT_START /* Player 2 controls */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* Button 3 - unused */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) /* Button 4 - unused */

#define DEC0_MACHINE_CONTROL \
	PORT_START /* Credits, start buttons */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) /* PL1 Button 5 - unused */ \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) /* PL2 Button 5 - unused */ \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

#define DEC0_COIN_SETTING \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) ) \

INPUT_PORTS_START( hbarrel )
	DEC0_PLAYER1_CONTROL
	DEC0_PLAYER2_CONTROL
	DEC0_MACHINE_CONTROL

	PORT_START	/* Dip switch bank 1 */
	DEC0_COIN_SETTING
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "30k 80k 160k" )
	PORT_DIPSETTING(    0x10, "50k 120k 190k" )
	PORT_DIPSETTING(    0x20, "100k 200k 300k" )
	PORT_DIPSETTING(    0x00, "150k 300k 450k" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 10, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 25, 10, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( baddudes )
	DEC0_PLAYER1_CONTROL
	DEC0_PLAYER2_CONTROL
	DEC0_MACHINE_CONTROL

	PORT_START	/* DSW0 */
	DEC0_COIN_SETTING
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused */

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused */
INPUT_PORTS_END

INPUT_PORTS_START( robocop )
	DEC0_PLAYER1_CONTROL
	DEC0_PLAYER2_CONTROL
	DEC0_MACHINE_CONTROL

	PORT_START	/* DSW0 */
	DEC0_COIN_SETTING
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Player Energy" )
	PORT_DIPSETTING(    0x01, "Low" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "High" )
	PORT_DIPSETTING(    0x00, "Very High" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Bonus Stage Energy" )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x20, "High" )
	PORT_DIPNAME( 0x40, 0x40, "Brink Time" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Less" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( hippodrm )
	DEC0_PLAYER1_CONTROL
	DEC0_PLAYER2_CONTROL
	DEC0_MACHINE_CONTROL

	PORT_START	/* Dip switch bank 1 */
	DEC0_COIN_SETTING
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x30, 0x30, "Player & Enemy Energy" )
	PORT_DIPSETTING(    0x10, "Low" )
	PORT_DIPSETTING(    0x20, "Medium" )
	PORT_DIPSETTING(    0x30, "High" )
	PORT_DIPSETTING(    0x00, "Very High" )
	PORT_DIPNAME( 0x40, 0x40, "Enemy Power on Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

#define DEC1_PLAYER1_CONTROL \
	PORT_START /* Player 1 controls */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* button 3 - unused */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

#define DEC1_PLAYER2_CONTROL \
	PORT_START /* Player 2 controls */ \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* button 3 - unused */ \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )


INPUT_PORTS_START( slyspy )
	DEC1_PLAYER1_CONTROL
	DEC1_PLAYER2_CONTROL

	PORT_START	/* Credits, start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )	/* screwed up colors with ACTIVE_LOW */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	DEC0_COIN_SETTING
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Energy" )
	PORT_DIPSETTING(    0x01, "Low" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "High" )
	PORT_DIPSETTING(    0x00, "Very High" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( midres )
	DEC1_PLAYER1_CONTROL
	DEC1_PLAYER2_CONTROL

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	DEC0_COIN_SETTING
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* player 1 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 10, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START	/* player 2 12-way rotary control - converted in controls_r() */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 25, 10, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( bouldash )
	DEC1_PLAYER1_CONTROL
	DEC1_PLAYER2_CONTROL

	PORT_START	/* Credits, start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )		/* extremely slow palette fades with ACTIVE_HIGH */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	DEC0_COIN_SETTING
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Medium" )
	PORT_DIPSETTING(    0x0c, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Game" )
	PORT_DIPSETTING(    0x20, "Part 1" )
	PORT_DIPSETTING(    0x00, "Part 2" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	RGN_FRAC(1,4),
	4,		/* 4 bits per pixel  */
	{ RGN_FRAC(0,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(1,4), RGN_FRAC(3,4), RGN_FRAC(0,4), RGN_FRAC(2,4) },
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 16 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout, 512, 16 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout, 768, 16 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &tilelayout, 256, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo midres_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 256, 16 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout, 512, 16 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout, 768, 16 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &tilelayout,   0, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static void sound_irq(int linestate)
{
	cpu_set_irq_line(1,0,linestate); /* IRQ */
}

static void sound_irq2(int linestate)
{
	cpu_set_irq_line(1,1,linestate); /* IRQ2 */
}

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* 12MHz clock divided by 8 = 1.50 MHz */
	{ YM2203_VOL(35,90) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3000000,	/* 12MHz clock divided by 4 = 3.00 MHz */
	{ 40 },
	{ sound_irq },
};

static struct YM3812interface ym3812b_interface =
{
	1,			/* 1 chip */
	3000000,
	{ 40 },
	{ sound_irq2 },
};

static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 7757 },           /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 80 }
};

/******************************************************************************/

static struct MachineDriver machine_driver_hbarrel =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			dec0_readmem,dec0_writemem,0,0,
			m68_level6_irq,1 /* VBL, level 5 interrupts from i8751 */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			dec0_s_readmem,dec0_s_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529, /* 58 Hz, 529ms Vblank */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	hbarrel_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_baddudes =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			dec0_readmem,dec0_writemem,0,0,
			m68_level6_irq,1 /* VBL, level 5 interrupts from i8751 */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			dec0_s_readmem,dec0_s_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529, /* 58 Hz, 529ms Vblank */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 64*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	baddudes_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_birdtry =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			dec0_readmem,dec0_writemem,0,0,
			m68_level6_irq,1 /* VBL, level 5 interrupts from i8751 */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			dec0_s_readmem,dec0_s_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529, /* 58 Hz, 529ms Vblank */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	birdtry_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_robocop =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			dec0_readmem,dec0_writemem,0,0,
			m68_level6_irq,1 /* VBL */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			dec0_s_readmem,dec0_s_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_H6280,
			21477200/16, /* 21.4772MHz clock */
			robocop_sub_readmem,robocop_sub_writemem,0,0,
			ignore_interrupt,0
		},
	},
	58, 529, /* 58 Hz, 529ms Vblank */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	robocop_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_robocopb =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			dec0_readmem,dec0_writemem,0,0,
			m68_level6_irq,1 /* VBL */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			dec0_s_readmem,dec0_s_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529, /* 58 Hz, 529ms Vblank */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	robocop_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_hippodrm =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,
			dec0_readmem,dec0_writemem,0,0,
			m68_level6_irq,1 /* VBL */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1500000,
			dec0_s_readmem,dec0_s_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_H6280,
			21477200/16, /* 21.4772MHz clock */
			hippodrm_sub_readmem,hippodrm_sub_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529, /* 58 Hz, 529ms Vblank */
	5,	/* Interleave between H6280 & 68000 */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec0_vh_start,
	dec0_vh_stop,
	hippodrm_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_slyspy =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,
			slyspy_readmem,slyspy_writemem,0,0,
			m68_level6_irq,1 /* VBL */
		},
		{
			CPU_H6280 | CPU_AUDIO_CPU,
			3000000,
			slyspy_s_readmem,slyspy_s_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529, /* 58 Hz, 529ms Vblank */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec0_nodma_vh_start,
	dec0_nodma_vh_stop,
	slyspy_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812b_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_midres =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,
			midres_readmem,midres_writemem,0,0,
			m68_level6_irq,1 /* VBL */
		},
		{
			CPU_H6280 | CPU_AUDIO_CPU,
			3000000,
			midres_s_readmem,midres_s_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529, /* 58 Hz, 529ms Vblank */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	midres_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec0_nodma_vh_start,
	dec0_nodma_vh_stop,
	midres_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812b_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/******************************************************************************/

ROM_START( hbarrel )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "hb04.bin",     0x00000, 0x10000, 0x4877b09e )
	ROM_LOAD16_BYTE( "hb01.bin",     0x00001, 0x10000, 0x8b41c219 )
	ROM_LOAD16_BYTE( "hb05.bin",     0x20000, 0x10000, 0x2087d570 )
	ROM_LOAD16_BYTE( "hb02.bin",     0x20001, 0x10000, 0x815536ae )
	ROM_LOAD16_BYTE( "hb06.bin",     0x40000, 0x10000, 0xda4e3fbc )
	ROM_LOAD16_BYTE( "hb03.bin",     0x40001, 0x10000, 0x7fed7c46 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 Sound */
	ROM_LOAD( "hb07.bin",     0x8000, 0x8000, 0xa127f0f7 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "hb25.bin",     0x00000, 0x10000, 0x8649762c )
	ROM_LOAD( "hb26.bin",     0x10000, 0x10000, 0xf8189bbd )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "hb18.bin",     0x00000, 0x10000, 0xef664373 )
	ROM_LOAD( "hb17.bin",     0x10000, 0x10000, 0xa4f186ac )
	ROM_LOAD( "hb20.bin",     0x20000, 0x10000, 0x2fc13be0 )
	ROM_LOAD( "hb19.bin",     0x30000, 0x10000, 0xd6b47869 )
	ROM_LOAD( "hb22.bin",     0x40000, 0x10000, 0x50d6a1ad )
	ROM_LOAD( "hb21.bin",     0x50000, 0x10000, 0xf01d75c5 )
	ROM_LOAD( "hb24.bin",     0x60000, 0x10000, 0xae377361 )
	ROM_LOAD( "hb23.bin",     0x70000, 0x10000, 0xbbdaf771 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "hb29.bin",     0x00000, 0x10000, 0x5514b296 )
	ROM_LOAD( "hb30.bin",     0x10000, 0x10000, 0x5855e8ef )
	ROM_LOAD( "hb27.bin",     0x20000, 0x10000, 0x99db7b9c )
	ROM_LOAD( "hb28.bin",     0x30000, 0x10000, 0x33ce2b1a )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "hb15.bin",     0x00000, 0x10000, 0x21816707 )
	ROM_LOAD( "hb16.bin",     0x10000, 0x10000, 0xa5684574 )
	ROM_LOAD( "hb11.bin",     0x20000, 0x10000, 0x5c768315 )
	ROM_LOAD( "hb12.bin",     0x30000, 0x10000, 0x8b64d7a4 )
	ROM_LOAD( "hb13.bin",     0x40000, 0x10000, 0x56e3ed65 )
	ROM_LOAD( "hb14.bin",     0x50000, 0x10000, 0xbedfe7f3 )
	ROM_LOAD( "hb09.bin",     0x60000, 0x10000, 0x26240ea0 )
	ROM_LOAD( "hb10.bin",     0x70000, 0x10000, 0x47d95447 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "hb08.bin",     0x0000, 0x10000, 0x645c5b68 )
ROM_END

ROM_START( hbarrelw )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "hb_ec04.rom",  0x00000, 0x10000, 0xd01bc3db )
	ROM_LOAD16_BYTE( "hb_ec01.rom",  0x00001, 0x10000, 0x6756f8ae )
	ROM_LOAD16_BYTE( "hb05.bin",     0x20000, 0x10000, 0x2087d570 )
	ROM_LOAD16_BYTE( "hb02.bin",     0x20001, 0x10000, 0x815536ae )
	ROM_LOAD16_BYTE( "hb_ec06.rom",  0x40000, 0x10000, 0x61ec20d8 )
	ROM_LOAD16_BYTE( "hb_ec03.rom",  0x40001, 0x10000, 0x720c6b13 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 Sound */
	ROM_LOAD( "hb_ec07.rom",  0x8000, 0x8000, 0x16a5a1aa )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "hb_ec25.rom",  0x00000, 0x10000, 0x2e5732a2 )
	ROM_LOAD( "hb_ec26.rom",  0x10000, 0x10000, 0x161a2c4d )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "hb18.bin",     0x00000, 0x10000, 0xef664373 )
	ROM_LOAD( "hb17.bin",     0x10000, 0x10000, 0xa4f186ac )
	ROM_LOAD( "hb20.bin",     0x20000, 0x10000, 0x2fc13be0 )
	ROM_LOAD( "hb19.bin",     0x30000, 0x10000, 0xd6b47869 )
	ROM_LOAD( "hb22.bin",     0x40000, 0x10000, 0x50d6a1ad )
	ROM_LOAD( "hb21.bin",     0x50000, 0x10000, 0xf01d75c5 )
	ROM_LOAD( "hb24.bin",     0x60000, 0x10000, 0xae377361 )
	ROM_LOAD( "hb23.bin",     0x70000, 0x10000, 0xbbdaf771 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "hb29.bin",     0x00000, 0x10000, 0x5514b296 )
	ROM_LOAD( "hb30.bin",     0x10000, 0x10000, 0x5855e8ef )
	ROM_LOAD( "hb27.bin",     0x20000, 0x10000, 0x99db7b9c )
	ROM_LOAD( "hb28.bin",     0x30000, 0x10000, 0x33ce2b1a )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "hb15.bin",     0x00000, 0x10000, 0x21816707 )
	ROM_LOAD( "hb16.bin",     0x10000, 0x10000, 0xa5684574 )
	ROM_LOAD( "hb11.bin",     0x20000, 0x10000, 0x5c768315 )
	ROM_LOAD( "hb12.bin",     0x30000, 0x10000, 0x8b64d7a4 )
	ROM_LOAD( "hb13.bin",     0x40000, 0x10000, 0x56e3ed65 )
	ROM_LOAD( "hb14.bin",     0x50000, 0x10000, 0xbedfe7f3 )
	ROM_LOAD( "hb09.bin",     0x60000, 0x10000, 0x26240ea0 )
	ROM_LOAD( "hb10.bin",     0x70000, 0x10000, 0x47d95447 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "hb_ec08.rom",  0x0000, 0x10000, 0x2159a609 )
ROM_END

ROM_START( baddudes )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code, middle 0x20000 unused */
	ROM_LOAD16_BYTE( "baddudes.4",   0x00000, 0x10000, 0x4bf158a7 )
	ROM_LOAD16_BYTE( "baddudes.1",   0x00001, 0x10000, 0x74f5110c )
	ROM_LOAD16_BYTE( "baddudes.6",   0x40000, 0x10000, 0x3ff8da57 )
	ROM_LOAD16_BYTE( "baddudes.3",   0x40001, 0x10000, 0xf8f2bd94 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "baddudes.7",   0x8000, 0x8000, 0x9fb1ef4b )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "baddudes.25",  0x00000, 0x08000, 0xbcf59a69 )
	ROM_LOAD( "baddudes.26",  0x08000, 0x08000, 0x9aff67b8 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "baddudes.18",  0x00000, 0x10000, 0x05cfc3e5 )
	ROM_LOAD( "baddudes.20",  0x10000, 0x10000, 0xe11e988f )
	ROM_LOAD( "baddudes.22",  0x20000, 0x10000, 0xb893d880 )
	ROM_LOAD( "baddudes.24",  0x30000, 0x10000, 0x6f226dda )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "baddudes.30",  0x08000, 0x08000, 0x982da0d1 )
	ROM_CONTINUE(             0x00000, 0x08000 )	/* the two halves are swapped */
	ROM_LOAD( "baddudes.28",  0x18000, 0x08000, 0xf01ebb3b )
	ROM_CONTINUE(             0x10000, 0x08000 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "baddudes.15",  0x00000, 0x10000, 0xa38a7d30 )
	ROM_LOAD( "baddudes.16",  0x10000, 0x08000, 0x17e42633 )
	ROM_LOAD( "baddudes.11",  0x20000, 0x10000, 0x3a77326c )
	ROM_LOAD( "baddudes.12",  0x30000, 0x08000, 0xfea2a134 )
	ROM_LOAD( "baddudes.13",  0x40000, 0x10000, 0xe5ae2751 )
	ROM_LOAD( "baddudes.14",  0x50000, 0x08000, 0xe83c760a )
	ROM_LOAD( "baddudes.9",   0x60000, 0x10000, 0x6901e628 )
	ROM_LOAD( "baddudes.10",  0x70000, 0x08000, 0xeeee8a1a )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "baddudes.8",   0x0000, 0x10000, 0x3c87463e )
ROM_END

ROM_START( drgninja )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code, middle 0x20000 unused */
	ROM_LOAD16_BYTE( "drgninja.04",  0x00000, 0x10000, 0x41b8b3f8 )
	ROM_LOAD16_BYTE( "drgninja.01",  0x00001, 0x10000, 0xe08e6885 )
	ROM_LOAD16_BYTE( "drgninja.06",  0x40000, 0x10000, 0x2b81faf7 )
	ROM_LOAD16_BYTE( "drgninja.03",  0x40001, 0x10000, 0xc52c2e9d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "drgninja.07",  0x8000, 0x8000, 0x001d2f51 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "drgninja.25",  0x00000, 0x08000, 0x6791bc20 )
	ROM_LOAD( "drgninja.26",  0x08000, 0x08000, 0x5d75fc8f )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "baddudes.18",  0x00000, 0x10000, 0x05cfc3e5 )
	ROM_LOAD( "baddudes.20",  0x10000, 0x10000, 0xe11e988f )
	ROM_LOAD( "baddudes.22",  0x20000, 0x10000, 0xb893d880 )
	ROM_LOAD( "baddudes.24",  0x30000, 0x10000, 0x6f226dda )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "drgninja.30",  0x08000, 0x08000, 0x2438e67e )
	ROM_CONTINUE(             0x00000, 0x08000 )	/* the two halves are swapped */
	ROM_LOAD( "drgninja.28",  0x18000, 0x08000, 0x5c692ab3 )
	ROM_CONTINUE(             0x10000, 0x08000 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "drgninja.15",  0x00000, 0x10000, 0x5617d67f )
	ROM_LOAD( "baddudes.16",  0x10000, 0x08000, 0x17e42633 )
	ROM_LOAD( "drgninja.11",  0x20000, 0x10000, 0xba83e8d8 )
	ROM_LOAD( "baddudes.12",  0x30000, 0x08000, 0xfea2a134 )
	ROM_LOAD( "drgninja.13",  0x40000, 0x10000, 0xfd91e08e )
	ROM_LOAD( "baddudes.14",  0x50000, 0x08000, 0xe83c760a )
	ROM_LOAD( "baddudes.9",   0x60000, 0x10000, 0x6901e628 )
	ROM_LOAD( "baddudes.10",  0x70000, 0x08000, 0xeeee8a1a )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "baddudes.8",   0x0000, 0x10000, 0x3c87463e )
ROM_END

ROM_START( birdtry )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "ek-04.bin",     0x00000, 0x10000, 0x5f0f4686 )
	ROM_LOAD16_BYTE( "ek-01.bin",     0x00001, 0x10000, 0x47f470db )
	ROM_LOAD16_BYTE( "ek-05.bin",     0x20000, 0x10000, 0xb508cffd )
	ROM_LOAD16_BYTE( "ek-02.bin",     0x20001, 0x10000, 0x0195d989 )
	ROM_LOAD16_BYTE( "ek-06.bin",     0x40000, 0x10000, 0x301d57d8 )
	ROM_LOAD16_BYTE( "ek-03.bin",     0x40001, 0x10000, 0x73b0acc5 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 Sound */
	ROM_LOAD( "ek-07.bin",     0x8000, 0x8000, 0x236549bc )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "ek-25.bin",     0x00000, 0x08000, 0x4df134ad )
	ROM_LOAD( "ek-26.bin",     0x08000, 0x08000, 0xa00d3e8e )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ek-18.bin",     0x00000, 0x10000, 0x9886fb70 )
	ROM_LOAD( "ek-17.bin",     0x10000, 0x10000, 0xbed91bf7 )
	ROM_LOAD( "ek-20.bin",     0x20000, 0x10000, 0x45d53965 )
	ROM_LOAD( "ek-19.bin",     0x30000, 0x10000, 0xc2949dd2 )
	ROM_LOAD( "ek-22.bin",     0x40000, 0x10000, 0x7f2cc80a )
	ROM_LOAD( "ek-21.bin",     0x50000, 0x10000, 0x281bc793 )
	ROM_LOAD( "ek-24.bin",     0x60000, 0x10000, 0x2244cc75 )
	ROM_LOAD( "ek-23.bin",     0x70000, 0x10000, 0xd0ed0116 )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	/* This game doesn't have the extra playfield chip, so no roms */

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ek-15.bin",     0x00000, 0x10000, 0xa6a041a3 )
	ROM_LOAD( "ek-16.bin",     0x10000, 0x08000, 0x784f62b0 )
	ROM_LOAD( "ek-11.bin",     0x20000, 0x10000, 0x9224a6b9 )
	ROM_LOAD( "ek-12.bin",     0x30000, 0x08000, 0x12deecfa )
	ROM_LOAD( "ek-13.bin",     0x40000, 0x10000, 0x1f023459 )
	ROM_LOAD( "ek-14.bin",     0x50000, 0x08000, 0x57d54943 )
	ROM_LOAD( "ek-09.bin",     0x60000, 0x10000, 0x6d2d488a )
	ROM_LOAD( "ek-10.bin",     0x70000, 0x08000, 0x580ba206 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ek-08.bin",     0x0000, 0x10000, 0xbe3db6cb )
ROM_END

ROM_START( robocop )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ep05-3", 0x00000, 0x10000, 0xba69bf84 )
	ROM_LOAD16_BYTE( "ep01-3", 0x00001, 0x10000, 0x2a9f6e2c )
	ROM_LOAD16_BYTE( "ep04-3", 0x20000, 0x10000, 0x39181778 )
	ROM_LOAD16_BYTE( "ep00-3", 0x20001, 0x10000, 0xe128541f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 Sound */
	ROM_LOAD( "ep03-3", 0x08000, 0x08000, 0x5b164b24 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* HuC6280 CPU */
	/* Filled in later */

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "ep23", 0x00000, 0x10000, 0xa77e4ab1 )
	ROM_LOAD( "ep22", 0x10000, 0x10000, 0x9fbd6903 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep20", 0x00000, 0x10000, 0x1d8d38b8 )
	ROM_LOAD( "ep21", 0x10000, 0x10000, 0x187929b2 )
	ROM_LOAD( "ep18", 0x20000, 0x10000, 0xb6580b5e )
	ROM_LOAD( "ep19", 0x30000, 0x10000, 0x9bad01c7 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep14", 0x00000, 0x08000, 0xca56ceda )
	ROM_LOAD( "ep15", 0x08000, 0x08000, 0xa945269c )
	ROM_LOAD( "ep16", 0x10000, 0x08000, 0xe7fa4d58 )
	ROM_LOAD( "ep17", 0x18000, 0x08000, 0x84aae89d )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ep07", 0x00000, 0x10000, 0x495d75cf )
	ROM_LOAD( "ep06", 0x10000, 0x08000, 0xa2ae32e2 )
	ROM_LOAD( "ep11", 0x20000, 0x10000, 0x62fa425a )
	ROM_LOAD( "ep10", 0x30000, 0x08000, 0xcce3bd95 )
	ROM_LOAD( "ep09", 0x40000, 0x10000, 0x11bed656 )
	ROM_LOAD( "ep08", 0x50000, 0x08000, 0xc45c7b4c )
	ROM_LOAD( "ep13", 0x60000, 0x10000, 0x8fca9f28 )
	ROM_LOAD( "ep12", 0x70000, 0x08000, 0x3cd1d0c3 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ep02", 0x00000, 0x10000, 0x711ce46f )
ROM_END

ROM_START( robocopu )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ep05-1", 0x00000, 0x10000, 0x8de5cb3d )
	ROM_LOAD16_BYTE( "ep01-1", 0x00001, 0x10000, 0xb3c6bc02 )
	ROM_LOAD16_BYTE( "ep04", 0x20000, 0x10000, 0xc38b9d18 )
	ROM_LOAD16_BYTE( "ep00", 0x20001, 0x10000, 0x374c91aa )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 Sound */
	ROM_LOAD( "ep03", 0x08000, 0x08000, 0x1089eab8 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* HuC6280 CPU */
	/* Filled in later */

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "ep23", 0x00000, 0x10000, 0xa77e4ab1 )
	ROM_LOAD( "ep22", 0x10000, 0x10000, 0x9fbd6903 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep20", 0x00000, 0x10000, 0x1d8d38b8 )
	ROM_LOAD( "ep21", 0x10000, 0x10000, 0x187929b2 )
	ROM_LOAD( "ep18", 0x20000, 0x10000, 0xb6580b5e )
	ROM_LOAD( "ep19", 0x30000, 0x10000, 0x9bad01c7 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep14", 0x00000, 0x08000, 0xca56ceda )
	ROM_LOAD( "ep15", 0x08000, 0x08000, 0xa945269c )
	ROM_LOAD( "ep16", 0x10000, 0x08000, 0xe7fa4d58 )
	ROM_LOAD( "ep17", 0x18000, 0x08000, 0x84aae89d )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ep07", 0x00000, 0x10000, 0x495d75cf )
	ROM_LOAD( "ep06", 0x10000, 0x08000, 0xa2ae32e2 )
	ROM_LOAD( "ep11", 0x20000, 0x10000, 0x62fa425a )
	ROM_LOAD( "ep10", 0x30000, 0x08000, 0xcce3bd95 )
	ROM_LOAD( "ep09", 0x40000, 0x10000, 0x11bed656 )
	ROM_LOAD( "ep08", 0x50000, 0x08000, 0xc45c7b4c )
	ROM_LOAD( "ep13", 0x60000, 0x10000, 0x8fca9f28 )
	ROM_LOAD( "ep12", 0x70000, 0x08000, 0x3cd1d0c3 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ep02", 0x00000, 0x10000, 0x711ce46f )
ROM_END

ROM_START( robocpu0 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ep05", 0x00000, 0x10000, 0xc465bdd8 )
	ROM_LOAD16_BYTE( "ep01", 0x00001, 0x10000, 0x1352d36e )
	ROM_LOAD16_BYTE( "ep04", 0x20000, 0x10000, 0xc38b9d18 )
	ROM_LOAD16_BYTE( "ep00", 0x20001, 0x10000, 0x374c91aa )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 Sound */
	ROM_LOAD( "ep03", 0x08000, 0x08000, 0x1089eab8 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* HuC6280 CPU */
	/* Filled in later */

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "ep23", 0x00000, 0x10000, 0xa77e4ab1 )
	ROM_LOAD( "ep22", 0x10000, 0x10000, 0x9fbd6903 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep20", 0x00000, 0x10000, 0x1d8d38b8 )
	ROM_LOAD( "ep21", 0x10000, 0x10000, 0x187929b2 )
	ROM_LOAD( "ep18", 0x20000, 0x10000, 0xb6580b5e )
	ROM_LOAD( "ep19", 0x30000, 0x10000, 0x9bad01c7 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep14", 0x00000, 0x08000, 0xca56ceda )
	ROM_LOAD( "ep15", 0x08000, 0x08000, 0xa945269c )
	ROM_LOAD( "ep16", 0x10000, 0x08000, 0xe7fa4d58 )
	ROM_LOAD( "ep17", 0x18000, 0x08000, 0x84aae89d )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ep07", 0x00000, 0x10000, 0x495d75cf )
	ROM_LOAD( "ep06", 0x10000, 0x08000, 0xa2ae32e2 )
	ROM_LOAD( "ep11", 0x20000, 0x10000, 0x62fa425a )
	ROM_LOAD( "ep10", 0x30000, 0x08000, 0xcce3bd95 )
	ROM_LOAD( "ep09", 0x40000, 0x10000, 0x11bed656 )
	ROM_LOAD( "ep08", 0x50000, 0x08000, 0xc45c7b4c )
	ROM_LOAD( "ep13", 0x60000, 0x10000, 0x8fca9f28 )
	ROM_LOAD( "ep12", 0x70000, 0x08000, 0x3cd1d0c3 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ep02", 0x00000, 0x10000, 0x711ce46f )
ROM_END

ROM_START( robocopb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "robop_05.rom", 0x00000, 0x10000, 0xbcef3e9b )
	ROM_LOAD16_BYTE( "robop_01.rom", 0x00001, 0x10000, 0xc9803685 )
	ROM_LOAD16_BYTE( "robop_04.rom", 0x20000, 0x10000, 0x9d7b79e0 )
	ROM_LOAD16_BYTE( "robop_00.rom", 0x20001, 0x10000, 0x80ba64ab )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 Sound */
	ROM_LOAD( "ep03-3", 0x08000, 0x08000, 0x5b164b24 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "ep23", 0x00000, 0x10000, 0xa77e4ab1 )
	ROM_LOAD( "ep22", 0x10000, 0x10000, 0x9fbd6903 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep20", 0x00000, 0x10000, 0x1d8d38b8 )
	ROM_LOAD( "ep21", 0x10000, 0x10000, 0x187929b2 )
	ROM_LOAD( "ep18", 0x20000, 0x10000, 0xb6580b5e )
	ROM_LOAD( "ep19", 0x30000, 0x10000, 0x9bad01c7 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep14", 0x00000, 0x08000, 0xca56ceda )
	ROM_LOAD( "ep15", 0x08000, 0x08000, 0xa945269c )
	ROM_LOAD( "ep16", 0x10000, 0x08000, 0xe7fa4d58 )
	ROM_LOAD( "ep17", 0x18000, 0x08000, 0x84aae89d )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ep07", 0x00000, 0x10000, 0x495d75cf )
	ROM_LOAD( "ep06", 0x10000, 0x08000, 0xa2ae32e2 )
	ROM_LOAD( "ep11", 0x20000, 0x10000, 0x62fa425a )
	ROM_LOAD( "ep10", 0x30000, 0x08000, 0xcce3bd95 )
	ROM_LOAD( "ep09", 0x40000, 0x10000, 0x11bed656 )
	ROM_LOAD( "ep08", 0x50000, 0x08000, 0xc45c7b4c )
	ROM_LOAD( "ep13", 0x60000, 0x10000, 0x8fca9f28 )
	ROM_LOAD( "ep12", 0x70000, 0x08000, 0x3cd1d0c3 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ep02", 0x00000, 0x10000, 0x711ce46f )
ROM_END

ROM_START( hippodrm )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "ew02",         0x00000, 0x10000, 0xdf0d7dc6 )
	ROM_LOAD16_BYTE( "ew01",         0x00001, 0x10000, 0xd5670aa7 )
	ROM_LOAD16_BYTE( "ew05",         0x20000, 0x10000, 0xc76d65ec )
	ROM_LOAD16_BYTE( "ew00",         0x20001, 0x10000, 0xe9b427a6 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 sound */
	ROM_LOAD( "ew04",         0x8000, 0x8000, 0x9871b98d )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* HuC6280 CPU */
	ROM_LOAD( "ew08",         0x00000, 0x10000, 0x53010534 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "ew14",         0x00000, 0x10000, 0x71ca593d )
	ROM_LOAD( "ew13",         0x10000, 0x10000, 0x86be5fa7 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ew19",         0x00000, 0x08000, 0x6b80d7a3 )
	ROM_LOAD( "ew18",         0x08000, 0x08000, 0x78d3d764 )
	ROM_LOAD( "ew20",         0x10000, 0x08000, 0xce9f5de3 )
	ROM_LOAD( "ew21",         0x18000, 0x08000, 0x487a7ba2 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ew24",         0x00000, 0x08000, 0x4e1bc2a4 )
	ROM_LOAD( "ew25",         0x08000, 0x08000, 0x9eb47dfb )
	ROM_LOAD( "ew23",         0x10000, 0x08000, 0x9ecf479e )
	ROM_LOAD( "ew22",         0x18000, 0x08000, 0xe55669aa )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ew15",         0x00000, 0x10000, 0x95423914 )
	ROM_LOAD( "ew16",         0x10000, 0x10000, 0x96233177 )
	ROM_LOAD( "ew10",         0x20000, 0x10000, 0x4c25dfe8 )
	ROM_LOAD( "ew11",         0x30000, 0x10000, 0xf2e007fc )
	ROM_LOAD( "ew06",         0x40000, 0x10000, 0xe4bb8199 )
	ROM_LOAD( "ew07",         0x50000, 0x10000, 0x470b6989 )
	ROM_LOAD( "ew17",         0x60000, 0x10000, 0x8c97c757 )
	ROM_LOAD( "ew12",         0x70000, 0x10000, 0xa2d244bc )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ew03",         0x0000, 0x10000, 0xb606924d )
ROM_END

ROM_START( ffantasy )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "ff-02-2.bin",  0x00000, 0x10000, 0x29fc22a7 )
	ROM_LOAD16_BYTE( "ff-01-2.bin",  0x00001, 0x10000, 0x9f617cb4 )
	ROM_LOAD16_BYTE( "ew05",         0x20000, 0x10000, 0xc76d65ec )
	ROM_LOAD16_BYTE( "ew00",         0x20001, 0x10000, 0xe9b427a6 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 sound */
	ROM_LOAD( "ew04",         0x8000, 0x8000, 0x9871b98d )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* HuC6280 CPU */
	ROM_LOAD( "ew08",         0x00000, 0x10000, 0x53010534 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "ev14",         0x00000, 0x10000, 0x686f72c1 )
	ROM_LOAD( "ev13",         0x10000, 0x10000, 0xb787dcc9 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ew19",         0x00000, 0x08000, 0x6b80d7a3 )
	ROM_LOAD( "ew18",         0x08000, 0x08000, 0x78d3d764 )
	ROM_LOAD( "ew20",         0x10000, 0x08000, 0xce9f5de3 )
	ROM_LOAD( "ew21",         0x18000, 0x08000, 0x487a7ba2 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ew24",         0x00000, 0x08000, 0x4e1bc2a4 )
	ROM_LOAD( "ew25",         0x08000, 0x08000, 0x9eb47dfb )
	ROM_LOAD( "ew23",         0x10000, 0x08000, 0x9ecf479e )
	ROM_LOAD( "ew22",         0x18000, 0x08000, 0xe55669aa )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ev15",         0x00000, 0x10000, 0x1d80f797 )
	ROM_LOAD( "ew16",         0x10000, 0x10000, 0x96233177 )
	ROM_LOAD( "ev10",         0x20000, 0x10000, 0xc4e7116b )
	ROM_LOAD( "ew11",         0x30000, 0x10000, 0xf2e007fc )
	ROM_LOAD( "ev06",         0x40000, 0x10000, 0x6c794f1a )
	ROM_LOAD( "ew07",         0x50000, 0x10000, 0x470b6989 )
	ROM_LOAD( "ev17",         0x60000, 0x10000, 0x045509d4 )
	ROM_LOAD( "ew12",         0x70000, 0x10000, 0xa2d244bc )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ew03",         0x0000, 0x10000, 0xb606924d )
ROM_END

ROM_START( ffantasa )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "ev02",         0x00000, 0x10000, 0x797a7860 )
	ROM_LOAD16_BYTE( "ev01",         0x00001, 0x10000, 0x0f17184d )
	ROM_LOAD16_BYTE( "ew05",         0x20000, 0x10000, 0xc76d65ec )
	ROM_LOAD16_BYTE( "ew00",         0x20001, 0x10000, 0xe9b427a6 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 sound */
	ROM_LOAD( "ew04",         0x8000, 0x8000, 0x9871b98d )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* HuC6280 CPU */
	ROM_LOAD( "ew08",         0x00000, 0x10000, 0x53010534 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "ev14",         0x00000, 0x10000, 0x686f72c1 )
	ROM_LOAD( "ev13",         0x10000, 0x10000, 0xb787dcc9 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ew19",         0x00000, 0x08000, 0x6b80d7a3 )
	ROM_LOAD( "ew18",         0x08000, 0x08000, 0x78d3d764 )
	ROM_LOAD( "ew20",         0x10000, 0x08000, 0xce9f5de3 )
	ROM_LOAD( "ew21",         0x18000, 0x08000, 0x487a7ba2 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ew24",         0x00000, 0x08000, 0x4e1bc2a4 )
	ROM_LOAD( "ew25",         0x08000, 0x08000, 0x9eb47dfb )
	ROM_LOAD( "ew23",         0x10000, 0x08000, 0x9ecf479e )
	ROM_LOAD( "ew22",         0x18000, 0x08000, 0xe55669aa )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ev15",         0x00000, 0x10000, 0x1d80f797 )
	ROM_LOAD( "ew16",         0x10000, 0x10000, 0x96233177 )
	ROM_LOAD( "ev10",         0x20000, 0x10000, 0xc4e7116b )
	ROM_LOAD( "ew11",         0x30000, 0x10000, 0xf2e007fc )
	ROM_LOAD( "ev06",         0x40000, 0x10000, 0x6c794f1a )
	ROM_LOAD( "ew07",         0x50000, 0x10000, 0x470b6989 )
	ROM_LOAD( "ev17",         0x60000, 0x10000, 0x045509d4 )
	ROM_LOAD( "ew12",         0x70000, 0x10000, 0xa2d244bc )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ew03",         0x0000, 0x10000, 0xb606924d )
ROM_END

ROM_START( slyspy )
	ROM_REGION( 0x60000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fa14-3.17l",   0x00000, 0x10000, 0x54353a84 )
	ROM_LOAD16_BYTE( "fa12-2.9l",    0x00001, 0x10000, 0x1b534294 )
	ROM_LOAD16_BYTE( "fa15.19l",     0x20000, 0x10000, 0x04a79266 )
	ROM_LOAD16_BYTE( "fa13.11l",     0x20001, 0x10000, 0x641cc4b3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fa10.5h",      0x00000, 0x10000, 0xdfd2ff25 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "fa05.11a",     0x04000, 0x04000, 0x09802924 )
	ROM_CONTINUE(             0x00000, 0x04000 )	/* the two halves are swapped */
	ROM_LOAD( "fa04.9a",      0x0c000, 0x04000, 0xec25b895 )
	ROM_CONTINUE(             0x08000, 0x04000 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fa07.17a",     0x00000, 0x10000, 0xe932268b )
	ROM_LOAD( "fa06.15a",     0x10000, 0x10000, 0xc4dd38c0 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fa09.22a",     0x00000, 0x20000, 0x1395e9be )
	ROM_LOAD( "fa08.21a",     0x20000, 0x20000, 0x4d7464db )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fa01.4a",      0x00000, 0x20000, 0x99b0cd92 )
	ROM_LOAD( "fa03.7a",      0x20000, 0x20000, 0x0e7ea74d )
	ROM_LOAD( "fa00.2a",      0x40000, 0x20000, 0xf7df3fd7 )
	ROM_LOAD( "fa02.5a",      0x60000, 0x20000, 0x84e8da9d )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fa11.11k",     0x00000, 0x20000, 0x4e547bad )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.21k",  0x0000, 0x0100, 0xad26e8d4 )	/* timing? (not used) */
ROM_END

ROM_START( slyspy2 )
	ROM_REGION( 0x60000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fa14-2.bin",   0x00000, 0x10000, 0x0e431e39 )
	ROM_LOAD16_BYTE( "fa12-2.9l",    0x00001, 0x10000, 0x1b534294 )
	ROM_LOAD16_BYTE( "fa15.19l",     0x20000, 0x10000, 0x04a79266 )
	ROM_LOAD16_BYTE( "fa13.11l",     0x20001, 0x10000, 0x641cc4b3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fa10.5h",      0x00000, 0x10000, 0xdfd2ff25 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "fa05.11a",     0x04000, 0x04000, 0x09802924 )
	ROM_CONTINUE(             0x00000, 0x04000 )	/* the two halves are swapped */
	ROM_LOAD( "fa04.9a",      0x0c000, 0x04000, 0xec25b895 )
	ROM_CONTINUE(             0x08000, 0x04000 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fa07.17a",     0x00000, 0x10000, 0xe932268b )
	ROM_LOAD( "fa06.15a",     0x10000, 0x10000, 0xc4dd38c0 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fa09.22a",     0x00000, 0x20000, 0x1395e9be )
	ROM_LOAD( "fa08.21a",     0x20000, 0x20000, 0x4d7464db )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fa01.4a",      0x00000, 0x20000, 0x99b0cd92 )
	ROM_LOAD( "fa03.7a",      0x20000, 0x20000, 0x0e7ea74d )
	ROM_LOAD( "fa00.2a",      0x40000, 0x20000, 0xf7df3fd7 )
	ROM_LOAD( "fa02.5a",      0x60000, 0x20000, 0x84e8da9d )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fa11.11k",     0x00000, 0x20000, 0x4e547bad )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.21k",  0x0000, 0x0100, 0xad26e8d4 )	/* timing? (not used) */
ROM_END

ROM_START( secretag )
	ROM_REGION( 0x60000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fb14.bin",   0x00000, 0x10000, 0x9be6ac90 )
	ROM_LOAD16_BYTE( "fb12.bin",   0x00001, 0x10000, 0x28904b6b )
	ROM_LOAD16_BYTE( "fb15.bin",   0x20000, 0x10000, 0x106bb26c )
	ROM_LOAD16_BYTE( "fb13.bin",   0x20001, 0x10000, 0x90523413 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fa10.5h",      0x00000, 0x10000, 0xdfd2ff25 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "fa05.11a",     0x04000, 0x04000, 0x09802924 )
	ROM_CONTINUE(             0x00000, 0x04000 )	/* the two halves are swapped */
	ROM_LOAD( "fa04.9a",      0x0c000, 0x04000, 0xec25b895 )
	ROM_CONTINUE(             0x08000, 0x04000 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fa07.17a",     0x00000, 0x10000, 0xe932268b )
	ROM_LOAD( "fa06.15a",     0x10000, 0x10000, 0xc4dd38c0 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fa09.22a",     0x00000, 0x20000, 0x1395e9be )
	ROM_LOAD( "fa08.21a",     0x20000, 0x20000, 0x4d7464db )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fa01.4a",      0x00000, 0x20000, 0x99b0cd92 )
	ROM_LOAD( "fa03.7a",      0x20000, 0x20000, 0x0e7ea74d )
	ROM_LOAD( "fa00.2a",      0x40000, 0x20000, 0xf7df3fd7 )
	ROM_LOAD( "fa02.5a",      0x60000, 0x20000, 0x84e8da9d )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fa11.11k",     0x00000, 0x20000, 0x4e547bad )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.21k",  0x0000, 0x0100, 0xad26e8d4 )	/* timing? (not used) */
ROM_END

ROM_START( secretab )
	ROM_REGION( 0x60000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "sa_05.bin",    0x00000, 0x10000, 0x00000001 )
	ROM_LOAD16_BYTE( "sa_03.bin",    0x00001, 0x10000, 0x00000001 )
	ROM_LOAD16_BYTE( "sa_06.bin",    0x20000, 0x10000, 0x00000001 )
	ROM_LOAD16_BYTE( "sa_04.bin",    0x20001, 0x10000, 0x00000001 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fa10.5h",      0x00000, 0x10000, 0xdfd2ff25 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "fa05.11a",     0x04000, 0x04000, 0x09802924 )
	ROM_CONTINUE(             0x00000, 0x04000 )	/* the two halves are swapped */
	ROM_LOAD( "fa04.9a",      0x0c000, 0x04000, 0xec25b895 )
	ROM_CONTINUE(             0x08000, 0x04000 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fa07.17a",     0x00000, 0x10000, 0xe932268b )
	ROM_LOAD( "fa06.15a",     0x10000, 0x10000, 0xc4dd38c0 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fa09.22a",     0x00000, 0x20000, 0x1395e9be )
	ROM_LOAD( "fa08.21a",     0x20000, 0x20000, 0x4d7464db )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fa01.4a",      0x00000, 0x20000, 0x99b0cd92 )
	ROM_LOAD( "fa03.7a",      0x20000, 0x20000, 0x0e7ea74d )
	ROM_LOAD( "fa00.2a",      0x40000, 0x20000, 0xf7df3fd7 )
	ROM_LOAD( "fa02.5a",      0x60000, 0x20000, 0x84e8da9d )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fa11.11k",     0x00000, 0x20000, 0x4e547bad )
ROM_END

ROM_START( midres )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fk_14.rom",    0x00000, 0x20000, 0xde7522df )
	ROM_LOAD16_BYTE( "fk_12.rom",    0x00001, 0x20000, 0x3494b8c9 )
	ROM_LOAD16_BYTE( "fl15",         0x40000, 0x20000, 0x1328354e )
	ROM_LOAD16_BYTE( "fl13",         0x40001, 0x20000, 0xe3b3955e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fl16",              0x00000, 0x10000, 0x66360bdf )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "fk_05.rom",         0x08000, 0x08000, 0x3cdb7453 )
	ROM_CONTINUE(                  0x00000, 0x08000 )	/* the two halves are swapped */
	ROM_LOAD( "fk_04.rom",         0x18000, 0x08000, 0x325ba20c )
	ROM_CONTINUE(                  0x10000, 0x08000 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fl09",              0x00000, 0x20000, 0x907d5910 )
	ROM_LOAD( "fl08",              0x20000, 0x20000, 0xa936c03c )
	ROM_LOAD( "fl07",              0x40000, 0x20000, 0x2068c45c )
	ROM_LOAD( "fl06",              0x60000, 0x20000, 0xb7241ab9 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fl11",              0x00000, 0x20000, 0xb86b73b4 )
	ROM_LOAD( "fl10",              0x20000, 0x20000, 0x92245b29 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fl01",              0x00000, 0x20000, 0x2c8b35a7 )
	ROM_LOAD( "fl03",              0x20000, 0x20000, 0x1eefed3c )
	ROM_LOAD( "fl00",              0x40000, 0x20000, 0x756fb801 )
	ROM_LOAD( "fl02",              0x60000, 0x20000, 0x54d2c120 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fl17",              0x00000, 0x20000, 0x9029965d )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "7114.prm",          0x0000, 0x0100, 0xeb539ffb )	/* timing? (not used) */
ROM_END

ROM_START( midresu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fl14",         0x00000, 0x20000, 0x2f9507a2 )
	ROM_LOAD16_BYTE( "fl12",         0x00001, 0x20000, 0x3815ad9f )
	ROM_LOAD16_BYTE( "fl15",         0x40000, 0x20000, 0x1328354e )
	ROM_LOAD16_BYTE( "fl13",         0x40001, 0x20000, 0xe3b3955e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fl16",              0x00000, 0x10000, 0x66360bdf )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "fl05",              0x08000, 0x08000, 0xd75aba06 )
	ROM_CONTINUE(                  0x00000, 0x08000 )	/* the two halves are swapped */
	ROM_LOAD( "fl04",              0x18000, 0x08000, 0x8f5bbb79 )
	ROM_CONTINUE(                  0x10000, 0x08000 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fl09",              0x00000, 0x20000, 0x907d5910 )
	ROM_LOAD( "fl08",              0x20000, 0x20000, 0xa936c03c )
	ROM_LOAD( "fl07",              0x40000, 0x20000, 0x2068c45c )
	ROM_LOAD( "fl06",              0x60000, 0x20000, 0xb7241ab9 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fl11",              0x00000, 0x20000, 0xb86b73b4 )
	ROM_LOAD( "fl10",              0x20000, 0x20000, 0x92245b29 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fl01",              0x00000, 0x20000, 0x2c8b35a7 )
	ROM_LOAD( "fl03",              0x20000, 0x20000, 0x1eefed3c )
	ROM_LOAD( "fl00",              0x40000, 0x20000, 0x756fb801 )
	ROM_LOAD( "fl02",              0x60000, 0x20000, 0x54d2c120 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fl17",              0x00000, 0x20000, 0x9029965d )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "7114.prm",          0x0000, 0x0100, 0xeb539ffb )	/* timing? (not used) */
ROM_END

ROM_START( midresj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fh14",         0x00000, 0x20000, 0x6d632a51 )
	ROM_LOAD16_BYTE( "fh12",         0x00001, 0x20000, 0x45143384 )
	ROM_LOAD16_BYTE( "fl15",         0x40000, 0x20000, 0x1328354e )
	ROM_LOAD16_BYTE( "fl13",         0x40001, 0x20000, 0xe3b3955e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fh16",              0x00000, 0x10000, 0x00736f32 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "fk_05.rom",         0x08000, 0x08000, 0x3cdb7453 )
	ROM_CONTINUE(                  0x00000, 0x08000 )	/* the two halves are swapped */
	ROM_LOAD( "fk_04.rom",         0x18000, 0x08000, 0x325ba20c )
	ROM_CONTINUE(                  0x10000, 0x08000 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fl09",              0x00000, 0x20000, 0x907d5910 )
	ROM_LOAD( "fl08",              0x20000, 0x20000, 0xa936c03c )
	ROM_LOAD( "fl07",              0x40000, 0x20000, 0x2068c45c )
	ROM_LOAD( "fl06",              0x60000, 0x20000, 0xb7241ab9 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fl11",              0x00000, 0x20000, 0xb86b73b4 )
	ROM_LOAD( "fl10",              0x20000, 0x20000, 0x92245b29 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fl01",              0x00000, 0x20000, 0x2c8b35a7 )
	ROM_LOAD( "fl03",              0x20000, 0x20000, 0x1eefed3c )
	ROM_LOAD( "fl00",              0x40000, 0x20000, 0x756fb801 )
	ROM_LOAD( "fl02",              0x60000, 0x20000, 0x54d2c120 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fh17",              0x00000, 0x20000, 0xc7b0a24e )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "7114.prm",          0x0000, 0x0100, 0xeb539ffb )	/* timing? (not used) */
ROM_END

ROM_START( bouldash )
	ROM_REGION( 0x60000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fn-15",   0x00000, 0x10000, 0xca19a967 )
	ROM_LOAD16_BYTE( "fn-12",   0x00001, 0x10000, 0x242bdc2a )
	ROM_LOAD16_BYTE( "fn-16",   0x20000, 0x10000, 0xb7217265 )
	ROM_LOAD16_BYTE( "fn-13",   0x20001, 0x10000, 0x19209ef4 )
	ROM_LOAD16_BYTE( "fn-17",   0x40000, 0x10000, 0x78a632a1 )
	ROM_LOAD16_BYTE( "fn-14",   0x40001, 0x10000, 0x69b6112d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "fn-10",      0x00000, 0x10000, 0xc74106e7 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "fn-04",        0x08000, 0x08000, 0x40f5a760 )
	ROM_CONTINUE(             0x00000, 0x08000 )	/* the two halves are swapped */
	ROM_LOAD( "fn-05",        0x18000, 0x08000, 0x824f2168 )
	ROM_CONTINUE(             0x10000, 0x08000 )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fn-07",        0x00000, 0x10000, 0xeac6a3b3 )
	ROM_LOAD( "fn-06",        0x10000, 0x10000, 0x3feee292 )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fn-09",        0x00000, 0x20000, 0xc2b27bd2 )
	ROM_LOAD( "fn-08",        0x20000, 0x20000, 0x5ac97178 )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fn-01",        0x00000, 0x10000, 0x9333121b )
	ROM_LOAD( "fn-03",        0x10000, 0x10000, 0x254ba60f )
	ROM_LOAD( "fn-00",        0x20000, 0x10000, 0xec18d098 )
	ROM_LOAD( "fn-02",        0x30000, 0x10000, 0x4f060cba )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "fn-11",      0x00000, 0x10000, 0x990fd8d9 )
ROM_END

/******************************************************************************/

GAMEX( 1987, hbarrel,  0,        hbarrel,  hbarrel,  hbarrel,  ROT270, "Data East USA",         "Heavy Barrel (US)", GAME_NO_COCKTAIL )
GAMEX( 1987, hbarrelw, hbarrel,  hbarrel,  hbarrel,  hbarrelw, ROT270, "Data East Corporation", "Heavy Barrel (World)", GAME_NO_COCKTAIL )
GAMEX( 1988, baddudes, 0,        baddudes, baddudes, baddudes, ROT0,   "Data East USA",         "Bad Dudes vs. Dragonninja (US)", GAME_NO_COCKTAIL )
GAMEX( 1988, drgninja, baddudes, baddudes, baddudes, baddudes, ROT0,   "Data East Corporation", "Dragonninja (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1988, birdtry,  0,        birdtry,  hbarrel,  birdtry,  ROT270, "Data East Corporation", "Birdie Try (Japan)", GAME_NOT_WORKING | GAME_NO_COCKTAIL)
GAMEX( 1988, robocop,  0,        robocop,  robocop,  robocop,  ROT0,   "Data East Corporation", "Robocop (World revision 3)", GAME_UNEMULATED_PROTECTION | GAME_NO_COCKTAIL )
GAMEX( 1988, robocopu, robocop,  robocop,  robocop,  robocop,  ROT0,   "Data East USA",         "Robocop (US revision 1)", GAME_UNEMULATED_PROTECTION | GAME_NO_COCKTAIL )
GAMEX( 1988, robocpu0, robocop,  robocop,  robocop,  robocop,  ROT0,   "Data East USA",         "Robocop (US revision 0)", GAME_UNEMULATED_PROTECTION | GAME_NO_COCKTAIL )
GAMEX( 1988, robocopb, robocop,  robocopb, robocop,  robocop,  ROT0,   "bootleg",               "Robocop (World bootleg)", GAME_NO_COCKTAIL )
GAMEX( 1989, hippodrm, 0,        hippodrm, hippodrm, hippodrm, ROT0,   "Data East USA",         "Hippodrome (US)", GAME_NO_COCKTAIL )
GAMEX( 1989, ffantasy, hippodrm, hippodrm, hippodrm, hippodrm, ROT0,   "Data East Corporation", "Fighting Fantasy (Japan revision 2)", GAME_NO_COCKTAIL )
GAMEX( 1989, ffantasa, hippodrm, hippodrm, hippodrm, hippodrm, ROT0,   "Data East Corporation", "Fighting Fantasy (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1989, slyspy,   0,        slyspy,   slyspy,   slyspy,   ROT0,   "Data East USA",         "Sly Spy (US revision 3)", GAME_NO_COCKTAIL )
GAMEX( 1989, slyspy2,  slyspy,   slyspy,   slyspy,   slyspy,   ROT0,   "Data East USA",         "Sly Spy (US revision 2)", GAME_NO_COCKTAIL )
GAMEX( 1989, secretag, slyspy,   slyspy,   slyspy,   slyspy,   ROT0,   "Data East Corporation", "Secret Agent (World)", GAME_NO_COCKTAIL )
GAMEX( 1989, secretab, slyspy,   slyspy,   slyspy,   slyspy,   ROT0,   "bootleg",               "Secret Agent (bootleg)", GAME_NO_COCKTAIL | GAME_NOT_WORKING )
GAMEX( 1989, midres,   0,        midres,   midres,   0,        ROT0,   "Data East Corporation", "Midnight Resistance (World)", GAME_NO_COCKTAIL )
GAMEX( 1989, midresu,  midres,   midres,   midres,   0,        ROT0,   "Data East USA",         "Midnight Resistance (US)", GAME_NO_COCKTAIL )
GAMEX( 1989, midresj,  midres,   midres,   midres,   0,        ROT0,   "Data East Corporation", "Midnight Resistance (Japan)", GAME_NO_COCKTAIL )
GAMEX( 1990, bouldash, 0,        slyspy,   bouldash, slyspy,   ROT0,   "Data East Corporation (licensed from First Star)", "Boulder Dash / Boulder Dash Part 2 (World)", GAME_NO_COCKTAIL )
