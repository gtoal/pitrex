/***************************************************************************

  Edward Randy      (c) 1990 Data East Corporation (World version)
  Edward Randy      (c) 1990 Data East Corporation (Japanese version)
  Caveman Ninja     (c) 1991 Data East Corporation (World version)
  Caveman Ninja     (c) 1991 Data East Corporation (USA version)
  Joe & Mac         (c) 1991 Data East Corporation (Japanese version)
  Robocop 2         (c) 1991 Data East Corporation (USA version)
  Stone Age         (Italian bootleg)

  Edward Randy runs on the same board as Caveman Ninja but the protection
  chip is different.  Robocop 2 also has a different protection chip but
  strangely makes very little use of it (only one check at the start).
  Robocop 2 is a different board but similar hardware.

  Robocop 2 road level is wrong (row offsets aren't written?!)
  Robocop 2 has a special graphics mode which displays high colour pictures
  in the attract mode (not implemented yet).

  The sound program of Stoneage is ripped from Block Out (by Technos!)

Caveman Ninja Issues:
  End of level 2 is corrupt.

  Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"
#include "cpu/z80/z80.h"

/* Video emulation definitions */
extern data16_t *cninja_pf1_rowscroll,*cninja_pf2_rowscroll;
extern data16_t *cninja_pf3_rowscroll,*cninja_pf4_rowscroll;
extern data16_t *cninja_pf1_data,*cninja_pf2_data;
extern data16_t *cninja_pf3_data,*cninja_pf4_data;

int  cninja_vh_start(void);
int  edrandy_vh_start(void);
int  robocop2_vh_start(void);
int  stoneage_vh_start(void);
void cninja_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void edrandy_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void robocop2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

WRITE16_HANDLER( cninja_pf1_data_w );
WRITE16_HANDLER( cninja_pf2_data_w );
WRITE16_HANDLER( cninja_pf3_data_w );
WRITE16_HANDLER( cninja_pf4_data_w );
WRITE16_HANDLER( cninja_control_0_w );
WRITE16_HANDLER( cninja_control_1_w );

WRITE16_HANDLER( cninja_palette_24bit_w );
WRITE16_HANDLER( robocop2_pri_w );

static data16_t loopback[0x80];
static data16_t *cninja_ram;

/**********************************************************************************/

static WRITE16_HANDLER( cninja_sound_w )
{
	soundlatch_w(0,data&0xff);
	cpu_cause_interrupt(1,H6280_INT_IRQ1);
}

static WRITE16_HANDLER( stoneage_sound_w )
{
	soundlatch_w(0,data&0xff);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}

static WRITE16_HANDLER( cninja_loopback_w )
{
	COMBINE_DATA(&loopback[offset]);
#if 0
	if ((offset>0x22 || offset<0x8) && (offset>0x94 || offset<0x80)
&& offset!=0x36 && offset!=0x9e && offset!=0x76 && offset!=0x58 && offset!=0x56
&& offset!=0x2c && offset!=0x34
&& (offset>0xb0 || offset<0xa0) /* in game prot writes */
)
logerror("Protection PC %06x: warning - write %04x to %04x\n",cpu_get_pc(),data,offset);
#endif
}

static READ16_HANDLER( cninja_prot_r )
{
 	switch (offset<<1) {
		case 0x80: /* Master level control */
			return loopback[0];

		case 0xde: /* Restart position control */
			return loopback[1];

		case 0xe6: /* The number of credits in the system. */
			return loopback[2];

		case 0x86: /* End of game check.  See 0x1814 */
			return loopback[3];

		/* Video registers */
		case 0x5a: /* Moved to 0x140000 on int */
			return loopback[8];
		case 0x84: /* Moved to 0x14000a on int */
			return loopback[9];
		case 0x20: /* Moved to 0x14000c on int */
			return loopback[10];
		case 0x72: /* Moved to 0x14000e on int */
			return loopback[11];
		case 0xdc: /* Moved to 0x150000 on int */
			return loopback[12];
		case 0x6e: /* Moved to 0x15000a on int */
			return loopback[13]; /* Not used on bootleg */
		case 0x6c: /* Moved to 0x15000c on int */
			return loopback[14];
		case 0x08: /* Moved to 0x15000e on int */
			return loopback[15];

		case 0x36: /* Dip switches */
			return (readinputport(3) + (readinputport(4) << 8));

		case 0x1c8: /* Coins */
			return readinputport(2);

		case 0x22c: /* Player 1 & 2 input ports */
			return (readinputport(0) + (readinputport(1) << 8));
	}
	logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",cpu_get_pc(),offset);
	return 0;
}

