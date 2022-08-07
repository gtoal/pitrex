/***************************************************************************

  Capcom System 1
  ===============

  Driver provided by:
  Paul Leaman (paul@vortexcomputing.demon.co.uk)

  68000 for game, Z80, YM-2151 and OKIM6295 for sound.

  68000 clock speeds are unknown for all games (except where commented)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"

#include "cps1.h"       /* External CPS1 definitions */

/* in machine/kabuki.c */
void wof_decode(void);
void dino_decode(void);
void punisher_decode(void);
void slammast_decode(void);



static READ16_HANDLER( cps1_input2_r )
{
	int buttons=readinputport(5);
	return buttons << 8 | buttons;
}

static READ16_HANDLER( cps1_input3_r )
{
    int buttons=readinputport(6);
	return buttons << 8 | buttons;
}


static int cps1_sound_fade_timer;

static WRITE_HANDLER( cps1_snd_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int length = memory_region_length(REGION_CPU2) - 0x10000;
	int bankaddr;

	bankaddr = (data * 0x4000) & (length-1);
	cpu_setbank(1,&RAM[0x10000 + bankaddr]);

	if (data & 0xfe) logerror("%04x: write %02x to f004\n",cpu_get_pc(),data);
}

static WRITE16_HANDLER( cps1_sound_fade_w )
{
	if (ACCESSING_LSB)
		cps1_sound_fade_timer = data & 0xff;
}

static READ_HANDLER( cps1_snd_fade_timer_r )
{
	return cps1_sound_fade_timer;
}

static WRITE16_HANDLER( cps1_sound_command_w )
{
	if (ACCESSING_LSB)
		soundlatch_w(0,data & 0xff);
}

static READ16_HANDLER( cps1_input_r )
{
	int control=readinputport(offset);
	return (control<<8) | control;
}

static int dial[2];

static READ16_HANDLER( forgottn_dial_0_r )
{
	return ((readinputport(5) - dial[0]) >> (8*offset)) & 0xff;
}

static READ16_HANDLER( forgottn_dial_1_r )
{
	return ((readinputport(6) - dial[1]) >> (8*offset)) & 0xff;
}

static WRITE16_HANDLER( forgottn_dial_0_reset_w )
{
	dial[0] = readinputport(5);
}

static WRITE16_HANDLER( forgottn_dial_1_reset_w )
{
	dial[1] = readinputport(6);
}

static WRITE16_HANDLER( cps1_coinctrl_w )
{
/*	usrintf_showmessage("coinctrl %04x",data); */

	if (ACCESSING_MSB)
	{
		coin_counter_w(0,data & 0x0100);
		coin_counter_w(1,data & 0x0200);
		coin_lockout_w(0,~data & 0x0400);
		coin_lockout_w(1,~data & 0x0800);
	}

	if (ACCESSING_LSB)
	{
		/* mercs sets bit 0 */
		set_led_status(0,data & 0x02);
		set_led_status(1,data & 0x04);
		set_led_status(2,data & 0x08);
	}
}

static WRITE16_HANDLER( cpsq_coinctrl2_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(2,data & 0x01);
		coin_lockout_w(2,~data & 0x02);
		coin_counter_w(3,data & 0x04);
		coin_lockout_w(3,~data & 0x08);
/*
  	{
       char baf[40];
       sprintf(baf,"0xf1c004=%04x", data);
       usrintf_showmessage(baf);
       }
*/
    }
}

static int cps1_interrupt(void)
{
	/* Strider also has a IRQ4 handler. It is input port related, but the game */
	/* works without it (maybe it's used to multiplex controls). It is the */
	/* *only* game to have that. */
	return 2;
}

/********************************************************************
*
*  Q Sound
*  =======
*
********************************************************************/

struct QSound_interface qsound_interface =
{
	QSOUND_CLOCK,
	REGION_SOUND1,
	{ 100,100 }
};

static unsigned char *qsound_sharedram1,*qsound_sharedram2;

int cps1_qsound_interrupt(void)
{
#if 0
I have removed CPU_AUDIO_CPU from the Z(0 so this is no longer necessary
	/* kludge to pass the sound board test with sound disabled */
	if (Machine->sample_rate == 0)
		qsound_sharedram1[0xfff] = 0x77;
#endif

	return 2;
}


READ16_HANDLER( qsound_rom_r )
{
	unsigned char *rom = memory_region(REGION_USER1);

	if (rom) return rom[offset] | 0xff00;
	else
	{
		usrintf_showmessage("%06x: read sound ROM byte %04x",cpu_get_pc(),offset);
		return 0;
	}
}

READ16_HANDLER( qsound_sharedram1_r )
{
	return qsound_sharedram1[offset] | 0xff00;
}

WRITE16_HANDLER( qsound_sharedram1_w )
{
	if (ACCESSING_LSB)
		qsound_sharedram1[offset] = data;
}

static READ16_HANDLER( qsound_sharedram2_r )
{
	return qsound_sharedram2[offset] | 0xff00;
}

static WRITE16_HANDLER( qsound_sharedram2_w )
{
	if (ACCESSING_LSB)
		qsound_sharedram2[offset] = data;
}

static WRITE_HANDLER( qsound_banksw_w )
{
	/*
	Z80 bank register for music note data. It's odd that it isn't encrypted
	though.
	*/
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bankaddress=0x10000+((data&0x0f)*0x4000);
	if (bankaddress >= memory_region_length(REGION_CPU2))
	{
		logerror("WARNING: Q sound bank overflow (%02x)\n", data);
		bankaddress=0x10000;
	}
	cpu_setbank(1, &RAM[bankaddress]);
}


/********************************************************************
*
*  EEPROM
*  ======
*
*   The EEPROM is accessed by a serial protocol using the register
*   0xf1c006
*
********************************************************************/

static struct EEPROM_interface qsound_eeprom_interface =
{
	7,		/* address bits */
	8,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};

static struct EEPROM_interface pang3_eeprom_interface =
{
	6,		/* address bits */
	16,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};

static void qsound_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&qsound_eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

static void pang3_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&pang3_eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

READ16_HANDLER( cps1_eeprom_port_r )
{
	return EEPROM_read_bit();
}

WRITE16_HANDLER( cps1_eeprom_port_w )
{
	if (ACCESSING_LSB)
	{
		/*
		bit 0 = data
		bit 6 = clock
		bit 7 = cs
		*/
		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x80) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
	}
}



static MEMORY_READ16_START( cps1_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM }, /* 68000 ROM */
	{ 0x800000, 0x800001, input_port_4_word_r }, /* Player input ports */
	{ 0x800010, 0x800011, input_port_4_word_r }, /* ?? */
	{ 0x800018, 0x80001f, cps1_input_r }, /* Input ports */
	{ 0x800020, 0x800021, MRA16_NOP }, /* ? Used by Rockman ? */
	{ 0x800052, 0x800055, forgottn_dial_0_r }, /* forgotten worlds */
	{ 0x80005a, 0x80005d, forgottn_dial_1_r }, /* forgotten worlds */
	{ 0x800176, 0x800177, cps1_input2_r }, /* Extra input ports */
	{ 0x8001fc, 0x8001fd, cps1_input2_r }, /* Input ports (SF Rev E) */
	{ 0x800100, 0x8001ff, cps1_output_r },   /* Output ports */
	{ 0x900000, 0x92ffff, MRA16_RAM },	/* SF2CE executes code from here */
	{ 0xf00000, 0xf0ffff, qsound_rom_r },		/* Slammasters protection */
	{ 0xf18000, 0xf19fff, qsound_sharedram1_r },	/* Q RAM */
	{ 0xf1c000, 0xf1c001, cps1_input2_r },   /* Player 3 controls (later games) */
	{ 0xf1c002, 0xf1c003, cps1_input3_r },   /* Player 4 controls (later games - muscle bombers) */
	{ 0xf1c006, 0xf1c007, cps1_eeprom_port_r },
	{ 0xf1e000, 0xf1ffff, qsound_sharedram2_r },	/* Q RAM */
	{ 0xff0000, 0xffffff, MRA16_RAM },   /* RAM */
MEMORY_END

static MEMORY_WRITE16_START( cps1_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },      /* ROM */
	{ 0x800030, 0x800031, cps1_coinctrl_w },
	{ 0x800040, 0x800041, forgottn_dial_0_reset_w },
	{ 0x800048, 0x800049, forgottn_dial_1_reset_w },
	{ 0x800180, 0x800181, cps1_sound_command_w },  /* Sound command */
	{ 0x800188, 0x800189, cps1_sound_fade_w },
	{ 0x800100, 0x8001ff, cps1_output_w, &cps1_output, &cps1_output_size },  /* Output ports */
	{ 0x900000, 0x92ffff, MWA16_RAM, &cps1_gfxram, &cps1_gfxram_size },
	{ 0xf18000, 0xf19fff, qsound_sharedram1_w }, /* Q RAM */
	{ 0xf1c004, 0xf1c005, cpsq_coinctrl2_w },   /* Coin control2 (later games) */
	{ 0xf1c006, 0xf1c007, cps1_eeprom_port_w },
	{ 0xf1e000, 0xf1ffff, qsound_sharedram2_w }, /* Q RAM */
	{ 0xff0000, 0xffffff, MWA16_RAM },        /* RAM */
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xf001, 0xf001, YM2151_status_port_0_r },
	{ 0xf002, 0xf002, OKIM6295_status_0_r },
	{ 0xf008, 0xf008, soundlatch_r },
	{ 0xf00a, 0xf00a, cps1_snd_fade_timer_r }, /* Sound timer fade */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2151_register_port_0_w },
	{ 0xf001, 0xf001, YM2151_data_port_0_w },
	{ 0xf002, 0xf002, OKIM6295_data_0_w },
	{ 0xf004, 0xf004, cps1_snd_bankswitch_w },
/*	{ 0xf006, 0xf006, MWA_NOP },    ???? Unknown ????    */
MEMORY_END

MEMORY_READ_START( qsound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },  /* banked (contains music data) */
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd007, 0xd007, qsound_status_r },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START( qsound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM, &qsound_sharedram1 },
	{ 0xd000, 0xd000, qsound_data_h_w },
	{ 0xd001, 0xd001, qsound_data_l_w },
	{ 0xd002, 0xd002, qsound_cmd_w },
	{ 0xd003, 0xd003, qsound_banksw_w },
	{ 0xf000, 0xffff, MWA_RAM, &qsound_sharedram2 },
MEMORY_END



INPUT_PORTS_START( forgottn )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
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
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
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

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_ANALOGX( 0x0fff, 0x0000, IPT_DIAL | IPF_PLAYER1, 100, 20, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START
	PORT_ANALOGX( 0x0fff, 0x0000, IPT_DIAL | IPF_PLAYER2, 100, 20, 0, 0, KEYCODE_N, KEYCODE_M, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( ghouls )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )  /* Service, but it doesn't give any credit */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easier" )
	PORT_DIPSETTING(    0x05, "Very Easy" )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x07, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "10K, 30K and every 30K" )
	PORT_DIPSETTING(    0x10, "20K, 50K and every 70K" )
	PORT_DIPSETTING(    0x30, "30K, 60K and every 70K" )
	PORT_DIPSETTING(    0x00, "40K, 70K and every 80K" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( strider )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0xc0, "Upright 1 Player" )
	PORT_DIPSETTING(    0x80, "Upright 2 Players" )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	/* 0x40 Cocktail */

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easiest" )
	PORT_DIPSETTING(    0x05, "Easier" )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x07, "Normal" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	/* TODO: this doesn't seem to work */
	PORT_DIPNAME( 0x08, 0x00, "Continue Coinage ?" )
	PORT_DIPSETTING(    0x00, "1 Coin" )
	PORT_DIPSETTING(    0x08, "2 Coins" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20000 60000" )
	PORT_DIPSETTING(    0x00, "30000 60000" )
	PORT_DIPSETTING(    0x30, "20000 40000 60000" )
	PORT_DIPSETTING(    0x20, "30000 50000 70000" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Freeze" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( dwj )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	/* 0x00 2 Coins/1 credit for both coin ports */
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy 2" )
	PORT_DIPSETTING(    0x06, "Easy 1" )
	PORT_DIPSETTING(    0x07, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult 1" )
	PORT_DIPSETTING(    0x02, "Difficult 2" )
	PORT_DIPSETTING(    0x01, "Difficult 3" )
	PORT_DIPSETTING(    0x00, "Very difficult" )
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

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, "Freeze" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Turbo Mode" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( willow )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit (1 to continue)" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit (1 to continue)" )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0xc0, "Upright 1 Player" )
	PORT_DIPSETTING(    0x80, "Upright 2 Players" )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	/* 0x40 Cocktail */

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Very easy" )
	PORT_DIPSETTING(    0x05, "Easier" )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x07, "Normal" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, "Nando Speed" )
	PORT_DIPSETTING(    0x10, "Slow" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) /* Unused ? */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x20, DEF_STR( Unknown ) ) /* Unused ? */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Stage Magic Continue (power up?)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* The test mode reports Stage Magic, a file with dip says if
	 power up are on the player gets sword and magic item without having
	 to buy them. To test */

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, "Vitality" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( unsquad )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit (1 to continue)" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit (1 to continue)" )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Super Easy" )
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Very Difficult" )
	PORT_DIPSETTING(    0x01, "Super Difficult" )
	PORT_DIPSETTING(    0x00, "Ultra Super Difficult" )
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

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ffight )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x04, "Difficulty Level 1" )
	PORT_DIPSETTING(    0x07, "Very easy" )
	PORT_DIPSETTING(    0x06, "Easier" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x10, "Difficulty Level 2" )
	PORT_DIPSETTING(    0x18, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x60, "100k" )
	PORT_DIPSETTING(    0x40, "200k" )
	PORT_DIPSETTING(    0x20, "100k and every 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( 1941 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "0 (Easier)" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x05, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "7 (Harder)" )
	PORT_DIPNAME( 0x18, 0x18, "Life Bar" )
	PORT_DIPSETTING(    0x18, "More Slowly" )
	PORT_DIPSETTING(    0x10, "Slowly" )
	PORT_DIPSETTING(    0x08, "Quickly" )
	PORT_DIPSETTING(    0x00, "More Quickly" )
	PORT_DIPNAME( 0x60, 0x60, "Bullet's Speed" )
	PORT_DIPSETTING(    0x60, "Very Slow" )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x20, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Initial Vitality" )
	PORT_DIPSETTING(    0x80, "3 Bars" )
	PORT_DIPSETTING(    0x00, "4 Bars" )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( mercs )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x38, "3" )
	PORT_DIPSETTING(    0x30, "4" )
	PORT_DIPSETTING(    0x28, "5" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPSETTING(    0x18, "7" )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Easiest" )
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Very Difficult" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x10, 0x10, "Max Players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x05, "5" )
	PORT_DIPSETTING(    0x06, "6" )
	PORT_DIPSETTING(    0x07, "7" )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Player 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )
INPUT_PORTS_END

INPUT_PORTS_START( mtwins )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Super Easy" )
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Very Difficult" )
	PORT_DIPSETTING(    0x01, "Super Difficult" )
	PORT_DIPSETTING(    0x00, "Ultra Super Difficult" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	/*  0x30 gives 1 life */
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( msword )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x04, "Level 1" )
	PORT_DIPSETTING(    0x07, "Easiest" )
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x38, 0x38, "Level 2" )
	PORT_DIPSETTING(    0x20, "Easiest" )
	PORT_DIPSETTING(    0x28, "Very Easy" )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x38, "Normal" )
	PORT_DIPSETTING(    0x18, "Difficult" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x08, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x00, "Stage Select" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, "Vitality" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cawing )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, "Difficulty Level (Enemy Strength)" )
	PORT_DIPSETTING(    0x07, "Very Easy" )
	PORT_DIPSETTING(    0x06, "Easy 2" )
	PORT_DIPSETTING(    0x05, "Easy 1" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult 1" )
	PORT_DIPSETTING(    0x02, "Difficult 2" )
	PORT_DIPSETTING(    0x01, "Difficult 3" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x18, 0x10, "Difficulty Level (Player Strength)" )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( nemo )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Very Easy" )
	PORT_DIPSETTING(    0x06, "Easy 1" )
	PORT_DIPSETTING(    0x05, "Easy 2" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult 1" )
	PORT_DIPSETTING(    0x02, "Difficult 2" )
	PORT_DIPSETTING(    0x01, "Difficult 3" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x18, 0x18, "Life Bar" )
	PORT_DIPSETTING(    0x00, "Minimun" )
	PORT_DIPSETTING(    0x18, "Medium" )
	PORT_DIPSETTING(    0x08, "Maximum" )
	/* 0x10 gives Medium */
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( sf2 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Easier" )
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
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

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Extra buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( sf2j )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Easier" )
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x08, 0x00, "2 Players Game" )
    PORT_DIPSETTING(    0x08, "1 Credit/No Continue" )
    PORT_DIPSETTING(    0x00, "2 Credits/Winner Continue" )
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

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Extra buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( 3wonders )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x03, 0x03, "Action Lives" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Action Difficulty" )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Shooting Lives" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, "Shooting Difficulty" )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, "Puzzle Lives" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, "Puzzle Difficulty" )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( kod )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) /* Service Coin, not player 3 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Test */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x10, 0x10, "Max Players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Easiest" )
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x38, "2" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "80k and every 400k" )
	PORT_DIPSETTING(    0xc0, "100k and every 450k" )
	PORT_DIPSETTING(    0x40, "160k and every 450k" )
	PORT_DIPSETTING(    0x00, "None" )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Player 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )
INPUT_PORTS_END

INPUT_PORTS_START( captcomm )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, "Difficulty 1" )
	PORT_DIPSETTING(    0x07, "Very Easy" )
	PORT_DIPSETTING(    0x06, "Easy 1" )
	PORT_DIPSETTING(    0x05, "Easy 2" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Very Difficult" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, "Difficulty 2" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Max Players" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "4" )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( knights )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN4 ) /* service */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* TEST */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x04, "Player speed and vitality consumption" )
	PORT_DIPSETTING(    0x07, "Very easy" )
	PORT_DIPSETTING(    0x06, "Easier" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x01, "Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x38, 0x38, "Enemy's vitality and attack power" )
	PORT_DIPSETTING(    0x10, "Very Easy" )
	PORT_DIPSETTING(    0x08, "Easier" )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x38, "Normal" )
	PORT_DIPSETTING(    0x30, "Medium" )
	PORT_DIPSETTING(    0x28, "Hard" )
	PORT_DIPSETTING(    0x20, "Harder" )
	PORT_DIPSETTING(    0x18, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPNAME( 0x80, 0x80, "Max Players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x80, "3" )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Player 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )
INPUT_PORTS_END

INPUT_PORTS_START( varth )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Very Easy" )
	PORT_DIPSETTING(    0x06, "Easy 1" )
	PORT_DIPSETTING(    0x05, "Easy 2" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPSETTING(    0x02, "Very Difficult" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "600k and every 1.400k" )
	PORT_DIPSETTING(    0x10, "600k 2.000k and 4500k" )
	PORT_DIPSETTING(    0x08, "1.200k 3.500k" )
	PORT_DIPSETTING(    0x00, "2000k only" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cworld2j )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Test Mode (Use with Service Mode)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "0" )
	PORT_DIPSETTING(    0x05, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPNAME( 0x18, 0x18, "Extend" )
	PORT_DIPSETTING(    0x18, "N" )
	PORT_DIPSETTING(    0x10, "E" )
	PORT_DIPSETTING(    0x00, "D" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0xe0, "3" )
	PORT_DIPSETTING(    0xa0, "4" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( wof )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, 0x40, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWB (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* Player 3 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )
INPUT_PORTS_END

INPUT_PORTS_START( dino )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, 0x40, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWB (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* Player 3 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )
INPUT_PORTS_END

INPUT_PORTS_START( punisher )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, 0x40, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWB (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( slammast )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, 0x40, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWB (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf7, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )

	PORT_START     /* Player 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START     /* Player 4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( pnickj )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x08, "Coin Slots" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
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

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "Easiest" )
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Hard" )
	PORT_DIPSETTING(    0x02, "Very Hard" )
	PORT_DIPSETTING(    0x01, "Hardest" )
	PORT_DIPSETTING(    0x00, "Master Level" )
	PORT_DIPNAME( 0x08, 0x00, "Unknkown" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x30, "4" )
	PORT_DIPNAME( 0xc0, 0xc0, "Vs Play Mode" )
	PORT_DIPSETTING(    0xc0, "1 Game Match" )
	PORT_DIPSETTING(    0x80, "3 Games Match" )
	PORT_DIPSETTING(    0x40, "5 Games Match" )
	PORT_DIPSETTING(    0x00, "7 Games Match" )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( qad )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x06, DEF_STR( Difficulty ) )
/*	PORT_DIPSETTING(    0x07, "Very Easy" ) */
	PORT_DIPSETTING(    0x06, "Very Easy" )
	PORT_DIPSETTING(    0x05, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x03, "Hard" )
	PORT_DIPSETTING(    0x02, "Very Hard" )
/*	PORT_DIPSETTING(    0x01, "Very Hard" ) */
/*	PORT_DIPSETTING(    0x00, "Very Hard" ) */
	PORT_DIPNAME( 0x18, 0x18, "Wisdom" )
	PORT_DIPSETTING(    0x18, "Low" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "High" )
	PORT_DIPSETTING(    0x00, "Brilliant" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x60, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0xc0, "4" )
	PORT_DIPSETTING(    0xe0, "5" )
/*	PORT_DIPSETTING(    0x40, "1" ) */
/*	PORT_DIPSETTING(    0x20, "1" ) */
/*	PORT_DIPSETTING(    0x00, "1" ) */

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( qadj )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "0" )
	PORT_DIPSETTING(    0x06, "1" )
	PORT_DIPSETTING(    0x05, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x03, "4" )
/*	PORT_DIPSETTING(    0x02, "4" ) */
/*	PORT_DIPSETTING(    0x01, "4" ) */
/*	PORT_DIPSETTING(    0x00, "4" ) */
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xa0, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xe0, "3" )
/*	PORT_DIPSETTING(    0x00, "1" ) */
/*	PORT_DIPSETTING(    0x20, "1" ) */
/*	PORT_DIPSETTING(    0x80, "1" ) */
/*	PORT_DIPSETTING(    0x40, "2" ) */
/*	PORT_DIPSETTING(    0x60, "3" ) */

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( qtono2 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x60, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0xe0, "3" )
	PORT_DIPSETTING(    0xa0, "4" )
	PORT_DIPSETTING(    0xc0, "5" )
/*	PORT_DIPSETTING(    0x40, "?" ) */
/*	PORT_DIPSETTING(    0x20, "?" ) */
/*	PORT_DIPSETTING(    0x00, "?" ) */

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( pang3 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, 0x40, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWB (not used, EEPROM) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( megaman )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x1f, 0x1f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x11, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x12, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x13, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x15, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x16, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0e, "2 Coins/1 Credit - 1 to continue (if on)" )
	PORT_DIPSETTING(    0x1f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x1e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x1d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x1b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x1a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x19, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_8C ) )
	PORT_DIPSETTING(    0x17, DEF_STR( 1C_9C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( Free_Play ) )
	/* 0x00 to 0x0c 1 Coin/1 Credit */
	PORT_DIPNAME( 0x60, 0x20, "2 Player Game" )
	PORT_DIPSETTING(    0x20, "1 Credit" )
	PORT_DIPSETTING(    0x40, "2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Credits - pl1 may play on right" )
	   /* Unused 0x00 */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Difficult" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x0c, 0x0c, "Time" )
	PORT_DIPSETTING(    0x0c, "100" )
	PORT_DIPSETTING(    0x08, "90" )
	PORT_DIPSETTING(    0x04, "70" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Voice" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSWC */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout layout8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 4*8 },	/* org displacement - 8x8 tiles are taken from the RIGHT side of the 16x16 tile
				   (fixes cawing which uses character 0x0002 as space, typo instead of 0x20?) */
	{ 8*8 },	/* line modulo */
	64*8		/* char modulo */
};

static struct GfxLayout layout16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	128*8		/* char modulo */
};

static struct GfxLayout layout32x32 =
{
	32,32,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 16*8 },	/* line modulo */
	512*8		/* char modulo */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout8x8,   0, 0x100 },
	{ REGION_GFX1, 0, &layout16x16, 0, 0x100 },
	{ REGION_GFX1, 0, &layout32x32, 0, 0x100 },
	{ -1 }
};



