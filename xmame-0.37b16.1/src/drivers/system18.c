/*	System18 Hardware
**
**	MC68000 + Z80
**	2xYM3438 + Custom PCM
**
**	Ace Attacker
**	Alien Storm
**	Bloxeed
**	Clutch Hitter
**	D.D. Crew
**	Laser Ghost
**	Michael Jackson's Moonwalker
**	Shadow Dancer
**	Search Wally
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "machine/system16.h"

/***************************************************************************/

static data16_t sys16_coinctrl;

static WRITE16_HANDLER( sys18_refreshenable_w ){
	if( ACCESSING_LSB ){
		sys16_coinctrl = data&0xff;
		sys16_refreshenable = sys16_coinctrl & 0x02;
		/* bit 2 is also used (0 in shadow dancer) */
		/* shadow dancer also sets bit 7 */
	}
}

/***************************************************************************/

static int sys18_interrupt( void ){
	return 4; /* Interrupt vector 4: VBlank */
}

/***************************************************************************/

static void set_fg_page( int data ){
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[2] = (data>>4)&0xf;
	sys16_fg_page[3] = data&0xf;
}

static void set_bg_page( int data ){
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[2] = (data>>4)&0xf;
	sys16_bg_page[3] = data&0xf;
}

static void set_fg2_page( int data ){
	sys16_fg2_page[0] = data>>12;
	sys16_fg2_page[1] = (data>>8)&0xf;
	sys16_fg2_page[2] = (data>>4)&0xf;
	sys16_fg2_page[3] = data&0xf;
}

static void set_bg2_page( int data ){
	sys16_bg2_page[0] = data>>12;
	sys16_bg2_page[1] = (data>>8)&0xf;
	sys16_bg2_page[2] = (data>>4)&0xf;
	sys16_bg2_page[3] = data&0xf;
}

/***************************************************************************/

static UINT8 *sys18_SoundMemBank;

static READ_HANDLER( system18_bank_r ){
	return sys18_SoundMemBank[offset];
}

static MEMORY_READ_START( sound_readmem_18 )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xbfff, system18_bank_r },
	/**** D/A register ****/
	{ 0xd000, 0xdfff, RF5C68_r },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_18 )
	{ 0x0000, 0xbfff, MWA_ROM },
	/**** D/A register ****/
	{ 0xc000, 0xc008, RF5C68_reg_w },
	{ 0xd000, 0xdfff, RF5C68_w },
	{ 0xe000, 0xffff, MWA_RAM },	/*?? */
MEMORY_END

static WRITE_HANDLER( sys18_soundbank_w ){
	/* select access bank for a000~bfff */
	unsigned char *RAM = memory_region(REGION_CPU2);
	int Bank=0;
	switch( data&0xc0 ){
		case 0x00:
			Bank = data<<13;
			break;
		case 0x40:
			Bank = ((data&0x1f) + 128/8)<<13;
			break;
		case 0x80:
			Bank = ((data&0x1f) + (256+128)/8)<<13;
			break;
		case 0xc0:
			Bank = ((data&0x1f) + (512+128)/8)<<13;
			break;
	}
	sys18_SoundMemBank = &RAM[Bank+0x10000];
}

static PORT_READ_START( sound_readport_18 )
	{ 0x80, 0x80, YM2612_status_port_0_A_r },
/*	{ 0x82, 0x82, YM2612_status_port_0_B_r }, */
/*	{ 0x90, 0x90, YM2612_status_port_1_A_r }, */
/*	{ 0x92, 0x92, YM2612_status_port_1_B_r }, */
	{ 0xc0, 0xc0, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport_18 )
	{ 0x80, 0x80, YM2612_control_port_0_A_w },
	{ 0x81, 0x81, YM2612_data_port_0_A_w },
	{ 0x82, 0x82, YM2612_control_port_0_B_w },
	{ 0x83, 0x83, YM2612_data_port_0_B_w },
	{ 0x90, 0x90, YM2612_control_port_1_A_w },
	{ 0x91, 0x91, YM2612_data_port_1_A_w },
	{ 0x92, 0x92, YM2612_control_port_1_B_w },
	{ 0x93, 0x93, YM2612_data_port_1_B_w },
	{ 0xa0, 0xa0, sys18_soundbank_w },
PORT_END

static WRITE16_HANDLER( sound_command_nmi_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}

/***************************************************************************/

static READ16_HANDLER( shdancer_skip_r ){
	if (cpu_get_pc()==0x2f76) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0];
}

static MEMORY_READ16_START( shdancer_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc00000, 0xc00007, SYS16_MRA16_EXTRAM },
	{ 0xe4000a, 0xe4000b, input_port_3_word_r }, /* dip1 */
	{ 0xe4000c, 0xe4000d, input_port_4_word_r }, /* dip2 */
	{ 0xe40000, 0xe40001, input_port_0_word_r }, /* player1 */
	{ 0xe40002, 0xe40003, input_port_1_word_r }, /* player2 */
	{ 0xe40008, 0xe40009, input_port_2_word_r }, /* service */
	{ 0xe43034, 0xe43035, MRA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( shdancer_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM },
	{ 0xc00000, 0xc00007, SYS16_MWA16_EXTRAM },
	{ 0xe4001c, 0xe4001d, sys18_refreshenable_w },
	{ 0xe43034, 0xe43035, MWA16_NOP },
	{ 0xfe0006, 0xfe0007, sound_command_nmi_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM },
MEMORY_END

/***************************************************************************/

static void shdancer_update_proc( void ){
	sys16_fg_scrolly = sys16_textram[0x0e90/2];
	sys16_bg_scrolly = sys16_textram[0x0e92/2];
	sys16_fg_scrollx = sys16_textram[0x0e98/2];
	sys16_bg_scrollx = sys16_textram[0x0e9a/2];/*?*/

	set_fg_page( sys16_textram[0x0e80/2] );
	set_bg_page( sys16_textram[0x0e82/2] );

	sys16_fg2_scrollx = sys16_textram[0x0e9c/2];
	sys16_bg2_scrollx = sys16_textram[0x0e9e/2];
	sys16_fg2_scrolly = sys16_textram[0x0e94/2];
	sys16_bg2_scrolly = sys16_textram[0x0e96/2];

	set_fg2_page( sys16_textram[0x0e84/2] );
	set_bg2_page( sys16_textram[0x0e86/2] );

	sys18_bg2_active=0;
	sys18_fg2_active=0;

	if(sys16_fg2_scrollx | sys16_fg2_scrolly | sys16_textram[0x0e84/2]) sys18_fg2_active=1;
	if(sys16_bg2_scrollx | sys16_bg2_scrolly | sys16_textram[0x0e86/2]) sys18_bg2_active=1;

	{
		data16_t data = sys16_extraram[0/2];
		sys16_tile_bank0 = data&0xf;
		sys16_tile_bank1 = (data>>4)&0xf;
	}
}

static void shdancer_init_machine( void ){
	sys16_spritelist_end=0x8000;
	sys16_update_proc = shdancer_update_proc;
}

static void init_shdancer( void ){
	unsigned char *RAM = memory_region(REGION_CPU2);
	sys16_onetime_init_machine();
	sys18_splittab_fg_x=&sys16_textram[0x0f80/2];
	sys18_splittab_bg_x=&sys16_textram[0x0fc0/2];
/*	install_mem_read_handler(0, 0xffc000, 0xffc001, shdancer_skip_r ); */
	sys16_MaxShadowColors=0; /* doesn't seem to use transparent shadows */

	memcpy(RAM,&RAM[0x10000],0xa000);
}

/***************************************************************************/

/*
static READ_HANDLER( shdancer_skip_r ){
	if (cpu_get_pc()==0x2f76) {cpu_spinuntil_int(); return 0xffff;}
	return READ_WORD(&sys16_workingram[0x0000]);
}
*/

static MEMORY_READ16_START( shdancbl_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc00000, 0xc00007, SYS16_MRA16_EXTRAM },
	{ 0xc40000, 0xc40001, input_port_3_word_r }, /* dip1 */
	{ 0xc40002, 0xc40003, input_port_4_word_r }, /* dip2 */
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41004, 0xc41005, input_port_1_word_r }, /* player2 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
/*	{ 0xc40000, 0xc4ffff, MRA16_EXTRAM3 }, */
	{ 0xe43034, 0xe43035, MRA16_NOP },