static READ16_HANDLER( edrandy_prot_r )
{
 	switch (offset<<1) {
		/* Video registers */
		case 0x32a: /* Moved to 0x140006 on int */
			return READ_WORD(&loopback[0x80]);
		case 0x380: /* Moved to 0x140008 on int */
			return READ_WORD(&loopback[0x84]);
		case 0x63a: /* Moved to 0x150002 on int */
			return READ_WORD(&loopback[0x88]);
		case 0x42a: /* Moved to 0x150004 on int */
			return READ_WORD(&loopback[0x8c]);
		case 0x030: /* Moved to 0x150006 on int */
			return READ_WORD(&loopback[0x90]);
		case 0x6b2: /* Moved to 0x150008 on int */
			return READ_WORD(&loopback[0x94]);




case 0x6c4: /* dma enable, bit 7 set, below bit 5 */
case 0x33e: return READ_WORD(&loopback[0x2c]); /* allows video registers */




		/* memcpy selectors, transfer occurs in interrupt */
		case 0x32e: return READ_WORD(&loopback[0x8]); /* src msb */
		case 0x6d8: return READ_WORD(&loopback[0xa]); /* src lsb */
		case 0x010: return READ_WORD(&loopback[0xc]); /* dst msb */
		case 0x07a: return READ_WORD(&loopback[0xe]); /* src lsb */

		case 0x37c: return READ_WORD(&loopback[0x10]); /* src msb */
		case 0x250: return READ_WORD(&loopback[0x12]);
		case 0x04e: return READ_WORD(&loopback[0x14]);
		case 0x5ba: return READ_WORD(&loopback[0x16]);
		case 0x5f4: return READ_WORD(&loopback[0x18]); /* length */

		case 0x38c: return READ_WORD(&loopback[0x1a]); /* src msb */
		case 0x02c: return READ_WORD(&loopback[0x1c]);
		case 0x1e6: return READ_WORD(&loopback[0x1e]);
		case 0x3e4: return READ_WORD(&loopback[0x20]);
		case 0x174: return READ_WORD(&loopback[0x22]); /* length */

		/* Player 1 & 2 controls, read in IRQ then written *back* to protection device */
		case 0x50: /* written to 9e byte */
			return readinputport(0);
		case 0x6f8: /* written to 76 byte */
			return readinputport(1);
		/* Controls are *really* read here! */
		case 0x6fa:
			return (READ_WORD(&loopback[0x9e])&0xff00) | ((READ_WORD(&loopback[0x76])>>8)&0xff);
		/* These two go to the low bytes of 9e and 76.. */
		case 0xc6: return 0;
		case 0x7bc: return 0;
		case 0x5c: /* After coin insert, high 0x8000 bit set starts game */
			return READ_WORD(&loopback[0x76]);
		case 0x3a6: /* Top byte OR'd with above, masked to 7 */
			return READ_WORD(&loopback[0x9e]);

/*		case 0xac:    Dip switches    */

		case 0xc2: /* Dip switches */
			return (readinputport(3) + (readinputport(4) << 8));
		case 0x5d4: /* The state of the dips _last_ frame */
			return READ_WORD(&loopback[0x34]);

		case 0x76a: /* Coins */
			return readinputport(2);



case 0x156: /* Interrupt regulate */

logerror("Int stop %04x\n",READ_WORD(&loopback[0x1a]));

cpu_spinuntil_int();
/*return readinputport(2); */

	/* 4058 or 4056? */
	return READ_WORD(&loopback[0x36])>>8;




#if 0

/* 5d4 == 80 when coin is inserted */

/* 284 == LOOKUP TABLE IMPROTANT (0 4 8 etc */
case 0x284: return 0;



case 0x2f6: /* btst 2, jsr 11a if set */
if (READ_WORD(&loopback[0x40])) return 0xffff;
return 0;/*READ_WORD(&loopback[0x40]);//0; */


/*case 0x5d4: */
/*case 0xc2: return READ_WORD(&loopback[0x40]);//0;    Screen flip related?!    */




/*case 0x33e: */
/*case 0x6c4: */
/*	case 0x6f8:    Player 1 & 2 input ports    */


/* return value 36 is important */


/*

6006 written to 0x40 at end of japan screen


*/



/*case 0xc2: */
/*case 0x5c:    9cde    */
/*case 0x5d4: */
/*return READ_WORD(&loopback[0x40]);//0; */

/*case 0x6fa: */





/* in game prot */
case 0x102: return READ_WORD(&loopback[0xa0]);
case 0x15a: return READ_WORD(&loopback[0xa2]);
case 0x566: return READ_WORD(&loopback[0xa4]);
case 0xd2: return READ_WORD(&loopback[0xa6]);
case 0x4a6: return READ_WORD(&loopback[0xa8]);
case 0x3dc: return READ_WORD(&loopback[0xb0]);
case 0x2a0: return READ_WORD(&loopback[0xb2]);
/*case 0x392: return READ_WORD(&loopback[0xa0]); */


		case 0x3b2: return READ_WORD(&loopback[0xd0]);

/* Enemy power related HIGH WORD*/
		case 0x440: return READ_WORD(&loopback[0xd2]);/* Enemy power related LOW WORD*/
			return 0;

/*case 0x6fa: */
/*return rand()%0xffff; */

/*case 0x5d4:    Top byte:  Player 1?  lsl.4 then mask 3 for fire buttons?    */
/*case 0x5d4: */

		return ~0;/*(readinputport(0) + (readinputport(1) << 8)); */
#endif
	}

/*	logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",cpu_get_pc(),offset); */
	return 0;
}

