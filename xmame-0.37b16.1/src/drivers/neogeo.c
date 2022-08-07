/***************************************************************************
	M.A.M.E. Neo Geo driver presented to you by the Shin Emu Keikaku team.

	The following people have all spent probably far too much time on this:

	AVDB
	Bryan McPhail
	Fuzz
	Ernesto Corvi
	Andrew Prime


	GRAPHICAL ISSUES :

	- Effects created using the Raster Interrupt are probably not 100% correct,
	  e.g.:
	  - there is currently a kludge to get zedblade working correctly. It seems
	    to control the line IRQ2 happens on differently from the other games.
	  - trally, neodrift and tpgolf need to use the raster driver, however that
	    way full screen zoom does NOT work correctly (the video driver has kludges
		to make it work without the raster driver).
	  - garoup enables IRQ2 on Terry's stage, but with no noticeable effect. Note
	    that it is NOT enabled in 2 players mode, only vs cpu.
	  - ridhero requires 262 raster lines to work correctly. However the aodk 100
		mega shock logo works better with 261.
	  - spinmast uses IRQ2 with no noticeable effect (it seems to be always near
	    the bottom of the screen).
	  - strhoop enables IRQ2 on every scanline during attract mode, with no
	    noticeable effect.
	  - Money Idol Exchanger runs slow during the "vs. Computer" mode. Solo mode
	    works fine.
	  - Super Dodge Balls runs about as slow as Exchanger (above), and doesn't
	    even seem to register key presses, other than the coin slots and start.
		This is related to bit 15 of neo_control_16_r(), it needs to be 1 more
		often than it is now.

	- Viewpoint resets halfway through level 1. This is a bug in the asm 68k core.

	- Gururin has bad tiles all over the place (used to work ..)

	- Start of Kof94 Japan stage seems to have incorrect scrolling. rbffspec has a similar prob?

	- Bad clipping during scrolling at the sides on some games.
		(tpgolf for example)

	- Zooming is not perfect, sometimes things that are meant to be zoomed down
	  to be invisible end up being a single line of pixels
		(tophuntr how to play, kuizkof map)

	AUDIO ISSUES :

	- Sound (Music) cuts out in ncommand and ncombat
		(both these are early Alpha Denshi / ADK Games .. Sloppy Coding?)

	- Some rather bad sounding parts in a couple of Games
		(doubledr Intro Sequence, shocktro End of Intro, savagere a lot of the time...)

	- In mahretsu music should stop when you begin play (correct after a continue) *untested*

	GAMEPLAY ISSUES / LOCKUPS :

	- magdrop2 behaves strangely when P2 wins a 2 Player game (reports both as losing)

	- popbounc without a patch this locks up when sound is disabled, also for this game 'paddle'
	  conroller can be selected in the setup menus, but Mame doesn't support this.

	- ssideki2 locks up sometimes during play *not tested recently, certainly used to*

	- 2020bb apparently resets when the batter gets hit by the pitcher *not tested*

	- some games apparently crash / reset when you finish them before you get the ending *untested*

	NON-ISSUES / FIXED ISSUES :

	- Auto Animation Speed is not quite right in Many Games
		(mslug waterfalls, some bg's in samsho4, blazstar lev 2 etc.)

	- shocktro locking up at the bosses, this was fixed a long long time ago, it was due to bugs
	  in the 68k Core.

	- sound, graphic, the odd game crash & any other strange happenings in kof99p and garoup are
	  probably because these machines are prototypes, the games are therefore not finished.  There
	  are 'patched' versions of these romsets available in some locations, however these will not
	  be supported.

	OTHER MINOR THINGS :

	- 2020bb version display, the program roms contains two version numbers, the one which always
	  get displayed when running in Mame is that which would be displayed on a console.
	  This depends on location 0x46 of nvram, which should be set in the arcade version. There
	  doesn't seem to be a place in the BIOS where this location is initialized.

	- Encryption on newer games (some have program code & gfx encrypted, some just have gfx, S1
	  rom is also missing from all these later boards, although the games still appear to get
	  this data from somewhere)  kof99 and garou can only be supported at present because the
	  romsets avaliable are beta versions.  Actually we don't have any valid encrytped program
	  code to look at, but we have what we can only assume is a good dump of encrypted kof99 gfx
	  roms.

	POSSIBLE BAD DUMPS :

	- there should be a more complete list of these over at www.mametesters.com
		S1 Rom in strhoop (holes in some gfx?)
		P  Roms in kof99p (these could be hacked..the most common dump is)

	NOTES ABOUT UNSUPPORTED GAMES :

	- Ganryu (Visco / SNK, 1999) - Not Dumped / Avaliable was thought to have encrypted GFX but some
	  screenshots have showed up from NRX on a Chinese(?) Website

	- Prehistoric Isle 2 (Yumekobo / SNK, 1999) - Dump is incomplete, Encrypted GFX?

	- King of Fighters '99 (Release Version) (SNK, 1999) - Dump is Bad, Encrypted Code + GFX?

	- Garou - Mark of the Wolves (Release Version (SNK, 1999) - Not Dumped, Encrypted Code + GFX?

	- Strikers 1945 Plus (Psikyo / SNK, 1999) - Not Dumped, Encrypted GFX?

	- Metal Slug 3 (SNK, 2000) - Not Dumped, Encrypted GFX + Code, Too New?

	- King of Fighters 2000 (SNK, 2000) - Not Dumped, Encrypted GFX + Code, Too New?

	- Diggerman (???, 2000) - Not A Real Arcade Game .. Will Not Be Supported.

=============================================================================

Points to note, known and proven information deleted from this map:

	0x3000001		Dipswitches
				bit 0 : Selftest
				bit 1 : Unknown (Unused ?) \ something to do with
				bit 2 : Unknown (Unused ?) / auto repeating keys ?
				bit 3 : \
				bit 4 :  | communication setting ?
				bit 5 : /
				bit 6 : free play
				bit 7 : stop mode ?

	0x320001		bit 0 : COIN 1
				bit 1 : COIN 2
				bit 2 : SERVICE
				bit 3 : UNKNOWN
				bit 4 : UNKNOWN
				bit 5 : UNKNOWN
				bit 6 : 4990 test pulse bit.
				bit 7 : 4990 data bit.

	0x380051		4990 control write register
				bit 0: C0
				bit 1: C1
				bit 2: C2
				bit 3-7: unused.

				0x02 = shift.
				0x00 = register hold.
				0x04 = ????.
				0x03 = time read (reset register).

	0x3c000c		IRQ acknowledge

	0x380011		Backup bank select

	0x3a0001		Enable display.
	0x3a0011		Disable display

	0x3a0003		Swap in Bios (0x80 bytes vector table of BIOS)
	0x3a0013		Swap in Rom  (0x80 bytes vector table of ROM bank)

	0x3a000d		lock backup ram
	0x3a001d		unlock backup ram

	0x3a000b		set game vector table (?)  mirror ?
	0x3a001b		set bios vector table (?)  mirror ?

	0x3a000c		Unknown (ghost pilots)
	0x31001c		Unknown (ghost pilots)

	IO word read

	0x3c0002		return vidram word (pointed to by 0x3c0000)
	0x3c0006		?????.
	0x3c0008		shadow adress for 0x3c0000 (not confirmed).
	0x3c000a		shadow adress for 0x3c0002 (confirmed, see
							   Puzzle de Pon).
	IO word write

	0x3c0006		Unknown, set vblank counter (?)

	0x3c0008		shadow address for 0x3c0000 (not confirmed)
	0x3c000a		shadow address for 0x3c0002 (not confirmed)

	The Neo Geo contains an NEC 4990 Serial I/O calendar & clock.
	accesed through 0x320001, 0x380050, 0x280050 (shadow adress).
	A schematic for this device can be found on the NEC webpages.

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/pd4990a.h"
#include "cpu/z80/z80.h"


#define RASTER_LINES 262	/* almost certainly correct */
#define FIRST_VISIBLE_LINE 16
#define LAST_VISIBLE_LINE 239


extern data16_t *neogeo_vidram16;
extern data16_t *neogeo_sram16;

WRITE16_HANDLER( neogeo_sram16_lock_w );
WRITE16_HANDLER( neogeo_sram16_unlock_w );
READ16_HANDLER( neogeo_sram16_r );
WRITE16_HANDLER( neogeo_sram16_w );
void neogeo_nvram_handler(void *file,int read_or_write);

extern int	memcard_status;
READ16_HANDLER( neogeo_memcard16_r );
WRITE16_HANDLER( neogeo_memcard16_w );



/* from vidhrdw/neogeo.c */
void neogeo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void neogeo_vh_raster_partial_refresh(struct osd_bitmap *bitmap,int current_line);
void neogeo_vh_raster_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  neogeo_mvs_vh_start(void);
void neogeo_vh_stop(void);
WRITE16_HANDLER( neogeo_paletteram16_w );
READ16_HANDLER( neogeo_paletteram16_r );
WRITE16_HANDLER( neogeo_setpalbank0_16_w );
WRITE16_HANDLER( neogeo_setpalbank1_16_w );

WRITE16_HANDLER( neo_board_fix_16_w );
WRITE16_HANDLER( neo_game_fix_16_w );

WRITE16_HANDLER( neogeo_vidram16_modulo_w );
WRITE16_HANDLER( neogeo_vidram16_data_w );
WRITE16_HANDLER( neogeo_vidram16_offset_w );

READ16_HANDLER( neogeo_vidram16_data_r );
READ16_HANDLER( neogeo_vidram16_modulo_r );


/* from machine/neogeo.c */
void neogeo_init_machine(void);
void init_neogeo(void);


/******************************************************************************/

unsigned int neogeo_frame_counter;
unsigned int neogeo_frame_counter_speed=4;

/******************************************************************************/

static int irq2start=1000,irq2control;
static int current_scanline,current_rasterline,scanline_read;
static UINT32 irq2pos_value;
static int vblank_int,scanline_int;
static int irq2taken;
int neogeo_raster_enable = 1;


static void update_interrupts(void)
{
	int level = 0;

	/* determine which interrupt is active */
	if (vblank_int) level = 1;
	if (scanline_int) level = 2;

	/* either set or clear the appropriate lines */
	if (level)
		cpu_set_irq_line(0, level, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}

static WRITE16_HANDLER( neo_irqack_w )
{
	if (ACCESSING_LSB)
	{
		if (data & 4) vblank_int = 0;
		if (data & 2) scanline_int = 0;
		update_interrupts();
	}
}


static int neogeo_interrupt(void)
{
	static int fc=0;
	int line = RASTER_LINES - cpu_getiloops();

	current_rasterline = line;
	current_scanline = line - (RASTER_LINES-(LAST_VISIBLE_LINE+1));

	if (line == RASTER_LINES)	/* vblank */
	{
		current_rasterline = 0;

		/* Add a timer tick to the pd4990a */
		pd4990a_addretrace();

		/* Animation counter, 1 once per frame is too fast, every 4 seems good */
		if (!(irq2control & 0x8))
		{
			if (fc>neogeo_frame_counter_speed)
			{
				fc=0;
				neogeo_frame_counter++;
			}
			fc++;
		}

		if (irq2control & 0x10)
			usrintf_showmessage("IRQ2 enabled, need raster driver");

		/* return a standard vblank interrupt */
		vblank_int = 1;	   /* vertical blank */
	}

	update_interrupts();
	return 0;
}


static int raster_interrupt(int busy)
{
	static int fc=0;
	int line = RASTER_LINES - cpu_getiloops();
	int do_refresh = 0;

	current_rasterline = line;
	current_scanline = line - (RASTER_LINES-(LAST_VISIBLE_LINE+1));

	irq2taken = 0;

/* kludge to get zedblade working correctly without breaking everything else */
if (!strcmp(Machine->gamedrv->name,"zedblade"))
	current_scanline = line-4;/* - (RASTER_LINES-(LAST_VISIBLE_LINE+1)); */

	if (busy)
	{
		if (neogeo_raster_enable && scanline_read)
		{
			do_refresh = 1;
/*logerror("partial refresh at raster line %d (screen line %d)\n",line,current_scanline); */
			scanline_read = 0;
		}
	}

	if (irq2control & 0x10)
	{
		if (line == irq2start)
		{
/*logerror("trigger IRQ2 at raster line %d (screen line %d)\n",line,current_scanline); */
			if (!busy)
			{
				if (neogeo_raster_enable)
					do_refresh = 1;
			}

			if (irq2control & 0x80)
				irq2start += (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */

			scanline_int = 1;
			irq2taken = 1;
		}
	}

	if (line == RASTER_LINES)	/* vblank */
	{
		current_rasterline = 0;

		if (keyboard_pressed_memory(KEYCODE_F1))
		{
			neogeo_raster_enable = (neogeo_raster_enable + 1) % 3;
			usrintf_showmessage("raster effects %sabled",neogeo_raster_enable ? "en" : "dis");
		}

		if (irq2control & 0x40)
			irq2start = (irq2pos_value + 3) / 0x180;	/* ridhero gives 0x17d */
		else
			irq2start = 1000;


		/* Add a timer tick to the pd4990a */
		pd4990a_addretrace();

		/* Animation counter */
		if(!(irq2control & 0x8))
		{
			if (fc>neogeo_frame_counter_speed)
			{
				fc=0;
				neogeo_frame_counter++;
			}
			fc++;
		}

		do_refresh = 1;

		/* return a standard vblank interrupt */
/*logerror("trigger IRQ1\n"); */
		vblank_int = 1;	   /* vertical blank */
	}

	if (do_refresh && osd_skip_this_frame() == 0)
		neogeo_vh_raster_partial_refresh(Machine->scrbitmap,current_scanline);

	update_interrupts();
	return 0;
}

static int neogeo_raster_interrupt(void)
{
	return raster_interrupt(0);
}

static int neogeo_raster_interrupt_busy(void)
{
	return raster_interrupt(1);
}



static int pending_command;
static int result_code;

/* Calendar, coins + Z80 communication */
static READ16_HANDLER( timer16_r )
{
	data16_t res;
	int coinflip = pd4990a_testbit_r(0);
	int databit = pd4990a_databit_r(0);

/*	logerror("CPU %04x - Read timer\n",cpu_get_pc()); */

	res = readinputport(4) ^ (coinflip << 6) ^ (databit << 7);

	if (Machine->sample_rate)
	{
		res |= result_code << 8;
		if (pending_command) res &= 0x7fff;
	}
	else
		res |= 0x0100;

	return res;
}

static WRITE16_HANDLER( neo_z80_w )
{
	if (ACCESSING_LSB)
		return;

	soundlatch_w(0,(data>>8)&0xff);
	pending_command = 1;
	cpu_cause_interrupt(1,Z80_NMI_INT);
	/* spin for a while to let the Z80 read the command (fixes hanging sound in pspikes2) */
	cpu_spinuntil_time(TIME_IN_USEC(50));
}



int neogeo_has_trackball;
static int ts;

static WRITE16_HANDLER( trackball_select_16_w )
{
	ts = data & 1;
}

static READ16_HANDLER( controller1_16_r )
{
	data16_t res;

	if (neogeo_has_trackball)
		res = (readinputport(ts?7:0) << 8) + readinputport(3);
	else
		res = (readinputport(0) << 8) + readinputport(3);

	return res;
}
static READ16_HANDLER( controller2_16_r )
{
	data16_t res;

	res = (readinputport(1) << 8);

	return res;
}
static READ16_HANDLER( controller3_16_r )
{
	if (memcard_status==0)
		return (readinputport(2) << 8);
	else
		return ((readinputport(2) << 8)&0x8FFF);
}
static READ16_HANDLER( controller4_16_r )
{
	return readinputport(6);
}

static WRITE16_HANDLER( neo_bankswitch_16_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;


	if (memory_region_length(REGION_CPU1) <= 0x100000)
	{
logerror("warning: bankswitch to %02x but no banks available\n",data);
		return;
	}

	data = data&0x7;
	bankaddress = (data+1)*0x100000;
	if (bankaddress >= memory_region_length(REGION_CPU1))
	{
logerror("PC %06x: warning: bankswitch to empty bank %02x\n",cpu_get_pc(),data);
		bankaddress = 0x100000;
	}

	cpu_setbank(4,&RAM[bankaddress]);
}



/* TODO: Figure out how this really works! */
static READ16_HANDLER( neo_control_16_r )
{
	int line,irq_bit,res;

	/*
		The format of this very important location is:	ABBB BBBB B??? CDDD

		A I think this is: (vblank OR irq2). sdodgeb loops waiting for
		  it to be 1; zedblade heavily depends on it to work correctly (it
		  checks it in the IRQ2 handler).
		B is the video beam line. mosyougi relies solely on this to do the
		  raster effects on the title screen.
		C is definitely a PAL/NTSC flag. Evidence:
		  1) trally changes the position of the speed indicator depending on
			 it (0 = lower 1 = higher).
		  2) samsho3 sets a variable to 60 when the bit is 0 and 50 when it's 1.
			 This is obviously the video refresh rate in Hz.
		  3) samsho3 sets another variable to 256 or 307. This could be the total
			 screen height (including vblank), or close to that.
		  Some games (e.g. lstbld2, samsho3) do this (or similar):
		  bclr	  #$0, $3c000e.l
		  when the bit is set, so 3c000e (whose function is unknown) has to be
		  related
		D is a variable speed counter. In blazstar, it controls the background
		  speed in level 2.
	*/

	line = current_scanline;
	scanline_read = 1;	/* needed for raster_busy optimization */

	irq_bit = irq2taken
			|| (line < FIRST_VISIBLE_LINE) || (line > LAST_VISIBLE_LINE);

	res =	((line << 7) & 0x7f80) |				/* scanline */
			(irq_bit << 15) |						/* vblank or irq2 */
			(neogeo_frame_counter & 0x0007);		/* frame counter */

	logerror("PC %06x: neo_control_16_r (%04x)\n",cpu_get_pc(),res);

	return res;
}


/* this does much more than this, but I'm not sure exactly what */
WRITE16_HANDLER( neo_control_16_w )
{
	logerror("%06x: neo_control_16_w %04x\n",cpu_get_pc(),data);

	/* Auto-Anim Speed Control */
	neogeo_frame_counter_speed = (data >> 8) & 0xff;

/*	0x01 - 0x80 flags:

	0x01 unknown, no game sets this ?
	0x02 unknown, no game sets this ?
	0x04 unknown, kof94 sets this at some point ?

	0x08 shocktro2, stops autoanim counter

	0x10 irq2 enable, tile engine scanline irq that is triggered
	when a certain scanline is reached.

	0x20 when set, the next values written in the irq position register
	sets irq2 to happen N lines after the current one

	0x40 when set, irq position register is automatically loaded at vblank to
	set the irq2 line.

	0x80 when set, every time irq2 is triggered the irq position register is
	automatically loaded to set the next irq2 line.

	0x80 and 0x40 may be set at the same time (Viewpoint does this).
*/
	irq2control = data & 0xff;
}

static WRITE16_HANDLER( neo_irq2pos_16_w )
{
	logerror("%06x: neo_irq2pos_16_w offset %d %04x\n",cpu_get_pc(),offset,data);

	if (offset)
		irq2pos_value = (irq2pos_value & 0xffff0000) | (UINT32)data;
	else
		irq2pos_value = (irq2pos_value & 0x0000ffff) | ((UINT32)data << 16);

	if (irq2control & 0x20)
	{
/*		int line = (irq2pos_value + 3) / 0x180;	   ridhero gives 0x17d    */
		int line = (irq2pos_value + 0x3b) / 0x180;	/* turfmast goes as low as 0x145 */

		irq2start = current_rasterline + line;

		logerror("irq2start = %d, current_rasterline = %d, current_scanline = %d\n",irq2start,current_rasterline,current_scanline);
	}
}


/******************************************************************************/

static MEMORY_READ16_START( neogeo_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },			/* Rom bank 1 */
	{ 0x100000, 0x10ffff, MRA16_BANK1 },		/* Ram bank 1 */
	{ 0x200000, 0x2fffff, MRA16_BANK4 },		/* Rom bank 2 */

	{ 0x300000, 0x300001, controller1_16_r },
	{ 0x300080, 0x300081, controller4_16_r },	/* Test switch in here */
	{ 0x320000, 0x320001, timer16_r },			/* Coins, Calendar, Z80 communication */
	{ 0x340000, 0x340001, controller2_16_r },
	{ 0x380000, 0x380001, controller3_16_r },
	{ 0x3c0000, 0x3c0001, neogeo_vidram16_data_r }, /* Baseball Stars */
	{ 0x3c0002, 0x3c0003, neogeo_vidram16_data_r },
	{ 0x3c0004, 0x3c0005, neogeo_vidram16_modulo_r },

	{ 0x3c0006, 0x3c0007, neo_control_16_r },
	{ 0x3c000a, 0x3c000b, neogeo_vidram16_data_r }, /* Puzzle de Pon */

	{ 0x400000, 0x401fff, neogeo_paletteram16_r },
	{ 0x6a0000, 0x6a1fff, MRA16_RAM },
	{ 0x800000, 0x800fff, neogeo_memcard16_r }, /* memory card */
	{ 0xc00000, 0xc1ffff, MRA16_BANK3 },		/* system bios rom */
	{ 0xd00000, 0xd0ffff, neogeo_sram16_r },	/* 64k battery backed SRAM */
MEMORY_END

static MEMORY_WRITE16_START( neogeo_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },	  /* ghost pilots writes to ROM */
	{ 0x100000, 0x10ffff, MWA16_BANK1 },	/* WORK RAM */
/*	{ 0x200000, 0x200fff, whp copies ROM data here. Why? Is there RAM in the banked ROM space? */
/* trally writes to 200000-200003 as well, probably looking for a serial link */
/* both games write to 0000fe before writing to 200000. The two things could be related. */
/* sidkicks reads and writes to several addresses in this range, using this for copy */
/* protection. Custom parts instead of the banked ROMs? */
/*	{ 0x280050, 0x280051, pd4990a_control_16_w }, */
	{ 0x2ffff0, 0x2fffff, neo_bankswitch_16_w },	/* NOTE THIS CHANGE TO END AT FF !!! */
	{ 0x300000, 0x300001, watchdog_reset16_w },
	{ 0x320000, 0x320001, neo_z80_w },				/* Sound CPU */
	{ 0x380000, 0x380001, trackball_select_16_w },	/* Used by bios, unknown */
	{ 0x380030, 0x380031, MWA16_NOP },				/* Used by bios, unknown */
	{ 0x380040, 0x380041, MWA16_NOP },				/* Output leds */
	{ 0x380050, 0x380051, pd4990a_control_16_w },
	{ 0x380060, 0x380063, MWA16_NOP },				/* Used by bios, unknown */
	{ 0x3800e0, 0x3800e3, MWA16_NOP },				/* Used by bios, unknown */

	{ 0x3a0000, 0x3a0001, MWA16_NOP },
	{ 0x3a0010, 0x3a0011, MWA16_NOP },
	{ 0x3a0002, 0x3a0003, MWA16_NOP },
	{ 0x3a0012, 0x3a0013, MWA16_NOP },
	{ 0x3a000a, 0x3a000b, neo_board_fix_16_w }, /* Select board FIX char rom */
	{ 0x3a001a, 0x3a001b, neo_game_fix_16_w },	/* Select game FIX char rom */
	{ 0x3a000c, 0x3a000d, neogeo_sram16_lock_w },
	{ 0x3a001c, 0x3a001d, neogeo_sram16_unlock_w },
	{ 0x3a000e, 0x3a000f, neogeo_setpalbank1_16_w },
	{ 0x3a001e, 0x3a001f, neogeo_setpalbank0_16_w },	/* Palette banking */

	{ 0x3c0000, 0x3c0001, neogeo_vidram16_offset_w },
	{ 0x3c0002, 0x3c0003, neogeo_vidram16_data_w },
	{ 0x3c0004, 0x3c0005, neogeo_vidram16_modulo_w },

	{ 0x3c0006, 0x3c0007, neo_control_16_w },	/* IRQ2 control */
	{ 0x3c0008, 0x3c000b, neo_irq2pos_16_w },	/* IRQ2 position */
	{ 0x3c000c, 0x3c000d, neo_irqack_w },		/* IRQ acknowledge */
/*	{ 0x3c000e, 0x3c000f },    Unknown, see control_r    */

	{ 0x400000, 0x401fff, neogeo_paletteram16_w },	/* COLOR RAM BANK1 */
	{ 0x6a0000, 0x6a1fff, MWA16_RAM },	/* COLOR RAM BANK0 (used only in startup tests?) */
	{ 0x800000, 0x800fff, neogeo_memcard16_w }, 	/* mem card */
	{ 0xd00000, 0xd0ffff, neogeo_sram16_w, &neogeo_sram16 },	/* 64k battery backed SRAM */
MEMORY_END

/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK5 },
	{ 0xc000, 0xdfff, MRA_BANK6 },
	{ 0xe000, 0xefff, MRA_BANK7 },
	{ 0xf000, 0xf7ff, MRA_BANK8 },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END


static READ_HANDLER( z80_port_r )
{
	static int bank[4];


#if 0
{
	char buf[80];
	sprintf(buf,"%05x %05x %05x %05x",bank[0],bank[1],bank[2],bank[3]);
	usrintf_showmessage(buf);
}
#endif

	switch (offset & 0xff)
	{
	case 0x00:
		pending_command = 0;
		return soundlatch_r(0);
		break;

	case 0x04:
		return YM2610_status_port_0_A_r(0);
		break;

	case 0x05:
		return YM2610_read_port_0_r(0);
		break;

	case 0x06:
		return YM2610_status_port_0_B_r(0);
		break;

	case 0x08:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[3] = 0x0800 * ((offset >> 8) & 0x7f);
			cpu_setbank(8,&mem08[bank[3]]);
			return 0;
			break;
		}

	case 0x09:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[2] = 0x1000 * ((offset >> 8) & 0x3f);
			cpu_setbank(7,&mem08[bank[2]]);
			return 0;
			break;
		}

	case 0x0a:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[1] = 0x2000 * ((offset >> 8) & 0x1f);
			cpu_setbank(6,&mem08[bank[1]]);
			return 0;
			break;
		}

	case 0x0b:
		{
			UINT8 *mem08 = memory_region(REGION_CPU2);
			bank[0] = 0x4000 * ((offset >> 8) & 0x0f);
			cpu_setbank(5,&mem08[bank[0]]);
			return 0;
			break;
		}

	default:
logerror("CPU #1 PC %04x: read unmapped port %02x\n",cpu_get_pc(),offset&0xff);
		return 0;
		break;
	}
}

static WRITE_HANDLER( z80_port_w )
{
	switch (offset & 0xff)
	{
	case 0x04:
		YM2610_control_port_0_A_w(0,data);
		break;

	case 0x05:
		YM2610_data_port_0_A_w(0,data);
		break;

	case 0x06:
		YM2610_control_port_0_B_w(0,data);
		break;

	case 0x07:
		YM2610_data_port_0_B_w(0,data);
		break;

	case 0x08:
		/* NMI enable / acknowledge? (the data written doesn't matter) */
		break;

	case 0x0c:
		result_code = data;
		break;

	case 0x18:
		/* NMI disable? (the data written doesn't matter) */
		break;

	default:
logerror("CPU #1 PC %04x: write %02x to unmapped port %02x\n",cpu_get_pc(),data,offset&0xff);
		break;
	}
}

static PORT_READ_START( neo_readio )
	{ 0x0000, 0xffff, z80_port_r },
PORT_END

static PORT_WRITE_START( neo_writeio )
	{ 0x0000, 0xffff, z80_port_w },
PORT_END

/******************************************************************************/

INPUT_PORTS_START( neogeo )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "SELECT 1",KEYCODE_6, IP_JOY_NONE ) /* Player 1 Select */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "SELECT 2",KEYCODE_7, IP_JOY_NONE ) /* Player 2 Select */
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* used, affects the values stored at location 0x47 of nvram */

	/* Fake  IN 5 */
	PORT_START
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
/*	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" ) */
/*	PORT_DIPSETTING(	0x00,"Home" ) */
/*	PORT_DIPSETTING(	0x04,"Arcade" ) */

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* used, affects the values stored at location 0x47 of nvram */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( irrmaze )
	PORT_START		/* IN0 multiplexed */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_X | IPF_REVERSE, 10, 20, 0, 0 )

	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START		/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "SELECT 1",KEYCODE_6, IP_JOY_NONE ) /* Player 1 Select */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "SELECT 2",KEYCODE_7, IP_JOY_NONE ) /* Player 2 Select */
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card inserted */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* memory card write protection */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin Chutes?" )
	PORT_DIPSETTING(	0x00, "1?" )
	PORT_DIPSETTING(	0x02, "2?" )
	PORT_DIPNAME( 0x04, 0x04, "Autofire (in some games)" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting" )
	PORT_DIPSETTING(	0x38, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x30, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )

	/* Fake  IN 5 */
	PORT_START
	PORT_DIPNAME( 0x03, 0x02,"Territory" )
	PORT_DIPSETTING(	0x00,"Japan" )
	PORT_DIPSETTING(	0x01,"USA" )
	PORT_DIPSETTING(	0x02,"Europe" )
/*	PORT_DIPNAME( 0x04, 0x04,"Machine Mode" ) */
/*	PORT_DIPSETTING(	0x00,"Home" ) */
/*	PORT_DIPSETTING(	0x04,"Arcade" ) */

	PORT_START		/* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* This bit is used.. */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* IN0 multiplexed */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_Y | IPF_REVERSE, 10, 20, 0, 0 )
INPUT_PORTS_END


/******************************************************************************/

/* character layout (same for all games) */
static struct GfxLayout charlayout =	/* All games */
{
	8,8,			/* 8 x 8 chars */
	4096,			/* 4096 in total */
	4,				/* 4 bits per pixel */
	{ 0, 1, 2, 3 },    /* planes are packed in a nibble */
	{ 33*4, 32*4, 49*4, 48*4, 1*4, 0*4, 17*4, 16*4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	/* 32 bytes per char */
};

/* Placeholder and also reminder of how this graphic format is put together */
static struct GfxLayout dummy_mvs_tilelayout =
{
	16,16,	 /* 16*16 sprites */
	20,
	4,
	{ 3*8, 1*8, 2*8, 0*8 }, 	/* plane offset */
	{ 64*8+7, 64*8+6, 64*8+5, 64*8+4, 64*8+3, 64*8+2, 64*8+1, 64*8+0,
			7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8	 /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_mvs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &charlayout, 0, 16 },
	{ REGION_GFX1, 0x020000, &charlayout, 0, 16 },
	{ REGION_GFX1, 0x000000, &dummy_mvs_tilelayout, 0, 256 },  /* Placeholder */
	{ -1 } /* end of array */
};

/******************************************************************************/

static void neogeo_sound_irq( int irq )
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

struct YM2610interface neogeo_ym2610_interface =
{
	1,
	8000000,
	{ MIXERG(15,MIXER_GAIN_4x,MIXER_PAN_CENTER) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ neogeo_sound_irq },
	{ REGION_SOUND2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(75,MIXER_PAN_LEFT,75,MIXER_PAN_RIGHT) }
};

/******************************************************************************/

static struct MachineDriver machine_driver_neogeo =
{
	{
		{
			CPU_M68000,
			12000000,
			neogeo_readmem,neogeo_writemem,0,0,
			neogeo_interrupt,RASTER_LINES
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU | CPU_16BIT_PORT,
			6000000,
			sound_readmem,sound_writemem,neo_readio,neo_writeio,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	neogeo_init_machine,
	40*8, 32*8, { 1*8, 39*8-1, 2*8, 30*8-1 },
	neogeo_mvs_gfxdecodeinfo,
	4096,4096,
	0,

	/* please don't put VIDEO_SUPPRTS_16BIT in all games. It is stupid, because */
	/* most games don't need it. Only put it in games that use more than 256 colors */
	/* at the same time (and let the MAME team know about it) */
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	neogeo_mvs_vh_start,
	neogeo_vh_stop,
	neogeo_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&neogeo_ym2610_interface,
		},
	},

	neogeo_nvram_handler
};

static struct MachineDriver machine_driver_raster =
{
	{
		{
			CPU_M68000,
			12000000,
			neogeo_readmem,neogeo_writemem,0,0,
			neogeo_raster_interrupt,RASTER_LINES
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU | CPU_16BIT_PORT,
			6000000,
			sound_readmem,sound_writemem,neo_readio,neo_writeio,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	neogeo_init_machine,
	40*8, 32*8, { 1*8, 39*8-1, FIRST_VISIBLE_LINE, LAST_VISIBLE_LINE },
	neogeo_mvs_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_RGB_DIRECT,
	0,
	neogeo_mvs_vh_start,
	neogeo_vh_stop,
	neogeo_vh_raster_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&neogeo_ym2610_interface,
		},
	},

	neogeo_nvram_handler
};

static struct MachineDriver machine_driver_raster_busy =
{
	{
		{
			CPU_M68000,
			12000000,
			neogeo_readmem,neogeo_writemem,0,0,
			neogeo_raster_interrupt_busy,RASTER_LINES
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU | CPU_16BIT_PORT,
			6000000,
			sound_readmem,sound_writemem,neo_readio,neo_writeio,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	neogeo_init_machine,
	40*8, 32*8, { 1*8, 39*8-1, FIRST_VISIBLE_LINE, LAST_VISIBLE_LINE },
	neogeo_mvs_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_RGB_DIRECT,
	0,
	neogeo_mvs_vh_start,
	neogeo_vh_stop,
	neogeo_vh_raster_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&neogeo_ym2610_interface,
		},
	},

	neogeo_nvram_handler
};

/******************************************************************************/

#define NEO_BIOS_SOUND_256K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION( 0x40000, REGION_CPU2, 0 ) \
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x40000, sum ) /* so overwrite it with the real thing */

#define NEO_BIOS_SOUND_128K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION( 0x40000, REGION_CPU2, 0 ) \
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x20000, sum ) /* so overwrite it with the real thing */

#define NEO_BIOS_SOUND_64K(name,sum) \
	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION( 0x40000, REGION_CPU2, 0 ) \
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */ \
	ROM_LOAD( name, 		0x00000, 0x10000, sum ) /* so overwrite it with the real thing */

#define NO_DELTAT_REGION

#define NEO_SFIX_128K(name,sum) \
	ROM_REGION( 0x40000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x20000, sum ) \
	ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

#define NEO_SFIX_64K(name,sum) \
	ROM_REGION( 0x40000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x10000, sum ) \
	ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

#define NEO_SFIX_32K(name,sum) \
	ROM_REGION( 0x40000, REGION_GFX1, 0 ) \
	ROM_LOAD( name, 		  0x000000, 0x08000, sum ) \
	ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

/******************************************************************************/

ROM_START( nam1975 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "nam_p1.rom", 0x000000, 0x080000, 0xcc9fc951 )

	NEO_SFIX_64K( "nam_s1.rom", 0x8ded55a5 )

	NEO_BIOS_SOUND_64K( "nam_m1.rom", 0xcd088502 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "nam_v11.rom", 0x000000, 0x080000, 0xa7c3d5e5 )

	ROM_REGION( 0x180000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "nam_v21.rom", 0x000000, 0x080000, 0x55e670b3 )
	ROM_LOAD( "nam_v22.rom", 0x080000, 0x080000, 0xab0d8368 )
	ROM_LOAD( "nam_v23.rom", 0x100000, 0x080000, 0xdf468e28 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "nam_c1.rom", 0x000000, 0x80000, 0x32ea98e1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "nam_c2.rom", 0x000001, 0x80000, 0xcbc4064c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "nam_c3.rom", 0x100000, 0x80000, 0x0151054c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "nam_c4.rom", 0x100001, 0x80000, 0x0a32570d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "nam_c5.rom", 0x200000, 0x80000, 0x90b74cc2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "nam_c6.rom", 0x200001, 0x80000, 0xe62bed58 ) /* Plane 2,3 */
ROM_END

ROM_START( bstars )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "bpro_p1.rom", 0x000000, 0x080000, 0x3bc7790e )

	NEO_SFIX_128K( "bpro_s1.rom", 0x1a7fd0c6 )

	NEO_BIOS_SOUND_64K( "bpro_m1.rom", 0x79a8f4c2 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "bpro_v11.rom", 0x000000, 0x080000, 0xb7b925bd )
	ROM_LOAD( "bpro_v12.rom", 0x080000, 0x080000, 0x329f26fc )
	ROM_LOAD( "bpro_v13.rom", 0x100000, 0x080000, 0x0c39f3c8 )
	ROM_LOAD( "bpro_v14.rom", 0x180000, 0x080000, 0xc7e11c38 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "bpro_v21.rom", 0x000000, 0x080000, 0x04a733d1 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "bpro_c1.rom", 0x000000, 0x080000, 0xaaff2a45 )
	ROM_LOAD16_BYTE( "bpro_c2.rom", 0x000001, 0x080000, 0x3ba0f7e4 )
	ROM_LOAD16_BYTE( "bpro_c3.rom", 0x100000, 0x080000, 0x96f0fdfa )
	ROM_LOAD16_BYTE( "bpro_c4.rom", 0x100001, 0x080000, 0x5fd87f2f )
	ROM_LOAD16_BYTE( "bpro_c5.rom", 0x200000, 0x080000, 0x807ed83b )
	ROM_LOAD16_BYTE( "bpro_c6.rom", 0x200001, 0x080000, 0x5a3cad41 )
ROM_END

ROM_START( tpgolf )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "topg_p1.rom", 0x000000, 0x080000, 0xf75549ba )
	ROM_LOAD16_WORD_SWAP( "topg_p2.rom", 0x080000, 0x080000, 0xb7809a8f )

	NEO_SFIX_128K( "topg_s1.rom", 0x7b3eb9b1 )

	NEO_BIOS_SOUND_64K( "topg_m1.rom", 0x7851d0d9 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "topg_v11.rom", 0x000000, 0x080000, 0xff97f1cb )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "topg_v21.rom", 0x000000, 0x080000, 0xd34960c6 )
	ROM_LOAD( "topg_v22.rom", 0x080000, 0x080000, 0x9a5f58d4 )
	ROM_LOAD( "topg_v23.rom", 0x100000, 0x080000, 0x30f53e54 )
	ROM_LOAD( "topg_v24.rom", 0x180000, 0x080000, 0x5ba0f501 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "topg_c1.rom", 0x000000, 0x80000, 0x0315fbaf )
	ROM_LOAD16_BYTE( "topg_c2.rom", 0x000001, 0x80000, 0xb4c15d59 )
	ROM_LOAD16_BYTE( "topg_c3.rom", 0x100000, 0x80000, 0xb09f1612 )
	ROM_LOAD16_BYTE( "topg_c4.rom", 0x100001, 0x80000, 0x150ea7a1 )
	ROM_LOAD16_BYTE( "topg_c5.rom", 0x200000, 0x80000, 0x9a7146da )
	ROM_LOAD16_BYTE( "topg_c6.rom", 0x200001, 0x80000, 0x1e63411a )
	ROM_LOAD16_BYTE( "topg_c7.rom", 0x300000, 0x80000, 0x2886710c )
	ROM_LOAD16_BYTE( "topg_c8.rom", 0x300001, 0x80000, 0x422af22d )
ROM_END

ROM_START( mahretsu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "maj_p1.rom", 0x000000, 0x080000, 0xfc6f53db )

	NEO_SFIX_64K( "maj_s1.rom", 0xb0d16529 )

	NEO_BIOS_SOUND_64K( "maj_m1.rom", 0x37965a73 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "maj_v1.rom", 0x000000, 0x080000, 0xb2fb2153 )
	ROM_LOAD( "maj_v2.rom", 0x080000, 0x080000, 0x8503317b )

	ROM_REGION( 0x180000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "maj_v3.rom", 0x000000, 0x080000, 0x4999fb27 )
	ROM_LOAD( "maj_v4.rom", 0x080000, 0x080000, 0x776fa2a2 )
	ROM_LOAD( "maj_v5.rom", 0x100000, 0x080000, 0xb3e7eeea )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "maj_c1.rom", 0x000000, 0x80000, 0xf1ae16bc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "maj_c2.rom", 0x000001, 0x80000, 0xbdc13520 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "maj_c3.rom", 0x100000, 0x80000, 0x9c571a37 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "maj_c4.rom", 0x100001, 0x80000, 0x7e81cb29 ) /* Plane 2,3 */
ROM_END

ROM_START( maglord )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "magl_p1.rom", 0x000000, 0x080000, 0xbd0a492d )

	NEO_SFIX_128K( "magl_s1.rom", 0x1c5369a2 )

	NEO_BIOS_SOUND_64K( "magl_m1.rom", 0x91ee1f73 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "magl_v11.rom", 0x000000, 0x080000, 0xcc0455fd )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "magl_v21.rom", 0x000000, 0x080000, 0xf94ab5b7 )
	ROM_LOAD( "magl_v22.rom", 0x080000, 0x080000, 0x232cfd04 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "magl_c1.rom", 0x000000, 0x80000, 0x806aee34 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "magl_c2.rom", 0x000001, 0x80000, 0x34aa9a86 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "magl_c3.rom", 0x100000, 0x80000, 0xc4c2b926 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "magl_c4.rom", 0x100001, 0x80000, 0x9c46dcf4 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "magl_c5.rom", 0x200000, 0x80000, 0x69086dec ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "magl_c6.rom", 0x200001, 0x80000, 0xab7ac142 ) /* Plane 2,3 */
ROM_END

ROM_START( maglordh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "maglh_p1.rom", 0x000000, 0x080000, 0x599043c5 )

	NEO_SFIX_128K( "magl_s1.rom", 0x1c5369a2 )

	NEO_BIOS_SOUND_64K( "magl_m1.rom", 0x91ee1f73 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "magl_v11.rom", 0x000000, 0x080000, 0xcc0455fd )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "magl_v21.rom", 0x000000, 0x080000, 0xf94ab5b7 )
	ROM_LOAD( "magl_v22.rom", 0x080000, 0x080000, 0x232cfd04 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "magl_c1.rom", 0x000000, 0x80000, 0x806aee34 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "magl_c2.rom", 0x000001, 0x80000, 0x34aa9a86 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "magl_c3.rom", 0x100000, 0x80000, 0xc4c2b926 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "magl_c4.rom", 0x100001, 0x80000, 0x9c46dcf4 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "magl_c5.rom", 0x200000, 0x80000, 0x69086dec ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "magl_c6.rom", 0x200001, 0x80000, 0xab7ac142 ) /* Plane 2,3 */
ROM_END

ROM_START( ridhero )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ridh_p1.rom", 0x000000, 0x080000, 0xd4aaf597 )

	NEO_SFIX_64K( "ridh_s1.rom", 0x197d1a28 )

	NEO_BIOS_SOUND_128K( "ridh_m1.rom", 0xf0b6425d )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ridh_v11.rom", 0x000000, 0x080000, 0xcdf74a42 )
	ROM_LOAD( "ridh_v12.rom", 0x080000, 0x080000, 0xe2fd2371 )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ridh_v21.rom", 0x000000, 0x080000, 0x94092bce )
	ROM_LOAD( "ridh_v22.rom", 0x080000, 0x080000, 0x4e2cd7c3 )
	ROM_LOAD( "ridh_v23.rom", 0x100000, 0x080000, 0x069c71ed )
	ROM_LOAD( "ridh_v24.rom", 0x180000, 0x080000, 0x89fbb825 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ridh_c1.rom", 0x000000, 0x080000, 0x4a5c7f78 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ridh_c2.rom", 0x000001, 0x080000, 0xe0b70ece ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ridh_c3.rom", 0x100000, 0x080000, 0x8acff765 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ridh_c4.rom", 0x100001, 0x080000, 0x205e3208 ) /* Plane 2,3 */
ROM_END

ROM_START( ridheroh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ridhh_p1.rom", 0x000000, 0x080000, 0x52445646 )

	NEO_SFIX_64K( "ridh_s1.rom", 0x197d1a28 )

	NEO_BIOS_SOUND_128K( "ridh_m1.rom", 0xf0b6425d )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ridh_v11.rom", 0x000000, 0x080000, 0xcdf74a42 )
	ROM_LOAD( "ridh_v12.rom", 0x080000, 0x080000, 0xe2fd2371 )

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ridh_v21.rom", 0x000000, 0x080000, 0x94092bce )
	ROM_LOAD( "ridh_v22.rom", 0x080000, 0x080000, 0x4e2cd7c3 )
	ROM_LOAD( "ridh_v23.rom", 0x100000, 0x080000, 0x069c71ed )
	ROM_LOAD( "ridh_v24.rom", 0x180000, 0x080000, 0x89fbb825 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ridh_c1.rom", 0x000000, 0x080000, 0x4a5c7f78 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ridh_c2.rom", 0x000001, 0x080000, 0xe0b70ece ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ridh_c3.rom", 0x100000, 0x080000, 0x8acff765 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ridh_c4.rom", 0x100001, 0x080000, 0x205e3208 ) /* Plane 2,3 */
ROM_END

ROM_START( alpham2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "lstg_p1.rom", 0x000000, 0x100000, 0x7b0ebe08 )

	NEO_SFIX_128K( "lstg_s1.rom", 0x85ec9acf )

	NEO_BIOS_SOUND_128K( "lstg_m1.rom", 0xf23d3076 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "lstg_v11.rom", 0x000000, 0x100000, 0xcd5db931 )
	ROM_LOAD( "lstg_v12.rom", 0x100000, 0x100000, 0x63e9b574 )

	ROM_REGION( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "lstg_v21.rom", 0x000000, 0x100000, 0xff7ebf79 )
	ROM_LOAD( "lstg_v22.rom", 0x080000, 0x100000, 0xf2028490 )
	ROM_LOAD( "lstg_v23.rom", 0x100000, 0x100000, 0x2e4f1e48 )
	ROM_LOAD( "lstg_v24.rom", 0x180000, 0x100000, 0x658ee845 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "lstg_c1.rom", 0x000000, 0x100000, 0x8fba8ff3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "lstg_c2.rom", 0x000001, 0x100000, 0x4dad2945 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "lstg_c3.rom", 0x200000, 0x080000, 0x68c2994e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "lstg_c4.rom", 0x200001, 0x080000, 0x7d588349 ) /* Plane 2,3 */
ROM_END

ROM_START( ncombat )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ncom_p1.rom", 0x000000, 0x080000, 0xb45fcfbf )

	NEO_SFIX_128K( "ncom_s1.rom", 0xd49afee8 )

	NEO_BIOS_SOUND_128K( "ncom_m1.rom", 0xb5819863 )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ncom_v11.rom", 0x000000, 0x080000, 0xcf32a59c )
	ROM_LOAD( "ncom_v12.rom", 0x080000, 0x080000, 0x7b3588b7 )
	ROM_LOAD( "ncom_v13.rom", 0x100000, 0x080000, 0x505a01b5 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ncom_v21.rom", 0x000000, 0x080000, 0x365f9011 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ncom_c1.rom", 0x000000, 0x80000, 0x33cc838e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ncom_c2.rom", 0x000001, 0x80000, 0x26877feb ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ncom_c3.rom", 0x100000, 0x80000, 0x3b60a05d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ncom_c4.rom", 0x100001, 0x80000, 0x39c2d039 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ncom_c5.rom", 0x200000, 0x80000, 0x67a4344e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ncom_c6.rom", 0x200001, 0x80000, 0x2eca8b19 ) /* Plane 2,3 */
ROM_END

ROM_START( cyberlip )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "cybl_p1.rom", 0x000000, 0x080000, 0x69a6b42d )

	NEO_SFIX_128K( "cybl_s1.rom", 0x79a35264 )

	NEO_BIOS_SOUND_64K( "cybl_m1.rom", 0x47980d3a )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "cybl_v11.rom", 0x000000, 0x080000, 0x90224d22 )
	ROM_LOAD( "cybl_v12.rom", 0x080000, 0x080000, 0xa0cf1834 )
	ROM_LOAD( "cybl_v13.rom", 0x100000, 0x080000, 0xae38bc84 )
	ROM_LOAD( "cybl_v14.rom", 0x180000, 0x080000, 0x70899bd2 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "cybl_v21.rom", 0x000000, 0x080000, 0x586f4cb2 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "cybl_c1.rom", 0x000000, 0x80000, 0x8bba5113 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "cybl_c2.rom", 0x000001, 0x80000, 0xcbf66432 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "cybl_c3.rom", 0x100000, 0x80000, 0xe4f86efc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "cybl_c4.rom", 0x100001, 0x80000, 0xf7be4674 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "cybl_c5.rom", 0x200000, 0x80000, 0xe8076da0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "cybl_c6.rom", 0x200001, 0x80000, 0xc495c567 ) /* Plane 2,3 */
ROM_END

