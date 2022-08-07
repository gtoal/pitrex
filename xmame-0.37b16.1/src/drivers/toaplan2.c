/*****************************************************************************

		ToaPlan and Raizing game hardware from 1991-1994
		-------------------------------------------------
		Driver by: Quench and Yochizo


   Raizing games and Truxton 2 are heavily dependent on the Raine source.
   Many thanks to Richard Bush and the Raine team. [Yochizo]

 * Battle Garegga and Armed Police Batrider have secret characters.   *
 * Try to input the following commands to use them.                   *
 * ----------------------------------------------------------------   *
 * Battle Garegga                                                     *
 *   After inserting a coin (pushing a credit button), input          *
 *     UP  UP  DOWN  DOWN  LEFT  RIGHT  LEFT  RIGHT  A  B  C  START   *
 *   then you can use Mahou Daisakusen characters.                    *
 *                                                                    *
 * Armed Police Batrider                                              *
 *   After inserting a coin (pushing a credit button), input          *
 *     UP  UP  DOWN  DOWN  LEFT  RIGHT  LEFT  RIGHT  A  B  START      *
 *   then you can use Mahou Daisakusen and Battle Garegga characters. *



Supported games:

	Name		Board No	Maker			Game name
	----------------------------------------------------------------------------
	tekipaki	TP-020		Toaplan			Teki Paki
	ghox		TP-021		Toaplan			Ghox
	dogyuun		TP-022		Toaplan			Dogyuun
	kbash		TP-023		Toaplan			Knuckle Bash
	truxton2	TP-024		Toaplan			Truxton 2 / Tatsujin 2
	pipibibs	TP-025		Toaplan			Pipi & Bibis
	whoopee		TP-025		Toaplan			Whoopee
	pipibibi	bootleg?	Toaplan			Pipi & Bibis
	fixeight	TP-026		Toaplan			FixEight
	grindstm	TP-027		Toaplan			Grind Stormer  (1992)
	vfive		TP-027		Toaplan			V-V  (V-Five)  (1993 - Japan only)
	batsugun	TP-030		Toaplan			Batsugun
	batugnsp	TP-030		Toaplan			Batsugun  (Special Version)
	snowbro2	??????		Toaplan			Snow Bros. 2 - With New Elves

	mahoudai	??????		Raizing			Mahou Daisakusen
	shippumd	??????		Raizing/8ing	Shippu Mahou Daisakusen
	battleg 	RA9503		Raizing/8ing	Battle Garegga
	batridra	??????		Raizing/8ing	Armed Police Batrider (Rev A)
	batrider	??????		Raizing/8ing	Armed Police Batrider (Rev B)

Game status:

Teki Paki                      Working, but no sound. Missing sound MCU dump
Ghox                           Working, but no sound. Missing sound MCU dump
Dogyuun                        Working, but no sound. MCU type unknown - its a Z?80 of some sort.
Knuckle Bash                   Working, but no sound. MCU dump exists, its a Z?80 of some sort.
Truxton 2                      Working.
Pipi & Bibis                   Working.
Whoopee                        Working. Missing sound MCU dump. Using bootleg sound CPU dump for now
Pipi & Bibis (Ryouta Kikaku)   Working.
FixEight                       Not working properly. Missing background GFX (controlled by MCU). MCU type unknown - its a Z?80 of some sort.
Grind Stormer                  Working, but no sound. MCU type unknown - its a Z?80 of some sort.
VFive                          Working, but no sound. MCU type unknown - its a Z?80 of some sort.
Batsugun                       Working, but no sound and wrong GFX priorities. MCU type unknown - its a Z?80 of some sort.
Batsugun Sp'                   Working, but no sound and wrong GFX priorities. MCU type unknown - its a Z?80 of some sort.
Snow Bros. 2                   Working.
Mahou Daisakusen               Working.
Shippu Mahou Daisakusen        Working.
Battle Garegga                 Working.
Armed Police Batrider (Ver. A) Working.
Armed Police Batrider (Ver. B) Working.

Notes:
	See Input Port definition header below, for instructions
	  on how to enter pause/slow motion modes.
	Code at $20A26 forces territory to Japan in V-Five. Some stuff
	  NOP'd at reset vector, and Z?80 CPU post test is skipped (bootleg ?)

To Do / Unknowns:
	- Whoopee/Teki Paki sometimes tests bit 5 of the territory port
		just after testing for vblank. Why ?
	- Whoppee is currently using the sound CPU ROM (Z80) from a differnt
		(pirate ?) version of Pipi and Bibis (Ryouta Kikaku copyright).
		It really has a HD647180 CPU, and its internal ROM needs to be dumped.
	- Fix top character layer.
	- Emulate the vcount hardware correctly to avoid the Truxton 2 ROM
		bug patch. See below int truxton2_init


*****************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"


/**************** Machine stuff ******************/
#define HD64x180 0		/* Define if CPU support is available */
#define Zx80     0

#define CPU_2_NONE		0x00
#define CPU_2_Z80		0x5a
#define CPU_2_HD647180	0xa5
#define CPU_2_Zx80		0xff

/************ Machine RAM related values ************/
static UINT8 *toaplan2_shared_ram;
static UINT8 *raizing_shared_ram;		/* Shared ram used in Shippumd and Mahoudai */
static data16_t *toaplan2_shared_ram16;	/* Really 8bit RAM connected to Z180 */
static data16_t *Zx80_shared_ram;		/* Really 8bit RAM connected to Z180 */
static data16_t *battleg_commram16;		/* Comm ram used in Battle Garegga */

/************ Video RAM related values ************/
extern data16_t *toaplan2_txvideoram16;
extern data16_t *toaplan2_txvideoram16_offs;
extern data16_t *toaplan2_txscrollram16;
extern data16_t *toaplan2_tx_gfxram16;
size_t toaplan2_tx_vram_size;
size_t toaplan2_tx_offs_vram_size;
size_t toaplan2_tx_scroll_vram_size;
size_t paletteram_size;

/********** Status related values **********/
int toaplan2_sub_cpu = 0;
static int mcu_data = 0;
static int video_status;
static int prev_scanline;
static int prev_beampos;
static INT8 old_p1_paddle_h;				/* For Ghox */
static INT8 old_p1_paddle_v;
static INT8 old_p2_paddle_h;
static INT8 old_p2_paddle_v;
static int current_bank = 2;				/* Z80 bank used in Battle Garegga and Batrider */
static int batrider_z80nmi_enabled = 0;		/* Z80 nmi enabled */
static int batrider_sound_data[8];

/**************** Video stuff ******************/
WRITE16_HANDLER( toaplan2_0_voffs_w );
WRITE16_HANDLER( toaplan2_1_voffs_w );

READ16_HANDLER ( toaplan2_0_videoram16_r );
READ16_HANDLER ( toaplan2_1_videoram16_r );
WRITE16_HANDLER( toaplan2_0_videoram16_w );
WRITE16_HANDLER( toaplan2_1_videoram16_w );

READ16_HANDLER ( toaplan2_txvideoram16_r );
WRITE16_HANDLER( toaplan2_txvideoram16_w );
READ16_HANDLER ( toaplan2_txvideoram16_offs_r );
WRITE16_HANDLER( toaplan2_txvideoram16_offs_w );
READ16_HANDLER ( toaplan2_txscrollram16_r );
WRITE16_HANDLER( toaplan2_txscrollram16_w );
READ16_HANDLER ( toaplan2_tx_gfxram16_r );
WRITE16_HANDLER( toaplan2_tx_gfxram16_w );
READ16_HANDLER ( raizing_tx_gfxram16_r );
WRITE16_HANDLER( raizing_tx_gfxram16_w );

WRITE16_HANDLER( toaplan2_0_scroll_reg_select_w );
WRITE16_HANDLER( toaplan2_1_scroll_reg_select_w );
WRITE16_HANDLER( toaplan2_0_scroll_reg_data_w );
WRITE16_HANDLER( toaplan2_1_scroll_reg_data_w );

WRITE16_HANDLER( batrider_objectbank_w );
WRITE16_HANDLER( batrider_textdata_decode );

void toaplan2_0_eof_callback(void);
void toaplan2_1_eof_callback(void);
void batrider_0_eof_callback(void);
int  toaplan2_0_vh_start(void);
int  toaplan2_1_vh_start(void);
int  truxton2_0_vh_start(void);
int  batrider_0_vh_start(void);
void toaplan2_0_vh_stop(void);
void toaplan2_1_vh_stop(void);
void toaplan2_0_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void truxton2_0_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void dogyuun_1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void batsugun_1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void batrider_0_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/********* Video wrappers for PIPIBIBI *********/
READ16_HANDLER ( pipibibi_videoram16_r );
WRITE16_HANDLER( pipibibi_videoram16_w );
READ16_HANDLER ( pipibibi_spriteram16_r );
WRITE16_HANDLER( pipibibi_spriteram16_w );
WRITE16_HANDLER( pipibibi_scroll_w );



/***************************************************************************
  Initialisation handlers
***************************************************************************/

static void init_toaplan2(void)
{
	old_p1_paddle_h = 0;
	old_p1_paddle_v = 0;
	old_p2_paddle_h = 0;
	old_p2_paddle_v = 0;
	toaplan2_sub_cpu = CPU_2_HD647180;
	mcu_data = 0;
}

static void init_toaplan3(void)
{
	toaplan2_sub_cpu = CPU_2_Zx80;
	mcu_data = 0;
}

static void init_fixeight(void)
{
	install_mem_read16_handler(0, 0x28f002, 0x28fbff, MRA16_RAM );
	install_mem_write16_handler(0, 0x28f002, 0x28fbff, MWA16_RAM );

	toaplan2_sub_cpu = CPU_2_Zx80;
	mcu_data = 0;
}

static void init_snowbro2(void)
{
	toaplan2_sub_cpu = CPU_2_NONE;
	mcu_data = 0;	/* not used here actually */
}

static void init_pipibibs(void)
{
	toaplan2_sub_cpu = CPU_2_Z80;
	mcu_data = 0;	/* not used here actually */
}

static void init_pipibibi(void)
{
	int A;
	int oldword, newword;

	data16_t *pipibibi_68k_rom = (data16_t *)(memory_region(REGION_CPU1));

	/* unscramble the 68K ROM data. */

	for (A = 0; A < (0x040000/2); A+=4)
	{
		newword = 0;
		oldword = pipibibi_68k_rom[A];
		newword |= ((oldword & 0x0001) << 9);
		newword |= ((oldword & 0x0002) << 14);
		newword |= ((oldword & 0x0004) << 8);
		newword |= ((oldword & 0x0018) << 1);
		newword |= ((oldword & 0x0020) << 9);
		newword |= ((oldword & 0x0040) << 7);
		newword |= ((oldword & 0x0080) << 5);
		newword |= ((oldword & 0x0100) << 3);
		newword |= ((oldword & 0x0200) >> 1);
		newword |= ((oldword & 0x0400) >> 8);
		newword |= ((oldword & 0x0800) >> 10);
		newword |= ((oldword & 0x1000) >> 12);
		newword |= ((oldword & 0x6000) >> 7);
		newword |= ((oldword & 0x8000) >> 12);
		pipibibi_68k_rom[A] = newword;

		newword = 0;
		oldword = pipibibi_68k_rom[A+1];
		newword |= ((oldword & 0x0001) << 8);
		newword |= ((oldword & 0x0002) << 12);
		newword |= ((oldword & 0x0004) << 5);
		newword |= ((oldword & 0x0008) << 11);
		newword |= ((oldword & 0x0010) << 2);
		newword |= ((oldword & 0x0020) << 10);
		newword |= ((oldword & 0x0040) >> 1);
		newword |= ((oldword & 0x0080) >> 7);
		newword |= ((oldword & 0x0100) >> 4);
		newword |= ((oldword & 0x0200) << 0);
		newword |= ((oldword & 0x0400) >> 7);
		newword |= ((oldword & 0x0800) >> 1);
		newword |= ((oldword & 0x1000) >> 10);
		newword |= ((oldword & 0x2000) >> 2);
		newword |= ((oldword & 0x4000) >> 13);
		newword |= ((oldword & 0x8000) >> 3);
		pipibibi_68k_rom[A+1] = newword;

		newword = 0;
		oldword = pipibibi_68k_rom[A+2];
		newword |= ((oldword & 0x000f) << 4);
		newword |= ((oldword & 0x00f0) >> 4);
		newword |= ((oldword & 0x0100) << 3);
		newword |= ((oldword & 0x0200) << 1);
		newword |= ((oldword & 0x0400) >> 1);
		newword |= ((oldword & 0x0800) >> 3);
		newword |= ((oldword & 0x1000) << 3);
		newword |= ((oldword & 0x2000) << 1);
		newword |= ((oldword & 0x4000) >> 1);
		newword |= ((oldword & 0x8000) >> 3);
		pipibibi_68k_rom[A+2] = newword;

		newword = 0;
		oldword = pipibibi_68k_rom[A+3];
		newword |= ((oldword & 0x000f) << 4);
		newword |= ((oldword & 0x00f0) >> 4);
		newword |= ((oldword & 0x0100) << 7);
		newword |= ((oldword & 0x0200) << 5);
		newword |= ((oldword & 0x0400) << 3);
		newword |= ((oldword & 0x0800) << 1);
		newword |= ((oldword & 0x1000) >> 1);
		newword |= ((oldword & 0x2000) >> 3);
		newword |= ((oldword & 0x4000) >> 5);
		newword |= ((oldword & 0x8000) >> 7);
		pipibibi_68k_rom[A+3] = newword;
	}

	toaplan2_sub_cpu = CPU_2_Z80;
	mcu_data = 0;	/* not used here actually */
}

static void init_truxton2(void)
{
	data16_t *truxton2_ROM = (data16_t *)memory_region(REGION_CPU1);

	/* The 68K ROM has a serious bug, which has shown up in emulation.
	   A branch condition instruction at $12BA keeps jumping back to the code
	   that subroutines to it. Sure enough the stack very quickly overflows
	   from the subroutine that keeps calling itself, and the game crashes.
	   By chance, the real hardware obviously never allows the branch
	   condition to be true.  In any case, the correct version of this code
	   is prevelant through-out the ROM, so the fix is obvious.
	   Todo: Emulate the hardware correctly to avoid this bug patch
	*/

	truxton2_ROM[0x012BA / 2] = 0x65EA;	 /* Patch the relative branch so it jumps to the right location */
	truxton2_ROM[0x2ED58 / 2] = 0x0E92;	 /* Patch the checksum byte */

	toaplan2_sub_cpu = CPU_2_NONE;
	mcu_data = 0;	/* not used here actually */
}

static void init_raizing(void)
{
	toaplan2_sub_cpu = CPU_2_Z80;
	mcu_data = 0;	/* not used here actually */
}

static void init_battleg(void)
{
	data8_t *Z80 = (data8_t *)memory_region(REGION_CPU2);

	/* Set Z80 bank switch */
	cpu_setbank(1, &Z80[0x10000]);		/* Default bank is 2 */

	toaplan2_sub_cpu = CPU_2_Z80;
	mcu_data = 0;	/* not used here actually */
}



/***************************************************************************
  Interrupt handlers
***************************************************************************/

static int toaplan2_interrupt(void)
{
	return MC68000_IRQ_4;
}

static int truxton2_interrupt(void)
{
	return MC68000_IRQ_2;
}

static int batrider_sound_interrupt(void)	/* Batrider uses NMI for sound cpu */
{
	if (batrider_z80nmi_enabled)
	{
		batrider_z80nmi_enabled = 0;
		return Z80_NMI_INT;
	}
	else
		return Z80_IGNORE_INT;
}


/***************************************************************************
  Toaplan games
***************************************************************************/

static READ16_HANDLER( video_count_r )
{
	/* +---------+---------+--------+---------------------------+ */
	/* | /H-Sync | /V-Sync | /Blank |           Count           | */
	/* | Bit 15  | Bit 14  | Bit 8  |  Bit 7-0 (count from #EF) | */
	/* +---------+---------+--------+---------------------------+ */
	/*************** Control Signals are active low ***************/
	/*                The following is still wrong                */

	int current_scanline = cpu_getscanline();
	int current_beampos = cpu_gethorzbeampos();

/*/	logerror("Was VC=%04x  Vbl=%02x  VS=%04x  HS=%04x - ",video_status,cpu_getvblank(),cpu_getscanline(),cpu_gethorzbeampos() ); */

	video_status |= 0x4100;

	if (prev_beampos != current_beampos)
	{
		prev_beampos = current_beampos;
		video_status += 0x8001;
		video_status |= 0x0100;
		video_status &= 0xc1ff;							/* H-Sync Clk ? */
		if (prev_scanline != current_scanline)
		{
			prev_scanline = current_scanline;
			video_status = 0x00ef;						/* V-Sync Clk ? */
		}
	}
	video_status &= cpu_getvblank() ? 0xc1ff : 0xc0ff;	/* V-Blank ? */

/*/	logerror("Now VC=%04x  Vbl=%02x  VS=%04x  HS=%04x\n",video_status,cpu_getvblank(),cpu_getscanline(),cpu_gethorzbeampos() ); */
	return video_status;
}

