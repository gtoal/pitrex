/***************************************************************************

Tube Panic
(c)1984 Nichibutsu

Driver by Jarek Burczynski.

Thanks to Al for PCB scans and to Tim for screenshots.
They were very helpful.


TODO
====

There are sprites and background graphics missing (screen refresh is
incomplete). I can not guess how to draw them without schematics.

Sprites are of different sizes. There are at least four possible sizes,
mixed in the graphics ROM. Seems like there is some kind of a blitter.


Some dip switches might be wrong aswell.


----
Tube Panic
Nichibutsu 1984

CPU
84P0100B

tp-b 6->1          19.968MHz

                  tp-2 tp-1  2147 2147 2147 2147 2147 2147 2147 2147

               +------ daughter board ------+
               tp-p 5->8 6116  6116 tp-p 4->1
               +----------------------------+

   z80a              z80a                     z80a

                8910 8910 8910     6116  - tp-s 2->1

       16MHz

 VID
 84P101B

   6MHz                                        +------+ daughter board
                                  6116          tp-c 1
   40pin on daughterboard                       tp-c 2
                              tp-g 3            tp-c 3
   tp-g 6                                       tp-c 4
                              tp-g 4
   tp-g 5                                       tp-c 8
                                                tp-c 7
   6116                                         tp-c 6
                                       tp-g 1   tp-c 5
                                               +------+
     tp-g 7
                                       tp-g 2
     tp-g 8
                                             6164 6164 6164 6164
                                        6164 6164 6164 6164


----

Roller Jammer
Nichibutsu 1985

84P0501A

               SW1      SW2                      16A

Z80   6116                        TP-B.5         16B     6116
TP-S.1 TP-S.2 TP-S.3 TP-B.1  8212 TP-B.2 TP-B.3          TP-B.4


 TP-P.1 TP-P.2 TP-P.3 TP-P.4 6116 6116 TP-P.5 TP-P.6 TP-P.7 TP-P.8    6116


       8910 8910 8910         Z80A      Z80A

                               16MHz                       19.968MHz



                      --------------------------------

  6MHz
                                     6116
                                                     TP-C.8
  40PIN_CUST                   TP-G.4                TP-C.7
                                                     TP-C.6
  TP-G.8                        TP-G.3               TP-C.5

  TP-G.7                                 TP-G.2
                                                     TP-C.4
  6116                                   TP-G.1      TP-C.3
                                                     TP-C.2
                                                     TP-C.1
   TP-G.6

   TP-G.5                                         6164 6164 6164 6164
                                             6164 6164 6164 6164
 2114

----




***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

void tubep_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
void tubep_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int tubep_vh_start(void);
void tubep_vh_stop(void);

extern data8_t * tubep_textram;
extern WRITE_HANDLER( tubep_textram_w );

static data8_t *sharedram;
static void * scanline_timer;
static int sound_latch;




/****************************** Main CPU ************************************/


static WRITE_HANDLER ( sharedram_w )
{
	sharedram[offset] = data;
}
static READ_HANDLER ( sharedram_r )
{
	return sharedram[offset];
}


static MEMORY_READ_START( tubep_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },	/*up to 9fff in rjammer */
	{ 0xa000, 0xa7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( tubep_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },	/*up to 9fff in rjammer */
	{ 0xa000, 0xa7ff, MWA_RAM },							/* 6116 #0 */
	{ 0xc000, 0xc7ff, tubep_textram_w, &tubep_textram }, 	/* 2147s  0x0-0x7ff  ?????? */
	{ 0xe000, 0xe06a, sharedram_w },						/* 6116 #1 */
MEMORY_END

static PORT_READ_START( tubep_readport )
	{ 0x80, 0x80, input_port_3_r },
	{ 0x90, 0x90, input_port_4_r },
	{ 0xa0, 0xa0, input_port_5_r },

	{ 0xb0, 0xb0, input_port_2_r },
	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xd0, 0xd0, input_port_0_r },
PORT_END



static WRITE_HANDLER( tubep_port80_w )
{
/*  ??? */
/*	if (data != 0x0f) */
/*		logerror("PORT80 data = %2x\n",data); */

	return;
}