ROM_START( superspy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sspy_p1.rom", 0x000000, 0x080000, 0xc7f944b5 )
	ROM_LOAD16_WORD_SWAP( "sspy_p2.rom", 0x080000, 0x020000, 0x811a4faf )

	NEO_SFIX_128K( "sspy_s1.rom", 0xec5fdb96 )

	NEO_BIOS_SOUND_128K( "sspy_m1.rom", 0xd59d5d12 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sspy_v11.rom", 0x000000, 0x100000, 0x5c674d5c )
	ROM_LOAD( "sspy_v12.rom", 0x100000, 0x100000, 0x7df8898b )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sspy_v21.rom", 0x000000, 0x100000, 0x1ebe94c7 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sspy_c1.rom", 0x000000, 0x100000, 0xcae7be57 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sspy_c2.rom", 0x000001, 0x100000, 0x9e29d986 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "sspy_c3.rom", 0x200000, 0x100000, 0x14832ff2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sspy_c4.rom", 0x200001, 0x100000, 0xb7f63162 ) /* Plane 2,3 */
ROM_END

ROM_START( mutnat )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mnat_p1.rom", 0x000000, 0x080000, 0x6f1699c8 )

	NEO_SFIX_128K( "mnat_s1.rom", 0x99419733 )

	NEO_BIOS_SOUND_128K( "mnat_m1.rom", 0xb6683092 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "mnat_v1.rom", 0x000000, 0x100000, 0x25419296 )
	ROM_LOAD( "mnat_v2.rom", 0x100000, 0x100000, 0x0de53d5e )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mnat_c1.rom", 0x000000, 0x100000, 0x5e4381bf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "mnat_c2.rom", 0x000001, 0x100000, 0x69ba4e18 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "mnat_c3.rom", 0x200000, 0x100000, 0x890327d5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "mnat_c4.rom", 0x200001, 0x100000, 0xe4002651 ) /* Plane 2,3 */
ROM_END

ROM_START( kotm )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kotm_p1.rom", 0x000000, 0x080000, 0x1b818731 )
	ROM_LOAD16_WORD_SWAP( "kotm_p2.rom", 0x080000, 0x020000, 0x12afdc2b )

	NEO_SFIX_128K( "kotm_s1.rom", 0x1a2eeeb3 )

	NEO_BIOS_SOUND_128K( "kotm_m1.rom", 0x0296abcb )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kotm_v1.rom", 0x000000, 0x100000, 0x86c0a502 )
	ROM_LOAD( "kotm_v2.rom", 0x100000, 0x100000, 0x5bc23ec5 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "kotm_c1.rom", 0x000000, 0x100000, 0x71471c25 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kotm_c2.rom", 0x000001, 0x100000, 0x320db048 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kotm_c3.rom", 0x200000, 0x100000, 0x98de7995 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kotm_c4.rom", 0x200001, 0x100000, 0x070506e2 ) /* Plane 2,3 */
ROM_END

ROM_START( sengoku )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sngku_p1.rom", 0x000000, 0x080000, 0xf8a63983 )
	ROM_LOAD16_WORD_SWAP( "sngku_p2.rom", 0x080000, 0x020000, 0x3024bbb3 )

	NEO_SFIX_128K( "sngku_s1.rom", 0xb246204d )

	NEO_BIOS_SOUND_128K( "sngku_m1.rom", 0x9b4f34c6 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sngku_v1.rom", 0x000000, 0x100000, 0x23663295 )
	ROM_LOAD( "sngku_v2.rom", 0x100000, 0x100000, 0xf61e6765 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sngku_c1.rom", 0x000000, 0x100000, 0xb4eb82a1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sngku_c2.rom", 0x000001, 0x100000, 0xd55c550d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "sngku_c3.rom", 0x200000, 0x100000, 0xed51ef65 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sngku_c4.rom", 0x200001, 0x100000, 0xf4f3c9cb ) /* Plane 2,3 */
ROM_END

ROM_START( sengokh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sngkh_p1.rom", 0x000000, 0x080000, 0x33eccae0 )
	ROM_LOAD16_WORD_SWAP( "sngku_p2.rom", 0x080000, 0x020000, 0x3024bbb3 )

	NEO_SFIX_128K( "sngku_s1.rom", 0xb246204d )

	NEO_BIOS_SOUND_128K( "sngku_m1.rom", 0x9b4f34c6 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sngku_v1.rom", 0x000000, 0x100000, 0x23663295 )
	ROM_LOAD( "sngku_v2.rom", 0x100000, 0x100000, 0xf61e6765 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sngku_c1.rom", 0x000000, 0x100000, 0xb4eb82a1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sngku_c2.rom", 0x000001, 0x100000, 0xd55c550d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "sngku_c3.rom", 0x200000, 0x100000, 0xed51ef65 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sngku_c4.rom", 0x200001, 0x100000, 0xf4f3c9cb ) /* Plane 2,3 */
ROM_END

ROM_START( burningf )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "burnf_p1.rom", 0x000000, 0x080000, 0x4092c8db )

	NEO_SFIX_128K( "burnf_s1.rom", 0x6799ea0d )

	NEO_BIOS_SOUND_128K( "burnf_m1.rom", 0x0c939ee2 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "burnf_v1.rom", 0x000000, 0x100000, 0x508c9ffc )
	ROM_LOAD( "burnf_v2.rom", 0x100000, 0x100000, 0x854ef277 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "burnf_c1.rom", 0x000000, 0x100000, 0x25a25e9b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "burnf_c2.rom", 0x000001, 0x100000, 0xd4378876 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "burnf_c3.rom", 0x200000, 0x100000, 0x862b60da ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "burnf_c4.rom", 0x200001, 0x100000, 0xe2e0aff7 ) /* Plane 2,3 */
ROM_END

ROM_START( burningh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "burnh_p1.rom", 0x000000, 0x080000, 0xddffcbf4 )

	NEO_SFIX_128K( "burnf_s1.rom", 0x6799ea0d )

	NEO_BIOS_SOUND_128K( "burnf_m1.rom", 0x0c939ee2 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "burnf_v1.rom", 0x000000, 0x100000, 0x508c9ffc )
	ROM_LOAD( "burnf_v2.rom", 0x100000, 0x100000, 0x854ef277 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "burnf_c1.rom", 0x000000, 0x100000, 0x25a25e9b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "burnf_c2.rom", 0x000001, 0x100000, 0xd4378876 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "burnf_c3.rom", 0x200000, 0x100000, 0x862b60da ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "burnf_c4.rom", 0x200001, 0x100000, 0xe2e0aff7 ) /* Plane 2,3 */
ROM_END

ROM_START( lbowling )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "019-p1.bin", 0x000000, 0x080000, 0xa2de8445 )

	NEO_SFIX_128K( "019-s1.bin", 0x5fcdc0ed )

	NEO_BIOS_SOUND_128K( "019-m1.bin", 0x589d7f25 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "019-v11.bin", 0x000000, 0x080000, 0x0fb74872 )
	ROM_LOAD( "019-v12.bin", 0x080000, 0x080000, 0x029faa57 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "019-v21.bin", 0x000000, 0x080000, 0x2efd5ada )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "019-c1.bin", 0x000000, 0x080000, 0x4ccdef18 )
	ROM_LOAD16_BYTE( "019-c2.bin", 0x000001, 0x080000, 0xd4dd0802 )
ROM_END

ROM_START( gpilots )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ghst_p1.rom", 0x000000, 0x080000, 0xe6f2fe64 )
	ROM_LOAD16_WORD_SWAP( "ghst_p2.rom", 0x080000, 0x020000, 0xedcb22ac )

	NEO_SFIX_128K( "ghst_s1.rom", 0xa6d83d53 )

	NEO_BIOS_SOUND_128K( "ghst_m1.rom", 0x48409377 )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ghst_v11.rom", 0x000000, 0x100000, 0x1b526c8b )
	ROM_LOAD( "ghst_v12.rom", 0x100000, 0x080000, 0x4a9e6f03 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ghst_v21.rom", 0x000000, 0x080000, 0x7abf113d )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ghst_c1.rom", 0x000000, 0x100000, 0xbd6fe78e )
	ROM_LOAD16_BYTE( "ghst_c2.rom", 0x000001, 0x100000, 0x5f4a925c )
	ROM_LOAD16_BYTE( "ghst_c3.rom", 0x200000, 0x100000, 0xd1e42fd0 )
	ROM_LOAD16_BYTE( "ghst_c4.rom", 0x200001, 0x100000, 0xedde439b )
ROM_END

ROM_START( joyjoy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "joy_p1.rom", 0x000000, 0x080000, 0x39c3478f )

	NEO_SFIX_128K( "joy_s1.rom", 0x6956d778 )

	NEO_BIOS_SOUND_64K( "joy_m1.rom", 0x058683ec )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "joy_v1.rom", 0x000000, 0x080000, 0x66c1e5c4 )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "joy_v2.rom", 0x000000, 0x080000, 0x8ed20a86 )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "joy_c1.rom", 0x000000, 0x080000, 0x509250ec )
	ROM_LOAD16_BYTE( "joy_c2.rom", 0x000001, 0x080000, 0x09ed5258 )
ROM_END

ROM_START( bjourney )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "bj-p1.rom", 0x000000, 0x100000, 0x6a2f6d4a )

	NEO_SFIX_128K( "bj-s1.rom", 0x843c3624 )

	NEO_BIOS_SOUND_64K( "bj-m1.rom",  0xa9e30496 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "bj-v11.rom", 0x000000, 0x100000, 0x2cb4ad91 )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "bj-v22.rom", 0x000000, 0x100000, 0x65a54d13 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "bj-c1.rom", 0x000000, 0x100000, 0x4d47a48c )
	ROM_LOAD16_BYTE( "bj-c2.rom", 0x000001, 0x100000, 0xe8c1491a )
	ROM_LOAD16_BYTE( "bj-c3.rom", 0x200000, 0x080000, 0x66e69753 )
	ROM_LOAD16_BYTE( "bj-c4.rom", 0x200001, 0x080000, 0x71bfd48a )
ROM_END

ROM_START( quizdais )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "quizd_p1.rom", 0x000000, 0x100000, 0xc488fda3 )

	NEO_SFIX_128K( "quizd_s1.rom", 0xac31818a )

	NEO_BIOS_SOUND_128K( "quizd_m1.rom", 0x2a2105e0 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "quizd_v1.rom", 0x000000, 0x100000, 0xa53e5bd3 )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "quizd_c1.rom", 0x000000, 0x100000, 0x2999535a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "quizd_c2.rom", 0x000001, 0x100000, 0x876a99e6 ) /* Plane 2,3 */
ROM_END

ROM_START( lresort )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "lr_p1.rom", 0x000000, 0x080000, 0x89c4ab97 )

	NEO_SFIX_128K( "lr_s1.rom", 0x5cef5cc6 )

	NEO_BIOS_SOUND_128K( "lr_m1.rom", 0xcec19742 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "lr_v1.rom", 0x000000, 0x100000, 0xefdfa063 )
	ROM_LOAD( "lr_v2.rom", 0x100000, 0x100000, 0x3c7997c0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "lr_c1.rom", 0x000000, 0x100000, 0x3617c2dc )
	ROM_LOAD16_BYTE( "lr_c2.rom", 0x000001, 0x100000, 0x3f0a7fd8 )
	ROM_LOAD16_BYTE( "lr_c3.rom", 0x200000, 0x080000, 0xe9f745f8 )
	ROM_LOAD16_BYTE( "lr_c4.rom", 0x200001, 0x080000, 0x7382fefb )
ROM_END

ROM_START( eightman )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "8man_p1.rom", 0x000000, 0x080000, 0x43344cb0 )

	NEO_SFIX_128K( "8man_s1.rom", 0xa402202b )

	NEO_BIOS_SOUND_128K( "8man_m1.rom", 0x9927034c )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "8man_v1.rom", 0x000000, 0x100000, 0x4558558a )
	ROM_LOAD( "8man_v2.rom", 0x100000, 0x100000, 0xc5e052e9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "8man_c1.rom", 0x000000, 0x100000, 0x555e16a4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "8man_c2.rom", 0x000001, 0x100000, 0xe1ee51c3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "8man_c3.rom", 0x200000, 0x080000, 0x0923d5b0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "8man_c4.rom", 0x200001, 0x080000, 0xe3eca67b ) /* Plane 2,3 */
ROM_END

ROM_START( minasan )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "027-p1.bin", 0x000000, 0x080000, 0xc8381327 )

	NEO_SFIX_128K( "027-s1.bin", 0xe5824baa )

	NEO_BIOS_SOUND_128K( "027-m1.bin", 0xadd5a226 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "027-v11.bin", 0x000000, 0x100000, 0x59ad4459 )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "027-v21.bin", 0x000000, 0x100000, 0xdf5b4eeb )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "027-c1.bin", 0x000000, 0x100000, 0xd0086f94 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "027-c2.bin", 0x000001, 0x100000, 0xda61f5a6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "027-c3.bin", 0x200000, 0x100000, 0x08df1228 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "027-c4.bin", 0x200001, 0x100000, 0x54e87696 ) /* Plane 2,3 */
ROM_END

ROM_START( legendos )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "joe_p1.rom", 0x000000, 0x080000, 0x9d563f19 )

	NEO_SFIX_128K( "joe_s1.rom",  0xbcd502f0 )

	NEO_BIOS_SOUND_64K( "joe_m1.rom", 0x909d4ed9 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "joe_v1.rom", 0x000000, 0x100000, 0x85065452 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "joe_c1.rom", 0x000000, 0x100000, 0x2f5ab875 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "joe_c2.rom", 0x000001, 0x100000, 0x318b2711 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "joe_c3.rom", 0x200000, 0x100000, 0x6bc52cb2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "joe_c4.rom", 0x200001, 0x100000, 0x37ef298c ) /* Plane 2,3 */
ROM_END

ROM_START( 2020bb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "2020_p1.rom", 0x000000, 0x080000, 0xd396c9cb )

	NEO_SFIX_128K( "2020_s1.rom", 0x7015b8fc )

	NEO_BIOS_SOUND_128K( "2020_m1.rom", 0x4cf466ec )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "2020_v1.rom", 0x000000, 0x100000, 0xd4ca364e )
	ROM_LOAD( "2020_v2.rom", 0x100000, 0x100000, 0x54994455 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "2020_c1.rom", 0x000000, 0x100000, 0x4f5e19bd ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "2020_c2.rom", 0x000001, 0x100000, 0xd6314bf0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "2020_c3.rom", 0x200000, 0x080000, 0x6a87ae30 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "2020_c4.rom", 0x200001, 0x080000, 0xbef75dd0 ) /* Plane 2,3 */
ROM_END

ROM_START( 2020bbh )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "2020h_p1.rom", 0x000000, 0x080000, 0x12d048d7 )

	NEO_SFIX_128K( "2020_s1.rom", 0x7015b8fc )

	NEO_BIOS_SOUND_128K( "2020_m1.rom", 0x4cf466ec )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "2020_v1.rom", 0x000000, 0x100000, 0xd4ca364e )
	ROM_LOAD( "2020_v2.rom", 0x100000, 0x100000, 0x54994455 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "2020_c1.rom", 0x000000, 0x100000, 0x4f5e19bd ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "2020_c2.rom", 0x000001, 0x100000, 0xd6314bf0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "2020_c3.rom", 0x200000, 0x080000, 0x6a87ae30 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "2020_c4.rom", 0x200001, 0x080000, 0xbef75dd0 ) /* Plane 2,3 */
ROM_END

ROM_START( socbrawl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sbrl_p1.rom", 0x000000, 0x080000, 0xa2801c24 )

	NEO_SFIX_64K( "sbrl_s1.rom", 0x2db38c3b )

	NEO_BIOS_SOUND_64K( "sbrl_m1.rom", 0x2f38d5d3 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sbrl_v1.rom", 0x000000, 0x100000, 0xcc78497e )
	ROM_LOAD( "sbrl_v2.rom", 0x100000, 0x100000, 0xdda043c6 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sbrl_c1.rom", 0x000000, 0x100000, 0xbd0a4eb8 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sbrl_c2.rom", 0x000001, 0x100000, 0xefde5382 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "sbrl_c3.rom", 0x200000, 0x080000, 0x580f7f33 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sbrl_c4.rom", 0x200001, 0x080000, 0xed297de8 ) /* Plane 2,3 */
ROM_END

ROM_START( roboarmy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "rarmy_p1.rom", 0x000000, 0x080000, 0xcd11cbd4 )

	NEO_SFIX_128K( "rarmy_s1.rom", 0xac0daa1b )

	NEO_BIOS_SOUND_128K( "rarmy_m1.rom", 0x98edc671 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "rarmy_v1.rom", 0x000000, 0x080000, 0xdaff9896 )
	ROM_LOAD( "rarmy_v2.rom", 0x080000, 0x080000, 0x8781b1bc )
	ROM_LOAD( "rarmy_v3.rom", 0x100000, 0x080000, 0xb69c1da5 )
	ROM_LOAD( "rarmy_v4.rom", 0x180000, 0x080000, 0x2c929c17 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "rarmy_c1.rom", 0x000000, 0x080000, 0xe17fa618 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "rarmy_c2.rom", 0x000001, 0x080000, 0xd5ebdb4d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "rarmy_c3.rom", 0x100000, 0x080000, 0xaa4d7695 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "rarmy_c4.rom", 0x100001, 0x080000, 0x8d4ebbe3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "rarmy_c5.rom", 0x200000, 0x080000, 0x40adfccd ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "rarmy_c6.rom", 0x200001, 0x080000, 0x462571de ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ffry_p1.rom", 0x000000, 0x080000, 0x47ebdc2f )
	ROM_LOAD16_WORD_SWAP( "ffry_p2.rom", 0x080000, 0x020000, 0xc473af1c )

	NEO_SFIX_128K( "ffry_s1.rom", 0x3c3bdf8c )

	NEO_BIOS_SOUND_128K( "ffry_m1.rom", 0xa8603979 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ffry_v1.rom", 0x000000, 0x100000, 0x212fd20d )
	ROM_LOAD( "ffry_v2.rom", 0x100000, 0x100000, 0xfa2ae47f )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ffry_c1.rom", 0x000000, 0x100000, 0x74317e54 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ffry_c2.rom", 0x000001, 0x100000, 0x5bb952f3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ffry_c3.rom", 0x200000, 0x100000, 0x9b714a7c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ffry_c4.rom", 0x200001, 0x100000, 0x9397476a ) /* Plane 2,3 */
ROM_END

ROM_START( fbfrenzy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "fbfr_p1.rom", 0x000000, 0x080000, 0xcdef6b19 )

	NEO_SFIX_128K( "fbfr_s1.rom", 0x8472ed44 )

	NEO_BIOS_SOUND_128K( "fbfr_m1.rom", 0xf41b16b8 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "fbfr_v1.rom", 0x000000, 0x100000, 0x50c9d0dd )
	ROM_LOAD( "fbfr_v2.rom", 0x100000, 0x100000, 0x5aa15686 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "fbfr_c1.rom", 0x000000, 0x100000, 0x91c56e78 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "fbfr_c2.rom", 0x000001, 0x100000, 0x9743ea2f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "fbfr_c3.rom", 0x200000, 0x080000, 0xe5aa65f5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "fbfr_c4.rom", 0x200001, 0x080000, 0x0eb138cc ) /* Plane 2,3 */
ROM_END

ROM_START( bakatono )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "036-p1.bin", 0x000000, 0x080000, 0x1c66b6fa )

	NEO_SFIX_128K( "036-s1.bin", 0xf3ef4485 )

	NEO_BIOS_SOUND_128K( "036-m1.bin", 0xf1385b96 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "036-v1.bin", 0x000000, 0x100000, 0x1c335dce )
	ROM_LOAD( "036-v2.bin", 0x100000, 0x100000, 0xbbf79342 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "036-c1.bin", 0x000000, 0x100000, 0xfe7f1010 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "036-c2.bin", 0x000001, 0x100000, 0xbbf003f5 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "036-c3.bin", 0x200000, 0x100000, 0x9ac0708e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "036-c4.bin", 0x200001, 0x100000, 0xf2577d22 ) /* Plane 2,3 */
ROM_END

ROM_START( crsword )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "csrd_p1.rom", 0x000000, 0x080000, 0xe7f2553c )

	NEO_SFIX_128K( "csrd_s1.rom", 0x74651f27 )

	NEO_BIOS_SOUND_128K( "csrd_m1.rom", 0x9c384263 )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "csrd_v1.rom",  0x000000, 0x100000, 0x61fedf65 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "csrd_c1.rom", 0x000000, 0x100000, 0x09df6892 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "csrd_c2.rom", 0x000001, 0x100000, 0xac122a78 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "csrd_c3.rom", 0x200000, 0x100000, 0x9d7ed1ca ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "csrd_c4.rom", 0x200001, 0x100000, 0x4a24395d ) /* Plane 2,3 */
ROM_END

ROM_START( trally )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "tral_p1.rom", 0x000000, 0x080000, 0x1e52a576 )
	ROM_LOAD16_WORD_SWAP( "tral_p2.rom", 0x080000, 0x080000, 0xa5193e2f )

	NEO_SFIX_128K( "tral_s1.rom", 0xfff62ae3 )

	NEO_BIOS_SOUND_128K( "tral_m1.rom", 0x0908707e )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "tral_v1.rom", 0x000000, 0x100000, 0x5ccd9fd5 )
	ROM_LOAD( "tral_v2.rom", 0x100000, 0x080000, 0xddd8d1e6 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "tral_c1.rom", 0x000000, 0x100000, 0xc58323d4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "tral_c2.rom", 0x000001, 0x100000, 0xbba9c29e ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "tral_c3.rom", 0x200000, 0x080000, 0x3bb7b9d6 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "tral_c4.rom", 0x200001, 0x080000, 0xa4513ecf ) /* Plane 2,3 */
ROM_END

ROM_START( kotm2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kotm2_p1.rom", 0x000000, 0x080000, 0xb372d54c )
	ROM_LOAD16_WORD_SWAP( "kotm2_p2.rom", 0x080000, 0x080000, 0x28661afe )

	NEO_SFIX_128K( "kotm2_s1.rom", 0x63ee053a )

	NEO_BIOS_SOUND_128K( "kotm2_m1.rom", 0x0c5b2ad5 )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kotm2_v1.rom", 0x000000, 0x200000, 0x86d34b25 )
	ROM_LOAD( "kotm2_v2.rom", 0x200000, 0x100000, 0x8fa62a0b )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "kotm2_c1.rom", 0x000000, 0x100000, 0x6d1c4aa9 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "kotm2_c2.rom", 0x000001, 0x100000, 0xf7b75337 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "kotm2_c3.rom", 0x200000, 0x100000, 0x40156dca ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "kotm2_c4.rom", 0x200001, 0x100000, 0xb0d44111 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x600001, 0x100000 )
ROM_END

ROM_START( sengoku2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "goku2_p1.rom", 0x000000, 0x080000, 0xcc245299 )
	ROM_LOAD16_WORD_SWAP( "goku2_p2.rom", 0x080000, 0x080000, 0x2e466360 )

	NEO_SFIX_128K( "goku2_s1.rom", 0xcd9802a3 )

	NEO_BIOS_SOUND_128K( "goku2_m1.rom", 0x9902dfa2 )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "goku2_v1.rom", 0x000000, 0x100000, 0xb3725ced )
	ROM_LOAD( "goku2_v2.rom", 0x100000, 0x100000, 0xb5e70a0e )
	ROM_LOAD( "goku2_v3.rom", 0x200000, 0x100000, 0xc5cece01 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "goku2_c1.rom", 0x000000, 0x200000, 0x3cacd552 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "goku2_c2.rom", 0x000001, 0x200000, 0xe2aadef3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "goku2_c3.rom", 0x400000, 0x200000, 0x037614d5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "goku2_c4.rom", 0x400001, 0x200000, 0xe9947e5b ) /* Plane 2,3 */
ROM_END

ROM_START( bstars2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "star2_p1.rom", 0x000000, 0x080000, 0x523567fd )

	NEO_SFIX_128K( "star2_s1.rom", 0x015c5c94 )

	NEO_BIOS_SOUND_64K( "star2_m1.rom", 0xb2611c03 )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "star2_v1.rom", 0x000000, 0x100000, 0xcb1da093 )
	ROM_LOAD( "star2_v2.rom", 0x100000, 0x100000, 0x1c954a9d )
	ROM_LOAD( "star2_v3.rom", 0x200000, 0x080000, 0xafaa0180 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "star2_c1.rom", 0x000000, 0x100000, 0xb39a12e1 )
	ROM_LOAD16_BYTE( "star2_c2.rom", 0x000001, 0x100000, 0x766cfc2f )
	ROM_LOAD16_BYTE( "star2_c3.rom", 0x200000, 0x100000, 0xfb31339d )
	ROM_LOAD16_BYTE( "star2_c4.rom", 0x200001, 0x100000, 0x70457a0c )