static WRITE_HANDLER( toaplan2_coin_w )
{
	/* +----------------+------ Bits 7-5 not used ------+--------------+ */
	/* | Coin Lockout 2 | Coin Lockout 1 | Coin Count 2 | Coin Count 1 | */
	/* |     Bit 3      |     Bit 2      |     Bit 1    |     Bit 0    | */

	if (data & 0x0f)
	{
		coin_lockout_w( 0, ((data & 4) ? 0 : 1) );
		coin_lockout_w( 1, ((data & 8) ? 0 : 1) );
		coin_counter_w( 0, (data & 1) ); coin_counter_w( 0, 0 );
		coin_counter_w( 1, (data & 2) ); coin_counter_w( 1, 0 );
	}
	else
	{
		coin_lockout_global_w(1); /* Lock all coin slots */
	}
	if (data & 0xe0)
	{
		logerror("Writing unknown upper bits (%02x) to coin control\n",data);
	}
}
static WRITE16_HANDLER( toaplan2_coin_word_w )
{
	if (ACCESSING_LSB)
	{
		toaplan2_coin_w(offset, data & 0xff);
		if (toaplan2_sub_cpu == CPU_2_Z80)
		{
			if (Machine->drv->sound[1].sound_type == SOUND_OKIM6295)
			{
				OKIM6295_set_bank_base(0, (((data & 0x10) >> 4) * 0x40000));
			}
		}
	}
	if (ACCESSING_MSB && (data & 0xff00) )
	{
		logerror("Writing unknown upper MSB command (%04x) to coin control\n",data & 0xff00);
	}
}

static READ16_HANDLER( toaplan2_shared_r )
{
	return toaplan2_shared_ram[offset] & 0xff;
}

static WRITE16_HANDLER( toaplan2_shared_w )
{
	if (ACCESSING_LSB)
	{
		toaplan2_shared_ram[offset] = data & 0xff;
	}
}

static WRITE16_HANDLER( toaplan2_hd647180_cpu_w )
{
	/* Command sent to secondary CPU. Support for HD647180 will be
	   required when a ROM dump becomes available for this hardware */

	if (ACCESSING_LSB)
	{
		if (toaplan2_sub_cpu == CPU_2_Z80)			/* Whoopee */
		{
			toaplan2_shared_ram[0] = data & 0xff;
		}
		else										/* Teki Paki */
		{
			mcu_data = data & 0xff;
			logerror("PC:%08x Writing command (%04x) to secondary CPU shared port\n",cpu_getpreviouspc(),mcu_data);
		}
	}
}

static READ16_HANDLER( c2map_port_6_r )
{
	/* For Teki Paki hardware */
	/* bit 4 high signifies secondary CPU is ready */
	/* bit 5 is tested low before V-Blank bit ??? */
	switch (toaplan2_sub_cpu)
	{
		case CPU_2_Z80:			mcu_data = toaplan2_shared_ram[0]; break; /* Whoopee */
		case CPU_2_HD647180:	mcu_data = 0xff; break;					  /* Teki Paki */
		default:				mcu_data = 0x00; break;
	}
	if (mcu_data == 0xff) mcu_data = 0x10;
	else mcu_data = 0x00;
	return ( mcu_data | input_port_6_r(0) );
}

static READ16_HANDLER( pipibibi_z80_status_r )
{
	return toaplan2_shared_ram[0] & 0xff;
}

static WRITE16_HANDLER( pipibibi_z80_task_w )
{
	if (ACCESSING_LSB)
	{
		toaplan2_shared_ram[0] = data & 0xff;
	}
}

static READ16_HANDLER( ghox_p1_h_analog_r )
{
	INT8 value, new_value;

	new_value = input_port_7_r(0);
	if (new_value == old_p1_paddle_h) return 0;
	value = new_value - old_p1_paddle_h;
	old_p1_paddle_h = new_value;
	return value;
}

static READ16_HANDLER( ghox_p1_v_analog_r )
{
	INT8 new_value;

	new_value = input_port_9_r(0);		/* fake vertical movement */
	if (new_value == old_p1_paddle_v) return input_port_1_r(0);
	if (new_value >  old_p1_paddle_v)
	{
		old_p1_paddle_v = new_value;
		return (input_port_1_r(0) | 2);
	}
	old_p1_paddle_v = new_value;
	return (input_port_1_r(0) | 1);
}

static READ16_HANDLER( ghox_p2_h_analog_r )
{
	INT8 value, new_value;

	new_value = input_port_8_r(0);
	if (new_value == old_p2_paddle_h) return 0;
	value = new_value - old_p2_paddle_h;
	old_p2_paddle_h = new_value;
	return value;
}

static READ16_HANDLER( ghox_p2_v_analog_r )
{
	INT8 new_value;

	new_value = input_port_10_r(0);		/* fake vertical movement */
	if (new_value == old_p2_paddle_v) return input_port_2_r(0);
	if (new_value >  old_p2_paddle_v)
	{
		old_p2_paddle_v = new_value;
		return (input_port_2_r(0) | 2);
	}
	old_p2_paddle_v = new_value;
	return (input_port_2_r(0) | 1);
}

static READ16_HANDLER( ghox_mcu_r )
{
	return 0xff;
}

static WRITE16_HANDLER( ghox_mcu_w )
{
	if (ACCESSING_LSB)
	{
		mcu_data = data;
		if ((data >= 0xd0) && (data < 0xe0))
		{
			offset = ((data & 0x0f) * 2) + (0x38 / 2);
			toaplan2_shared_ram16[offset  ] = 0x0005;	/* Return address for */
			toaplan2_shared_ram16[offset-1] = 0x0056;	/*   RTS instruction */
		}
		else
		{
			logerror("PC:%08x Writing %08x to HD647180 cpu shared ram status port\n",cpu_getpreviouspc(),mcu_data);
		}
		toaplan2_shared_ram16[0x56 / 2] = 0x004e;	/* Return a RTS instruction */
		toaplan2_shared_ram16[0x58 / 2] = 0x0075;

		if (data == 0xd3)
		{
		toaplan2_shared_ram16[0x56 / 2] = 0x003a;	/*	move.w  d1,d5 */
		toaplan2_shared_ram16[0x58 / 2] = 0x0001;
		toaplan2_shared_ram16[0x5a / 2] = 0x0008;	/*	bclr.b  #0,d5 */
		toaplan2_shared_ram16[0x5c / 2] = 0x0085;
		toaplan2_shared_ram16[0x5e / 2] = 0x0000;
		toaplan2_shared_ram16[0x60 / 2] = 0x0000;
		toaplan2_shared_ram16[0x62 / 2] = 0x00cb;	/*	muls.w  #3,d5 */
		toaplan2_shared_ram16[0x64 / 2] = 0x00fc;
		toaplan2_shared_ram16[0x66 / 2] = 0x0000;
		toaplan2_shared_ram16[0x68 / 2] = 0x0003;
		toaplan2_shared_ram16[0x6a / 2] = 0x0090;	/*	sub.w   d5,d0 */
		toaplan2_shared_ram16[0x6c / 2] = 0x0045;
		toaplan2_shared_ram16[0x6e / 2] = 0x00e5;	/*	lsl.b   #2,d1 */
		toaplan2_shared_ram16[0x70 / 2] = 0x0009;
		toaplan2_shared_ram16[0x72 / 2] = 0x004e;	/*	rts */
		toaplan2_shared_ram16[0x74 / 2] = 0x0075;
		}
	}
}

static READ16_HANDLER( ghox_shared_ram_r )
{
	/* Ghox 68K reads data from MCU shared RAM and writes it to main RAM.
	   It then subroutine jumps to main RAM and executes this code.
	   Here, i'm just returning a RTS instruction for now.
	   See above ghox_mcu_w routine.

	   Offset $56 and $58 is accessed around PC:F814

	   Offset $38 and $36 is accessed from around PC:DA7C
	   Offset $3c and $3a is accessed from around PC:2E3C
	   Offset $40 and $3E is accessed from around PC:103EE
	   Offset $44 and $42 is accessed from around PC:FB52
	   Offset $48 and $46 is accessed from around PC:6776
	*/

	return toaplan2_shared_ram16[offset] & 0xff;
}
static WRITE16_HANDLER( ghox_shared_ram_w )
{
	if (ACCESSING_LSB)
	{
		toaplan2_shared_ram16[offset] = data & 0xff;
	}
}
static READ16_HANDLER( kbash_sub_cpu_r )
{
/*	Knuckle Bash's  68000 reads secondary CPU status via an I/O port.
	If a value of 2 is read, then secondary CPU is busy.
	Secondary CPU must report 0xff when no longer busy, to signify that it
	has passed POST.
*/
	return 0xff;
}

static WRITE16_HANDLER( kbash_sub_cpu_w )
{
	logerror("PC:%08x writing %04x to Zx80 secondary CPU status port %02x\n",cpu_getpreviouspc(),mcu_data,offset/2);
}

static READ16_HANDLER( shared_ram_r )
{
/*	Other games using a Zx80 based secondary CPU, have shared memory between
	the 68000 and the Zx80 CPU. The 68000 reads the status of the Zx80
	via a location of the shared memory.
*/
	return toaplan2_shared_ram16[offset] & 0xff;
}

static WRITE16_HANDLER( shared_ram_w )
{
	if (ACCESSING_LSB)
	{
		data &= 0xff;
		switch (offset * 2)
		{
			case 0x6e8:
			case 0x9e8:
			case 0x9f0:
			case 0xcf0:
			case 0xcf8:
			case 0xff8: toaplan2_shared_ram16[offset + 1] = data; /* Dogyuun */
						toaplan2_shared_ram16[offset + 2] = data; /* FixEight */
						logerror("PC:%08x Writing  (%04x) to secondary CPU\n",cpu_getpreviouspc(),data);
						if (data == 0x81) data = 0x0001;
						break;
			default:	break;
		}
		toaplan2_shared_ram16[offset] = data;
	}
}

static READ16_HANDLER( Zx80_status_port_r )
{
/*** Status port includes Zx80 CPU POST codes. ************
 *** This is actually a part of the 68000/Zx80 Shared RAM */

	/*** Dogyuun mcu post data ***/
	if (mcu_data == 0x800000aa) mcu_data = 0xff;
	if (mcu_data == 0x00) mcu_data = 0x800000aa;

	/*** FixEight mcu post data ***/
	if (mcu_data == 0x8000ffaa)
	{
#if 0 	/* check the 37B6 code */
		/* copy nvram data to shared ram after post is complete */
		fixeight_sharedram[0] = fixeight_nvram[0];	/* Dip Switch A */
		fixeight_sharedram[1] = fixeight_nvram[1];	/* Dip Switch B */
		fixeight_sharedram[2] = fixeight_nvram[2];	/* Territory */
#endif
		/* Hack Alert ! Fixeight does not have any DSW. The main CPU has a */
		/* game keeping service mode. It writes/reads the settings to/from */
		/* these shared RAM locations. The secondary CPU reads/writes them */
		/* from/to nvram to store the settings (a 94C45 EEPROM) */
		install_mem_read16_handler (0, 0x28f002, 0x28f003, MRA16_RAM);
		install_mem_read16_handler (0, 0x28f004, 0x28f005, input_port_5_word_r);	/* Dip Switch A - Wrong !!! */
		install_mem_read16_handler (0, 0x28f006, 0x28f007, input_port_6_word_r);	/* Dip Switch B - Wrong !!! */
		install_mem_read16_handler (0, 0x28f008, 0x28f009, input_port_7_word_r);	/* Territory Jumper block - Wrong !!! */
		install_mem_read16_handler (0, 0x28f00a, 0x28fbff, MRA16_RAM);
		install_mem_write16_handler (0, 0x28f002, 0x28f003, MWA16_RAM);
		install_mem_write16_handler (0, 0x28f004, 0x28f009, MWA16_NOP);
		install_mem_write16_handler (0, 0x28f00a, 0x28fbff, MWA16_RAM);

		mcu_data = 0xffff;
	}
	if (mcu_data == 0xffaa) mcu_data = 0x8000ffaa;
	if (mcu_data == 0xff00) mcu_data = 0xffaa;

	logerror("PC:%08x reading %08x from Zx80 secondary CPU command/status port\n",cpu_getpreviouspc(),mcu_data);
	return mcu_data & 0xff;
}

static WRITE16_HANDLER( Zx80_command_port_w )
{
	if (ACCESSING_LSB)
	{
		mcu_data = data;
	logerror("PC:%08x Writing command (%04x) to Zx80 secondary CPU command/status port\n",cpu_getpreviouspc(),mcu_data);
}
}

static READ16_HANDLER( Zx80_sharedram_r )
{
	return Zx80_shared_ram[offset] & 0xff;
}

static WRITE16_HANDLER( Zx80_sharedram_w )
{
	if (ACCESSING_LSB)
	{
		Zx80_shared_ram[offset] = data & 0xff;
	}
}

static WRITE16_HANDLER( oki_bankswitch_w )
{
	if (ACCESSING_LSB)
	{
		OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
	}
}



/***************************************************************************
  Raizing games
***************************************************************************/

static READ16_HANDLER( raizing_shared_ram_r )
{
	return raizing_shared_ram[offset] & 0xff;
}

static WRITE16_HANDLER( raizing_shared_ram_w )
{
	if (ACCESSING_LSB)
	{
		raizing_shared_ram[offset] = data & 0xff;
	}
}

static READ16_HANDLER( battleg_commram_r )
{
	return battleg_commram16[offset];
}

static WRITE16_HANDLER( battleg_commram_w )
{
	COMBINE_DATA(&battleg_commram16[offset]);
	cpu_cause_interrupt(1, Z80_IRQ_INT);
}

static READ_HANDLER( battleg_commram_check_r0 )
{
	data8_t *battleg_common_RAM = (data8_t *)battleg_commram16;

	return battleg_common_RAM[offset * 2];
}

static WRITE_HANDLER( battleg_commram_check_w0 )
{
	data8_t *battleg_common_RAM = (data8_t *)battleg_commram16;

	battleg_common_RAM[1] = data;
}

static READ16_HANDLER( battleg_z80check_r )
{
	return raizing_shared_ram[offset + 0x10] & 0xff;
}

static WRITE_HANDLER( battleg_bankswitch_w )
{
	data8_t *RAM = (data8_t *)memory_region(REGION_CPU2);
	int bankaddress;
	int bank;

	bank = (data & 0x0f) - 10;

	if (bank != current_bank)
	{
		current_bank = bank;
		bankaddress = 0x10000 + 0x4000 * current_bank;
		cpu_setbank(1, &RAM[bankaddress]);
	}
}

static void raizing_oki6295_set_bankbase( int chip, int channel, int base )
{
	/* The OKI6295 ROM space is divided in four banks, each one independantly */
	/* controlled. The sample table at the beginning of the addressing space  */
	/* is divided in four pages as well, banked together with the sample data */

	data8_t *rom = (data8_t *)memory_region(REGION_SOUND1 + chip);

	/* copy the samples */
	memcpy(rom + channel * 0x10000, rom + 0x40000 + base, 0x10000);

	/* and also copy the samples address table */
	rom += channel * 0x100;
	memcpy(rom, rom + 0x40000 + base, 0x100);
}


static WRITE_HANDLER( raizing_okim6295_bankselect_0 )
{
	raizing_oki6295_set_bankbase( 0, 0,  (data       & 0x0f) * 0x10000);
	raizing_oki6295_set_bankbase( 0, 1, ((data >> 4) & 0x0f) * 0x10000);
}

static WRITE_HANDLER( raizing_okim6295_bankselect_1 )
{
	raizing_oki6295_set_bankbase( 0, 2,  (data       & 0x0f) * 0x10000);
	raizing_oki6295_set_bankbase( 0, 3, ((data >> 4) & 0x0f) * 0x10000);
}

static WRITE_HANDLER( raizing_okim6295_bankselect_2 )
{
	raizing_oki6295_set_bankbase( 1, 0,  (data       & 0x0f) * 0x10000);
	raizing_oki6295_set_bankbase( 1, 1, ((data >> 4) & 0x0f) * 0x10000);
}

static WRITE_HANDLER( raizing_okim6295_bankselect_3 )
{
	raizing_oki6295_set_bankbase( 1, 2,  (data       & 0x0f) * 0x10000);
	raizing_oki6295_set_bankbase( 1, 3, ((data >> 4) & 0x0f) * 0x10000);
}

static READ16_HANDLER( batrider_z80rom_r )
{
	data8_t *batrider_Z80_ROM = (data8_t *)memory_region(REGION_CPU2);

	if (offset < 0x8000)
		return batrider_Z80_ROM[offset] & 0xff;

	return batrider_Z80_ROM[offset + 0x8000] & 0xff;
}

static WRITE_HANDLER( batrider_bankswitch_w )
{
	data8_t *RAM = (data8_t *)memory_region(REGION_CPU2);
	int bankaddress;
	int bank;

	bank = data & 0x0f;

	if (bank != current_bank)
	{
		current_bank = bank;
		logerror("batrider Z80 cpu set bank #%d\n", bank);
		if (bank > 1)
			bankaddress = 0x10000 + 0x4000 * (current_bank - 2);
		else
			bankaddress = 0x4000 * current_bank;
		cpu_setbank(1, &RAM[bankaddress]);
	}
}

static READ16_HANDLER( batrider_sound_data_68000_r )
{
	logerror("Batrider 68K sound data read %02x from offs:%02x\n", batrider_sound_data[(offset | 4)], (offset | 4));
	return batrider_sound_data[(offset | 4)];
}

static WRITE16_HANDLER( batrider_sound_data_68000_w )
{
	if (ACCESSING_LSB)
	{
		logerror("Batrider 68K sound data write %02x to offs:%02x\n", data, offset);
		batrider_sound_data[offset] = data;
		if ( !(offset & 2) )
			batrider_z80nmi_enabled = 1;
	}
}

static WRITE16_HANDLER( batrider_sound_data6_68000_w )
{
	batrider_sound_data[6] = data;
}