static unsigned char b01[2];

static WRITE_HANDLER( tubep_portb01_w )
{
	b01[offset] = data;
/*
	port b0: bit0 - coin 1 counter
	port b1	 bit0 - coin 2 counter
*/
/*	if (data) */
/*		usrintf_showmessage("CPU 0, port b0=%2x, b1=%2x", b01[0], b01[1]); */

	return;
}

static WRITE_HANDLER( tubep_portb6_w )
{
/*	if (data) */
/*		usrintf_showmessage("CPU 0, port b6=%2x pc=%4x", data, cpu_get_pc() ); */

	return;
}

static WRITE_HANDLER( tubep_soundlatch_w )
{
	sound_latch = data | 0x80;
	/*usrintf_showmessage("CPU 0 port d0=%4x", data ); */
	/*logerror("SOUND COMM WRITE %2x\n",sound_latch); */
}

static PORT_WRITE_START( tubep_writeport )
	{ 0x80, 0x80, tubep_port80_w },
	{ 0xb0, 0xb1, tubep_portb01_w },
	 /*b5 is also written, but rarely and only with data=0 */
	{ 0xb6, 0xb6, tubep_portb6_w },
	{ 0xd0, 0xd0, tubep_soundlatch_w },
PORT_END


static WRITE_HANDLER( rjammer_soundlatch_w )
{
	sound_latch = data;
	/*usrintf_showmessage("CPU 0 soundlatch=%4x", data ); */

	cpu_set_nmi_line(2, PULSE_LINE);

	logerror("RJAMMER SOUND COMM WRITE %2x\n",sound_latch);
}

static PORT_WRITE_START( rjammer_writeport )
	/*{ 0xd0, 0xd0, tubep_soundlatch_w }, */
	{ 0xd5, 0xd5, tubep_port80_w },
	{ 0xe0, 0xe0, tubep_port80_w },
	{ 0xf0, 0xf0, rjammer_soundlatch_w },
PORT_END

static MEMORY_READ_START( rjammer_readmem )
	{ 0x0000, 0x9fff, MRA_ROM },	/*only up to 7fff in tube panic */
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( rjammer_writemem )
	{ 0x0000, 0x9fff, MWA_ROM },	/*only up to 7fff in tube panic */
	{ 0xa000, 0xa7ff, MWA_RAM },							/* */
	{ 0xe000, 0xe7ff, MWA_RAM },							/* */
	{ 0xc000, 0xc7ff, tubep_textram_w, &tubep_textram }, 	/*  */
MEMORY_END



/****************************** Graph CPU ************************************/


static READ_HANDLER( tubep_g_fd4a_r )
{
	if (offset==0)
		return 0x00;
	if (offset==1)
		return 0xfc;

/*read only in one place: PC=0x008e  ld hl,($fd4a) */
/*expects fc00 or fca5 */
/*if value returned is not one of the above then program jumps to zero */
/*if value was OK, then it copies part of the shared ram to that value address */

	return 0x00;
}

static MEMORY_READ_START( tubep_g_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xe07f, sharedram_r },
	{ 0xe080, 0xe7ff, MRA_RAM },
	{ 0xfd4a, 0xfd4b, tubep_g_fd4a_r }, /*could it be a bad ROM ???*/
MEMORY_END

static WRITE_HANDLER( tubep_a000_w )
{
/*
	Z position of a tube (how deep in the tube we are)
	this changes while player moves into the tube (ie constantly)
	range: 0x00-0xff
*/
/*	usrintf_showmessage("A000=%2x",data); */
}
static WRITE_HANDLER( tubep_c000_w )
{
/*
	X position of a tube
	this changes when player moves to the left or right
	range: 0x00-0xff
*/
/*	usrintf_showmessage("c000=%2x",data); */
}


static MEMORY_WRITE_START( tubep_g_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },

	{ 0xa000, 0xa000, tubep_a000_w },
	{ 0xc000, 0xc000, tubep_c000_w },

	{ 0xe000, 0xe07f, sharedram_w, &sharedram },	/* 6116 #1 */
	{ 0xe080, 0xe7ff, MWA_RAM },					/* 6116 #1 */

	{ 0xe800, 0xebff, MWA_RAM },					/* 2147s 0x800 - 0xbff ?????? */

	{ 0xf000, 0xf01f, MWA_NOP }, /* Background color lookup table ?? */
	{ 0xfc00, 0xfcff, MWA_NOP }, /* program copies here part of shared ram ?? */