/*	{ 0xffc000, 0xffc001, shdancer_skip_r }, */
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( shdancbl_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM },
	{ 0xc00000, 0xc00007, SYS16_MWA16_EXTRAM },
/*	{ 0xc40000, 0xc4ffff, SYS16_MWA16_EXTRAM3 }, */
	{ 0xe4001c, 0xe4001d, sys18_refreshenable_w },
	{ 0xe43034, 0xe43035, MWA16_NOP },
	{ 0xfe0006, 0xfe0007, sound_command_nmi_w },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM },
MEMORY_END

/***************************************************************************/

static void shdancbl_update_proc( void ){
	/* this is all wrong and needs re-doing */
	sys16_fg_scrollx = sys16_textram[0x0e98/2];
	sys16_bg_scrollx = sys16_textram[0x0e9a/2];
	sys16_fg_scrolly = sys16_textram[0x0e90/2];
	sys16_bg_scrolly = sys16_textram[0x0e92/2];

	set_fg_page( sys16_textram[0x0e80/2] );
	set_bg_page( sys16_textram[0x0e82/2] );

	sys16_fg2_scrollx = sys16_textram[0x0e9c/2];
	sys16_bg2_scrollx = sys16_textram[0x0e9e/2];
	sys16_fg2_scrolly = sys16_textram[0x0e94/2];
	sys16_bg2_scrolly = sys16_textram[0x0e96/2];

	set_fg2_page( sys16_textram[0x0e84/2] );
	set_bg2_page( sys16_textram[0x0e82/2] );

	sys18_bg2_active=0;
	sys18_fg2_active=0;

	if(sys16_fg2_scrollx | sys16_fg2_scrolly | sys16_textram[0x0e84/2])
		sys18_fg2_active=1;
	if(sys16_bg2_scrollx | sys16_bg2_scrolly | sys16_textram[0x0e86/2])
		sys18_bg2_active=1;

	{
		data16_t data = sys16_extraram[0/2];
		sys16_tile_bank0 = data&0xf;
		sys16_tile_bank1 = (data>>4)&0xf;
	}
}


static void shdancbl_init_machine( void ){
	sys16_spritelist_end=0x8000;
	sys16_sprxoffset = -0xbc+0x77;

	sys16_update_proc = shdancbl_update_proc;
}

static void init_shdancbl( void ){
	unsigned char *RAM= memory_region(REGION_CPU2);
	int i;

	sys16_onetime_init_machine();
	sys18_splittab_fg_x=&sys16_textram[0x0f80/2];
	sys18_splittab_bg_x=&sys16_textram[0x0fc0/2];
/*	install_mem_read_handler(0, 0xffc000, 0xffc001, shdancer_skip_r ); */
	sys16_MaxShadowColors=0;		/* doesn't seem to use transparent shadows */

	memcpy(RAM,&RAM[0x10000],0xa000);

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0xc0000; i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;
}

/***************************************************************************/
static READ16_HANDLER( shdancrj_skip_r ){
	if (cpu_get_pc()==0x2f70) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0xc000/2];
}

static void shdancrj_init_machine( void ){
	sys16_spritelist_end=0x8000;
	sys16_patch_code(0x6821, 0xdf);
	sys16_update_proc = shdancer_update_proc;
}

static void init_shdancrj( void ){
	unsigned char *RAM= memory_region(REGION_CPU2);
	sys16_onetime_init_machine();
	sys18_splittab_fg_x=&sys16_textram[0x0f80/2];
	sys18_splittab_bg_x=&sys16_textram[0x0fc0/2];
/*	install_mem_read_handler(0, 0xffc000, 0xffc001, shdancrj_skip_r ); */

	memcpy(RAM,&RAM[0x10000],0xa000);
}

/***************************************************************************/

static READ16_HANDLER( moonwlkb_skip_r ){
	if (cpu_get_pc()==0x308a) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x202c/2];
}

static MEMORY_READ16_START( moonwalk_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MRA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc00000, 0xc0ffff, SYS16_MRA16_EXTRAM },
	{ 0xc40000, 0xc40001, input_port_3_word_r }, /* dip1 */
	{ 0xc40002, 0xc40003, input_port_4_word_r }, /* dip2 */
	{ 0xc41002, 0xc41003, input_port_0_word_r }, /* player1 */
	{ 0xc41004, 0xc41005, input_port_1_word_r }, /* player2 */
	{ 0xc41006, 0xc41007, input_port_5_word_r }, /* player3 */
	{ 0xc41000, 0xc41001, input_port_2_word_r }, /* service */
	{ 0xe40000, 0xe4ffff, SYS16_MRA16_EXTRAM2 },
	{ 0xfe0000, 0xfeffff, SYS16_MRA16_EXTRAM4 },
	{ 0xffe02c, 0xffe02d, moonwlkb_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( moonwalk_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM },
	{ 0x840000, 0x840fff, SYS16_MWA16_PALETTERAM },
	{ 0xc00000, 0xc0ffff, SYS16_MWA16_EXTRAM },
	{ 0xc40006, 0xc40007, sound_command_nmi_w },
	{ 0xc46600, 0xc46601, sys18_refreshenable_w },
	{ 0xc46800, 0xc46801, SYS16_MWA16_EXTRAM3 },
	{ 0xe40000, 0xe4ffff, SYS16_MWA16_EXTRAM2 },
	{ 0xfe0000, 0xfeffff, SYS16_MWA16_EXTRAM4 },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM },
MEMORY_END

/***************************************************************************/

static void moonwalk_update_proc( void ){
	sys16_fg_scrollx = sys16_textram[0x0e98/2];
	sys16_bg_scrollx = sys16_textram[0x0e9a/2];
	sys16_fg_scrolly = sys16_textram[0x0e90/2];
	sys16_bg_scrolly = sys16_textram[0x0e92/2];

	set_fg_page( sys16_textram[0x0e80/2] );
	set_bg_page( sys16_textram[0x0e82/2] );

	sys16_fg2_scrollx = sys16_textram[0x0e9c/2];
	sys16_bg2_scrollx = sys16_textram[0x0e9e/2];
	sys16_fg2_scrolly = sys16_textram[0x0e94/2];
	sys16_bg2_scrolly = sys16_textram[0x0e96/2];

	set_fg2_page( sys16_textram[0x0e84/2] );
	set_bg2_page( sys16_textram[0x0e86/2] );

	if(sys16_fg2_scrollx | sys16_fg2_scrolly | sys16_textram[0x0e84/2])
		sys18_fg2_active=1;
	else
		sys18_fg2_active=0;
	if(sys16_bg2_scrollx | sys16_bg2_scrolly | sys16_textram[0x0e86/2])
		sys18_bg2_active=1;
	else
		sys18_bg2_active=0;

	{
		data16_t data = sys16_extraram3[0/2];
		sys16_tile_bank0 = data&0xf;
		sys16_tile_bank1 = (data>>4)&0xf;
	}
}