static READ_HANDLER( batrider_sound_data_z80_r )
{
	offset /= 2;
	switch( offset )
	{
		case 0x00:
		case 0x01:
			logerror("Batrider Z80 sound data read %02x from offs:%02x\n", batrider_sound_data[offset], offset);
			return batrider_sound_data[offset];
			break;
		default:
			logerror("Batrider Z80 sound data read %02x from offs:%02x  INVALID !!!\n", batrider_sound_data[offset], offset);
			break;
	}
	return 0;
}

static WRITE_HANDLER( batrider_sound_data_z80_w )
{
	offset /= 2;
	switch( offset )
	{
		case 0x00:
		case 0x01:
/*		case 0x02:		Actually written from 68K */
		case 0x03:
			logerror("Batrider Z80 sound data write %02x to offs:%02x\n", data, (offset | 4));
			batrider_sound_data[(offset | 4)] = data;
			break;
		default:
			logerror("Batrider Z80 sound data write %02x to offs:%02x  INVALID !!!\n", data, (offset | 4));
			break;
	}
}



/******* This is temporary stuff for batrider to view the sound data ********/

static READ16_HANDLER( batrider_sound_data_68000_r1 )
{
	return batrider_sound_data[offset];
}
static WRITE16_HANDLER( batrider_sound_data_68000_w1 )
{
	batrider_sound_data[offset] = data;
}

static READ_HANDLER( batrider_sound_data_z80_r1 )
{
	return batrider_sound_data[offset];
}
static WRITE_HANDLER( batrider_sound_data_z80_w1 )
{
	batrider_sound_data[offset] = data;
}



static MEMORY_READ16_START( tekipaki_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x020000, 0x03ffff, MRA16_ROM },				/* extra for Whoopee */
	{ 0x080000, 0x082fff, MRA16_RAM },
	{ 0x0c0000, 0x0c0fff, paletteram16_word_r },
	{ 0x140004, 0x140007, toaplan2_0_videoram16_r },
	{ 0x14000c, 0x14000d, input_port_0_word_r },	/* VBlank */
	{ 0x180000, 0x180001, input_port_4_word_r },	/* Dip Switch A */
	{ 0x180010, 0x180011, input_port_5_word_r },	/* Dip Switch B */
	{ 0x180020, 0x180021, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x180030, 0x180031, c2map_port_6_r },			/* CPU 2 busy and Territory Jumper block */
	{ 0x180050, 0x180051, input_port_1_word_r },	/* Player 1 controls */
	{ 0x180060, 0x180061, input_port_2_word_r },	/* Player 2 controls */
MEMORY_END

static MEMORY_WRITE16_START( tekipaki_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x020000, 0x03ffff, MWA16_ROM },				/* extra for Whoopee */
	{ 0x080000, 0x082fff, MWA16_RAM },
	{ 0x0c0000, 0x0c0fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x140000, 0x140001, toaplan2_0_voffs_w },
	{ 0x140004, 0x140007, toaplan2_0_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x140008, 0x140009, toaplan2_0_scroll_reg_select_w },
	{ 0x14000c, 0x14000d, toaplan2_0_scroll_reg_data_w },
	{ 0x180040, 0x180041, toaplan2_coin_word_w },	/* Coin count/lock */
	{ 0x180070, 0x180071, toaplan2_hd647180_cpu_w },
MEMORY_END

static MEMORY_READ16_START( ghox_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x040001, ghox_p2_h_analog_r },		/* Paddle 2 */
	{ 0x080000, 0x083fff, MRA16_RAM },
	{ 0x0c0000, 0x0c0fff, paletteram16_word_r },
	{ 0x100000, 0x100001, ghox_p1_h_analog_r },		/* Paddle 1 */
	{ 0x140004, 0x140007, toaplan2_0_videoram16_r },
	{ 0x14000c, 0x14000d, input_port_0_word_r },	/* VBlank */
	{ 0x180000, 0x180001, ghox_mcu_r },				/* really part of shared RAM */
	{ 0x180006, 0x180007, input_port_4_word_r },	/* Dip Switch A */
	{ 0x180008, 0x180009, input_port_5_word_r },	/* Dip Switch B */
	{ 0x180010, 0x180011, input_port_3_word_r },	/* Coin/System inputs */
/*	{ 0x18000c, 0x18000d, input_port_1_word_r },	   Player 1 controls (real)    */
/*	{ 0x18000e, 0x18000f, input_port_2_word_r },	   Player 2 controls (real)    */
	{ 0x18000c, 0x18000d, ghox_p1_v_analog_r },		/* Player 1 controls */
	{ 0x18000e, 0x18000f, ghox_p2_v_analog_r },		/* Player 2 controls */
	{ 0x180500, 0x180fff, ghox_shared_ram_r },
	{ 0x18100c, 0x18100d, input_port_6_word_r },	/* Territory Jumper block */
MEMORY_END

static MEMORY_WRITE16_START( ghox_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x083fff, MWA16_RAM },
	{ 0x0c0000, 0x0c0fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x140000, 0x140001, toaplan2_0_voffs_w },
	{ 0x140004, 0x140007, toaplan2_0_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x140008, 0x140009, toaplan2_0_scroll_reg_select_w },
	{ 0x14000c, 0x14000d, toaplan2_0_scroll_reg_data_w },
	{ 0x180000, 0x180001, ghox_mcu_w },				/* really part of shared RAM */
	{ 0x180500, 0x180fff, ghox_shared_ram_w, &toaplan2_shared_ram16 },
	{ 0x181000, 0x181001, toaplan2_coin_word_w },
MEMORY_END

static MEMORY_READ16_START( dogyuun_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x200010, 0x200011, input_port_1_word_r },	/* Player 1 controls */
	{ 0x200014, 0x200015, input_port_2_word_r },	/* Player 2 controls */
	{ 0x200018, 0x200019, input_port_3_word_r },	/* Coin/System inputs */
#if Zx80
	{ 0x21e000, 0x21fbff, shared_ram_r },			/* $21f000 status port */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_r },		/* 16-bit on 68000 side, 8-bit on Zx80 side */
#else
	{ 0x21e000, 0x21efff, shared_ram_r },
	{ 0x21f000, 0x21f001, Zx80_status_port_r },		/* Zx80 status port */
	{ 0x21f004, 0x21f005, input_port_4_word_r },	/* Dip Switch A */
	{ 0x21f006, 0x21f007, input_port_5_word_r },	/* Dip Switch B */
	{ 0x21f008, 0x21f009, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_r },		/* 16-bit on 68000 side, 8-bit on Zx80 side */
#endif
	/***** The following in 0x30000x are for video controller 1 ******/
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },/* tile layers */
	{ 0x30000c, 0x30000d, input_port_0_word_r },	/* VBlank */
	{ 0x400000, 0x400fff, paletteram16_word_r },
	/***** The following in 0x50000x are for video controller 2 ******/
	{ 0x500004, 0x500007, toaplan2_1_videoram16_r },/* tile layers 2 */
	{ 0x700000, 0x700001, video_count_r },			/* test bit 8 */
MEMORY_END

static MEMORY_WRITE16_START( dogyuun_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x200008, 0x200009, OKIM6295_data_0_lsb_w },
	{ 0x20001c, 0x20001d, toaplan2_coin_word_w },
#if Zx80
	{ 0x21e000, 0x21fbff, shared_ram_w, &toaplan2_shared_ram16 },	/* $21F000 */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_w, &Zx80_shared_ram },	/* 16-bit on 68000 side, 8-bit on Zx80 side */
#else
	{ 0x21e000, 0x21efff, shared_ram_w, &toaplan2_shared_ram16 },
	{ 0x21f000, 0x21f001, Zx80_command_port_w },	/* Zx80 command port */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_w, &Zx80_shared_ram },	/* 16-bit on 68000 side, 8-bit on Zx80 side */
#endif
	/***** The following in 0x30000x are for video controller 1 ******/
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },		/* VideoRAM selector/offset */
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	/***** The following in 0x50000x are for video controller 2 ******/
	{ 0x500000, 0x500001, toaplan2_1_voffs_w },		/* VideoRAM selector/offset */
	{ 0x500004, 0x500007, toaplan2_1_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x500008, 0x500009, toaplan2_1_scroll_reg_select_w },
	{ 0x50000c, 0x50000d, toaplan2_1_scroll_reg_data_w },
MEMORY_END

static MEMORY_READ16_START( kbash_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x200000, 0x200001, kbash_sub_cpu_r },
	{ 0x200004, 0x200005, input_port_4_word_r },	/* Dip Switch A */
	{ 0x200006, 0x200007, input_port_5_word_r },	/* Dip Switch B */
	{ 0x200008, 0x200009, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x208010, 0x208011, input_port_1_word_r },	/* Player 1 controls */
	{ 0x208014, 0x208015, input_port_2_word_r },	/* Player 2 controls */
	{ 0x208018, 0x208019, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },/* tile layers */
	{ 0x30000c, 0x30000d, input_port_0_word_r },	/* VBlank */
	{ 0x400000, 0x400fff, paletteram16_word_r },
	{ 0x700000, 0x700001, video_count_r },			/* test bit 8 */
MEMORY_END

static MEMORY_WRITE16_START( kbash_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x200000, 0x200003, kbash_sub_cpu_w },		/* sound number to play */
/*	{ 0x200002, 0x200003, kbash_sub_cpu_w2 },		   ???    */
	{ 0x20801c, 0x20801d, toaplan2_coin_word_w },
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( truxton2_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x200004, 0x200007, toaplan2_0_videoram16_r },
	{ 0x20000c, 0x20000d, input_port_0_word_r },	/* VBlank */
	{ 0x300000, 0x300fff, paletteram16_word_r },
	{ 0x400000, 0x401fff, toaplan2_txvideoram16_r },
	{ 0x402000, 0x4021ff, toaplan2_txvideoram16_offs_r },
	{ 0x402200, 0x402fff, MRA16_RAM },
	{ 0x403000, 0x4031ff, toaplan2_txscrollram16_r },
	{ 0x403200, 0x403fff, MRA16_RAM },
	{ 0x500000, 0x50ffff, toaplan2_tx_gfxram16_r },
	{ 0x600000, 0x600001, video_count_r },
	{ 0x700000, 0x700001, input_port_4_word_r },	/* Dip Switch A */
	{ 0x700002, 0x700003, input_port_5_word_r },	/* Dip Switch B */
	{ 0x700004, 0x700005, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x700006, 0x700007, input_port_1_word_r },	/* Player 1 controls */
	{ 0x700008, 0x700009, input_port_2_word_r },	/* Player 2 controls */
	{ 0x70000a, 0x70000b, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x700010, 0x700011, OKIM6295_status_0_lsb_r },
	{ 0x700014, 0x700015, MRA16_NOP },
	{ 0x700016, 0x700017, YM2151_status_port_0_lsb_r },
MEMORY_END

static MEMORY_WRITE16_START( truxton2_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x200001, toaplan2_0_voffs_w },		/* VideoRAM selector/offset */
	{ 0x200004, 0x200007, toaplan2_0_videoram16_w },
	{ 0x200008, 0x200009, toaplan2_0_scroll_reg_select_w },
	{ 0x20000c, 0x20000d, toaplan2_0_scroll_reg_data_w },
	{ 0x300000, 0x300fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x400000, 0x401fff, toaplan2_txvideoram16_w, &toaplan2_txvideoram16, &toaplan2_tx_vram_size },
	{ 0x402000, 0x4021ff, toaplan2_txvideoram16_offs_w, &toaplan2_txvideoram16_offs, &toaplan2_tx_offs_vram_size },
	{ 0x402200, 0x402fff, MWA16_RAM },
	{ 0x403000, 0x4031ff, toaplan2_txscrollram16_w, &toaplan2_txscrollram16, &toaplan2_tx_scroll_vram_size },
	{ 0x403200, 0x403fff, MWA16_RAM },
	{ 0x500000, 0x50ffff, toaplan2_tx_gfxram16_w, &toaplan2_tx_gfxram16 },
	{ 0x700010, 0x700011, OKIM6295_data_0_lsb_w },
	{ 0x700014, 0x700015, YM2151_register_port_0_lsb_w },
	{ 0x700016, 0x700017, YM2151_data_port_0_lsb_w },
	{ 0x70001e, 0x70001f, toaplan2_coin_word_w },	/* Coin count/lock */
MEMORY_END

static MEMORY_READ16_START( pipibibs_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x082fff, MRA16_RAM },
	{ 0x0c0000, 0x0c0fff, paletteram16_word_r },
	{ 0x140004, 0x140007, toaplan2_0_videoram16_r },
	{ 0x14000c, 0x14000d, input_port_0_word_r },	/* VBlank */
	{ 0x190000, 0x190fff, toaplan2_shared_r },
	{ 0x19c020, 0x19c021, input_port_4_word_r },	/* Dip Switch A */
	{ 0x19c024, 0x19c025, input_port_5_word_r },	/* Dip Switch B */
	{ 0x19c028, 0x19c029, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x19c02c, 0x19c02d, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x19c030, 0x19c031, input_port_1_word_r },	/* Player 1 controls */
	{ 0x19c034, 0x19c035, input_port_2_word_r },	/* Player 2 controls */
MEMORY_END

static MEMORY_WRITE16_START( pipibibs_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x082fff, MWA16_RAM },
	{ 0x0c0000, 0x0c0fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x140000, 0x140001, toaplan2_0_voffs_w },
	{ 0x140004, 0x140007, toaplan2_0_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x140008, 0x140009, toaplan2_0_scroll_reg_select_w },
	{ 0x14000c, 0x14000d, toaplan2_0_scroll_reg_data_w },
	{ 0x190000, 0x190fff, toaplan2_shared_w },
	{ 0x19c01c, 0x19c01d, toaplan2_coin_word_w },	/* Coin count/lock */
MEMORY_END

static MEMORY_READ16_START( pipibibi_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x082fff, MRA16_RAM },
	{ 0x083000, 0x0837ff, pipibibi_spriteram16_r },
	{ 0x083800, 0x087fff, MRA16_RAM },
	{ 0x0c0000, 0x0c0fff, paletteram16_word_r },
	{ 0x120000, 0x120fff, MRA16_RAM },
	{ 0x180000, 0x182fff, pipibibi_videoram16_r },
	{ 0x190002, 0x190003, pipibibi_z80_status_r },	/* Z80 ready ? */
	{ 0x19c020, 0x19c021, input_port_4_word_r },	/* Dip Switch A */
	{ 0x19c024, 0x19c025, input_port_5_word_r },	/* Dip Switch B */
	{ 0x19c028, 0x19c029, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x19c02c, 0x19c02d, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x19c030, 0x19c031, input_port_1_word_r },	/* Player 1 controls */
	{ 0x19c034, 0x19c035, input_port_2_word_r },	/* Player 2 controls */
MEMORY_END

static MEMORY_WRITE16_START( pipibibi_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x082fff, MWA16_RAM },
	{ 0x083000, 0x0837ff, pipibibi_spriteram16_w },	/* SpriteRAM */
	{ 0x083800, 0x087fff, MWA16_RAM },				/* SpriteRAM (unused) */
	{ 0x0c0000, 0x0c0fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x120000, 0x120fff, MWA16_RAM },				/* Copy of SpriteRAM ? */
/*	{ 0x13f000, 0x13f001, MWA16_NOP },				   ???    */
	{ 0x180000, 0x182fff, pipibibi_videoram16_w },	/* TileRAM */
	{ 0x188000, 0x18800f, pipibibi_scroll_w },
	{ 0x190010, 0x190011, pipibibi_z80_task_w },	/* Z80 task to perform */
	{ 0x19c01c, 0x19c01d, toaplan2_coin_word_w },	/* Coin count/lock */
MEMORY_END

static MEMORY_READ16_START( fixeight_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x200000, 0x200001, input_port_1_word_r },	/* Player 1 controls */
	{ 0x200004, 0x200005, input_port_2_word_r },	/* Player 2 controls */
	{ 0x200008, 0x200009, input_port_3_word_r },	/* Player 3 controls */
	{ 0x200010, 0x200011, input_port_4_word_r },	/* Coin/System inputs */
	{ 0x280000, 0x28dfff, MRA16_RAM },				/* part of shared ram ? */
#if Zx80
	{ 0x28e000, 0x28fbff, shared_ram_r },			/* $28f000 status port */
	{ 0x28fc00, 0x28ffff, Zx80_sharedram_r },		/* 16-bit on 68000 side, 8-bit on Zx80 side */
#else
	{ 0x28e000, 0x28efff, shared_ram_r },
	{ 0x28f000, 0x28f001, Zx80_status_port_r },		/* Zx80 status port */
/*	{ 0x28f002, 0x28f003, MRA16_RAM },				   part of shared ram    */
/*	{ 0x28f004, 0x28f005, input_port_5_word_r },	   Dip Switch A - Wrong !!!    */
/*	{ 0x28f006, 0x28f007, input_port_6_word_r },	   Dip Switch B - Wrong !!!    */
/*	{ 0x28f008, 0x28f009, input_port_7_word_r },	   Territory Jumper block - Wrong !!!    */
/*	{ 0x28f002, 0x28fbff, MRA16_RAM },				   part of shared ram    */
	{ 0x28fc00, 0x28ffff, Zx80_sharedram_r },		/* 16-bit on 68000 side, 8-bit on Zx80 side */
#endif
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },
	{ 0x30000c, 0x30000d, input_port_0_word_r },
	{ 0x400000, 0x400fff, paletteram16_word_r },
	{ 0x500000, 0x501fff, toaplan2_txvideoram16_r },
	{ 0x502000, 0x5021ff, toaplan2_txvideoram16_offs_r },
	{ 0x503000, 0x5031ff, toaplan2_txscrollram16_r },
	{ 0x600000, 0x60ffff, toaplan2_tx_gfxram16_r },
	{ 0x800000, 0x800001, video_count_r },