MEMORY_END



static MEMORY_READ_START( rjammer_g_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xe000, 0xe0ff, MRA_RAM },

	{ 0xf800, 0xf9ff, MRA_RAM },

	/*{ 0xe000, 0xe07f, sharedram_r }, */
	/*{ 0xe080, 0xe7ff, MRA_RAM }, */
	/*{ 0xfd4a, 0xfd4b, tubep_g_fd4a_r },    ???    */
MEMORY_END


static MEMORY_WRITE_START( rjammer_g_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xa7ff, MWA_RAM },		/*  */

	{ 0xe000, 0xe0ff, MWA_RAM },

	{ 0xe800, 0xefff, MWA_RAM },		/*  */

	{ 0xf800, 0xf9ff, MWA_RAM },		/*  */

	/*{ 0xa000, 0xa000, tubep_a000_w }, */
	/*{ 0xc000, 0xc000, tubep_c000_w }, */

/*	{ 0xe000, 0xe07f, sharedram_w, &sharedram },	   6116 #1    */
/*	{ 0xe080, 0xe7ff, MWA_RAM },					   6116 #1    */
/*	{ 0xe800, 0xebff, MWA_RAM },					   2147s 0x800 - 0xbff ??????    */
/*	{ 0xf000, 0xf01f, MWA_NOP },    Background color lookup table ??    */
/*	{ 0xfc00, 0xfcff, MWA_NOP },    program copies here part of shared ram ??    */
MEMORY_END



/****************************** Sound CPU ************************************/


static READ_HANDLER( tubep_soundlatch_r )
{
 	int res;

	res = sound_latch;
	sound_latch = 0; /* "=0" ????  or "&= 0x7f" ?????  works either way */

	/*logerror("SOUND COMM READ %2x\n",res);*/

	return res;
}

static READ_HANDLER( tubep_sound_irq_ack )
{
	cpu_set_irq_line(2, 0, CLEAR_LINE);
	return 0;
}

static WRITE_HANDLER( tubep_sound_unknown )
{
	/*logerror("Sound CPU writes to port 0x07 - unknown function\n");*/
	return;
}

static WRITE_HANDLER( rjammer_sound_unknown_w )
{
	/*logerror("Sound CPU writes to port 0x80 - unknown function\n");*/
	cpu_set_irq_line(2, 0, CLEAR_LINE); /*????? */
	return;
}

static MEMORY_READ_START( tubep_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xd000, 0xd000, tubep_sound_irq_ack },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( tubep_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM },		/* 6116 #3 */
MEMORY_END

static PORT_READ_START( tubep_sound_readport )
	{ 0x06, 0x06, tubep_soundlatch_r },
PORT_END

static PORT_WRITE_START( tubep_sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x02, 0x02, AY8910_control_port_1_w },
	{ 0x03, 0x03, AY8910_write_port_1_w },
	{ 0x04, 0x04, AY8910_control_port_2_w },
	{ 0x05, 0x05, AY8910_write_port_2_w },
	{ 0x07, 0x07, tubep_sound_unknown },
PORT_END

static READ_HANDLER( rjammer_soundlatch_r )
{
 	int res;

	res = sound_latch;
	sound_latch = 0; /* not needed */
	/*cpu_set_nmi_line(2, CLEAR_LINE); */

	logerror("RJAMMER SOUND COMM READ %2x\n",res);


	return res;
}

static PORT_READ_START( rjammer_sound_readport )
	{ 0x00, 0x00, rjammer_soundlatch_r },
PORT_END

static PORT_WRITE_START( rjammer_sound_writeport )
	{ 0x90, 0x90, AY8910_control_port_0_w },
	{ 0x91, 0x91, AY8910_write_port_0_w },
	{ 0x92, 0x92, AY8910_control_port_1_w },
	{ 0x93, 0x93, AY8910_write_port_1_w },
	{ 0x94, 0x94, AY8910_control_port_2_w },
	{ 0x95, 0x95, AY8910_write_port_2_w },
	{ 0x10, 0x10, tubep_sound_unknown },
	{ 0x80, 0x80, rjammer_sound_unknown_w },