static void cps1_irq_handler_mus(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,  /* 1 chip */
	3579580,    /* 3.579580 MHz ? */
	{ YM3012_VOL(35,MIXER_PAN_LEFT,35,MIXER_PAN_RIGHT) },
	{ cps1_irq_handler_mus }
};

static struct OKIM6295interface okim6295_interface_6061 =
{
	1,  /* 1 chip */
	{ 6061 },
	{ REGION_SOUND1 },
	{ 30 }
};

static struct OKIM6295interface okim6295_interface_7576 =
{
	1,  /* 1 chip */
	{ 7576 },
	{ REGION_SOUND1 },
	{ 30 }
};



/********************************************************************
*
*  Machine Driver macro
*  ====================
*
*  Abusing the pre-processor.
*
********************************************************************/

#define MACHINE_DRIVER(DRVNAME,CPU_FRQ,OKI_FREQ,NVRAM)						\
static struct MachineDriver machine_driver_##DRVNAME =				\
{																			\
	/* basic machine hardware */											\
	{																		\
		{																	\
			CPU_M68000,														\
			CPU_FRQ,														\
			cps1_readmem,cps1_writemem,0,0,									\
			cps1_interrupt, 1												\
		},																	\
		{																	\
			CPU_Z80 | CPU_AUDIO_CPU,										\
			4000000,  /* 4 MHz ??? TODO: find real FRQ */					\
			sound_readmem,sound_writemem,0,0,								\
			ignore_interrupt,0												\
		}																	\
	},																		\
	60, DEFAULT_60HZ_VBLANK_DURATION,										\
	1,																		\
	0,																		\
																			\
	/* video hardware */													\
	0x30*8+32*2, 0x1c*8+32*3, { 32, 32+0x30*8-1, 32+16, 32+16+0x1c*8-1 },	\
	gfxdecodeinfo,															\
	4096, 4096,																\
	0,																		\
																			\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,								\
	cps1_eof_callback,														\
	cps1_vh_start,															\
	cps1_vh_stop,															\
	cps1_vh_screenrefresh,													\
																			\
	/* sound hardware */													\
	0,0,0,0,																\
	{ { SOUND_YM2151,  &ym2151_interface },									\
	  { SOUND_OKIM6295,  &okim6295_interface_##OKI_FREQ }					\
	},																		\
	NVRAM																	\
};

static struct MachineDriver machine_driver_qsound =
{
	{
		{
			CPU_M68000,
			10000000,	/* ??? */
			cps1_readmem,cps1_writemem,0,0,
			cps1_qsound_interrupt, 1  /* ??? interrupts per frame */
		},
		{
			CPU_Z80,	/* can't use CPU_AUDIO_CPU, slammast requires the Z80 for protection */
			6000000,  /* 6 MHz ??? TODO: find real FRQ */
			qsound_readmem,qsound_writemem,0,0,
			0,0,
			interrupt,250	/* ?? */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	0x30*8+32*2, 0x1c*8+32*3, { 32, 32+0x30*8-1, 32+16, 32+16+0x1c*8-1 },
	gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	cps1_eof_callback,
	cps1_vh_start,
	cps1_vh_stop,
	cps1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_QSOUND,
			&qsound_interface
		}
	},

	qsound_nvram_handler
};


MACHINE_DRIVER( forgottn, 10000000, 6061, 0 )
MACHINE_DRIVER( cps1,     10000000, 7576, 0 )	/* 10 MHz should be the "standard" freq */
MACHINE_DRIVER( sf2,      12000000, 7576, 0 )	/* 12 MHz */
MACHINE_DRIVER( sf2accp2, 12000000, 7576, 0 )	/* 12 MHz */
MACHINE_DRIVER( pang3,    10000000, 7576, pang3_nvram_handler )	/* 10 MHz?? */



/***************************************************************************

  Game driver(s)

***************************************************************************/

#define CODE_SIZE 0x200000

ROM_START( forgottn )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "lwu11a",        0x00000, 0x20000, 0xddf78831 )
	ROM_LOAD16_BYTE( "lwu15a",        0x00001, 0x20000, 0xf7ce2097 )
	ROM_LOAD16_BYTE( "lwu10a",        0x40000, 0x20000, 0x8cb38c81 )
	ROM_LOAD16_BYTE( "lwu14a",        0x40001, 0x20000, 0xd70ef9fd )
	ROM_LOAD16_WORD_SWAP( "lw-07",         0x80000, 0x80000, 0xfd252a26 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "lw-02",         0x000000, 0x80000, 0x43e6c5c8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-09",         0x000002, 0x80000, 0x899cb4ad, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-06",         0x000004, 0x80000, 0x5b9edffc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-13",         0x000006, 0x80000, 0x8e058ef5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-01",         0x200000, 0x80000, 0x0318f298, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-08",         0x200002, 0x80000, 0x25a8e43c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-05",         0x200004, 0x80000, 0xe4552fd7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-12",         0x200006, 0x80000, 0x8e6a832b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, REGION_GFX2, 0 )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x8000 )	/* stars */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "lwu00",         0x00000, 0x08000, 0x59df2a63 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "lw-03u",        0x00000, 0x20000, 0x807d051f )
	ROM_LOAD( "lw-04u",        0x20000, 0x20000, 0xe6cd098e )
ROM_END

ROM_START( lostwrld )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "lw-11c.14f",    0x00000, 0x20000, 0x67e42546 )
	ROM_LOAD16_BYTE( "lw-15c.14g",    0x00001, 0x20000, 0x402e2a46 )
	ROM_LOAD16_BYTE( "lw-10c.13f",    0x40000, 0x20000, 0xc46479d7 )
	ROM_LOAD16_BYTE( "lw-14c.13g",    0x40001, 0x20000, 0x97670f4a )
	ROM_LOAD16_WORD_SWAP( "lw-07",         0x80000, 0x80000, 0xfd252a26 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "lw-02",         0x000000, 0x80000, 0x43e6c5c8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-09",         0x000002, 0x80000, 0x899cb4ad, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-06",         0x000004, 0x80000, 0x5b9edffc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-13",         0x000006, 0x80000, 0x8e058ef5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-01",         0x200000, 0x80000, 0x0318f298, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-08",         0x200002, 0x80000, 0x25a8e43c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-05",         0x200004, 0x80000, 0xe4552fd7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "lw-12",         0x200006, 0x80000, 0x8e6a832b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, REGION_GFX2, 0 )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x8000 )	/* stars */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "lwu00",         0x00000, 0x08000, 0x59df2a63 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "lw-03.14c",     0x00000, 0x20000, 0xce2159e7 )
	ROM_LOAD( "lw-04.13c",     0x20000, 0x20000, 0x39305536 )
ROM_END

ROM_START( ghouls )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ghl29.bin",    0x00000, 0x20000, 0x166a58a2 )
	ROM_LOAD16_BYTE( "ghl30.bin",    0x00001, 0x20000, 0x7ac8407a )
	ROM_LOAD16_BYTE( "ghl27.bin",    0x40000, 0x20000, 0xf734b2be )
	ROM_LOAD16_BYTE( "ghl28.bin",    0x40001, 0x20000, 0x03d3e714 )
	ROM_LOAD16_WORD( "ghl17.bin",    0x80000, 0x80000, 0x3ea1b0f2 )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )
	ROMX_LOAD( "ghl5.bin",   0x000000, 0x80000, 0x0ba9c0b0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl7.bin",   0x000002, 0x80000, 0x5d760ab9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl6.bin",   0x000004, 0x80000, 0x4ba90b59, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl8.bin",   0x000006, 0x80000, 0x4bdee9de, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl09.bin",  0x200000, 0x10000, 0xae24bb19, ROM_SKIP(7) )
	ROMX_LOAD( "ghl18.bin",  0x200001, 0x10000, 0xd34e271a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl13.bin",  0x200002, 0x10000, 0x3f70dd37, ROM_SKIP(7) )
	ROMX_LOAD( "ghl22.bin",  0x200003, 0x10000, 0x7e69e2e6, ROM_SKIP(7) )
	ROMX_LOAD( "ghl11.bin",  0x200004, 0x10000, 0x37c9b6c6, ROM_SKIP(7) )
	ROMX_LOAD( "ghl20.bin",  0x200005, 0x10000, 0x2f1345b4, ROM_SKIP(7) )
	ROMX_LOAD( "ghl15.bin",  0x200006, 0x10000, 0x3c2a212a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl24.bin",  0x200007, 0x10000, 0x889aac05, ROM_SKIP(7) )
	ROMX_LOAD( "ghl10.bin",  0x280000, 0x10000, 0xbcc0f28c, ROM_SKIP(7) )
	ROMX_LOAD( "ghl19.bin",  0x280001, 0x10000, 0x2a40166a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl14.bin",  0x280002, 0x10000, 0x20f85c03, ROM_SKIP(7) )
	ROMX_LOAD( "ghl23.bin",  0x280003, 0x10000, 0x8426144b, ROM_SKIP(7) )
	ROMX_LOAD( "ghl12.bin",  0x280004, 0x10000, 0xda088d61, ROM_SKIP(7) )
	ROMX_LOAD( "ghl21.bin",  0x280005, 0x10000, 0x17e11df0, ROM_SKIP(7) )
	ROMX_LOAD( "ghl16.bin",  0x280006, 0x10000, 0xf187ba1c, ROM_SKIP(7) )
	ROMX_LOAD( "ghl25.bin",  0x280007, 0x10000, 0x29f79c78, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "ghl26.bin",     0x00000, 0x08000, 0x3692f6e5 )
	ROM_CONTINUE(              0x10000, 0x08000 )
ROM_END

ROM_START( ghoulsu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "dmu29",        0x00000, 0x20000, 0x334d85b2 )
	ROM_LOAD16_BYTE( "dmu30",        0x00001, 0x20000, 0xcee8ceb5 )
	ROM_LOAD16_BYTE( "dmu27",        0x40000, 0x20000, 0x4a524140 )
	ROM_LOAD16_BYTE( "dmu28",        0x40001, 0x20000, 0x94aae205 )
	ROM_LOAD16_WORD( "ghl17.bin",    0x80000, 0x80000, 0x3ea1b0f2 )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )
	ROMX_LOAD( "ghl5.bin",   0x000000, 0x80000, 0x0ba9c0b0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl7.bin",   0x000002, 0x80000, 0x5d760ab9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl6.bin",   0x000004, 0x80000, 0x4ba90b59, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl8.bin",   0x000006, 0x80000, 0x4bdee9de, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl09.bin",  0x200000, 0x10000, 0xae24bb19, ROM_SKIP(7) )
	ROMX_LOAD( "ghl18.bin",  0x200001, 0x10000, 0xd34e271a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl13.bin",  0x200002, 0x10000, 0x3f70dd37, ROM_SKIP(7) )
	ROMX_LOAD( "ghl22.bin",  0x200003, 0x10000, 0x7e69e2e6, ROM_SKIP(7) )
	ROMX_LOAD( "ghl11.bin",  0x200004, 0x10000, 0x37c9b6c6, ROM_SKIP(7) )
	ROMX_LOAD( "ghl20.bin",  0x200005, 0x10000, 0x2f1345b4, ROM_SKIP(7) )
	ROMX_LOAD( "ghl15.bin",  0x200006, 0x10000, 0x3c2a212a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl24.bin",  0x200007, 0x10000, 0x889aac05, ROM_SKIP(7) )
	ROMX_LOAD( "ghl10.bin",  0x280000, 0x10000, 0xbcc0f28c, ROM_SKIP(7) )
	ROMX_LOAD( "ghl19.bin",  0x280001, 0x10000, 0x2a40166a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl14.bin",  0x280002, 0x10000, 0x20f85c03, ROM_SKIP(7) )
	ROMX_LOAD( "ghl23.bin",  0x280003, 0x10000, 0x8426144b, ROM_SKIP(7) )
	ROMX_LOAD( "ghl12.bin",  0x280004, 0x10000, 0xda088d61, ROM_SKIP(7) )
	ROMX_LOAD( "ghl21.bin",  0x280005, 0x10000, 0x17e11df0, ROM_SKIP(7) )
	ROMX_LOAD( "ghl16.bin",  0x280006, 0x10000, 0xf187ba1c, ROM_SKIP(7) )
	ROMX_LOAD( "ghl25.bin",  0x280007, 0x10000, 0x29f79c78, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "ghl26.bin",     0x00000, 0x08000, 0x3692f6e5 )
	ROM_CONTINUE(              0x10000, 0x08000 )
ROM_END

ROM_START( daimakai )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "dmj_38.bin",   0x00000, 0x20000, 0x82fd1798 )
	ROM_LOAD16_BYTE( "dmj_39.bin",   0x00001, 0x20000, 0x35366ccc )
	ROM_LOAD16_BYTE( "dmj_40.bin",   0x40000, 0x20000, 0xa17c170a )
	ROM_LOAD16_BYTE( "dmj_41.bin",   0x40001, 0x20000, 0x6af0b391 )
	ROM_LOAD16_WORD( "ghl17.bin",    0x80000, 0x80000, 0x3ea1b0f2 )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )
	ROMX_LOAD( "ghl5.bin",   0x000000, 0x80000, 0x0ba9c0b0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl7.bin",   0x000002, 0x80000, 0x5d760ab9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl6.bin",   0x000004, 0x80000, 0x4ba90b59, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl8.bin",   0x000006, 0x80000, 0x4bdee9de, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ghl09.bin",  0x200000, 0x10000, 0xae24bb19, ROM_SKIP(7) )
	ROMX_LOAD( "ghl18.bin",  0x200001, 0x10000, 0xd34e271a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl13.bin",  0x200002, 0x10000, 0x3f70dd37, ROM_SKIP(7) )
	ROMX_LOAD( "ghl22.bin",  0x200003, 0x10000, 0x7e69e2e6, ROM_SKIP(7) )
	ROMX_LOAD( "ghl11.bin",  0x200004, 0x10000, 0x37c9b6c6, ROM_SKIP(7) )
	ROMX_LOAD( "ghl20.bin",  0x200005, 0x10000, 0x2f1345b4, ROM_SKIP(7) )
	ROMX_LOAD( "ghl15.bin",  0x200006, 0x10000, 0x3c2a212a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl24.bin",  0x200007, 0x10000, 0x889aac05, ROM_SKIP(7) )
	ROMX_LOAD( "ghl10.bin",  0x280000, 0x10000, 0xbcc0f28c, ROM_SKIP(7) )
	ROMX_LOAD( "ghl19.bin",  0x280001, 0x10000, 0x2a40166a, ROM_SKIP(7) )
	ROMX_LOAD( "ghl14.bin",  0x280002, 0x10000, 0x20f85c03, ROM_SKIP(7) )
	ROMX_LOAD( "ghl23.bin",  0x280003, 0x10000, 0x8426144b, ROM_SKIP(7) )
	ROMX_LOAD( "ghl12.bin",  0x280004, 0x10000, 0xda088d61, ROM_SKIP(7) )
	ROMX_LOAD( "ghl21.bin",  0x280005, 0x10000, 0x17e11df0, ROM_SKIP(7) )
	ROMX_LOAD( "ghl16.bin",  0x280006, 0x10000, 0xf187ba1c, ROM_SKIP(7) )
	ROMX_LOAD( "ghl25.bin",  0x280007, 0x10000, 0x29f79c78, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "ghl26.bin",     0x00000, 0x08000, 0x3692f6e5 )
	ROM_CONTINUE(              0x10000, 0x08000 )
ROM_END

ROM_START( strider )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "strider.30",   0x00000, 0x20000, 0xda997474 )
	ROM_LOAD16_BYTE( "strider.35",   0x00001, 0x20000, 0x5463aaa3 )
	ROM_LOAD16_BYTE( "strider.31",   0x40000, 0x20000, 0xd20786db )
	ROM_LOAD16_BYTE( "strider.36",   0x40001, 0x20000, 0x21aa2863 )
	ROM_LOAD16_WORD_SWAP( "strider.32",   0x80000, 0x80000, 0x9b3cfc08 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "strider.06",   0x000000, 0x80000, 0x4eee9aea, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.08",   0x000002, 0x80000, 0x2d7f21e4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.02",   0x000004, 0x80000, 0x7705aa46, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.04",   0x000006, 0x80000, 0x5b18b722, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.05",   0x200000, 0x80000, 0x005f000b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.07",   0x200002, 0x80000, 0xb9441519, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.01",   0x200004, 0x80000, 0xb7d04e8b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.03",   0x200006, 0x80000, 0x6b4713b4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, REGION_GFX2, 0 )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x8000 )	/* stars */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "strider.09",    0x00000, 0x08000, 0x2ed403bc )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "strider.18",   0x00000, 0x20000, 0x4386bc80 )
	ROM_LOAD( "strider.19",   0x20000, 0x20000, 0x444536d7 )
ROM_END

ROM_START( striderj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sthj23.bin",   0x00000, 0x080000, 0x046e7b12 )
	ROM_LOAD16_WORD_SWAP( "strider.32",   0x80000, 0x80000, 0x9b3cfc08 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "strider.06",   0x000000, 0x80000, 0x4eee9aea, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.08",   0x000002, 0x80000, 0x2d7f21e4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.02",   0x000004, 0x80000, 0x7705aa46, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.04",   0x000006, 0x80000, 0x5b18b722, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.05",   0x200000, 0x80000, 0x005f000b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.07",   0x200002, 0x80000, 0xb9441519, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.01",   0x200004, 0x80000, 0xb7d04e8b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.03",   0x200006, 0x80000, 0x6b4713b4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, REGION_GFX2, 0 )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x8000 )	/* stars */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "strider.09",    0x00000, 0x08000, 0x2ed403bc )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "strider.18",   0x00000, 0x20000, 0x4386bc80 )
	ROM_LOAD( "strider.19",   0x20000, 0x20000, 0x444536d7 )
ROM_END

ROM_START( stridrja )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sth36.bin",   0x00000, 0x20000, 0x53c7b006 )
	ROM_LOAD16_BYTE( "sth42.bin",   0x00001, 0x20000, 0x4037f65f )
	ROM_LOAD16_BYTE( "sth37.bin",   0x40000, 0x20000, 0x80e8877d )
	ROM_LOAD16_BYTE( "sth43.bin",   0x40001, 0x20000, 0x6b3fa466 )
	ROM_LOAD16_WORD_SWAP( "strider.32",   0x80000, 0x80000, 0x9b3cfc08 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "strider.06",   0x000000, 0x80000, 0x4eee9aea, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.08",   0x000002, 0x80000, 0x2d7f21e4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.02",   0x000004, 0x80000, 0x7705aa46, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.04",   0x000006, 0x80000, 0x5b18b722, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.05",   0x200000, 0x80000, 0x005f000b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.07",   0x200002, 0x80000, 0xb9441519, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.01",   0x200004, 0x80000, 0xb7d04e8b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "strider.03",   0x200006, 0x80000, 0x6b4713b4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, REGION_GFX2, 0 )
	ROM_COPY( REGION_GFX1, 0x000000, 0x000000, 0x8000 )	/* stars */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "strider.09",    0x00000, 0x08000, 0x2ed403bc )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "strider.18",   0x00000, 0x20000, 0x4386bc80 )
	ROM_LOAD( "strider.19",   0x20000, 0x20000, 0x444536d7 )
ROM_END

ROM_START( dwj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "36.bin",       0x00000, 0x20000, 0x1a516657 )
	ROM_LOAD16_BYTE( "42.bin",       0x00001, 0x20000, 0x12a290a0 )
	ROM_LOAD16_BYTE( "37.bin",       0x40000, 0x20000, 0x932fc943 )
	ROM_LOAD16_BYTE( "43.bin",       0x40001, 0x20000, 0x872ad76d )
	ROM_LOAD16_BYTE( "34.bin",       0x80000, 0x20000, 0x8f663d00 )
	ROM_LOAD16_BYTE( "40.bin",       0x80001, 0x20000, 0x1586dbf3 )
	ROM_LOAD16_BYTE( "35.bin",       0xc0000, 0x20000, 0x9db93d7a )
	ROM_LOAD16_BYTE( "41.bin",       0xc0001, 0x20000, 0x1aae69a4 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "09.bin",       0x000000, 0x20000, 0xc3e83c69, ROM_SKIP(7) )
	ROMX_LOAD( "01.bin",       0x000001, 0x20000, 0x187b2886, ROM_SKIP(7) )
	ROMX_LOAD( "13.bin",       0x000002, 0x20000, 0x0273d87d, ROM_SKIP(7) )
	ROMX_LOAD( "05.bin",       0x000003, 0x20000, 0x339378b8, ROM_SKIP(7) )
	ROMX_LOAD( "24.bin",       0x000004, 0x20000, 0xc6909b6f, ROM_SKIP(7) )
	ROMX_LOAD( "17.bin",       0x000005, 0x20000, 0x2e2f8320, ROM_SKIP(7) )
	ROMX_LOAD( "38.bin",       0x000006, 0x20000, 0xcd7923ed, ROM_SKIP(7) )
	ROMX_LOAD( "32.bin",       0x000007, 0x20000, 0x21a0a453, ROM_SKIP(7) )
	ROMX_LOAD( "10.bin",       0x100000, 0x20000, 0xff28f8d0, ROM_SKIP(7) )
	ROMX_LOAD( "02.bin",       0x100001, 0x20000, 0xcc83c02f, ROM_SKIP(7) )
	ROMX_LOAD( "14.bin",       0x100002, 0x20000, 0x18fb232c, ROM_SKIP(7) )
	ROMX_LOAD( "06.bin",       0x100003, 0x20000, 0x6f9edd75, ROM_SKIP(7) )
	ROMX_LOAD( "25.bin",       0x100004, 0x20000, 0x152ea74a, ROM_SKIP(7) )
	ROMX_LOAD( "18.bin",       0x100005, 0x20000, 0x1833f932, ROM_SKIP(7) )
	ROMX_LOAD( "39.bin",       0x100006, 0x20000, 0xbc09b360, ROM_SKIP(7) )
	ROMX_LOAD( "33.bin",       0x100007, 0x20000, 0x89de1533, ROM_SKIP(7) )
	ROMX_LOAD( "11.bin",       0x200000, 0x20000, 0x29eaf490, ROM_SKIP(7) )
	ROMX_LOAD( "03.bin",       0x200001, 0x20000, 0x7bf51337, ROM_SKIP(7) )
	ROMX_LOAD( "15.bin",       0x200002, 0x20000, 0xd36cdb91, ROM_SKIP(7) )
	ROMX_LOAD( "07.bin",       0x200003, 0x20000, 0xe04af054, ROM_SKIP(7) )
	ROMX_LOAD( "26.bin",       0x200004, 0x20000, 0x07fc714b, ROM_SKIP(7) )
	ROMX_LOAD( "19.bin",       0x200005, 0x20000, 0x7114e5c6, ROM_SKIP(7) )
	ROMX_LOAD( "28.bin",       0x200006, 0x20000, 0xaf62bf07, ROM_SKIP(7) )
	ROMX_LOAD( "21.bin",       0x200007, 0x20000, 0x523f462a, ROM_SKIP(7) )
	ROMX_LOAD( "12.bin",       0x300000, 0x20000, 0x38652339, ROM_SKIP(7) )
	ROMX_LOAD( "04.bin",       0x300001, 0x20000, 0x4951bc0f, ROM_SKIP(7) )
	ROMX_LOAD( "16.bin",       0x300002, 0x20000, 0x381608ae, ROM_SKIP(7) )
	ROMX_LOAD( "08.bin",       0x300003, 0x20000, 0xb475d4e9, ROM_SKIP(7) )
	ROMX_LOAD( "27.bin",       0x300004, 0x20000, 0xa27e81fa, ROM_SKIP(7) )
	ROMX_LOAD( "20.bin",       0x300005, 0x20000, 0x002796dc, ROM_SKIP(7) )
	ROMX_LOAD( "29.bin",       0x300006, 0x20000, 0x6b41f82d, ROM_SKIP(7) )
	ROMX_LOAD( "22.bin",       0x300007, 0x20000, 0x52145369, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "23.bin",        0x00000, 0x08000, 0xb3b79d4f )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "30.bin",       0x00000, 0x20000, 0x7e5f6cb4 )
	ROM_LOAD( "31.bin",       0x20000, 0x20000, 0x4a30c737 )
ROM_END

ROM_START( willow )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "wlu_30.rom",   0x00000, 0x20000, 0xd604dbb1 )
	ROM_LOAD16_BYTE( "wlu_35.rom",   0x00001, 0x20000, 0xdaee72fe )
	ROM_LOAD16_BYTE( "wlu_31.rom",   0x40000, 0x20000, 0x0eb48a83 )
	ROM_LOAD16_BYTE( "wlu_36.rom",   0x40001, 0x20000, 0x36100209 )
	ROM_LOAD16_WORD_SWAP( "wl_32.rom",    0x80000, 0x80000, 0xdfd9f643 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "wl_gfx5.rom",  0x000000, 0x80000, 0xafa74b73, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wl_gfx7.rom",  0x000002, 0x80000, 0x12a0dc0b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wl_gfx1.rom",  0x000004, 0x80000, 0xc6f2abce, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wl_gfx3.rom",  0x000006, 0x80000, 0x4aa4c6d3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wl_24.rom",    0x200000, 0x20000, 0x6f0adee5, ROM_SKIP(7) )
	ROMX_LOAD( "wl_14.rom",    0x200001, 0x20000, 0x9cf3027d, ROM_SKIP(7) )
	ROMX_LOAD( "wl_26.rom",    0x200002, 0x20000, 0xf09c8ecf, ROM_SKIP(7) )
	ROMX_LOAD( "wl_16.rom",    0x200003, 0x20000, 0xe35407aa, ROM_SKIP(7) )
	ROMX_LOAD( "wl_20.rom",    0x200004, 0x20000, 0x84992350, ROM_SKIP(7) )
	ROMX_LOAD( "wl_10.rom",    0x200005, 0x20000, 0xb87b5a36, ROM_SKIP(7) )
	ROMX_LOAD( "wl_22.rom",    0x200006, 0x20000, 0xfd3f89f0, ROM_SKIP(7) )
	ROMX_LOAD( "wl_12.rom",    0x200007, 0x20000, 0x7da49d69, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "wl_09.rom",     0x00000, 0x08000, 0xf6b3d060 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "wl_18.rom",    0x00000, 0x20000, 0xbde23d4d )
	ROM_LOAD( "wl_19.rom",    0x20000, 0x20000, 0x683898f5 )
ROM_END

ROM_START( willowj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "wl36.bin",     0x00000, 0x20000, 0x2b0d7cbc )
	ROM_LOAD16_BYTE( "wl42.bin",     0x00001, 0x20000, 0x1ac39615 )
	ROM_LOAD16_BYTE( "wl37.bin",     0x40000, 0x20000, 0x30a717fa )
	ROM_LOAD16_BYTE( "wl43.bin",     0x40001, 0x20000, 0xd0dddc9e )
	ROM_LOAD16_WORD_SWAP( "wl_32.rom",    0x80000, 0x80000, 0xdfd9f643 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "wl_gfx5.rom",  0x000000, 0x80000, 0xafa74b73, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wl_gfx7.rom",  0x000002, 0x80000, 0x12a0dc0b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wl_gfx1.rom",  0x000004, 0x80000, 0xc6f2abce, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wl_gfx3.rom",  0x000006, 0x80000, 0x4aa4c6d3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wl_24.rom",    0x200000, 0x20000, 0x6f0adee5, ROM_SKIP(7) )
	ROMX_LOAD( "wl_14.rom",    0x200001, 0x20000, 0x9cf3027d, ROM_SKIP(7) )
	ROMX_LOAD( "wl_26.rom",    0x200002, 0x20000, 0xf09c8ecf, ROM_SKIP(7) )
	ROMX_LOAD( "wl_16.rom",    0x200003, 0x20000, 0xe35407aa, ROM_SKIP(7) )
	ROMX_LOAD( "wl_20.rom",    0x200004, 0x20000, 0x84992350, ROM_SKIP(7) )
	ROMX_LOAD( "wl_10.rom",    0x200005, 0x20000, 0xb87b5a36, ROM_SKIP(7) )
	ROMX_LOAD( "wl_22.rom",    0x200006, 0x20000, 0xfd3f89f0, ROM_SKIP(7) )
	ROMX_LOAD( "wl_12.rom",    0x200007, 0x20000, 0x7da49d69, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "wl_09.rom",     0x00000, 0x08000, 0xf6b3d060 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "wl_18.rom",    0x00000, 0x20000, 0xbde23d4d )
	ROM_LOAD( "wl_19.rom",    0x20000, 0x20000, 0x683898f5 )
ROM_END

ROM_START( unsquad )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "unsquad.30",   0x00000, 0x20000, 0x24d8f88d )
	ROM_LOAD16_BYTE( "unsquad.35",   0x00001, 0x20000, 0x8b954b59 )
	ROM_LOAD16_BYTE( "unsquad.31",   0x40000, 0x20000, 0x33e9694b )
	ROM_LOAD16_BYTE( "unsquad.36",   0x40001, 0x20000, 0x7cc8fb9e )
	ROM_LOAD16_WORD_SWAP( "unsquad.32",   0x80000, 0x80000, 0xae1d7fb0 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "unsquad.05",   0x000000, 0x80000, 0xbf4575d8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "unsquad.07",   0x000002, 0x80000, 0xa02945f4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "unsquad.01",   0x000004, 0x80000, 0x5965ca8d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "unsquad.03",   0x000006, 0x80000, 0xac6db17d, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "unsquad.09",    0x00000, 0x08000, 0xf3dd1367 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "unsquad.18",   0x00000, 0x20000, 0x584b43a9 )
ROM_END

ROM_START( area88 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "ar36.bin",     0x00000, 0x20000, 0x65030392 )
	ROM_LOAD16_BYTE( "ar42.bin",     0x00001, 0x20000, 0xc48170de )
	ROM_LOAD16_BYTE( "unsquad.31",   0x40000, 0x20000, 0x33e9694b )
	ROM_LOAD16_BYTE( "unsquad.36",   0x40001, 0x20000, 0x7cc8fb9e )
	ROM_LOAD16_WORD_SWAP( "unsquad.32",   0x80000, 0x80000, 0xae1d7fb0 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "unsquad.05",   0x000000, 0x80000, 0xbf4575d8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "unsquad.07",   0x000002, 0x80000, 0xa02945f4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "unsquad.01",   0x000004, 0x80000, 0x5965ca8d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "unsquad.03",   0x000006, 0x80000, 0xac6db17d, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "unsquad.09",    0x00000, 0x08000, 0xf3dd1367 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "unsquad.18",   0x00000, 0x20000, 0x584b43a9 )
ROM_END

ROM_START( ffight )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "ff30-36.bin",  0x00000, 0x20000, 0xf9a5ce83 )
	ROM_LOAD16_BYTE( "ff35-42.bin",  0x00001, 0x20000, 0x65f11215 )
	ROM_LOAD16_BYTE( "ff31-37.bin",  0x40000, 0x20000, 0xe1033784 )
	ROM_LOAD16_BYTE( "ff36-43.bin",  0x40001, 0x20000, 0x995e968a )
	ROM_LOAD16_WORD_SWAP( "ff32-32m.bin", 0x80000, 0x80000, 0xc747696e )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ff05-05m.bin", 0x000000, 0x80000, 0x9c284108, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ff07-07m.bin", 0x000002, 0x80000, 0xa7584dfb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ff01-01m.bin", 0x000004, 0x80000, 0x0b605e44, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ff03-03m.bin", 0x000006, 0x80000, 0x52291cd2, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ff09-09.bin",   0x00000, 0x08000, 0xb8367eb5 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ff18-18.bin",  0x00000, 0x20000, 0x375c66e7 )
	ROM_LOAD( "ff19-19.bin",  0x20000, 0x20000, 0x1ef137f9 )