MEMORY_END

static MEMORY_WRITE16_START( fixeight_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x20001c, 0x20001d, toaplan2_coin_word_w },	/* Coin count/lock */
	{ 0x280000, 0x28dfff, MWA16_RAM },				/* part of shared ram ? */
#if Zx80
	{ 0x28e000, 0x28fbff, shared_ram_w, &toaplan2_shared_ram16 },	/* $28F000 */
	{ 0x28fc00, 0x28ffff, Zx80_sharedram_w, &Zx80_shared_ram },	/* 16-bit on 68000 side, 8-bit on Zx80 side */
#else
	{ 0x28e000, 0x28efff, shared_ram_w, &toaplan2_shared_ram16 },
	{ 0x28f000, 0x28f001, Zx80_command_port_w },	/* Zx80 command port */
/*	{ 0x28f002, 0x28f003, MWA16_RAM },				   part of shared ram    */
/*	{ 0x28f004, 0x28f009, MWA16_NOP },				   part of shared ram    */
/*	{ 0x28f002, 0x28fbff, MWA16_RAM },				   part of shared ram    */
	{ 0x28fc00, 0x28ffff, Zx80_sharedram_w, &Zx80_shared_ram },	/* 16-bit on 68000 side, 8-bit on Zx80 side */
#endif
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },		/* VideoRAM selector/offset */
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x500000, 0x501fff, toaplan2_txvideoram16_w, &toaplan2_txvideoram16, &toaplan2_tx_vram_size },
	{ 0x502000, 0x5021ff, toaplan2_txvideoram16_offs_w, &toaplan2_txvideoram16_offs, &toaplan2_tx_offs_vram_size },
	{ 0x503000, 0x5031ff, toaplan2_txscrollram16_w, &toaplan2_txscrollram16, &toaplan2_tx_scroll_vram_size },
	{ 0x600000, 0x60ffff, toaplan2_tx_gfxram16_w, &toaplan2_tx_gfxram16 },
MEMORY_END

static MEMORY_READ16_START( vfive_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
/*	{ 0x200000, 0x20ffff, MRA16_ROM },				   Sound ROM is here ???    */
	{ 0x200010, 0x200011, input_port_1_word_r },	/* Player 1 controls */
	{ 0x200014, 0x200015, input_port_2_word_r },	/* Player 2 controls */
	{ 0x200018, 0x200019, input_port_3_word_r },	/* Coin/System inputs */
#if Zx80
	{ 0x21e000, 0x21fbff, shared_ram_r },			/* $21f000 status port */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_r },		/* 16-bit on 68000 side, 8-bit on Zx80 side */
#else
	{ 0x21e000, 0x21efff, shared_ram_r },
	{ 0x21f000, 0x21f001, Zx80_status_port_r },		/* Zx80 status port */
	{ 0x21f004, 0x21f005, input_port_4_word_r },	/* Dip Switch A */
	{ 0x21f006, 0x21f007, input_port_5_word_r },	/* Dip Switch B */
	{ 0x21f008, 0x21f009, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_r },		/* 16-bit on 68000 side, 8-bit on Zx80 side */
#endif
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },
	{ 0x30000c, 0x30000d, input_port_0_word_r },
	{ 0x400000, 0x400fff, paletteram16_word_r },
	{ 0x700000, 0x700001, video_count_r },
MEMORY_END

static MEMORY_WRITE16_START( vfive_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
/*	{ 0x200000, 0x20ffff, MWA16_ROM },				   Sound ROM is here ???    */
	{ 0x20001c, 0x20001d, toaplan2_coin_word_w },	/* Coin count/lock */
#if Zx80
	{ 0x21e000, 0x21fbff, shared_ram_w, &toaplan2_shared_ram16 },	/* $21F000 */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_w, &Zx80_shared_ram },	/* 16-bit on 68000 side, 8-bit on Zx80 side */
#else
	{ 0x21e000, 0x21efff, shared_ram_w, &toaplan2_shared_ram16 },
	{ 0x21f000, 0x21f001, Zx80_command_port_w },	/* Zx80 command port */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_w, &Zx80_shared_ram },	/* 16-bit on 68000 side, 8-bit on Zx80 side */
#endif
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },		/* VideoRAM selector/offset */
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( batsugun_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x200010, 0x200011, input_port_1_word_r },	/* Player 1 controls */
	{ 0x200014, 0x200015, input_port_2_word_r },	/* Player 2 controls */
	{ 0x200018, 0x200019, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x210000, 0x21bbff, MRA16_RAM },
#if Zx80
	{ 0x21e000, 0x21fbff, shared_ram_r },			/* $21f000 status port */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_r },		/* 16-bit on 68000 side, 8-bit on Zx80 side */
#else
	{ 0x21e000, 0x21efff, shared_ram_r },
	{ 0x21f000, 0x21f001, Zx80_status_port_r },		/* Zx80 status port */
	{ 0x21f004, 0x21f005, input_port_4_word_r },	/* Dip Switch A */
	{ 0x21f006, 0x21f007, input_port_5_word_r },	/* Dip Switch B */
	{ 0x21f008, 0x21f009, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_r },		/* 16-bit on 68000 side, 8-bit on Zx80 side */
#endif
	/***** The following in 0x30000x are for video controller 2 ******/
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },/* tile layers */
	{ 0x30000c, 0x30000d, input_port_0_word_r },	/* VBlank */
	{ 0x400000, 0x400fff, paletteram16_word_r },
	/***** The following in 0x50000x are for video controller 1 ******/
	{ 0x500004, 0x500007, toaplan2_1_videoram16_r },/* tile layers 2 */
	{ 0x700000, 0x700001, video_count_r },
MEMORY_END

static MEMORY_WRITE16_START( batsugun_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x20001c, 0x20001d, toaplan2_coin_word_w },	/* Coin count/lock */
	{ 0x210000, 0x21bbff, MWA16_RAM },
#if Zx80
	{ 0x21e000, 0x21fbff, shared_ram_w, &toaplan2_shared_ram16 },	/* $21F000 */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_w, &Zx80_shared_ram },	/* 16-bit on 68000 side, 8-bit on Zx80 side */
#else
	{ 0x21e000, 0x21efff, shared_ram_w, &toaplan2_shared_ram16 },
	{ 0x21f000, 0x21f001, Zx80_command_port_w },	/* Zx80 command port */
	{ 0x21fc00, 0x21ffff, Zx80_sharedram_w, &Zx80_shared_ram },	/* 16-bit on 68000 side, 8-bit on Zx80 side */
#endif
	/***** The following in 0x30000x are for video controller 2 ******/
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },		/* VideoRAM selector/offset */
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	/***** The following in 0x50000x are for video controller 1 ******/
	{ 0x500000, 0x500001, toaplan2_1_voffs_w },		/* VideoRAM selector/offset */
	{ 0x500004, 0x500007, toaplan2_1_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x500008, 0x500009, toaplan2_1_scroll_reg_select_w },
	{ 0x50000c, 0x50000d, toaplan2_1_scroll_reg_data_w },
MEMORY_END

static MEMORY_READ16_START( snowbro2_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },/* tile layers */
	{ 0x30000c, 0x30000d, input_port_0_word_r },	/* VBlank */
	{ 0x400000, 0x400fff, paletteram16_word_r },
	{ 0x500002, 0x500003, YM2151_status_port_0_lsb_r },
	{ 0x600000, 0x600001, OKIM6295_status_0_lsb_r },
	{ 0x700000, 0x700001, input_port_8_word_r },	/* Territory Jumper block */
	{ 0x700004, 0x700005, input_port_6_word_r },	/* Dip Switch A */
	{ 0x700008, 0x700009, input_port_7_word_r },	/* Dip Switch B */
	{ 0x70000c, 0x70000d, input_port_1_word_r },	/* Player 1 controls */
	{ 0x700010, 0x700011, input_port_2_word_r },	/* Player 2 controls */
	{ 0x700014, 0x700015, input_port_3_word_r },	/* Player 3 controls */
	{ 0x700018, 0x700019, input_port_4_word_r },	/* Player 4 controls */
	{ 0x70001c, 0x70001d, input_port_5_word_r },	/* Coin/System inputs */
MEMORY_END

static MEMORY_WRITE16_START( snowbro2_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },		/* VideoRAM selector/offset */
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },/* Tile/Sprite VideoRAM */
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x500000, 0x500001, YM2151_register_port_0_lsb_w },
	{ 0x500002, 0x500003, YM2151_data_port_0_lsb_w },
	{ 0x600000, 0x600001, OKIM6295_data_0_lsb_w },
	{ 0x700030, 0x700031, oki_bankswitch_w },		/* Sample bank switch */
	{ 0x700034, 0x700035, toaplan2_coin_word_w },	/* Coin count/lock */
MEMORY_END

static MEMORY_READ16_START( mahoudai_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x218000, 0x21bfff, raizing_shared_ram_r },
	{ 0x21c020, 0x21c021, input_port_1_word_r },	/* Player 1 controls */
	{ 0x21c024, 0x21c025, input_port_2_word_r },	/* Player 2 controls */
	{ 0x21c028, 0x21c029, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x21c02c, 0x21c02d, input_port_4_word_r },	/* Dip Switch A */
	{ 0x21c030, 0x21c031, input_port_5_word_r },	/* Dip Switch B */
	{ 0x21c034, 0x21c035, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x21c03c, 0x21c03d, video_count_r },
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },/* Tile/Sprite VideoRAM */
	{ 0x30000c, 0x30000d, input_port_0_word_r },	/* VBlank */
	{ 0x400000, 0x400fff, paletteram16_word_r },
	{ 0x401000, 0x4017ff, MRA16_RAM },				/* Unused PaletteRAM */
	{ 0x500000, 0x501fff, toaplan2_txvideoram16_r },
	{ 0x502000, 0x5021ff, toaplan2_txvideoram16_offs_r },
	{ 0x502200, 0x502fff, MRA16_RAM },
	{ 0x503000, 0x5031ff, toaplan2_txscrollram16_r },
	{ 0x503200, 0x503fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( mahoudai_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x218000, 0x21bfff, raizing_shared_ram_w },
	{ 0x21c01c, 0x21c01d, toaplan2_coin_word_w },
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x401000, 0x4017ff, MWA16_RAM },
	{ 0x500000, 0x501fff, toaplan2_txvideoram16_w, &toaplan2_txvideoram16, &toaplan2_tx_vram_size },
	{ 0x502000, 0x5021ff, toaplan2_txvideoram16_offs_w, &toaplan2_txvideoram16_offs, &toaplan2_tx_offs_vram_size },
	{ 0x502200, 0x502fff, MWA16_RAM },
	{ 0x503000, 0x5031ff, toaplan2_txscrollram16_w, &toaplan2_txscrollram16, &toaplan2_tx_scroll_vram_size },
	{ 0x503200, 0x503fff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( shippumd_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x218000, 0x21bfff, raizing_shared_ram_r },
	{ 0x21c020, 0x21c021, input_port_1_word_r },	/* Player 1 controls */
	{ 0x21c024, 0x21c025, input_port_2_word_r },	/* Player 2 controls */
	{ 0x21c028, 0x21c029, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x21c02c, 0x21c02d, input_port_4_word_r },	/* Dip Switch A */
	{ 0x21c030, 0x21c031, input_port_5_word_r },	/* Dip Switch B */
	{ 0x21c034, 0x21c035, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x21c03c, 0x21c03d, video_count_r },
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },/* Tile/Sprite VideoRAM */
	{ 0x30000c, 0x30000d, input_port_0_word_r },	/* VBlank */
	{ 0x400000, 0x400fff, paletteram16_word_r },
	{ 0x401000, 0x4017ff, MRA16_RAM },				/* Unused PaletteRAM */
	{ 0x500000, 0x501fff, toaplan2_txvideoram16_r },
	{ 0x502000, 0x5021ff, toaplan2_txvideoram16_offs_r },
	{ 0x502200, 0x502fff, MRA16_RAM },
	{ 0x503000, 0x5031ff, toaplan2_txscrollram16_r },
	{ 0x503200, 0x503fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( shippumd_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x218000, 0x21bfff, raizing_shared_ram_w },
/*	{ 0x21c008, 0x21c009, MWA16_NOP },				   ???    */
	{ 0x21c01c, 0x21c01d, toaplan2_coin_word_w },
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x401000, 0x4017ff, MWA16_RAM },
	{ 0x500000, 0x501fff, toaplan2_txvideoram16_w, &toaplan2_txvideoram16, &toaplan2_tx_vram_size },
	{ 0x502000, 0x5021ff, toaplan2_txvideoram16_offs_w, &toaplan2_txvideoram16_offs, &toaplan2_tx_offs_vram_size },
	{ 0x502200, 0x502fff, MWA16_RAM },
	{ 0x503000, 0x5031ff, toaplan2_txscrollram16_w, &toaplan2_txscrollram16, &toaplan2_tx_scroll_vram_size },
	{ 0x503200, 0x503fff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( battleg_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x218020, 0x218023, battleg_z80check_r },
	{ 0x21c020, 0x21c021, input_port_1_word_r },	/* Player 1 controls */
	{ 0x21c024, 0x21c025, input_port_2_word_r },	/* Player 2 controls */
	{ 0x21c028, 0x21c029, input_port_3_word_r },	/* Coin/System inputs */
	{ 0x21c02c, 0x21c02d, input_port_4_word_r },	/* Dip Switch A */
	{ 0x21c030, 0x21c031, input_port_5_word_r },	/* Dip Switch B */
	{ 0x21c034, 0x21c035, input_port_6_word_r },	/* Territory Jumper block */
	{ 0x21c03c, 0x21c03d, video_count_r },
	{ 0x300004, 0x300007, toaplan2_0_videoram16_r },/* Tile/Sprite VideoRAM */
	{ 0x30000c, 0x30000d, input_port_0_word_r },	/* VBlank */
	{ 0x400000, 0x400fff, paletteram16_word_r },
	{ 0x500000, 0x501fff, toaplan2_txvideoram16_r },
	{ 0x502000, 0x5021ff, toaplan2_txvideoram16_offs_r },
	{ 0x502200, 0x502fff, MRA16_RAM },
	{ 0x503000, 0x5031ff, toaplan2_txscrollram16_r },
	{ 0x503200, 0x503fff, MRA16_RAM },
	{ 0x600000, 0x600fff, battleg_commram_r },		/* CommRAM check */
MEMORY_END

static MEMORY_WRITE16_START( battleg_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x21c01c, 0x21c01d, toaplan2_coin_word_w },
	{ 0x300000, 0x300001, toaplan2_0_voffs_w },
	{ 0x300004, 0x300007, toaplan2_0_videoram16_w },
	{ 0x300008, 0x300009, toaplan2_0_scroll_reg_select_w },
	{ 0x30000c, 0x30000d, toaplan2_0_scroll_reg_data_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x500000, 0x501fff, toaplan2_txvideoram16_w, &toaplan2_txvideoram16, &toaplan2_tx_vram_size },
	{ 0x502000, 0x5021ff, toaplan2_txvideoram16_offs_w, &toaplan2_txvideoram16_offs, &toaplan2_tx_offs_vram_size },
	{ 0x502200, 0x502fff, MWA16_RAM },
	{ 0x503000, 0x5031ff, toaplan2_txscrollram16_w, &toaplan2_txscrollram16, &toaplan2_tx_scroll_vram_size },
	{ 0x503200, 0x503fff, MWA16_RAM },
	{ 0x600000, 0x600fff, battleg_commram_w, &battleg_commram16 },
MEMORY_END

static MEMORY_READ16_START( batrider_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x201fff, toaplan2_txvideoram16_r },/* Text VideoRAM */
	{ 0x202000, 0x202fff, paletteram16_word_r },
	{ 0x203000, 0x2031ff, toaplan2_txvideoram16_offs_r },
	{ 0x203200, 0x2033ff, toaplan2_txscrollram16_r },
	{ 0x203400, 0x207fff, raizing_tx_gfxram16_r },	/* Main RAM actually */
	{ 0x208000, 0x20ffff, MRA16_RAM },
	{ 0x300000, 0x37ffff, batrider_z80rom_r },
	{ 0x400000, 0x400001, input_port_0_word_r },	/* VBlank */
	{ 0x400008, 0x40000b, toaplan2_0_videoram16_r },/* Tile/Sprite VideoRAM */
	{ 0x500000, 0x500001, input_port_1_word_r },
	{ 0x500002, 0x500003, input_port_2_word_r },
	{ 0x500004, 0x500005, input_port_3_word_r },
	{ 0x500006, 0x500007, video_count_r },
	{ 0x500008, 0x50000f, batrider_sound_data_68000_r },
	{ 0x600000, 0x60000f, batrider_sound_data_68000_r1 }, /*/ */
MEMORY_END

static MEMORY_WRITE16_START( batrider_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x201fff, toaplan2_txvideoram16_w, &toaplan2_txvideoram16, &toaplan2_tx_vram_size },
	{ 0x202000, 0x202fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 , &paletteram_size },
	{ 0x203000, 0x2031ff, toaplan2_txvideoram16_offs_w, &toaplan2_txvideoram16_offs, &toaplan2_tx_offs_vram_size },
	{ 0x203200, 0x2033ff, toaplan2_txscrollram16_w, &toaplan2_txscrollram16, &toaplan2_tx_scroll_vram_size },
	{ 0x203400, 0x207fff, raizing_tx_gfxram16_w },
	{ 0x208000, 0x20ffff, MWA16_RAM },
	{ 0x400000, 0x400001, toaplan2_0_scroll_reg_data_w },
	{ 0x400004, 0x400005, toaplan2_0_scroll_reg_select_w },
	{ 0x400008, 0x40000b, toaplan2_0_videoram16_w },
	{ 0x40000c, 0x40000d, toaplan2_0_voffs_w },
	{ 0x500010, 0x500011, toaplan2_coin_word_w },
	{ 0x500020, 0x50002f, batrider_sound_data_68000_w },
	{ 0x500060, 0x500061, batrider_sound_data6_68000_w },
	{ 0x500080, 0x500081, batrider_textdata_decode },
	{ 0x5000c0, 0x5000cf, batrider_objectbank_w },
	{ 0x600000, 0x60000f, batrider_sound_data_68000_w1 }, /*/ */
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xe000, 0xe000, YM3812_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &toaplan2_shared_ram },
	{ 0xe000, 0xe000, YM3812_control_port_0_w },
	{ 0xe001, 0xe001, YM3812_write_port_0_w },