ROM_END

ROM_START( quizdai2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mein_p1.rom", 0x000000, 0x100000, 0xed719dcf)

	NEO_SFIX_128K( "mein_s1.rom", 0x164fd6e6 )

	NEO_BIOS_SOUND_128K( "mein_m1.rom", 0xbb19995d )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "mein_v1.rom", 0x000000, 0x100000, 0xaf7f8247 )
	ROM_LOAD( "mein_v2.rom", 0x100000, 0x100000, 0xc6474b59 )

	NO_DELTAT_REGION

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mein_c1.rom", 0x000000, 0x100000, 0xcb5809a1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "mein_c2.rom", 0x000001, 0x100000, 0x1436dfeb ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "mein_c3.rom", 0x200000, 0x080000, 0xbcd4a518 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "mein_c4.rom", 0x200001, 0x080000, 0xd602219b ) /* Plane 2,3 */
ROM_END

ROM_START( 3countb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "3cb_p1.rom", 0x000000, 0x080000, 0xeb2714c4 )
	ROM_LOAD16_WORD_SWAP( "3cb_p2.rom", 0x080000, 0x080000, 0x5e764567 )

	NEO_SFIX_128K( "3cb_s1.rom", 0xc362d484 )

	NEO_BIOS_SOUND_128K( "3cb_m1.rom", 0x3377cda3 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "3cb_v1.rom", 0x000000, 0x200000, 0x63688ce8 )
	ROM_LOAD( "3cb_v2.rom", 0x200000, 0x200000, 0xc69a827b )

	NO_DELTAT_REGION

	ROM_REGION( 0x0800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "3cb_c1.rom", 0x0000000, 0x200000, 0xd290cc33 )
	ROM_LOAD16_BYTE( "3cb_c2.rom", 0x0000001, 0x200000, 0x0b28095d )
	ROM_LOAD16_BYTE( "3cb_c3.rom", 0x0400000, 0x200000, 0xbcc0cb35 )
	ROM_LOAD16_BYTE( "3cb_c4.rom", 0x0400001, 0x200000, 0x4d1ff7b9 )
ROM_END

ROM_START( aof )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "aof_p1.rom", 0x000000, 0x080000, 0xca9f7a6d )

	NEO_SFIX_128K( "aof_s1.rom", 0x89903f39 )

	NEO_BIOS_SOUND_128K( "aof_m1.rom", 0x981345f8 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "aof_v2.rom", 0x000000, 0x200000, 0x3ec632ea )
	ROM_LOAD( "aof_v4.rom", 0x200000, 0x200000, 0x4b0f8e23 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "aof_c1.rom", 0x000000, 0x100000, 0xddab98a7 ) /* Plane 0,1 */
	ROM_CONTINUE(      			 0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "aof_c2.rom", 0x000001, 0x100000, 0xd8ccd575 ) /* Plane 2,3 */
	ROM_CONTINUE(      			 0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "aof_c3.rom", 0x200000, 0x100000, 0x403e898a ) /* Plane 0,1 */
	ROM_CONTINUE(      			 0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "aof_c4.rom", 0x200001, 0x100000, 0x6235fbaa ) /* Plane 2,3 */
	ROM_CONTINUE(      			 0x600001, 0x100000 )
ROM_END

ROM_START( samsho )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "samsh_p1.rom", 0x000000, 0x080000, 0x80aa6c97 )
	ROM_LOAD16_WORD_SWAP( "samsh_p2.rom", 0x080000, 0x080000, 0x71768728 )
	ROM_LOAD16_WORD_SWAP( "samsh_p3.rom", 0x100000, 0x080000, 0x38ee9ba9 )

	NEO_SFIX_128K( "samsh_s1.rom", 0x9142a4d3 )

	NEO_BIOS_SOUND_128K( "samsh_m1.rom", 0x95170640 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "samsh_v1.rom", 0x000000, 0x200000, 0x37f78a9b )
	ROM_LOAD( "samsh_v2.rom", 0x200000, 0x200000, 0x568b20cf )

	NO_DELTAT_REGION

	ROM_REGION( 0x900000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "samsh_c1.rom", 0x000000, 0x200000, 0x2e5873a4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "samsh_c2.rom", 0x000001, 0x200000, 0x04febb10 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "samsh_c3.rom", 0x400000, 0x200000, 0xf3dabd1e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "samsh_c4.rom", 0x400001, 0x200000, 0x935c62f0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "samsh_c5.rom", 0x800000, 0x080000, 0xa2bb8284 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "samsh_c6.rom", 0x800001, 0x080000, 0x4fa71252 ) /* Plane 2,3 */
ROM_END

ROM_START( tophuntr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "046-p1.bin", 0x000000, 0x100000, 0x69fa9e29 )
	ROM_LOAD16_WORD_SWAP( "046-p2.sp2", 0x100000, 0x100000, 0xf182cb3e )

	NEO_SFIX_128K( "046-s1.bin", 0x14b01d7b )

	NEO_BIOS_SOUND_128K( "046-m1.bin", 0x3f84bb9f )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "046-v1.bin", 0x000000, 0x100000, 0xc1f9c2db )
	ROM_LOAD( "046-v2.bin", 0x100000, 0x100000, 0x56254a64 )
	ROM_LOAD( "046-v3.bin", 0x200000, 0x100000, 0x58113fb1 )
	ROM_LOAD( "046-v4.bin", 0x300000, 0x100000, 0x4f54c187 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "046-c1.bin", 0x000000, 0x100000, 0xfa720a4a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c2.bin", 0x000001, 0x100000, 0xc900c205 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c3.bin", 0x200000, 0x100000, 0x880e3c25 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c4.bin", 0x200001, 0x100000, 0x7a2248aa ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c5.bin", 0x400000, 0x100000, 0x4b735e45 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c6.bin", 0x400001, 0x100000, 0x273171df ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "046-c7.bin", 0x600000, 0x100000, 0x12829c4c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "046-c8.bin", 0x600001, 0x100000, 0xc944e03d ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "fury2_p1.rom", 0x000000, 0x080000, 0xbe40ea92 )
	ROM_LOAD16_WORD_SWAP( "fury2_p2.rom", 0x080000, 0x080000, 0x2a9beac5 )

	NEO_SFIX_128K( "fury2_s1.rom", 0xd7dbbf39 )

	NEO_BIOS_SOUND_128K( "fury2_m1.rom", 0x820b0ba7 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "fury2_v1.rom", 0x000000, 0x200000, 0xd9d00784 )
	ROM_LOAD( "fury2_v2.rom", 0x200000, 0x200000, 0x2c9a4b33 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "fury2_c1.rom", 0x000000, 0x100000, 0xf72a939e ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "fury2_c2.rom", 0x000001, 0x100000, 0x05119a0d ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "fury2_c3.rom", 0x200000, 0x100000, 0x01e00738 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x600000, 0x100000 )
	ROM_LOAD16_BYTE( "fury2_c4.rom", 0x200001, 0x100000, 0x9fe27432 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x600001, 0x100000 )
ROM_END

ROM_START( janshin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "jans-p1.rom", 0x000000, 0x100000, 0x7514cb7a )

	NEO_SFIX_128K( "jans-s1.rom", 0x8285b25a )

	NEO_BIOS_SOUND_64K( "jans-m1.rom", 0xe191f955 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "jans-v1.rom", 0x000000, 0x200000, 0xf1947d2b )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "jans-c1.rom", 0x000000, 0x200000, 0x3fa890e9 )
	ROM_LOAD16_BYTE( "jans-c2.rom", 0x000001, 0x200000, 0x59c48ad8 )
ROM_END

ROM_START( androdun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "adun_p1.rom", 0x000000, 0x080000, 0x3b857da2 )
	ROM_LOAD16_WORD_SWAP( "adun_p2.rom", 0x080000, 0x080000, 0x2f062209 )

	NEO_SFIX_128K( "adun_s1.rom", 0x6349de5d )

	NEO_BIOS_SOUND_128K( "adun_m1.rom", 0x1a009f8c )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "adun_v1.rom", 0x000000, 0x080000, 0x577c85b3 )
	ROM_LOAD( "adun_v2.rom", 0x080000, 0x080000, 0xe14551c4 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "adun_c1.rom", 0x000000, 0x100000, 0x7ace6db3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "adun_c2.rom", 0x000001, 0x100000, 0xb17024f7 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "adun_c3.rom", 0x200000, 0x100000, 0x2e0f3f9a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "adun_c4.rom", 0x200001, 0x100000, 0x4a19fb92 ) /* Plane 2,3 */
ROM_END

ROM_START( ncommand )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "050-p1.bin", 0x000000, 0x100000, 0x4e097c40 )

	NEO_SFIX_128K( "050-s1.bin", 0xdb8f9c8e )

	NEO_BIOS_SOUND_128K( "050-m1.bin", 0x6fcf07d3 )

	ROM_REGION( 0x180000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "050-v1.bin", 0x000000, 0x100000, 0x23c3ab42 )
	ROM_LOAD( "050-v2.bin", 0x100000, 0x080000, 0x80b8a984 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "050-c1.bin", 0x000000, 0x100000, 0x87421a0a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "050-c2.bin", 0x000001, 0x100000, 0xc4cf5548 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "050-c3.bin", 0x200000, 0x100000, 0x03422c1e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "050-c4.bin", 0x200001, 0x100000, 0x0845eadb ) /* Plane 2,3 */
ROM_END

ROM_START( viewpoin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "viewp_p1.rom", 0x000000, 0x100000, 0x17aa899d )

	NEO_SFIX_64K( "viewp_s1.rom", 0x6d0f146a )

	NEO_BIOS_SOUND_64K( "viewp_m1.rom", 0xd57bd7af )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "viewp_v1.rom", 0x000000, 0x200000, 0x019978b6 )
	ROM_LOAD( "viewp_v2.rom", 0x200000, 0x200000, 0x5758f38c )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "viewp_c1.rom", 0x000000, 0x100000, 0xd624c132 )
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "viewp_c2.rom", 0x000001, 0x100000, 0x40d69f1e )
	ROM_CONTINUE(      			   0x400001, 0x100000 )
ROM_END

ROM_START( ssideki )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sidek_p1.rom", 0x000000, 0x080000, 0x9cd97256 )

	NEO_SFIX_128K( "sidek_s1.rom", 0x97689804 )

	NEO_BIOS_SOUND_128K( "sidek_m1.rom", 0x49f17d2d )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sidek_v1.rom", 0x000000, 0x200000, 0x22c097a5 )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sidek_c1.rom", 0x000000, 0x100000, 0x53e1c002 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "sidek_c2.rom", 0x000001, 0x100000, 0x776a2d1f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x400001, 0x100000 )
ROM_END

ROM_START( wh1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "wh_p1.rom", 0x000000, 0x080000, 0x95b574cb )
	ROM_LOAD16_WORD_SWAP( "wh_p2.rom", 0x080000, 0x080000, 0xf198ed45 )

	NEO_SFIX_128K( "wh_s1.rom", 0x8c2c2d6b )

	NEO_BIOS_SOUND_128K( "wh_m1.rom", 0x1bd9d04b )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "wh_v2.rom", 0x000000, 0x200000, 0xa68df485 )
	ROM_LOAD( "wh_v4.rom", 0x200000, 0x100000, 0x7bea8f66 )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "wh_c1.rom", 0x000000, 0x100000, 0x85eb5bce ) /* Plane 0,1 */
	ROM_CONTINUE(      			0x400000, 0x100000 )
	ROM_LOAD16_BYTE( "wh_c2.rom", 0x000001, 0x100000, 0xec93b048 ) /* Plane 2,3 */
	ROM_CONTINUE(      			0x400001, 0x100000 )
	ROM_LOAD16_BYTE( "wh_c3.rom", 0x200000, 0x100000, 0x0dd64965 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "wh_c4.rom", 0x200001, 0x100000, 0x9270d954 ) /* Plane 2,3 */
ROM_END

ROM_START( kof94 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kof94_p1.rom", 0x100000, 0x100000, 0xf10a2042 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "kof94_s1.rom", 0x825976c1 )

	NEO_BIOS_SOUND_128K( "kof94_m1.rom", 0xf6e77cf5 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof94_v1.rom", 0x000000, 0x200000, 0x8889596d )
	ROM_LOAD( "kof94_v2.rom", 0x200000, 0x200000, 0x25022b27 )
	ROM_LOAD( "kof94_v3.rom", 0x400000, 0x200000, 0x83cf32c0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "kof94_c1.rom", 0x000000, 0x200000, 0xb96ef460 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof94_c2.rom", 0x000001, 0x200000, 0x15e096a7 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof94_c3.rom", 0x400000, 0x200000, 0x54f66254 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof94_c4.rom", 0x400001, 0x200000, 0x0b01765f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof94_c5.rom", 0x800000, 0x200000, 0xee759363 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof94_c6.rom", 0x800001, 0x200000, 0x498da52c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof94_c7.rom", 0xc00000, 0x200000, 0x62f66888 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof94_c8.rom", 0xc00001, 0x200000, 0xfe0a235d ) /* Plane 2,3 */
ROM_END

ROM_START( aof2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "aof2_p1.rom", 0x000000, 0x100000, 0xa3b1d021 )

	NEO_SFIX_128K( "aof2_s1.rom", 0x8b02638e )

	NEO_BIOS_SOUND_128K( "aof2_m1.rom", 0xf27e9d52 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "aof2_v1.rom", 0x000000, 0x200000, 0x4628fde0 )
	ROM_LOAD( "aof2_v2.rom", 0x200000, 0x200000, 0xb710e2f2 )
	ROM_LOAD( "aof2_v3.rom", 0x400000, 0x100000, 0xd168c301 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "aof2_c1.rom", 0x000000, 0x200000, 0x17b9cbd2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aof2_c2.rom", 0x000001, 0x200000, 0x5fd76b67 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "aof2_c3.rom", 0x400000, 0x200000, 0xd2c88768 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aof2_c4.rom", 0x400001, 0x200000, 0xdb39b883 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "aof2_c5.rom", 0x800000, 0x200000, 0xc3074137 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aof2_c6.rom", 0x800001, 0x200000, 0x31de68d3 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "aof2_c7.rom", 0xc00000, 0x200000, 0x3f36df57 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aof2_c8.rom", 0xc00001, 0x200000, 0xe546d7a8 ) /* Plane 2,3 */
ROM_END

ROM_START( wh2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "hero2_p1.rom", 0x100000, 0x100000, 0x65a891d9 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "hero2_s1.rom", 0xfcaeb3a4 )

	NEO_BIOS_SOUND_128K( "hero2_m1.rom", 0x8fa3bc77 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "hero2_v1.rom", 0x000000, 0x200000, 0x8877e301 )
	ROM_LOAD( "hero2_v2.rom", 0x200000, 0x200000, 0xc1317ff4 )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "hero2_c1.rom", 0x000000, 0x200000, 0x21c6bb91 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "hero2_c2.rom", 0x000001, 0x200000, 0xa3999925 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "hero2_c3.rom", 0x400000, 0x200000, 0xb725a219 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "hero2_c4.rom", 0x400001, 0x200000, 0x8d96425e ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "hero2_c5.rom", 0x800000, 0x200000, 0xb20354af ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "hero2_c6.rom", 0x800001, 0x200000, 0xb13d1de3 ) /* Plane 2,3 */
ROM_END

ROM_START( fatfursp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ffspe_p1.rom", 0x000000, 0x100000, 0x2f585ba2 )
	ROM_LOAD16_WORD_SWAP( "ffspe_p2.rom", 0x100000, 0x080000, 0xd7c71a6b )
	ROM_LOAD16_WORD_SWAP( "ffspe_p3.rom", 0x180000, 0x080000, 0x9f0c1e1a )

	NEO_SFIX_128K( "ffspe_s1.rom", 0x2df03197 )

	NEO_BIOS_SOUND_128K( "ffspe_m1.rom", 0xccc5186e )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ffspe_v1.rom", 0x000000, 0x200000, 0x55d7ce84 )
	ROM_LOAD( "ffspe_v2.rom", 0x200000, 0x200000, 0xee080b10 )
	ROM_LOAD( "ffspe_v3.rom", 0x400000, 0x100000, 0xf9eb3d4a )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ffspe_c1.rom", 0x000000, 0x200000, 0x044ab13c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ffspe_c2.rom", 0x000001, 0x200000, 0x11e6bf96 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ffspe_c3.rom", 0x400000, 0x200000, 0x6f7938d5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ffspe_c4.rom", 0x400001, 0x200000, 0x4ad066ff ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ffspe_c5.rom", 0x800000, 0x200000, 0x49c5e0bf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ffspe_c6.rom", 0x800001, 0x200000, 0x8ff1f43d ) /* Plane 2,3 */
ROM_END

ROM_START( savagere )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "savag_p1.rom", 0x100000, 0x100000, 0x01d4e9c0 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "savag_s1.rom", 0xe08978ca )

	NEO_BIOS_SOUND_128K( "savag_m1.rom", 0x29992eba )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "savag_v1.rom", 0x000000, 0x200000, 0x530c50fd )
	ROM_LOAD( "savag_v2.rom", 0x200000, 0x200000, BADCRC( 0xe79a9bd0 ) )
	ROM_LOAD( "savag_v3.rom", 0x400000, 0x200000, 0x7038c2f9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "savag_c1.rom", 0x000000, 0x200000, 0x763ba611 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "savag_c2.rom", 0x000001, 0x200000, 0xe05e8ca6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "savag_c3.rom", 0x400000, 0x200000, 0x3e4eba4b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "savag_c4.rom", 0x400001, 0x200000, 0x3c2a3808 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "savag_c5.rom", 0x800000, 0x200000, 0x59013f9e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "savag_c6.rom", 0x800001, 0x200000, 0x1c8d5def ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "savag_c7.rom", 0xc00000, 0x200000, 0xc88f7035 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "savag_c8.rom", 0xc00001, 0x200000, 0x484ce3ba ) /* Plane 2,3 */
ROM_END

ROM_START( fightfev )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ff_p1.rom", 0x000000, 0x080000, 0x3032041b )
	ROM_LOAD16_WORD_SWAP( "ff_p2.rom", 0x080000, 0x080000, 0xb0801d5f )

	NEO_SFIX_128K( "ff_s1.rom", 0x70727a1e )

	NEO_BIOS_SOUND_128K( "ff_m1.rom", 0x0b7c4e65 )

	ROM_REGION(  0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ff_v1.rom", 0x000000, 0x200000, 0xf417c215 )
	ROM_LOAD( "ff_v2.rom", 0x200000, 0x100000, 0x64470036 )

	NO_DELTAT_REGION

	ROM_REGION( 0x0800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ff_c1.rom", 0x0000000, 0x200000, 0x8908fff9 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ff_c2.rom", 0x0000001, 0x200000, 0xc6649492 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ff_c3.rom", 0x0400000, 0x200000, 0x0956b437 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ff_c4.rom", 0x0400001, 0x200000, 0x026f3b62 ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kick2_p1.rom", 0x000000, 0x100000, 0x5969e0dc )

	NEO_SFIX_128K( "kick2_s1.rom", 0x226d1b68 )

	NEO_BIOS_SOUND_128K( "kick2_m1.rom", 0x156f6951 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kick2_v1.rom", 0x000000, 0x200000, 0xf081c8d3 )
	ROM_LOAD( "kick2_v2.rom", 0x200000, 0x200000, 0x7cd63302 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "kick2_c1.rom", 0x000000, 0x200000, 0xa626474f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kick2_c2.rom", 0x000001, 0x200000, 0xc3be42ae ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kick2_c3.rom", 0x400000, 0x200000, 0x2a7b98b9 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kick2_c4.rom", 0x400001, 0x200000, 0xc0be9a1f ) /* Plane 2,3 */
ROM_END

ROM_START( spinmast )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "spnm_p1.rom", 0x000000, 0x100000, 0x37aba1aa )
	ROM_LOAD16_WORD_SWAP( "spnm_p2.rom", 0x100000, 0x080000, 0x43763ad2 )

	NEO_SFIX_128K( "spnm_s1.rom", 0x289e2bbe )

	NEO_BIOS_SOUND_128K( "spnm_m1.rom", 0x76108b2f )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "spnm_v1.rom", 0x000000, 0x100000, 0xcc281aef )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "spnm_c1.rom", 0x000000, 0x100000, 0xa9375aa2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "spnm_c2.rom", 0x000001, 0x100000, 0x0e73b758 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "spnm_c3.rom", 0x200000, 0x100000, 0xdf51e465 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "spnm_c4.rom", 0x200001, 0x100000, 0x38517e90 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "spnm_c5.rom", 0x400000, 0x100000, 0x7babd692 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "spnm_c6.rom", 0x400001, 0x100000, 0xcde5ade5 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "spnm_c7.rom", 0x600000, 0x100000, 0xbb2fd7c0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "spnm_c8.rom", 0x600001, 0x100000, 0x8d7be933 ) /* Plane 2,3 */
ROM_END

ROM_START( samsho2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sams2_p1.rom", 0x100000, 0x100000, 0x22368892 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "sams2_s1.rom", 0x64a5cd66 )

	NEO_BIOS_SOUND_128K( "sams2_m1.rom", 0x56675098 )

	ROM_REGION( 0x700000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sams2_v1.rom", 0x000000, 0x200000, 0x37703f91 )
	ROM_LOAD( "sams2_v2.rom", 0x200000, 0x200000, 0x0142bde8 )
	ROM_LOAD( "sams2_v3.rom", 0x400000, 0x200000, 0xd07fa5ca )
	ROM_LOAD( "sams2_v4.rom", 0x600000, 0x100000, 0x24aab4bb )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sams2_c1.rom", 0x000000, 0x200000, 0x86cd307c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sams2_c2.rom", 0x000001, 0x200000, 0xcdfcc4ca ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "sams2_c3.rom", 0x400000, 0x200000, 0x7a63ccc7 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sams2_c4.rom", 0x400001, 0x200000, 0x751025ce ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "sams2_c5.rom", 0x800000, 0x200000, 0x20d3a475 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sams2_c6.rom", 0x800001, 0x200000, 0xae4c0a88 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "sams2_c7.rom", 0xc00000, 0x200000, 0x2df3cbcf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sams2_c8.rom", 0xc00001, 0x200000, 0x1ffc6dfa ) /* Plane 2,3 */
ROM_END

ROM_START( wh2j )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "wh2j_p1.rom", 0x100000, 0x100000, 0x385a2e86 )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "wh2j_s1.rom", 0x2a03998a )

	NEO_BIOS_SOUND_128K( "wh2j_m1.rom", 0xd2eec9d3 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "wh2j_v1.rom", 0x000000, 0x200000, 0xaa277109 )
	ROM_LOAD( "wh2j_v2.rom", 0x200000, 0x200000, 0xb6527edd )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "wh2j_c1.rom", 0x000000, 0x200000, 0x2ec87cea ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "wh2j_c2.rom", 0x000001, 0x200000, 0x526b81ab ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "wh2j_c3.rom", 0x400000, 0x200000, 0x436d1b31 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "wh2j_c4.rom", 0x400001, 0x200000, 0xf9c8dd26 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "wh2j_c5.rom", 0x800000, 0x200000, 0x8e34a9f4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "wh2j_c6.rom", 0x800001, 0x200000, 0xa43e4766 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "wh2j_c7.rom", 0xc00000, 0x200000, 0x59d97215 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "wh2j_c8.rom", 0xc00001, 0x200000, 0xfc092367 ) /* Plane 0,1 */
ROM_END