#if 0
static WRITE_HANDLER( log_m_w )
{
	logerror("INTERRUPT %06x: warning - write address %04x\n",cpu_get_pc(),offset);

}
#endif

static READ16_HANDLER( robocop2_prot_r )
{
 	switch (offset<<1) {
		case 0x320: /* Coins */
			return readinputport(2);
		case 0x4e6: /* Dip switches */
			return (readinputport(3) + (readinputport(4) << 8));
		case 0x41a: /* Player 1 & 2 input ports */
			return (readinputport(0) + (readinputport(1) << 8));
		case 0x504: /* PC: 6b6.  b4, 2c, 36 written before read */
			logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",cpu_get_pc(),offset);
			return 0x84;
	}
	logerror("Protection PC %06x: warning - read unmapped memory address %04x\n",cpu_get_pc(),offset);
	return 0;
}

/**********************************************************************************/

static MEMORY_READ16_START( cninja_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x144000, 0x144fff, MRA16_RAM },
	{ 0x15c000, 0x15c7ff, MRA16_RAM },
	{ 0x184000, 0x187fff, MRA16_RAM },
	{ 0x190004, 0x190005, MRA16_NOP }, /* Seemingly unused */
	{ 0x19c000, 0x19dfff, MRA16_RAM },
	{ 0x1a4000, 0x1a47ff, MRA16_RAM }, /* Sprites */
	{ 0x1bc000, 0x1bcfff, cninja_prot_r }, /* Protection device */
MEMORY_END

static MEMORY_WRITE16_START( cninja_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },

	{ 0x140000, 0x14000f, cninja_control_1_w },
	{ 0x144000, 0x144fff, cninja_pf1_data_w, &cninja_pf1_data },
	{ 0x146000, 0x146fff, cninja_pf4_data_w, &cninja_pf4_data },
	{ 0x14c000, 0x14c7ff, MWA16_RAM, &cninja_pf1_rowscroll },
	{ 0x14e000, 0x14e7ff, MWA16_RAM, &cninja_pf4_rowscroll },

	{ 0x150000, 0x15000f, cninja_control_0_w },
	{ 0x154000, 0x154fff, cninja_pf3_data_w, &cninja_pf3_data },
	{ 0x156000, 0x156fff, cninja_pf2_data_w, &cninja_pf2_data },
	{ 0x15c000, 0x15c7ff, MWA16_RAM, &cninja_pf3_rowscroll },
	{ 0x15e000, 0x15e7ff, MWA16_RAM, &cninja_pf2_rowscroll },

	{ 0x184000, 0x187fff, MWA16_RAM, &cninja_ram }, /* Main ram */
	{ 0x190000, 0x190007, MWA16_NOP }, /* IRQ Ack + DMA flags? */
	{ 0x19c000, 0x19dfff, cninja_palette_24bit_w, &paletteram16 },
	{ 0x1a4000, 0x1a47ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x1b4000, 0x1b4001, buffer_spriteram16_w }, /* DMA flag */
	{ 0x1bc000, 0x1bc0ff, cninja_loopback_w }, /* Protection writes */
	{ 0x308000, 0x308fff, MWA16_NOP }, /* Bootleg only */
MEMORY_END

static MEMORY_READ16_START( edrandy_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x144000, 0x144fff, MRA16_RAM },
	{ 0x15c000, 0x15c7ff, MRA16_RAM },
	{ 0x188000, 0x189fff, MRA16_RAM },
	{ 0x194000, 0x197fff, MRA16_RAM },
	{ 0x198000, 0x198fff, edrandy_prot_r }, /* Protection device */
	{ 0x1bc000, 0x1bc7ff, MRA16_RAM }, /* Sprites */
MEMORY_END