MEMORY_END

static MEMORY_READ_START( raizing_sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe001, 0xe001, YM2151_status_port_0_r },
	{ 0xe004, 0xe004, OKIM6295_status_0_r },
MEMORY_END

static MEMORY_WRITE_START( raizing_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM, &raizing_shared_ram },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
	{ 0xe004, 0xe004, OKIM6295_data_0_w },
	{ 0xe00e, 0xe00e, toaplan2_coin_w },
MEMORY_END

static MEMORY_READ_START( battleg_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe001, 0xe001, YM2151_status_port_0_r },
	{ 0xe004, 0xe004, OKIM6295_status_0_r },
	{ 0xe01c, 0xe01d, battleg_commram_check_r0 },
MEMORY_END

static MEMORY_WRITE_START( battleg_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM, &raizing_shared_ram },
	{ 0xe000, 0xe000, YM2151_register_port_0_w },
	{ 0xe001, 0xe001, YM2151_data_port_0_w },
	{ 0xe004, 0xe004, OKIM6295_data_0_w },
	{ 0xe006, 0xe006, raizing_okim6295_bankselect_0 },
	{ 0xe008, 0xe008, raizing_okim6295_bankselect_1 },
	{ 0xe00a, 0xe00a, battleg_bankswitch_w },
	{ 0xe00c, 0xe00c, battleg_commram_check_w0 },
MEMORY_END

static MEMORY_READ_START( batrider_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_BANK2 },
	{ 0xf000, 0xf007, batrider_sound_data_z80_r1 }, /*/ */
MEMORY_END

static MEMORY_WRITE_START( batrider_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_BANK2 },
	{ 0xf000, 0xf007, batrider_sound_data_z80_w1 }, /*/ */
MEMORY_END

static PORT_READ_START( batrider_sound_readport )
	{ 0x48, 0x4a, batrider_sound_data_z80_r },
/*	{ 0x80, 0x80, YM2151_status_port_0_r }, */
	{ 0x81, 0x81, YM2151_status_port_0_r },
	{ 0x82, 0x82, OKIM6295_status_0_r },
	{ 0x84, 0x84, OKIM6295_status_1_r },
MEMORY_END

static PORT_WRITE_START( batrider_sound_writeport )
	{ 0x40, 0x46, batrider_sound_data_z80_w },
	{ 0x80, 0x80, YM2151_register_port_0_w },
	{ 0x81, 0x81, YM2151_data_port_0_w },
	{ 0x82, 0x82, OKIM6295_data_0_w },
	{ 0x84, 0x84, OKIM6295_data_1_w },
	{ 0x88, 0x88, batrider_bankswitch_w },
	{ 0xc0, 0xc0, raizing_okim6295_bankselect_0 },
	{ 0xc2, 0xc2, raizing_okim6295_bankselect_1 },
	{ 0xc4, 0xc4, raizing_okim6295_bankselect_2 },
	{ 0xc6, 0xc6, raizing_okim6295_bankselect_3 },
PORT_END

#if HD64x180
static MEMORY_READ_START( hd647180_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xfe00, 0xffff, MRA_RAM },			/* Internal 512 bytes of RAM */
MEMORY_END

static MEMORY_WRITE_START( hd647180_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xfe00, 0xffff, MWA_RAM },			/* Internal 512 bytes of RAM */
MEMORY_END
#endif


#if Zx80
static MEMORY_READ_START( Zx80_readmem )
	{ 0x00000, 0x03fff, MRA_ROM },
/*	{ 0x00000, 0x007ff, MRA_RAM },			   External shared RAM (Banked)    */
	{ 0x04000, 0x04000, YM2151_status_port_0_r },
	{ 0x04002, 0x04002, OKIM6295_status_0_r },
	{ 0x04008, 0x04008, input_port_1_r },
	{ 0x0400a, 0x0400a, input_port_2_r },
	{ 0x0400c, 0x0400c, input_port_3_r },
	{ 0x0fe00, 0x0ffff, MRA_RAM },			/* Internal 512 bytes of RAM */
	{ 0x80000, 0x87fff, MRA_RAM },			/* External shared RAM (ROM for KBASH) */
MEMORY_END

static MEMORY_WRITE_START( Zx80_writemem )
	{ 0x00000, 0x03fff, MWA_ROM, },
/*	{ 0x00000, 0x007ff, MWA_RAM, },			   External shared RAM (Banked)    */
	{ 0x04000, 0x04000, YM2151_register_port_0_w },
	{ 0x04001, 0x04001, YM2151_data_port_0_w },
	{ 0x04002, 0x04002, OKIM6295_data_0_w },
	{ 0x04004, 0x04004, oki_bankswitch_w },
	{ 0x0400e, 0x0400e, toaplan2_coin_w },
	{ 0x0fe00, 0x0ffff, MWA_RAM },			/* Internal 512 bytes of RAM */
	{ 0x80000, 0x87fff, MWA_RAM, &Zx80_sharedram },	/* External shared RAM (ROM for KBASH) */
MEMORY_END

static PORT_READ_START( Zx80_readport )
	{ 0x0060, 0x0060, input_port_4_r },		/* Directly mapped I/O ports */
	{ 0x0061, 0x0061, input_port_5_r },		/* Directly mapped I/O ports */
	{ 0x0062, 0x0062, input_port_6_r },		/* Directly mapped I/O ports */
PORT_END
#endif



/*****************************************************************************
	Input Port definitions
	Service input of the TOAPLAN2_SYSTEM_INPUTS is used as a Pause type input.
	If you press then release the following buttons, the following occurs:
	Service & P2 start            : The game will pause.
	P1 start                      : The game will continue.
	Service & P1 start & P2 start : The game will play in slow motion.
*****************************************************************************/

#define  TOAPLAN2_PLAYER_INPUT( player, button3 )								\
	PORT_START																	\
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | player )	\
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | player )	\
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | player )	\
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | player )	\
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 | player )					\
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 | player )					\
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, button3 )									\
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#define  TOAPLAN2_SYSTEM_INPUTS						\
	PORT_START										\
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN3 )	\
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_TILT )	\
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_SERVICE1 )\
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )	\
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )	\
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )	\
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )	\
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#define DSWA_8											\
	PORT_START		/* (4) DSWA */						\
	PORT_DIPNAME( 0x01,	0x00, DEF_STR( Unused ) )		\
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )			\
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )			\
	PORT_DIPNAME( 0x02,	0x00, DEF_STR( Flip_Screen ) )	\
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )			\
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )			\
	PORT_SERVICE( 0x04,	IP_ACTIVE_HIGH )				\
	PORT_DIPNAME( 0x08,	0x00, DEF_STR( Demo_Sounds ) )	\
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )			\
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

#define EUROPEAN_COINAGE_8							\
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Coin_A ) )	\
	PORT_DIPSETTING(	0x30, DEF_STR( 4C_1C ) )	\
	PORT_DIPSETTING(	0x20, DEF_STR( 3C_1C ) )	\
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )	\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )	\
	PORT_DIPNAME( 0xc0,	0x00, DEF_STR( Coin_B ) )	\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )	\
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )	\
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_4C ) )	\
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_6C ) )

#define NONEUROPEAN_COINAGE_8						\
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Coin_A ) )	\
	PORT_DIPSETTING(	0x20, DEF_STR( 2C_1C ) )	\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )	\
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_3C ) )	\
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )	\
	PORT_DIPNAME( 0xc0,	0x00, DEF_STR( Coin_B ) )	\
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )	\
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )	\
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_3C ) )	\
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )

#define EUROPEAN_COINAGE_16								\
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Coin_A ) )	\
	PORT_DIPSETTING(		0x0030, DEF_STR( 4C_1C ) )	\
	PORT_DIPSETTING(		0x0020, DEF_STR( 3C_1C ) )	\
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )	\
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )	\
	PORT_DIPNAME( 0x00c0,	0x0000, DEF_STR( Coin_B ) )	\
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_2C ) )	\
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )	\
	PORT_DIPSETTING(		0x0080, DEF_STR( 1C_4C ) )	\
	PORT_DIPSETTING(		0x00c0, DEF_STR( 1C_6C ) )

#define NONEUROPEAN_COINAGE_16							\
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Coin_A ) )	\
	PORT_DIPSETTING(		0x0020, DEF_STR( 2C_1C ) )	\
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )	\
	PORT_DIPSETTING(		0x0030, DEF_STR( 2C_3C ) )	\
	PORT_DIPSETTING(		0x0010, DEF_STR( 1C_2C ) )	\
	PORT_DIPNAME( 0xc0,		0x0000, DEF_STR( Coin_B ) )	\
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )	\
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )	\
	PORT_DIPSETTING(		0x00c0, DEF_STR( 2C_3C ) )	\
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_2C ) )

#define DIFFICULTY_8									\
	PORT_DIPNAME( 0x03,	0x00, DEF_STR( Difficulty ) )	\
	PORT_DIPSETTING(	0x01, "Easy" )					\
	PORT_DIPSETTING(	0x00, "Medium" )				\
	PORT_DIPSETTING(	0x02, "Hard" )					\
	PORT_DIPSETTING(	0x03, "Hardest" )

#define LIVES_8										\
	PORT_DIPNAME( 0x30,	0x00, DEF_STR( Lives ) )	\
	PORT_DIPSETTING(	0x30, "1" )					\
	PORT_DIPSETTING(	0x20, "2" )					\
	PORT_DIPSETTING(	0x00, "3" )					\
	PORT_DIPSETTING(	0x10, "5" )

#define DIFFICULTY_16										\
	PORT_DIPNAME( 0x0003,	0x0000, DEF_STR( Difficulty ) )	\
	PORT_DIPSETTING(		0x0001, "Easy" )				\
	PORT_DIPSETTING(		0x0000, "Medium" )				\
	PORT_DIPSETTING(		0x0002, "Hard" )				\
	PORT_DIPSETTING(		0x0003, "Hardest" )

#define LIVES_16										\
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )	\
	PORT_DIPSETTING(		0x0030, "1" )				\
	PORT_DIPSETTING(		0x0020, "2" )				\
	PORT_DIPSETTING(		0x0000, "3" )				\
	PORT_DIPSETTING(		0x0010, "5" )




INPUT_PORTS_START( tekipaki )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	DSWA_8
	EUROPEAN_COINAGE_8
/*	NONEUROPEAN_COINAGE_8 */

	PORT_START		/* (5) DSWB */
	DIFFICULTY_8
	PORT_DIPNAME( 0x04,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40,	0x00, "Game Mode" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Stop" )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x0f,	0x02, "Territory" )
	PORT_DIPSETTING(	0x02, "Europe" )
	PORT_DIPSETTING(	0x01, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x03, "Hong Kong" )
	PORT_DIPSETTING(	0x05, "Taiwan" )
	PORT_DIPSETTING(	0x04, "Korea" )
	PORT_DIPSETTING(	0x07, "USA (Romstar)" )
	PORT_DIPSETTING(	0x08, "Hong Kong (Honest Trading Co.)" )
	PORT_DIPSETTING(	0x06, "Taiwan (Spacy Co. Ltd)" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ghox )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	DSWA_8
	EUROPEAN_COINAGE_8
/*	NONEUROPEAN_COINAGE_8 */

	PORT_START		/* (5) DSWB */
	DIFFICULTY_8
	PORT_DIPNAME( 0x0c,	0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "100k and every 200k" )
	PORT_DIPSETTING(	0x04, "100k and every 300k" )
	PORT_DIPSETTING(	0x08, "100k only" )
	PORT_DIPSETTING(	0x0c, "None" )
	LIVES_8
	PORT_BITX(	  0x40,	0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x0f,	0x02, "Territory" )
	PORT_DIPSETTING(	0x02, "Europe" )
	PORT_DIPSETTING(	0x01, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x04, "Korea" )
	PORT_DIPSETTING(	0x03, "Hong Kong (Honest Trading Co." )
	PORT_DIPSETTING(	0x05, "Taiwan" )
	PORT_DIPSETTING(	0x06, "Spain & Portugal (APM Electronics SA)" )
	PORT_DIPSETTING(	0x07, "Italy (Star Electronica SRL)" )
	PORT_DIPSETTING(	0x08, "UK (JP Leisure Ltd)" )
	PORT_DIPSETTING(	0x0a, "Europe (Nova Apparate GMBH & Co)" )
	PORT_DIPSETTING(	0x0d, "Europe (Taito Corporation Japan)" )
	PORT_DIPSETTING(	0x09, "USA (Romstar)" )
	PORT_DIPSETTING(	0x0b, "USA (Taito America Corporation)" )
	PORT_DIPSETTING(	0x0c, "USA (Taito Corporation Japan)" )
	PORT_DIPSETTING(	0x0e, "Japan (Taito Corporation)" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (7)  Paddle 1 (left-right)  read at $100000 */
	PORT_ANALOG( 0xff,	0x00, IPT_DIAL | IPF_PLAYER1, 25, 15, 0, 0xff )

	PORT_START		/* (8)  Paddle 2 (left-right)  read at $040000 */
	PORT_ANALOG( 0xff,	0x00, IPT_DIAL | IPF_PLAYER2, 25, 15, 0, 0xff )

	PORT_START		/* (9)  Paddle 1 (fake up-down) */
	PORT_ANALOG( 0xff,	0x00, IPT_DIAL_V | IPF_PLAYER1, 15, 0, 0, 0xff )

	PORT_START		/* (10) Paddle 2 (fake up-down) */
	PORT_ANALOG( 0xff,	0x00, IPT_DIAL_V | IPF_PLAYER2, 15, 0, 0, 0xff )
INPUT_PORTS_END

INPUT_PORTS_START( dogyuun )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 | IPF_PLAYER1 )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 | IPF_PLAYER2 )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Free_Play) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	EUROPEAN_COINAGE_16
/*	NONEUROPEAN_COINAGE_16 */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0004, "200k, 400k and 600k" )
	PORT_DIPSETTING(		0x0000, "200k only" )
	PORT_DIPSETTING(		0x0008, "400k only" )
	PORT_DIPSETTING(		0x000c, "None" )
	LIVES_16
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x0f,	0x03, "Territory" )
	PORT_DIPSETTING(	0x03, "Europe" )
	PORT_DIPSETTING(	0x01, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x05, "Korea (Unite Trading license)" )
	PORT_DIPSETTING(	0x04, "Hong Kong (Charterfield license)" )
	PORT_DIPSETTING(	0x06, "Taiwan" )
	PORT_DIPSETTING(	0x08, "South East Asia (Charterfield license)" )
	PORT_DIPSETTING(	0x0c, "USA (Atari Games Corp license)" )
	PORT_DIPSETTING(	0x0f, "Japan (Taito Corp license)" )
/*	Duplicate settings
	PORT_DIPSETTING(	0x0b, "Europe" )
	PORT_DIPSETTING(	0x07, "USA" )
	PORT_DIPSETTING(	0x0a, "Korea (Unite Trading license)" )
	PORT_DIPSETTING(	0x09, "Hong Kong (Charterfield license)" )
	PORT_DIPSETTING(	0x0b, "Taiwan" )
	PORT_DIPSETTING(	0x0d, "South East Asia (Charterfield license)" )
	PORT_DIPSETTING(	0x0c, "USA (Atari Games Corp license)" )
*/
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */
INPUT_PORTS_END

INPUT_PORTS_START( kbash )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 | IPF_PLAYER1 )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 | IPF_PLAYER2 )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, "Continue Mode" )
	PORT_DIPSETTING(		0x0000, "Normal" )
	PORT_DIPSETTING(		0x0001, "Discount" )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	EUROPEAN_COINAGE_16
/*	NONEUROPEAN_COINAGE_16 */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0000, "100k and every 400k" )
	PORT_DIPSETTING(		0x0004, "100k only" )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x000c, "None" )
	/* Lives are different in this game */
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0000, "2" )
	PORT_DIPSETTING(		0x0020, "3" )
	PORT_DIPSETTING(		0x0010, "4" )
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x000f,	0x000a, "Territory" )
	PORT_DIPSETTING(		0x000a, "Europe" )
	PORT_DIPSETTING(		0x0009, "USA" )
	PORT_DIPSETTING(		0x0000, "Japan" )
	PORT_DIPSETTING(		0x0003, "Korea" )
	PORT_DIPSETTING(		0x0004, "Hong Kong" )
	PORT_DIPSETTING(		0x0007, "Taiwan" )
	PORT_DIPSETTING(		0x0006, "South East Asia" )
	PORT_DIPSETTING(		0x0002, "Europe, USA (Atari License)" )
	PORT_DIPSETTING(		0x0001, "USA, Europe (Atari License)" )
	PORT_BIT( 0xfff0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( truxton2 )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	EUROPEAN_COINAGE_16
/*	NONEUROPEAN_COINAGE_16 */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0000, "70k and 200k" )
	PORT_DIPSETTING(		0x0004, "100k and 250k" )
	PORT_DIPSETTING(		0x0008, "100k only" )
	PORT_DIPSETTING(		0x000c, "200k only" )
	LIVES_16
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x07,	0x02, "Territory" )
	PORT_DIPSETTING(	0x02, "Europe" )
	PORT_DIPSETTING(	0x01, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x03, "Hong Kong" )
	PORT_DIPSETTING(	0x05, "Taiwan" )
	PORT_DIPSETTING(	0x06, "Asia" )
	PORT_DIPSETTING(	0x04, "Korea" )