static void moonwalk_init_machine( void ){
	sys16_bg_priority_value=0x1000;
	sys16_sprxoffset = -0x238;
	sys16_spritelist_end=0x8000;

	sys16_patch_code( 0x70116, 0x4e);
	sys16_patch_code( 0x70117, 0x71);

	sys16_patch_code( 0x314a, 0x46);
	sys16_patch_code( 0x314b, 0x42);

	sys16_patch_code( 0x311b, 0x3f);

	sys16_patch_code( 0x70103, 0x00);
	sys16_patch_code( 0x70109, 0x00);
	sys16_patch_code( 0x07727, 0x00);
	sys16_patch_code( 0x07729, 0x00);
	sys16_patch_code( 0x0780d, 0x00);
	sys16_patch_code( 0x0780f, 0x00);
	sys16_patch_code( 0x07861, 0x00);
	sys16_patch_code( 0x07863, 0x00);
	sys16_patch_code( 0x07d47, 0x00);
	sys16_patch_code( 0x07863, 0x00);
	sys16_patch_code( 0x08533, 0x00);
	sys16_patch_code( 0x08535, 0x00);
	sys16_patch_code( 0x085bd, 0x00);
	sys16_patch_code( 0x085bf, 0x00);
	sys16_patch_code( 0x09a4b, 0x00);
	sys16_patch_code( 0x09a4d, 0x00);
	sys16_patch_code( 0x09b2f, 0x00);
	sys16_patch_code( 0x09b31, 0x00);
	sys16_patch_code( 0x0a05b, 0x00);
	sys16_patch_code( 0x0a05d, 0x00);
	sys16_patch_code( 0x0a23f, 0x00);
	sys16_patch_code( 0x0a241, 0x00);
	sys16_patch_code( 0x10159, 0x00);
	sys16_patch_code( 0x1015b, 0x00);
	sys16_patch_code( 0x109fb, 0x00);
	sys16_patch_code( 0x109fd, 0x00);

	/* * SEGA mark */
	sys16_patch_code( 0x70212, 0x4e);
	sys16_patch_code( 0x70213, 0x71);

	sys16_update_proc = moonwalk_update_proc;
}

static void init_moonwalk( void ){
	unsigned char *RAM= memory_region(REGION_CPU2);
	sys16_onetime_init_machine();
	sys18_splittab_fg_x=&sys16_textram[0x0f80/2];
	sys18_splittab_bg_x=&sys16_textram[0x0fc0/2];

	memcpy(RAM,&RAM[0x10000],0xa000);
}

/***************************************************************************/

static READ16_HANDLER( astorm_skip_r ){
	if (cpu_get_pc()==0x3d4c) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_workingram[0x2c2c/2];
}

static MEMORY_READ16_START( astorm_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, SYS16_MRA16_TILERAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_TEXTRAM },
	{ 0x140000, 0x140fff, SYS16_MRA16_PALETTERAM },
	{ 0x200000, 0x200fff, SYS16_MRA16_SPRITERAM },
	{ 0xa00000, 0xa00001, input_port_3_word_r }, /* dip1 */
	{ 0xa00002, 0xa00003, input_port_4_word_r }, /* dip2 */
	{ 0xa01002, 0xa01003, input_port_0_word_r }, /* player1 */
	{ 0xa01004, 0xa01005, input_port_1_word_r }, /* player2 */
	{ 0xa01006, 0xa01007, input_port_5_word_r }, /* player3 */
	{ 0xa01000, 0xa01001, input_port_2_word_r }, /* service */
	{ 0xa00000, 0xa0ffff, SYS16_MRA16_EXTRAM2 },
	{ 0xc00000, 0xc0ffff, SYS16_MRA16_EXTRAM },
	{ 0xffec2c, 0xffec2d, astorm_skip_r },
	{ 0xffc000, 0xffffff, SYS16_MRA16_WORKINGRAM },
MEMORY_END

static MEMORY_WRITE16_START( astorm_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, SYS16_MWA16_TILERAM },
	{ 0x110000, 0x110fff, SYS16_MWA16_TEXTRAM },
	{ 0x140000, 0x140fff, SYS16_MWA16_PALETTERAM },
	{ 0x200000, 0x200fff, SYS16_MWA16_SPRITERAM },
	{ 0xa00006, 0xa00007, sound_command_nmi_w },
	{ 0xa00000, 0xa0ffff, SYS16_MWA16_EXTRAM2 },
	{ 0xc00000, 0xc0ffff, SYS16_MWA16_EXTRAM },
	{ 0xc46600, 0xc46601, sys18_refreshenable_w },
	{ 0xfe0020, 0xfe003f, MWA16_NOP },
	{ 0xffc000, 0xffffff, SYS16_MWA16_WORKINGRAM },
MEMORY_END

/***************************************************************************/

static void astorm_update_proc( void ){
	data16_t data;
	sys16_fg_scrollx = sys16_textram[0x0e98/2];
	sys16_bg_scrollx = sys16_textram[0x0e9a/2];
	sys16_fg_scrolly = sys16_textram[0x0e90/2];
	sys16_bg_scrolly = sys16_textram[0x0e92/2];

	data = sys16_textram[0x0e80/2];
	sys16_fg_page[1] = data>>12;
	sys16_fg_page[3] = (data>>8)&0xf;
	sys16_fg_page[0] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;

	data = sys16_textram[0x0e82/2];
	sys16_bg_page[1] = data>>12;
	sys16_bg_page[3] = (data>>8)&0xf;
	sys16_bg_page[0] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;

	sys16_fg2_scrollx = sys16_textram[0x0e9c/2];
	sys16_bg2_scrollx = sys16_textram[0x0e9e/2];
	sys16_fg2_scrolly = sys16_textram[0x0e94/2];
	sys16_bg2_scrolly = sys16_textram[0x0e96/2];

	data = sys16_textram[0x0e84/2];
	sys16_fg2_page[1] = data>>12;
	sys16_fg2_page[3] = (data>>8)&0xf;
	sys16_fg2_page[0] = (data>>4)&0xf;
	sys16_fg2_page[2] = data&0xf;

	data = sys16_textram[0x0e86/2];
	sys16_bg2_page[1] = data>>12;
	sys16_bg2_page[3] = (data>>8)&0xf;
	sys16_bg2_page[0] = (data>>4)&0xf;
	sys16_bg2_page[2] = data&0xf;

/* enable regs */
	if(sys16_fg2_scrollx | sys16_fg2_scrolly | sys16_textram[0x0e84/2])
		sys18_fg2_active=1;
	else
		sys18_fg2_active=0;
	if(sys16_bg2_scrollx | sys16_bg2_scrolly | sys16_textram[0x0e86/2])
		sys18_bg2_active=1;
	else
		sys18_bg2_active=0;

	{
		data = sys16_extraram2[0xe/2]; /* 0xa0000f */
		sys16_tile_bank0 = data&0xf;
		sys16_tile_bank1 = (data>>4)&0xf;
	}
}