ROM_START( wjammers )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "windj_p1.rom", 0x000000, 0x080000, 0xe81e7a31 )

	NEO_SFIX_128K( "windj_s1.rom", 0x66cb96eb )

	NEO_BIOS_SOUND_128K( "windj_m1.rom", 0x52c23cfc )

	ROM_REGION( 0x380000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "windj_v1.rom", 0x000000, 0x100000, 0xce8b3698 )
	ROM_LOAD( "windj_v2.rom", 0x100000, 0x100000, 0x659f9b96 )
	ROM_LOAD( "windj_v3.rom", 0x200000, 0x100000, 0x39f73061 )
	ROM_LOAD( "windj_v4.rom", 0x300000, 0x080000, 0x3740edde )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "windj_c1.rom", 0x000000, 0x100000, 0xc7650204 )
	ROM_LOAD16_BYTE( "windj_c2.rom", 0x000001, 0x100000, 0xd9f3e71d )
	ROM_LOAD16_BYTE( "windj_c3.rom", 0x200000, 0x100000, 0x40986386 )
	ROM_LOAD16_BYTE( "windj_c4.rom", 0x200001, 0x100000, 0x715e15ff )
ROM_END

ROM_START( karnovr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "karev_p1.rom", 0x000000, 0x100000, 0x8c86fd22 )

	NEO_SFIX_128K( "karev_s1.rom", 0xbae5d5e5 )

	NEO_BIOS_SOUND_128K( "karev_m1.rom", 0x030beae4 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "karev_v1.rom", 0x000000, 0x200000, 0x0b7ea37a )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "karev_c1.rom", 0x000000, 0x200000, 0x09dfe061 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "karev_c2.rom", 0x000001, 0x200000, 0xe0f6682a ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "karev_c3.rom", 0x400000, 0x200000, 0xa673b4f7 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "karev_c4.rom", 0x400001, 0x200000, 0xcb3dc5f4 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "karev_c5.rom", 0x800000, 0x200000, 0x9a28785d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "karev_c6.rom", 0x800001, 0x200000, 0xc15c01ed ) /* Plane 2,3 */
ROM_END

ROM_START( gururin )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "gurin_p1.rom", 0x000000, 0x80000, 0x4cea8a49 )

	NEO_SFIX_128K( "gurin_s1.rom", 0x4f0cbd58 )

	NEO_BIOS_SOUND_64K( "gurin_m1.rom", 0x833cdf1b )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "gurin_v1.rom", 0x000000, 0x80000, 0xcf23afd0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "gurin_c1.rom", 0x000000, 0x200000, 0x35866126 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "gurin_c2.rom", 0x000001, 0x200000, 0x9db64084 ) /* Plane 2,3 */
ROM_END

ROM_START( pspikes2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "spike_p1.rom", 0x000000, 0x100000, 0x105a408f )

	NEO_SFIX_128K( "spike_s1.rom", 0x18082299 )

	NEO_BIOS_SOUND_128K( "spike_m1.rom", 0xb1c7911e )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "spike_v1.rom", 0x000000, 0x100000, 0x2ced86df )  /* == pbobble */
	ROM_LOAD( "spike_v2.rom", 0x100000, 0x100000, 0x970851ab )  /* == pbobble */
	ROM_LOAD( "spike_v3.rom", 0x200000, 0x100000, 0x81ff05aa )

	NO_DELTAT_REGION

	ROM_REGION( 0x600000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "spike_c1.rom", 0x000000, 0x100000, 0x7f250f76 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "spike_c2.rom", 0x000001, 0x100000, 0x20912873 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "spike_c3.rom", 0x200000, 0x100000, 0x4b641ba1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "spike_c4.rom", 0x200001, 0x100000, 0x35072596 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "spike_c5.rom", 0x400000, 0x100000, 0x151dd624 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "spike_c6.rom", 0x400001, 0x100000, 0xa6722604 ) /* Plane 2,3 */
ROM_END

ROM_START( fatfury3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "069-p1.bin", 0x000000, 0x100000, 0xa8bcfbbc )
	ROM_LOAD16_WORD_SWAP( "069-p2.bin", 0x100000, 0x200000, 0xdbe963ed )

	NEO_SFIX_128K( "069-s1.bin", 0x0b33a800 )

	NEO_BIOS_SOUND_128K( "069-m1.bin", 0xfce72926 )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "069-v1.bin", 0x000000, 0x400000, 0x2bdbd4db )
	ROM_LOAD( "069-v2.bin", 0x400000, 0x400000, 0xa698a487 )
	ROM_LOAD( "069-v3.bin", 0x800000, 0x200000, 0x581c5304 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "069-c1.bin", 0x0000000, 0x400000, 0xe302f93c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c2.bin", 0x0000001, 0x400000, 0x1053a455 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c3.bin", 0x0800000, 0x400000, 0x1c0fde2f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c4.bin", 0x0800001, 0x400000, 0xa25fc3d0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c5.bin", 0x1000000, 0x200000, 0xb3ec6fa6 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c6.bin", 0x1000001, 0x200000, 0x69210441 ) /* Plane 2,3 */
ROM_END

ROM_START( panicbom )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "panic_p1.rom", 0x000000, 0x040000, 0x0b21130d )

	NEO_SFIX_128K( "panic_s1.rom", 0xb876de7e )

	NEO_BIOS_SOUND_128K( "panic_m1.rom", 0x3cdf5d88 )

	ROM_REGION( 0x300000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "panic_v1.rom", 0x000000, 0x200000, 0x7fc86d2f )
	ROM_LOAD( "panic_v2.rom", 0x200000, 0x100000, 0x082adfc7 )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "panic_c1.rom", 0x000000, 0x100000, 0x8582e1b5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "panic_c2.rom", 0x000001, 0x100000, 0xe15a093b ) /* Plane 2,3 */
ROM_END

ROM_START( aodk )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "aodk_p1.rom", 0x100000, 0x100000, 0x62369553 )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "aodk_s1.rom", 0x96148d2b )

	NEO_BIOS_SOUND_128K( "aodk_m1.rom", 0x5a52a9d1 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "aodk_v1.rom", 0x000000, 0x200000, 0x7675b8fa )
	ROM_LOAD( "aodk_v2.rom", 0x200000, 0x200000, 0xa9da86e9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "aodk_c1.rom", 0x000000, 0x200000, 0xa0b39344 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aodk_c2.rom", 0x000001, 0x200000, 0x203f6074 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "aodk_c3.rom", 0x400000, 0x200000, 0x7fff4d41 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aodk_c4.rom", 0x400001, 0x200000, 0x48db3e0a ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "aodk_c5.rom", 0x800000, 0x200000, 0xc74c5e51 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aodk_c6.rom", 0x800001, 0x200000, 0x73e8e7e0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "aodk_c7.rom", 0xc00000, 0x200000, 0xac7daa01 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aodk_c8.rom", 0xc00001, 0x200000, 0x14e7ad71 ) /* Plane 2,3 */
ROM_END

ROM_START( sonicwi2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "afig2_p1.rom", 0x100000, 0x100000, 0x92871738 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "afig2_s1.rom", 0x47cc6177 )

	NEO_BIOS_SOUND_128K( "afig2_m1.rom", 0xbb828df1 )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "afig2_v1.rom", 0x000000, 0x200000, 0x7577e949 )
	ROM_LOAD( "afig2_v2.rom", 0x200000, 0x080000, 0x6d0a728e )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "afig2_c1.rom", 0x000000, 0x200000, 0x3278e73e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "afig2_c2.rom", 0x000001, 0x200000, 0xfe6355d6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "afig2_c3.rom", 0x400000, 0x200000, 0xc1b438f1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "afig2_c4.rom", 0x400001, 0x200000, 0x1f777206 ) /* Plane 2,3 */
ROM_END

ROM_START( zedblade )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "zedbl_p1.rom", 0x000000, 0x080000, 0xd7c1effd )

	NEO_SFIX_128K( "zedbl_s1.rom", 0xf4c25dd5 )

	NEO_BIOS_SOUND_128K( "zedbl_m1.rom", 0x7b5f3d0a )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "zedbl_v1.rom", 0x000000, 0x200000, 0x1a21d90c )
	ROM_LOAD( "zedbl_v2.rom", 0x200000, 0x200000, 0xb61686c3 )
	ROM_LOAD( "zedbl_v3.rom", 0x400000, 0x100000, 0xb90658fa )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "zedbl_c1.rom", 0x000000, 0x200000, 0x4d9cb038 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "zedbl_c2.rom", 0x000001, 0x200000, 0x09233884 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "zedbl_c3.rom", 0x400000, 0x200000, 0xd06431e3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "zedbl_c4.rom", 0x400001, 0x200000, 0x4b1c089b ) /* Plane 2,3 */
ROM_END

ROM_START( galaxyfg )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "galfi_p1.rom", 0x100000, 0x100000, 0x45906309 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "galfi_s1.rom", 0x72f8923e )

	NEO_BIOS_SOUND_128K( "galfi_m1.rom", 0x8e9e3b10 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "galfi_v1.rom", 0x000000, 0x200000, 0xe3b735ac )
	ROM_LOAD( "galfi_v2.rom", 0x200000, 0x200000, 0x6a8e78c2 )
	ROM_LOAD( "galfi_v3.rom", 0x400000, 0x100000, 0x70bca656 )

	NO_DELTAT_REGION

	ROM_REGION( 0xe00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "galfi_c1.rom", 0x000000, 0x200000, 0xc890c7c0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "galfi_c2.rom", 0x000001, 0x200000, 0xb6d25419 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "galfi_c3.rom", 0x400000, 0x200000, 0x9d87e761 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "galfi_c4.rom", 0x400001, 0x200000, 0x765d7cb8 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "galfi_c5.rom", 0x800000, 0x200000, 0xe6b77e6a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "galfi_c6.rom", 0x800001, 0x200000, 0xd779a181 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "galfi_c7.rom", 0xc00000, 0x100000, 0x4f27d580 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "galfi_c8.rom", 0xc00001, 0x100000, 0x0a7cc0d8 ) /* Plane 2,3 */
ROM_END

ROM_START( strhoop )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "shoop_p1.rom", 0x000000, 0x100000, 0x5e78328e )

	NEO_SFIX_128K( "shoop_s1.rom", 0xa8205610 )

	NEO_BIOS_SOUND_64K( "shoop_m1.rom", 0x1a5f08db )

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "shoop_v1.rom", 0x000000, 0x200000, 0x718a2400 )
	ROM_LOAD( "shoop_v2.rom", 0x200000, 0x080000, 0xb19884f8 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "shoop_c1.rom", 0x000000, 0x200000, 0x0581c72a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "shoop_c2.rom", 0x000001, 0x200000, 0x5b9b8fb6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "shoop_c3.rom", 0x400000, 0x200000, 0xcd65bb62 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "shoop_c4.rom", 0x400001, 0x200000, 0xa4c90213 ) /* Plane 2,3 */
ROM_END

ROM_START( quizkof )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "qkof-p1.rom", 0x000000, 0x100000, 0x4440315e )

	NEO_SFIX_128K( "qkof-s1.rom", 0xd7b86102 )

	NEO_BIOS_SOUND_128K( "qkof-m1.rom", 0xf5f44172 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "qkof-v1.rom", 0x000000, 0x200000, 0x0be18f60 )
	ROM_LOAD( "qkof-v2.rom", 0x200000, 0x200000, 0x4abde3ff )
	ROM_LOAD( "qkof-v3.rom", 0x400000, 0x200000, 0xf02844e2 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "qkof-c1.rom",  0x000000, 0x200000, 0xea1d764a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "qkof-c2.rom",  0x000001, 0x200000, 0xc78c49da ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "qkof-c3.rom",  0x400000, 0x200000, 0xb4851bfe ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "qkof-c4.rom",  0x400001, 0x200000, 0xca6f5460 ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "side3_p1.rom", 0x100000, 0x100000, 0x6bc27a3d )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "side3_s1.rom", 0x7626da34 )

	NEO_BIOS_SOUND_128K( "side3_m1.rom", 0x82fcd863 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "side3_v1.rom", 0x000000, 0x200000, 0x201fa1e1 )
	ROM_LOAD( "side3_v2.rom", 0x200000, 0x200000, 0xacf29d96 )
	ROM_LOAD( "side3_v3.rom", 0x400000, 0x200000, 0xe524e415 )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "side3_c1.rom", 0x000000, 0x200000, 0x1fb68ebe ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "side3_c2.rom", 0x000001, 0x200000, 0xb28d928f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "side3_c3.rom", 0x400000, 0x200000, 0x3b2572e8 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "side3_c4.rom", 0x400001, 0x200000, 0x47d26a7c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "side3_c5.rom", 0x800000, 0x200000, 0x17d42f0d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "side3_c6.rom", 0x800001, 0x200000, 0x6b53fb75 ) /* Plane 2,3 */
ROM_END

ROM_START( doubledr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ddrag_p1.rom", 0x100000, 0x100000, 0x34ab832a )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "ddrag_s1.rom", 0xbef995c5 )

	NEO_BIOS_SOUND_128K( "ddrag_m1.rom", 0x10b144de )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ddrag_v1.rom", 0x000000, 0x200000, 0xcc1128e4 )
	ROM_LOAD( "ddrag_v2.rom", 0x200000, 0x200000, 0xc3ff5554 )

	NO_DELTAT_REGION

	ROM_REGION( 0xe00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ddrag_c1.rom", 0x000000, 0x200000, 0xb478c725 )
	ROM_LOAD16_BYTE( "ddrag_c2.rom", 0x000001, 0x200000, 0x2857da32 )
	ROM_LOAD16_BYTE( "ddrag_c3.rom", 0x400000, 0x200000, 0x8b0d378e )
	ROM_LOAD16_BYTE( "ddrag_c4.rom", 0x400001, 0x200000, 0xc7d2f596 )
	ROM_LOAD16_BYTE( "ddrag_c5.rom", 0x800000, 0x200000, 0xec87bff6 )
	ROM_LOAD16_BYTE( "ddrag_c6.rom", 0x800001, 0x200000, 0x844a8a11 )
	ROM_LOAD16_BYTE( "ddrag_c7.rom", 0xc00000, 0x100000, 0x727c4d02 )
	ROM_LOAD16_BYTE( "ddrag_c8.rom", 0xc00001, 0x100000, 0x69a5fa37 )
ROM_END

ROM_START( pbobble )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "puzzb_p1.rom", 0x000000, 0x040000, 0x7c3c34e1 )

	NEO_SFIX_128K( "puzzb_s1.rom", 0x9caae538 )

	NEO_BIOS_SOUND_64K( "puzzb_m1.rom", 0x129e6054 )

	ROM_REGION( 0x380000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* 0x000000-0x1fffff empty */
	ROM_LOAD( "puzzb_v3.rom", 0x200000, 0x100000, 0x0840cbc4 )
	ROM_LOAD( "puzzb_v4.rom", 0x300000, 0x080000, 0x0a548948 )

	NO_DELTAT_REGION

	ROM_REGION( 0x100000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "puzzb_c5.rom", 0x000000, 0x080000, 0xe89ad494 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "puzzb_c6.rom", 0x000001, 0x080000, 0x4b42d7eb ) /* Plane 2,3 */
ROM_END

ROM_START( kof95 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kof95_p1.rom", 0x100000, 0x100000, 0x5e54cf95 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "kof95_s1.rom", 0xde716f8a )

	NEO_BIOS_SOUND_128K( "kof95_m1.rom", 0x6f2d7429 )

	ROM_REGION( 0x900000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof95_v1.rom", 0x000000, 0x400000, 0x21469561 )
	ROM_LOAD( "kof95_v2.rom", 0x400000, 0x200000, 0xb38a2803 )
	/* 600000-7fffff empty */
	ROM_LOAD( "kof95_v3.rom", 0x800000, 0x100000, 0xd683a338 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1a00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "kof95_c1.rom", 0x0400000, 0x200000, 0x33bf8657 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "kof95_c2.rom", 0x0400001, 0x200000, 0xf21908a4 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "kof95_c3.rom", 0x0c00000, 0x200000, 0x0cee1ddb ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "kof95_c4.rom", 0x0c00001, 0x200000, 0x729db15d ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "kof95_c5.rom", 0x1000000, 0x200000, 0x8a2c1edc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof95_c6.rom", 0x1000001, 0x200000, 0xf593ac35 ) /* Plane 2,3 */
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "kof95_c7.rom", 0x1800000, 0x100000, 0x9904025f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof95_c8.rom", 0x1800001, 0x100000, 0x78eb0f9b ) /* Plane 2,3 */
ROM_END

ROM_START( tws96 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "tecmo_p1.rom", 0x000000, 0x100000, 0x03e20ab6 )

	NEO_SFIX_128K( "tecmo_s1.rom", 0x6f5e2b3a )

	NEO_BIOS_SOUND_64K( "tecmo_m1.rom", 0x860ba8c7 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "tecmo_v1.rom", 0x000000, 0x200000, 0x97bf1986 )
	ROM_LOAD( "tecmo_v2.rom", 0x200000, 0x200000, 0xb7eb05df )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "tecmo_c1.rom", 0x400000, 0x200000, 0xd301a867 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "tecmo_c2.rom", 0x400001, 0x200000, 0x305fc74f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "tecmo_c3.rom", 0x800000, 0x100000, 0x750ddc0c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "tecmo_c4.rom", 0x800001, 0x100000, 0x7a6e7d82 ) /* Plane 2,3 */
ROM_END

ROM_START( samsho3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sams3_p1.rom", 0x000000, 0x100000, 0x282a336e )
	ROM_LOAD16_WORD_SWAP( "sams3_p2.rom", 0x100000, 0x200000, 0x9bbe27e0 )

	NEO_SFIX_128K( "sams3_s1.rom", 0x74ec7d9f )

	NEO_BIOS_SOUND_128K( "sams3_m1.rom", 0x8e6440eb )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sams3_v1.rom", 0x000000, 0x400000, 0x84bdd9a0 )
	ROM_LOAD( "sams3_v2.rom", 0x400000, 0x200000, 0xac0f261a )

	NO_DELTAT_REGION

	ROM_REGION( 0x1a00000, REGION_GFX2, 0 )	/* lowering this to 0x1900000 corrupts the graphics */
	ROM_LOAD16_BYTE( "sams3_c1.rom", 0x0400000, 0x200000, 0xe079f767 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "sams3_c2.rom", 0x0400001, 0x200000, 0xfc045909 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "sams3_c3.rom", 0x0c00000, 0x200000, 0xc61218d7 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "sams3_c4.rom", 0x0c00001, 0x200000, 0x054ec754 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "sams3_c5.rom", 0x1400000, 0x200000, 0x05feee47 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "sams3_c6.rom", 0x1400001, 0x200000, 0xef7d9e29 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "sams3_c7.rom", 0x1800000, 0x080000, 0x7a01f666 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sams3_c8.rom", 0x1800001, 0x080000, 0xffd009c2 ) /* Plane 2,3 */
ROM_END

ROM_START( stakwin )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "stakw_p1.rom",  0x100000, 0x100000, 0xbd5814f6 )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "stakw_s1.rom", 0x073cb208 )

	NEO_BIOS_SOUND_128K( "stakw_m1.rom", 0x2fe1f499 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "stakw_v1.rom", 0x000000, 0x200000, 0xb7785023 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "stakw_c1.rom", 0x000000, 0x200000, 0x6e733421 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "stakw_c2.rom", 0x000001, 0x200000, 0x4d865347 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "stakw_c3.rom", 0x400000, 0x200000, 0x8fa5a9eb ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "stakw_c4.rom", 0x400001, 0x200000, 0x4604f0dc ) /* Plane 2,3 */
ROM_END

ROM_START( pulstar )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "pstar_p1.rom", 0x000000, 0x100000, 0x5e5847a2 )
	ROM_LOAD16_WORD_SWAP( "pstar_p2.rom", 0x100000, 0x200000, 0x028b774c )

	NEO_SFIX_128K( "pstar_s1.rom", 0xc79fc2c8 )

	NEO_BIOS_SOUND_128K( "pstar_m1.rom", 0xff3df7c7 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "pstar_v1.rom", 0x000000, 0x400000, 0xb458ded2 )
	ROM_LOAD( "pstar_v2.rom", 0x400000, 0x400000, 0x9d2db551 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "pstar_c1.rom", 0x0400000, 0x200000, 0x63020fc6 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "pstar_c2.rom", 0x0400001, 0x200000, 0x260e9d4d ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "pstar_c3.rom", 0x0c00000, 0x200000, 0x21ef41d7 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "pstar_c4.rom", 0x0c00001, 0x200000, 0x3b9e288f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "pstar_c5.rom", 0x1400000, 0x200000, 0x6fe9259c ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "pstar_c6.rom", 0x1400001, 0x200000, 0xdc32f2b4 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "pstar_c7.rom", 0x1800000, 0x200000, 0x6a5618ca ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "pstar_c8.rom", 0x1800001, 0x200000, 0xa223572d ) /* Plane 2,3 */
ROM_END

ROM_START( whp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "whp_p1.rom", 0x100000, 0x100000, 0xafaa4702 )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "whp_s1.rom",  0x174a880f )

	NEO_BIOS_SOUND_128K( "whp_m1.rom", 0x28065668 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "whp_v1.rom", 0x000000, 0x200000, 0x30cf2709 )
	ROM_LOAD( "whp_v2.rom", 0x200000, 0x200000, 0xb6527edd )
	ROM_LOAD( "whp_v3.rom", 0x400000, 0x200000, 0x1908a7ce )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "whp_c1.rom", 0x0400000, 0x200000, 0xaecd5bb1 )
	ROM_CONTINUE(      			 0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "whp_c2.rom", 0x0400001, 0x200000, 0x7566ffc0 )
	ROM_CONTINUE(      			 0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "whp_c3.rom", 0x0800000, 0x200000, 0x436d1b31 )
	ROM_LOAD16_BYTE( "whp_c4.rom", 0x0800001, 0x200000, 0xf9c8dd26 )
	/* 0c00000-0ffffff empty */
	ROM_LOAD16_BYTE( "whp_c5.rom", 0x1000000, 0x200000, 0x8e34a9f4 )
	ROM_LOAD16_BYTE( "whp_c6.rom", 0x1000001, 0x200000, 0xa43e4766 )
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "whp_c7.rom", 0x1800000, 0x200000, 0x59d97215 )
	ROM_LOAD16_BYTE( "whp_c8.rom", 0x1800001, 0x200000, 0xfc092367 )
ROM_END

ROM_START( kabukikl )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "klash_p1.rom", 0x100000, 0x100000, 0x28ec9b77 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "klash_s1.rom", 0xa3d68ee2 )

	NEO_BIOS_SOUND_128K( "klash_m1.rom", 0x91957ef6 )

	ROM_REGION( 0x700000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "klash_v1.rom", 0x000000, 0x200000, 0x69e90596 )
	ROM_LOAD( "klash_v2.rom", 0x200000, 0x200000, 0x7abdb75d )
	ROM_LOAD( "klash_v3.rom", 0x400000, 0x200000, 0xeccc98d3 )
	ROM_LOAD( "klash_v4.rom", 0x600000, 0x100000, 0xa7c9c949 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "klash_c1.rom", 0x400000, 0x200000, 0x4d896a58 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "klash_c2.rom", 0x400001, 0x200000, 0x3cf78a18 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "klash_c3.rom", 0xc00000, 0x200000, 0x58c454e7 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "klash_c4.rom", 0xc00001, 0x200000, 0xe1a8aa6a ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( neobombe )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "bombm_p1.rom", 0x000000, 0x100000, 0xa1a71d0d )

	NEO_SFIX_128K( "bombm_s1.rom", 0x4b3fa119 )

	NEO_BIOS_SOUND_128K( "bombm_m1.rom", 0xe81e780b )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "bombm_v1.rom", 0x200000, 0x200000, 0x43057e99 )
	ROM_CONTINUE(			  0x000000, 0x200000 )
	ROM_LOAD( "bombm_v2.rom", 0x400000, 0x200000, 0xa92b8b3d )

	NO_DELTAT_REGION

	ROM_REGION( 0x900000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "bombm_c1.rom", 0x400000, 0x200000, 0xb90ebed4 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "bombm_c2.rom", 0x400001, 0x200000, 0x41e62b4f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "bombm_c3.rom", 0x800000, 0x080000, 0xe37578c5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "bombm_c4.rom", 0x800001, 0x080000, 0x59826783 ) /* Plane 2,3 */
ROM_END