ROM_END

ROM_START( ffightu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "36",           0x00000, 0x20000, 0xe2a48af9 )
	ROM_LOAD16_BYTE( "42",           0x00001, 0x20000, 0xf4bb480e )
	ROM_LOAD16_BYTE( "37",           0x40000, 0x20000, 0xc371c667 )
	ROM_LOAD16_BYTE( "43",           0x40001, 0x20000, 0x2f5771f9 )
	ROM_LOAD16_WORD_SWAP( "ff32-32m.bin", 0x80000, 0x80000, 0xc747696e )

	/* Note: the gfx ROMs were missing from this set. I used the ones from */
	/* the World version, assuming the if the scantily clad woman shouldn't */
	/* be seen in Europe, it shouldn't be seen in the USA as well. */
	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ff05-05m.bin", 0x000000, 0x80000, 0x9c284108, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ff07-07m.bin", 0x000002, 0x80000, 0xa7584dfb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ff01-01m.bin", 0x000004, 0x80000, 0x0b605e44, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ff03-03m.bin", 0x000006, 0x80000, 0x52291cd2, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ff09-09.bin",   0x00000, 0x08000, 0xb8367eb5 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ff18-18.bin",  0x00000, 0x20000, 0x375c66e7 )
	ROM_LOAD( "ff19-19.bin",  0x20000, 0x20000, 0x1ef137f9 )
ROM_END

ROM_START( ffightj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "ff30-36.bin",  0x00000, 0x20000, 0xf9a5ce83 )
	ROM_LOAD16_BYTE( "ff35-42.bin",  0x00001, 0x20000, 0x65f11215 )
	ROM_LOAD16_BYTE( "ff31-37.bin",  0x40000, 0x20000, 0xe1033784 )
	ROM_LOAD16_BYTE( "ff43.bin",     0x40001, 0x20000, 0xb6dee1c3 )
	ROM_LOAD16_WORD_SWAP( "ff32-32m.bin", 0x80000, 0x80000, 0xc747696e )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ff09.bin",     0x000000, 0x20000, 0x5b116d0d, ROM_SKIP(7) )
	ROMX_LOAD( "ff01.bin",     0x000001, 0x20000, 0x815b1797, ROM_SKIP(7) )
	ROMX_LOAD( "ff13.bin",     0x000002, 0x20000, 0x8721a7da, ROM_SKIP(7) )
	ROMX_LOAD( "ff05.bin",     0x000003, 0x20000, 0xd0fcd4b5, ROM_SKIP(7) )
	ROMX_LOAD( "ff24.bin",     0x000004, 0x20000, 0xa1ab607a, ROM_SKIP(7) )
	ROMX_LOAD( "ff17.bin",     0x000005, 0x20000, 0x2dc18cf4, ROM_SKIP(7) )
	ROMX_LOAD( "ff38.bin",     0x000006, 0x20000, 0x6535a57f, ROM_SKIP(7) )
	ROMX_LOAD( "ff32.bin",     0x000007, 0x20000, 0xc8bc4a57, ROM_SKIP(7) )
	ROMX_LOAD( "ff10.bin",     0x100000, 0x20000, 0x624a924a, ROM_SKIP(7) )
	ROMX_LOAD( "ff02.bin",     0x100001, 0x20000, 0x5d91f694, ROM_SKIP(7) )
	ROMX_LOAD( "ff14.bin",     0x100002, 0x20000, 0x0a2e9101, ROM_SKIP(7) )
	ROMX_LOAD( "ff06.bin",     0x100003, 0x20000, 0x1c18f042, ROM_SKIP(7) )
	ROMX_LOAD( "ff25.bin",     0x100004, 0x20000, 0x6e8181ea, ROM_SKIP(7) )
	ROMX_LOAD( "ff18.bin",     0x100005, 0x20000, 0xb19ede59, ROM_SKIP(7) )
	ROMX_LOAD( "ff39.bin",     0x100006, 0x20000, 0x9416b477, ROM_SKIP(7) )
	ROMX_LOAD( "ff33.bin",     0x100007, 0x20000, 0x7369fa07, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ff09-09.bin",   0x00000, 0x08000, 0xb8367eb5 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ff18-18.bin",  0x00000, 0x20000, 0x375c66e7 )
	ROM_LOAD( "ff19-19.bin",  0x20000, 0x20000, 0x1ef137f9 )
ROM_END

ROM_START( 1941 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "41e_30.rom",   0x00000, 0x20000, 0x9deb1e75 )
	ROM_LOAD16_BYTE( "41e_35.rom",   0x00001, 0x20000, 0xd63942b3 )
	ROM_LOAD16_BYTE( "41e_31.rom",   0x40000, 0x20000, 0xdf201112 )
	ROM_LOAD16_BYTE( "41e_36.rom",   0x40001, 0x20000, 0x816a818f )
	ROM_LOAD16_WORD_SWAP( "41_32.rom",    0x80000, 0x80000, 0x4e9648ca )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "41_gfx5.rom",  0x000000, 0x80000, 0x01d1cb11, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "41_gfx7.rom",  0x000002, 0x80000, 0xaeaa3509, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "41_gfx1.rom",  0x000004, 0x80000, 0xff77985a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "41_gfx3.rom",  0x000006, 0x80000, 0x983be58f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "41_09.rom",     0x00000, 0x08000, 0x0f9d8527 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "41_18.rom",    0x00000, 0x20000, 0xd1f15aeb )
	ROM_LOAD( "41_19.rom",    0x20000, 0x20000, 0x15aec3a6 )
ROM_END

ROM_START( 1941j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "4136.bin",     0x00000, 0x20000, 0x7fbd42ab )
	ROM_LOAD16_BYTE( "4142.bin",     0x00001, 0x20000, 0xc7781f89 )
	ROM_LOAD16_BYTE( "4137.bin",     0x40000, 0x20000, 0xc6464b0b )
	ROM_LOAD16_BYTE( "4143.bin",     0x40001, 0x20000, 0x440fc0b5 )
	ROM_LOAD16_WORD_SWAP( "41_32.rom",    0x80000, 0x80000, 0x4e9648ca )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "41_gfx5.rom",  0x000000, 0x80000, 0x01d1cb11, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "41_gfx7.rom",  0x000002, 0x80000, 0xaeaa3509, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "41_gfx1.rom",  0x000004, 0x80000, 0xff77985a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "41_gfx3.rom",  0x000006, 0x80000, 0x983be58f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "41_09.rom",     0x00000, 0x08000, 0x0f9d8527 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "41_18.rom",    0x00000, 0x20000, 0xd1f15aeb )
	ROM_LOAD( "41_19.rom",    0x20000, 0x20000, 0x15aec3a6 )
ROM_END

ROM_START( mercs )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "so2_30e.rom",  0x00000, 0x20000, 0xe17f9bf7 )
	ROM_LOAD16_BYTE( "so2_35e.rom",  0x00001, 0x20000, 0x78e63575 )
	ROM_LOAD16_BYTE( "so2_31e.rom",  0x40000, 0x20000, 0x51204d36 )
	ROM_LOAD16_BYTE( "so2_36e.rom",  0x40001, 0x20000, 0x9cfba8b4 )
	ROM_LOAD16_WORD_SWAP( "so2_32.rom",   0x80000, 0x80000, 0x2eb5cf0c )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "so2_gfx6.rom", 0x000000, 0x80000, 0xaa6102af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx8.rom", 0x000002, 0x80000, 0x839e6869, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx2.rom", 0x000004, 0x80000, 0x597c2875, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx4.rom", 0x000006, 0x80000, 0x912a9ca0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_24.rom",   0x200000, 0x20000, 0x3f254efe, ROM_SKIP(7) )
	ROMX_LOAD( "so2_14.rom",   0x200001, 0x20000, 0xf5a8905e, ROM_SKIP(7) )
	ROMX_LOAD( "so2_26.rom",   0x200002, 0x20000, 0xf3aa5a4a, ROM_SKIP(7) )
	ROMX_LOAD( "so2_16.rom",   0x200003, 0x20000, 0xb43cd1a8, ROM_SKIP(7) )
	ROMX_LOAD( "so2_20.rom",   0x200004, 0x20000, 0x8ca751a3, ROM_SKIP(7) )
	ROMX_LOAD( "so2_10.rom",   0x200005, 0x20000, 0xe9f569fd, ROM_SKIP(7) )
	ROMX_LOAD( "so2_22.rom",   0x200006, 0x20000, 0xfce9a377, ROM_SKIP(7) )
	ROMX_LOAD( "so2_12.rom",   0x200007, 0x20000, 0xb7df8a06, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "so2_09.rom",    0x00000, 0x08000, 0xd09d7c7a )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "so2_18.rom",   0x00000, 0x20000, 0xbbea1643 )
	ROM_LOAD( "so2_19.rom",   0x20000, 0x20000, 0xac58aa71 )
ROM_END

ROM_START( mercsu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "so2_30e.rom",  0x00000, 0x20000, 0xe17f9bf7 )
	ROM_LOAD16_BYTE( "s02-35",       0x00001, 0x20000, 0x4477df61 )
	ROM_LOAD16_BYTE( "so2_31e.rom",  0x40000, 0x20000, 0x51204d36 )
	ROM_LOAD16_BYTE( "so2_36e.rom",  0x40001, 0x20000, 0x9cfba8b4 )
	ROM_LOAD16_WORD_SWAP( "so2_32.rom",   0x80000, 0x80000, 0x2eb5cf0c )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "so2_gfx6.rom", 0x000000, 0x80000, 0xaa6102af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx8.rom", 0x000002, 0x80000, 0x839e6869, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx2.rom", 0x000004, 0x80000, 0x597c2875, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx4.rom", 0x000006, 0x80000, 0x912a9ca0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_24.rom",   0x200000, 0x20000, 0x3f254efe, ROM_SKIP(7) )
	ROMX_LOAD( "so2_14.rom",   0x200001, 0x20000, 0xf5a8905e, ROM_SKIP(7) )
	ROMX_LOAD( "so2_26.rom",   0x200002, 0x20000, 0xf3aa5a4a, ROM_SKIP(7) )
	ROMX_LOAD( "so2_16.rom",   0x200003, 0x20000, 0xb43cd1a8, ROM_SKIP(7) )
	ROMX_LOAD( "so2_20.rom",   0x200004, 0x20000, 0x8ca751a3, ROM_SKIP(7) )
	ROMX_LOAD( "so2_10.rom",   0x200005, 0x20000, 0xe9f569fd, ROM_SKIP(7) )
	ROMX_LOAD( "so2_22.rom",   0x200006, 0x20000, 0xfce9a377, ROM_SKIP(7) )
	ROMX_LOAD( "so2_12.rom",   0x200007, 0x20000, 0xb7df8a06, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "so2_09.rom",    0x00000, 0x08000, 0xd09d7c7a )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "so2_18.rom",   0x00000, 0x20000, 0xbbea1643 )
	ROM_LOAD( "so2_19.rom",   0x20000, 0x20000, 0xac58aa71 )
ROM_END

ROM_START( mercsj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "so2_30e.rom",  0x00000, 0x20000, 0xe17f9bf7 )
	ROM_LOAD16_BYTE( "so2_42.bin",   0x00001, 0x20000, 0x2c3884c6 )
	ROM_LOAD16_BYTE( "so2_31e.rom",  0x40000, 0x20000, 0x51204d36 )
	ROM_LOAD16_BYTE( "so2_36e.rom",  0x40001, 0x20000, 0x9cfba8b4 )
	ROM_LOAD16_WORD_SWAP( "so2_32.rom",   0x80000, 0x80000, 0x2eb5cf0c )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "so2_gfx6.rom", 0x000000, 0x80000, 0xaa6102af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx8.rom", 0x000002, 0x80000, 0x839e6869, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx2.rom", 0x000004, 0x80000, 0x597c2875, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_gfx4.rom", 0x000006, 0x80000, 0x912a9ca0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "so2_24.rom",   0x200000, 0x20000, 0x3f254efe, ROM_SKIP(7) )
	ROMX_LOAD( "so2_14.rom",   0x200001, 0x20000, 0xf5a8905e, ROM_SKIP(7) )
	ROMX_LOAD( "so2_26.rom",   0x200002, 0x20000, 0xf3aa5a4a, ROM_SKIP(7) )
	ROMX_LOAD( "so2_16.rom",   0x200003, 0x20000, 0xb43cd1a8, ROM_SKIP(7) )
	ROMX_LOAD( "so2_20.rom",   0x200004, 0x20000, 0x8ca751a3, ROM_SKIP(7) )
	ROMX_LOAD( "so2_10.rom",   0x200005, 0x20000, 0xe9f569fd, ROM_SKIP(7) )
	ROMX_LOAD( "so2_22.rom",   0x200006, 0x20000, 0xfce9a377, ROM_SKIP(7) )
	ROMX_LOAD( "so2_12.rom",   0x200007, 0x20000, 0xb7df8a06, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "so2_09.rom",    0x00000, 0x08000, 0xd09d7c7a )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "so2_18.rom",   0x00000, 0x20000, 0xbbea1643 )
	ROM_LOAD( "so2_19.rom",   0x20000, 0x20000, 0xac58aa71 )
ROM_END

ROM_START( mtwins )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "che_30.rom",   0x00000, 0x20000, 0x9a2a2db1 )
	ROM_LOAD16_BYTE( "che_35.rom",   0x00001, 0x20000, 0xa7f96b02 )
	ROM_LOAD16_BYTE( "che_31.rom",   0x40000, 0x20000, 0xbbff8a99 )
	ROM_LOAD16_BYTE( "che_36.rom",   0x40001, 0x20000, 0x0fa00c39 )
	ROM_LOAD16_WORD_SWAP( "ch_32.rom",    0x80000, 0x80000, 0x9b70bd41 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ch_gfx5.rom",  0x000000, 0x80000, 0x4ec75f15, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ch_gfx7.rom",  0x000002, 0x80000, 0xd85d00d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ch_gfx1.rom",  0x000004, 0x80000, 0xf33ca9d4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ch_gfx3.rom",  0x000006, 0x80000, 0x0ba2047f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ch_09.rom",     0x00000, 0x08000, 0x4d4255b7 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ch_18.rom",    0x00000, 0x20000, 0xf909e8de )
	ROM_LOAD( "ch_19.rom",    0x20000, 0x20000, 0xfc158cf7 )
ROM_END