static void astorm_init_machine( void ){
	sys16_fgxoffset = sys16_bgxoffset = -9;

	sys16_patch_code( 0x2D6E, 0x32 );
	sys16_patch_code( 0x2D6F, 0x3c );
	sys16_patch_code( 0x2D70, 0x80 );
	sys16_patch_code( 0x2D71, 0x00 );
	sys16_patch_code( 0x2D72, 0x33 );
	sys16_patch_code( 0x2D73, 0xc1 );
	sys16_patch_code( 0x2ea2, 0x30 );
	sys16_patch_code( 0x2ea3, 0x38 );
	sys16_patch_code( 0x2ea4, 0xec );
	sys16_patch_code( 0x2ea5, 0xf6 );
	sys16_patch_code( 0x2ea6, 0x30 );
	sys16_patch_code( 0x2ea7, 0x80 );
	sys16_patch_code( 0x2e5c, 0x30 );
	sys16_patch_code( 0x2e5d, 0x38 );
	sys16_patch_code( 0x2e5e, 0xec );
	sys16_patch_code( 0x2e5f, 0xe2 );
	sys16_patch_code( 0x2e60, 0xc0 );
	sys16_patch_code( 0x2e61, 0x7c );

	sys16_patch_code( 0x4cd8, 0x02 );
	sys16_patch_code( 0x4cec, 0x03 );
	sys16_patch_code( 0x2dc6c, 0xe9 );
	sys16_patch_code( 0x2dc64, 0x10 );
	sys16_patch_code( 0x2dc65, 0x10 );
	sys16_patch_code( 0x3a100, 0x10 );
	sys16_patch_code( 0x3a101, 0x13 );
	sys16_patch_code( 0x3a102, 0x90 );
	sys16_patch_code( 0x3a103, 0x2b );
	sys16_patch_code( 0x3a104, 0x00 );
	sys16_patch_code( 0x3a105, 0x01 );
	sys16_patch_code( 0x3a106, 0x0c );
	sys16_patch_code( 0x3a107, 0x00 );
	sys16_patch_code( 0x3a108, 0x00 );
	sys16_patch_code( 0x3a109, 0x01 );
	sys16_patch_code( 0x3a10a, 0x66 );
	sys16_patch_code( 0x3a10b, 0x06 );
	sys16_patch_code( 0x3a10c, 0x42 );
	sys16_patch_code( 0x3a10d, 0x40 );
	sys16_patch_code( 0x3a10e, 0x54 );
	sys16_patch_code( 0x3a10f, 0x8b );
	sys16_patch_code( 0x3a110, 0x60 );
	sys16_patch_code( 0x3a111, 0x02 );
	sys16_patch_code( 0x3a112, 0x30 );
	sys16_patch_code( 0x3a113, 0x1b );
	sys16_patch_code( 0x3a114, 0x34 );
	sys16_patch_code( 0x3a115, 0xc0 );
	sys16_patch_code( 0x3a116, 0x34 );
	sys16_patch_code( 0x3a117, 0xdb );
	sys16_patch_code( 0x3a118, 0x24 );
	sys16_patch_code( 0x3a119, 0xdb );
	sys16_patch_code( 0x3a11a, 0x24 );
	sys16_patch_code( 0x3a11b, 0xdb );
	sys16_patch_code( 0x3a11c, 0x4e );
	sys16_patch_code( 0x3a11d, 0x75 );
	sys16_patch_code( 0xaf8e, 0x66 );

	/* fix missing credit text */
	sys16_patch_code( 0x3f9a, 0xec );
	sys16_patch_code( 0x3f9b, 0x36 );

	sys16_update_proc = astorm_update_proc;
}

static void init_astorm( void ){
	unsigned char *RAM= memory_region(REGION_CPU2);
	sys16_onetime_init_machine();
	sys18_splittab_fg_x=&sys16_textram[0x0f80/2];
	sys18_splittab_bg_x=&sys16_textram[0x0fc0/2];

	memcpy(RAM,&RAM[0x10000],0xa000);
	sys16_MaxShadowColors = 0; /* doesn't seem to use transparent shadows */
}

/*****************************************************************************/

#define MACHINE_DRIVER_18( GAMENAME,READMEM,WRITEMEM,INITMACHINE) \
static struct MachineDriver GAMENAME = \
{ \
	{ \
		{ \
			CPU_M68000, \
			10000000, \
			READMEM,WRITEMEM,0,0, \
			sys18_interrupt,1 \
		}, \
		{ \
			CPU_Z80 | CPU_AUDIO_CPU, \
			4096000*2, /* overclocked to fix sound, but wrong! */ \
			sound_readmem_18,sound_writemem_18,sound_readport_18,sound_writeport_18, \
			ignore_interrupt,1 \
		}, \
	}, \
	60, DEFAULT_60HZ_VBLANK_DURATION, \
	1, \
	INITMACHINE, \
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 }, \
	sys16_gfxdecodeinfo, \
	2048*ShadowColorsMultiplier,2048*ShadowColorsMultiplier, \
	0, \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE, \
	0, \
	sys18_vh_start, \
	sys16_vh_stop, \
	sys18_vh_screenrefresh, \
	SOUND_SUPPORTS_STEREO,0,0,0, \
	{ \
		{ \
			SOUND_YM3438, \
			&sys18_ym3438_interface \
		}, \
		{ \
			SOUND_RF5C68, \
			&sys18_rf5c68_interface, \
		} \
	} \
};

MACHINE_DRIVER_18( machine_driver_astorm, \
	astorm_readmem,astorm_writemem,astorm_init_machine )

MACHINE_DRIVER_18( machine_driver_moonwalk, \
	moonwalk_readmem,moonwalk_writemem,moonwalk_init_machine )

MACHINE_DRIVER_18( machine_driver_shdancer, \
	shdancer_readmem,shdancer_writemem,shdancer_init_machine )

MACHINE_DRIVER_18( machine_driver_shdancbl, \
	shdancbl_readmem,shdancbl_writemem,shdancbl_init_machine )

MACHINE_DRIVER_18( machine_driver_shdancrj, \
	shdancer_readmem,shdancer_writemem,shdancrj_init_machine )

/***************************************************************************/

INPUT_PORTS_START( astorm )
	PORT_START /* player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_START /* player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
	SYS16_COINAGE
	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "2 Credits to Start" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easiest" )
	PORT_DIPSETTING(    0x08, "Easier" )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x1c, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x14, "Harder" )
	PORT_DIPSETTING(    0x18, "Hardest" )
	PORT_DIPSETTING(    0x00, "Special" )
	PORT_DIPNAME( 0x20, 0x20, "Coin Chutes" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_START /* player 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
INPUT_PORTS_END

INPUT_PORTS_START( moonwalk )
	PORT_START /* player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_START /* player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_START /* service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_TILT, "Test", KEYCODE_T, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	SYS16_COINAGE
	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "2 Credits to Start" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x08, 0x08, "Player Vitality" )
	PORT_DIPSETTING(    0x08, "Low" )
	PORT_DIPSETTING(    0x00, "High" )
	PORT_DIPNAME( 0x10, 0x00, "Play Mode" )
	PORT_DIPSETTING(    0x10, "2 Players" )
	PORT_DIPSETTING(    0x00, "3 Players" )
	PORT_DIPNAME( 0x20, 0x20, "Coin Mode" )
	PORT_DIPSETTING(    0x20, "Common" )
	PORT_DIPSETTING(    0x00, "Individual" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_START /* player 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
/*	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
INPUT_PORTS_END

INPUT_PORTS_START( shdancer )
	PORT_START /* player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_START /* player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	SYS16_SERVICE
	SYS16_COINAGE
	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "2 Credits to Start" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, "Time Adjust" )
	PORT_DIPSETTING(    0x00, "2.20" )
	PORT_DIPSETTING(    0x40, "2.40" )
	PORT_DIPSETTING(    0xc0, "3.00" )
	PORT_DIPSETTING(    0x80, "3.30" )
INPUT_PORTS_END

/*****************************************************************************/