static MEMORY_WRITE16_START( edrandy_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },

	{ 0x140000, 0x14000f, cninja_control_1_w },
	{ 0x144000, 0x144fff, cninja_pf1_data_w, &cninja_pf1_data },
	{ 0x146000, 0x146fff, cninja_pf4_data_w, &cninja_pf4_data },
	{ 0x14c000, 0x14c7ff, MWA16_RAM, &cninja_pf1_rowscroll },
	{ 0x14e000, 0x14e7ff, MWA16_RAM, &cninja_pf4_rowscroll },

	{ 0x150000, 0x15000f, cninja_control_0_w },
	{ 0x154000, 0x154fff, cninja_pf3_data_w, &cninja_pf3_data },
	{ 0x156000, 0x156fff, cninja_pf2_data_w, &cninja_pf2_data },
	{ 0x15c000, 0x15c7ff, MWA16_RAM, &cninja_pf3_rowscroll },
	{ 0x15e000, 0x15e7ff, MWA16_RAM, &cninja_pf2_rowscroll },

	{ 0x188000, 0x189fff, cninja_palette_24bit_w, &paletteram16 },
	{ 0x194000, 0x197fff, MWA16_RAM, &cninja_ram }, /* Main ram */
	{ 0x198064, 0x198065, cninja_sound_w }, /* Soundlatch is amongst protection */
	{ 0x198000, 0x1980ff, cninja_loopback_w }, /* Protection writes */
	{ 0x1a4000, 0x1a4007, MWA16_NOP }, /* IRQ Ack + DMA flags? */
	{ 0x1ac000, 0x1ac001, buffer_spriteram16_w }, /* DMA flag */
	{ 0x1bc000, 0x1bc7ff, MWA16_RAM, &spriteram16, &spriteram_size },
MEMORY_END

static MEMORY_READ16_START( robocop2_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x180000, 0x1807ff, MRA16_RAM },
	{ 0x18c000, 0x18c7ff, robocop2_prot_r }, /* Protection device */
	{ 0x1a8000, 0x1a9fff, MRA16_RAM },
	{ 0x1b8000, 0x1bbfff, MRA16_RAM },
	{ 0x144000, 0x144fff, MRA16_RAM },
	{ 0x146000, 0x146fff, MRA16_RAM },
	{ 0x154000, 0x154fff, MRA16_RAM },
	{ 0x156000, 0x156fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( robocop2_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x140000, 0x14000f, cninja_control_1_w },
	{ 0x144000, 0x144fff, cninja_pf1_data_w, &cninja_pf1_data },
	{ 0x146000, 0x146fff, cninja_pf4_data_w, &cninja_pf4_data },
	{ 0x14c000, 0x14c7ff, MWA16_RAM, &cninja_pf1_rowscroll },
	{ 0x14e000, 0x14e7ff, MWA16_RAM, &cninja_pf4_rowscroll },

	{ 0x150000, 0x15000f, cninja_control_0_w },
	{ 0x154000, 0x154fff, cninja_pf3_data_w, &cninja_pf3_data },
	{ 0x156000, 0x156fff, cninja_pf2_data_w, &cninja_pf2_data },
	{ 0x15c000, 0x15c7ff, MWA16_RAM, &cninja_pf3_rowscroll },
	{ 0x15e000, 0x15e7ff, MWA16_RAM, &cninja_pf2_rowscroll },
/*check background layer - wrong row offset??!? */
	{ 0x180000, 0x1807ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x18c064, 0x18c065, cninja_sound_w },
/*	{ 0x18c000, 0x18c0ff, cninja_loopback_w },    Protection writes    */
	{ 0x198000, 0x198001, buffer_spriteram16_w }, /* DMA flag */
	{ 0x1a8000, 0x1a9fff, cninja_palette_24bit_w, &paletteram16 },
	{ 0x1b0004, 0x1b0005, MWA16_NOP }, /* ? */
	{ 0x1b8000, 0x1bbfff, MWA16_RAM, &cninja_ram }, /* Main ram */
	{ 0x1f0000, 0x1f0001, robocop2_pri_w },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM2151_w )
{
	switch (offset) {
	case 0:
		YM2151_register_port_0_w(0,data);
		break;
	case 1:
		YM2151_data_port_0_w(0,data);
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

static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, YM2203_status_port_0_r },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, YM2203_w },
	{ 0x110000, 0x110001, YM2151_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

static MEMORY_READ_START( stoneage_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
MEMORY_END

static MEMORY_WRITE_START( stoneage_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

/**********************************************************************************/

#define PORTS_COINS \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK ) \
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( cninja )

	PORTS_COINS

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( cninjau )

	PORTS_COINS

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Credit(s) to Start" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
/* Also, if Coin A and B are on 1/1, 0x00 gives 2 to start, 1 to continue */

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( robocop2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/**********************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 0x08000*8, 0x18000*8, 0x00000*8, 0x10000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 16, 0, 24, 8 },
	{ 64*8+0, 64*8+1, 64*8+2, 64*8+3, 64*8+4, 64*8+5, 64*8+6, 64*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0,  },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 16 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,  512, 64 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout,  256, 16 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout,768, 32 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/**********************************************************************************/

static struct YM2203interface ym2203_interface =
{
	1,
	32220000/8, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM2203_VOL(60,60) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static void sound_irq2(int state)
{
	cpu_set_irq_line(1,0,state);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	/* the second OKIM6295 ROM is bank switched */
	OKIM6295_set_bank_base(1, (data & 1) * 0x40000);
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct YM2151interface ym2151_interface2 =
{
	1,
	3579545,	/* 3.579545 MHz (?) */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ sound_irq2 }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 75, 60 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};

/**********************************************************************************/

static struct MachineDriver machine_driver_cninja =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,
			cninja_readmem,cninja_writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_H6280 | CPU_AUDIO_CPU,
			32220000/8,	/* Accurate */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM,
	0,
	cninja_vh_start,
	0,
	cninja_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono */
  	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_stoneage =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,
			cninja_readmem,cninja_writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,
			stoneage_s_readmem,stoneage_s_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM,
	0,
	stoneage_vh_start,
	0,
	cninja_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono */
	{
		{
			SOUND_YM2151,
			&ym2151_interface2
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_edrandy =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,
			edrandy_readmem,edrandy_writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_H6280 | CPU_AUDIO_CPU,
			32220000/8,	/* Accurate */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	58, 529,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM,
	0,
	edrandy_vh_start,
	0,
	edrandy_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono */
  	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_robocop2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			14000000,
			robocop2_readmem,robocop2_writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_H6280 | CPU_AUDIO_CPU,
			32220000/8,	/* Accurate */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, 529,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM,
	0,
	robocop2_vh_start,
	0,
	robocop2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0, /* Mono? */
  	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/**********************************************************************************/

ROM_START( cninja )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gn02rev3.bin", 0x00000, 0x20000, 0x39aea12a )
	ROM_LOAD16_BYTE( "gn05rev2.bin", 0x00001, 0x20000, 0x0f4360ef )
	ROM_LOAD16_BYTE( "gn01rev2.bin", 0x40000, 0x20000, 0xf740ef7e )
	ROM_LOAD16_BYTE( "gn04rev2.bin", 0x40001, 0x20000, 0xc98fcb62 )
	ROM_LOAD16_BYTE( "gn-00.rom",    0x80000, 0x20000, 0x0b110b16 )
	ROM_LOAD16_BYTE( "gn-03.rom",    0x80001, 0x20000, 0x1e28e697 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gl-07.rom",  0x00000,  0x10000,  0xca8bef96 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gl-08.rom",  0x00000,  0x10000,  0x33a2b400 )	/* chars */
	ROM_LOAD( "gl-09.rom",  0x10000,  0x10000,  0x5a2d4752 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  0xa8f05d33 )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  0x5b399eed )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  0xde89c69a )	/* tiles 3 */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  0x2220eb9f )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  0x56a53254 )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  0x144b94cc )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  0x82d44749 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gl-06.rom",  0x00000,  0x20000,  0xd92e519d )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mag-07.rom", 0x00000,  0x80000,  0x08eb5264 )	/* banked */
ROM_END

ROM_START( cninja0 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gn-02.rom", 0x00000, 0x20000, 0xccc59524 )
	ROM_LOAD16_BYTE( "gn-05.rom", 0x00001, 0x20000, 0xa002cbe4 )
	ROM_LOAD16_BYTE( "gn-01.rom", 0x40000, 0x20000, 0x18f0527c )
	ROM_LOAD16_BYTE( "gn-04.rom", 0x40001, 0x20000, 0xea4b6d53 )
	ROM_LOAD16_BYTE( "gn-00.rom", 0x80000, 0x20000, 0x0b110b16 )
	ROM_LOAD16_BYTE( "gn-03.rom", 0x80001, 0x20000, 0x1e28e697 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gl-07.rom",  0x00000,  0x10000,  0xca8bef96 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gl-08.rom",  0x00000,  0x10000,  0x33a2b400 )	/* chars */
	ROM_LOAD( "gl-09.rom",  0x10000,  0x10000,  0x5a2d4752 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  0xa8f05d33 )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  0x5b399eed )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  0xde89c69a )	/* tiles 3 */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  0x2220eb9f )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  0x56a53254 )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  0x144b94cc )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  0x82d44749 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gl-06.rom",  0x00000,  0x20000,  0xd92e519d )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mag-07.rom", 0x00000,  0x80000,  0x08eb5264 )	/* banked */