ROM_START( chikij )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "chj36a.bin",   0x00000, 0x20000, 0xec1328d8 )
	ROM_LOAD16_BYTE( "chj42a.bin",   0x00001, 0x20000, 0x4ae13503 )
	ROM_LOAD16_BYTE( "chj37a.bin",   0x40000, 0x20000, 0x46d2cf7b )
	ROM_LOAD16_BYTE( "chj43a.bin",   0x40001, 0x20000, 0x8d387fe8 )
	ROM_LOAD16_WORD_SWAP( "ch_32.rom",    0x80000, 0x80000, 0x9b70bd41 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ch_gfx5.rom",  0x000000, 0x80000, 0x4ec75f15, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ch_gfx7.rom",  0x000002, 0x80000, 0xd85d00d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ch_gfx1.rom",  0x000004, 0x80000, 0xf33ca9d4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ch_gfx3.rom",  0x000006, 0x80000, 0x0ba2047f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ch_09.rom",     0x00000, 0x08000, 0x4d4255b7 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ch_18.rom",    0x00000, 0x20000, 0xf909e8de )
	ROM_LOAD( "ch_19.rom",    0x20000, 0x20000, 0xfc158cf7 )
ROM_END

ROM_START( msword )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "mse_30.rom",   0x00000, 0x20000, 0x03fc8dbc )
	ROM_LOAD16_BYTE( "mse_35.rom",   0x00001, 0x20000, 0xd5bf66cd )
	ROM_LOAD16_BYTE( "mse_31.rom",   0x40000, 0x20000, 0x30332bcf )
	ROM_LOAD16_BYTE( "mse_36.rom",   0x40001, 0x20000, 0x8f7d6ce9 )
	ROM_LOAD16_WORD_SWAP( "ms_32.rom",    0x80000, 0x80000, 0x2475ddfc )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ms_gfx5.rom",  0x000000, 0x80000, 0xc00fe7e2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx7.rom",  0x000002, 0x80000, 0x4ccacac5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx1.rom",  0x000004, 0x80000, 0x0d2bbe00, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx3.rom",  0x000006, 0x80000, 0x3a1a5bf4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ms_9.rom",      0x00000, 0x08000, 0x57b29519 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ms_18.rom",    0x00000, 0x20000, 0xfb64e90d )
	ROM_LOAD( "ms_19.rom",    0x20000, 0x20000, 0x74f892b9 )
ROM_END

ROM_START( mswordu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "msu30",   0x00000, 0x20000, 0xd963c816 )
	ROM_LOAD16_BYTE( "msu35",   0x00001, 0x20000, 0x72f179b3 )
	ROM_LOAD16_BYTE( "msu31",   0x40000, 0x20000, 0x20cd7904 )
	ROM_LOAD16_BYTE( "msu36",   0x40001, 0x20000, 0xbf88c080 )
	ROM_LOAD16_WORD_SWAP( "ms_32.rom",    0x80000, 0x80000, 0x2475ddfc )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ms_gfx5.rom",  0x000000, 0x80000, 0xc00fe7e2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx7.rom",  0x000002, 0x80000, 0x4ccacac5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx1.rom",  0x000004, 0x80000, 0x0d2bbe00, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx3.rom",  0x000006, 0x80000, 0x3a1a5bf4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ms_9.rom",      0x00000, 0x08000, 0x57b29519 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ms_18.rom",    0x00000, 0x20000, 0xfb64e90d )
	ROM_LOAD( "ms_19.rom",    0x20000, 0x20000, 0x74f892b9 )
ROM_END

ROM_START( mswordj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "msj_36.bin",   0x00000, 0x20000, 0x04f0ef50 )
	ROM_LOAD16_BYTE( "msj_42.bin",   0x00001, 0x20000, 0x9fcbb9cd )
	ROM_LOAD16_BYTE( "msj_37.bin",   0x40000, 0x20000, 0x6c060d70 )
	ROM_LOAD16_BYTE( "msj_43.bin",   0x40001, 0x20000, 0xaec77787 )
	ROM_LOAD16_WORD_SWAP( "ms_32.rom",    0x80000, 0x80000, 0x2475ddfc )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ms_gfx5.rom",  0x000000, 0x80000, 0xc00fe7e2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx7.rom",  0x000002, 0x80000, 0x4ccacac5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx1.rom",  0x000004, 0x80000, 0x0d2bbe00, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ms_gfx3.rom",  0x000006, 0x80000, 0x3a1a5bf4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ms_9.rom",      0x00000, 0x08000, 0x57b29519 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ms_18.rom",    0x00000, 0x20000, 0xfb64e90d )
	ROM_LOAD( "ms_19.rom",    0x20000, 0x20000, 0x74f892b9 )
ROM_END