/* Ace Attacker */
ROM_START( aceattac )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "11491.4a", 0x000000, 0x10000, 0x77b820f1 )
	ROM_LOAD16_BYTE( "11489.1a", 0x000001, 0x10000, 0xbbe623c5 )
	ROM_LOAD16_BYTE( "11492.5a", 0x020000, 0x10000, 0xd8bd3139 )
	ROM_LOAD16_BYTE( "11490.2a", 0x020001, 0x10000, 0x38cb3a41 )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "11493.9b",  0x00000, 0x10000, 0x654485d9 )
	ROM_LOAD( "11494.10b", 0x10000, 0x10000, 0xb67971ab )
	ROM_LOAD( "11495.11b", 0x20000, 0x10000, 0xb687ab61 )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "11501.1b", 0x00001, 0x10000, 0x09179ead )
	ROM_LOAD16_BYTE( "11502.2b", 0x00000, 0x10000, 0xa3ee36b8 )
	ROM_LOAD16_BYTE( "11503.3b", 0x20001, 0x10000, 0x344c0692 )
	ROM_LOAD16_BYTE( "11504.4b", 0x20000, 0x10000, 0x7cae7920 )
	ROM_LOAD16_BYTE( "11505.5b", 0x40001, 0x10000, 0xb67f1ecf )
	ROM_LOAD16_BYTE( "11506.6b", 0x40000, 0x10000, 0xb0104def )
	ROM_LOAD16_BYTE( "11507.7b", 0x60001, 0x10000, 0xa2af710a )
	ROM_LOAD16_BYTE( "11508.8b", 0x60000, 0x10000, 0x5cbb833c )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "11496.7a",	 0x00000, 0x08000, 0x82cb40a9 )
	ROM_LOAD( "11497.8a",    0x10000, 0x08000, 0xb04f62cc )
	ROM_LOAD( "11498.9a",    0x18000, 0x08000, 0x97baf52b )
	ROM_LOAD( "11499.10a",   0x20000, 0x08000, 0xea332866 )
	ROM_LOAD( "11500.11a",   0x28000, 0x08000, 0x2ddf1c31 )
ROM_END

/* Alien Storm */
ROM_START( astorm )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13085.bin", 0x000000, 0x40000, 0x15f74e2d )
	ROM_LOAD16_BYTE( "epr13084.bin", 0x000001, 0x40000, 0x9687b38f )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr13073.bin", 0x00000, 0x40000, 0xdf5d0a61 )
	ROM_LOAD( "epr13074.bin", 0x40000, 0x40000, 0x787afab8 )
	ROM_LOAD( "epr13075.bin", 0x80000, 0x40000, 0x4e01b477 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13082.bin", 0x000001, 0x40000, 0xa782b704 )
	ROM_LOAD16_BYTE( "mpr13089.bin", 0x000000, 0x40000, 0x2a4227f0 )
	ROM_LOAD16_BYTE( "mpr13081.bin", 0x080001, 0x40000, 0xeb510228 )
	ROM_LOAD16_BYTE( "mpr13088.bin", 0x080000, 0x40000, 0x3b6b4c55 )
	ROM_LOAD16_BYTE( "mpr13080.bin", 0x100001, 0x40000, 0xe668eefb )
	ROM_LOAD16_BYTE( "mpr13087.bin", 0x100000, 0x40000, 0x2293427d )
	ROM_LOAD16_BYTE( "epr13079.bin", 0x180001, 0x40000, 0xde9221ed )
	ROM_LOAD16_BYTE( "epr13086.bin", 0x180000, 0x40000, 0x8c9a71c4 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr13083.bin", 0x10000, 0x20000, 0x5df3af20 )
	ROM_LOAD( "epr13076.bin", 0x30000, 0x40000, 0x94e6c76e )
	ROM_LOAD( "epr13077.bin", 0x70000, 0x40000, 0xe2ec0d8d )
	ROM_LOAD( "epr13078.bin", 0xb0000, 0x40000, 0x15684dc5 )
ROM_END

ROM_START( astorm2p )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13182.bin", 0x000000, 0x40000, 0xe31f2a1c )
	ROM_LOAD16_BYTE( "epr13181.bin", 0x000001, 0x40000, 0x78cd3b26 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr13073.bin", 0x00000, 0x40000, 0xdf5d0a61 )
	ROM_LOAD( "epr13074.bin", 0x40000, 0x40000, 0x787afab8 )
	ROM_LOAD( "epr13075.bin", 0x80000, 0x40000, 0x4e01b477 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13082.bin", 0x000001, 0x40000, 0xa782b704 )
	ROM_LOAD16_BYTE( "mpr13089.bin", 0x000000, 0x40000, 0x2a4227f0 )
	ROM_LOAD16_BYTE( "mpr13081.bin", 0x080001, 0x40000, 0xeb510228 )
	ROM_LOAD16_BYTE( "mpr13088.bin", 0x080000, 0x40000, 0x3b6b4c55 )
	ROM_LOAD16_BYTE( "mpr13080.bin", 0x100001, 0x40000, 0xe668eefb )
	ROM_LOAD16_BYTE( "mpr13087.bin", 0x100000, 0x40000, 0x2293427d )
	ROM_LOAD16_BYTE( "epr13079.bin", 0x180001, 0x40000, 0xde9221ed )
	ROM_LOAD16_BYTE( "epr13086.bin", 0x180000, 0x40000, 0x8c9a71c4 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ep13083a.bin", 0x10000, 0x20000, 0xe7528e06 )
	ROM_LOAD( "epr13076.bin", 0x30000, 0x40000, 0x94e6c76e )
	ROM_LOAD( "epr13077.bin", 0x70000, 0x40000, 0xe2ec0d8d )
	ROM_LOAD( "epr13078.bin", 0xb0000, 0x40000, 0x15684dc5 )
ROM_END

ROM_START( astormbl )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "astorm.a6", 0x000000, 0x40000, 0x7682ed3e )
	ROM_LOAD16_BYTE( "astorm.a5", 0x000001, 0x40000, 0xefe9711e )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr13073.bin", 0x00000, 0x40000, 0xdf5d0a61 )
	ROM_LOAD( "epr13074.bin", 0x40000, 0x40000, 0x787afab8 )
	ROM_LOAD( "epr13075.bin", 0x80000, 0x40000, 0x4e01b477 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13082.bin", 0x000001, 0x40000, 0xa782b704 )
	ROM_LOAD16_BYTE( "astorm.a11",   0x000000, 0x40000, 0x7829c4f3 )
	ROM_LOAD16_BYTE( "mpr13081.bin", 0x080001, 0x40000, 0xeb510228 )
	ROM_LOAD16_BYTE( "mpr13088.bin", 0x080000, 0x40000, 0x3b6b4c55 )
	ROM_LOAD16_BYTE( "mpr13080.bin", 0x100001, 0x40000, 0xe668eefb )
	ROM_LOAD16_BYTE( "mpr13087.bin", 0x100000, 0x40000, 0x2293427d )
	ROM_LOAD16_BYTE( "epr13079.bin", 0x180001, 0x40000, 0xde9221ed )
	ROM_LOAD16_BYTE( "epr13086.bin", 0x180000, 0x40000, 0x8c9a71c4 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr13083.bin", 0x10000, 0x20000, 0x5df3af20 )
	ROM_LOAD( "epr13076.bin", 0x30000, 0x40000, 0x94e6c76e )
	ROM_LOAD( "epr13077.bin", 0x70000, 0x40000, 0xe2ec0d8d )
	ROM_LOAD( "epr13078.bin", 0xb0000, 0x40000, 0x15684dc5 )
ROM_END

/* Bloxeed */
ROM_START( bloxeed )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "rom-e.rom", 0x000000, 0x20000, 0xa481581a )
	ROM_LOAD16_BYTE( "rom-o.rom", 0x000001, 0x20000, 0xdd1bc3bf )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "scr0.rom", 0x00000, 0x10000, 0xe024aa33 )
	ROM_LOAD( "scr1.rom", 0x10000, 0x10000, 0x8041b814 )
	ROM_LOAD( "scr2.rom", 0x20000, 0x10000, 0xde32285e )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "obj0-e.rom", 0x00001, 0x10000, 0x90d31a8c )
	ROM_LOAD16_BYTE( "obj0-o.rom", 0x00000, 0x10000, 0xf0c0f49d )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "sound0.rom",	 0x00000, 0x20000, 0x6f2fc63c )
ROM_END

/* Clutch Hitter */
ROM_START( cltchitr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr13795.6a", 0x000000, 0x40000, 0xb0b60b67 )
	ROM_LOAD16_BYTE( "epr13751.4a", 0x000001, 0x40000, 0xc8d80233 )
	ROM_LOAD16_BYTE( "epr13786.7a", 0x080000, 0x40000, 0x3095dac0 )
	ROM_LOAD16_BYTE( "epr13784.5a", 0x080001, 0x40000, 0x80c8180d )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr13787.10a", 0x000000, 0x80000, 0xf05c68c6 )
	ROM_LOAD( "mpr13788.11a", 0x080000, 0x80000, 0x0106fea6 )
	ROM_LOAD( "mpr13789.12a", 0x100000, 0x80000, 0x09ba8835 )

	ROM_REGION( 0x300000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13773.1c",  0x000001, 0x80000, 0x3fc600e5 )
	ROM_LOAD16_BYTE( "mpr13774.2c",  0x000000, 0x80000, 0x2411a824 )
	ROM_LOAD16_BYTE( "mpr13775.3c",  0x100001, 0x80000, 0xcf527bf6 )
	ROM_LOAD16_BYTE( "mpr13779.10c", 0x100000, 0x80000, 0xc707f416 )
	ROM_LOAD16_BYTE( "mpr13780.11c", 0x200001, 0x80000, 0xa4c341e0 )
	ROM_LOAD16_BYTE( "mpr13781.12c", 0x200000, 0x80000, 0xf33b13af )

	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr13793.7c",    0x000000, 0x80000, 0xa3d31944 )
	ROM_LOAD( "epr13791.5c",	0x080000, 0x80000, 0x35c16d80 )
	ROM_LOAD( "epr13792.6c",    0x100000, 0x80000, 0x808f9695 )
ROM_END

/* DD Crew */
ROM_START( ddcrew )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "14153.6a", 0x000000, 0x40000, 0xe01fae0c )
	ROM_LOAD16_BYTE( "14152.4a", 0x000001, 0x40000, 0x69c7b571 )
	ROM_LOAD16_BYTE( "14141.7a", 0x080000, 0x40000, 0x080a494b )
	ROM_LOAD16_BYTE( "14139.5a", 0x080001, 0x40000, 0x06c31531 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "14127.1c", 0x00000, 0x40000, 0x2228cd88 )
	ROM_LOAD( "14128.2c", 0x40000, 0x40000, 0xedba8e10 )
	ROM_LOAD( "14129.3c", 0x80000, 0x40000, 0xe8ecc305 )

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "14134.10c", 0x000001, 0x80000, 0x4fda6a4b )
	ROM_LOAD16_BYTE( "14142.10a", 0x000000, 0x80000, 0x3cbf1f2a )
	ROM_LOAD16_BYTE( "14135.11c", 0x100001, 0x80000, 0xe9c74876 )
	ROM_LOAD16_BYTE( "14143.11a", 0x100000, 0x80000, 0x59022c31 )
	ROM_LOAD16_BYTE( "14136.12c", 0x200001, 0x80000, 0x720d9858 )
	ROM_LOAD16_BYTE( "14144.12a", 0x200000, 0x80000, 0x7775fdd4 )
	ROM_LOAD16_BYTE( "14137.13c", 0x300001, 0x80000, 0x846c4265 )
	ROM_LOAD16_BYTE( "14145.13a", 0x300000, 0x80000, 0x0e76c797 )

	ROM_REGION( 0x1a0000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "14133.7c",	 0x000000, 0x20000, 0xcff96665 )
	ROM_LOAD( "14130.4c",    0x020000, 0x80000, 0x948f34a1 )
	ROM_LOAD( "14131.5c",    0x0a0000, 0x80000, 0xbe5a7d0b )
	ROM_LOAD( "14132.6c",    0x120000, 0x80000, 0x1fae0220 )
ROM_END

/* Laser Ghost */
ROM_START( lghost )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "13429", 0x000000, 0x20000, 0x0e0ccf26 )
	ROM_LOAD16_BYTE( "13437", 0x000001, 0x20000, 0x38b4dc2f )
	ROM_LOAD16_BYTE( "13411", 0x040000, 0x20000, 0xc3aeae07 )
	ROM_LOAD16_BYTE( "13413", 0x040001, 0x20000, 0x75f43e21 )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "13414", 0x00000, 0x20000, 0x82025f3b )
	ROM_LOAD( "13415", 0x20000, 0x20000, 0xa76852e9 )
	ROM_LOAD( "13416", 0x40000, 0x20000, 0xe88db149 )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "13603", 0x00001, 0x20000, 0x2e3cc07b )
	ROM_LOAD16_BYTE( "13604", 0x00000, 0x20000, 0x576388af )
	ROM_LOAD16_BYTE( "13421", 0x40001, 0x20000, 0xabee8771 )
	ROM_LOAD16_BYTE( "13424", 0x40000, 0x20000, 0x260ab077 )
	ROM_LOAD16_BYTE( "13422", 0x80001, 0x20000, 0x36cef12c )
	ROM_LOAD16_BYTE( "13425", 0x80000, 0x20000, 0xe0ff8807 )
	ROM_LOAD16_BYTE( "13423", 0xc0001, 0x20000, 0x5b8e0053 )
	ROM_LOAD16_BYTE( "13426", 0xc0000, 0x20000, 0xc689853b )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "13417",	 0x00000, 0x20000, 0xcd7beb49 )
	ROM_LOAD( "13420",   0x20000, 0x20000, 0x03199cbb )
	ROM_LOAD( "13419",   0x40000, 0x20000, 0xa918ef68 )
	ROM_LOAD( "13418",   0x60000, 0x20000, 0x4006c9f1 )