ROM_START( gowcaizr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "vfgow_p1.rom", 0x100000, 0x100000, 0x33019545 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "vfgow_s1.rom", 0x2f8748a2 )

	NEO_BIOS_SOUND_128K( "vfgow_m1.rom", 0x78c851cb )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "vfgow_v1.rom", 0x000000, 0x200000, 0x6c31223c )
	ROM_LOAD( "vfgow_v2.rom", 0x200000, 0x200000, 0x8edb776c )
	ROM_LOAD( "vfgow_v3.rom", 0x400000, 0x100000, 0xc63b9285 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "vfgow_c1.rom", 0x000000, 0x200000, 0x042f6af5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "vfgow_c2.rom", 0x000001, 0x200000, 0x0fbcd046 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "vfgow_c3.rom", 0x400000, 0x200000, 0x58bfbaa1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "vfgow_c4.rom", 0x400001, 0x200000, 0x9451ee73 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "vfgow_c5.rom", 0x800000, 0x200000, 0xff9cf48c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "vfgow_c6.rom", 0x800001, 0x200000, 0x31bbd918 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "vfgow_c7.rom", 0xc00000, 0x200000, 0x2091ec04 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "vfgow_c8.rom", 0xc00001, 0x200000, 0x0d31dee6 ) /* Plane 2,3 */
ROM_END

ROM_START( rbff1 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "095-p1.bin", 0x000000, 0x100000, 0x63b4d8ae )
	ROM_LOAD16_WORD_SWAP( "095-p2.bin", 0x100000, 0x200000, 0xcc15826e )

	NEO_SFIX_128K( "095-s1.bin", 0xb6bf5e08 )

	NEO_BIOS_SOUND_128K( "095-m1.bin", 0x653492a7 )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "069-v1.bin", 0x000000, 0x400000, 0x2bdbd4db )
	ROM_LOAD( "069-v2.bin", 0x400000, 0x400000, 0xa698a487 )
	ROM_LOAD( "095-v3.bin", 0x800000, 0x400000, 0x189d1c6c )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "069-c1.bin", 0x0000000, 0x400000, 0xe302f93c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c2.bin", 0x0000001, 0x400000, 0x1053a455 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "069-c3.bin", 0x0800000, 0x400000, 0x1c0fde2f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "069-c4.bin", 0x0800001, 0x400000, 0xa25fc3d0 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "095-c5.bin", 0x1000000, 0x400000, 0x8b9b65df ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "095-c6.bin", 0x1000001, 0x400000, 0x3e164718 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "095-c7.bin", 0x1800000, 0x200000, 0xca605e12 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "095-c8.bin", 0x1800001, 0x200000, 0x4e6beb6c ) /* Plane 2,3 */
ROM_END

ROM_START( aof3 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "aof3_p1.rom", 0x000000, 0x100000, 0x9edb420d )
	ROM_LOAD16_WORD_SWAP( "aof3_p2.rom", 0x100000, 0x200000, 0x4d5a2602 )

	NEO_SFIX_128K( "aof3_s1.rom", 0xcc7fd344 )

	NEO_BIOS_SOUND_128K( "aof3_m1.rom", 0xcb07b659 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "aof3_v1.rom", 0x000000, 0x200000, 0xe2c32074 )
	ROM_LOAD( "aof3_v2.rom", 0x200000, 0x200000, 0xa290eee7 )
	ROM_LOAD( "aof3_v3.rom", 0x400000, 0x200000, 0x199d12ea )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "aof3_c1.rom", 0x0400000, 0x200000, 0xf6c74731 ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "aof3_c2.rom", 0x0400001, 0x200000, 0xf587f149 ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "aof3_c3.rom", 0x0c00000, 0x200000, 0x7749f5e6 ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "aof3_c4.rom", 0x0c00001, 0x200000, 0xcbd58369 ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "aof3_c5.rom", 0x1400000, 0x200000, 0x1718bdcd ) /* Plane 0,1 */
	ROM_CONTINUE(      			  0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "aof3_c6.rom", 0x1400001, 0x200000, 0x4fca967f ) /* Plane 2,3 */
	ROM_CONTINUE(      			  0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "aof3_c7.rom", 0x1800000, 0x200000, 0x51bd8ab2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "aof3_c8.rom", 0x1800001, 0x200000, 0x9a34f99c ) /* Plane 2,3 */
ROM_END

ROM_START( sonicwi3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sonw3_p1.rom", 0x100000, 0x100000, 0x0547121d )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "sonw3_s1.rom", 0x8dd66743 )

	NEO_BIOS_SOUND_128K( "sonw3_m1.rom", 0xb20e4291 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sonw3_v1.rom", 0x000000, 0x400000, 0x6f885152 )
	ROM_LOAD( "sonw3_v2.rom", 0x400000, 0x100000, 0x32187ccd )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sonw3_c1.rom", 0x400000, 0x200000, 0x3ca97864 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "sonw3_c2.rom", 0x400001, 0x200000, 0x1da4b3a9 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "sonw3_c3.rom", 0x800000, 0x200000, 0xc339fff5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sonw3_c4.rom", 0x800001, 0x200000, 0x84a40c6e ) /* Plane 2,3 */
ROM_END

ROM_START( turfmast )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "turfm_p1.rom",  0x100000, 0x100000, 0x28c83048 )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "turfm_s1.rom", 0x9a5402b2 )

	NEO_BIOS_SOUND_128K( "turfm_m1.rom", 0x9994ac00 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "turfm_v1.rom", 0x000000, 0x200000, 0x00fd48d2 )
	ROM_LOAD( "turfm_v2.rom", 0x200000, 0x200000, 0x082acb31 )
	ROM_LOAD( "turfm_v3.rom", 0x400000, 0x200000, 0x7abca053 )
	ROM_LOAD( "turfm_v4.rom", 0x600000, 0x200000, 0x6c7b4902 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "turfm_c1.rom", 0x400000, 0x200000, 0x8c6733f2 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "turfm_c2.rom", 0x400001, 0x200000, 0x596cc256 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
ROM_END

ROM_START( mslug )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "mslug_p1.rom", 0x100000, 0x100000, 0x08d8daa5 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "mslug_s1.rom", 0x2f55958d )

	NEO_BIOS_SOUND_128K( "mslug_m1.rom", 0xc28b3253 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "mslug_v1.rom", 0x000000, 0x400000, 0x23d22ed1 )
	ROM_LOAD( "mslug_v2.rom", 0x400000, 0x400000, 0x472cf9db )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "mslug_c1.rom", 0x400000, 0x200000, 0xd00bd152 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "mslug_c2.rom", 0x400001, 0x200000, 0xddff1dea ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "mslug_c3.rom", 0xc00000, 0x200000, 0xd3d5f9e5 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "mslug_c4.rom", 0xc00001, 0x200000, 0x5ac1d497 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( puzzledp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "pdpon_p1.rom", 0x000000, 0x080000, 0x2b61415b )

	NEO_SFIX_64K( "pdpon_s1.rom", 0x4a421612 )

	NEO_BIOS_SOUND_128K( "pdpon_m1.rom", 0x9c0291ea )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "pdpon_v1.rom", 0x000000, 0x080000, 0xdebeb8fb )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "pdpon_c1.rom", 0x000000, 0x100000, 0xcc0095ef ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "pdpon_c2.rom", 0x000001, 0x100000, 0x42371307 ) /* Plane 2,3 */
ROM_END

ROM_START( mosyougi )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "syoug_p1.rom", 0x000000, 0x100000, 0x7ba70e2d )

	NEO_SFIX_128K( "syoug_s1.rom", 0x4e132fac )

	NEO_BIOS_SOUND_128K( "syoug_m1.rom", 0xa602c2c2 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "syoug_v1.rom", 0x000000, 0x200000, 0xbaa2b9a5 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "syoug_c1.rom",  0x000000, 0x200000, 0xbba9e8c0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "syoug_c2.rom",  0x000001, 0x200000, 0x2574be03 ) /* Plane 2,3 */
ROM_END

ROM_START( marukodq )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "maru-p1.rom", 0x000000, 0x100000, 0xc33ed21e )

	NEO_SFIX_32K( "maru-s1.rom", 0x3b52a219 )

	NEO_BIOS_SOUND_128K( "maru-m1.rom", 0x0e22902e )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "maru-v1.rom", 0x000000, 0x200000, 0x5385eca8 )
	ROM_LOAD( "maru-v2.rom", 0x200000, 0x200000, 0xf8c55404 )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "maru-c1.rom", 0x000000, 0x400000, 0x4bd5e70f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "maru-c2.rom", 0x000001, 0x400000, 0x67dbe24d ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "maru-c3.rom", 0x800000, 0x100000, 0x79aa2b48 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "maru-c4.rom", 0x800001, 0x100000, 0x55e1314d ) /* Plane 2,3 */
ROM_END

ROM_START( neomrdo )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "neomd-p1.rom", 0x000000, 0x80000, 0x39efdb82 )

	NEO_SFIX_64K( "neomd-s1.rom", 0x6c4b09c4 )

	NEO_BIOS_SOUND_128K( "neomd-m1.rom", 0x81eade02 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "neomd-v1.rom", 0x000000, 0x200000, 0x4143c052 )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "neomd-c1.rom", 0x000000, 0x200000, 0xc7541b9d )
	ROM_LOAD16_BYTE( "neomd-c2.rom", 0x000001, 0x200000, 0xf57166d2 )
ROM_END

ROM_START( sdodgeb )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "dodge_p1.rom", 0x100000, 0x100000, 0x127f3d32 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "dodge_s1.rom", 0x64abd6b3 )

	NEO_BIOS_SOUND_128K( "dodge_m1.rom", 0x0a5f3325 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "dodge_v1.rom", 0x000000, 0x200000, 0x8b53e945 )
	ROM_LOAD( "dodge_v2.rom", 0x200000, 0x200000, 0xaf37ebf8 )

	NO_DELTAT_REGION

	ROM_REGION( 0x0c00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "dodge_c1.rom", 0x0000000, 0x400000, 0x93d8619b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "dodge_c2.rom", 0x0000001, 0x400000, 0x1c737bb6 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "dodge_c3.rom", 0x0800000, 0x200000, 0x14cb1703 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "dodge_c4.rom", 0x0800001, 0x200000, 0xc7165f19 ) /* Plane 2,3 */
ROM_END

ROM_START( goalx3 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "goal!_p1.rom", 0x100000, 0x100000, 0x2a019a79 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "goal!_s1.rom", 0xc0eaad86 )

	NEO_BIOS_SOUND_64K( "goal!_m1.rom", 0xdd945773 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "goal!_v1.rom", 0x000000, 0x200000, 0xef214212 )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "goal!_c1.rom", 0x400000, 0x200000, 0xd061f1f5 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "goal!_c2.rom", 0x400001, 0x200000, 0x3f63c1a2 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "goal!_c3.rom", 0x800000, 0x100000, 0x5f91bace ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "goal!_c4.rom", 0x800001, 0x100000, 0x1e9f76f2 ) /* Plane 2,3 */
ROM_END

ROM_START( overtop )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ovr_p1.rom", 0x100000, 0x100000, 0x16c063a9 )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "ovr_s1.rom",  0x481d3ddc )

	NEO_BIOS_SOUND_128K( "ovr_m1.rom", 0xfcab6191 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ovr_v1.rom", 0x000000, 0x400000, 0x013d4ef9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ovr_c1.rom", 0x0000000, 0x400000, 0x50f43087 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ovr_c2.rom", 0x0000001, 0x400000, 0xa5b39807 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ovr_c3.rom", 0x0800000, 0x400000, 0x9252ea02 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ovr_c4.rom", 0x0800001, 0x400000, 0x5f41a699 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ovr_c5.rom", 0x1000000, 0x200000, 0xfc858bef ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ovr_c6.rom", 0x1000001, 0x200000, 0x0589c15e ) /* Plane 2,3 */
ROM_END

ROM_START( neodrift )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "drift_p1.rom",  0x100000, 0x100000, 0xe397d798 )
	ROM_CONTINUE(						 0x000000, 0x100000)

	NEO_SFIX_128K( "drift_s1.rom", 0xb76b61bc )

	NEO_BIOS_SOUND_128K( "drift_m1.rom", 0x200045f1 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "drift_v1.rom", 0x000000, 0x200000, 0xa421c076 )
	ROM_LOAD( "drift_v2.rom", 0x200000, 0x200000, 0x233c7dd9 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "drift_c1.rom", 0x400000, 0x200000, 0x62c5edc9 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "drift_c2.rom", 0x400001, 0x200000, 0x9dc9c72a ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
ROM_END

ROM_START( kof96 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kof96_p1.rom", 0x000000, 0x100000, 0x52755d74 )
	ROM_LOAD16_WORD_SWAP( "kof96_p2.rom", 0x100000, 0x200000, 0x002ccb73 )

	NEO_SFIX_128K( "kof96_s1.rom", 0x1254cbdb )

	NEO_BIOS_SOUND_128K( "kof96_m1.rom", 0xdabc427c )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof96_v1.rom", 0x000000, 0x400000, 0x63f7b045 )
	ROM_LOAD( "kof96_v2.rom", 0x400000, 0x400000, 0x25929059 )
	ROM_LOAD( "kof96_v3.rom", 0x800000, 0x200000, 0x92a2257d )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "kof96_c1.rom", 0x0000000, 0x400000, 0x7ecf4aa2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof96_c2.rom", 0x0000001, 0x400000, 0x05b54f37 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof96_c3.rom", 0x0800000, 0x400000, 0x64989a65 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof96_c4.rom", 0x0800001, 0x400000, 0xafbea515 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof96_c5.rom", 0x1000000, 0x400000, 0x2a3bbd26 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof96_c6.rom", 0x1000001, 0x400000, 0x44d30dc7 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof96_c7.rom", 0x1800000, 0x400000, 0x3687331b ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof96_c8.rom", 0x1800001, 0x400000, 0xfa1461ad ) /* Plane 2,3 */
ROM_END

ROM_START( ssideki4 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "side4_p1.rom", 0x100000, 0x100000, 0x519b4ba3 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "side4_s1.rom", 0xf0fe5c36 )

	NEO_BIOS_SOUND_128K( "side4_m1.rom", 0xa932081d )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "side4_v1.rom", 0x200000, 0x200000, 0xc4bfed62 )
	ROM_CONTINUE(			  0x000000, 0x200000 )
	ROM_LOAD( "side4_v2.rom", 0x400000, 0x200000, 0x1bfa218b )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "side4_c1.rom", 0x0400000, 0x200000, 0x288a9225 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "side4_c2.rom", 0x0400001, 0x200000, 0x3fc9d1c4 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "side4_c3.rom", 0x0c00000, 0x200000, 0xfedfaebe ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "side4_c4.rom", 0x0c00001, 0x200000, 0x877a5bb2 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "side4_c5.rom", 0x1000000, 0x200000, 0x0c6f97ec ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "side4_c6.rom", 0x1000001, 0x200000, 0x329c5e1b ) /* Plane 2,3 */
ROM_END

ROM_START( kizuna )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ke_p1.rom", 0x100000, 0x100000, 0x75d2b3de )
	ROM_CONTINUE(					 0x000000, 0x100000 )

	NEO_SFIX_128K( "ke_s1.rom",   0xefdc72d7 )

	NEO_BIOS_SOUND_128K( "ke_m1.rom", 0x1b096820 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ke_v1.rom", 0x000000, 0x200000, 0x530c50fd )
	ROM_LOAD( "ke_v2.rom", 0x200000, 0x200000, 0x03667a8d )
	ROM_LOAD( "ke_v3.rom", 0x400000, 0x200000, 0x7038c2f9 )
	ROM_LOAD( "ke_v4.rom", 0x600000, 0x200000, 0x31b99bd6 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1c00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ke_c1.rom", 0x0000000, 0x200000, 0x763ba611 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ke_c2.rom", 0x0000001, 0x200000, 0xe05e8ca6 ) /* Plane 2,3 */
	/* 400000-7fffff empty */
	ROM_LOAD16_BYTE( "ke_c3.rom", 0x0800000, 0x400000, 0x665c9f16 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ke_c4.rom", 0x0800001, 0x400000, 0x7f5d03db ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ke_c5.rom", 0x1000000, 0x200000, 0x59013f9e ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ke_c6.rom", 0x1000001, 0x200000, 0x1c8d5def ) /* Plane 2,3 */
	/* 1400000-17fffff empty */
	ROM_LOAD16_BYTE( "ke_c7.rom", 0x1800000, 0x200000, 0xc88f7035 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ke_c8.rom", 0x1800001, 0x200000, 0x484ce3ba ) /* Plane 2,3 */
ROM_END

ROM_START( ninjamas )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ninjm_p1.rom", 0x000000, 0x100000, 0x3e97ed69 )
	ROM_LOAD16_WORD_SWAP( "ninjm_p2.rom", 0x100000, 0x200000, 0x191fca88 )

	NEO_SFIX_128K( "ninjm_s1.rom", 0x8ff782f0 )

	NEO_BIOS_SOUND_128K( "ninjm_m1.rom", 0xd00fb2af )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ninjm_v1.rom", 0x000000, 0x400000, 0x1c34e013 )
	ROM_LOAD( "ninjm_v2.rom", 0x400000, 0x200000, 0x22f1c681 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ninjm_c1.rom", 0x0400000, 0x200000, 0x58f91ae0 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "ninjm_c2.rom", 0x0400001, 0x200000, 0x4258147f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "ninjm_c3.rom", 0x0c00000, 0x200000, 0x36c29ce3 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "ninjm_c4.rom", 0x0c00001, 0x200000, 0x17e97a6e ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "ninjm_c5.rom", 0x1400000, 0x200000, 0x4683ffc0 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "ninjm_c6.rom", 0x1400001, 0x200000, 0xde004f4a ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "ninjm_c7.rom", 0x1c00000, 0x200000, 0x3e1885c0 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "ninjm_c8.rom", 0x1c00001, 0x200000, 0x5a5df034 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( ragnagrd )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "rgard_p1.rom", 0x100000, 0x100000, 0xca372303 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "rgard_s1.rom", 0x7d402f9a )

	NEO_BIOS_SOUND_128K( "rgard_m1.rom", 0x17028bcf )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "rgard_v1.rom", 0x000000, 0x400000, 0x61eee7f4 )
	ROM_LOAD( "rgard_v2.rom", 0x400000, 0x400000, 0x6104e20b )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "rgard_c1.rom", 0x0400000, 0x200000, 0x18f61d79 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "rgard_c2.rom", 0x0400001, 0x200000, 0xdbf4ff4b ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "rgard_c3.rom", 0x0c00000, 0x200000, 0x108d5589 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "rgard_c4.rom", 0x0c00001, 0x200000, 0x7962d5ac ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "rgard_c5.rom", 0x1400000, 0x200000, 0x4b74021a ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "rgard_c6.rom", 0x1400001, 0x200000, 0xf5cf90bc ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "rgard_c7.rom", 0x1c00000, 0x200000, 0x32189762 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "rgard_c8.rom", 0x1c00001, 0x200000, 0xd5915828 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( pgoal )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "pgoal_p1.rom", 0x100000, 0x100000, 0x6af0e574 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "pgoal_s1.rom", 0x002f3c88 )

	NEO_BIOS_SOUND_128K( "pgoal_m1.rom", 0x958efdc8 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "pgoal_v1.rom", 0x000000, 0x200000, 0x2cc1bd05 )
	ROM_LOAD( "pgoal_v2.rom", 0x200000, 0x200000, 0x06ac1d3f )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "pgoal_c1.rom", 0x0000000, 0x200000, 0x2dc69faf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "pgoal_c2.rom", 0x0000001, 0x200000, 0x5db81811 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "pgoal_c3.rom", 0x0400000, 0x200000, 0x9dbfece5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "pgoal_c4.rom", 0x0400001, 0x200000, 0xc9f4324c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "pgoal_c5.rom", 0x0800000, 0x200000, 0x5fdad0a5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "pgoal_c6.rom", 0x0800001, 0x200000, 0xf57b4a1c ) /* Plane 2,3 */
ROM_END

ROM_START( magdrop2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "drop2_p1.rom", 0x000000, 0x80000, 0x7be82353 )

	NEO_SFIX_128K( "drop2_s1.rom", 0x2a4063a3 )

	NEO_BIOS_SOUND_128K( "drop2_m1.rom", 0xbddae628 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "drop2_v1.rom", 0x000000, 0x200000, 0x7e5e53e4 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "drop2_c1.rom", 0x000000, 0x400000, 0x1f862a14 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "drop2_c2.rom", 0x000001, 0x400000, 0x14b90536 ) /* Plane 2,3 */
ROM_END

ROM_START( samsho4 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sams4_p1.rom", 0x000000, 0x100000, 0x1a5cb56d )
	ROM_LOAD16_WORD_SWAP( "sams4_p2.rom", 0x300000, 0x200000, 0x7587f09b )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "sams4_s1.rom", 0x8d3d3bf9 )

	NEO_BIOS_SOUND_128K( "sams4_m1.rom", 0x7615bc1b )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sams4_v1.rom", 0x000000, 0x400000, 0x7d6ba95f )
	ROM_LOAD( "sams4_v2.rom", 0x400000, 0x400000, 0x6c33bb5d )
	ROM_LOAD( "sams4_v3.rom", 0x800000, 0x200000, 0x831ea8c0 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sams4_c1.rom", 0x0400000, 0x200000, 0x289100fa ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "sams4_c2.rom", 0x0400001, 0x200000, 0xc2716ea0 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "sams4_c3.rom", 0x0c00000, 0x200000, 0x6659734f ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "sams4_c4.rom", 0x0c00001, 0x200000, 0x91ebea00 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "sams4_c5.rom", 0x1400000, 0x200000, 0xe22254ed ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "sams4_c6.rom", 0x1400001, 0x200000, 0x00947b2e ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "sams4_c7.rom", 0x1c00000, 0x200000, 0xe3e3b0cd ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "sams4_c8.rom", 0x1c00001, 0x200000, 0xf33967f1 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( rbffspec )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "rbffs_p1.rom", 0x000000, 0x100000, 0xf84a2d1d )
	ROM_LOAD16_WORD_SWAP( "rbffs_p2.rom", 0x300000, 0x200000, 0x27e3e54b )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "rbffs_s1.rom", 0x7ecd6e8c )

	NEO_BIOS_SOUND_128K( "rbffs_m1.rom", 0x3fee46bf )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "rbffs_v1.rom", 0x000000, 0x400000, 0x76673869 )
	ROM_LOAD( "rbffs_v2.rom", 0x400000, 0x400000, 0x7a275acd )
	ROM_LOAD( "rbffs_v3.rom", 0x800000, 0x400000, 0x5a797fd2 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "rbffs_c1.rom", 0x0400000, 0x200000, 0x436edad4 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "rbffs_c2.rom", 0x0400001, 0x200000, 0xcc7dc384 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "rbffs_c3.rom", 0x0c00000, 0x200000, 0x375954ea ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "rbffs_c4.rom", 0x0c00001, 0x200000, 0xc1a98dd7 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "rbffs_c5.rom", 0x1400000, 0x200000, 0x12c5418e ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "rbffs_c6.rom", 0x1400001, 0x200000, 0xc8ad71d5 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "rbffs_c7.rom", 0x1c00000, 0x200000, 0x5c33d1d8 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "rbffs_c8.rom", 0x1c00001, 0x200000, 0xefdeb140 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( twinspri )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sprit_p1.rom", 0x100000, 0x100000, 0x7697e445 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "sprit_s1.rom", 0xeeed5758 )

	NEO_BIOS_SOUND_128K( "sprit_m1.rom", 0x364d6f96 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sprit_v1.rom", 0x000000, 0x400000, 0xff57f088 )
	ROM_LOAD( "sprit_v2.rom", 0x400000, 0x200000, 0x7ad26599 )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sprit_c1.rom", 0x400000, 0x200000, 0x73b2a70b ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "sprit_c2.rom", 0x400001, 0x200000, 0x3a3e506c ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "sprit_c3.rom", 0x800000, 0x100000, 0xc59e4129 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sprit_c4.rom", 0x800001, 0x100000, 0xb5532e53 ) /* Plane 2,3 */
ROM_END

ROM_START( wakuwak7 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "waku7_p1.rom", 0x000000, 0x100000, 0xb14da766 )
	ROM_LOAD16_WORD_SWAP( "waku7_p2.rom", 0x100000, 0x200000, 0xfe190665 )

	NEO_SFIX_128K( "waku7_s1.rom", 0x71c4b4b5 )

	NEO_BIOS_SOUND_128K( "waku7_m1.rom", 0x0634bba6 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "waku7_v1.rom", 0x000000, 0x400000, 0x6195c6b4 )
	ROM_LOAD( "waku7_v2.rom", 0x400000, 0x400000, 0x6159c5fe )

	NO_DELTAT_REGION

	ROM_REGION( 0x1800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "waku7_c1.rom", 0x0400000, 0x200000, 0xd91d386f ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "waku7_c2.rom", 0x0400001, 0x200000, 0x36b5cf41 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "waku7_c3.rom", 0x0c00000, 0x200000, 0x02fcff2f ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "waku7_c4.rom", 0x0c00001, 0x200000, 0xcd7f1241 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "waku7_c5.rom", 0x1400000, 0x200000, 0x03d32f25 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "waku7_c6.rom", 0x1400001, 0x200000, 0xd996a90a ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
ROM_END

ROM_START( stakwin2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sw2_p1.rom", 0x100000, 0x100000, 0xdaf101d2 )
	ROM_CONTINUE(					  0x000000, 0x100000 )

	NEO_SFIX_128K( "sw2_s1.rom", 0x2a8c4462 )

	NEO_BIOS_SOUND_128K( "sw2_m1.rom", 0xc8e5e0f9 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "sw2_v1.rom", 0x000000, 0x400000, 0xb8f24181 )
	ROM_LOAD( "sw2_v2.rom", 0x400000, 0x400000, 0xee39e260 )

	NO_DELTAT_REGION

	ROM_REGION( 0xc00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sw2_c1.rom", 0x0000000, 0x400000, 0x7d6c2af4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sw2_c2.rom", 0x0000001, 0x400000, 0x7e402d39 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "sw2_c3.rom", 0x0800000, 0x200000, 0x93dfd660 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "sw2_c4.rom", 0x0800001, 0x200000, 0x7efea43a ) /* Plane 2,3 */
ROM_END

ROM_START( breakers )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "break_p1.rom", 0x100000, 0x100000, 0xed24a6e6 )
	ROM_CONTINUE(						0x000000, 0x100000 )

	NEO_SFIX_128K( "break_s1.rom", 0x076fb64c )

	NEO_BIOS_SOUND_128K( "break_m1.rom", 0x3951a1c1 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "break_v1.rom", 0x000000, 0x400000, 0x7f9ed279 )
	ROM_LOAD( "break_v2.rom", 0x400000, 0x400000, 0x1d43e420 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "break_c1.rom", 0x000000, 0x400000, 0x68d4ae76 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "break_c2.rom", 0x000001, 0x400000, 0xfdee05cd ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "break_c3.rom", 0x800000, 0x400000, 0x645077f3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "break_c4.rom", 0x800001, 0x400000, 0x63aeb74c ) /* Plane 2,3 */
ROM_END

ROM_START( miexchng )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "miex-p1.rom", 0x000000, 0x80000, 0x61be1810 )

	NEO_SFIX_128K( "miex-s1.rom", 0xfe0c0c53 )

	NEO_BIOS_SOUND_128K( "miex-m1.rom", 0xde41301b )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "miex-v1.rom", 0x000000, 0x400000, 0x113fb898 )

	NO_DELTAT_REGION

	ROM_REGION( 0x500000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "miex-c1.rom", 0x000000, 0x200000, 0x6c403ba3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "miex-c2.rom", 0x000001, 0x200000, 0x554bcd9b ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "miex-c3.rom", 0x400000, 0x080000, 0x14524eb5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "miex-c4.rom", 0x400001, 0x080000, 0x1694f171 ) /* Plane 2,3 */