ROM_END

ROM_START( cninjau )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gm02-3.1k", 0x00000, 0x20000, 0xd931c3b1 )
	ROM_LOAD16_BYTE( "gm05-2.3k", 0x00001, 0x20000, 0x7417d3fb )
	ROM_LOAD16_BYTE( "gm01-2.1j", 0x40000, 0x20000, 0x72041f7e )
	ROM_LOAD16_BYTE( "gm04-2.3j", 0x40001, 0x20000, 0x2104d005 )
	ROM_LOAD16_BYTE( "gn-00.rom", 0x80000, 0x20000, 0x0b110b16 )
	ROM_LOAD16_BYTE( "gn-03.rom", 0x80001, 0x20000, 0x1e28e697 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gl-07.rom",  0x00000,  0x10000,  0xca8bef96 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gl-08.rom",  0x00000,  0x10000,  0x33a2b400 )	/* chars */
	ROM_LOAD( "gl-09.rom",  0x10000,  0x10000,  0x5a2d4752 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  0xa8f05d33 )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  0x5b399eed )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  0xde89c69a )	/* tiles 3 */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  0x2220eb9f )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  0x56a53254 )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  0x144b94cc )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  0x82d44749 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gl-06.rom",  0x00000,  0x20000,  0xd92e519d )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mag-07.rom", 0x00000,  0x80000,  0x08eb5264 )	/* banked */