ROM_END

ROM_START( moonwalk )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code - custom cpu 317-0159 */
	ROM_LOAD16_BYTE( "epr13235.a6", 0x000000, 0x40000, 0x6983e129 )
	ROM_LOAD16_BYTE( "epr13234.a5", 0x000001, 0x40000, 0xc9fd20f2 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr13216.b1", 0x00000, 0x40000, 0x862d2c03 )
	ROM_LOAD( "mpr13217.b2", 0x40000, 0x40000, 0x7d1ac3ec )
	ROM_LOAD( "mpr13218.b3", 0x80000, 0x40000, 0x56d3393c )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13224.b11", 0x000001, 0x40000, 0xc59f107b )
	ROM_LOAD16_BYTE( "mpr13231.a11", 0x000000, 0x40000, 0xa5e96346 )
	ROM_LOAD16_BYTE( "mpr13223.b10", 0x080001, 0x40000, 0x364f60ff )
	ROM_LOAD16_BYTE( "mpr13230.a10", 0x080000, 0x40000, 0x9550091f )
	ROM_LOAD16_BYTE( "mpr13222.b9",  0x100001, 0x40000, 0x523df3ed )
	ROM_LOAD16_BYTE( "mpr13229.a9",  0x100000, 0x40000, 0xf40dc45d )
	ROM_LOAD16_BYTE( "epr13221.b8",  0x180001, 0x40000, 0x9ae7546a )
	ROM_LOAD16_BYTE( "epr13228.a8",  0x180000, 0x40000, 0xde3786be )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr13225.a4", 0x10000, 0x20000, 0x56c2e82b )
	ROM_LOAD( "mpr13219.b4", 0x30000, 0x40000, 0x19e2061f )
	ROM_LOAD( "mpr13220.b5", 0x70000, 0x40000, 0x58d4d9ce )
	ROM_LOAD( "mpr13249.b6", 0xb0000, 0x40000, 0x623edc5d )
ROM_END

ROM_START( moonwlka )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code - custom cpu 317-0158 */
	ROM_LOAD16_BYTE( "epr13233", 0x000000, 0x40000, 0xf3dac671 )
	ROM_LOAD16_BYTE( "epr13232", 0x000001, 0x40000, 0x541d8bdf )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr13216.b1", 0x00000, 0x40000, 0x862d2c03 )
	ROM_LOAD( "mpr13217.b2", 0x40000, 0x40000, 0x7d1ac3ec )
	ROM_LOAD( "mpr13218.b3", 0x80000, 0x40000, 0x56d3393c )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13224.b11", 0x000001, 0x40000, 0xc59f107b )
	ROM_LOAD16_BYTE( "mpr13231.a11", 0x000000, 0x40000, 0xa5e96346 )
	ROM_LOAD16_BYTE( "mpr13223.b10", 0x080001, 0x40000, 0x364f60ff )
	ROM_LOAD16_BYTE( "mpr13230.a10", 0x080000, 0x40000, 0x9550091f )
	ROM_LOAD16_BYTE( "mpr13222.b9",  0x100001, 0x40000, 0x523df3ed )
	ROM_LOAD16_BYTE( "mpr13229.a9",  0x100000, 0x40000, 0xf40dc45d )
	ROM_LOAD16_BYTE( "epr13221.b8",  0x180001, 0x40000, 0x9ae7546a )
	ROM_LOAD16_BYTE( "epr13228.a8",  0x180000, 0x40000, 0xde3786be )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr13225.a4", 0x10000, 0x20000, 0x56c2e82b )
	ROM_LOAD( "mpr13219.b4", 0x30000, 0x40000, 0x19e2061f )
	ROM_LOAD( "mpr13220.b5", 0x70000, 0x40000, 0x58d4d9ce )
	ROM_LOAD( "mpr13249.b6", 0xb0000, 0x40000, 0x623edc5d )
ROM_END

ROM_START( moonwlkb )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "moonwlkb.01", 0x000000, 0x10000, 0xf49cdb16 )
	ROM_LOAD16_BYTE( "moonwlkb.05", 0x000001, 0x10000, 0xc483f29f )
	ROM_LOAD16_BYTE( "moonwlkb.02", 0x020000, 0x10000, 0x0bde1896 )
	ROM_LOAD16_BYTE( "moonwlkb.06", 0x020001, 0x10000, 0x5b9fc688 )
	ROM_LOAD16_BYTE( "moonwlkb.03", 0x040000, 0x10000, 0x0c5fe15c )
	ROM_LOAD16_BYTE( "moonwlkb.07", 0x040001, 0x10000, 0x9e600704 )
	ROM_LOAD16_BYTE( "moonwlkb.04", 0x060000, 0x10000, 0x64692f79 )
	ROM_LOAD16_BYTE( "moonwlkb.08", 0x060001, 0x10000, 0x546ca530 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "mpr13216.b1", 0x00000, 0x40000, 0x862d2c03 )
	ROM_LOAD( "mpr13217.b2", 0x40000, 0x40000, 0x7d1ac3ec )
	ROM_LOAD( "mpr13218.b3", 0x80000, 0x40000, 0x56d3393c )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr13224.b11", 0x000001, 0x40000, 0xc59f107b )
	ROM_LOAD16_BYTE( "mpr13231.a11", 0x000000, 0x40000, 0xa5e96346 )
	ROM_LOAD16_BYTE( "mpr13223.b10", 0x080001, 0x40000, 0x364f60ff )
	ROM_LOAD16_BYTE( "mpr13230.a10", 0x080000, 0x40000, 0x9550091f )
	ROM_LOAD16_BYTE( "mpr13222.b9",  0x100001, 0x40000, 0x523df3ed )
	ROM_LOAD16_BYTE( "mpr13229.a9",  0x100000, 0x40000, 0xf40dc45d )
	ROM_LOAD16_BYTE( "epr13221.b8",  0x180001, 0x40000, 0x9ae7546a )
	ROM_LOAD16_BYTE( "epr13228.a8",  0x180000, 0x40000, 0xde3786be )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr13225.a4", 0x10000, 0x20000, 0x56c2e82b )
	ROM_LOAD( "mpr13219.b4", 0x30000, 0x40000, 0x19e2061f )
	ROM_LOAD( "mpr13220.b5", 0x70000, 0x40000, 0x58d4d9ce )
	ROM_LOAD( "mpr13249.b6", 0xb0000, 0x40000, 0x623edc5d )