PORT_END


static WRITE_HANDLER( ay8910_portA_0_w )
{
		/*unknown sound control */
}
static WRITE_HANDLER( ay8910_portB_0_w )
{
		/*unknown sound control */
}
static WRITE_HANDLER( ay8910_portA_1_w )
{
		/*unknown sound control */
}
static WRITE_HANDLER( ay8910_portB_1_w )
{
		/*unknown sound control */
}
static WRITE_HANDLER( ay8910_portA_2_w )
{
		/*unknown sound control */
}
static WRITE_HANDLER( ay8910_portB_2_w )
{
		/*unknown sound control */
}


static void scanline_callback(int scanline)
{
	cpu_set_irq_line(2,0,ASSERT_LINE);	/* sound cpu interrupt (music tempo) */

	scanline += 128;
	scanline &= 255;

	scanline_timer = timer_set( cpu_getscanlinetime( scanline ), scanline, scanline_callback );
}

static void init_machine(void)
{
	scanline_timer = timer_set(cpu_getscanlinetime( 64 ), 64, scanline_callback );
}

static void scanline_callback_rjammer (int scanline)
{
	cpu_set_irq_line(2,0,ASSERT_LINE);	/* sound cpu interrupt (music tempo) */

	scanline += 8;
	scanline &= 255;

	scanline_timer = timer_set( cpu_getscanlinetime( scanline ), scanline, scanline_callback_rjammer );
}
static void init_machine_rjammer(void)
{
	scanline_timer = timer_set(cpu_getscanlinetime( 8 ), 8, scanline_callback_rjammer );
}


INPUT_PORTS_START( tubep )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Coin, Start */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "40000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x04, "60000" )
	PORT_DIPSETTING(    0x00, "80000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
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
	PORT_DIPNAME( 0x20, 0x20, "In Game Sounds" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END




INPUT_PORTS_START( rjammer )
	PORT_START	/* DSW3 */
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
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )/*ok */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Bonus Time x" )/*ok */
	PORT_DIPSETTING(    0x02, "100" )
	PORT_DIPSETTING(    0x00, "200" )
	PORT_DIPNAME( 0x0c, 0x0c, "Clear Men" )/*ok */
	PORT_DIPSETTING(    0x0c, "20" )
	PORT_DIPSETTING(    0x08, "30" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )/*ok */
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Time" )
	PORT_DIPSETTING(    0x20, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )/*ok */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )/*ok */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )/*ok */
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Coin, Start */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8, 8,	/* 8*8 characters */
	512,	/* 512 characters */
	1,		/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

#if 1
#define LS (64)
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */

	{ 0, 1, 2, 3 }, /* plane offset */

	{ 4, 0,  12, 8,	 20, 16, 28, 24,
 	 36, 32, 44, 40, 52, 48, 60, 56 },

/*	{ 0*LS, 1*LS, 2*LS,  3*LS,  4*LS,  5*LS,  6*LS,  7*LS, */
/*	  8*LS, 9*LS, 10*LS, 11*LS, 12*LS, 13*LS, 14*LS, 15*LS }, */
	{ 15*LS, 14*LS, 13*LS, 12*LS, 11*LS, 10*LS, 9*LS, 8*LS,
      7*LS,  6*LS,  5*LS,  4*LS,  3*LS,  2*LS,  1*LS, 0*LS },

	128*8
};
#endif

#if 0
#define LS (32)
static struct GfxLayout spritelayout =
{
	8,8,	/* 8*8 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */

	{ 0, 1, 2, 3 }, /* plane offset */

	{ 4, 0,  12, 8,	 20, 16, 28, 24 },

	{ 7*LS, 6*LS, 5*LS, 4*LS, 3*LS, 2*LS, 1*LS, 0*LS },

	32*8
};
#endif