INPUT_PORTS_END

INPUT_PORTS_START( pipibibs )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	DSWA_8
	EUROPEAN_COINAGE_8
/*	NONEUROPEAN_COINAGE_8 */

	PORT_START		/* (5) DSWB */
	DIFFICULTY_8
	PORT_DIPNAME( 0x0c,	0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x04, "150k and every 200k" )
	PORT_DIPSETTING(	0x00, "200k and every 300k" )
	PORT_DIPSETTING(	0x08, "200k only" )
	PORT_DIPSETTING(	0x0c, "None" )
	LIVES_8
	PORT_BITX(	  0x40,	0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x07,	0x06, "Territory" )
	PORT_DIPSETTING(	0x06, "Europe" )
	PORT_DIPSETTING(	0x04, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x02, "Hong Kong" )
	PORT_DIPSETTING(	0x03, "Taiwan" )
	PORT_DIPSETTING(	0x01, "Asia" )
	PORT_DIPSETTING(	0x07, "Europe (Nova Apparate GMBH & Co)" )
	PORT_DIPSETTING(	0x05, "USA (Romstar)" )
	PORT_DIPNAME( 0x08,	0x00, "Nudity" )
	PORT_DIPSETTING(	0x08, "Low" )
	PORT_DIPSETTING(	0x00, "High, but censored" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( whoopee )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	DSWA_8
/*	EUROPEAN_COINAGE_8 */
	NONEUROPEAN_COINAGE_8

	PORT_START		/* (5) DSWB */
	DIFFICULTY_8
	PORT_DIPNAME( 0x0c,	0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x04, "150k and every 200k" )
	PORT_DIPSETTING(	0x00, "200k and every 300k" )
	PORT_DIPSETTING(	0x08, "200k only" )
	PORT_DIPSETTING(	0x0c, "None" )
	LIVES_8
	PORT_BITX(	  0x40,	0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x07,	0x00, "Territory" )
	PORT_DIPSETTING(	0x06, "Europe" )
	PORT_DIPSETTING(	0x04, "USA" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x02, "Hong Kong" )
	PORT_DIPSETTING(	0x03, "Taiwan" )
	PORT_DIPSETTING(	0x01, "Asia" )
	PORT_DIPSETTING(	0x07, "Europe (Nova Apparate GMBH & Co)" )
	PORT_DIPSETTING(	0x05, "USA (Romstar)" )
	PORT_DIPNAME( 0x08,	0x08, "Nudity" )
	PORT_DIPSETTING(	0x08, "Low" )
	PORT_DIPSETTING(	0x00, "High, but censored" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */
INPUT_PORTS_END

INPUT_PORTS_START( pipibibi )
	PORT_START		/* (0) VBlank */
/*	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )		   This video HW    */
/*	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )		   doesnt wait for VBlank    */

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x01,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	/* This video HW doesn't support flip screen */
/*	PORT_DIPNAME( 0x02,	0x00, DEF_STR( Flip_Screen ) ) */
/*	PORT_DIPSETTING(	0x00, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(	0x02, DEF_STR( On ) ) */
	PORT_SERVICE( 0x04,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x08,	0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
/*	EUROPEAN_COINAGE_8 */
	NONEUROPEAN_COINAGE_8

	PORT_START		/* (5) DSWB */
	DIFFICULTY_8
	PORT_DIPNAME( 0x0c,	0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x04, "150k and every 200k" )
	PORT_DIPSETTING(	0x00, "200k and every 300k" )
	PORT_DIPSETTING(	0x08, "200k only" )
	PORT_DIPSETTING(	0x0c, "None" )
	LIVES_8
	PORT_BITX(	  0x40,	0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x07,	0x05, "Territory" )
	PORT_DIPSETTING(	0x07, "World (Ryouta Kikaku)" )
	PORT_DIPSETTING(	0x00, "Japan (Ryouta Kikaku)" )
	PORT_DIPSETTING(	0x02, "World" )
	PORT_DIPSETTING(	0x05, "Europe" )
	PORT_DIPSETTING(	0x04, "USA" )
	PORT_DIPSETTING(	0x01, "Hong Kong (Honest Trading Co." )
	PORT_DIPSETTING(	0x06, "Spain & Portugal (APM Electronics SA)" )
/*	PORT_DIPSETTING(	0x03, "World" ) */
	PORT_DIPNAME( 0x08,	0x00, "Nudity" )
	PORT_DIPSETTING(	0x08, "Low" )
	PORT_DIPSETTING(	0x00, "High, but censored" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( fixeight )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER3, IPT_START3 )

	PORT_START		/* service input is a push-button marked 'Test SW' */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BITX(0x0004, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#if 0
	PORT_START		/* Fake input, to display message */
	PORT_DIPNAME( 0x00,	0x00, "    Press service button" )
	PORT_DIPSETTING(	0x00, "" )
	PORT_DIPNAME( 0x00,	0x00, "  for game keeping options" )
	PORT_DIPSETTING(	0x00, "" )
	PORT_DIPNAME( 0x00,	0x00, "" )
	PORT_DIPSETTING(	0x00, "" )
#endif

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, "Maximum Players" )
	PORT_DIPSETTING(		0x0000, "2" )
	PORT_DIPSETTING(		0x0001, "3" )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004,	0x0004, "Shooting style" )
	PORT_DIPSETTING(		0x0004, "Semi-auto" )
	PORT_DIPSETTING(		0x0000, "Fully-auto" )
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	EUROPEAN_COINAGE_16
/*	NONEUROPEAN_COINAGE_16 */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0004, "300k and every 300k" )
	PORT_DIPSETTING(		0x0008, "300k only" )
	PORT_DIPSETTING(		0x0000, "500k and every 500k" )
	PORT_DIPSETTING(		0x000c, "None" )
	LIVES_16
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x0f,	0x09, "Territory" )
	PORT_DIPSETTING(	0x09, "Europe" )
	PORT_DIPSETTING(	0x08, "Europe (Taito Corp)" )
	PORT_DIPSETTING(	0x0b, "USA" )
	PORT_DIPSETTING(	0x0a, "USA (Taito America Corp)" )
	PORT_DIPSETTING(	0x0e, "Japan" )
	PORT_DIPSETTING(	0x0f, "Japan (Taito corp)" )
	PORT_DIPSETTING(	0x01, "Korea" )
	PORT_DIPSETTING(	0x00, "Korea (Taito Corp)" )
	PORT_DIPSETTING(	0x03, "Hong Kong" )
	PORT_DIPSETTING(	0x02, "Hong Kong (Taito Corp)" )
	PORT_DIPSETTING(	0x05, "Taiwan" )
	PORT_DIPSETTING(	0x04, "Taiwan (Taito corp)" )
	PORT_DIPSETTING(	0x07, "South East Asia" )
	PORT_DIPSETTING(	0x06, "South East Asia (Taito corp)" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( grindstm )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	EUROPEAN_COINAGE_16
/*	NONEUROPEAN_COINAGE_16 */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x0004, "300k and every 800k" )
	PORT_DIPSETTING(		0x0000, "300k and 800k" )
	PORT_DIPSETTING(		0x000c, "None" )
	LIVES_16
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x0f,	0x08, "Territory" )
	PORT_DIPSETTING(	0x08, "Europe" )
	PORT_DIPSETTING(	0x0b, "USA" )
	PORT_DIPSETTING(	0x01, "Korea" )
	PORT_DIPSETTING(	0x03, "Hong Kong" )
	PORT_DIPSETTING(	0x05, "Taiwan" )
	PORT_DIPSETTING(	0x07, "South East Asia" )
	PORT_DIPSETTING(	0x0a, "USA (American Sammy Corporation License)" )
	PORT_DIPSETTING(	0x00, "Korea (Unite Trading License)" )
	PORT_DIPSETTING(	0x02, "Hong Kong (Charterfield License)" )
	PORT_DIPSETTING(	0x04, "Taiwan (Anomoto International Inc License)" )
	PORT_DIPSETTING(	0x06, "South East Asia (Charterfield License)" )
/*	Duplicate settings
	PORT_DIPSETTING(	0x09, "Europe" )
	PORT_DIPSETTING(	0x0d, "USA" )
	PORT_DIPSETTING(	0x0e, "Korea" )
	PORT_DIPSETTING(	0x0f, "Korea" )
	PORT_DIPSETTING(	0x0c, "USA (American Sammy Corporation License)" )
*/
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */
INPUT_PORTS_END

INPUT_PORTS_START( vfive )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Upright ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	NONEUROPEAN_COINAGE_16
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x0004, "300k and every 800k" )
	PORT_DIPSETTING(		0x0000, "300k and 800k" )
	PORT_DIPSETTING(		0x000c, "None" )
	LIVES_16
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_START		/* (6) Territory Jumper block */
	/* Territory is forced to Japan in this set. */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */
INPUT_PORTS_END

INPUT_PORTS_START( batsugun )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, "Continue Mode" )
	PORT_DIPSETTING(		0x0000, "Normal" )
	PORT_DIPSETTING(		0x0001, "Discount" )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	EUROPEAN_COINAGE_16
/*	NONEUROPEAN_COINAGE_16 */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0004, "500k and every 600k" )
	PORT_DIPSETTING(		0x0000, "1000k only" )
	PORT_DIPSETTING(		0x0008, "1500k only" )
	PORT_DIPSETTING(		0x000c, "None" )
	LIVES_16
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_DIPNAME( 0x000f,	0x0009, "Territory" )
	PORT_DIPSETTING(		0x0009, "Europe" )
	PORT_DIPSETTING(		0x000b, "USA" )
	PORT_DIPSETTING(		0x000e, "Japan" )
/*	PORT_DIPSETTING(		0x000f, "Japan" ) */
	PORT_DIPSETTING(		0x0001, "Korea" )
	PORT_DIPSETTING(		0x0003, "Hong Kong" )
	PORT_DIPSETTING(		0x0005, "Taiwan" )
	PORT_DIPSETTING(		0x0007, "South East Asia" )
	PORT_DIPSETTING(		0x0008, "Europe (Taito Corp License)" )
	PORT_DIPSETTING(		0x000a, "USA (Taito Corp License)" )
	PORT_DIPSETTING(		0x000c, "Japan (Taito Corp License)" )
/*	PORT_DIPSETTING(		0x000d, "Japan (Taito Corp License)" ) */
	PORT_DIPSETTING(		0x0000, "Korea (Unite Trading License)" )
	PORT_DIPSETTING(		0x0002, "Hong Kong (Taito Corp License)" )
	PORT_DIPSETTING(		0x0004, "Taiwan (Taito Corp License)" )
	PORT_DIPSETTING(		0x0006, "South East Asia (Taito Corp License)" )
	PORT_BIT( 0xfff0, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* bit 0x10 sound ready */
INPUT_PORTS_END

INPUT_PORTS_START( snowbro2 )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER3, IPT_START3 )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER4, IPT_START4 )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (6) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, "Continue Mode" )
	PORT_DIPSETTING(		0x0000, "Normal" )
	PORT_DIPSETTING(		0x0001, "Discount" )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	NONEUROPEAN_COINAGE_16
	/*  The following are listed in service mode for European territory,
		but are not actually used in game play. */
/*	EUROPEAN_COINAGE_16 */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (7) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0004, "100k and every 500k" )
	PORT_DIPSETTING(		0x0000, "100k only" )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x000c, "None" )
	/* Lives have one different value */
	PORT_DIPNAME( 0x0030,	0x0000, DEF_STR( Lives ) )
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "4" )
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Maximum Players" )
	PORT_DIPSETTING(		0x0080, "2" )
	PORT_DIPSETTING(		0x0000, "4" )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (8) Territory Jumper block */
	PORT_DIPNAME( 0x1c00,	0x0800, "Territory" )
	PORT_DIPSETTING(		0x0800, "Europe" )
	PORT_DIPSETTING(		0x0400, "USA" )
	PORT_DIPSETTING(		0x0000, "Japan" )
	PORT_DIPSETTING(		0x0c00, "Korea" )
	PORT_DIPSETTING(		0x1000, "Hong Kong" )
	PORT_DIPSETTING(		0x1400, "Taiwan" )
	PORT_DIPSETTING(		0x1800, "South East Asia" )
	PORT_DIPSETTING(		0x1c00, DEF_STR( Unused ) )
	PORT_DIPNAME( 0x2000,	0x0000, "Show All Rights Reserved" )
	PORT_DIPSETTING(		0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(		0x2000, DEF_STR( Yes ) )
	PORT_BIT( 0xc3ff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( mahoudai )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	NONEUROPEAN_COINAGE_16
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0004, "200k and 500k" )
	PORT_DIPSETTING(		0x0000, "Every 300k" )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x000c, "None" )
	LIVES_16
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )

	PORT_START		/* (6) Territory Jumper block */
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* not used, it seems */
INPUT_PORTS_END