ROM_END

/* Shadow Dancer */
ROM_START( shdancer )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "shdancer.a6", 0x000000, 0x40000, 0x3d5b3fa9 )
	ROM_LOAD16_BYTE( "shdancer.a5", 0x000001, 0x40000, 0x2596004e )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sd12712.bin", 0x00000, 0x40000, 0x9bdabe3d )
	ROM_LOAD( "sd12713.bin", 0x40000, 0x40000, 0x852d2b1c )
	ROM_LOAD( "sd12714.bin", 0x80000, 0x40000, 0x448226ce )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "sd12719.bin",  0x000001, 0x40000, 0xd6888534 )
	ROM_LOAD16_BYTE( "sd12726.bin",  0x000000, 0x40000, 0xff344945 )
	ROM_LOAD16_BYTE( "sd12718.bin",  0x080001, 0x40000, 0xba2efc0c )
	ROM_LOAD16_BYTE( "sd12725.bin",  0x080000, 0x40000, 0x268a0c17 )
	ROM_LOAD16_BYTE( "sd12717.bin",  0x100001, 0x40000, 0xc81cc4f8 )
	ROM_LOAD16_BYTE( "sd12724.bin",  0x100000, 0x40000, 0x0f4903dc )
	ROM_LOAD16_BYTE( "sd12716.bin",  0x180001, 0x40000, 0xa870e629 )
	ROM_LOAD16_BYTE( "sd12723.bin",  0x180000, 0x40000, 0xc606cf90 )

	ROM_REGION( 0x70000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "sd12720.bin", 0x10000, 0x20000, 0x7a0d8de1 )
	ROM_LOAD( "sd12715.bin", 0x30000, 0x40000, 0x07051a52 )
ROM_END