ROM_END

ROM_START( joemac )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gl02-2.k1", 0x00000, 0x20000,  0x80da12e2 )
	ROM_LOAD16_BYTE( "gl05-2.k3", 0x00001, 0x20000,  0xfe4dbbbb )
	ROM_LOAD16_BYTE( "gl01-2.j1", 0x40000, 0x20000,  0x0b245307 )
	ROM_LOAD16_BYTE( "gl04-2.j3", 0x40001, 0x20000,  0x1b331f61 )
	ROM_LOAD16_BYTE( "gn-00.rom", 0x80000, 0x20000,  0x0b110b16 )
	ROM_LOAD16_BYTE( "gn-03.rom", 0x80001, 0x20000,  0x1e28e697 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gl-07.rom",  0x00000,  0x10000,  0xca8bef96 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gl-08.rom",  0x00000,  0x10000,  0x33a2b400 )	/* chars */
	ROM_LOAD( "gl-09.rom",  0x10000,  0x10000,  0x5a2d4752 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  0xa8f05d33 )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  0x5b399eed )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  0xde89c69a )	/* tiles 3 */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  0x2220eb9f )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  0x56a53254 )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  0x144b94cc )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  0x82d44749 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gl-06.rom",  0x00000,  0x20000,  0xd92e519d )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mag-07.rom", 0x00000,  0x80000,  0x08eb5264 )	/* banked */
ROM_END

ROM_START( stoneage )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "sa_1_019.bin", 0x00000, 0x20000,  0x7fb8c44f )
	ROM_LOAD16_BYTE( "sa_1_033.bin", 0x00001, 0x20000,  0x961c752b )
	ROM_LOAD16_BYTE( "sa_1_018.bin", 0x40000, 0x20000,  0xa4043022 )
	ROM_LOAD16_BYTE( "sa_1_032.bin", 0x40001, 0x20000,  0xf52a3286 )
	ROM_LOAD16_BYTE( "sa_1_017.bin", 0x80000, 0x20000,  0x08d6397a )
	ROM_LOAD16_BYTE( "sa_1_031.bin", 0x80001, 0x20000,  0x103079f5 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "sa_1_012.bin",  0x00000,  0x10000, 0x56058934 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gl-08.rom",  0x00000,  0x10000,  0x33a2b400 )	/* chars */
	ROM_LOAD( "gl-09.rom",  0x10000,  0x10000,  0x5a2d4752 )

	/* The bootleg graphics are stored in a different arrangement but
		seem to be the same as the original set */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  0xa8f05d33 )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  0x5b399eed )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  0xde89c69a )	/* tiles 3 */

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  0x2220eb9f )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  0x56a53254 )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  0x144b94cc )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  0x82d44749 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "sa_1_069.bin",  0x00000,  0x40000, 0x2188f3ca )

	/* No extra Oki samples in the bootleg */
ROM_END