#if 0
#define LS (128)
static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	128,	/* 128 sprites */
	4,	/* 4 bits per pixel */

	{ 0, 1, 2, 3 }, /* plane offset */

	{ 4, 0,  12, 8,	 20, 16, 28, 24,
 	 36, 32, 44, 40, 52, 48, 60, 56,
	 68, 64, 76, 72, 84, 80, 92, 88,
 	 100, 96,108,104,116,112,124,120 },

	{ 31*LS, 30*LS, 29*LS,  28*LS,  27*LS, 26*LS, 25*LS, 24*LS,
	  23*LS, 22*LS, 21*LS,  20*LS,  19*LS, 18*LS, 17*LS, 16*LS,
	  15*LS, 14*LS, 13*LS,  12*LS,  11*LS, 10*LS, 9*LS,   8*LS,
	   7*LS, 6*LS,  5*LS,   4*LS,   3*LS,  2*LS,  1*LS,   0*LS },

	512*8
};
#endif

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX3,      0, &charlayout,       0, 16 }, /*16 color codes*/
	{ REGION_GFX3, 0x1000, &charlayout,       0, 2 }, /**/
	{ REGION_GFX3, 0x2000, &charlayout,       0, 2 }, /**/
	{ REGION_GFX3, 0x3000, &charlayout,       0, 2 }, /**/

	{ REGION_GFX2, 0, &spritelayout,          2*16, 1 }, /*1 color code*/
	{ -1 }
};


static struct AY8910interface ay8910_interface =
{
	3,			/* 3 chips */
	16000000/8,		/* 2 MHz ??? */
	{ 25, 25, 25 },
	{ 0, 0, 0 }, /*read port A*/
	{ 0, 0, 0 }, /*read port B*/
	{ ay8910_portA_0_w, ay8910_portA_1_w, ay8910_portA_2_w }, /*write port A*/
	{ ay8910_portB_0_w, ay8910_portB_1_w, ay8910_portB_2_w }  /*write port B*/
};

static struct MachineDriver machine_driver_tubep =
{
	{
		{
			CPU_Z80,
			16000000/8,	/* 2 MHz ???*/
			tubep_readmem, tubep_writemem, tubep_readport, tubep_writeport,
			interrupt, 1
		},
		{
			CPU_Z80,
			16000000/8,	/* 2 MHz ??? */
			tubep_g_readmem, tubep_g_writemem, 0, 0,
			interrupt, 1
		},
#if 1
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/8,	/* 2 MHz ???*/
			tubep_sound_readmem, tubep_sound_writemem, tubep_sound_readport, tubep_sound_writeport,
			ignore_interrupt, 1
		},
#endif
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64, 2*16 + 16*2,
	tubep_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	tubep_vh_start,
	tubep_vh_stop,
	tubep_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};


static struct MachineDriver machine_driver_rjammer =
{
	{
		{
			CPU_Z80,
			16000000/8,	/* 2 MHz ???*/
			rjammer_readmem, rjammer_writemem, tubep_readport, rjammer_writeport,
			interrupt, 1
		},
		{
			CPU_Z80,
			16000000/8,	/* 2 MHz ??? */
			rjammer_g_readmem, rjammer_g_writemem, 0, 0,
			interrupt, 1
		},
#if 1
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/8,	/* 2 MHz ???*/
			tubep_sound_readmem, tubep_sound_writemem, rjammer_sound_readport, rjammer_sound_writeport,
			ignore_interrupt, 1
		},
#endif
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	init_machine_rjammer,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64, 2*16 + 16*2,
	tubep_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	tubep_vh_start,
	tubep_vh_stop,
	tubep_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



ROM_START( tubep )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* Z80 (main) cpu code */
	ROM_LOAD( "tp-p_5", 0x0000, 0x2000, 0xd5e0cc2f )
	ROM_LOAD( "tp-p.6", 0x2000, 0x2000, 0x97b791a0 )
	ROM_LOAD( "tp-p.7", 0x4000, 0x2000, 0xadd9983e )
	ROM_LOAD( "tp-p.8", 0x6000, 0x2000, 0xb3793cb5 )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* Z80 (graphics??) cpu code */
	ROM_LOAD( "tp-p_1", 0x0000, 0x2000, 0xb4020fcc )
	ROM_LOAD( "tp-p_2", 0x2000, 0x2000, 0xa69862d6 )
	ROM_LOAD( "tp-p.3", 0x4000, 0x2000, 0xf1d86e00 )
	ROM_LOAD( "tp-p.4", 0x6000, 0x2000, 0x0a1027bc )