ROM_START( shdancbl )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ic39", 0x000000, 0x10000, 0xadc1781c )
	ROM_LOAD16_BYTE( "ic53", 0x000001, 0x10000, 0x1c1ac463 )
	ROM_LOAD16_BYTE( "ic38", 0x020000, 0x10000, 0xcd6e155b )
	ROM_LOAD16_BYTE( "ic52", 0x020001, 0x10000, 0xbb3c49a4 )
	ROM_LOAD16_BYTE( "ic37", 0x040000, 0x10000, 0x1bd8d5c3 )
	ROM_LOAD16_BYTE( "ic51", 0x040001, 0x10000, 0xce2e71b4 )
	ROM_LOAD16_BYTE( "ic36", 0x060000, 0x10000, 0xbb861290 )
	ROM_LOAD16_BYTE( "ic50", 0x060001, 0x10000, 0x7f7b82b1 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ic4",  0x00000, 0x20000, 0xf0a016fe )
	ROM_LOAD( "ic18", 0x20000, 0x20000, 0xf6bee053 )
	ROM_LOAD( "ic3",  0x40000, 0x20000, 0xe07e6b5d )
	ROM_LOAD( "ic17", 0x60000, 0x20000, 0xf59deba1 )
	ROM_LOAD( "ic2",  0x80000, 0x20000, 0x60095070 )
	ROM_LOAD( "ic16", 0xa0000, 0x20000, 0x0f0d5dd3 )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ic73", 0x000001, 0x10000, 0x59e77c96 )
	ROM_LOAD16_BYTE( "ic74", 0x000000, 0x10000, 0x90ea5407 )
	ROM_LOAD16_BYTE( "ic75", 0x020001, 0x10000, 0x27d2fa61 )
	ROM_LOAD16_BYTE( "ic76", 0x020000, 0x10000, 0xf36db688 )
	ROM_LOAD16_BYTE( "ic58", 0x040001, 0x10000, 0x9cd5c8c7 )
	ROM_LOAD16_BYTE( "ic59", 0x040000, 0x10000, 0xff40e872 )
	ROM_LOAD16_BYTE( "ic60", 0x060001, 0x10000, 0x826d7245 )
	ROM_LOAD16_BYTE( "ic61", 0x060000, 0x10000, 0xdcf8068b )
	ROM_LOAD16_BYTE( "ic77", 0x080001, 0x10000, 0xf93470b7 )
	ROM_LOAD16_BYTE( "ic78", 0x080000, 0x10000, 0x4d523ea3 )
	ROM_LOAD16_BYTE( "ic95", 0x0a0001, 0x10000, 0x828b8294 )
	ROM_LOAD16_BYTE( "ic94", 0x0a0000, 0x10000, 0x542b2d1e )
	ROM_LOAD16_BYTE( "ic62", 0x0c0001, 0x10000, 0x50ca8065 )
	ROM_LOAD16_BYTE( "ic63", 0x0c0000, 0x10000, 0xd1866aa9 )
	ROM_LOAD16_BYTE( "ic90", 0x0e0001, 0x10000, 0x3602b758 )
	ROM_LOAD16_BYTE( "ic89", 0x0e0000, 0x10000, 0x1ba4be93 )
	ROM_LOAD16_BYTE( "ic79", 0x100001, 0x10000, 0xf22548ee )
	ROM_LOAD16_BYTE( "ic80", 0x100000, 0x10000, 0x6209f7f9 )
	ROM_LOAD16_BYTE( "ic81", 0x120001, 0x10000, 0x34692f23 )
	ROM_LOAD16_BYTE( "ic82", 0x120000, 0x10000, 0x7ae40237 )
	ROM_LOAD16_BYTE( "ic64", 0x140001, 0x10000, 0x7a8b7bcc )
	ROM_LOAD16_BYTE( "ic65", 0x140000, 0x10000, 0x90ffca14 )
	ROM_LOAD16_BYTE( "ic66", 0x160001, 0x10000, 0x5d655517 )
	ROM_LOAD16_BYTE( "ic67", 0x160000, 0x10000, 0x0e5d0855 )
	ROM_LOAD16_BYTE( "ic83", 0x180001, 0x10000, 0xa9040a32 )
	ROM_LOAD16_BYTE( "ic84", 0x180000, 0x10000, 0xd6810031 )
	ROM_LOAD16_BYTE( "ic92", 0x1a0001, 0x10000, 0xb57d5cb5 )
	ROM_LOAD16_BYTE( "ic91", 0x1a0000, 0x10000, 0x49def6c8 )
	ROM_LOAD16_BYTE( "ic68", 0x1c0001, 0x10000, 0x8d684e53 )
	ROM_LOAD16_BYTE( "ic69", 0x1c0000, 0x10000, 0xc47d32e2 )
	ROM_LOAD16_BYTE( "ic88", 0x1e0001, 0x10000, 0x9de140e1 )
	ROM_LOAD16_BYTE( "ic87", 0x1e0000, 0x10000, 0x8172a991 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ic45", 0x10000, 0x10000, 0x576b3a81 )
	ROM_LOAD( "ic46", 0x20000, 0x10000, 0xc84e8c84 )
ROM_END

ROM_START( shdancrj )
	ROM_REGION( 0x080000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "sd12722b.bin", 0x000000, 0x40000, 0xc00552a2 )
	ROM_LOAD16_BYTE( "sd12721b.bin", 0x000001, 0x40000, 0x653d351a )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sd12712.bin",  0x00000, 0x40000, 0x9bdabe3d )
	ROM_LOAD( "sd12713.bin",  0x40000, 0x40000, 0x852d2b1c )
	ROM_LOAD( "sd12714.bin",  0x80000, 0x40000, 0x448226ce )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "sd12719.bin",  0x000001, 0x40000, 0xd6888534 )
	ROM_LOAD16_BYTE( "sd12726.bin",  0x000000, 0x40000, 0xff344945 )
	ROM_LOAD16_BYTE( "sd12718.bin",  0x080001, 0x40000, 0xba2efc0c )
	ROM_LOAD16_BYTE( "sd12725.bin",  0x080000, 0x40000, 0x268a0c17 )
	ROM_LOAD16_BYTE( "sd12717.bin",  0x100001, 0x40000, 0xc81cc4f8 )
	ROM_LOAD16_BYTE( "sd12724.bin",  0x100000, 0x40000, 0x0f4903dc )
	ROM_LOAD16_BYTE( "sd12716.bin",  0x180001, 0x40000, 0xa870e629 )
	ROM_LOAD16_BYTE( "sd12723.bin",  0x180000, 0x40000, 0xc606cf90 )

	ROM_REGION( 0x70000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "sd12720.bin", 0x10000, 0x20000, 0x7a0d8de1 )
	ROM_LOAD( "sd12715.bin", 0x30000, 0x40000, 0x07051a52 )
ROM_END

/*****************************************************************************/

GAMEX(1990, astorm,   0,        astorm,   astorm,   astorm,   ROT0_16BIT,   "Sega",    "Alien Storm", GAME_NOT_WORKING )
GAMEX(1990, astorm2p, astorm,   astorm,   astorm,   astorm,   ROT0_16BIT,   "Sega",    "Alien Storm (2 Player)", GAME_NOT_WORKING )
GAME( 1990, astormbl, astorm,   astorm,   astorm,   astorm,   ROT0_16BIT,   "bootleg", "Alien Storm (bootleg)" )
GAMEX(1990, moonwalk, 0,        moonwalk, moonwalk, moonwalk, ROT0,         "Sega",    "Moon Walker (Set 1)", GAME_NOT_WORKING )
GAMEX(1990, moonwlka, moonwalk, moonwalk, moonwalk, moonwalk, ROT0,         "Sega",    "Moon Walker (Set 2)", GAME_NOT_WORKING )
GAME( 1990, moonwlkb, moonwalk, moonwalk, moonwalk, moonwalk, ROT0,         "bootleg", "Moon Walker (bootleg)" )
GAME( 1989, shdancer, 0,        shdancer, shdancer, shdancer, ROT0,         "Sega",    "Shadow Dancer (US)" )
GAMEX(1989, shdancbl, shdancer, shdancbl, shdancer, shdancbl, ROT0,         "bootleg", "Shadow Dancer (bootleg)", GAME_NOT_WORKING )
GAME( 1989, shdancrj, shdancer, shdancrj, shdancer, shdancrj, ROT0,         "Sega",    "Shadow Dancer (Japan)" )

GAMEX(????, aceattac, 0,        shdancer, shdancer, shdancer, ROT0,         "Sega", "Ace Attacker", GAME_NOT_WORKING )
GAMEX(????, bloxeed,  0,        shdancer, shdancer, shdancer, ROT0,         "Sega", "Bloxeed", GAME_NOT_WORKING )
GAMEX(????, cltchitr, 0,        shdancer, shdancer, shdancer, ROT0,         "Sega", "Clutch Hitter", GAME_NOT_WORKING )
GAMEX(????, ddcrew,   0,        shdancer, shdancer, shdancer, ROT0,         "Sega", "DD Crew", GAME_NOT_WORKING )
GAMEX(????, lghost,   0,        shdancer, shdancer, shdancer, ROT0,         "Sega", "Laser Ghost", GAME_NOT_WORKING )