INPUT_PORTS_START( shippumd )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_DIPNAME( 0x0001,	0x0000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0002, DEF_STR( On ) )
	PORT_SERVICE( 0x0004,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	NONEUROPEAN_COINAGE_16
	/*  When Territory is set to Europe, the Coin A and B have
		different values */
/*	EUROPEAN_COINAGE_16 */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x000c,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x0004, "200k and 500k" )
	PORT_DIPSETTING(		0x0000, "Every 300k" )
	PORT_DIPSETTING(		0x0008, "200k only" )
	PORT_DIPSETTING(		0x000c, "None" )
	LIVES_16
	PORT_BITX(	  0x0040,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0080, DEF_STR( No ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Yes ) )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (6) Territory Jumper block */
	/* Title screen is wrong when set to other countries */
	/* It suggests text ROM may be different for other territories */
	PORT_DIPNAME( 0x000e,	0x0000, "Territory" )
	PORT_DIPSETTING(		0x0004, "Europe" )
	PORT_DIPSETTING(		0x0002, "USA" )
	PORT_DIPSETTING(		0x0000, "Japan" )
	PORT_DIPSETTING(		0x0006, "South East Asia" )
	PORT_DIPSETTING(		0x0008, "China" )
	PORT_DIPSETTING(		0x000a, "Korea" )
	PORT_DIPSETTING(		0x000c, "Hong Kong" )
	PORT_DIPSETTING(		0x000e, "Taiwan" )
	PORT_BIT( 0xfff1, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( battleg )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 | IPF_PLAYER1 )

	TOAPLAN2_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 | IPF_PLAYER2 )

	TOAPLAN2_SYSTEM_INPUTS

	PORT_START		/* (4) DSWA */
	PORT_SERVICE( 0x0001,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0002,	0x0000, "Credits to Start" )
	PORT_DIPSETTING(		0x0000, "1" )
	PORT_DIPSETTING(		0x0002, "2" )
	PORT_DIPNAME( 0x001c,	0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(		0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x0014, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(		0x001c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0,	0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(		0x00c0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x00a0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x0060, DEF_STR( 1C_4C ) )
/*	PORT_DIPSETTING(		0x00e0, DEF_STR( 1C_1C ) ) */
	/*  When Coin_A is set to Free_Play, Coin_A becomes Coin_A and Coin_B,
		and Coin_B becomes the following dips */
/*	PORT_DIPNAME( 0x0020,	0x0000, "Stick Mode" ) */
/*	PORT_DIPSETTING(		0x0000, "Special" ) */
/*	PORT_DIPSETTING(		0x0020, "Normal" ) */
/*	PORT_DIPNAME( 0x0040,	0x0000, "Effect" ) */
/*	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(		0x0040, DEF_STR( On ) ) */
/*	PORT_DIPNAME( 0x0080,	0x0000, "Music" ) */
/*	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(		0x0080, DEF_STR( On ) ) */
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (5) DSWB */
	DIFFICULTY_16
	PORT_DIPNAME( 0x0004,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0070,	0x0000, DEF_STR( Lives ) )
	PORT_DIPSETTING(		0x0030, "1" )
	PORT_DIPSETTING(		0x0020, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x0010, "4" )
	PORT_DIPSETTING(		0x0040, "5" )
	PORT_DIPSETTING(		0x0050, "6" )
	PORT_BITX( 0,			0x0060, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", 0, 0 )
/*	PORT_BITX( 0,			0x0070, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Invulnerability", 0, 0 ) */
	PORT_DIPNAME( 0x0080,	0x0000, DEF_STR( Bonus_Life ) )
	/* Bonus_Life for Non European territories */
/*	PORT_DIPSETTING(		0x0000, "Every 1000k" ) */
/*	PORT_DIPSETTING(		0x0080, "1000k and 2000k" ) */
	/* Bonus_Life values for European territories */
	PORT_DIPSETTING(		0x0080, "Every 2000k" )
	PORT_DIPSETTING(		0x0000, "None" )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (6) DSWC / Territory Jumper block */
	PORT_DIPNAME( 0x0004,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008,	0x0000, "Stage Edit" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0003,	0x0001, "Territory" )
	PORT_DIPSETTING(		0x0001, "Europe (German Tuning license)" )
	PORT_DIPSETTING(		0x0002, "USA (Fabtek license)" )
	PORT_DIPSETTING(		0x0000, "Japan" )
	PORT_DIPSETTING(		0x0003, "Asia" )
	PORT_BIT( 0xffe0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( batrider )
	PORT_START		/* (0) VBlank */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (1) Player Inputs */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8080, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* (2) Coin/System and DSWC */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x0100,	0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200,	0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(		0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400,	0x0000, "Stage Edit" )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800,	0x0000, "Allow Continue" )
	PORT_DIPSETTING(		0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( On ) )
	PORT_BITX(	  0x1000,	0x0000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x1000, DEF_STR( On ) )
	/*  These Dips are showed only when Coin_A is set to Free_Play.
		They are the last 3 Unused dips. Seems to be debug options */
/*	PORT_DIPNAME( 0x2000,	0x0000, "Guest Player" ) */
/*	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(		0x2000, DEF_STR( On ) ) */
/*	PORT_DIPNAME( 0x4000,	0x0000, "Player Select" ) */
/*	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(		0x4000, DEF_STR( On ) ) */
/*	PORT_DIPNAME( 0x8000,	0x0000, "Special Course" ) */
/*	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(		0x8000, DEF_STR( On ) ) */
	PORT_DIPNAME( 0x2000,	0x0000, DEF_STR( Unused ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000,	0x0000, DEF_STR( Unused ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000,	0x0000, DEF_STR( Unused ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(		0x8000, DEF_STR( On ) )

	PORT_START		/* (3) DSWA and DSWB */
	PORT_SERVICE( 0x0001,	IP_ACTIVE_HIGH )		/* Service Mode */
	PORT_DIPNAME( 0x0002,	0x0000, "Credits to Start" )
	PORT_DIPSETTING(		0x0000, "1" )
	PORT_DIPSETTING(		0x0002, "2" )
	/* When Coin_A is set to Free_Play, dip 0x0002 becomes: */
/*	PORT_DIPNAME( 0x0002,	0x0000, "Stick Mode" ) */
/*	PORT_DIPSETTING(		0x0000, "Normal" ) */
/*	PORT_DIPSETTING(		0x0002, "Special" ) */
	PORT_DIPNAME( 0x001c,	0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(		0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x0014, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(		0x001c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0,	0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(		0x00c0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(		0x00a0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(		0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(		0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(		0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(		0x0040, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(		0x0060, DEF_STR( 1C_4C ) )
/*	PORT_DIPSETTING(		0x00e0, DEF_STR( 1C_1C ) ) */
	/* Coin_B becomes the followings dips, when Coin_A is set to Free_Play */
/*	PORT_DIPNAME( 0x0020,	0x0000, "Hit Score" ) */
/*	PORT_DIPSETTING(		0x0020, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(		0x0000, DEF_STR( On ) ) */
/*	PORT_DIPNAME( 0x0040,	0x0000, "Sound Effect" ) */
/*	PORT_DIPSETTING(		0x0040, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(		0x0000, DEF_STR( On ) ) */
/*	PORT_DIPNAME( 0x0080,	0x0000, "Music" ) */
/*	PORT_DIPSETTING(		0x0080, DEF_STR( Off ) ) */
/*	PORT_DIPSETTING(		0x0000, DEF_STR( On ) ) */
	PORT_DIPNAME( 0x0300,	0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(		0x0100, "Easy" )
	PORT_DIPSETTING(		0x0000, "Medium" )
	PORT_DIPSETTING(		0x0200, "Hard" )
	PORT_DIPSETTING(		0x0300, "Hardest" )
	PORT_DIPNAME( 0x0c00,	0x0000, "Timer" )
	PORT_DIPSETTING(		0x0400, "Easy" )
	PORT_DIPSETTING(		0x0000, "Medium" )
	PORT_DIPSETTING(		0x0800, "Hard" )
	PORT_DIPSETTING(		0x0c00, "Hardest" )
	PORT_DIPNAME( 0x3000,	0x0000, DEF_STR( Lives ) )
	PORT_DIPSETTING(		0x3000, "1" )
	PORT_DIPSETTING(		0x2000, "2" )
	PORT_DIPSETTING(		0x0000, "3" )
	PORT_DIPSETTING(		0x1000, "4" )
	PORT_DIPNAME( 0xc000,	0x0000, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(		0x4000, "Every 1000k" )
	PORT_DIPSETTING(		0x0000, "Every 1500k" )
	PORT_DIPSETTING(		0x8000, "Every 2000k" )
	PORT_DIPSETTING(		0xc000, "None" )
INPUT_PORTS_END


static struct GfxLayout tilelayout =
{
	16,16,	/* 16x16 */
	RGN_FRAC(1,2),	/* Number of tiles */
	4,		/* 4 bits per pixel */
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	8*4*16
};

static struct GfxLayout spritelayout =
{
	8,8,	/* 8x8 */
	RGN_FRAC(1,2),	/* Number of 8x8 sprites */
	4,		/* 4 bits per pixel */
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static struct GfxLayout raizing_textlayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

#ifdef LSB_FIRST
static struct GfxLayout truxton2_tx_tilelayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 16, 20, 32, 36, 48, 52 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};
#else
static struct GfxLayout truxton2_tx_tilelayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0+8, 4+8, 16+8, 20+8, 32+8, 36+8, 48+8, 52+8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};
#endif

#ifdef LSB_FIRST
static struct GfxLayout batrider_tx_tilelayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};
#else
static struct GfxLayout batrider_tx_tilelayout =
{
	8,8,	/* 8x8 characters */
	1024,	/* 1024 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 8, 12, 0, 4, 24, 28, 16, 20 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};
#endif

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0, 128 },
	{ REGION_GFX1, 0, &spritelayout, 0,  64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_2[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0, 128 },
	{ REGION_GFX1, 0, &spritelayout, 0,  64 },
	{ REGION_GFX2, 0, &tilelayout,   0, 128 },
	{ REGION_GFX2, 0, &spritelayout, 0,  64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo truxton2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0,       &tilelayout            , 0, 128 },
	{ REGION_GFX1, 0,       &spritelayout          , 0,  64 },
/*	{ REGION_CPU1, 0x40000, &truxton2_tx_tilelayout, 0, 128 },	   Truxton 2    */
/*	{ REGION_CPU1, 0x68000, &truxton2_tx_tilelayout, 0, 128 },	   Fix Eight    */
	{ 0, 0, &truxton2_tx_tilelayout,  0, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo raizing_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,         0, 128 },
	{ REGION_GFX1, 0, &spritelayout,       0,  64 },
	{ REGION_GFX2, 0, &raizing_textlayout, 0, 128 },		/* Extra-text layer */
	{ -1 } /* end of array */
};

/* This is wrong a bit. Text layer is dynamically changed. */
static struct GfxDecodeInfo batrider_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,             0, 128 },
	{ REGION_GFX1, 0, &spritelayout,           0,  64 },
	{ 0,           0, &batrider_tx_tilelayout, 0,  16 },
	{ -1 } /* end of array */
};


static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip  */
	27000000/8,		/* 3.375MHz , 27MHz Oscillator */
	{ 100 },		/* volume */
	{ irqhandler },
};

static struct YM2151interface ym2151_interface =
{
	1,				/* 1 chip */
	27000000/8,		/* 3.375MHz , 27MHz Oscillator */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct YM2151interface raizing_ym2151_interface =
{
	1,				/* 1 chip */
	32000000/8,		/* 4.00MHz , 32MHz Oscillator */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct OKIM6295interface okim6295_interface =
{
	1,						/* 1 chip */
	{ 27000000/10/132 },	/* frequency (Hz). 2.7MHz to 6295 (using B mode) */
	{ REGION_SOUND1 },		/* memory region */
	{ 47 }
};

static struct OKIM6295interface raizing_okim6295_interface =
{
	1,						/* 1 chip */
	{ 32000000/32/132 },	/* frequency (Hz) 1MHz to 6295 (using B mode) */
	{ REGION_SOUND1 },		/* memory region */
	{ 47 }
};

static struct OKIM6295interface battleg_okim6295_interface =
{
	1,						/* 1 chip */
	{ 32000000/16/132 },	/* frequency (Hz). 2MHz to 6295 (using B mode) */
	{ REGION_SOUND1 },		/* memory region */
	{ 47 }
};

static struct OKIM6295interface batrider_okim6295_interface =
{
	2,										/* 2 chips */
	{ 32000000/10/132, 32000000/10/165 },	/* frequency (Hz). 3.2MHz to two 6295 (using B mode / A mode) */
	{ REGION_SOUND1, REGION_SOUND2 },		/* memory region */
	{ 47, 47 }
};


static struct MachineDriver machine_driver_tekipaki =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,			/* 10MHz Oscillator */
			tekipaki_readmem,tekipaki_writemem,0,0,
			toaplan2_interrupt,1
		},
#if HD64x180
		{
			CPU_Z180,			/* HD647180 CPU actually */
			10000000,			/* 10MHz Oscillator */
			hd647180_readmem,hd647180_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_toaplan2,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	toaplan2_0_vh_start,
	toaplan2_0_vh_stop,
	toaplan2_0_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

static struct MachineDriver machine_driver_ghox =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,			/* 10MHz Oscillator */
			ghox_readmem,ghox_writemem,0,0,
			toaplan2_interrupt,1
		},
#if HD64x180
		{
			CPU_Z180,			/* HD647180 CPU actually */
			10000000,			/* 10MHz Oscillator */
			hd647180_readmem,hd647180_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_toaplan2,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	toaplan2_0_vh_start,
	toaplan2_0_vh_stop,
	toaplan2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

static struct MachineDriver machine_driver_dogyuun =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,			/* 16MHz Oscillator */
			dogyuun_readmem,dogyuun_writemem,0,0,
			toaplan2_interrupt,1
		},
#if Zx80
		{
			CPU_Z180,			/* Z?80 type Toaplan marked CPU ??? */
			16000000,			/* 16MHz Oscillator */
			Zx80_readmem,Zx80_writemem,Zx80_readport,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_toaplan3,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo_2,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_1_eof_callback,
	toaplan2_1_vh_start,
	toaplan2_1_vh_stop,
	dogyuun_1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
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

static struct MachineDriver machine_driver_kbash =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,			/* 16MHz Oscillator */
			kbash_readmem,kbash_writemem,0,0,
			toaplan2_interrupt,1
		},
#if Zx80
		{
			CPU_Z180,			/* Z?80 type Toaplan marked CPU ??? */
			16000000,			/* 16MHz Oscillator */
			Zx80_readmem,Zx80_writemem,Zx80_readport,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_toaplan2,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	toaplan2_0_vh_start,
	toaplan2_0_vh_stop,
	toaplan2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
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

static struct MachineDriver machine_driver_truxton2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,			/* 16MHz Oscillator */
			truxton2_readmem,truxton2_writemem,0,0,
			truxton2_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_truxton2,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	truxton2_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	truxton2_0_vh_start,
	toaplan2_0_vh_stop,
	truxton2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
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

static struct MachineDriver machine_driver_pipibibs =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,			/* 10MHz Oscillator */
			pipibibs_readmem,pipibibs_writemem,0,0,
			toaplan2_interrupt,1
		},
		{
			CPU_Z80,
			27000000/8,			/* ??? 3.37MHz , 27MHz Oscillator */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	init_pipibibs,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo,
	(128*16), (128*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	toaplan2_0_vh_start,
	toaplan2_0_vh_stop,
	toaplan2_0_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

static struct MachineDriver machine_driver_whoopee =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,			/* 10MHz Oscillator */
			tekipaki_readmem,tekipaki_writemem,0,0,
			toaplan2_interrupt,1
		},
		{
			CPU_Z80,			/* This should be a HD647180 */
			27000000/8,			/* Change this to 10MHz when HD647180 gets dumped. 10MHz Oscillator */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	init_pipibibs,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo,
	(128*16), (128*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	toaplan2_0_vh_start,
	toaplan2_0_vh_stop,
	toaplan2_0_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

static struct MachineDriver machine_driver_pipibibi =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,			/* 10MHz Oscillator */
			pipibibi_readmem,pipibibi_writemem,0,0,
			toaplan2_interrupt,1
		},
		{
			CPU_Z80,
			27000000/8,			/* ??? 3.37MHz */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	init_pipibibs,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo,
	(128*16), (128*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	toaplan2_0_vh_start,
	toaplan2_0_vh_stop,
	toaplan2_0_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};

static struct MachineDriver machine_driver_fixeight =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,			/* 16MHz Oscillator */
			fixeight_readmem,fixeight_writemem,0,0,
			toaplan2_interrupt,1
		},
#if Zx80
		{
			CPU_Z180,			/* Z?80 type Toaplan marked CPU ??? */
			16000000,			/* 16MHz Oscillator */
			Zx80_readmem,Zx80_writemem,Zx80_readport,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_fixeight,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	truxton2_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	truxton2_0_vh_start,
	toaplan2_0_vh_stop,
	truxton2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
/*/	fixeight_nvram_handler		   See 37B6 code    */
};

static struct MachineDriver machine_driver_vfive =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,			/* 10MHz Oscillator */
			vfive_readmem,vfive_writemem,0,0,
			toaplan2_interrupt,1
		},
#if Zx80
		{
			CPU_Z180,			/* Z?80 type Toaplan marked CPU ??? */
			10000000,			/* 10MHz Oscillator */
			Zx80_readmem,Zx80_writemem,Zx80_readport,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_toaplan3,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	toaplan2_0_vh_start,
	toaplan2_0_vh_stop,
	toaplan2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

static struct MachineDriver machine_driver_batsugun =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			32000000/2,			/* 16MHz , 32MHz Oscillator */
			batsugun_readmem,batsugun_writemem,0,0,
			toaplan2_interrupt,1
		},
#if Zx80
		{
			CPU_Z180,			/* Z?80 type Toaplan marked CPU ??? */
			32000000/2,			/* 16MHz , 32MHz Oscillator */
			Zx80_readmem,Zx80_writemem,Zx80_readport,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_toaplan3,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo_2,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_1_eof_callback,
	toaplan2_1_vh_start,
	toaplan2_1_vh_stop,
	batsugun_1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
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

static struct MachineDriver machine_driver_snowbro2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,
			snowbro2_readmem,snowbro2_writemem,0,0,
			toaplan2_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	init_toaplan2,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	toaplan2_0_vh_start,
	toaplan2_0_vh_stop,
	toaplan2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
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

static struct MachineDriver machine_driver_mahoudai =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			32000000/2,			/* 16MHz , 32MHz Oscillator */
			mahoudai_readmem,mahoudai_writemem,0,0,
			toaplan2_interrupt,1
		},
		{
			CPU_Z80,
			32000000/8,			/* 4MHz , 32MHz Oscillator */
			raizing_sound_readmem,raizing_sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	raizing_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	truxton2_0_vh_start,
	toaplan2_0_vh_stop,
	truxton2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&raizing_okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_shippumd =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			32000000/2,			/* 16MHz , 32MHz Oscillator */
			shippumd_readmem,shippumd_writemem,0,0,
			toaplan2_interrupt,1
		},
		{
			CPU_Z80,
			32000000/8,			/* 4MHz , 32MHz Oscillator */
			raizing_sound_readmem,raizing_sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	raizing_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	truxton2_0_vh_start,
	toaplan2_0_vh_stop,
	truxton2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&raizing_okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_battleg =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			32000000/2,			/* 16MHz , 32MHz Oscillator */
			battleg_readmem,battleg_writemem,0,0,
			toaplan2_interrupt,1
		},
		{
			CPU_Z80,
			32000000/8,			/* 4MHz , 32MHz Oscillator */
			battleg_sound_readmem,battleg_sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	120,
	init_battleg,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	raizing_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	toaplan2_0_eof_callback,
	truxton2_0_vh_start,
	toaplan2_0_vh_stop,
	truxton2_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&raizing_ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&battleg_okim6295_interface
		}
	}
};

static struct MachineDriver machine_driver_batrider =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			32000000/2,			/* 16MHz , 32MHz Oscillator */
			batrider_readmem,batrider_writemem,0,0,
			truxton2_interrupt,1
		},
		{
			CPU_Z80,
			32000000/8,			/* 4MHz , 32MHz Oscillator */
			batrider_sound_readmem,batrider_sound_writemem,batrider_sound_readport,batrider_sound_writeport,
			batrider_sound_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,
	0,

	/* video hardware */
	32*16, 32*16, { 0, 319, 0, 239 },
	batrider_gfxdecodeinfo,
	(64*16)+(64*16), (64*16)+(64*16),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK, /* Sprites are buffered too */
	batrider_0_eof_callback,
	batrider_0_vh_start,
	toaplan2_0_vh_stop,
	batrider_0_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&raizing_ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&batrider_okim6295_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

/* -------------------------- Toaplan games ------------------------- */
ROM_START( tekipaki )
	ROM_REGION( 0x020000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "tp020-1.bin", 0x000000, 0x010000, 0xd8420bd5 )
	ROM_LOAD16_BYTE( "tp020-2.bin", 0x000001, 0x010000, 0x7222de8e )

#if HD64x180
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound HD647180 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM - not yet supported */
	ROM_LOAD( "hd647180.020", 0x00000, 0x08000, 0x00000000 )
#endif

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp020-4.bin", 0x000000, 0x080000, 0x3ebbe41e )
	ROM_LOAD( "tp020-3.bin", 0x080000, 0x080000, 0x2d5e2201 )
ROM_END

ROM_START( ghox )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "tp021-01.u10", 0x000000, 0x020000, 0x9e56ac67 )
	ROM_LOAD16_BYTE( "tp021-02.u11", 0x000001, 0x020000, 0x15cac60f )

#if HD64x180
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound HD647180 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM - not yet supported */
	ROM_LOAD( "hd647180.021", 0x00000, 0x08000, 0x00000000 )
#endif

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp021-03.u36", 0x000000, 0x080000, 0xa15d8e9d )
	ROM_LOAD( "tp021-04.u37", 0x080000, 0x080000, 0x26ed1c9a )
ROM_END

ROM_START( dogyuun )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD( "tp022_1.r16", 0x000000, 0x080000, 0x72f18907 )

#if Zx80
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Secondary CPU code */
	/* Secondary CPU is a Toaplan marked chip ??? */
/*	ROM_LOAD( "tp022.mcu", 0x00000, 0x08000, 0x00000000 ) */
#endif

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "tp022_3.r16", 0x000000, 0x100000, 0x191b595f )
	ROM_LOAD16_WORD_SWAP( "tp022_4.r16", 0x100000, 0x100000, 0xd58d29ca )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "tp022_5.r16", 0x000000, 0x200000, 0xd4c1db45 )
	ROM_LOAD16_WORD_SWAP( "tp022_6.r16", 0x200000, 0x200000, 0xd48dc74f )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "tp022_2.rom", 0x00000, 0x40000, 0x043271b3 )
ROM_END

ROM_START( kbash )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "kbash01.bin", 0x000000, 0x080000, 0x2965f81d )

	/* Secondary CPU is a Toaplan marked chip, (TS-004-Dash  TOA PLAN) */
	/* Its a Z?80 of some sort - 94 pin chip. */
#if Zx80
	ROM_REGION( 0x88000, REGION_CPU2, 0 )			/* Sound Z?80 code */
	ROM_LOAD( "kbash02.bin", 0x80000, 0x08000, 0x4cd882a1 )
#else
	ROM_REGION( 0x08000, REGION_USER1, 0 )
	ROM_LOAD( "kbash02.bin", 0x00200, 0x07e00, 0x4cd882a1 )
	ROM_CONTINUE(			 0x00000, 0x00200 )
#endif

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "kbash03.bin", 0x000000, 0x200000, 0x32ad508b )
	ROM_LOAD( "kbash05.bin", 0x200000, 0x200000, 0xb84c90eb )
	ROM_LOAD( "kbash04.bin", 0x400000, 0x200000, 0xe493c077 )
	ROM_LOAD( "kbash06.bin", 0x600000, 0x200000, 0x9084b50a )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "kbash07.bin", 0x00000, 0x40000, 0x3732318f )
ROM_END

ROM_START( truxton2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD( "tp024_1.bin", 0x000000, 0x080000, 0xf5cfe6ee )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp024_4.bin", 0x000000, 0x100000, 0x805c449e )
	ROM_LOAD( "tp024_3.bin", 0x100000, 0x100000, 0x47587164 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp024_2.bin", 0x00000, 0x80000, 0xf2f6cae4 )
ROM_END

ROM_START( pipibibs )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "tp025-1.bin", 0x000000, 0x020000, 0xb2ea8659 )
	ROM_LOAD16_BYTE( "tp025-2.bin", 0x000001, 0x020000, 0xdc53b939 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "tp025-5.bin", 0x0000, 0x8000, 0xbf8ffde5 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp025-4.bin", 0x000000, 0x100000, 0xab97f744 )
	ROM_LOAD( "tp025-3.bin", 0x100000, 0x100000, 0x7b16101e )
ROM_END

ROM_START( whoopee )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "whoopee.1", 0x000000, 0x020000, 0x28882e7e )
	ROM_LOAD16_BYTE( "whoopee.2", 0x000001, 0x020000, 0x6796f133 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM - not yet supported */
	/* use the Z80 version from the bootleg Pipi & Bibis set for now */
	ROM_LOAD( "hd647180.025", 0x00000, 0x08000, BADCRC( 0x101c0358 ) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp025-4.bin", 0x000000, 0x100000, 0xab97f744 )
	ROM_LOAD( "tp025-3.bin", 0x100000, 0x100000, 0x7b16101e )
ROM_END

ROM_START( pipibibi )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "ppbb06.bin", 0x000000, 0x020000, 0x14c92515 )
	ROM_LOAD16_BYTE( "ppbb05.bin", 0x000001, 0x020000, 0x3d51133c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ppbb08.bin", 0x0000, 0x8000, 0x101c0358 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	/* GFX data differs slightly from Toaplan boards ??? */
	ROM_LOAD16_BYTE( "ppbb01.bin", 0x000000, 0x080000, 0x0fcae44b )
	ROM_LOAD16_BYTE( "ppbb02.bin", 0x000001, 0x080000, 0x8bfcdf87 )
	ROM_LOAD16_BYTE( "ppbb03.bin", 0x100000, 0x080000, 0xabdd2b8b )
	ROM_LOAD16_BYTE( "ppbb04.bin", 0x100001, 0x080000, 0x70faa734 )

	ROM_REGION( 0x8000, REGION_USER1, 0 )			/* ??? Some sort of table */
	ROM_LOAD( "ppbb07.bin", 0x0000, 0x8000, 0x456dd16e )
ROM_END

ROM_START( fixeight )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp-026-1", 0x000000, 0x080000, 0xf7b1746a )

#if Zx80
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Secondary CPU code */
	/* Secondary CPU is a Toaplan marked chip, (TS-001-Turbo  TOA PLAN) */
	/* Its a Z?80 of some sort - 94 pin chip. */
/*	ROM_LOAD( "tp-026.mcu", 0x0000, 0x8000, 0x00000000 ) */
#endif

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp-026-3", 0x000000, 0x200000, 0xe5578d98 )
	ROM_LOAD( "tp-026-4", 0x200000, 0x200000, 0xb760cb53 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp-026-2", 0x00000, 0x40000, 0x85063f1f )

	ROM_REGION( 0x80, REGION_USER1, 0 )
	/* Serial EEPROM (93C45) connected to Secondary CPU */
	ROM_LOAD( "93c45.u21", 0x00, 0x80, 0x40d75df0 )
ROM_END

ROM_START( grindstm )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "01.bin", 0x000000, 0x080000, 0x4923f790 )

#if Zx80
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its a Z?80 of some sort - 94 pin chip. */
/*	ROM_LOAD( "tp027.mcu", 0x8000, 0x8000, 0x00000000 ) */
#endif

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp027_02.bin", 0x000000, 0x100000, 0x877b45e8 )
	ROM_LOAD( "tp027_03.bin", 0x100000, 0x100000, 0xb1fc6362 )
ROM_END

ROM_START( vfive )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp027_01.bin", 0x000000, 0x080000, 0x731d50f4 )

#if Zx80
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its a Z?80 of some sort - 94 pin chip. */
/*	ROM_LOAD( "tp027.mcu", 0x8000, 0x8000, 0x00000000 ) */
#endif

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp027_02.bin", 0x000000, 0x100000, 0x877b45e8 )
	ROM_LOAD( "tp027_03.bin", 0x100000, 0x100000, 0xb1fc6362 )
ROM_END

ROM_START( batsugun )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp030_01.bin", 0x000000, 0x080000, 0x3873d7dd )

#if Zx80
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its a Z?80 of some sort - 94 pin chip. */
/*	ROM_LOAD( "tp030.mcu", 0x8000, 0x8000, 0x00000000 ) */
#endif

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_3l.bin", 0x000000, 0x100000, 0x3024b793 )
	ROM_LOAD( "tp030_3h.bin", 0x100000, 0x100000, 0xed75730b )
	ROM_LOAD( "tp030_4l.bin", 0x200000, 0x100000, 0xfedb9861 )
	ROM_LOAD( "tp030_4h.bin", 0x300000, 0x100000, 0xd482948b )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_5.bin",  0x000000, 0x100000, 0xbcf5ba05 )
	ROM_LOAD( "tp030_6.bin",  0x100000, 0x100000, 0x0666fecd )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp030_2.bin", 0x00000, 0x40000, 0x276146f5 )
ROM_END

ROM_START( batugnsp )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "tp030-sp.u69", 0x000000, 0x080000, 0x8072a0cd )

#if Zx80
	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound CPU code */
	/* Secondary CPU is a Toaplan marked chip, (TS-007-Spy  TOA PLAN) */
	/* Its a Z?80 of some sort - 94 pin chip. */
/*	ROM_LOAD( "tp030.mcu", 0x8000, 0x8000, 0x00000000 ) */
#endif

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_3l.bin", 0x000000, 0x100000, 0x3024b793 )
	ROM_LOAD( "tp030_3h.bin", 0x100000, 0x100000, 0xed75730b )
	ROM_LOAD( "tp030_4l.bin", 0x200000, 0x100000, 0xfedb9861 )
	ROM_LOAD( "tp030_4h.bin", 0x300000, 0x100000, 0xd482948b )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tp030_5.bin",  0x000000, 0x100000, 0xbcf5ba05 )
	ROM_LOAD( "tp030_6.bin",  0x100000, 0x100000, 0x0666fecd )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "tp030_2.bin", 0x00000, 0x40000, 0x276146f5 )
ROM_END

ROM_START( snowbro2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "pro-4", 0x000000, 0x080000, 0x4c7ee341 )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom2-l", 0x000000, 0x100000, 0xe9d366a9 )
	ROM_LOAD( "rom2-h", 0x100000, 0x080000, 0x9aab7a62 )
	ROM_LOAD( "rom3-l", 0x180000, 0x100000, 0xeb06e332 )
	ROM_LOAD( "rom3-h", 0x280000, 0x080000, 0xdf4a952a )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "rom4", 0x00000, 0x80000, 0x638f341e )
ROM_END

/* -------------------------- Raizing games ------------------------- */
ROM_START( mahoudai )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_WORD_SWAP( "ra_ma_01.01", 0x000000, 0x080000, 0x970ccc5c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ra_ma_01.02", 0x00000, 0x10000, 0xeabfa46d )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ra_ma_01.03",  0x000000, 0x100000, 0x54e2bd95 )
	ROM_LOAD( "ra_ma_01.04",  0x100000, 0x100000, 0x21cd378f )

	ROM_REGION( 0x008000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ra_ma_01.05",  0x000000, 0x008000, 0xc00d1e80 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "ra_ma_01.06", 0x00000, 0x40000, 0x6edb2ab8 )
ROM_END

ROM_START( shippumd )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "ma02rom1.bin", 0x000000, 0x080000, 0xa678b149 )
	ROM_LOAD16_BYTE( "ma02rom0.bin", 0x000001, 0x080000, 0xf226a212 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* Sound Z80 code */
	ROM_LOAD( "ma02rom2.bin", 0x00000, 0x10000, 0xdde8a57e )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ma02rom3.bin",  0x000000, 0x200000, 0x0e797142 )
	ROM_LOAD( "ma02rom4.bin",  0x200000, 0x200000, 0x72a6fa53 )

	ROM_REGION( 0x008000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ma02rom5.bin",  0x000000, 0x008000, 0x116ae559 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )			/* ADPCM Samples */
	ROM_LOAD( "ma02rom6.bin", 0x00000, 0x80000, 0x199e7cae )
ROM_END

ROM_START( battleg )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )			/* Main 68K code */
	ROM_LOAD16_BYTE( "prg0.bin", 0x000000, 0x080000, 0xf80c2fc2 )
	ROM_LOAD16_BYTE( "prg1.bin", 0x000001, 0x080000, 0x2ccfdd1e )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.bin", 0x00000, 0x08000, 0x68632952 )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom4.bin",  0x000000, 0x200000, 0xb333d81f )
	ROM_LOAD( "rom3.bin",  0x200000, 0x200000, 0x51b9ebfb )
	ROM_LOAD( "rom2.bin",  0x400000, 0x200000, 0xb330e5e2 )
	ROM_LOAD( "rom1.bin",  0x600000, 0x200000, 0x7eafdd70 )

	ROM_REGION( 0x010000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "text.bin", 0x000000, 0x010000, 0xb239cd05 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples */
	ROM_LOAD( "rom5.bin", 0x040000, 0x100000, 0xf6d49863 )
ROM_END

ROM_START( batrider )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0.u22" , 0x000000, 0x080000, 0x4f3fc729 )
	ROM_LOAD16_BYTE( "prg1b.u23", 0x000001, 0x080000, 0x8e70b492 )
	ROM_LOAD16_BYTE( "prg2.u21" , 0x100000, 0x080000, 0xbdaa5fbf )
	ROM_LOAD16_BYTE( "prg3.u24" , 0x100001, 0x080000, 0x7aa9f941 )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, 0x56682696 )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, 0x0df69ca2 )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, 0x60167d38 )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, 0x1bfea593 )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, 0xbee03c94 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, 0x4274daf6 )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, 0x2a1c2426 )
ROM_END

ROM_START( batridra )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )			/* Main 68k code */
	ROM_LOAD16_BYTE( "prg0.bin", 0x000000, 0x080000, 0xf93ea27c )
	ROM_LOAD16_BYTE( "prg1.bin", 0x000001, 0x080000, 0x8ae7f592 )
	ROM_LOAD16_BYTE( "prg2.u21", 0x100000, 0x080000, 0xbdaa5fbf )
	ROM_LOAD16_BYTE( "prg3.u24", 0x100001, 0x080000, 0x7aa9f941 )

	ROM_REGION( 0x48000, REGION_CPU2, 0 )			/* Sound Z80 code + bank */
	ROM_LOAD( "snd.u77", 0x00000, 0x08000, 0x56682696 )
	ROM_CONTINUE(        0x10000, 0x38000 )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom-1.bin", 0x000000, 0x400000, 0x0df69ca2 )
	ROM_LOAD( "rom-3.bin", 0x400000, 0x400000, 0x60167d38 )
	ROM_LOAD( "rom-2.bin", 0x800000, 0x400000, 0x1bfea593 )
	ROM_LOAD( "rom-4.bin", 0xc00000, 0x400000, 0xbee03c94 )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )		/* ADPCM Samples 1 */
	ROM_LOAD( "rom-5.bin", 0x040000, 0x100000, 0x4274daf6 )

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )		/* ADPCM Samples 2 */
	ROM_LOAD( "rom-6.bin", 0x040000, 0x100000, 0x2a1c2426 )