	ROM_REGION( 0x10000,REGION_CPU3, 0 ) /* Z80 (sound) cpu code */
	ROM_LOAD( "tp-s.1", 0x0000, 0x2000, 0x78964fcc )
	ROM_LOAD( "tp-s.2", 0x2000, 0x2000, 0x61232e29 )

	ROM_REGION( 0xc000, REGION_GFX1, 0 ) /* */
	ROM_LOAD( "tp-b.1", 0x0000, 0x2000, 0xfda355e0 )
	ROM_LOAD( "tp-b.2", 0x2000, 0x2000, 0xcbe30149 )
	ROM_LOAD( "tp-b.3", 0x4000, 0x2000, 0xf5d118e7 )
	ROM_LOAD( "tp-b.4", 0x6000, 0x2000, 0x01952144 )
	ROM_LOAD( "tp-b.5", 0x8000, 0x2000, 0x4dabea43 )
	ROM_LOAD( "tp-b.6", 0xa000, 0x2000, 0x01952144 )

	ROM_REGION( 0x10000,REGION_GFX2, 0 ) /* */
	ROM_LOAD( "tp-c.1", 0x0000, 0x2000, 0xec002af2 )
	ROM_LOAD( "tp-c.2", 0x2000, 0x2000, 0xc44f7128 )
	ROM_LOAD( "tp-c.3", 0x4000, 0x2000, 0x4146b0c9 )
	ROM_LOAD( "tp-c.4", 0x6000, 0x2000, 0x552b58cf )
	ROM_LOAD( "tp-c.5", 0x8000, 0x2000, 0x2bb481d7 )
	ROM_LOAD( "tp-c.6", 0xa000, 0x2000, 0xc07a4338 )
	ROM_LOAD( "tp-c.7", 0xc000, 0x2000, 0x87b8700a )
	ROM_LOAD( "tp-c.8", 0xe000, 0x2000, 0xa6497a03 )

	ROM_REGION( 0x4000, REGION_GFX3, 0 ) /*  */
	ROM_LOAD( "tp-g.3", 0x0000, 0x1000, 0x657a465d )
	ROM_LOAD( "tp-g.4", 0x1000, 0x1000, 0x40a1fe00 )
	ROM_LOAD( "tp-g.1", 0x2000, 0x1000, 0x4a7407a2 )
	ROM_LOAD( "tp-g.2", 0x3000, 0x1000, 0xf0b26c2e )

	ROM_REGION( 0x8000, REGION_GFX4, 0 ) /*  */
	ROM_LOAD( "tp-g.5", 0x0000, 0x2000, 0x9f375b27 )
	ROM_LOAD( "tp-g.6", 0x2000, 0x2000, 0x3ea127b8 )
	ROM_LOAD( "tp-g.7", 0x4000, 0x2000, 0x105cb9e4 )
	ROM_LOAD( "tp-g.8", 0x6000, 0x2000, 0x27e5e6c1 )

	ROM_REGION( 0x100,   REGION_PROMS, 0 ) /* color proms */
	ROM_LOAD( "tp-2.c12", 0x0000, 0x0020, 0xac7e582f ) /* text and sprites palette */
	ROM_LOAD( "tp-1.c13", 0x0020, 0x0020, 0xcd0910d6 ) /* background palette ??? */
ROM_END

ROM_START( rjammer )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* Z80 (main) cpu code */
	ROM_LOAD( "tp-p.1", 0x0000, 0x2000, 0x93eeed67 )
	ROM_LOAD( "tp-p.2", 0x2000, 0x2000, 0xed2830c4 )
	ROM_LOAD( "tp-p.3", 0x4000, 0x2000, 0xe29f25e3 )
	ROM_LOAD( "tp-p.4", 0x8000, 0x2000, 0x6ed71fbc ) /* 0x4000 !!!! */
	ROM_CONTINUE(       0x6000, 0x2000 )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* Z80 (graphics??) cpu code */
	ROM_LOAD( "tp-p.8", 0x0000, 0x2000, 0x388b9c66 )
	ROM_LOAD( "tp-p.7", 0x2000, 0x2000, 0x595030bb )
	ROM_LOAD( "tp-p.6", 0x4000, 0x2000, 0xb5aa0f89 )
	ROM_LOAD( "tp-p.5", 0x6000, 0x2000, 0x56eae9ac )