ROM_END

ROM_START( kof97 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kof97_p1.rom", 0x000000, 0x100000, 0x7db81ad9 )
	ROM_LOAD16_WORD_SWAP( "kof97_p2.rom", 0x100000, 0x400000, 0x158b23f6 )

	NEO_SFIX_128K( "kof97_s1.rom", 0x8514ecf5 )

	NEO_BIOS_SOUND_128K( "kof97_m1.rom", 0x45348747 )

	ROM_REGION( 0xc00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof97_v1.rom", 0x000000, 0x400000, 0x22a2b5b5 )
	ROM_LOAD( "kof97_v2.rom", 0x400000, 0x400000, 0x2304e744 )
	ROM_LOAD( "kof97_v3.rom", 0x800000, 0x400000, 0x759eb954 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "kof97_c1.rom", 0x0000000, 0x800000, 0x5f8bf0a1 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof97_c2.rom", 0x0000001, 0x800000, 0xe4d45c81 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof97_c3.rom", 0x1000000, 0x800000, 0x581d6618 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof97_c4.rom", 0x1000001, 0x800000, 0x49bb1e68 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof97_c5.rom", 0x2000000, 0x400000, 0x34fc4e51 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof97_c6.rom", 0x2000001, 0x400000, 0x4ff4d47b ) /* Plane 2,3 */
ROM_END

ROM_START( magdrop3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "drop3_p1.rom", 0x000000, 0x100000, 0x931e17fa )

	NEO_SFIX_128K( "drop3_s1.rom", 0x7399e68a )

	NEO_BIOS_SOUND_128K( "drop3_m1.rom", 0x5beaf34e )

	ROM_REGION( 0x480000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "drop3_v1.rom", 0x000000, 0x400000, 0x58839298 )
	ROM_LOAD( "drop3_v2.rom", 0x400000, 0x080000, 0xd5e30df4 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "drop3_c1.rom", 0x400000, 0x200000, 0x734db3d6 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x000000, 0x200000 )
	ROM_LOAD16_BYTE( "drop3_c2.rom", 0x400001, 0x200000, 0xd78f50e5 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x000001, 0x200000 )
	ROM_LOAD16_BYTE( "drop3_c3.rom", 0xc00000, 0x200000, 0xec65f472 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x800000, 0x200000 )
	ROM_LOAD16_BYTE( "drop3_c4.rom", 0xc00001, 0x200000, 0xf55dddf3 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x800001, 0x200000 )
ROM_END

ROM_START( lastblad )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "lb_p1.rom", 0x000000, 0x100000, 0xcd01c06d )
	ROM_LOAD16_WORD_SWAP( "lb_p2.rom", 0x100000, 0x400000, 0x0fdc289e )

	NEO_SFIX_128K( "lb_s1.rom", 0x95561412 )

	NEO_BIOS_SOUND_128K( "lb_m1.rom", 0x087628ea )

	ROM_REGION( 0xe00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "lb_v1.rom", 0x000000, 0x400000, 0xed66b76f )
	ROM_LOAD( "lb_v2.rom", 0x400000, 0x400000, 0xa0e7f6e2 )
	ROM_LOAD( "lb_v3.rom", 0x800000, 0x400000, 0xa506e1e2 )
	ROM_LOAD( "lb_v4.rom", 0xc00000, 0x200000, 0x13583c4b )

	NO_DELTAT_REGION

	ROM_REGION( 0x2400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "lb_c1.rom", 0x0000000, 0x800000, 0x9f7e2bd3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "lb_c2.rom", 0x0000001, 0x800000, 0x80623d3c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "lb_c3.rom", 0x1000000, 0x800000, 0x91ab1a30 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "lb_c4.rom", 0x1000001, 0x800000, 0x3d60b037 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "lb_c5.rom", 0x2000000, 0x200000, 0x17bbd7ca ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "lb_c6.rom", 0x2000001, 0x200000, 0x5c35d541 ) /* Plane 2,3 */
ROM_END

ROM_START( puzzldpr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "pdpnr_p1.rom", 0x000000, 0x080000, 0xafed5de2 )

	NEO_SFIX_64K( "pdpnr_s1.rom", 0x5a68d91e )

	NEO_BIOS_SOUND_128K( "pdpon_m1.rom", 0x9c0291ea )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "pdpon_v1.rom", 0x000000, 0x080000, 0xdebeb8fb )

	NO_DELTAT_REGION

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "pdpon_c1.rom", 0x000000, 0x100000, 0xcc0095ef ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "pdpon_c2.rom", 0x000001, 0x100000, 0x42371307 ) /* Plane 2,3 */
ROM_END

ROM_START( irrmaze )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "im_p1.rom", 0x000000, 0x200000, 0x6d536c6e )

	NEO_SFIX_128K( "im_s1.rom", 0x5d1ca640 )

	ROM_REGION16_BE( 0x20000, REGION_USER1, 0 )
	/* special BIOS with trackball support */
	ROM_LOAD16_WORD_SWAP( "im_bios.rom", 0x00000, 0x020000, 0x853e6b96 )
	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )  /* we don't use the BIOS anyway... */
	ROM_LOAD( "im_m1.rom",  0x00000, 0x20000, 0x880a1abd )  /* so overwrite it with the real thing */

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "im_v1.rom", 0x000000, 0x200000, 0x5f89c3b4 )

	ROM_REGION( 0x100000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "im_v2.rom", 0x000000, 0x100000, 0x1e843567 )

	ROM_REGION( 0x0800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "im_c1.rom", 0x000000, 0x400000, 0xc1d47902 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "im_c2.rom", 0x000001, 0x400000, 0xe15f972e ) /* Plane 2,3 */
ROM_END

ROM_START( popbounc )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "pnb-p1.rom", 0x000000, 0x100000, 0xbe96e44f )

	NEO_SFIX_128K( "pnb-s1.rom", 0xb61cf595 )

	NEO_BIOS_SOUND_128K( "pnb-m1.rom", 0xd4c946dd )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "pnb-v1.rom", 0x000000, 0x200000, 0xedcb1beb )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "pnb-c1.rom", 0x000000, 0x200000, 0xeda42d66 )
	ROM_LOAD16_BYTE( "pnb-c2.rom", 0x000001, 0x200000, 0x5e633c65 )
ROM_END

ROM_START( shocktro )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "shock_p1.rom", 0x000000, 0x100000, 0x5677456f )
	ROM_LOAD16_WORD_SWAP( "shock_p2.rom", 0x300000, 0x200000, 0x646f6c76 )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "shock_s1.rom", 0x1f95cedb )

	NEO_BIOS_SOUND_128K( "shock_m1.rom", 0x075b9518 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "shock_v1.rom", 0x000000, 0x400000, 0x260c0bef )
	ROM_LOAD( "shock_v2.rom", 0x400000, 0x200000, 0x4ad7d59e )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "shock_c1.rom", 0x0400000, 0x200000, 0xaad087fc ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c2.rom", 0x0400001, 0x200000, 0x7e39df1f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c3.rom", 0x0c00000, 0x200000, 0x6682a458 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c4.rom", 0x0c00001, 0x200000, 0xcbef1f17 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c5.rom", 0x1400000, 0x200000, 0xe17762b1 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c6.rom", 0x1400001, 0x200000, 0x28beab71 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c7.rom", 0x1c00000, 0x200000, 0xa47e62d2 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c8.rom", 0x1c00001, 0x200000, 0xe8e890fb ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( shocktrj )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "238-pg1.p1",   0x000000, 0x100000, 0xefedf8dc )
	ROM_LOAD16_WORD_SWAP( "shock_p2.rom", 0x300000, 0x200000, 0x646f6c76 )
	ROM_CONTINUE(						0x100000, 0x200000 )

	NEO_SFIX_128K( "shock_s1.rom", 0x1f95cedb )

	NEO_BIOS_SOUND_128K( "shock_m1.rom", 0x075b9518 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "shock_v1.rom", 0x000000, 0x400000, 0x260c0bef )
	ROM_LOAD( "shock_v2.rom", 0x400000, 0x200000, 0x4ad7d59e )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "shock_c1.rom", 0x0400000, 0x200000, 0xaad087fc ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c2.rom", 0x0400001, 0x200000, 0x7e39df1f ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c3.rom", 0x0c00000, 0x200000, 0x6682a458 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c4.rom", 0x0c00001, 0x200000, 0xcbef1f17 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c5.rom", 0x1400000, 0x200000, 0xe17762b1 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c6.rom", 0x1400001, 0x200000, 0x28beab71 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c7.rom", 0x1c00000, 0x200000, 0xa47e62d2 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "shock_c8.rom", 0x1c00001, 0x200000, 0xe8e890fb ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( blazstar )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "bstar_p1.rom", 0x000000, 0x100000, 0x183682f8 )
	ROM_LOAD16_WORD_SWAP( "bstar_p2.rom", 0x100000, 0x200000, 0x9a9f4154 )

	NEO_SFIX_128K( "bstar_s1.rom", 0xd56cb498 )

	NEO_BIOS_SOUND_128K( "bstar_m1.rom", 0xd31a3aea )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "bstar_v1.rom", 0x000000, 0x400000, 0x1b8d5bf7 )
	ROM_LOAD( "bstar_v2.rom", 0x400000, 0x400000, 0x74cf0a70 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "bstar_c1.rom", 0x0400000, 0x200000, 0x754744e0 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0000000, 0x200000 )
	ROM_LOAD16_BYTE( "bstar_c2.rom", 0x0400001, 0x200000, 0xaf98c037 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0000001, 0x200000 )
	ROM_LOAD16_BYTE( "bstar_c3.rom", 0x0c00000, 0x200000, 0x7b39b590 ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x0800000, 0x200000 )
	ROM_LOAD16_BYTE( "bstar_c4.rom", 0x0c00001, 0x200000, 0x6e731b30 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x0800001, 0x200000 )
	ROM_LOAD16_BYTE( "bstar_c5.rom", 0x1400000, 0x200000, 0x9ceb113b ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1000000, 0x200000 )
	ROM_LOAD16_BYTE( "bstar_c6.rom", 0x1400001, 0x200000, 0x6a78e810 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1000001, 0x200000 )
	ROM_LOAD16_BYTE( "bstar_c7.rom", 0x1c00000, 0x200000, 0x50d28eca ) /* Plane 0,1 */
	ROM_CONTINUE(      			   0x1800000, 0x200000 )
	ROM_LOAD16_BYTE( "bstar_c8.rom", 0x1c00001, 0x200000, 0xcdbbb7d7 ) /* Plane 2,3 */
	ROM_CONTINUE(      			   0x1800001, 0x200000 )
ROM_END

ROM_START( rbff2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "rb2_p1.rom", 0x000000, 0x100000, 0xb6969780 )
	ROM_LOAD16_WORD_SWAP( "rb2_p2.rom", 0x100000, 0x400000, 0x960aa88d )

	NEO_SFIX_128K( "rb2_s1.rom",  0xda3b40de )

	NEO_BIOS_SOUND_256K( "rb2_m1.rom", 0xed482791 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "rb2_v1.rom", 0x000000, 0x400000, 0xf796265a )
	ROM_LOAD( "rb2_v2.rom", 0x400000, 0x400000, 0x2cb3f3bb )
	ROM_LOAD( "rb2_v3.rom", 0x800000, 0x400000, 0xdf77b7fa )
	ROM_LOAD( "rb2_v4.rom", 0xc00000, 0x400000, 0x33a356ee )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "rb2_c1.rom", 0x0000000, 0x800000, 0xeffac504 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "rb2_c2.rom", 0x0000001, 0x800000, 0xed182d44 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "rb2_c3.rom", 0x1000000, 0x800000, 0x22e0330a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "rb2_c4.rom", 0x1000001, 0x800000, 0xc19a07eb ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "rb2_c5.rom", 0x2000000, 0x800000, 0x244dff5a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "rb2_c6.rom", 0x2000001, 0x800000, 0x4609e507 ) /* Plane 2,3 */
ROM_END

ROM_START( mslug2 )
	ROM_REGION( 0x300000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ms2_p1.rom", 0x000000, 0x100000, 0x2a53c5da )
	ROM_LOAD16_WORD_SWAP( "ms2_p2.rom", 0x100000, 0x200000, 0x38883f44 )

	NEO_SFIX_128K( "ms2_s1.rom",  0xf3d32f0f )

	NEO_BIOS_SOUND_128K( "ms2_m1.rom", 0x94520ebd )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ms2_v1.rom", 0x000000, 0x400000, 0x99ec20e8 )
	ROM_LOAD( "ms2_v2.rom", 0x400000, 0x400000, 0xecb16799 )

	NO_DELTAT_REGION

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ms2_c1.rom", 0x0000000, 0x800000, 0x394b5e0d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ms2_c2.rom", 0x0000001, 0x800000, 0xe5806221 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "ms2_c3.rom", 0x1000000, 0x800000, 0x9f6bfa6f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ms2_c4.rom", 0x1000001, 0x800000, 0x7d3e306f ) /* Plane 2,3 */
ROM_END

ROM_START( kof98 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "kof98_p1.rom", 0x000000, 0x100000, 0x61ac868a )
	ROM_LOAD16_WORD_SWAP( "kof98_p2.rom", 0x100000, 0x400000, 0x980aba4c )

	NEO_SFIX_128K( "kof98_s1.rom", 0x7f7b4805 )

	NEO_BIOS_SOUND_256K( "kof98_m1.rom", 0x4e7a6b1b )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof98_v1.rom", 0x000000, 0x400000, 0xb9ea8051 )
	ROM_LOAD( "kof98_v2.rom", 0x400000, 0x400000, 0xcc11106e )
	ROM_LOAD( "kof98_v3.rom", 0x800000, 0x400000, 0x044ea4e1 )
	ROM_LOAD( "kof98_v4.rom", 0xc00000, 0x400000, 0x7985ea30 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "kof98_c1.rom", 0x0000000, 0x800000, 0xe564ecd6 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof98_c2.rom", 0x0000001, 0x800000, 0xbd959b60 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof98_c3.rom", 0x1000000, 0x800000, 0x22127b4f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof98_c4.rom", 0x1000001, 0x800000, 0x0b4fa044 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof98_c5.rom", 0x2000000, 0x800000, 0x9d10bed3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof98_c6.rom", 0x2000001, 0x800000, 0xda07b6a2 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kof98_c7.rom", 0x3000000, 0x800000, 0xf6d7a38a ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kof98_c8.rom", 0x3000001, 0x800000, 0xc823e045 ) /* Plane 2,3 */
ROM_END

ROM_START( lastbld2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "lb2_p1.rom", 0x000000, 0x100000, 0xaf1e6554 )
	ROM_LOAD16_WORD_SWAP( "lb2_p2.rom", 0x100000, 0x400000, 0xadd4a30b )

	NEO_SFIX_128K( "lb2_s1.rom", 0xc9cd2298 )

	NEO_BIOS_SOUND_128K( "lb2_m1.rom", 0xacf12d10 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "lb2_v1.rom", 0x000000, 0x400000, 0xf7ee6fbb )
	ROM_LOAD( "lb2_v2.rom", 0x400000, 0x400000, 0xaa9e4df6 )
	ROM_LOAD( "lb2_v3.rom", 0x800000, 0x400000, 0x4ac750b2 )
	ROM_LOAD( "lb2_v4.rom", 0xc00000, 0x400000, 0xf5c64ba6 )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "lb2_c1.rom",  0x0000000, 0x800000, 0x5839444d ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "lb2_c2.rom",  0x0000001, 0x800000, 0xdd087428 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "lb2_c3.rom",  0x1000000, 0x800000, 0x6054cbe0 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "lb2_c4.rom",  0x1000001, 0x800000, 0x8bd2a9d2 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "lb2_c5.rom",  0x2000000, 0x800000, 0x6a503dcf ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "lb2_c6.rom",  0x2000001, 0x800000, 0xec9c36d0 ) /* Plane 2,3 */
ROM_END

ROM_START( neocup98 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "nc98_p1.rom", 0x100000, 0x100000, 0xf8fdb7a5 )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "nc98_s1.rom", 0x9bddb697 )

	NEO_BIOS_SOUND_128K( "nc98_m1.rom", 0xa701b276 )

	ROM_REGION( 0x600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "nc98_v1.rom", 0x000000, 0x400000, 0x79def46d )
	ROM_LOAD( "nc98_v2.rom", 0x400000, 0x200000, 0xb231902f )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "nc98_c1.rom", 0x000000, 0x800000, 0xd2c40ec7 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "nc98_c2.rom", 0x000001, 0x800000, 0x33aa0f35 ) /* Plane 2,3 */
ROM_END

ROM_START( breakrev )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "brev_p1.rom", 0x100000, 0x100000, 0xc828876d )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "brev_s1.rom", 0xe7660a5d )

	NEO_BIOS_SOUND_128K( "brev_m1.rom", 0x00f31c66 )

	ROM_REGION(  0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "brev_v1.rom", 0x000000, 0x400000, 0xe255446c )
	ROM_LOAD( "brev_v2.rom", 0x400000, 0x400000, 0x9068198a )

	NO_DELTAT_REGION

	ROM_REGION( 0x1400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "break_c1.rom", 0x0000000, 0x400000, 0x68d4ae76 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "break_c2.rom", 0x0000001, 0x400000, 0xfdee05cd ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "break_c3.rom", 0x0800000, 0x400000, 0x645077f3 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "break_c4.rom", 0x0800001, 0x400000, 0x63aeb74c ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "brev_c5.rom",  0x1000000, 0x200000, 0x28ff1792 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "brev_c6.rom",  0x1000001, 0x200000, 0x23c65644 ) /* Plane 2,3 */
ROM_END

ROM_START( shocktr2 )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "st2_p1.rom", 0x000000, 0x100000, 0x6d4b7781 )
	ROM_LOAD16_WORD_SWAP( "st2_p2.rom", 0x100000, 0x400000, 0x72ea04c3 )

	NEO_SFIX_128K( "st2_s1.rom", 0x2a360637 )

	NEO_BIOS_SOUND_128K( "st2_m1.rom", 0xd0604ad1 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "st2_v1.rom", 0x000000, 0x400000, 0x16986fc6 )
	ROM_LOAD( "st2_v2.rom", 0x400000, 0x400000, 0xada41e83 )
	ROM_LOAD( "st2_v3.rom", 0x800000, 0x200000, 0xa05ba5db )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "st2_c1.rom", 0x0000000, 0x800000, 0x47ac9ec5 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "st2_c2.rom", 0x0000001, 0x800000, 0x7bcab64f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "st2_c3.rom", 0x1000000, 0x800000, 0xdb2f73e8 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "st2_c4.rom", 0x1000001, 0x800000, 0x5503854e ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "st2_c5.rom", 0x2000000, 0x800000, 0x055b3701 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "st2_c6.rom", 0x2000001, 0x800000, 0x7e2caae1 ) /* Plane 2,3 */
ROM_END

ROM_START( flipshot )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "flip_p1.rom", 0x000000, 0x080000, 0xd2e7a7e3 )

	NEO_SFIX_128K( "flip_s1.rom", 0x6300185c )

	NEO_BIOS_SOUND_128K( "flip_m1.rom", 0xa9fe0144 )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "flip_v1.rom", 0x000000, 0x200000, 0x42ec743d )

	NO_DELTAT_REGION

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "flip_c1.rom",  0x000000, 0x200000, 0xc9eedcb2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "flip_c2.rom",  0x000001, 0x200000, 0x7d6d6e87 ) /* Plane 2,3 */
ROM_END

ROM_START( pbobbl2n )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "pb2_p1.rom", 0x000000, 0x100000, 0x9d6c0754 )

	NEO_SFIX_128K( "pb2_s1.rom", 0x0a3fee41 )

	NEO_BIOS_SOUND_128K( "pb2_m1.rom", 0x883097a9 )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "pb2_v1.rom", 0x000000, 0x400000, 0x57fde1fa )
	ROM_LOAD( "pb2_v2.rom", 0x400000, 0x400000, 0x4b966ef3 )

	NO_DELTAT_REGION

	ROM_REGION( 0xa00000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "pb2_c1.rom", 0x000000, 0x400000, 0xd9115327 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "pb2_c2.rom", 0x000001, 0x400000, 0x77f9fdac ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "pb2_c3.rom", 0x800000, 0x100000, 0x8890bf7c ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "pb2_c4.rom", 0x800001, 0x100000, 0x8efead3f ) /* Plane 2,3 */
ROM_END