ROM_END


/* The following is in order of Toaplan Board/game numbers */
/* See list at top of file */
/* Whoopee machine to be changed to Teki Paki when (if) HD647180 is dumped */

/*   ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR       COMPANY    FULLNAME     FLAGS ) */
GAMEX( 1991, tekipaki, 0,        tekipaki, tekipaki, toaplan2, ROT0,         "Toaplan", "Teki Paki", GAME_NO_SOUND )
GAMEX( 1991, ghox,     0,        ghox,     ghox,     toaplan2, ROT270,       "Toaplan", "Ghox", GAME_NO_SOUND )
GAMEX( 1991, dogyuun,  0,        dogyuun,  dogyuun,  toaplan3, ROT270,       "Toaplan", "Dogyuun", GAME_NO_SOUND )
GAMEX( 1993, kbash,    0,        kbash,    kbash,    toaplan2, ROT0_16BIT,   "Toaplan", "Knuckle Bash", GAME_NO_SOUND )
GAME ( 1992, truxton2, 0,        truxton2, truxton2, truxton2, ROT270,       "Toaplan", "Truxton II / Tatsujin II / Tatsujin Oh (Japan)" )
GAME ( 1991, pipibibs, 0,        pipibibs, pipibibs, pipibibs, ROT0,         "Toaplan", "Pipi & Bibis / Whoopee (Japan)" )
GAME ( 1991, whoopee,  pipibibs, whoopee,  whoopee,  pipibibs, ROT0,         "Toaplan", "Whoopee (Japan) / Pipi & Bibis (World)" )
GAME ( 1991, pipibibi, pipibibs, pipibibi, pipibibi, pipibibi, ROT0,         "[Toaplan] Ryouta Kikaku", "Pipi & Bibis / Whoopee (Japan) [bootleg ?]" )
GAMEX( 1992, fixeight, 0,        fixeight, fixeight, fixeight, ROT270,       "Toaplan", "FixEight", GAME_NOT_WORKING )
GAMEX( 1992, grindstm, vfive,    vfive,    grindstm, toaplan3, ROT270,       "Toaplan", "Grind Stormer", GAME_NO_SOUND )
GAMEX( 1993, vfive,    0,        vfive,    vfive,    toaplan3, ROT270,       "Toaplan", "V-Five (Japan)", GAME_NO_SOUND )
GAMEX( 1993, batsugun, 0,        batsugun, batsugun, toaplan3, ROT270_16BIT, "Toaplan", "Batsugun", GAME_NO_SOUND )
GAMEX( 1993, batugnsp, batsugun, batsugun, batsugun, toaplan3, ROT270_16BIT, "Toaplan", "Batsugun Special Ver.", GAME_NO_SOUND )
GAME ( 1994, snowbro2, 0,        snowbro2, snowbro2, snowbro2, ROT0_16BIT,   "[Toaplan] Hanafram", "Snow Bros. 2 - With New Elves" )
GAME ( 1993, mahoudai, 0,        mahoudai, mahoudai, raizing,  ROT270_16BIT, "Raizing (Able license)", "Mahou Daisakusen (Japan)" )
GAME ( 1994, shippumd, 0,        shippumd, shippumd, raizing,  ROT270_16BIT, "Raizing/8ing", "Shippu Mahou Daisakusen (Japan)" )
GAME ( 1996, battleg,  0,        battleg,  battleg,  battleg,  ROT270_16BIT, "Raizing/8ing", "Battle Garegga" )
GAME ( 1998, batrider, 0,        batrider, batrider, battleg,  ROT270_16BIT, "Raizing/8ing", "Armed Police Batrider (Japan, version B)" )
GAME ( 1998, batridra, batrider, batrider, batrider, battleg,  ROT270_16BIT, "Raizing/8ing", "Armed Police Batrider (Japan, version A)" )