	ROM_REGION( 0x10000,REGION_CPU3, 0 ) /* Z80 (sound) cpu code */
	ROM_LOAD( "tp-b.1", 0x0000, 0x2000, 0xb1c2525c )
	ROM_LOAD( "tp-s.3", 0x2000, 0x2000, 0x90c9d0b9 )
	ROM_LOAD( "tp-s.2", 0x4000, 0x2000, 0x444b6a1d )
	ROM_LOAD( "tp-s.1", 0x6000, 0x2000, 0x391097cd )

	ROM_REGION( 0x8000, REGION_GFX1, 0 ) /* */
	ROM_LOAD( "tp-b.2", 0x0000, 0x2000, 0x8cd2c917 )
	ROM_LOAD( "tp-b.3", 0x2000, 0x1000, 0xb80ef399 ) /* 0x1000 !!!!! */
	ROM_LOAD( "tp-b.4", 0x4000, 0x2000, 0x6600f306 )
	ROM_LOAD( "tp-b.5", 0x6000, 0x2000, 0x0f260bfe )

	ROM_REGION( 0x10000,REGION_GFX2, 0 ) /* */
	ROM_LOAD( "tp-c.1", 0x0000, 0x2000, 0xef573117 )
	ROM_LOAD( "tp-c.2", 0x2000, 0x2000, 0x1d29f1e6 )
	ROM_LOAD( "tp-c.3", 0x4000, 0x2000, 0x086511a7 )
	ROM_LOAD( "tp-c.4", 0x6000, 0x2000, 0x49f372ea )
	ROM_LOAD( "tp-c.5", 0x8000, 0x2000, 0x513f8777 )
	ROM_LOAD( "tp-c.6", 0xa000, 0x2000, 0x11f9752b )
	ROM_LOAD( "tp-c.7", 0xc000, 0x2000, 0xcbf093f1 )
	ROM_LOAD( "tp-c.8", 0xe000, 0x2000, 0x9f31ecb5 )

	ROM_REGION( 0x4000, REGION_GFX3, 0 ) /*  */
	ROM_LOAD( "tp-g.4", 0x0000, 0x1000, 0x99e72549 )
	ROM_LOAD( "tp-g.3", 0x1000, 0x1000, 0x1f2abec5 )
	ROM_LOAD( "tp-g.2", 0x2000, 0x1000, 0x4a7407a2 )
	ROM_LOAD( "tp-g.1", 0x3000, 0x1000, 0xf0b26c2e )

	ROM_REGION( 0x8000, REGION_GFX4, 0 ) /*  */
	ROM_LOAD( "tp-g.5", 0x0000, 0x2000, 0x27e5e6c1 )
	ROM_LOAD( "tp-g.6", 0x2000, 0x2000, 0x105cb9e4 )
	ROM_LOAD( "tp-g.7", 0x4000, 0x2000, 0x9f375b27 )
	ROM_LOAD( "tp-g.8", 0x6000, 0x2000, 0x2e619fec )

	ROM_REGION( 0x100,   REGION_PROMS, 0 ) /* color proms */
	ROM_LOAD( "16b",    0x0000, 0x0020, 0x9a12873a ) /* background palette ??? */
	ROM_LOAD( "16a",    0x0020, 0x0020, 0x90222a71 ) /* text and sprites palette */
ROM_END

/*     year  rom      parent  machine  inp   init */
GAMEX( 1984, tubep,   0,      tubep,   tubep,   0, ROT0, "Nichibutsu + Fujitek", "Tube Panic", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1984, rjammer, 0,      rjammer, rjammer, 0, ROT0, "Nichibutsu + Alice", "Roller Jammer", GAME_IMPERFECT_GRAPHICS )