ROM_START( edrandy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
  	ROM_LOAD16_BYTE( "gg-00-2", 0x00000, 0x20000, 0xce1ba964 )
  	ROM_LOAD16_BYTE( "gg-04-2", 0x00001, 0x20000, 0x24caed19 )
	ROM_LOAD16_BYTE( "gg-01-2", 0x40000, 0x20000, 0x33677b80 )
 	ROM_LOAD16_BYTE( "gg-05-2", 0x40001, 0x20000, 0x79a68ca6 )
	ROM_LOAD16_BYTE( "ge-02",   0x80000, 0x20000, 0xc2969fbb )
	ROM_LOAD16_BYTE( "ge-06",   0x80001, 0x20000, 0x5c2e6418 )
	ROM_LOAD16_BYTE( "ge-03",   0xc0000, 0x20000, 0x5e7b19a8 )
	ROM_LOAD16_BYTE( "ge-07",   0xc0001, 0x20000, 0x5eb819a1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "ge-09",    0x00000, 0x10000, 0x9f94c60b )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gg-11",    0x000000, 0x10000, 0xee567448 )
  	ROM_LOAD( "gg-10",    0x010000, 0x10000, 0xb96c6cbe )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-00",   0x000000, 0x40000, 0x3735b22d ) /* tiles 1 */
	ROM_CONTINUE(         0x080000, 0x40000 )
	ROM_LOAD( "mad-01",   0x040000, 0x40000, 0x7bb13e1c ) /* tiles 2 */
	ROM_CONTINUE(         0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-02",   0x000000, 0x80000, 0x6c76face ) /* tiles 3 */

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mad-03",   0x000000, 0x80000, 0xc0bff892 ) /* sprites */
	ROM_LOAD16_BYTE( "mad-05",   0x000001, 0x80000, 0x3f2ccf95 )
	ROM_LOAD16_BYTE( "mad-04",   0x100000, 0x80000, 0x464f3eb9 )
	ROM_LOAD16_BYTE( "mad-06",   0x100001, 0x80000, 0x60871f77 )
	ROM_LOAD16_BYTE( "mad-07",   0x200000, 0x80000, 0xac03466e )
	ROM_LOAD16_BYTE( "mad-08",   0x200001, 0x80000, 0x1b420ec8 )
	ROM_LOAD16_BYTE( "mad-10",   0x300000, 0x80000, 0x42da8ef0 )
	ROM_LOAD16_BYTE( "mad-11",   0x300001, 0x80000, 0x03c1f982 )
	ROM_LOAD16_BYTE( "mad-09",   0x400000, 0x80000, 0x930f4900 )
	ROM_LOAD16_BYTE( "mad-12",   0x400001, 0x80000, 0xa0bd62b6 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ge-08",    0x00000, 0x20000, 0xdfe28c7b )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mad-13", 0x00000, 0x80000, 0x6ab28eba )	/* banked */
ROM_END

ROM_START( edrandyj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
  	ROM_LOAD16_BYTE( "ge-00-2",   0x00000, 0x20000, 0xb3d2403c )
  	ROM_LOAD16_BYTE( "ge-04-2",   0x00001, 0x20000, 0x8a9624d6 )
	ROM_LOAD16_BYTE( "ge-01-2",   0x40000, 0x20000, 0x84360123 )
 	ROM_LOAD16_BYTE( "ge-05-2",   0x40001, 0x20000, 0x0bf85d9d )
	ROM_LOAD16_BYTE( "ge-02",     0x80000, 0x20000, 0xc2969fbb )
	ROM_LOAD16_BYTE( "ge-06",     0x80001, 0x20000, 0x5c2e6418 )
	ROM_LOAD16_BYTE( "ge-03",     0xc0000, 0x20000, 0x5e7b19a8 )
	ROM_LOAD16_BYTE( "ge-07",     0xc0001, 0x20000, 0x5eb819a1 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "ge-09",    0x00000, 0x10000, 0x9f94c60b )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ge-10",    0x000000, 0x10000, 0x2528d795 )
  	ROM_LOAD( "ge-11",    0x010000, 0x10000, 0xe34a931e )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-00",   0x000000, 0x40000, 0x3735b22d ) /* tiles 1 */
	ROM_CONTINUE(         0x080000, 0x40000 )
	ROM_LOAD( "mad-01",   0x040000, 0x40000, 0x7bb13e1c ) /* tiles 2 */
	ROM_CONTINUE(         0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-02",   0x000000, 0x80000, 0x6c76face ) /* tiles 3 */

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mad-03",   0x000000, 0x80000, 0xc0bff892 ) /* sprites */
	ROM_LOAD16_BYTE( "mad-05",   0x000001, 0x80000, 0x3f2ccf95 )
	ROM_LOAD16_BYTE( "mad-04",   0x100000, 0x80000, 0x464f3eb9 )
	ROM_LOAD16_BYTE( "mad-06",   0x100001, 0x80000, 0x60871f77 )
	ROM_LOAD16_BYTE( "mad-07",   0x200000, 0x80000, 0xac03466e )
	ROM_LOAD16_BYTE( "mad-08",   0x200001, 0x80000, 0x1b420ec8 )
	ROM_LOAD16_BYTE( "mad-10",   0x300000, 0x80000, 0x42da8ef0 )
	ROM_LOAD16_BYTE( "mad-11",   0x300001, 0x80000, 0x03c1f982 )
	ROM_LOAD16_BYTE( "mad-09",   0x400000, 0x80000, 0x930f4900 )
	ROM_LOAD16_BYTE( "mad-12",   0x400001, 0x80000, 0xa0bd62b6 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ge-08",    0x00000, 0x20000, 0xdfe28c7b )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mad-13", 0x00000, 0x80000, 0x6ab28eba )	/* banked */
ROM_END

ROM_START( robocop2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "robo03.k1", 0x00000, 0x20000, 0xf4c96cc9 )
	ROM_LOAD16_BYTE( "robo07.k3", 0x00001, 0x20000, 0x11e53a7c )
	ROM_LOAD16_BYTE( "robo02.j1", 0x40000, 0x20000, 0xfa086a0d )
	ROM_LOAD16_BYTE( "robo06.j3", 0x40001, 0x20000, 0x703b49d0 )
	ROM_LOAD16_BYTE( "robo01.h1", 0x80000, 0x20000, 0xab5356c0 )
	ROM_LOAD16_BYTE( "robo05.h3", 0x80001, 0x20000, 0xce21bda5 )
	ROM_LOAD16_BYTE( "robo00.f1", 0xc0000, 0x20000, 0xa93369ea )
	ROM_LOAD16_BYTE( "robo04.f3", 0xc0001, 0x20000, 0xee2f6ad9 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gp-09.k13",  0x00000,  0x10000,  0x4a4e0f8d )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gp10-1.y6",  0x00000,  0x10000,  0xd25d719c )	/* chars */
	ROM_LOAD( "gp11-1.z6",  0x10000,  0x10000,  0x030ded47 )

	ROM_REGION( 0x180000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mah-01.z1", 0x000000, 0x40000,  0x26e0dfff )
	ROM_CONTINUE(          0x0c0000, 0x40000 )
	ROM_LOAD( "mah-00.y1", 0x040000, 0x40000,  0x7bd69e41 )
	ROM_CONTINUE(          0x100000, 0x40000 )
	ROM_LOAD( "mah-02.a1", 0x080000, 0x40000,  0x328a247d )
	ROM_CONTINUE(          0x140000, 0x40000 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mah-04.z4", 0x000000, 0x40000,  0x9b6ca18c )
	ROM_CONTINUE(          0x080000, 0x40000 )
	ROM_LOAD( "mah-03.y4", 0x040000, 0x40000,  0x37894ddc )
	ROM_CONTINUE(          0x0c0000, 0x40000 )

	ROM_REGION( 0x300000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mah-05.y9",  0x000000, 0x80000,  0x6773e613 )
	ROM_LOAD16_BYTE( "mah-08.y12", 0x000001, 0x80000,  0x88d310a5 )
	ROM_LOAD16_BYTE( "mah-06.z9",  0x100000, 0x80000,  0x27a8808a )
	ROM_LOAD16_BYTE( "mah-09.z12", 0x100001, 0x80000,  0xa58c43a7 )
	ROM_LOAD16_BYTE( "mah-07.a9",  0x200000, 0x80000,  0x526f4190 )
	ROM_LOAD16_BYTE( "mah-10.a12", 0x200001, 0x80000,  0x14b770da )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gp-08.j13",  0x00000,  0x20000,  0x365183b1 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mah-11.f13", 0x00000,  0x80000,  0x642bc692 )	/* banked */
ROM_END

/**********************************************************************************/

static void cninja_patch(void)
{
	data16_t *RAM = (UINT16 *)memory_region(REGION_CPU1);
	int i;

	for (i=0; i<0x80000/2; i++) {
		int aword=RAM[i];

		if (aword==0x66ff || aword==0x67ff) {
			data16_t doublecheck=RAM[i-4];

			/* Cmpi + btst controlling opcodes */
			if (doublecheck==0xc39 || doublecheck==0x839) {
				RAM[i]=0x4E71;
		        RAM[i-1]=0x4E71;
		        RAM[i-2]=0x4E71;
		        RAM[i-3]=0x4E71;
		        RAM[i-4]=0x4E71;
			}
		}
	}
}

#if 0
static void edrandyj_patch(void)
{
/*	unsigned char *RAM = memory_region(REGION_CPU1); */

/*	WRITE_WORD (&RAM[0x98cc],0x4E71); */
/*	WRITE_WORD (&RAM[0x98ce],0x4E71); */

}
#endif

/**********************************************************************************/

static void init_cninja(void)
{
	install_mem_write16_handler(0, 0x1bc0a8, 0x1bc0a9, cninja_sound_w);
	cninja_patch();
}

static void init_stoneage(void)
{
	install_mem_write16_handler(0, 0x1bc0a8, 0x1bc0a9, stoneage_sound_w);
}

/**********************************************************************************/

GAMEX(1990, edrandy,  0,       edrandy,  cninja,  0,        ROT0_16BIT, "Data East Corporation", "Edward Randy (World)", GAME_UNEMULATED_PROTECTION )
GAMEX(1990, edrandyj, edrandy, edrandy,  cninja,  0,        ROT0_16BIT, "Data East Corporation", "Edward Randy (Japan)", GAME_UNEMULATED_PROTECTION )
GAME( 1991, cninja,   0,       cninja,   cninja,  cninja,   ROT0,       "Data East Corporation", "Caveman Ninja (World revision 3)" )
GAME( 1991, cninja0,  cninja,  cninja,   cninja,  cninja,   ROT0,       "Data East Corporation", "Caveman Ninja (World revision 0)" )
GAME( 1991, cninjau,  cninja,  cninja,   cninjau, cninja,   ROT0,       "Data East Corporation", "Caveman Ninja (US)" )
GAME( 1991, joemac,   cninja,  cninja,   cninja,  cninja,   ROT0,       "Data East Corporation", "Joe & Mac (Japan)" )
GAME( 1991, stoneage, cninja,  stoneage, cninja,  stoneage, ROT0,       "bootleg", "Stoneage" )
GAMEX(1991, robocop2, 0,       robocop2, robocop2,0,        ROT0_16BIT, "Data East Corporation", "Robocop 2 (US)", GAME_IMPERFECT_GRAPHICS )