ROM_START( cawing )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "cae_30a.rom",  0x00000, 0x20000, 0x91fceacd )
	ROM_LOAD16_BYTE( "cae_35a.rom",  0x00001, 0x20000, 0x3ef03083 )
	ROM_LOAD16_BYTE( "cae_31a.rom",  0x40000, 0x20000, 0xe5b75caf )
	ROM_LOAD16_BYTE( "cae_36a.rom",  0x40001, 0x20000, 0xc73fd713 )
	ROM_LOAD16_WORD_SWAP( "ca_32.rom", 0x80000, 0x80000, 0x0c4837d4 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "ca_gfx5.rom",  0x000000, 0x80000, 0x66d4cc37, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ca_gfx7.rom",  0x000002, 0x80000, 0xb6f896f2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ca_gfx1.rom",  0x000004, 0x80000, 0x4d0620fd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ca_gfx3.rom",  0x000006, 0x80000, 0x0b0341c3, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ca_9.rom",      0x00000, 0x08000, 0x96fe7485 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ca_18.rom",    0x00000, 0x20000, 0x4a613a2c )
	ROM_LOAD( "ca_19.rom",    0x20000, 0x20000, 0x74584493 )
ROM_END

ROM_START( cawingj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "cae_30a.rom",  0x00000, 0x20000, 0x91fceacd )
	ROM_LOAD16_BYTE( "caj42a.bin",   0x00001, 0x20000, 0x039f8362 )
	ROM_LOAD16_BYTE( "cae_31a.rom",  0x40000, 0x20000, 0xe5b75caf )
	ROM_LOAD16_BYTE( "cae_36a.rom",  0x40001, 0x20000, 0xc73fd713 )
	ROM_LOAD16_BYTE( "caj34.bin",    0x80000, 0x20000, 0x51ea57f4 )
	ROM_LOAD16_BYTE( "caj40.bin",    0x80001, 0x20000, 0x2ab71ae1 )
	ROM_LOAD16_BYTE( "caj35.bin",    0xc0000, 0x20000, 0x01d71973 )
	ROM_LOAD16_BYTE( "caj41.bin",    0xc0001, 0x20000, 0x3a43b538 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "caj09.bin",    0x000000, 0x20000, 0x41b0f9a6, ROM_SKIP(7) )
	ROMX_LOAD( "caj01.bin",    0x000001, 0x20000, 0x1002d0b8, ROM_SKIP(7) )
	ROMX_LOAD( "caj13.bin",    0x000002, 0x20000, 0x6f3948b2, ROM_SKIP(7) )
	ROMX_LOAD( "caj05.bin",    0x000003, 0x20000, 0x207373d7, ROM_SKIP(7) )
	ROMX_LOAD( "caj24.bin",    0x000004, 0x20000, 0xe356aad7, ROM_SKIP(7) )
	ROMX_LOAD( "caj17.bin",    0x000005, 0x20000, 0x540f2fd8, ROM_SKIP(7) )
	ROMX_LOAD( "caj38.bin",    0x000006, 0x20000, 0x2464d4ab, ROM_SKIP(7) )
	ROMX_LOAD( "caj32.bin",    0x000007, 0x20000, 0x9b5836b3, ROM_SKIP(7) )
	ROMX_LOAD( "caj10.bin",    0x100000, 0x20000, 0xbf8a5f52, ROM_SKIP(7) )
	ROMX_LOAD( "caj02.bin",    0x100001, 0x20000, 0x125b018d, ROM_SKIP(7) )
	ROMX_LOAD( "caj14.bin",    0x100002, 0x20000, 0x8458e7d7, ROM_SKIP(7) )
	ROMX_LOAD( "caj06.bin",    0x100003, 0x20000, 0xcf80e164, ROM_SKIP(7) )
	ROMX_LOAD( "caj25.bin",    0x100004, 0x20000, 0xcdd0204d, ROM_SKIP(7) )
	ROMX_LOAD( "caj18.bin",    0x100005, 0x20000, 0x29c1d4b1, ROM_SKIP(7) )
	ROMX_LOAD( "caj39.bin",    0x100006, 0x20000, 0xeea23b67, ROM_SKIP(7) )
	ROMX_LOAD( "caj33.bin",    0x100007, 0x20000, 0xdde3891f, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ca_9.rom",      0x00000, 0x08000, 0x96fe7485 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ca_18.rom",    0x00000, 0x20000, 0x4a613a2c )
	ROM_LOAD( "ca_19.rom",    0x20000, 0x20000, 0x74584493 )
ROM_END

ROM_START( nemo )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "nme_30a.rom",  0x00000, 0x20000, 0xd2c03e56 )
	ROM_LOAD16_BYTE( "nme_35a.rom",  0x00001, 0x20000, 0x5fd31661 )
	ROM_LOAD16_BYTE( "nme_31a.rom",  0x40000, 0x20000, 0xb2bd4f6f )
	ROM_LOAD16_BYTE( "nme_36a.rom",  0x40001, 0x20000, 0xee9450e3 )
	ROM_LOAD16_WORD_SWAP( "nm_32.rom",    0x80000, 0x80000, 0xd6d1add3 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "nm_gfx5.rom",  0x000000, 0x80000, 0x487b8747, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nm_gfx7.rom",  0x000002, 0x80000, 0x203dc8c6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nm_gfx1.rom",  0x000004, 0x80000, 0x9e878024, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nm_gfx3.rom",  0x000006, 0x80000, 0xbb01e6b6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "nm_09.rom",     0x00000, 0x08000, 0x0f4b0581 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "nm_18.rom",    0x00000, 0x20000, 0xbab333d4 )
	ROM_LOAD( "nm_19.rom",    0x20000, 0x20000, 0x2650a0a8 )
ROM_END

ROM_START( nemoj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "nm36.bin",     0x00000, 0x20000, 0xdaeceabb )
	ROM_LOAD16_BYTE( "nm42.bin",     0x00001, 0x20000, 0x55024740 )
	ROM_LOAD16_BYTE( "nm37.bin",     0x40000, 0x20000, 0x619068b6 )
	ROM_LOAD16_BYTE( "nm43.bin",     0x40001, 0x20000, 0xa948a53b )
	ROM_LOAD16_WORD_SWAP( "nm_32.rom",    0x80000, 0x80000, 0xd6d1add3 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "nm_gfx5.rom",  0x000000, 0x80000, 0x487b8747, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nm_gfx7.rom",  0x000002, 0x80000, 0x203dc8c6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nm_gfx1.rom",  0x000004, 0x80000, 0x9e878024, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "nm_gfx3.rom",  0x000006, 0x80000, 0xbb01e6b6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "nm_09.rom",     0x00000, 0x08000, 0x0f4b0581 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "nm_18.rom",    0x00000, 0x20000, 0xbab333d4 )
	ROM_LOAD( "nm_19.rom",    0x20000, 0x20000, 0x2650a0a8 )
ROM_END

ROM_START( sf2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sf2_30a.bin",   0x00000, 0x20000, 0x57bd7051 )
	ROM_LOAD16_BYTE( "sf2e_37b.rom",  0x00001, 0x20000, 0x62691cdd )
	ROM_LOAD16_BYTE( "sf2_31a.bin",   0x40000, 0x20000, 0xa673143d )
	ROM_LOAD16_BYTE( "sf2_38a.bin",   0x40001, 0x20000, 0x4c2ccef7 )
	ROM_LOAD16_BYTE( "sf2_28a.bin",   0x80000, 0x20000, 0x4009955e )
	ROM_LOAD16_BYTE( "sf2_35a.bin",   0x80001, 0x20000, 0x8c1f3994 )
	ROM_LOAD16_BYTE( "sf2_29a.bin",   0xc0000, 0x20000, 0xbb4af315 )
	ROM_LOAD16_BYTE( "sf2_36a.bin",   0xc0001, 0x20000, 0xc02a13eb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "sf2_06.bin", 0x000000, 0x80000, 0x22c9cc8e, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx02.rom */
	ROMX_LOAD( "sf2_08.bin", 0x000002, 0x80000, 0x57213be8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx04.rom */
	ROMX_LOAD( "sf2_05.bin", 0x000004, 0x80000, 0xba529b4f, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx01.rom */
	ROMX_LOAD( "sf2_07.bin", 0x000006, 0x80000, 0x4b1b33a8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx03.rom */
	ROMX_LOAD( "sf2_15.bin", 0x200000, 0x80000, 0x2c7e2229, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx11.rom */
	ROMX_LOAD( "sf2_17.bin", 0x200002, 0x80000, 0xb5548f17, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx13.rom */
	ROMX_LOAD( "sf2_14.bin", 0x200004, 0x80000, 0x14b84312, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx10.rom */
	ROMX_LOAD( "sf2_16.bin", 0x200006, 0x80000, 0x5e9cd89a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx12.rom */
	ROMX_LOAD( "sf2_25.bin", 0x400000, 0x80000, 0x994bfa58, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx21.rom */
	ROMX_LOAD( "sf2_27.bin", 0x400002, 0x80000, 0x3e66ad9d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx23.rom */
	ROMX_LOAD( "sf2_24.bin", 0x400004, 0x80000, 0xc1befaa8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx20.rom */
	ROMX_LOAD( "sf2_26.bin", 0x400006, 0x80000, 0x0627c831, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx22.rom */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2_09.bin",    0x00000, 0x08000, 0xa4823a1b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sf2_18.bin",       0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "sf2_19.bin",       0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2ua )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sf2u.30a",      0x00000, 0x20000, 0x08beb861 )
	ROM_LOAD16_BYTE( "sf2u.37a",      0x00001, 0x20000, 0xb7638d69 )
	ROM_LOAD16_BYTE( "sf2u.31a",      0x40000, 0x20000, 0x0d5394e0 )
	ROM_LOAD16_BYTE( "sf2u.38a",      0x40001, 0x20000, 0x42d6a79e )
	ROM_LOAD16_BYTE( "sf2u.28a",      0x80000, 0x20000, 0x387a175c )
	ROM_LOAD16_BYTE( "sf2u.35a",      0x80001, 0x20000, 0xa1a5adcc )
	ROM_LOAD16_BYTE( "sf2_29a.bin",   0xc0000, 0x20000, 0xbb4af315 )
	ROM_LOAD16_BYTE( "sf2_36a.bin",   0xc0001, 0x20000, 0xc02a13eb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "sf2_06.bin", 0x000000, 0x80000, 0x22c9cc8e, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx02.rom */
	ROMX_LOAD( "sf2_08.bin", 0x000002, 0x80000, 0x57213be8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx04.rom */
	ROMX_LOAD( "sf2_05.bin", 0x000004, 0x80000, 0xba529b4f, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx01.rom */
	ROMX_LOAD( "sf2_07.bin", 0x000006, 0x80000, 0x4b1b33a8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx03.rom */
	ROMX_LOAD( "sf2_15.bin", 0x200000, 0x80000, 0x2c7e2229, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx11.rom */
	ROMX_LOAD( "sf2_17.bin", 0x200002, 0x80000, 0xb5548f17, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx13.rom */
	ROMX_LOAD( "sf2_14.bin", 0x200004, 0x80000, 0x14b84312, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx10.rom */
	ROMX_LOAD( "sf2_16.bin", 0x200006, 0x80000, 0x5e9cd89a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx12.rom */
	ROMX_LOAD( "sf2_25.bin", 0x400000, 0x80000, 0x994bfa58, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx21.rom */
	ROMX_LOAD( "sf2_27.bin", 0x400002, 0x80000, 0x3e66ad9d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx23.rom */
	ROMX_LOAD( "sf2_24.bin", 0x400004, 0x80000, 0xc1befaa8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx20.rom */
	ROMX_LOAD( "sf2_26.bin", 0x400006, 0x80000, 0x0627c831, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx22.rom */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2_09.bin",    0x00000, 0x08000, 0xa4823a1b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sf2_18.bin",       0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "sf2_19.bin",       0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2ub )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sf2_30a.bin",   0x00000, 0x20000, 0x57bd7051 )
	ROM_LOAD16_BYTE( "sf2u.37b",      0x00001, 0x20000, 0x4a54d479 )
	ROM_LOAD16_BYTE( "sf2_31a.bin",   0x40000, 0x20000, 0xa673143d )
	ROM_LOAD16_BYTE( "sf2_38a.bin",   0x40001, 0x20000, 0x4c2ccef7 )
	ROM_LOAD16_BYTE( "sf2_28a.bin",   0x80000, 0x20000, 0x4009955e )
	ROM_LOAD16_BYTE( "sf2_35a.bin",   0x80001, 0x20000, 0x8c1f3994 )
	ROM_LOAD16_BYTE( "sf2_29a.bin",   0xc0000, 0x20000, 0xbb4af315 )
	ROM_LOAD16_BYTE( "sf2_36a.bin",   0xc0001, 0x20000, 0xc02a13eb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "sf2_06.bin", 0x000000, 0x80000, 0x22c9cc8e, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx02.rom */
	ROMX_LOAD( "sf2_08.bin", 0x000002, 0x80000, 0x57213be8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx04.rom */
	ROMX_LOAD( "sf2_05.bin", 0x000004, 0x80000, 0xba529b4f, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx01.rom */
	ROMX_LOAD( "sf2_07.bin", 0x000006, 0x80000, 0x4b1b33a8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx03.rom */
	ROMX_LOAD( "sf2_15.bin", 0x200000, 0x80000, 0x2c7e2229, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx11.rom */
	ROMX_LOAD( "sf2_17.bin", 0x200002, 0x80000, 0xb5548f17, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx13.rom */
	ROMX_LOAD( "sf2_14.bin", 0x200004, 0x80000, 0x14b84312, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx10.rom */
	ROMX_LOAD( "sf2_16.bin", 0x200006, 0x80000, 0x5e9cd89a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx12.rom */
	ROMX_LOAD( "sf2_25.bin", 0x400000, 0x80000, 0x994bfa58, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx21.rom */
	ROMX_LOAD( "sf2_27.bin", 0x400002, 0x80000, 0x3e66ad9d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx23.rom */
	ROMX_LOAD( "sf2_24.bin", 0x400004, 0x80000, 0xc1befaa8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx20.rom */
	ROMX_LOAD( "sf2_26.bin", 0x400006, 0x80000, 0x0627c831, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx22.rom */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2_09.bin",    0x00000, 0x08000, 0xa4823a1b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sf2_18.bin",       0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "sf2_19.bin",       0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2ue )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sf2u.30e",      0x00000, 0x20000, 0xf37cd088 )
	ROM_LOAD16_BYTE( "sf2u.37e",      0x00001, 0x20000, 0x6c61a513 )
	ROM_LOAD16_BYTE( "sf2u.31e",      0x40000, 0x20000, 0x7c4771b4 )
	ROM_LOAD16_BYTE( "sf2u.38e",      0x40001, 0x20000, 0xa4bd0cd9 )
	ROM_LOAD16_BYTE( "sf2u.28e",      0x80000, 0x20000, 0xe3b95625 )
	ROM_LOAD16_BYTE( "sf2u.35e",      0x80001, 0x20000, 0x3648769a )
	ROM_LOAD16_BYTE( "sf2_29a.bin",   0xc0000, 0x20000, 0xbb4af315 )
	ROM_LOAD16_BYTE( "sf2_36a.bin",   0xc0001, 0x20000, 0xc02a13eb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "sf2_06.bin", 0x000000, 0x80000, 0x22c9cc8e, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx02.rom */
	ROMX_LOAD( "sf2_08.bin", 0x000002, 0x80000, 0x57213be8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx04.rom */
	ROMX_LOAD( "sf2_05.bin", 0x000004, 0x80000, 0xba529b4f, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx01.rom */
	ROMX_LOAD( "sf2_07.bin", 0x000006, 0x80000, 0x4b1b33a8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx03.rom */
	ROMX_LOAD( "sf2_15.bin", 0x200000, 0x80000, 0x2c7e2229, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx11.rom */
	ROMX_LOAD( "sf2_17.bin", 0x200002, 0x80000, 0xb5548f17, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx13.rom */
	ROMX_LOAD( "sf2_14.bin", 0x200004, 0x80000, 0x14b84312, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx10.rom */
	ROMX_LOAD( "sf2_16.bin", 0x200006, 0x80000, 0x5e9cd89a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx12.rom */
	ROMX_LOAD( "sf2_25.bin", 0x400000, 0x80000, 0x994bfa58, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx21.rom */
	ROMX_LOAD( "sf2_27.bin", 0x400002, 0x80000, 0x3e66ad9d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx23.rom */
	ROMX_LOAD( "sf2_24.bin", 0x400004, 0x80000, 0xc1befaa8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx20.rom */
	ROMX_LOAD( "sf2_26.bin", 0x400006, 0x80000, 0x0627c831, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx22.rom */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2_09.bin",    0x00000, 0x08000, 0xa4823a1b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sf2_18.bin",       0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "sf2_19.bin",       0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2ui )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sf2u.30i",      0x00000, 0x20000, 0xfe39ee33 )
	ROM_LOAD16_BYTE( "sf2u.37i",      0x00001, 0x20000, 0x9df707dd )
	ROM_LOAD16_BYTE( "sf2u.31i",      0x40000, 0x20000, 0x69a0a301 )
	ROM_LOAD16_BYTE( "sf2u.38i",      0x40001, 0x20000, 0x4cb46daf )
	ROM_LOAD16_BYTE( "sf2u.28i",      0x80000, 0x20000, 0x1580be4c )
	ROM_LOAD16_BYTE( "sf2u.35i",      0x80001, 0x20000, 0x1468d185 )
	ROM_LOAD16_BYTE( "sf2_29a.bin",   0xc0000, 0x20000, 0xbb4af315 )
	ROM_LOAD16_BYTE( "sf2_36a.bin",   0xc0001, 0x20000, 0xc02a13eb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "sf2_06.bin", 0x000000, 0x80000, 0x22c9cc8e, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx02.rom */
	ROMX_LOAD( "sf2_08.bin", 0x000002, 0x80000, 0x57213be8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx04.rom */
	ROMX_LOAD( "sf2_05.bin", 0x000004, 0x80000, 0xba529b4f, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx01.rom */
	ROMX_LOAD( "sf2_07.bin", 0x000006, 0x80000, 0x4b1b33a8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx03.rom */
	ROMX_LOAD( "sf2_15.bin", 0x200000, 0x80000, 0x2c7e2229, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx11.rom */
	ROMX_LOAD( "sf2_17.bin", 0x200002, 0x80000, 0xb5548f17, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx13.rom */
	ROMX_LOAD( "sf2_14.bin", 0x200004, 0x80000, 0x14b84312, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx10.rom */
	ROMX_LOAD( "sf2_16.bin", 0x200006, 0x80000, 0x5e9cd89a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx12.rom */
	ROMX_LOAD( "sf2_25.bin", 0x400000, 0x80000, 0x994bfa58, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx21.rom */
	ROMX_LOAD( "sf2_27.bin", 0x400002, 0x80000, 0x3e66ad9d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx23.rom */
	ROMX_LOAD( "sf2_24.bin", 0x400004, 0x80000, 0xc1befaa8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx20.rom */
	ROMX_LOAD( "sf2_26.bin", 0x400006, 0x80000, 0x0627c831, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx22.rom */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2_09.bin",    0x00000, 0x08000, 0xa4823a1b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sf2_18.bin",       0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "sf2_19.bin",       0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sf2j30.bin",    0x00000, 0x20000, 0x79022b31 )
	ROM_LOAD16_BYTE( "sf2j37.bin",    0x00001, 0x20000, 0x516776ec )
	ROM_LOAD16_BYTE( "sf2j31.bin",    0x40000, 0x20000, 0xfe15cb39 )
	ROM_LOAD16_BYTE( "sf2j38.bin",    0x40001, 0x20000, 0x38614d70 )
	ROM_LOAD16_BYTE( "sf2j28.bin",    0x80000, 0x20000, 0xd283187a )
	ROM_LOAD16_BYTE( "sf2j35.bin",    0x80001, 0x20000, 0xd28158e4 )
	ROM_LOAD16_BYTE( "sf2_29a.bin",   0xc0000, 0x20000, 0xbb4af315 )
	ROM_LOAD16_BYTE( "sf2_36a.bin",   0xc0001, 0x20000, 0xc02a13eb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "sf2_06.bin", 0x000000, 0x80000, 0x22c9cc8e, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx02.rom */
	ROMX_LOAD( "sf2_08.bin", 0x000002, 0x80000, 0x57213be8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx04.rom */
	ROMX_LOAD( "sf2_05.bin", 0x000004, 0x80000, 0xba529b4f, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx01.rom */
	ROMX_LOAD( "sf2_07.bin", 0x000006, 0x80000, 0x4b1b33a8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx03.rom */
	ROMX_LOAD( "sf2_15.bin", 0x200000, 0x80000, 0x2c7e2229, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx11.rom */
	ROMX_LOAD( "sf2_17.bin", 0x200002, 0x80000, 0xb5548f17, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx13.rom */
	ROMX_LOAD( "sf2_14.bin", 0x200004, 0x80000, 0x14b84312, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx10.rom */
	ROMX_LOAD( "sf2_16.bin", 0x200006, 0x80000, 0x5e9cd89a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx12.rom */
	ROMX_LOAD( "sf2_25.bin", 0x400000, 0x80000, 0x994bfa58, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx21.rom */
	ROMX_LOAD( "sf2_27.bin", 0x400002, 0x80000, 0x3e66ad9d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx23.rom */
	ROMX_LOAD( "sf2_24.bin", 0x400004, 0x80000, 0xc1befaa8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx20.rom */
	ROMX_LOAD( "sf2_26.bin", 0x400006, 0x80000, 0x0627c831, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx22.rom */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2_09.bin",    0x00000, 0x08000, 0xa4823a1b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sf2_18.bin",       0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "sf2_19.bin",       0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2ja )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sf2_30a.bin",   0x00000, 0x20000, 0x57bd7051 )
	ROM_LOAD16_BYTE( "sf2j_37a.bin",  0x00001, 0x20000, 0x1e1f6844 )
	ROM_LOAD16_BYTE( "sf2_31a.bin",   0x40000, 0x20000, 0xa673143d )
	ROM_LOAD16_BYTE( "sf2_38a.bin",   0x40001, 0x20000, 0x4c2ccef7 )
	ROM_LOAD16_BYTE( "sf2_28a.bin",   0x80000, 0x20000, 0x4009955e )
	ROM_LOAD16_BYTE( "sf2_35a.bin",   0x80001, 0x20000, 0x8c1f3994 )
	ROM_LOAD16_BYTE( "sf2_29a.bin",   0xc0000, 0x20000, 0xbb4af315 )
	ROM_LOAD16_BYTE( "sf2_36a.bin",   0xc0001, 0x20000, 0xc02a13eb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "sf2_06.bin", 0x000000, 0x80000, 0x22c9cc8e, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx02.rom */
	ROMX_LOAD( "sf2_08.bin", 0x000002, 0x80000, 0x57213be8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx04.rom */
	ROMX_LOAD( "sf2_05.bin", 0x000004, 0x80000, 0xba529b4f, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx01.rom */
	ROMX_LOAD( "sf2_07.bin", 0x000006, 0x80000, 0x4b1b33a8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx03.rom */
	ROMX_LOAD( "sf2_15.bin", 0x200000, 0x80000, 0x2c7e2229, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx11.rom */
	ROMX_LOAD( "sf2_17.bin", 0x200002, 0x80000, 0xb5548f17, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx13.rom */
	ROMX_LOAD( "sf2_14.bin", 0x200004, 0x80000, 0x14b84312, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx10.rom */
	ROMX_LOAD( "sf2_16.bin", 0x200006, 0x80000, 0x5e9cd89a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx12.rom */
	ROMX_LOAD( "sf2_25.bin", 0x400000, 0x80000, 0x994bfa58, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx21.rom */
	ROMX_LOAD( "sf2_27.bin", 0x400002, 0x80000, 0x3e66ad9d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx23.rom */
	ROMX_LOAD( "sf2_24.bin", 0x400004, 0x80000, 0xc1befaa8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx20.rom */
	ROMX_LOAD( "sf2_26.bin", 0x400006, 0x80000, 0x0627c831, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx22.rom */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2_09.bin",    0x00000, 0x08000, 0xa4823a1b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sf2_18.bin",       0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "sf2_19.bin",       0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2jc )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "sf2_30c.bin",   0x00000, 0x20000, 0x8add35ec )
	ROM_LOAD16_BYTE( "sf2j_37c.bin",  0x00001, 0x20000, 0x0d74a256 )
	ROM_LOAD16_BYTE( "sf2_31c.bin",   0x40000, 0x20000, 0xc4fff4a9 )
	ROM_LOAD16_BYTE( "sf2_38c.bin",   0x40001, 0x20000, 0x8210fc0e )
	ROM_LOAD16_BYTE( "sf2_28c.bin",   0x80000, 0x20000, 0x6eddd5e8 )
	ROM_LOAD16_BYTE( "sf2_35c.bin",   0x80001, 0x20000, 0x6bcb404c )
	ROM_LOAD16_BYTE( "sf2_29a.bin",   0xc0000, 0x20000, 0xbb4af315 )
	ROM_LOAD16_BYTE( "sf2_36a.bin",   0xc0001, 0x20000, 0xc02a13eb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "sf2_06.bin", 0x000000, 0x80000, 0x22c9cc8e, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx02.rom */
	ROMX_LOAD( "sf2_08.bin", 0x000002, 0x80000, 0x57213be8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx04.rom */
	ROMX_LOAD( "sf2_05.bin", 0x000004, 0x80000, 0xba529b4f, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx01.rom */
	ROMX_LOAD( "sf2_07.bin", 0x000006, 0x80000, 0x4b1b33a8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx03.rom */
	ROMX_LOAD( "sf2_15.bin", 0x200000, 0x80000, 0x2c7e2229, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx11.rom */
	ROMX_LOAD( "sf2_17.bin", 0x200002, 0x80000, 0xb5548f17, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx13.rom */
	ROMX_LOAD( "sf2_14.bin", 0x200004, 0x80000, 0x14b84312, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx10.rom */
	ROMX_LOAD( "sf2_16.bin", 0x200006, 0x80000, 0x5e9cd89a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx12.rom */
	ROMX_LOAD( "sf2_25.bin", 0x400000, 0x80000, 0x994bfa58, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx21.rom */
	ROMX_LOAD( "sf2_27.bin", 0x400002, 0x80000, 0x3e66ad9d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx23.rom */
	ROMX_LOAD( "sf2_24.bin", 0x400004, 0x80000, 0xc1befaa8, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx20.rom */
	ROMX_LOAD( "sf2_26.bin", 0x400006, 0x80000, 0x0627c831, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2gfx22.rom */

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sf2_09.bin",    0x00000, 0x08000, 0xa4823a1b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "sf2_18.bin",       0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "sf2_19.bin",       0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( 3wonders )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "rte.30a",      0x00000, 0x20000, 0xef5b8b33 )
	ROM_LOAD16_BYTE( "rte.35a",      0x00001, 0x20000, 0x7d705529 )
	ROM_LOAD16_BYTE( "rte.31a",      0x40000, 0x20000, 0x32835e5e )
	ROM_LOAD16_BYTE( "rte.36a",      0x40001, 0x20000, 0x7637975f )
	ROM_LOAD16_BYTE( "3wonders.28",  0x80000, 0x20000, 0x054137c8 )
	ROM_LOAD16_BYTE( "3wonders.33",  0x80001, 0x20000, 0x7264cb1b )
	ROM_LOAD16_BYTE( "rte.29a",      0xc0000, 0x20000, 0xcddaa919 )
	ROM_LOAD16_BYTE( "rte.34a",      0xc0001, 0x20000, 0xed52e7e5 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "3wonders.05",  0x000000, 0x80000, 0x86aef804, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.07",  0x000002, 0x80000, 0x4f057110, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.01",  0x000004, 0x80000, 0x902489d0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.03",  0x000006, 0x80000, 0xe35ce720, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.06",  0x200000, 0x80000, 0x13cb0e7c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.08",  0x200002, 0x80000, 0x1f055014, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.02",  0x200004, 0x80000, 0xe9a034f4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.04",  0x200006, 0x80000, 0xdf0eea8b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "3wonders.09",   0x00000, 0x08000, 0xabfca165 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "3wonders.18",  0x00000, 0x20000, 0x26b211ab )
	ROM_LOAD( "3wonders.19",  0x20000, 0x20000, 0xdbe64ad0 )
ROM_END

ROM_START( 3wonderu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "3wonders.30",  0x00000, 0x20000, 0x0b156fd8 )
	ROM_LOAD16_BYTE( "3wonders.35",  0x00001, 0x20000, 0x57350bf4 )
	ROM_LOAD16_BYTE( "3wonders.31",  0x40000, 0x20000, 0x0e723fcc )
	ROM_LOAD16_BYTE( "3wonders.36",  0x40001, 0x20000, 0x523a45dc )
	ROM_LOAD16_BYTE( "3wonders.28",  0x80000, 0x20000, 0x054137c8 )
	ROM_LOAD16_BYTE( "3wonders.33",  0x80001, 0x20000, 0x7264cb1b )
	ROM_LOAD16_BYTE( "3wonders.29",  0xc0000, 0x20000, 0x37ba3e20 )
	ROM_LOAD16_BYTE( "3wonders.34",  0xc0001, 0x20000, 0xf99f46c0 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "3wonders.05",  0x000000, 0x80000, 0x86aef804, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.07",  0x000002, 0x80000, 0x4f057110, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.01",  0x000004, 0x80000, 0x902489d0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.03",  0x000006, 0x80000, 0xe35ce720, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.06",  0x200000, 0x80000, 0x13cb0e7c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.08",  0x200002, 0x80000, 0x1f055014, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.02",  0x200004, 0x80000, 0xe9a034f4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.04",  0x200006, 0x80000, 0xdf0eea8b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "3wonders.09",   0x00000, 0x08000, 0xabfca165 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "3wonders.18",  0x00000, 0x20000, 0x26b211ab )
	ROM_LOAD( "3wonders.19",  0x20000, 0x20000, 0xdbe64ad0 )
ROM_END

ROM_START( wonder3 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "rtj36.bin",    0x00000, 0x20000, 0xe3741247 )
	ROM_LOAD16_BYTE( "rtj42.bin",    0x00001, 0x20000, 0xb4baa117 )
	ROM_LOAD16_BYTE( "rtj37.bin",    0x40000, 0x20000, 0xa1f677b0 )
	ROM_LOAD16_BYTE( "rtj43.bin",    0x40001, 0x20000, 0x85337a47 )

	ROM_LOAD16_BYTE( "3wonders.28",  0x80000, 0x20000, 0x054137c8 )
	ROM_LOAD16_BYTE( "3wonders.33",  0x80001, 0x20000, 0x7264cb1b )
	ROM_LOAD16_BYTE( "rtj35.bin",    0xc0000, 0x20000, 0xe72f9ea3 )
	ROM_LOAD16_BYTE( "rtj41.bin",    0xc0001, 0x20000, 0xa11ee998 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "3wonders.05",  0x000000, 0x80000, 0x86aef804, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.07",  0x000002, 0x80000, 0x4f057110, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.01",  0x000004, 0x80000, 0x902489d0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.03",  0x000006, 0x80000, 0xe35ce720, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.06",  0x200000, 0x80000, 0x13cb0e7c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.08",  0x200002, 0x80000, 0x1f055014, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.02",  0x200004, 0x80000, 0xe9a034f4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "3wonders.04",  0x200006, 0x80000, 0xdf0eea8b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "rt23.bin",      0x00000, 0x08000, 0x7d5a77a7 )    /* could have one bad byte */
	ROM_CONTINUE(              0x10000, 0x08000 )                /* (compare with US version, */
														/* which is verified to be correct) */
	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "3wonders.18",  0x00000, 0x20000, 0x26b211ab )
	ROM_LOAD( "3wonders.19",  0x20000, 0x20000, 0xdbe64ad0 )
ROM_END

ROM_START( kod )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "kod30.rom",    0x00000, 0x20000, 0xc7414fd4 )
	ROM_LOAD16_BYTE( "kod37.rom",    0x00001, 0x20000, 0xa5bf40d2 )
	ROM_LOAD16_BYTE( "kod31.rom",    0x40000, 0x20000, 0x1fffc7bd )
	ROM_LOAD16_BYTE( "kod38.rom",    0x40001, 0x20000, 0x89e57a82 )
	ROM_LOAD16_BYTE( "kod28.rom",    0x80000, 0x20000, 0x9367bcd9 )
	ROM_LOAD16_BYTE( "kod35.rom",    0x80001, 0x20000, 0x4ca6a48a )
	ROM_LOAD16_BYTE( "kod29.rom",    0xc0000, 0x20000, 0x6a0ba878 )
	ROM_LOAD16_BYTE( "kod36.rom",    0xc0001, 0x20000, 0xb509b39d )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "kod02.rom",    0x000000, 0x80000, 0xe45b8701, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod04.rom",    0x000002, 0x80000, 0xa7750322, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod01.rom",    0x000004, 0x80000, 0x5f74bf78, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod03.rom",    0x000006, 0x80000, 0x5e5303bf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod11.rom",    0x200000, 0x80000, 0x113358f3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod13.rom",    0x200002, 0x80000, 0x38853c44, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod10.rom",    0x200004, 0x80000, 0x9ef36604, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod12.rom",    0x200006, 0x80000, 0x402b9b4f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kod09.rom",     0x00000, 0x08000, 0xf5514510 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "kod18.rom",    0x00000, 0x20000, 0x69ecb2c8 )
	ROM_LOAD( "kod19.rom",    0x20000, 0x20000, 0x02d851c1 )
ROM_END

ROM_START( kodu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "kdu-30b.bin",  0x00000, 0x20000, 0x825817f9 )
	ROM_LOAD16_BYTE( "kdu-37b.bin",  0x00001, 0x20000, 0xd2422dfb )
	ROM_LOAD16_BYTE( "kdu-31b.bin",  0x40000, 0x20000, 0x9af36039 )
	ROM_LOAD16_BYTE( "kdu-38b.bin",  0x40001, 0x20000, 0xbe8405a1 )
	ROM_LOAD16_BYTE( "kod28.rom",    0x80000, 0x20000, 0x9367bcd9 )
	ROM_LOAD16_BYTE( "kod35.rom",    0x80001, 0x20000, 0x4ca6a48a )
	ROM_LOAD16_BYTE( "kd-29.bin",    0xc0000, 0x20000, 0x0360fa72 )
	ROM_LOAD16_BYTE( "kd-36a.bin",   0xc0001, 0x20000, 0x95a3cef8 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "kod02.rom",    0x000000, 0x80000, 0xe45b8701, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod04.rom",    0x000002, 0x80000, 0xa7750322, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod01.rom",    0x000004, 0x80000, 0x5f74bf78, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod03.rom",    0x000006, 0x80000, 0x5e5303bf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod11.rom",    0x200000, 0x80000, 0x113358f3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod13.rom",    0x200002, 0x80000, 0x38853c44, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod10.rom",    0x200004, 0x80000, 0x9ef36604, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod12.rom",    0x200006, 0x80000, 0x402b9b4f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kd09.bin",      0x00000, 0x08000, 0xbac6ec26 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "kd18.bin",    0x00000, 0x20000, 0x4c63181d )
	ROM_LOAD( "kd19.bin",    0x20000, 0x20000, 0x92941b80 )
ROM_END

ROM_START( kodj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "kd30.bin",    0x00000, 0x20000, 0xebc788ad )
	ROM_LOAD16_BYTE( "kd37.bin",    0x00001, 0x20000, 0xe55c3529 )
	ROM_LOAD16_BYTE( "kd31.bin",    0x40000, 0x20000, 0xc710d722 )
	ROM_LOAD16_BYTE( "kd38.bin",    0x40001, 0x20000, 0x57d6ed3a )
	ROM_LOAD16_WORD_SWAP("kd33.bin", 0x80000, 0x80000, 0x9bd7ad4b)

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "kod02.rom",    0x000000, 0x80000, 0xe45b8701, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod04.rom",    0x000002, 0x80000, 0xa7750322, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod01.rom",    0x000004, 0x80000, 0x5f74bf78, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod03.rom",    0x000006, 0x80000, 0x5e5303bf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod11.rom",    0x200000, 0x80000, 0x113358f3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod13.rom",    0x200002, 0x80000, 0x38853c44, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod10.rom",    0x200004, 0x80000, 0x9ef36604, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod12.rom",    0x200006, 0x80000, 0x402b9b4f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kd09.bin",      0x00000, 0x08000, 0xbac6ec26 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "kd18.bin",    0x00000, 0x20000, 0x4c63181d )
	ROM_LOAD( "kd19.bin",    0x20000, 0x20000, 0x92941b80 )
ROM_END

ROM_START( kodb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "kod.17",    0x00000, 0x080000, 0x036dd74c )
	ROM_LOAD16_BYTE( "kod.18",    0x00001, 0x080000, 0x3e4b7295 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "kod02.rom",    0x000000, 0x80000, 0xe45b8701, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod04.rom",    0x000002, 0x80000, 0xa7750322, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod01.rom",    0x000004, 0x80000, 0x5f74bf78, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod03.rom",    0x000006, 0x80000, 0x5e5303bf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod11.rom",    0x200000, 0x80000, 0x113358f3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod13.rom",    0x200002, 0x80000, 0x38853c44, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod10.rom",    0x200004, 0x80000, 0x9ef36604, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kod12.rom",    0x200006, 0x80000, 0x402b9b4f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kod.15",        0x00000, 0x08000, 0x01cae60c )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "kd18.bin",    0x00000, 0x20000, 0x4c63181d )
	ROM_LOAD( "kd19.bin",    0x20000, 0x20000, 0x92941b80 )
ROM_END

ROM_START( captcomm )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "cce_23d.rom",  0x000000, 0x80000, 0x19c58ece )
	ROM_LOAD16_WORD_SWAP( "cc_22d.rom",   0x080000, 0x80000, 0xa91949b7 )
	ROM_LOAD16_BYTE( "cc_24d.rom",        0x100000, 0x20000, 0x680e543f )
	ROM_LOAD16_BYTE( "cc_28d.rom",        0x100001, 0x20000, 0x8820039f )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "gfx_01.rom",   0x000000, 0x80000, 0x7261d8ba, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_03.rom",   0x000002, 0x80000, 0x6a60f949, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_02.rom",   0x000004, 0x80000, 0x00637302, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_04.rom",   0x000006, 0x80000, 0xcc87cf61, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_05.rom",   0x200000, 0x80000, 0x28718bed, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_07.rom",   0x200002, 0x80000, 0xd4acc53a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_06.rom",   0x200004, 0x80000, 0x0c69f151, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_08.rom",   0x200006, 0x80000, 0x1f9ebb97, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "cc_09.rom",     0x00000, 0x08000, 0x698e8b58 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "cc_18.rom",    0x00000, 0x20000, 0x6de2c2db )
	ROM_LOAD( "cc_19.rom",    0x20000, 0x20000, 0xb99091ae )
ROM_END

ROM_START( captcomu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "23b",   0x000000, 0x80000, 0x03da44fd )
	ROM_LOAD16_WORD_SWAP( "22c",   0x080000, 0x80000, 0x9b82a052 )
	ROM_LOAD16_BYTE( "24b",        0x100000, 0x20000, 0x84ff99b2 )
	ROM_LOAD16_BYTE( "28b",        0x100001, 0x20000, 0xfbcec223 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "gfx_01.rom",   0x000000, 0x80000, 0x7261d8ba, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_03.rom",   0x000002, 0x80000, 0x6a60f949, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_02.rom",   0x000004, 0x80000, 0x00637302, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_04.rom",   0x000006, 0x80000, 0xcc87cf61, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_05.rom",   0x200000, 0x80000, 0x28718bed, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_07.rom",   0x200002, 0x80000, 0xd4acc53a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_06.rom",   0x200004, 0x80000, 0x0c69f151, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_08.rom",   0x200006, 0x80000, 0x1f9ebb97, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "cc_09.rom",     0x00000, 0x08000, 0x698e8b58 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "cc_18.rom",    0x00000, 0x20000, 0x6de2c2db )
	ROM_LOAD( "cc_19.rom",    0x20000, 0x20000, 0xb99091ae )
ROM_END

ROM_START( captcomj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "cc23.bin",   0x000000, 0x80000, 0x5b482b62 )
	ROM_LOAD16_WORD_SWAP( "cc22.bin",   0x080000, 0x80000, 0x0fd34195 )
	ROM_LOAD16_BYTE( "cc24.bin",        0x100000, 0x20000, 0x3a794f25 )
	ROM_LOAD16_BYTE( "cc28.bin",        0x100001, 0x20000, 0xfc3c2906 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "gfx_01.rom",   0x000000, 0x80000, 0x7261d8ba, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_03.rom",   0x000002, 0x80000, 0x6a60f949, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_02.rom",   0x000004, 0x80000, 0x00637302, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_04.rom",   0x000006, 0x80000, 0xcc87cf61, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_05.rom",   0x200000, 0x80000, 0x28718bed, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_07.rom",   0x200002, 0x80000, 0xd4acc53a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_06.rom",   0x200004, 0x80000, 0x0c69f151, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "gfx_08.rom",   0x200006, 0x80000, 0x1f9ebb97, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "cc_09.rom",     0x00000, 0x08000, 0x698e8b58 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "cc_18.rom",    0x00000, 0x20000, 0x6de2c2db )
	ROM_LOAD( "cc_19.rom",    0x20000, 0x20000, 0xb99091ae )
ROM_END

ROM_START( knights )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "kr_23e.rom",   0x00000, 0x80000, 0x1b3997eb )
	ROM_LOAD16_WORD_SWAP( "kr_22.rom",    0x80000, 0x80000, 0xd0b671a9 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "kr_gfx1.rom",  0x000000, 0x80000, 0x9e36c1a4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx3.rom",  0x000002, 0x80000, 0xc5832cae, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx2.rom",  0x000004, 0x80000, 0xf095be2d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx4.rom",  0x000006, 0x80000, 0x179dfd96, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx5.rom",  0x200000, 0x80000, 0x1f4298d2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx7.rom",  0x200002, 0x80000, 0x37fa8751, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx6.rom",  0x200004, 0x80000, 0x0200bc3d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx8.rom",  0x200006, 0x80000, 0x0bb2b4e7, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kr_09.rom",     0x00000, 0x08000, 0x5e44d9ee )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "kr_18.rom",    0x00000, 0x20000, 0xda69d15f )
	ROM_LOAD( "kr_19.rom",    0x20000, 0x20000, 0xbfc654e9 )
ROM_END

ROM_START( knightsu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "kru23.rom",    0x00000, 0x80000, 0x252bc2ba )
	ROM_LOAD16_WORD_SWAP( "kr_22.rom",    0x80000, 0x80000, 0xd0b671a9 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "kr_gfx1.rom",  0x000000, 0x80000, 0x9e36c1a4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx3.rom",  0x000002, 0x80000, 0xc5832cae, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx2.rom",  0x000004, 0x80000, 0xf095be2d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx4.rom",  0x000006, 0x80000, 0x179dfd96, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx5.rom",  0x200000, 0x80000, 0x1f4298d2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx7.rom",  0x200002, 0x80000, 0x37fa8751, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx6.rom",  0x200004, 0x80000, 0x0200bc3d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx8.rom",  0x200006, 0x80000, 0x0bb2b4e7, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kr_09.rom",     0x00000, 0x08000, 0x5e44d9ee )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "kr_18.rom",    0x00000, 0x20000, 0xda69d15f )
	ROM_LOAD( "kr_19.rom",    0x20000, 0x20000, 0xbfc654e9 )
ROM_END

ROM_START( knightsj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "krj30.bin",   0x00000, 0x20000, 0xad3d1a8e )
	ROM_LOAD16_BYTE( "krj37.bin",   0x00001, 0x20000, 0xe694a491 )
	ROM_LOAD16_BYTE( "krj31.bin",   0x40000, 0x20000, 0x85596094 )
	ROM_LOAD16_BYTE( "krj38.bin",   0x40001, 0x20000, 0x9198bf8f )
	ROM_LOAD16_WORD_SWAP( "kr_22.rom",    0x80000, 0x80000, 0xd0b671a9 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "kr_gfx1.rom",  0x000000, 0x80000, 0x9e36c1a4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx3.rom",  0x000002, 0x80000, 0xc5832cae, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx2.rom",  0x000004, 0x80000, 0xf095be2d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx4.rom",  0x000006, 0x80000, 0x179dfd96, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx5.rom",  0x200000, 0x80000, 0x1f4298d2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx7.rom",  0x200002, 0x80000, 0x37fa8751, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx6.rom",  0x200004, 0x80000, 0x0200bc3d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "kr_gfx8.rom",  0x200006, 0x80000, 0x0bb2b4e7, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kr_09.rom",     0x00000, 0x08000, 0x5e44d9ee )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "kr_18.rom",    0x00000, 0x20000, 0xda69d15f )
	ROM_LOAD( "kr_19.rom",    0x20000, 0x20000, 0xbfc654e9 )
ROM_END

ROM_START( sf2ce )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sf2ce.23",     0x000000, 0x80000, 0x3f846b74 )
	ROM_LOAD16_WORD_SWAP( "sf2ce.22",     0x080000, 0x80000, 0x99f1cca4 )
	ROM_LOAD16_WORD_SWAP( "s92_21a.bin",  0x100000, 0x80000, 0x925a7877 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2ceua )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "s92u-23a",     0x000000, 0x80000, 0xac44415b )
	ROM_LOAD16_WORD_SWAP( "sf2ce.22",     0x080000, 0x80000, 0x99f1cca4 )
	ROM_LOAD16_WORD_SWAP( "s92_21a.bin",  0x100000, 0x80000, 0x925a7877 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2ceub )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "s92-23b",      0x000000, 0x80000, 0x996a3015 )
	ROM_LOAD16_WORD_SWAP( "s92-22b",      0x080000, 0x80000, 0x2bbe15ed )
	ROM_LOAD16_WORD_SWAP( "s92-21b",      0x100000, 0x80000, 0xb383cb1c )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2cej )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "s92j_23b.bin", 0x000000, 0x80000, 0x140876c5 )
	ROM_LOAD16_WORD_SWAP( "s92j_22b.bin", 0x080000, 0x80000, 0x2fbb3bfe )
	ROM_LOAD16_WORD_SWAP( "s92_21a.bin",  0x100000, 0x80000, 0x925a7877 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2rb )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD( "sf2d__23.rom", 0x000000, 0x80000, 0x450532b0 )
	ROM_LOAD16_WORD( "sf2d__22.rom", 0x080000, 0x80000, 0xfe9d9cf5 )
	ROM_LOAD16_WORD_SWAP( "s92_21a.bin",  0x100000, 0x80000, 0x925a7877 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2rb2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "27.bin",    0x000000, 0x20000, 0x40296ecd )
	ROM_LOAD16_BYTE( "31.bin",    0x000001, 0x20000, 0x87954a41 )
	ROM_LOAD16_BYTE( "26.bin",    0x040000, 0x20000, 0xa6974195 )
	ROM_LOAD16_BYTE( "30.bin",    0x040001, 0x20000, 0x8141fe32 )
	ROM_LOAD16_BYTE( "25.bin",    0x080000, 0x20000, 0x9ef8f772 )
	ROM_LOAD16_BYTE( "29.bin",    0x080001, 0x20000, 0x7d9c479c )
	ROM_LOAD16_BYTE( "24.bin",    0x0c0000, 0x20000, 0x93579684 )
	ROM_LOAD16_BYTE( "28.bin",    0x0c0001, 0x20000, 0xff728865 )
	ROM_LOAD16_WORD_SWAP( "s92_21a.bin",     0x100000, 0x80000, 0x925a7877 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2red )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sf2red.23",    0x000000, 0x80000, 0x40276abb )
	ROM_LOAD16_WORD_SWAP( "sf2red.22",    0x080000, 0x80000, 0x18daf387 )
	ROM_LOAD16_WORD_SWAP( "sf2red.21",    0x100000, 0x80000, 0x52c486bb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2v004 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sf2v004.23",   0x000000, 0x80000, 0x52d19f2c )
	ROM_LOAD16_WORD_SWAP( "sf2v004.22",   0x080000, 0x80000, 0x4b26fde7 )
	ROM_LOAD16_WORD_SWAP( "sf2red.21",    0x100000, 0x80000, 0x52c486bb )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2accp2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sf2ca-23.bin", 0x000000, 0x80000, 0x36c3ba2f )
	ROM_LOAD16_WORD_SWAP( "sf2ca-22.bin", 0x080000, 0x80000, 0x0550453d )
	ROM_LOAD16_WORD_SWAP( "sf2ca-21.bin", 0x100000, 0x40000, 0x4c1c43ba )
	/* ROM space ends at 13ffff, but the code checks 180ca6 and */
	/* crashes if it isn't 0 - protection? */

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_10.bin",   0x400000, 0x80000, 0x960687d5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_11.bin",   0x400002, 0x80000, 0x978ecd18, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.11 */
	ROMX_LOAD( "s92_12.bin",   0x400004, 0x80000, 0xd6ec9a0a, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.12 */
	ROMX_LOAD( "s92_13.bin",   0x400006, 0x80000, 0xed2c67f6, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( varth )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "vae_30a.rom",  0x00000, 0x20000, 0x7fcd0091 )
	ROM_LOAD16_BYTE( "vae_35a.rom",  0x00001, 0x20000, 0x35cf9509 )
	ROM_LOAD16_BYTE( "vae_31a.rom",  0x40000, 0x20000, 0x15e5ee81 )
	ROM_LOAD16_BYTE( "vae_36a.rom",  0x40001, 0x20000, 0x153a201e )
	ROM_LOAD16_BYTE( "vae_28a.rom",  0x80000, 0x20000, 0x7a0e0d25 )
	ROM_LOAD16_BYTE( "vae_33a.rom",  0x80001, 0x20000, 0xf2365922 )
	ROM_LOAD16_BYTE( "vae_29a.rom",  0xc0000, 0x20000, 0x5e2cd2c3 )
	ROM_LOAD16_BYTE( "vae_34a.rom",  0xc0001, 0x20000, 0x3d9bdf83 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "va_gfx5.rom",  0x000000, 0x80000, 0xb1fb726e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx7.rom",  0x000002, 0x80000, 0x4c6588cd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx1.rom",  0x000004, 0x80000, 0x0b1ace37, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx3.rom",  0x000006, 0x80000, 0x44dfe706, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "va_09.rom",     0x00000, 0x08000, 0x7a99446e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "va_18.rom",    0x00000, 0x20000, 0xde30510e )
	ROM_LOAD( "va_19.rom",    0x20000, 0x20000, 0x0610a4ac )
ROM_END

ROM_START( varthu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vau23a.bin",  0x00000, 0x80000, 0xfbe68726 )
	ROM_LOAD16_BYTE( "vae_28a.rom",  0x80000, 0x20000, 0x7a0e0d25 )
	ROM_LOAD16_BYTE( "vae_33a.rom",  0x80001, 0x20000, 0xf2365922 )
	ROM_LOAD16_BYTE( "vae_29a.rom",  0xc0000, 0x20000, 0x5e2cd2c3 )
	ROM_LOAD16_BYTE( "vae_34a.rom",  0xc0001, 0x20000, 0x3d9bdf83 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "va_gfx5.rom",  0x000000, 0x80000, 0xb1fb726e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx7.rom",  0x000002, 0x80000, 0x4c6588cd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx1.rom",  0x000004, 0x80000, 0x0b1ace37, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx3.rom",  0x000006, 0x80000, 0x44dfe706, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "va_09.rom",     0x00000, 0x08000, 0x7a99446e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "va_18.rom",    0x00000, 0x20000, 0xde30510e )
	ROM_LOAD( "va_19.rom",    0x20000, 0x20000, 0x0610a4ac )
ROM_END

ROM_START( varthj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "vaj36b.bin",   0x00000, 0x20000, 0x1d798d6a )
	ROM_LOAD16_BYTE( "vaj42b.bin",   0x00001, 0x20000, 0x0f720233 )
	ROM_LOAD16_BYTE( "vaj37b.bin",   0x40000, 0x20000, 0x24414b17 )
	ROM_LOAD16_BYTE( "vaj43b.bin",   0x40001, 0x20000, 0x34b4b06c )
	ROM_LOAD16_BYTE( "vaj34b.bin",   0x80000, 0x20000, 0x87c79aed )
	ROM_LOAD16_BYTE( "vaj40b.bin",   0x80001, 0x20000, 0x210b4bd0 )
	ROM_LOAD16_BYTE( "vaj35b.bin",   0xc0000, 0x20000, 0x6b0da69f )
	ROM_LOAD16_BYTE( "vaj41b.bin",   0xc0001, 0x20000, 0x6542c8a4 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "va_gfx5.rom",  0x000000, 0x80000, 0xb1fb726e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx7.rom",  0x000002, 0x80000, 0x4c6588cd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx1.rom",  0x000004, 0x80000, 0x0b1ace37, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "va_gfx3.rom",  0x000006, 0x80000, 0x44dfe706, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "va_09.rom",     0x00000, 0x08000, 0x7a99446e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "va_18.rom",    0x00000, 0x20000, 0xde30510e )
	ROM_LOAD( "va_19.rom",    0x20000, 0x20000, 0x0610a4ac )
ROM_END

ROM_START( cworld2j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "q536.bin",       0x00000, 0x20000, 0x38a08099 )
	ROM_LOAD16_BYTE( "q542.bin",       0x00001, 0x20000, 0x4d29b3a4 )
	ROM_LOAD16_BYTE( "q537.bin",       0x40000, 0x20000, 0xeb547ebc )
	ROM_LOAD16_BYTE( "q543.bin",       0x40001, 0x20000, 0x3ef65ea8 )
	ROM_LOAD16_BYTE( "q534.bin",       0x80000, 0x20000, 0x7fcc1317 )
	ROM_LOAD16_BYTE( "q540.bin",       0x80001, 0x20000, 0x7f14b7b4 )
	ROM_LOAD16_BYTE( "q535.bin",       0xc0000, 0x20000, 0xabacee26 )
	ROM_LOAD16_BYTE( "q541.bin",       0xc0001, 0x20000, 0xd3654067 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "q509.bin",   0x000000, 0x20000, 0x48496d80, ROM_SKIP(7) )
	ROMX_LOAD( "q501.bin",   0x000001, 0x20000, 0xc5453f56, ROM_SKIP(7) )
	ROMX_LOAD( "q513.bin",   0x000002, 0x20000, 0xc741ac52, ROM_SKIP(7) )
	ROMX_LOAD( "q505.bin",   0x000003, 0x20000, 0x143e068f, ROM_SKIP(7) )
	ROMX_LOAD( "q524.bin",   0x000004, 0x20000, 0xb419d139, ROM_SKIP(7) )
	ROMX_LOAD( "q517.bin",   0x000005, 0x20000, 0xbd3b4d11, ROM_SKIP(7) )
	ROMX_LOAD( "q538.bin",   0x000006, 0x20000, 0x9c24670c, ROM_SKIP(7) )
	ROMX_LOAD( "q532.bin",   0x000007, 0x20000, 0x3ef9c7c2, ROM_SKIP(7) )
	ROMX_LOAD( "q510.bin",   0x100000, 0x20000, 0x119e5e93, ROM_SKIP(7) )
	ROMX_LOAD( "q502.bin",   0x100001, 0x20000, 0xa2cadcbe, ROM_SKIP(7) )
	ROMX_LOAD( "q514.bin",   0x100002, 0x20000, 0xa8755f82, ROM_SKIP(7) )
	ROMX_LOAD( "q506.bin",   0x100003, 0x20000, 0xc92a91fc, ROM_SKIP(7) )
	ROMX_LOAD( "q525.bin",   0x100004, 0x20000, 0x979237cb, ROM_SKIP(7) )
	ROMX_LOAD( "q518.bin",   0x100005, 0x20000, 0xc57da03c, ROM_SKIP(7) )
	ROMX_LOAD( "q539.bin",   0x100006, 0x20000, 0xa5839b25, ROM_SKIP(7) )
	ROMX_LOAD( "q533.bin",   0x100007, 0x20000, 0x04d03930, ROM_SKIP(7) )


	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "q523.bin",      0x00000, 0x08000, 0xe14dc524 )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "q530.bin",       0x00000, 0x20000, 0xd10c1b68 )
	ROM_LOAD( "q531.bin",       0x20000, 0x20000, 0x7d17e496 )
ROM_END

ROM_START( wof )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "tk2e_23b.rom",  0x000000, 0x80000, 0x11fb2ed1 )
	ROM_LOAD16_WORD_SWAP( "tk2e_22b.rom",  0x080000, 0x80000, 0x479b3f24 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "tk2_gfx1.rom",   0x000000, 0x80000, 0x0d9cb9bf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx3.rom",   0x000002, 0x80000, 0x45227027, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx2.rom",   0x000004, 0x80000, 0xc5ca2460, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx4.rom",   0x000006, 0x80000, 0xe349551c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx5.rom",   0x200000, 0x80000, 0x291f0f0b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx7.rom",   0x200002, 0x80000, 0x3edeb949, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx6.rom",   0x200004, 0x80000, 0x1abd14d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx8.rom",   0x200006, 0x80000, 0xb27948e3, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "tk2_qa.rom",     0x00000, 0x08000, 0xc9183a0d )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "tk2_q1.rom",     0x000000, 0x80000, 0x611268cf )
	ROM_LOAD( "tk2_q2.rom",     0x080000, 0x80000, 0x20f55ca9 )
	ROM_LOAD( "tk2_q3.rom",     0x100000, 0x80000, 0xbfcf6f52 )
	ROM_LOAD( "tk2_q4.rom",     0x180000, 0x80000, 0x36642e88 )
ROM_END

ROM_START( wofa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "tk2a_23b.rom",  0x000000, 0x80000, 0x2e024628 )
	ROM_LOAD16_WORD_SWAP( "tk2a_22b.rom",  0x080000, 0x80000, 0x900ad4cd )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "tk2_gfx1.rom",   0x000000, 0x80000, 0x0d9cb9bf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx3.rom",   0x000002, 0x80000, 0x45227027, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx2.rom",   0x000004, 0x80000, 0xc5ca2460, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx4.rom",   0x000006, 0x80000, 0xe349551c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx5.rom",   0x200000, 0x80000, 0x291f0f0b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx7.rom",   0x200002, 0x80000, 0x3edeb949, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx6.rom",   0x200004, 0x80000, 0x1abd14d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx8.rom",   0x200006, 0x80000, 0xb27948e3, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "tk2_qa.rom",     0x00000, 0x08000, 0xc9183a0d )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "tk2_q1.rom",     0x000000, 0x80000, 0x611268cf )
	ROM_LOAD( "tk2_q2.rom",     0x080000, 0x80000, 0x20f55ca9 )
	ROM_LOAD( "tk2_q3.rom",     0x100000, 0x80000, 0xbfcf6f52 )
	ROM_LOAD( "tk2_q4.rom",     0x180000, 0x80000, 0x36642e88 )
ROM_END

ROM_START( wofu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "tk2u.23c",  0x000000, 0x80000, 0x29b89c12 )
	ROM_LOAD16_WORD_SWAP( "tk2u.22c",  0x080000, 0x80000, 0xf5af4774 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "tk2_gfx1.rom",   0x000000, 0x80000, 0x0d9cb9bf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx3.rom",   0x000002, 0x80000, 0x45227027, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx2.rom",   0x000004, 0x80000, 0xc5ca2460, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx4.rom",   0x000006, 0x80000, 0xe349551c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx5.rom",   0x200000, 0x80000, 0x291f0f0b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx7.rom",   0x200002, 0x80000, 0x3edeb949, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx6.rom",   0x200004, 0x80000, 0x1abd14d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx8.rom",   0x200006, 0x80000, 0xb27948e3, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "tk2_qa.rom",     0x00000, 0x08000, 0xc9183a0d )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "tk2_q1.rom",     0x000000, 0x80000, 0x611268cf )
	ROM_LOAD( "tk2_q2.rom",     0x080000, 0x80000, 0x20f55ca9 )
	ROM_LOAD( "tk2_q3.rom",     0x100000, 0x80000, 0xbfcf6f52 )
	ROM_LOAD( "tk2_q4.rom",     0x180000, 0x80000, 0x36642e88 )
ROM_END

ROM_START( wofj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "tk2j23c.bin",  0x000000, 0x80000, 0x9b215a68 )
	ROM_LOAD16_WORD_SWAP( "tk2j22c.bin",  0x080000, 0x80000, 0xb74b09ac )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "tk2_gfx1.rom",   0x000000, 0x80000, 0x0d9cb9bf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx3.rom",   0x000002, 0x80000, 0x45227027, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx2.rom",   0x000004, 0x80000, 0xc5ca2460, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx4.rom",   0x000006, 0x80000, 0xe349551c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk205.bin",      0x200000, 0x80000, 0xe4a44d53, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk206.bin",      0x200002, 0x80000, 0x58066ba8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk207.bin",      0x200004, 0x80000, 0xd706568e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk208.bin",      0x200006, 0x80000, 0xd4a19a02, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "tk2_qa.rom",     0x00000, 0x08000, 0xc9183a0d )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "tk2_q1.rom",     0x000000, 0x80000, 0x611268cf )
	ROM_LOAD( "tk2_q2.rom",     0x080000, 0x80000, 0x20f55ca9 )
	ROM_LOAD( "tk2_q3.rom",     0x100000, 0x80000, 0xbfcf6f52 )
	ROM_LOAD( "tk2_q4.rom",     0x180000, 0x80000, 0x36642e88 )
ROM_END

ROM_START( sf2t )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sf2.23",        0x000000, 0x80000, 0x89a1fc38 )
	ROM_LOAD16_WORD_SWAP( "sf2_22.bin",    0x080000, 0x80000, 0xaea6e035 )
	ROM_LOAD16_WORD_SWAP( "sf2_21.bin",    0x100000, 0x80000, 0xfd200288 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s2t_10.bin",   0x400000, 0x80000, 0x3c042686, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s2t_11.bin",   0x400002, 0x80000, 0x8b7e7183, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2t.12 */
	ROMX_LOAD( "s2t_12.bin",   0x400004, 0x80000, 0x293c888c, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2t.11 */
	ROMX_LOAD( "s2t_13.bin",   0x400006, 0x80000, 0x842b35a4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( sf2tj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "s2tj_23.bin",   0x000000, 0x80000, 0xea73b4dc )
	ROM_LOAD16_WORD_SWAP( "s2t_22.bin",    0x080000, 0x80000, 0xaea6e035 )
	ROM_LOAD16_WORD_SWAP( "s2t_21.bin",    0x100000, 0x80000, 0xfd200288 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "s92_01.bin",   0x000000, 0x80000, 0x03b0d852, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_02.bin",   0x000002, 0x80000, 0x840289ec, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.03 */
	ROMX_LOAD( "s92_03.bin",   0x000004, 0x80000, 0xcdb5f027, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.02 */
	ROMX_LOAD( "s92_04.bin",   0x000006, 0x80000, 0xe2799472, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_05.bin",   0x200000, 0x80000, 0xba8a2761, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s92_06.bin",   0x200002, 0x80000, 0xe584bfb5, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.07 */
	ROMX_LOAD( "s92_07.bin",   0x200004, 0x80000, 0x21e3f87d, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2.06 */
	ROMX_LOAD( "s92_08.bin",   0x200006, 0x80000, 0xbefc47df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s2t_10.bin",   0x400000, 0x80000, 0x3c042686, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "s2t_11.bin",   0x400002, 0x80000, 0x8b7e7183, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2t.12 */
	ROMX_LOAD( "s2t_12.bin",   0x400004, 0x80000, 0x293c888c, ROM_GROUPWORD | ROM_SKIP(6) ) /* sf2t.11 */
	ROMX_LOAD( "s2t_13.bin",   0x400006, 0x80000, 0x842b35a4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "s92_09.bin",    0x00000, 0x08000, 0x08f6b60e )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s92_18.bin",    0x00000, 0x20000, 0x7f162009 )
	ROM_LOAD( "s92_19.bin",    0x20000, 0x20000, 0xbeade53f )
ROM_END

ROM_START( dino )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "cde_23a.rom",  0x000000, 0x80000, 0x8f4e585e )
	ROM_LOAD16_WORD_SWAP( "cde_22a.rom",  0x080000, 0x80000, 0x9278aa12 )
	ROM_LOAD16_WORD_SWAP( "cde_21a.rom",  0x100000, 0x80000, 0x66d23de2 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "cd_gfx01.rom",   0x000000, 0x80000, 0x8da4f917, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx03.rom",   0x000002, 0x80000, 0x6c40f603, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx02.rom",   0x000004, 0x80000, 0x09c8fc2d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx04.rom",   0x000006, 0x80000, 0x637ff38f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx05.rom",   0x200000, 0x80000, 0x470befee, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx07.rom",   0x200002, 0x80000, 0x22bfb7a3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx06.rom",   0x200004, 0x80000, 0xe7599ac4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx08.rom",   0x200006, 0x80000, 0x211b4b15, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "cd_q.rom",       0x00000, 0x08000, 0x605fdb0b )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "cd_q1.rom",      0x000000, 0x80000, 0x60927775 )
	ROM_LOAD( "cd_q2.rom",      0x080000, 0x80000, 0x770f4c47 )
	ROM_LOAD( "cd_q3.rom",      0x100000, 0x80000, 0x2f273ffc )
	ROM_LOAD( "cd_q4.rom",      0x180000, 0x80000, 0x2c67821d )
ROM_END

ROM_START( dinoj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "cdj-23a.8f",   0x000000, 0x80000, 0x5f3ece96 )
	ROM_LOAD16_WORD_SWAP( "cdj-22a.7f",   0x080000, 0x80000, 0xa0d8de29 )
	ROM_LOAD16_WORD_SWAP( "cde_21a.rom",  0x100000, 0x80000, 0x66d23de2 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "cd_gfx01.rom",   0x000000, 0x80000, 0x8da4f917, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx03.rom",   0x000002, 0x80000, 0x6c40f603, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx02.rom",   0x000004, 0x80000, 0x09c8fc2d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx04.rom",   0x000006, 0x80000, 0x637ff38f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx05.rom",   0x200000, 0x80000, 0x470befee, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx07.rom",   0x200002, 0x80000, 0x22bfb7a3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx06.rom",   0x200004, 0x80000, 0xe7599ac4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cd_gfx08.rom",   0x200006, 0x80000, 0x211b4b15, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "cd_q.rom",       0x00000, 0x08000, 0x605fdb0b )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "cd_q1.rom",      0x000000, 0x80000, 0x60927775 )
	ROM_LOAD( "cd_q2.rom",      0x080000, 0x80000, 0x770f4c47 )
	ROM_LOAD( "cd_q3.rom",      0x100000, 0x80000, 0x2f273ffc )
	ROM_LOAD( "cd_q4.rom",      0x180000, 0x80000, 0x2c67821d )
ROM_END

ROM_START( punisher )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "pse_26.rom",       0x000000, 0x20000, 0x389a99d2 )
	ROM_LOAD16_BYTE( "pse_30.rom",       0x000001, 0x20000, 0x68fb06ac )
	ROM_LOAD16_BYTE( "pse_27.rom",       0x040000, 0x20000, 0x3eb181c3 )
	ROM_LOAD16_BYTE( "pse_31.rom",       0x040001, 0x20000, 0x37108e7b )
	ROM_LOAD16_BYTE( "pse_24.rom",       0x080000, 0x20000, 0x0f434414 )
	ROM_LOAD16_BYTE( "pse_28.rom",       0x080001, 0x20000, 0xb732345d )
	ROM_LOAD16_BYTE( "pse_25.rom",       0x0c0000, 0x20000, 0xb77102e2 )
	ROM_LOAD16_BYTE( "pse_29.rom",       0x0c0001, 0x20000, 0xec037bce )
	ROM_LOAD16_WORD_SWAP( "ps_21.rom",   0x100000, 0x80000, 0x8affa5a9 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "ps_gfx1.rom",   0x000000, 0x80000, 0x77b7ccab, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx3.rom",   0x000002, 0x80000, 0x0122720b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx2.rom",   0x000004, 0x80000, 0x64fa58d4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx4.rom",   0x000006, 0x80000, 0x60da42c8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx5.rom",   0x200000, 0x80000, 0xc54ea839, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx7.rom",   0x200002, 0x80000, 0x04c5acbd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx6.rom",   0x200004, 0x80000, 0xa544f4cc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx8.rom",   0x200006, 0x80000, 0x8f02f436, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "ps_q.rom",       0x00000, 0x08000, 0x49ff4446 )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ps_q1.rom",      0x000000, 0x80000, 0x31fd8726 )
	ROM_LOAD( "ps_q2.rom",      0x080000, 0x80000, 0x980a9eef )
	ROM_LOAD( "ps_q3.rom",      0x100000, 0x80000, 0x0dd44491 )
	ROM_LOAD( "ps_q4.rom",      0x180000, 0x80000, 0xbed42f03 )
ROM_END

ROM_START( punishru )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE ( "psu26.rom",       0x000000, 0x20000, 0x9236d121 )
	ROM_LOAD16_BYTE ( "psu30.rom",       0x000001, 0x20000, 0x8320e501 )
	ROM_LOAD16_BYTE ( "psu27.rom",       0x040000, 0x20000, 0x61c960a1 )
	ROM_LOAD16_BYTE ( "psu31.rom",       0x040001, 0x20000, 0x78d4c298 )
	ROM_LOAD16_BYTE ( "psu24.rom",       0x080000, 0x20000, 0x1cfecad7 )
	ROM_LOAD16_BYTE ( "psu28.rom",       0x080001, 0x20000, 0xbdf921c1 )
	ROM_LOAD16_BYTE ( "psu25.rom",       0x0c0000, 0x20000, 0xc51acc94 )
	ROM_LOAD16_BYTE ( "psu29.rom",       0x0c0001, 0x20000, 0x52dce1ca )
	ROM_LOAD16_WORD_SWAP( "ps_21.rom",   0x100000, 0x80000, 0x8affa5a9 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "ps_gfx1.rom",   0x000000, 0x80000, 0x77b7ccab, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx3.rom",   0x000002, 0x80000, 0x0122720b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx2.rom",   0x000004, 0x80000, 0x64fa58d4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx4.rom",   0x000006, 0x80000, 0x60da42c8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx5.rom",   0x200000, 0x80000, 0xc54ea839, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx7.rom",   0x200002, 0x80000, 0x04c5acbd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx6.rom",   0x200004, 0x80000, 0xa544f4cc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx8.rom",   0x200006, 0x80000, 0x8f02f436, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "ps_q.rom",       0x00000, 0x08000, 0x49ff4446 )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ps_q1.rom",      0x000000, 0x80000, 0x31fd8726 )
	ROM_LOAD( "ps_q2.rom",      0x080000, 0x80000, 0x980a9eef )
	ROM_LOAD( "ps_q3.rom",      0x100000, 0x80000, 0x0dd44491 )
	ROM_LOAD( "ps_q4.rom",      0x180000, 0x80000, 0xbed42f03 )
ROM_END

ROM_START( punishrj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "psj23.bin",   0x000000, 0x80000, 0x6b2fda52 )
	ROM_LOAD16_WORD_SWAP( "psj22.bin",   0x080000, 0x80000, 0xe01036bc )
	ROM_LOAD16_WORD_SWAP( "ps_21.rom",   0x100000, 0x80000, 0x8affa5a9 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "ps_gfx1.rom",   0x000000, 0x80000, 0x77b7ccab, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx3.rom",   0x000002, 0x80000, 0x0122720b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx2.rom",   0x000004, 0x80000, 0x64fa58d4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx4.rom",   0x000006, 0x80000, 0x60da42c8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx5.rom",   0x200000, 0x80000, 0xc54ea839, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx7.rom",   0x200002, 0x80000, 0x04c5acbd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx6.rom",   0x200004, 0x80000, 0xa544f4cc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ps_gfx8.rom",   0x200006, 0x80000, 0x8f02f436, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "ps_q.rom",       0x00000, 0x08000, 0x49ff4446 )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ps_q1.rom",      0x000000, 0x80000, 0x31fd8726 )
	ROM_LOAD( "ps_q2.rom",      0x080000, 0x80000, 0x980a9eef )
	ROM_LOAD( "ps_q3.rom",      0x100000, 0x80000, 0x0dd44491 )
	ROM_LOAD( "ps_q4.rom",      0x180000, 0x80000, 0xbed42f03 )
ROM_END

ROM_START( slammast )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mbe_23e.rom",  0x000000, 0x80000, 0x5394057a )
	ROM_LOAD16_BYTE( "mbe_24b.rom",       0x080000, 0x20000, 0x95d5e729 )
	ROM_LOAD16_BYTE( "mbe_28b.rom",       0x080001, 0x20000, 0xb1c7cbcb )
	ROM_LOAD16_BYTE( "mbe_25b.rom",       0x0c0000, 0x20000, 0xa50d3fd4 )
	ROM_LOAD16_BYTE( "mbe_29b.rom",       0x0c0001, 0x20000, 0x08e32e56 )
	ROM_LOAD16_WORD_SWAP( "mbe_21a.rom",  0x100000, 0x80000, 0xd5007b05 )
	ROM_LOAD16_WORD_SWAP( "mbe_20a.rom",  0x180000, 0x80000, 0xaeb557b0 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "mb_gfx01.rom",   0x000000, 0x80000, 0x41468e06, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx03.rom",   0x000002, 0x80000, 0xf453aa9e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx02.rom",   0x000004, 0x80000, 0x2ffbfea8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx04.rom",   0x000006, 0x80000, 0x1eb9841d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_05.bin",      0x200000, 0x80000, 0x506b9dc9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_07.bin",      0x200002, 0x80000, 0xaff8c2fb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_06.bin",      0x200004, 0x80000, 0xb76c70e9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_08.bin",      0x200006, 0x80000, 0xe60c9556, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_10.bin",      0x400000, 0x80000, 0x97976ff5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_12.bin",      0x400002, 0x80000, 0xb350a840, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_11.bin",      0x400004, 0x80000, 0x8fb94743, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_13.bin",      0x400006, 0x80000, 0xda810d5f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "mb_qa.rom",      0x00000, 0x08000, 0xe21a03c4 )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x8000, REGION_USER1, 0 )
	/* the encrypted Z80 ROM will be copied here, where the main CPU can read it. */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "mb_q1.bin",      0x000000, 0x80000, 0x0630c3ce )
	ROM_LOAD( "mb_q2.bin",      0x080000, 0x80000, 0x354f9c21 )
	ROM_LOAD( "mb_q3.bin",      0x100000, 0x80000, 0x7838487c )
	ROM_LOAD( "mb_q4.bin",      0x180000, 0x80000, 0xab66e087 )
	ROM_LOAD( "mb_q5.bin",      0x200000, 0x80000, 0xc789fef2 )
	ROM_LOAD( "mb_q6.bin",      0x280000, 0x80000, 0xecb81b61 )
	ROM_LOAD( "mb_q7.bin",      0x300000, 0x80000, 0x041e49ba )
	ROM_LOAD( "mb_q8.bin",      0x380000, 0x80000, 0x59fe702a )
ROM_END

ROM_START( slammasu )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mbu-23e.rom",  0x000000, 0x80000, 0x224f0062 )
	ROM_LOAD16_BYTE( "mbe_24b.rom",       0x080000, 0x20000, 0x95d5e729 )
	ROM_LOAD16_BYTE( "mbe_28b.rom",       0x080001, 0x20000, 0xb1c7cbcb )
	ROM_LOAD16_BYTE( "mbe_25b.rom",       0x0c0000, 0x20000, 0xa50d3fd4 )
	ROM_LOAD16_BYTE( "mbe_29b.rom",       0x0c0001, 0x20000, 0x08e32e56 )
	ROM_LOAD16_WORD_SWAP( "mbe_21a.rom",  0x100000, 0x80000, 0xd5007b05 )
	ROM_LOAD16_WORD_SWAP( "mbu-20a.rom",  0x180000, 0x80000, 0xfc848af5 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "mb_gfx01.rom",   0x000000, 0x80000, 0x41468e06, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx03.rom",   0x000002, 0x80000, 0xf453aa9e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx02.rom",   0x000004, 0x80000, 0x2ffbfea8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx04.rom",   0x000006, 0x80000, 0x1eb9841d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_05.bin",      0x200000, 0x80000, 0x506b9dc9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_07.bin",      0x200002, 0x80000, 0xaff8c2fb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_06.bin",      0x200004, 0x80000, 0xb76c70e9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_08.bin",      0x200006, 0x80000, 0xe60c9556, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_10.bin",      0x400000, 0x80000, 0x97976ff5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_12.bin",      0x400002, 0x80000, 0xb350a840, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_11.bin",      0x400004, 0x80000, 0x8fb94743, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_13.bin",      0x400006, 0x80000, 0xda810d5f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "mb_qa.rom",      0x00000, 0x08000, 0xe21a03c4 )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x8000, REGION_USER1, 0 )
	/* the encrypted Z80 ROM will be copied here, where the main CPU can read it. */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "mb_q1.bin",      0x000000, 0x80000, 0x0630c3ce )
	ROM_LOAD( "mb_q2.bin",      0x080000, 0x80000, 0x354f9c21 )
	ROM_LOAD( "mb_q3.bin",      0x100000, 0x80000, 0x7838487c )
	ROM_LOAD( "mb_q4.bin",      0x180000, 0x80000, 0xab66e087 )
	ROM_LOAD( "mb_q5.bin",      0x200000, 0x80000, 0xc789fef2 )
	ROM_LOAD( "mb_q6.bin",      0x280000, 0x80000, 0xecb81b61 )
	ROM_LOAD( "mb_q7.bin",      0x300000, 0x80000, 0x041e49ba )
	ROM_LOAD( "mb_q8.bin",      0x380000, 0x80000, 0x59fe702a )
ROM_END

ROM_START( mbomberj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mbj23e",       0x000000, 0x80000, 0x0d06036a )
	ROM_LOAD16_BYTE( "mbe_24b.rom",       0x080000, 0x20000, 0x95d5e729 )
	ROM_LOAD16_BYTE( "mbe_28b.rom",       0x080001, 0x20000, 0xb1c7cbcb )
	ROM_LOAD16_BYTE( "mbe_25b.rom",       0x0c0000, 0x20000, 0xa50d3fd4 )
	ROM_LOAD16_BYTE( "mbe_29b.rom",       0x0c0001, 0x20000, 0x08e32e56 )
	ROM_LOAD16_WORD_SWAP( "mbe_21a.rom",  0x100000, 0x80000, 0xd5007b05 )
	ROM_LOAD16_WORD_SWAP( "mbe_20a.rom",  0x180000, 0x80000, 0xaeb557b0 )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "mbj_01.bin",     0x000000, 0x80000, 0xa53b1c81, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mbj_03.bin",     0x000002, 0x80000, 0x23fe10f6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mbj_02.bin",     0x000004, 0x80000, 0xcb866c2f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mbj_04.bin",     0x000006, 0x80000, 0xc9143e75, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_05.bin",      0x200000, 0x80000, 0x506b9dc9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_07.bin",      0x200002, 0x80000, 0xaff8c2fb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_06.bin",      0x200004, 0x80000, 0xb76c70e9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_08.bin",      0x200006, 0x80000, 0xe60c9556, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_10.bin",      0x400000, 0x80000, 0x97976ff5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_12.bin",      0x400002, 0x80000, 0xb350a840, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_11.bin",      0x400004, 0x80000, 0x8fb94743, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_13.bin",      0x400006, 0x80000, 0xda810d5f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "mb_qa.rom",      0x00000, 0x08000, 0xe21a03c4 )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x8000, REGION_USER1, 0 )
	/* the encrypted Z80 ROM will be copied here, where the main CPU can read it. */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "mb_q1.bin",      0x000000, 0x80000, 0x0630c3ce )
	ROM_LOAD( "mb_q2.bin",      0x080000, 0x80000, 0x354f9c21 )
	ROM_LOAD( "mb_q3.bin",      0x100000, 0x80000, 0x7838487c )
	ROM_LOAD( "mb_q4.bin",      0x180000, 0x80000, 0xab66e087 )
	ROM_LOAD( "mb_q5.bin",      0x200000, 0x80000, 0xc789fef2 )
	ROM_LOAD( "mb_q6.bin",      0x280000, 0x80000, 0xecb81b61 )
	ROM_LOAD( "mb_q7.bin",      0x300000, 0x80000, 0x041e49ba )
	ROM_LOAD( "mb_q8.bin",      0x380000, 0x80000, 0x59fe702a )
ROM_END

ROM_START( mbombrd )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "mbd_26.bin",        0x000000, 0x20000, 0x72b7451c )
	ROM_LOAD16_BYTE( "mbde_30.rom",       0x000001, 0x20000, 0xa036dc16 )
	ROM_LOAD16_BYTE( "mbd_27.bin",        0x040000, 0x20000, 0x4086f534 )
	ROM_LOAD16_BYTE( "mbd_31.bin",        0x040001, 0x20000, 0x085f47f0 )
	ROM_LOAD16_BYTE( "mbd_24.bin",        0x080000, 0x20000, 0xc20895a5 )
	ROM_LOAD16_BYTE( "mbd_28.bin",        0x080001, 0x20000, 0x2618d5e1 )
	ROM_LOAD16_BYTE( "mbd_25.bin",        0x0c0000, 0x20000, 0x9bdb6b11 )
	ROM_LOAD16_BYTE( "mbd_29.bin",        0x0c0001, 0x20000, 0x3f52d5e5 )
	ROM_LOAD16_WORD_SWAP( "mbd_21.bin",   0x100000, 0x80000, 0x690c026a )
	ROM_LOAD16_WORD_SWAP( "mbd_20.bin",   0x180000, 0x80000, 0xb8b2139b )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "mb_gfx01.rom",   0x000000, 0x80000, 0x41468e06, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx03.rom",   0x000002, 0x80000, 0xf453aa9e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx02.rom",   0x000004, 0x80000, 0x2ffbfea8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_gfx04.rom",   0x000006, 0x80000, 0x1eb9841d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_05.bin",      0x200000, 0x80000, 0x506b9dc9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_07.bin",      0x200002, 0x80000, 0xaff8c2fb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_06.bin",      0x200004, 0x80000, 0xb76c70e9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_08.bin",      0x200006, 0x80000, 0xe60c9556, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_10.bin",      0x400000, 0x80000, 0x97976ff5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_12.bin",      0x400002, 0x80000, 0xb350a840, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_11.bin",      0x400004, 0x80000, 0x8fb94743, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_13.bin",      0x400006, 0x80000, 0xda810d5f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "mb_q.bin",       0x00000, 0x08000, 0xd6fa76d1 )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "mb_q1.bin",      0x000000, 0x80000, 0x0630c3ce )
	ROM_LOAD( "mb_q2.bin",      0x080000, 0x80000, 0x354f9c21 )
	ROM_LOAD( "mb_q3.bin",      0x100000, 0x80000, 0x7838487c )
	ROM_LOAD( "mb_q4.bin",      0x180000, 0x80000, 0xab66e087 )
	ROM_LOAD( "mb_q5.bin",      0x200000, 0x80000, 0xc789fef2 )
	ROM_LOAD( "mb_q6.bin",      0x280000, 0x80000, 0xecb81b61 )
	ROM_LOAD( "mb_q7.bin",      0x300000, 0x80000, 0x041e49ba )
	ROM_LOAD( "mb_q8.bin",      0x380000, 0x80000, 0x59fe702a )
ROM_END

ROM_START( mbombrdj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "mbd_26.bin",        0x000000, 0x20000, 0x72b7451c )
	ROM_LOAD16_BYTE( "mbdj_30.bin",       0x000001, 0x20000, 0xbeff31cf )
	ROM_LOAD16_BYTE( "mbd_27.bin",        0x040000, 0x20000, 0x4086f534 )
	ROM_LOAD16_BYTE( "mbd_31.bin",        0x040001, 0x20000, 0x085f47f0 )
	ROM_LOAD16_BYTE( "mbd_24.bin",        0x080000, 0x20000, 0xc20895a5 )
	ROM_LOAD16_BYTE( "mbd_28.bin",        0x080001, 0x20000, 0x2618d5e1 )
	ROM_LOAD16_BYTE( "mbd_25.bin",        0x0c0000, 0x20000, 0x9bdb6b11 )
	ROM_LOAD16_BYTE( "mbd_29.bin",        0x0c0001, 0x20000, 0x3f52d5e5 )
	ROM_LOAD16_WORD_SWAP( "mbd_21.bin",   0x100000, 0x80000, 0x690c026a )
	ROM_LOAD16_WORD_SWAP( "mbd_20.bin",   0x180000, 0x80000, 0xb8b2139b )

	ROM_REGION( 0x600000, REGION_GFX1, 0 )
	ROMX_LOAD( "mbj_01.bin",     0x000000, 0x80000, 0xa53b1c81, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mbj_03.bin",     0x000002, 0x80000, 0x23fe10f6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mbj_02.bin",     0x000004, 0x80000, 0xcb866c2f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mbj_04.bin",     0x000006, 0x80000, 0xc9143e75, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_05.bin",      0x200000, 0x80000, 0x506b9dc9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_07.bin",      0x200002, 0x80000, 0xaff8c2fb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_06.bin",      0x200004, 0x80000, 0xb76c70e9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_08.bin",      0x200006, 0x80000, 0xe60c9556, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_10.bin",      0x400000, 0x80000, 0x97976ff5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_12.bin",      0x400002, 0x80000, 0xb350a840, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_11.bin",      0x400004, 0x80000, 0x8fb94743, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mb_13.bin",      0x400006, 0x80000, 0xda810d5f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 2*0x28000, REGION_CPU2, 0 ) /* QSound Z80 code + space for decrypted opcodes */
	ROM_LOAD( "mb_q.bin",       0x00000, 0x08000, 0xd6fa76d1 )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "mb_q1.bin",      0x000000, 0x80000, 0x0630c3ce )
	ROM_LOAD( "mb_q2.bin",      0x080000, 0x80000, 0x354f9c21 )
	ROM_LOAD( "mb_q3.bin",      0x100000, 0x80000, 0x7838487c )
	ROM_LOAD( "mb_q4.bin",      0x180000, 0x80000, 0xab66e087 )
	ROM_LOAD( "mb_q5.bin",      0x200000, 0x80000, 0xc789fef2 )
	ROM_LOAD( "mb_q6.bin",      0x280000, 0x80000, 0xecb81b61 )
	ROM_LOAD( "mb_q7.bin",      0x300000, 0x80000, 0x041e49ba )
	ROM_LOAD( "mb_q8.bin",      0x380000, 0x80000, 0x59fe702a )
ROM_END

ROM_START( pnickj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "pnij36.bin",   0x00000, 0x20000, 0x2d4ffb2b )
	ROM_LOAD16_BYTE( "pnij42.bin",   0x00001, 0x20000, 0xc085dfaf )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "pnij09.bin",   0x000000, 0x20000, 0x48177b0a, ROM_SKIP(7) )
	ROMX_LOAD( "pnij01.bin",   0x000001, 0x20000, 0x01a0f311, ROM_SKIP(7) )
	ROMX_LOAD( "pnij13.bin",   0x000002, 0x20000, 0x406451b0, ROM_SKIP(7) )
	ROMX_LOAD( "pnij05.bin",   0x000003, 0x20000, 0x8c515dc0, ROM_SKIP(7) )
	ROMX_LOAD( "pnij26.bin",   0x000004, 0x20000, 0xe2af981e, ROM_SKIP(7) )
	ROMX_LOAD( "pnij18.bin",   0x000005, 0x20000, 0xf17a0e56, ROM_SKIP(7) )
	ROMX_LOAD( "pnij38.bin",   0x000006, 0x20000, 0xeb75bd8c, ROM_SKIP(7) )
	ROMX_LOAD( "pnij32.bin",   0x000007, 0x20000, 0x84560bef, ROM_SKIP(7) )
	ROMX_LOAD( "pnij10.bin",   0x100000, 0x20000, 0xc2acc171, ROM_SKIP(7) )
	ROMX_LOAD( "pnij02.bin",   0x100001, 0x20000, 0x0e21fc33, ROM_SKIP(7) )
	ROMX_LOAD( "pnij14.bin",   0x100002, 0x20000, 0x7fe59b19, ROM_SKIP(7) )
	ROMX_LOAD( "pnij06.bin",   0x100003, 0x20000, 0x79f4bfe3, ROM_SKIP(7) )
	ROMX_LOAD( "pnij27.bin",   0x100004, 0x20000, 0x83d5cb0e, ROM_SKIP(7) )
	ROMX_LOAD( "pnij19.bin",   0x100005, 0x20000, 0xaf08b230, ROM_SKIP(7) )
	ROMX_LOAD( "pnij39.bin",   0x100006, 0x20000, 0x70fbe579, ROM_SKIP(7) )
	ROMX_LOAD( "pnij33.bin",   0x100007, 0x20000, 0x3ed2c680, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pnij17.bin",    0x00000, 0x08000, 0xe86f787a )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "pnij24.bin",   0x00000, 0x20000, 0x5092257d )
	ROM_LOAD( "pnij25.bin",   0x20000, 0x20000, 0x22109aaa )
ROM_END

ROM_START( qad )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "qdu_36a.rom",  0x00000, 0x20000, 0xde9c24a0 )
	ROM_LOAD16_BYTE( "qdu_42a.rom",  0x00001, 0x20000, 0xcfe36f0c )
	ROM_LOAD16_BYTE( "qdu_37a.rom",  0x40000, 0x20000, 0x10d22320 )
	ROM_LOAD16_BYTE( "qdu_43a.rom",  0x40001, 0x20000, 0x15e6beb9 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "qdu_09.rom", 0x000000, 0x20000, 0x8c3f9f44, ROM_SKIP(7) )
	ROMX_LOAD( "qdu_01.rom", 0x000001, 0x20000, 0xf688cf8f, ROM_SKIP(7) )
	ROMX_LOAD( "qdu_13.rom", 0x000002, 0x20000, 0xafbd551b, ROM_SKIP(7) )
	ROMX_LOAD( "qdu_05.rom", 0x000003, 0x20000, 0xc3db0910, ROM_SKIP(7) )
	ROMX_LOAD( "qdu_24.rom", 0x000004, 0x20000, 0x2f1bd0ec, ROM_SKIP(7) )
	ROMX_LOAD( "qdu_17.rom", 0x000005, 0x20000, 0xa812f9e2, ROM_SKIP(7) )
	ROMX_LOAD( "qdu_38.rom", 0x000006, 0x20000, 0xccdddd1f, ROM_SKIP(7) )
	ROMX_LOAD( "qdu_32.rom", 0x000007, 0x20000, 0xa8d295d3, ROM_SKIP(7) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "qdu_23.rom",    0x00000, 0x08000, 0xcfb5264b )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "qdu_30.rom",  0x00000, 0x20000, 0xf190da84 )
	ROM_LOAD( "qdu_31.rom",  0x20000, 0x20000, 0xb7583f73 )
ROM_END

ROM_START( qadj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "qad23a.bin",   0x00000, 0x080000, 0x4d3553de )
	ROM_LOAD16_WORD_SWAP( "qad22a.bin",   0x80000, 0x80000, 0x3191ddd0 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROMX_LOAD( "qad01.bin",   0x000000, 0x80000, 0x9d853b57, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "qad02.bin",   0x000002, 0x80000, 0xb35976c4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "qad03.bin",   0x000004, 0x80000, 0xcea4ca8c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "qad04.bin",   0x000006, 0x80000, 0x41b74d1b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "qad09.bin",     0x00000, 0x08000, 0x733161cc )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "qad18.bin",   0x00000, 0x20000, 0x2bfe6f6a )
	ROM_LOAD( "qad19.bin",   0x20000, 0x20000, 0x13d3236b )
ROM_END

ROM_START( qtono2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_BYTE( "tn2j-30.11e",  0x00000, 0x20000, 0x9226eb5e )
	ROM_LOAD16_BYTE( "tn2j-37.11f",  0x00001, 0x20000, 0xd1d30da1 )
	ROM_LOAD16_BYTE( "tn2j-31.12e",  0x40000, 0x20000, 0x015e6a8a )
	ROM_LOAD16_BYTE( "tn2j-38.12f",  0x40001, 0x20000, 0x1f139bcc )
	ROM_LOAD16_BYTE( "tn2j-28.9e",   0x80000, 0x20000, 0x86d27f71 )
	ROM_LOAD16_BYTE( "tn2j-35.9f",   0x80001, 0x20000, 0x7a1ab87d )
	ROM_LOAD16_BYTE( "tn2j-29.10e",  0xc0000, 0x20000, 0x9c384e99 )
	ROM_LOAD16_BYTE( "tn2j-36.10f",  0xc0001, 0x20000, 0x4c4b2a0a )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "tn2-02m.4a",   0x000000, 0x80000, 0xf2016a34, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tn2-04m.6a",   0x000002, 0x80000, 0x094e0fb1, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tn2-01m.3a",   0x000004, 0x80000, 0xcb950cf9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tn2-03m.5a",   0x000006, 0x80000, 0x18a5bf59, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tn2-11m.4c",   0x200000, 0x80000, 0xd0edd30b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tn2-13m.6c",   0x200002, 0x80000, 0x426621c3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tn2-10m.3c",   0x200004, 0x80000, 0xa34ece70, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tn2-12m.5c",   0x200006, 0x80000, 0xe04ff2f4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "tn2j-09.12a",   0x00000, 0x08000, 0x6d8edcef )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "tn2j-18.11c",  0x00000, 0x20000, 0xa40bf9a7 )
	ROM_LOAD( "tn2j-19.12c",  0x20000, 0x20000, 0x5b3b931e )
ROM_END

ROM_START( pang3 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pa3w-17.11l",  0x00000, 0x80000, 0x12138234 )
	ROM_LOAD16_WORD_SWAP( "pa3w-16.10l",  0x80000, 0x80000, 0xd1ba585c )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pa3-01m.2c",   0x000000, 0x100000, 0x068a152c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROM_CONTINUE(              0x000004, 0x100000 )
	ROMX_LOAD( "pa3-07m.2f",   0x000002, 0x100000, 0x3a4a619d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROM_CONTINUE(              0x000006, 0x100000 )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pa3-11.11f",    0x00000, 0x08000, 0x90a08c46 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "pa3-05.10d",    0x00000, 0x20000, 0x73a10d5d )
	ROM_LOAD( "pa3-06.11d",    0x20000, 0x20000, 0xaffa4f82 )
ROM_END

ROM_START( pang3j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pa3j-17.11l",  0x00000, 0x80000, 0x21f6e51f )
	ROM_LOAD16_WORD_SWAP( "pa3j-16.10l",  0x80000, 0x80000, 0xca1d7897 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pa3-01m.2c",   0x000000, 0x100000, 0x068a152c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROM_CONTINUE(              0x000004, 0x100000 )
	ROMX_LOAD( "pa3-07m.2f",   0x000002, 0x100000, 0x3a4a619d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROM_CONTINUE(              0x000006, 0x100000 )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pa3-11.11f",    0x00000, 0x08000, 0x90a08c46 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "pa3-05.10d",    0x00000, 0x20000, 0x73a10d5d )
	ROM_LOAD( "pa3-06.11d",    0x20000, 0x20000, 0xaffa4f82 )
ROM_END

ROM_START( megaman )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rcma_23b.rom",   0x000000, 0x80000, 0x61e4a397 )
	ROM_LOAD16_WORD_SWAP( "rcma_22b.rom",   0x080000, 0x80000, 0x708268c4 )
	ROM_LOAD16_WORD_SWAP( "rcma_21a.rom",   0x100000, 0x80000, 0x4376ea95 )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROMX_LOAD( "rcm_01.rom",    0x000000, 0x80000, 0x6ecdf13f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_02.rom",    0x000002, 0x80000, 0x944d4f0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_03.rom",    0x000004, 0x80000, 0x36f3073c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_04.rom",    0x000006, 0x80000, 0x54e622ff, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_05.rom",    0x200000, 0x80000, 0x5dd131fd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_06.rom",    0x200002, 0x80000, 0xf0faf813, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_07.rom",    0x200004, 0x80000, 0x826de013, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_08.rom",    0x200006, 0x80000, 0xfbff64cf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_10.rom",    0x400000, 0x80000, 0x4dc8ada9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_11.rom",    0x400002, 0x80000, 0xf2b9ee06, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_12.rom",    0x400004, 0x80000, 0xfed5f203, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_13.rom",    0x400006, 0x80000, 0x5069d4a9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_14.rom",    0x600000, 0x80000, 0x303be3bd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_15.rom",    0x600002, 0x80000, 0x4f2d372f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_16.rom",    0x600004, 0x80000, 0x93d97fde, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_17.rom",    0x600006, 0x80000, 0x92371042, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "rcm_09.rom",    0x00000, 0x08000, 0x9632d6ef )
	ROM_CONTINUE(              0x10000, 0x18000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "rcm_18.rom",    0x00000, 0x20000, 0x80f1f8aa )
	ROM_LOAD( "rcm_19.rom",    0x20000, 0x20000, 0xf257dbe1 )
ROM_END

ROM_START( rockmanj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rcm23a.bin",   0x000000, 0x80000, 0xefd96cb2 )
	ROM_LOAD16_WORD_SWAP( "rcm22a.bin",   0x080000, 0x80000, 0x8729a689 )
	ROM_LOAD16_WORD_SWAP( "rcm21a.bin",   0x100000, 0x80000, 0x517ccde2 )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROMX_LOAD( "rcm_01.rom",    0x000000, 0x80000, 0x6ecdf13f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_02.rom",    0x000002, 0x80000, 0x944d4f0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_03.rom",    0x000004, 0x80000, 0x36f3073c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_04.rom",    0x000006, 0x80000, 0x54e622ff, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_05.rom",    0x200000, 0x80000, 0x5dd131fd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_06.rom",    0x200002, 0x80000, 0xf0faf813, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_07.rom",    0x200004, 0x80000, 0x826de013, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_08.rom",    0x200006, 0x80000, 0xfbff64cf, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_10.rom",    0x400000, 0x80000, 0x4dc8ada9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_11.rom",    0x400002, 0x80000, 0xf2b9ee06, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_12.rom",    0x400004, 0x80000, 0xfed5f203, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_13.rom",    0x400006, 0x80000, 0x5069d4a9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_14.rom",    0x600000, 0x80000, 0x303be3bd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_15.rom",    0x600002, 0x80000, 0x4f2d372f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_16.rom",    0x600004, 0x80000, 0x93d97fde, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rcm_17.rom",    0x600006, 0x80000, 0x92371042, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "rcm_09.rom",    0x00000, 0x08000, 0x9632d6ef )
	ROM_CONTINUE(              0x10000, 0x18000 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "rcm_18.rom",    0x00000, 0x20000, 0x80f1f8aa )
	ROM_LOAD( "rcm_19.rom",    0x20000, 0x20000, 0xf257dbe1 )
ROM_END



static void init_wof(void)
{
	wof_decode();
	init_cps1();
}

static void init_dino(void)
{
	dino_decode();
	init_cps1();
}

static void init_punisher(void)
{
	punisher_decode();
	init_cps1();
}

static void init_slammast(void)
{
	slammast_decode();
	init_cps1();
}

static void init_pang3(void)
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);
	int A,src,dst;

	for (A = 0x80000;A < 0x100000;A += 2)
	{
		/* only the low 8 bits of each word are encrypted */
		src = rom[A/2];
		dst = src & 0xff00;
		if ( src & 0x01) dst ^= 0x04;
		if ( src & 0x02) dst ^= 0x21;
		if ( src & 0x04) dst ^= 0x01;
		if (~src & 0x08) dst ^= 0x50;
		if ( src & 0x10) dst ^= 0x40;
		if ( src & 0x20) dst ^= 0x06;
		if ( src & 0x40) dst ^= 0x08;
		if (~src & 0x80) dst ^= 0x88;
		rom[A/2] = dst;
	}

	init_cps1();
}



GAME( 1988, forgottn, 0,        forgottn, forgottn, cps1,     ROT0,       "Capcom", "Forgotten Worlds (US)" )
GAME( 1988, lostwrld, forgottn, forgottn, forgottn, cps1,     ROT0,       "Capcom", "Lost Worlds (Japan)" )
GAME( 1988, ghouls,   0,        cps1,     ghouls,   cps1,     ROT0,       "Capcom", "Ghouls'n Ghosts (World)" )
GAME( 1988, ghoulsu,  ghouls,   cps1,     ghouls,   cps1,     ROT0,       "Capcom", "Ghouls'n Ghosts (US)" )
GAME( 1988, daimakai, ghouls,   cps1,     ghouls,   cps1,     ROT0,       "Capcom", "Dai Makai-Mura (Japan)" )
GAME( 1989, strider,  0,        cps1,     strider,  cps1,     ROT0,       "Capcom", "Strider (US)" )
GAME( 1989, striderj, strider,  cps1,     strider,  cps1,     ROT0,       "Capcom", "Strider Hiryu (Japan set 1)" )
GAME( 1989, stridrja, strider,  cps1,     strider,  cps1,     ROT0,       "Capcom", "Strider Hiryu (Japan set 2)" )
GAME( 1989, dwj,      0,        cps1,     dwj,      cps1,     ROT0,       "Capcom", "Tenchi wo Kurau (Japan)" )
GAME( 1989, willow,   0,        cps1,     willow,   cps1,     ROT0,       "Capcom", "Willow (Japan, English)" )
GAME( 1989, willowj,  willow,   cps1,     willow,   cps1,     ROT0,       "Capcom", "Willow (Japan, Japanese)" )
GAME( 1989, unsquad,  0,        cps1,     unsquad,  cps1,     ROT0,       "Capcom", "U.N. Squadron (US)" )
GAME( 1989, area88,   unsquad,  cps1,     unsquad,  cps1,     ROT0,       "Capcom", "Area 88 (Japan)" )
GAME( 1989, ffight,   0,        cps1,     ffight,   cps1,     ROT0,       "Capcom", "Final Fight (World)" )
GAME( 1989, ffightu,  ffight,   cps1,     ffight,   cps1,     ROT0,       "Capcom", "Final Fight (US)" )
GAME( 1989, ffightj,  ffight,   cps1,     ffight,   cps1,     ROT0,       "Capcom", "Final Fight (Japan)" )
GAME( 1990, 1941,     0,        cps1,     1941,     cps1,     ROT270,     "Capcom", "1941 - Counter Attack (World)" )
GAME( 1990, 1941j,    1941,     cps1,     1941,     cps1,     ROT270,     "Capcom", "1941 - Counter Attack (Japan)" )
GAME( 1990, mercs,    0,        cps1,     mercs,    cps1,     ROT270,     "Capcom", "Mercs (World)" )
GAME( 1990, mercsu,   mercs,    cps1,     mercs,    cps1,     ROT270,     "Capcom", "Mercs (US)" )
GAME( 1990, mercsj,   mercs,    cps1,     mercs,    cps1,     ROT270,     "Capcom", "Senjo no Ookami II (Japan)" )
GAME( 1990, mtwins,   0,        cps1,     mtwins,   cps1,     ROT0,       "Capcom", "Mega Twins (World)" )
GAME( 1990, chikij,   mtwins,   cps1,     mtwins,   cps1,     ROT0,       "Capcom", "Chiki Chiki Boys (Japan)" )
GAME( 1990, msword,   0,        cps1,     msword,   cps1,     ROT0,       "Capcom", "Magic Sword - Heroic Fantasy (World)" )
GAME( 1990, mswordu,  msword,   cps1,     msword,   cps1,     ROT0,       "Capcom", "Magic Sword - Heroic Fantasy (US)" )
GAME( 1990, mswordj,  msword,   cps1,     msword,   cps1,     ROT0,       "Capcom", "Magic Sword (Japan)" )
GAME( 1990, cawing,   0,        cps1,     cawing,   cps1,     ROT0,       "Capcom", "Carrier Air Wing (World)" )
GAME( 1990, cawingj,  cawing,   cps1,     cawing,   cps1,     ROT0,       "Capcom", "U.S. Navy (Japan)" )
GAME( 1990, nemo,     0,        cps1,     nemo,     cps1,     ROT0,       "Capcom", "Nemo (World)" )
GAME( 1990, nemoj,    nemo,     cps1,     nemo,     cps1,     ROT0,       "Capcom", "Nemo (Japan)" )
GAME( 1991, sf2,      0,        sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II - The World Warrior (World 910214)" )
GAME( 1991, sf2ua,    sf2,      sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II - The World Warrior (US 910206)" )
GAME( 1991, sf2ub,    sf2,      sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II - The World Warrior (US 910214)" )
GAME( 1991, sf2ue,    sf2,      sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II - The World Warrior (US 910228)" )
GAME( 1991, sf2ui,    sf2,      sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II - The World Warrior (US 910522)" )
GAME( 1991, sf2j,     sf2,      sf2,      sf2j,     cps1,     ROT0_16BIT, "Capcom", "Street Fighter II - The World Warrior (Japan 911210)" )
GAME( 1991, sf2ja,    sf2,      sf2,      sf2j,     cps1,     ROT0_16BIT, "Capcom", "Street Fighter II - The World Warrior (Japan 910214)" )
GAME( 1991, sf2jc,    sf2,      sf2,      sf2j,     cps1,     ROT0_16BIT, "Capcom", "Street Fighter II - The World Warrior (Japan 910306)" )
GAME( 1991, 3wonders, 0,        cps1,     3wonders, cps1,     ROT0_16BIT, "Capcom", "Three Wonders (World)" )
GAME( 1991, 3wonderu, 3wonders, cps1,     3wonders, cps1,     ROT0_16BIT, "Capcom", "Three Wonders (US)" )
GAME( 1991, wonder3,  3wonders, cps1,     3wonders, cps1,     ROT0_16BIT, "Capcom", "Wonder 3 (Japan)" )
GAME( 1991, kod,      0,        cps1,     kod,      cps1,     ROT0,       "Capcom", "The King of Dragons (World)" )
GAME( 1991, kodu,     kod,      cps1,     kod,      cps1,     ROT0,       "Capcom", "The King of Dragons (US)" )
GAME( 1991, kodj,     kod,      cps1,     kod,      cps1,     ROT0,       "Capcom", "The King of Dragons (Japan)" )
GAMEX(1991, kodb,     kod,      cps1,     kod,      cps1,     ROT0,       "Capcom", "The King of Dragons (bootleg)", GAME_NOT_WORKING )
GAME( 1991, captcomm, 0,        cps1,     captcomm, cps1,     ROT0_16BIT, "Capcom", "Captain Commando (World)" )
GAME( 1991, captcomu, captcomm, cps1,     captcomm, cps1,     ROT0_16BIT, "Capcom", "Captain Commando (US)" )
GAME( 1991, captcomj, captcomm, cps1,     captcomm, cps1,     ROT0_16BIT, "Capcom", "Captain Commando (Japan)" )
GAME( 1991, knights,  0,        cps1,     knights,  cps1,     ROT0_16BIT, "Capcom", "Knights of the Round (World)" )
GAME( 1991, knightsu, knights,  cps1,     knights,  cps1,     ROT0_16BIT, "Capcom", "Knights of the Round (US)" )
GAME( 1991, knightsj, knights,  cps1,     knights,  cps1,     ROT0_16BIT, "Capcom", "Knights of the Round (Japan)" )
GAME( 1992, sf2ce,    0,        sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II' - Champion Edition (World)" )
GAME( 1992, sf2ceua,  sf2ce,    sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II' - Champion Edition (US rev A)" )
GAME( 1992, sf2ceub,  sf2ce,    sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II' - Champion Edition (US rev B)" )
GAME( 1992, sf2cej,   sf2ce,    sf2,      sf2j,     cps1,     ROT0_16BIT, "Capcom", "Street Fighter II' - Champion Edition (Japan)" )
GAME( 1992, sf2rb,    sf2ce,    sf2,      sf2,      cps1,     ROT0_16BIT, "hack",  "Street Fighter II' - Champion Edition (Rainbow set 1)" )
GAME( 1992, sf2rb2,   sf2ce,    sf2,      sf2,      cps1,     ROT0_16BIT, "hack",  "Street Fighter II' - Champion Edition (Rainbow set 2)" )
GAME( 1992, sf2red,   sf2ce,    sf2,      sf2,      cps1,     ROT0_16BIT, "hack",  "Street Fighter II' - Champion Edition (Red Wave)" )
GAME( 1992, sf2v004,  sf2ce,    sf2,      sf2,      cps1,     ROT0_16BIT, "hack",  "Street Fighter II! - Champion Edition (V004)" )
GAME( 1992, sf2accp2, sf2ce,    sf2accp2, sf2,      cps1,     ROT0_16BIT, "hack",  "Street Fighter II' - Champion Edition (Accelerator Pt.II)" )
GAME( 1992, varth,    0,        cps1,     varth,    cps1,     ROT270,     "Capcom", "Varth - Operation Thunderstorm (World)" )
GAME( 1992, varthu,   varth,    cps1,     varth,    cps1,     ROT270,     "Capcom (Romstar license)", "Varth - Operation Thunderstorm (US)" )
GAME( 1992, varthj,   varth,    cps1,     varth,    cps1,     ROT270,     "Capcom", "Varth - Operation Thunderstorm (Japan)" )
GAME( 1992, cworld2j, 0,        cps1,     cworld2j, cps1,     ROT0_16BIT, "Capcom", "Capcom World 2 (Japan)" )
GAME( 1992, sf2t,     sf2ce,    sf2,      sf2,      cps1,     ROT0_16BIT, "Capcom", "Street Fighter II' - Hyper Fighting (US)" )
GAME( 1992, sf2tj,    sf2ce,    sf2,      sf2j,     cps1,     ROT0_16BIT, "Capcom", "Street Fighter II' Turbo - Hyper Fighting (Japan)" )
GAME( 1992, qad,      0,        cps1,     qad,      cps1,     ROT0,       "Capcom", "Quiz & Dragons (US)" )
GAME( 1994, qadj,     qad,      cps1,     qadj,     cps1,     ROT0,       "Capcom", "Quiz & Dragons (Japan)" )
GAME( 1995, qtono2,   0,        cps1,     qtono2,   cps1,     ROT0,       "Capcom", "Quiz Tonosama no Yabou 2 Zenkoku-ban (Japan)" )
GAME( 1995, megaman,  0,        cps1,     megaman,  cps1,     ROT0_16BIT, "Capcom", "Mega Man - The Power Battle (Asia)" )
GAME( 1995, rockmanj, megaman,  cps1,     megaman,  cps1,     ROT0_16BIT, "Capcom", "Rockman - The Power Battle (Japan)" )

GAME( 1992, wof,      0,        qsound,   wof,      wof,      ROT0,       "Capcom", "Warriors of Fate (World)" )
GAME( 1992, wofa,     wof,      qsound,   wof,      wof,      ROT0,       "Capcom", "Sangokushi II (Asia)" )
GAME( 1992, wofu,     wof,      qsound,   wof,      wof,      ROT0,       "Capcom", "Warriors of Fate (US)" )
GAME( 1992, wofj,     wof,      qsound,   wof,      wof,      ROT0,       "Capcom", "Tenchi wo Kurau II - Sekiheki no Tatakai (Japan)" )
GAME( 1993, dino,     0,        qsound,   dino,     dino,     ROT0,       "Capcom", "Cadillacs and Dinosaurs (World)" )
GAME( 1993, dinoj,    dino,     qsound,   dino,     dino ,    ROT0,       "Capcom", "Cadillacs Kyouryuu-Shinseiki (Japan)" )
GAME( 1993, punisher, 0,        qsound,   punisher, punisher, ROT0,       "Capcom", "The Punisher (World)" )
GAME( 1993, punishru, punisher, qsound,   punisher, punisher, ROT0,       "Capcom", "The Punisher (US)" )
GAME( 1993, punishrj, punisher, qsound,   punisher, punisher, ROT0,       "Capcom", "The Punisher (Japan)" )
GAME( 1993, slammast, 0,        qsound,   slammast, slammast, ROT0_16BIT, "Capcom", "Saturday Night Slam Masters (World)" )
GAME( 1993, slammasu, slammast, qsound,   slammast, slammast, ROT0_16BIT, "Capcom", "Saturday Night Slam Masters (US)" )
GAME( 1993, mbomberj, slammast, qsound,   slammast, slammast, ROT0_16BIT, "Capcom", "Muscle Bomber - The Body Explosion (Japan)" )
GAME( 1993, mbombrd,  slammast, qsound,   slammast, slammast, ROT0_16BIT, "Capcom", "Muscle Bomber Duo - Ultimate Team Battle (World)" )
GAME( 1993, mbombrdj, slammast, qsound,   slammast, slammast, ROT0_16BIT, "Capcom", "Muscle Bomber Duo - Heat Up Warriors (Japan)" )

GAME( 1994, pnickj,   0,        cps1,     pnickj,   cps1,     ROT0,       "Compile (Capcom license)", "Pnickies (Japan)" )
/* Japanese version of Pang 3 is encrypted, Euro version is not */
GAME( 1995, pang3,    0,        pang3,    pang3,    cps1,     ROT0_16BIT, "Mitchell", "Pang! 3 (Euro)" )
GAME( 1995, pang3j,   pang3,    pang3,    pang3,    pang3,    ROT0_16BIT, "Mitchell", "Pang! 3 (Japan)" )