ROM_START( ctomaday )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ctom_p1.rom", 0x100000, 0x100000, 0xc9386118 )
	ROM_CONTINUE(					   0x000000, 0x100000 )

	NEO_SFIX_128K( "ctom_s1.rom", 0xdc9eb372 )

	NEO_BIOS_SOUND_128K( "ctom_m1.rom", 0x80328a47 )

	ROM_REGION( 0x500000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "ctom_v1.rom", 0x000000, 0x400000, 0xde7c8f27 )
	ROM_LOAD( "ctom_v2.rom", 0x400000, 0x100000, 0xc8e40119 )

	NO_DELTAT_REGION

	ROM_REGION( 0x800000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ctom_c1.rom",  0x000000, 0x400000, 0x041fb8ee ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "ctom_c2.rom",  0x000001, 0x400000, 0x74f3cdf4 ) /* Plane 2,3 */
ROM_END

ROM_START( mslugx )
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "msx_p1.rom", 0x000000, 0x100000, 0x81f1f60b )
	ROM_LOAD16_WORD_SWAP( "msx_p2.rom", 0x100000, 0x400000, 0x1fda2e12 )

	NEO_SFIX_128K( "msx_s1.rom",  0xfb6f441d )

	NEO_BIOS_SOUND_128K( "msx_m1.rom", 0xfd42a842 )

	ROM_REGION( 0xa00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "msx_v1.rom", 0x000000, 0x400000, 0xc79ede73 )
	ROM_LOAD( "msx_v2.rom", 0x400000, 0x400000, 0xea9aabe1 )
	ROM_LOAD( "msx_v3.rom", 0x800000, 0x200000, 0x2ca65102 )

	NO_DELTAT_REGION

	ROM_REGION( 0x3000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "msx_c1.rom", 0x0000000, 0x800000, 0x09a52c6f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "msx_c2.rom", 0x0000001, 0x800000, 0x31679821 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "msx_c3.rom", 0x1000000, 0x800000, 0xfd602019 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "msx_c4.rom", 0x1000001, 0x800000, 0x31354513 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "msx_c5.rom", 0x2000000, 0x800000, 0xa4b56124 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "msx_c6.rom", 0x2000001, 0x800000, 0x83e3e69d ) /* Plane 0,1 */
ROM_END

ROM_START( kof99 ) /* Original Version - Encrypted Code(?) & GFX */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	/* Current Dump of Code is Bad, although the original is thought to be encrypted anyway
	   figures at end of lines indicate size and CRC32 of current dumps                     */
	ROM_LOAD16_WORD_SWAP( "kof99_p1.rom", 0x000000, 0x100000, 0x00000000 ) /* 0x400000 0x006e4532 */
	ROM_LOAD16_WORD_SWAP( "kof99_p2.rom", 0x100000, 0x400000, 0x00000000 ) /* 0x400000 0x90175f12 */

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from elsewhere? */
	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "kof99_m1.rom", 0x5e74539c )

	ROM_REGION( 0x0c00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof99_v1.rom", 0x000000, 0x400000, 0xef2eecc8 )
	ROM_LOAD( "kof99_v2.rom", 0x400000, 0x400000, 0x73e211ca )
	ROM_LOAD( "kof99_v3.rom", 0x800000, 0x400000, 0x821901da )

	ROM_REGION( 0x0200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof99_v4.rom", 0x000000, 0x200000, 0xb49e6178 )

	ROM_REGION( 0x4000000, REGION_GFX2, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "kof99_c1.rom", 0x0000000, 0x800000, 0x0f9e93fe )
	ROM_LOAD16_BYTE( "kof99_c2.rom", 0x0000001, 0x800000, 0xe71e2ea3 )
	ROM_LOAD16_BYTE( "kof99_c3.rom", 0x1000000, 0x800000, 0x238755d2 )
	ROM_LOAD16_BYTE( "kof99_c4.rom", 0x1000001, 0x800000, 0x438c8b22 )
	ROM_LOAD16_BYTE( "kof99_c5.rom", 0x2000000, 0x800000, 0x0b0abd0a )
	ROM_LOAD16_BYTE( "kof99_c6.rom", 0x2000001, 0x800000, 0x65bbf281 )
	ROM_LOAD16_BYTE( "kof99_c7.rom", 0x3000000, 0x800000, 0xff65f62e )
	ROM_LOAD16_BYTE( "kof99_c8.rom", 0x3000001, 0x800000, 0x8d921c68 )
ROM_END

ROM_START( kof99p ) /* Prototype Version - Possibly Hacked */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	/* these program roms are possibly hacked, P1 especially, this was the earliest copy i could find..
	   I guess we'll need another prototype to verify ...                                               */
	ROM_LOAD16_WORD_SWAP( "kf99p_p1.rom", 0x000000, 0x100000, BADCRC(0x5d41395d) )
	ROM_LOAD16_WORD_SWAP( "kf99p_p2.rom", 0x100000, 0x400000, BADCRC(0x739742ad) )

	/* This is the S1 from the prototype, the final is certainly be different */
	NEO_SFIX_128K( "kf99p_s1.rom", 0xfb1498ed )

	/* Did the Prototype really use the same sound program / voice roms, sound isn't great .. */
	NEO_BIOS_SOUND_128K( "kof99_m1.rom", 0x5e74539c )

	ROM_REGION( 0x0c00000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof99_v1.rom", 0x000000, 0x400000, 0xef2eecc8 )
	ROM_LOAD( "kof99_v2.rom", 0x400000, 0x400000, 0x73e211ca )
	ROM_LOAD( "kof99_v3.rom", 0x800000, 0x400000, 0x821901da )

	ROM_REGION( 0x0200000, REGION_SOUND2, ROMREGION_SOUNDONLY )
	ROM_LOAD( "kof99_v4.rom", 0x000000, 0x200000, 0xb49e6178 )

	ROM_REGION( 0x4000000, REGION_GFX2, 0 )
	/* these are probably decrypted versions of the roms found in the final */
	ROM_LOAD16_BYTE( "kf99p_c1.rom", 0x0000000, 0x800000, 0xe5d8ffa4 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf99p_c2.rom", 0x0000001, 0x800000, 0xd822778f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf99p_c3.rom", 0x1000000, 0x800000, 0xf20959e8 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf99p_c4.rom", 0x1000001, 0x800000, 0x54ffbe9f ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf99p_c5.rom", 0x2000000, 0x800000, 0xd87a3bbc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf99p_c6.rom", 0x2000001, 0x800000, 0x4d40a691 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "kf99p_c7.rom", 0x3000000, 0x800000, 0xa4479a58 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "kf99p_c8.rom", 0x3000001, 0x800000, 0xead513ce ) /* Plane 2,3 */
ROM_END

ROM_START( ganryu ) /* Original Version - Encrypted GFX */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "252-p1.bin", 0x100000, 0x100000, 0x4b8ac4fb )
	ROM_CONTINUE(						0x000000, 0x100000 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from elsewhere? */
	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "252-m1.bin", 0x30cc4099 )

	ROM_REGION( 0x0400000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "252-v1.bin", 0x000000, 0x400000, 0xe5946733 )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	/* Encrypted */
	ROM_LOAD16_BYTE( "252-c1.bin", 0x0000000, 0x800000, 0x50ee7882 )
	ROM_LOAD16_BYTE( "252-c2.bin", 0x0000001, 0x800000, 0x62585474 )
ROM_END

ROM_START( preisle2 ) /* Original Version, Encrypted GFX Roms -NOT DUMPED- */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "pi2_p1.rom", 0x000000, 0x100000, 0xdfa3c0f3 )
	ROM_LOAD16_WORD_SWAP( "pi2_p2.rom", 0x100000, 0x400000, 0x42050b80 )

	/* The Encrypted Boards do _not_ have an s1 rom, data for it comes from elsewhere? */
	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_FILL(                 0x000000, 0x20000, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

	NEO_BIOS_SOUND_128K( "pi2_m1.rom", 0x8efd4014 )

	ROM_REGION( 0x0600000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "pi2_v1.rom", 0x000000, 0x400000, 0x5a14543d )
	ROM_LOAD( "pi2_v2.rom", 0x400000, 0x200000, 0x6610d91a )

	NO_DELTAT_REGION

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	/* this would be encrypted, and the roms aren't dumped yet. */
	ROM_LOAD16_BYTE( "pi2_c1.bin", 0x0000000, 0x800000, 0x00000000 )
	ROM_LOAD16_BYTE( "pi2_c2.bin", 0x0000001, 0x800000, 0x00000000 )
ROM_END

ROM_START( garoup ) /* Prototype Version, seems genuine */
	ROM_REGION( 0x500000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "motwp_p1.rom", 0x000000, 0x100000, 0xc72f0c16 )
	ROM_LOAD16_WORD_SWAP( "motwp_p2.rom", 0x100000, 0x400000, 0xbf8de565 )

	NEO_SFIX_128K( "motwp_s1.rom", 0x779989de )

	NEO_BIOS_SOUND_256K( "motw_m1.rom", 0xbbe464f7 )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "motw_v1.rom", 0x000000, 0x400000, 0x274f3923 )
	ROM_LOAD( "motw_v2.rom", 0x400000, 0x400000, 0x8f86dabe )
	ROM_LOAD( "motw_v3.rom", 0x800000, 0x400000, 0x05fd06cd )
	ROM_LOAD( "motw_v4.rom", 0xc00000, 0x400000, 0x14984063 )

	NO_DELTAT_REGION

	ROM_REGION( 0x4000000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "motwp_c1.rom", 0x0000000, 0x800000, 0x5bb5d137 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "motwp_c2.rom", 0x0000001, 0x800000, 0x5c8d2960 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "motwp_c3.rom", 0x1000000, 0x800000, 0x234d16fc ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "motwp_c4.rom", 0x1000001, 0x800000, 0xb9b5b993 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "motwp_c5.rom", 0x2000000, 0x800000, 0x722615d2 ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "motwp_c6.rom", 0x2000001, 0x800000, 0x0a6fab38 ) /* Plane 2,3 */
	ROM_LOAD16_BYTE( "motwp_c7.rom", 0x3000000, 0x800000, 0xd68e806f ) /* Plane 0,1 */
	ROM_LOAD16_BYTE( "motwp_c8.rom", 0x3000001, 0x800000, 0xf778fe99 ) /* Plane 2,3 */
ROM_END

/******************************************************************************/

/* dummy entry for the dummy bios driver */
ROM_START( neogeo )
	ROM_REGION16_BE( 0x020000, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )
	ROM_LOAD( "ng-sm1.rom", 0x00000, 0x20000, 0x97cf998b )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )
ROM_END

/******************************************************************************/

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the neo-geo.rom file, so that we do not have to include */
/* it in every zip file */
GAMEX( 1990, neogeo, 0, neogeo, neogeo, neogeo, ROT0, "SNK", "Neo-Geo", NOT_A_DRIVER )

/******************************************************************************/

/*	  YEAR	NAME	  PARENT	MACHINE INPUT	 INIT	 MONITOR  */

/* SNK */
GAME( 1990, nam1975,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "NAM-1975" )
GAME( 1990, bstars,   neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Baseball Stars Professional" )
GAME( 1990, tpgolf,   neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Top Player's Golf" )
GAME( 1990, mahretsu, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Mahjong Kyoretsuden" )
GAME( 1990, ridhero,  neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Riding Hero (set 1)" )
GAME( 1990, ridheroh, ridhero,  raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Riding Hero (set 2)" )
GAME( 1991, alpham2,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Alpha Mission II / ASO II - Last Guardian" )
GAME( 1990, cyberlip, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Cyber-Lip" )
GAME( 1990, superspy, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "The Super Spy" )
GAME( 1992, mutnat,   neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Mutation Nation" )
GAME( 1991, kotm,     neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "King of the Monsters" )
GAME( 1991, sengoku,  neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Sengoku / Sengoku Denshou (set 1)" )
GAME( 1991, sengokh,  sengoku,  raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Sengoku / Sengoku Denshou (set 2)" )
GAME( 1991, burningf, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Burning Fight (set 1)" )
GAME( 1991, burningh, burningf, neogeo, neogeo,  neogeo, ROT0,       "SNK", "Burning Fight (set 2)" )
GAME( 1990, lbowling, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "League Bowling" )
GAME( 1991, gpilots,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Ghost Pilots" )
GAME( 1990, joyjoy,   neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Puzzled / Joy Joy Kid" )
GAME( 1991, quizdais, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Quiz Daisousa Sen - The Last Count Down" )
GAME( 1992, lresort,  neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Last Resort" )
GAME( 1991, eightman, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK / Pallas", "Eight Man" )
GAME( 1991, legendos, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Legend of Success Joe / Ashitano Joe Densetsu" )
GAME( 1991, 2020bb,   neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK / Pallas", "2020 Super Baseball (set 1)" )
GAME( 1991, 2020bbh,  2020bb,   neogeo, neogeo,  neogeo, ROT0,       "SNK / Pallas", "2020 Super Baseball (set 2)" )
GAME( 1991, socbrawl, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Soccer Brawl" )
GAME( 1991, fatfury1, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Fatal Fury - King of Fighters / Garou Densetsu - shukumei no tatakai" )
GAME( 1991, roboarmy, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Robo Army" )
GAME( 1992, fbfrenzy, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Football Frenzy" )
GAME( 1992, kotm2,    neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "King of the Monsters 2 - The Next Thing" )
GAME( 1993, sengoku2, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Sengoku 2 / Sengoku Denshou 2")
GAME( 1992, bstars2,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Baseball Stars 2" )
GAME( 1992, quizdai2, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Quiz Meintantei Neo Geo - Quiz Daisousa Sen Part 2" )
GAME( 1993, 3countb,  neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "3 Count Bout / Fire Suplex" )
GAME( 1992, aof,      neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Art of Fighting / Ryuuko no Ken" )
GAME( 1993, samsho,   neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Samurai Shodown / Samurai Spirits" )
GAME( 1994, tophuntr, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Top Hunter - Roddy & Cathy" )
GAME( 1992, fatfury2, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Fatal Fury 2 / Garou Densetsu 2 - arata-naru tatakai" )
GAME( 1992, ssideki,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Super Sidekicks / Tokuten Ou" )
GAME( 1994, kof94,    neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "The King of Fighters '94" )
GAME( 1994, aof2,     neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Art of Fighting 2 / Ryuuko no Ken 2" )
GAME( 1993, fatfursp, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "SNK", "Fatal Fury Special / Garou Densetsu Special" )
GAME( 1995, savagere, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Savage Reign / Fu'un Mokushiroku - kakutou sousei" )
GAME( 1994, ssideki2, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Super Sidekicks 2 - The World Championship / Tokuten Ou 2 - real fight football" )
GAME( 1994, samsho2,  neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Samurai Shodown II / Shin Samurai Spirits - Haohmaru jigokuhen" )
GAME( 1995, fatfury3, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Fatal Fury 3 - Road to the Final Victory / Garou Densetsu 3 - haruka-naru tatakai" )
GAME( 1995, ssideki3, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Super Sidekicks 3 - The Next Glory / Tokuten Ou 3 - eikoue no michi" )
GAME( 1995, kof95,    neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "The King of Fighters '95" )
GAME( 1995, samsho3,  neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Samurai Shodown III / Samurai Spirits - Zankurou Musouken" )
GAME( 1995, rbff1,    neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Real Bout Fatal Fury / Real Bout Garou Densetsu" )
GAME( 1996, aof3,     neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Art of Fighting 3 - The Path of the Warrior / Art of Fighting - Ryuuko no Ken Gaiden" )
GAME( 1996, kof96,    neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "The King of Fighters '96" )
GAME( 1996, ssideki4, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "The Ultimate 11 / Tokuten Ou - Honoo no Libero" )
GAME( 1996, kizuna,   neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Kizuna Encounter - Super Tag Battle / Fu'un Super Tag Battle" )
GAME( 1996, samsho4,  neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Samurai Shodown IV - Amakusa's Revenge / Samurai Spirits - Amakusa Kourin" )
GAME( 1996, rbffspec, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Real Bout Fatal Fury Special / Real Bout Garou Densetsu Special" )
GAME( 1997, kof97,    neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "The King of Fighters '97" )
GAME( 1997, lastblad, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "The Last Blade / Bakumatsu Roman - Gekkano Kenshi" )
GAME( 1997, irrmaze,  neogeo,   neogeo, irrmaze, neogeo, ROT0,       "SNK / Saurus", "The Irritating Maze / Ultra Denryu Iraira Bou" )
GAME( 1998, rbff2,    neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Real Bout Fatal Fury 2 - The Newcomers / Real Bout Garou Densetsu 2 - the newcomers" )
GAME( 1998, mslug2,   neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Metal Slug 2 - Super Vehicle-001/II" )
GAME( 1998, kof98,    neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "The King of Fighters '98 - The Slugfest / King of Fighters '98 - dream match never ends" )
GAME( 1998, lastbld2, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "The Last Blade 2 / Bakumatsu Roman - Dai Ni Maku Gekkano Kenshi" )
GAME( 1998, neocup98, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Neo-Geo Cup '98 - The Road to the Victory" )
GAME( 1999, mslugx,   neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK", "Metal Slug X - Super Vehicle-001" )
GAMEX(1999, kof99,    neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "The King of Fighters '99 - Millennium Battle", GAME_NOT_WORKING ) /* Encrypted Code & GFX, Bad Dump */
GAME( 1999, kof99p,   kof99,    raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "The King of Fighters '99 - Millennium Battle (prototype)" )
GAME( 1999, garoup,   neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "SNK", "Garou - Mark of the Wolves (prototype)" )
GAMEX(1999, preisle2, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "SNK / Yumekobo", "Prehistoric Isle 2", GAME_NOT_WORKING ) /* Encrypted GFX, Incomplete Dump */

/* Alpha Denshi Co. / ADK (changed name in 1993) */
GAME( 1990, maglord,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Alpha Denshi Co.", "Magician Lord (set 1)" )
GAME( 1990, maglordh, maglord,  neogeo, neogeo,  neogeo, ROT0,       "Alpha Denshi Co.", "Magician Lord (set 2)" )
GAME( 1990, ncombat,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Alpha Denshi Co.", "Ninja Combat" )
GAME( 1990, bjourney, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Alpha Denshi Co.", "Blue's Journey / Raguy" )
GAME( 1991, crsword,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Alpha Denshi Co.", "Crossed Swords" )
GAME( 1991, trally,   neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Alpha Denshi Co.", "Thrash Rally" )
GAME( 1992, ncommand, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Alpha Denshi Co.", "Ninja Commando" )
GAME( 1992, wh1,      neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Alpha Denshi Co.", "World Heroes" )
GAME( 1993, wh2,      neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "ADK",              "World Heroes 2" )
GAME( 1994, wh2j,     neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "ADK / SNK",        "World Heroes 2 Jet" )
GAME( 1994, aodk,     neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "ADK / SNK",        "Aggressors of Dark Kombat / Tsuukai GANGAN Koushinkyoku" )
GAME( 1995, whp,      neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "ADK / SNK",        "World Heroes Perfect" )
GAME( 1995, mosyougi, neogeo,   raster_busy, neogeo,  neogeo, ROT0_16BIT, "ADK / SNK",        "Syougi No Tatsujin - Master of Syougi" )
GAME( 1996, overtop,  neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "ADK",              "Over Top" )
GAME( 1996, ninjamas, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "ADK / SNK",        "Ninja Master's - haoh-ninpo-cho" )
GAME( 1996, twinspri, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "ADK",              "Twinkle Star Sprites" )

/* Aicom */
GAME( 1994, janshin,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Aicom", "Jyanshin Densetsu - Quest of Jongmaster" )
GAME( 1995, pulstar,  neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Aicom", "Pulstar" )

/* Data East Corporation */
GAME( 1993, spinmast, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Data East Corporation", "Spinmaster / Miracle Adventure" )
GAME( 1994, wjammers, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Data East Corporation", "Windjammers / Flying Power Disc" )
GAME( 1994, karnovr,  neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Data East Corporation", "Karnov's Revenge / Fighter's History Dynamite" )
GAME( 1994, strhoop,  neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Data East Corporation", "Street Hoop / Street Slam / Dunk Dream" )
GAME( 1996, magdrop2, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Data East Corporation", "Magical Drop II" )
GAME( 1997, magdrop3, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Data East Corporation", "Magical Drop III" )

/* Face */
GAME( 1994, gururin,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Face", "Gururin" )
GAME( 1997, miexchng, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Face", "Money Puzzle Exchanger / Money Idol Exchanger" )

/* Hudson Soft */
GAME( 1994, panicbom, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Eighting / Hudson", "Panic Bomber" )
GAME( 1995, kabukikl, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Hudson", "Kabuki Klash - Far East of Eden / Tengai Makyou Shinden - Far East of Eden" )
GAME( 1997, neobombe, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Hudson", "Neo Bomberman" )

/* Monolith Corp. */
GAME( 1990, minasan,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Monolith Corp.", "Minnasanno Okagesamadesu" )
GAME( 1991, bakatono, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Monolith Corp.", "Bakatonosama Mahjong Manyuki" )

/* Nazca */
GAME( 1996, turfmast, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Nazca", "Neo Turf Masters / Big Tournament Golf" )
GAME( 1996, mslug,    neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Nazca", "Metal Slug - Super Vehicle-001" )

/* NMK */
GAME( 1994, zedblade, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "NMK", "Zed Blade / Operation Ragnarok" )

/* Sammy */
GAME( 1992, viewpoin, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Sammy", "Viewpoint" )

/* Saurus */
GAME( 1995, quizkof,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Saurus", "Quiz King of Fighters" )
GAME( 1995, stakwin,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Saurus", "Stakes Winner / Stakes Winner - GI kinzen seihae no michi" )
GAME( 1996, ragnagrd, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Saurus", "Operation Ragnagard / Shin-Oh-Ken" )
GAME( 1996, pgoal,    neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Saurus", "Pleasure Goal / Futsal - 5 on 5 Mini Soccer" )
GAME( 1996, stakwin2, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Saurus", "Stakes Winner 2" )
GAME( 1997, shocktro, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Saurus", "Shock Troopers" )
GAME( 1997, shocktrj, shocktro, neogeo, neogeo,  neogeo, ROT0_16BIT, "Saurus", "Shock Troopers (Japan)" )
GAME( 1998, shocktr2, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Saurus", "Shock Troopers - 2nd Squad" )

/* Sunsoft */
GAME( 1995, galaxyfg, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Sunsoft", "Galaxy Fight - Universal Warriors" )
GAME( 1996, wakuwak7, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Sunsoft", "Waku Waku 7" )

/* Taito */
GAME( 1994, pbobble,  neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Taito", "Puzzle Bobble / Bust-A-Move (Neo-Geo)" )
GAME( 1999, pbobbl2n, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Taito (SNK license)", "Puzzle Bobble 2 / Bust-A-Move Again (Neo-Geo)" )

/* Takara */
GAME( 1995, marukodq, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Takara", "Chibi Marukochan Deluxe Quiz" )

/* Technos */
GAME( 1995, doubledr, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Technos", "Double Dragon (Neo-Geo)" )
GAME( 1995, gowcaizr, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Technos", "Voltage Fighter - Gowcaizer / Choujin Gakuen Gowcaizer")
GAME( 1996, sdodgeb,  neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Technos", "Super Dodge Ball / Kunio no Nekketsu Toukyuu Densetsu" )

/* Tecmo */
GAME( 1996, tws96,    neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Tecmo", "Tecmo World Soccer '96" )

/* Yumekobo */
GAME( 1998, blazstar, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Yumekobo", "Blazing Star" )

/* Viccom */
GAME( 1994, fightfev, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Viccom", "Fight Fever / Crystal Legacy" )

/* Video System Co. */
GAME( 1994, pspikes2, neogeo,   raster, neogeo,  neogeo, ROT0_16BIT, "Video System Co.", "Power Spikes II" )
GAME( 1994, sonicwi2, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Video System Co.", "Aero Fighters 2 / Sonic Wings 2" )
GAME( 1995, sonicwi3, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Video System Co.", "Aero Fighters 3 / Sonic Wings 3" )
GAME( 1997, popbounc, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Video System Co.", "Pop 'n Bounce / Gapporin" )

/* Visco */
GAME( 1992, androdun, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Visco", "Andro Dunos" )
GAME( 1995, puzzledp, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Taito (Visco license)", "Puzzle De Pon" )
GAME( 1996, neomrdo,  neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Visco", "Neo Mr. Do!" )
GAME( 1995, goalx3,   neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Visco", "Goal! Goal! Goal!" )
GAME( 1996, neodrift, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Visco", "Neo Drift Out - New Technology" )
GAME( 1996, breakers, neogeo,   neogeo, neogeo,  neogeo, ROT0_16BIT, "Visco", "Breakers" )
GAME( 1997, puzzldpr, puzzledp, neogeo, neogeo,  neogeo, ROT0,       "Taito (Visco license)", "Puzzle De Pon R" )
GAME( 1998, breakrev, breakers, neogeo, neogeo,  neogeo, ROT0_16BIT, "Visco", "Breakers Revenge")
GAME( 1998, flipshot, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Visco", "Battle Flip Shot" )
GAME( 1999, ctomaday, neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Visco", "Captain Tomaday" )
GAMEX(1999, ganryu,   neogeo,   neogeo, neogeo,  neogeo, ROT0,       "Visco", "Musashi Ganryuuki", GAME_NOT_WORKING )
