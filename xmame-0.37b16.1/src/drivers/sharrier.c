/*	Space Harrier Hardware
**
**	2xMC68000 + Z80
**	YM2151 or YM2203 + Custom PCM
**
**	Enduro Racer
**	Space Harrier
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "machine/system16.h"

/***************************************************************************/

static void generate_gr_screen(
	int w, /* 512 */
	int bitmap_width, /* 1024 */
	int skip, /* 8 */
	int start_color,int end_color, /* 0..4 */
	int source_size )
{
	/* preprocess road data, expanding it into a form more easily rendered */
	UINT8 *buf = malloc( source_size );
	if( buf ){
		UINT8 *buf0 = buf; /* remember so we can free and not leak memory */
		UINT8 *gr = memory_region(REGION_GFX3); /* road gfx data */
		UINT8 *grr = NULL;
	    int row_offset,byte_offset,bit_offset;
	    int center_offset=0;
		sys16_gr_bitmap_width = bitmap_width;

/*logerror( "generating road gfx; bitmap_width = %d\n", bitmap_width ); */

		memcpy( buf,gr,source_size ); /* copy from ROM to temp buffer */
		memset( gr,0,256*bitmap_width ); /* erase */

		if( w!=sys16_gr_bitmap_width ){
			if( skip>0 ) /* needs mirrored RHS */
				grr=gr;
			else {
				center_offset= bitmap_width-w;
				gr+=center_offset/2;
			}
		}

		for( row_offset=0; row_offset<256; row_offset++ ){ /* build gr_bitmap */
			UINT8 last_bit;
			UINT8 color_data[4];

			color_data[0]=start_color;
			color_data[1]=start_color+1;
			color_data[2]=start_color+2;
			color_data[3]=start_color+3;

			last_bit = ((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
			for( byte_offset=0; byte_offset<w/8; byte_offset++ ){
				for( bit_offset=0; bit_offset<8; bit_offset++ ){
					UINT8 bit=((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
					if( bit!=last_bit && bit==0 && row_offset>1 ){
						/* color flipped to 0? advance color[0] */
						if (color_data[0]+end_color <= end_color){
							color_data[0]+=end_color;
						}
						else{
							color_data[0]-=end_color;
						}
					}
					*gr++ = color_data[bit];
/*					logerror( "%01x", color_data[bit] ); */
					last_bit=bit;
					buf[0] <<= 1; buf[0x4000] <<= 1;
				}
				buf++;
			}
/*			logerror( "\n" ); */

			if( grr!=NULL ){ /* need mirrored RHS */
				const UINT8 *temp = gr-1-skip;
				for( byte_offset=0; byte_offset<w-skip; byte_offset++){
					*gr++ = *temp--;
				}
				for( byte_offset=0; byte_offset<skip; byte_offset++){
					*gr++ = 0;
				}
			}
			else {
				gr += center_offset;
			}
		}
		{
			int i=1;
			while ( (1<<i) < sys16_gr_bitmap_width ) i++;
			sys16_gr_bitmap_width=i; /* power of 2 */
		}
/*		logerror( "width = %d\n", sys16_gr_bitmap_width ); */
		free( buf0 );
	}
}

static void set_tile_bank( int data ){
	sys16_tile_bank1 = data&0xf;
	sys16_tile_bank0 = (data>>4)&0xf;
}

static void set_tile_bank18( int data ){
	sys16_tile_bank0 = data&0xf;
	sys16_tile_bank1 = (data>>4)&0xf;
}

static void set_page( int page[4], data16_t data ){
	page[1] = data>>12;
	page[0] = (data>>8)&0xf;
	page[3] = (data>>4)&0xf;
	page[2] = data&0xf;
}

static int sys16_interrupt( void ){
	if(sys16_custom_irq) sys16_custom_irq();
	return 4; /* Interrupt vector 4, used by VBlank */
}

static WRITE16_HANDLER( sound_command_nmi_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}

static data16_t coinctrl;

static WRITE16_HANDLER( sys16_3d_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x10;
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (0 in fantzone) */

		/* Hang-On, Super Hang-On, Space Harrier, Enduro Racer */
		set_led_status(0,coinctrl & 0x04);

		/* Space Harrier */
		set_led_status(1,coinctrl & 0x08);
	}
}

static READ16_HANDLER( sys16_coinctrl_r ){
	return coinctrl;
}

static WRITE16_HANDLER( sys16_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x20;
		coin_counter_w(0,coinctrl & 0x01);
		set_led_status(0,coinctrl & 0x04);
		set_led_status(1,coinctrl & 0x08);
		/* bit 6 is also used (1 most of the time; 0 in dduxbl, sdi, wb3;
		   tturf has it normally 1 but 0 after coin insertion) */
		/* eswat sets bit 4 */
	}
}



static READ16_HANDLER( ho_io_x_r ){ return input_port_0_r( offset ); }
static READ16_HANDLER( ho_io_y_r ){ return (input_port_1_r( offset ) << 8) + input_port_5_r( offset ); }

static READ16_HANDLER( ho_io_highscoreentry_r ){
	int mode= sys16_extraram4[0x3000/2];
	if( mode&4 ){	/* brake */
		if(ho_io_y_r(0,0) & 0x00ff) return 0xffff;
	}
	else if( mode&8 ){ /* button */
		if(ho_io_y_r(0,0) & 0xff00) return 0xffff;
	}
	return 0;
}

static READ16_HANDLER( hangon1_skip_r ){
	if (cpu_get_pc()==0x17e6) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_extraram[0x0400/2];
}

static MEMORY_READ16_START( hangon_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x20c400, 0x20c401, hangon1_skip_r },
	{ 0x20c000, 0x20ffff, SYS16_MRA16_EXTRAM },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x600000, 0x600fff, SYS16_MRA16_SPRITERAM },
	{ 0xa00000, 0xa00fff, SYS16_MRA16_PALETTERAM },
	{ 0xc68000, 0xc68fff, SYS16_MRA16_EXTRAM2 },
	{ 0xc7e000, 0xc7ffff, SYS16_MRA16_EXTRAM3 },
	{ 0xe00002, 0xe00003, sys16_coinctrl_r },
	{ 0xe01000, 0xe01001, input_port_2_word_r }, /* service */
	{ 0xe0100c, 0xe0100d, input_port_4_word_r }, /* dip2 */
	{ 0xe0100a, 0xe0100b, input_port_3_word_r }, /* dip1 */
	{ 0xe03020, 0xe03021, ho_io_highscoreentry_r },
	{ 0xe03028, 0xe03029, ho_io_x_r },
	{ 0xe0302a, 0xe0302b, ho_io_y_r },
MEMORY_END

static MEMORY_WRITE16_START( hangon_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x20c000, 0x20ffff, SYS16_MWA16_EXTRAM },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM },
	{ 0x600000, 0x600fff, SYS16_MWA16_SPRITERAM },
	{ 0xa00000, 0xa00fff, SYS16_MWA16_PALETTERAM },
	{ 0xc68000, 0xc68fff, SYS16_MWA16_EXTRAM2 },
	{ 0xc7e000, 0xc7ffff, SYS16_MWA16_EXTRAM3 },
	{ 0xe00000, 0xe00001, sound_command_nmi_w },
	{ 0xe00002, 0xe00003, sys16_3d_coinctrl_w },
	{ 0xe00004, 0xe00005, MWA16_NOP }, /* ? */
	{ 0xe02000, 0xe02001, MWA16_NOP }, /* ? */
	{ 0xe03000, 0xe03001, MWA16_NOP }, /* ? */
MEMORY_END

static READ16_HANDLER( hangon2_skip_r ){
	if (cpu_get_pc()==0xf66) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_extraram3[0x01000/2];
}

static MEMORY_READ16_START( hangon_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xc7f000, 0xc7f001, hangon2_skip_r },
	{ 0xc68000, 0xc68fff, SYS16_MRA16_EXTRAM2 },
	{ 0xc7e000, 0xc7ffff, SYS16_MRA16_EXTRAM3 },
MEMORY_END

static MEMORY_WRITE16_START( hangon_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MWA16_EXTRAM2 },
	{ 0xc7e000, 0xc7ffff, SYS16_MWA16_EXTRAM3 },
MEMORY_END

static MEMORY_READ_START( hangon_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, YM2203_status_port_0_r },
	{ 0xe000, 0xe7ff, SegaPCM_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( hangon_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, YM2203_control_port_0_w },
	{ 0xd001, 0xd001, YM2203_write_port_0_w },
	{ 0xe000, 0xe7ff, SegaPCM_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( hangon_sound_readport )
	{ 0x40, 0x40, soundlatch_r },
PORT_END

static PORT_WRITE_START( hangon_sound_writeport )
PORT_END

/***************************************************************************/

static void hangon_update_proc( void ){
	set_page( sys16_bg_page, sys16_textram[0x74e] );
	set_page( sys16_fg_page, sys16_textram[0x74f] );
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static void hangon_init_machine( void ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_hangon;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_patch_code( 0x83bd, 0x29);
	sys16_patch_code( 0x8495, 0x2a);
	sys16_patch_code( 0x84f9, 0x2b);

	sys16_update_proc = hangon_update_proc;

	sys16_gr_ver = &sys16_extraram2[0x0];
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x06 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x04 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x02 / 2;
}

static void init_hangon( void ){
	sys16_onetime_init_machine();
	generate_gr_screen(512,1024,8,0,4,0x8000);
}

/***************************************************************************/

static struct MachineDriver machine_driver_hangon =
{
	{
		{
			CPU_M68000,
			10000000,
			hangon_readmem,hangon_writemem,0,0,
			sys16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,
			hangon_sound_readmem,hangon_sound_writemem,hangon_sound_readport,hangon_sound_writeport,
/*			ignore_interrupt,1 */
			interrupt,4
		},
		{
			CPU_M68000,
			10000000,
			hangon_readmem2,hangon_writemem2,0,0,
			sys16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	hangon_init_machine,
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	sys16_gfxdecodeinfo,
	2048*ShadowColorsMultiplier,2048*ShadowColorsMultiplier,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_hangon_vh_start,
	sys16_vh_stop,
	sys16_hangon_vh_screenrefresh,
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2203,
			&sys16_ym2203_interface
		},
		{			/* wrong sound chip?? */
			SOUND_SEGAPCM,
			&sys16_segapcm_interface_32k,
		}
	}
};




static READ16_HANDLER( sh_io_joy_r ){ return (input_port_5_r( offset ) << 8) + input_port_6_r( offset ); }

static data16_t *shared_ram;
static READ16_HANDLER( shared_ram_r ){
	return shared_ram[offset];
}
static WRITE16_HANDLER( shared_ram_w ){
	COMBINE_DATA( &shared_ram[offset] );
}

static READ16_HANDLER( sh_motor_status_r ) { return 0x0; }

static MEMORY_READ16_START( harrier_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, SYS16_MRA16_EXTRAM },
	{ 0x100000, 0x107fff, SYS16_MRA16_TILERAM },
	{ 0x108000, 0x108fff, SYS16_MRA16_TEXTRAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_PALETTERAM },
	{ 0x124000, 0x127fff, shared_ram_r },
	{ 0x130000, 0x130fff, SYS16_MRA16_SPRITERAM },
	{ 0x140002, 0x140003, sys16_coinctrl_r },
	{ 0x140010, 0x140011, input_port_2_word_r }, /* service */
	{ 0x140014, 0x140015, input_port_3_word_r }, /* dip1 */
	{ 0x140016, 0x140017, input_port_4_word_r }, /* dip2 */
	{ 0x140024, 0x140027, sh_motor_status_r },
	{ 0xc68000, 0xc68fff, SYS16_MRA16_EXTRAM2 },
MEMORY_END

static MEMORY_WRITE16_START( harrier_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x040000, 0x043fff, SYS16_MWA16_EXTRAM },
	{ 0x100000, 0x107fff, SYS16_MWA16_TILERAM },
	{ 0x108000, 0x108fff, SYS16_MWA16_TEXTRAM },
	{ 0x110000, 0x110fff, SYS16_MWA16_PALETTERAM },
	{ 0x124000, 0x127fff, shared_ram_w, &shared_ram },
	{ 0x130000, 0x130fff, SYS16_MWA16_SPRITERAM },
	{ 0x140000, 0x140001, sound_command_nmi_w },
	{ 0x140002, 0x140003, sys16_3d_coinctrl_w },
	{ 0xc68000, 0xc68fff, SYS16_MWA16_EXTRAM2 },
MEMORY_END

static MEMORY_READ16_START( harrier_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MRA16_EXTRAM2 },
	{ 0xc7c000, 0xc7ffff, shared_ram_r },
MEMORY_END

static MEMORY_WRITE16_START( harrier_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MWA16_EXTRAM2 },
	{ 0xc7c000, 0xc7ffff, shared_ram_w, &shared_ram },
MEMORY_END

static MEMORY_READ_START( harrier_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xd000, 0xd000, YM2203_status_port_0_r },
	{ 0xe000, 0xe0ff, SegaPCM_r },
	{ 0x8000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( harrier_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xd000, 0xd000, YM2203_control_port_0_w },
	{ 0xd001, 0xd001, YM2203_write_port_0_w },
	{ 0xe000, 0xe0ff, SegaPCM_w },
	{ 0x8000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( harrier_sound_readport )
	{ 0x40, 0x40, soundlatch_r },
PORT_END


static PORT_WRITE_START( harrier_sound_writeport )
PORT_END

/***************************************************************************/

static void harrier_update_proc( void ){
	int data;
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x01ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;

	data = sys16_textram[0x74f];
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;

	data = sys16_textram[0x74e];
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;

	sys16_extraram[0x492/2] = sh_io_joy_r(0,0);
}

static void harrier_init_machine( void ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_sharrier;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

/**disable illegal rom writes */
	sys16_patch_code( 0x8112, 0x4a);
	sys16_patch_code( 0x83d2, 0x4a);
	sys16_patch_code( 0x83d6, 0x4a);
	sys16_patch_code( 0x82c4, 0x4a);
	sys16_patch_code( 0x82c8, 0x4a);
	sys16_patch_code( 0x84d0, 0x4a);
	sys16_patch_code( 0x84d4, 0x4a);
	sys16_patch_code( 0x85de, 0x4a);
	sys16_patch_code( 0x85e2, 0x4a);

	sys16_update_proc = harrier_update_proc;

	sys16_gr_ver = sys16_extraram2;
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x00 / 2;
	sys16_gr_colorflip[0][1]=0x02 / 2;
	sys16_gr_colorflip[0][2]=0x04 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x00 / 2;
	sys16_gr_colorflip[1][1]=0x00 / 2;
	sys16_gr_colorflip[1][2]=0x06 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_sh_shadowpal=0;
}

static void init_sharrier( void )
{
	sys16_onetime_init_machine();
	sys16_MaxShadowColors=NumOfShadowColors / 2;
	sys16_interleave_sprite_data( 0x100000 );
	generate_gr_screen(512,512,0,0,4,0x8000);
}
/***************************************************************************/

static struct MachineDriver machine_driver_sharrier =
{
	{
		{
			CPU_M68000,
			10000000,
			harrier_readmem,harrier_writemem,0,0,
			sys16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,
			harrier_sound_readmem,harrier_sound_writemem,harrier_sound_readport,harrier_sound_writeport,
/*			ignore_interrupt,1 */
			interrupt,4
		},
		{
			CPU_M68000,
			10000000,
			harrier_readmem2,harrier_writemem2,0,0,
			sys16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	harrier_init_machine,
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	sys16_gfxdecodeinfo,
	2048*ShadowColorsMultiplier,2048*ShadowColorsMultiplier,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_hangon_vh_start,
	sys16_vh_stop,
	sys16_hangon_vh_screenrefresh,
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2203,
			&sys16_ym2203_interface
		},
		{
			SOUND_SEGAPCM,
			&sys16_segapcm_interface_32k,
		}
	}
};

/***************************************************************************/

data16_t er_io_analog_sel;

static READ16_HANDLER( er_io_analog_r )
{
	switch( er_io_analog_sel )
	{
		case 0:		/* accel */
			if(input_port_1_r( offset ) & 1)
				return 0xff;
			else
				return 0;
		case 4:		/* brake */
			if(input_port_1_r( offset ) & 2)
				return 0xff;
			else
				return 0;
		case 8:		/* bank up down? */
			if(input_port_1_r( offset ) & 4)
				return 0xff;
			else
				return 0;
		case 12:	/* handle */
			return input_port_0_r( offset );

	}
	return 0;
}

static WRITE16_HANDLER( er_io_analog_w )
{
	COMBINE_DATA( &er_io_analog_sel );
}

static READ16_HANDLER( er_reset2_r )
{
	cpu_set_reset_line(2,PULSE_LINE);
	return 0;
}

static MEMORY_READ16_START( enduror_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, SYS16_MRA16_EXTRAM },
	{ 0x100000, 0x107fff, SYS16_MRA16_TILERAM },
	{ 0x108000, 0x108fff, SYS16_MRA16_TEXTRAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_PALETTERAM },
	{ 0x124000, 0x127fff, shared_ram_r },
	{ 0x130000, 0x130fff, SYS16_MRA16_SPRITERAM },
	{ 0x140002, 0x140003, sys16_coinctrl_r },
	{ 0x140010, 0x140011, input_port_2_word_r }, /* service */
	{ 0x140014, 0x140015, input_port_3_word_r }, /* dip1 */
	{ 0x140016, 0x140017, input_port_4_word_r }, /* dip2 */
	{ 0x140030, 0x140031, er_io_analog_r },
	{ 0xe00000, 0xe00001, er_reset2_r },
MEMORY_END

static MEMORY_WRITE16_START( enduror_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x040000, 0x043fff, SYS16_MWA16_EXTRAM },
	{ 0x100000, 0x107fff, SYS16_MWA16_TILERAM },
	{ 0x108000, 0x108fff, SYS16_MWA16_TEXTRAM },
	{ 0x110000, 0x110fff, SYS16_MWA16_PALETTERAM },
	{ 0x124000, 0x127fff, shared_ram_w, &shared_ram },
	{ 0x130000, 0x130fff, SYS16_MWA16_SPRITERAM },
	{ 0x140000, 0x140001, sound_command_nmi_w },
	{ 0x140002, 0x140003, sys16_3d_coinctrl_w },
	{ 0x140030, 0x140031, er_io_analog_w },
MEMORY_END

static READ16_HANDLER( enduro_p2_skip_r ){
	if (cpu_get_pc()==0x4ba) {cpu_spinuntil_int(); return 0xffff;}
	return shared_ram[0x2000/2];
}

static MEMORY_READ16_START( enduror_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MRA16_EXTRAM2 },
	{ 0xc7e000, 0xc7e001, enduro_p2_skip_r },
	{ 0xc7c000, 0xc7ffff, shared_ram_r },
MEMORY_END

static MEMORY_WRITE16_START( enduror_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MWA16_EXTRAM2 },
	{ 0xc7c000, 0xc7ffff, shared_ram_w, &shared_ram },
MEMORY_END

static MEMORY_READ_START( enduror_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, YM2203_status_port_0_r },
	{ 0xe000, 0xe7ff, SegaPCM_r },
MEMORY_END

static MEMORY_WRITE_START( enduror_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, YM2203_control_port_0_w },
	{ 0xd001, 0xd001, YM2203_write_port_0_w },
	{ 0xe000, 0xe7ff, SegaPCM_w },
MEMORY_END

static PORT_READ_START( enduror_sound_readport )
	{ 0x40, 0x40, soundlatch_r },
PORT_END

static PORT_WRITE_START( enduror_sound_writeport )
PORT_END

static MEMORY_READ_START( enduror_b2_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
/*	{ 0xc000, 0xc7ff, MRA_RAM }, */
	{ 0xf000, 0xf7ff, SegaPCM_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( enduror_b2_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
/*	{ 0xc000, 0xc7ff, MWA_RAM }, */
	{ 0xf000, 0xf7ff, SegaPCM_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( enduror_b2_sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x80, 0x80, YM2203_status_port_1_r },
	{ 0xc0, 0xc0, YM2203_status_port_2_r },
	{ 0x40, 0x40, soundlatch_r },
PORT_END

static PORT_WRITE_START( enduror_b2_sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, YM2203_control_port_1_w },
	{ 0x81, 0x81, YM2203_write_port_1_w },
	{ 0xc0, 0xc0, YM2203_control_port_2_w },
	{ 0xc1, 0xc1, YM2203_write_port_2_w },
PORT_END

/***************************************************************************/

static void enduror_update_proc( void ){
	int data;
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x01ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;

	data = sys16_textram[0x74f];
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;

	data = sys16_textram[0x74e];
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;
}

static void enduror_init_machine( void ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_sharrier;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 13;
/*	sys16_sprxoffset = -0xbb; */
/*	sys16_fgxoffset = 8; */
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_update_proc = enduror_update_proc;

	sys16_gr_ver = &sys16_extraram2[0x0];
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x00 / 2;
	sys16_gr_colorflip[0][1]=0x02 / 2;
	sys16_gr_colorflip[0][2]=0x04 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x00 / 2;
	sys16_gr_colorflip[1][1]=0x00 / 2;
	sys16_gr_colorflip[1][2]=0x06 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_sh_shadowpal=0xff;
}

static void enduror_sprite_decode( void ){
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);
	sys16_interleave_sprite_data( 8*0x20000 );
	generate_gr_screen(512,1024,8,0,4,0x8000);

/*	enduror_decode_data (rom,rom,0x10000);	// no decrypt info. */
	enduror_decode_data (rom+0x10000/2,rom+0x10000/2,0x10000);
	enduror_decode_data2(rom+0x20000/2,rom+0x20000/2,0x10000);
}

static void endurob_sprite_decode( void ){
	sys16_interleave_sprite_data( 8*0x20000 );
	generate_gr_screen(512,1024,8,0,4,0x8000);
}

static void endurora_opcode_decode( void )
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);
	int diff = 0x50000;	/* place decrypted opcodes in a hole after RAM */


	memory_set_opcode_base(0,rom+diff/2);

	memcpy(rom+(diff+0x10000)/2,rom+0x10000/2,0x20000);
	memcpy(rom+diff/2,rom+0x30000/2,0x10000);

	/* patch code to force a reset on cpu2 when starting a new game. */
	/* Undoubtly wrong, but something like it is needed for the game to work */
	rom[(0x1866 + diff)/2] = 0x4a79;
	rom[(0x1868 + diff)/2] = 0x00e0;
	rom[(0x186a + diff)/2] = 0x0000;
}

static void endurob2_opcode_decode( void )
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);
	int diff = 0x50000;	/* place decrypted opcodes in a hole after RAM */


	memory_set_opcode_base(0,rom+diff/2);

	memcpy(rom+diff/2,rom,0x30000);

	endurob2_decode_data (rom,rom+diff/2,0x10000);
	endurob2_decode_data2(rom+0x10000/2,rom+(diff+0x10000)/2,0x10000);

	/* patch code to force a reset on cpu2 when starting a new game. */
	/* Undoubtly wrong, but something like it is needed for the game to work */
	rom[(0x1866 + diff)/2] = 0x4a79;
	rom[(0x1868 + diff)/2] = 0x00e0;
	rom[(0x186a + diff)/2] = 0x0000;
}

static void init_enduror( void )
{
	sys16_onetime_init_machine();
	sys16_MaxShadowColors=NumOfShadowColors / 2;
/*	sys16_MaxShadowColors=0; */

	enduror_sprite_decode();
}

static void init_endurobl( void )
{
	sys16_onetime_init_machine();
	sys16_MaxShadowColors=NumOfShadowColors / 2;
/*	sys16_MaxShadowColors=0; */

	endurob_sprite_decode();
	endurora_opcode_decode();
}

static void init_endurob2( void )
{
	sys16_onetime_init_machine();
	sys16_MaxShadowColors=NumOfShadowColors / 2;
/*	sys16_MaxShadowColors=0; */

	endurob_sprite_decode();
	endurob2_opcode_decode();
}

/***************************************************************************/

static struct MachineDriver machine_driver_enduror =
{
	{
		{
			CPU_M68000,
			10000000,
			enduror_readmem,enduror_writemem,0,0,
			sys16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,
			enduror_sound_readmem,enduror_sound_writemem,enduror_sound_readport,enduror_sound_writeport,
			interrupt,4
		},
		{
			CPU_M68000,
			10000000,
			enduror_readmem2,enduror_writemem2,0,0,
			sys16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	enduror_init_machine,
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	sys16_gfxdecodeinfo,
	2048*ShadowColorsMultiplier,2048*ShadowColorsMultiplier,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_hangon_vh_start,
	sys16_vh_stop,
	sys16_hangon_vh_screenrefresh,
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2203,
			&sys16_ym2203_interface
		},
		{
			SOUND_SEGAPCM,
			&sys16_segapcm_interface_32k,
		}
	}
};

static struct MachineDriver machine_driver_endurob2 =
{
	{
		{
			CPU_M68000,
			10000000,
			enduror_readmem,enduror_writemem,0,0,
			sys16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,
			enduror_b2_sound_readmem,enduror_b2_sound_writemem,enduror_b2_sound_readport,enduror_b2_sound_writeport,
			ignore_interrupt,1
		},
		{
			CPU_M68000,
			10000000,
			enduror_readmem2,enduror_writemem2,0,0,
			sys16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	2,
	enduror_init_machine,
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },
	sys16_gfxdecodeinfo,
	2048*ShadowColorsMultiplier,2048*ShadowColorsMultiplier,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sys16_hangon_vh_start,
	sys16_vh_stop,
	sys16_hangon_vh_screenrefresh,
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2203,
			&sys16_3xym2203_interface
		},
		{
			SOUND_SEGAPCM,
			&sys16_segapcm_interface_15k,
		}
	}
};

/*****************************************************************************/

ROM_START( hangon )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "6918.rom", 0x000000, 0x8000, 0x20b1c2b0 )
	ROM_LOAD16_BYTE( "6916.rom", 0x000001, 0x8000, 0x7d9db1bf )
	ROM_LOAD16_BYTE( "6917.rom", 0x010000, 0x8000, 0xfea12367 )
	ROM_LOAD16_BYTE( "6915.rom", 0x010001, 0x8000, 0xac883240 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "6841.rom", 0x00000, 0x08000, 0x54d295dc )
	ROM_LOAD( "6842.rom", 0x08000, 0x08000, 0xf677b568 )
	ROM_LOAD( "6843.rom", 0x10000, 0x08000, 0xa257f0da )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "6819.rom", 0x000001, 0x8000, 0x469dad07 )
	ROM_LOAD16_BYTE( "6820.rom", 0x000000, 0x8000, 0x87cbc6de )
	ROM_LOAD16_BYTE( "6821.rom", 0x010001, 0x8000, 0x15792969 )
	ROM_LOAD16_BYTE( "6822.rom", 0x010000, 0x8000, 0xe9718de5 )
	ROM_LOAD16_BYTE( "6823.rom", 0x020001, 0x8000, 0x49422691 )
	ROM_LOAD16_BYTE( "6824.rom", 0x020000, 0x8000, 0x701deaa4 )
	ROM_LOAD16_BYTE( "6825.rom", 0x030001, 0x8000, 0x6e23c8b4 )
	ROM_LOAD16_BYTE( "6826.rom", 0x030000, 0x8000, 0x77d0de2c )
	ROM_LOAD16_BYTE( "6827.rom", 0x040001, 0x8000, 0x7fa1bfb6 )
	ROM_LOAD16_BYTE( "6828.rom", 0x040000, 0x8000, 0x8e880c93 )
	ROM_LOAD16_BYTE( "6829.rom", 0x050001, 0x8000, 0x7ca0952d )
	ROM_LOAD16_BYTE( "6830.rom", 0x050000, 0x8000, 0xb1a63aef )
	ROM_LOAD16_BYTE( "6845.rom", 0x060001, 0x8000, 0xba08c9b8 )
	ROM_LOAD16_BYTE( "6846.rom", 0x060000, 0x8000, 0xf21e57a3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "6833.rom", 0x00000, 0x4000, 0x3b942f5f )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "6831.rom", 0x00000, 0x8000, 0xcfef5481 )
	ROM_LOAD( "6832.rom", 0x08000, 0x8000, 0x4165aea5 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "6920.rom", 0x0000, 0x8000, 0x1c95013e )
	ROM_LOAD16_BYTE( "6919.rom", 0x0001, 0x8000, 0x6ca30d69 )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "6840.rom", 0x0000, 0x8000, 0x581230e3 )
ROM_END

ROM_START( enduror )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "7640a.rom",0x00000, 0x8000, 0x1d1dc5d4 )
	ROM_LOAD16_BYTE( "7636a.rom",0x00001, 0x8000, 0x84131639 )

	ROM_LOAD16_BYTE( "7641.rom", 0x10000, 0x8000, 0x2503ae7c )
	ROM_LOAD16_BYTE( "7637.rom", 0x10001, 0x8000, 0x82a27a8c )
	ROM_LOAD16_BYTE( "7642.rom", 0x20000, 0x8000, 0x1c453bea )	/* enduro.a06 / .a09 */
	ROM_LOAD16_BYTE( "7638.rom", 0x20001, 0x8000, 0x70544779 )	/* looks like encrypted versions of */

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, 0xe7a4ff90 )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, 0x4caa0095 )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, 0x7e432683 )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "7678.rom", 0x00000, 0x8000, 0x9fb5e656 )
	ROM_LOAD( "7677.rom", 0x08000, 0x8000, 0x7764765b )
	ROM_LOAD( "7676.rom", 0x10000, 0x8000, 0x2e42e0d4 )
	ROM_LOAD( "7675.rom", 0x18000, 0x8000, 0x5cd2d61 )
	ROM_LOAD( "7674.rom", 0x20000, 0x8000, 0x1a129acf )
	ROM_LOAD( "7673.rom", 0x28000, 0x8000, 0x82602394 )
	ROM_LOAD( "7672.rom", 0x30000, 0x8000, 0xd11452f7 )
	ROM_LOAD( "7671.rom", 0x38000, 0x8000, 0xb0c7fdc6 )
	ROM_LOAD( "7670.rom", 0x40000, 0x8000, 0xdbbe2f6e )
	ROM_LOAD( "7669.rom", 0x48000, 0x8000, 0xf9525faa )
	ROM_LOAD( "7668.rom", 0x50000, 0x8000, 0xe115ce33 )
	ROM_LOAD( "7667.rom", 0x58000, 0x8000, 0x923bde9d )
	ROM_LOAD( "7666.rom", 0x60000, 0x8000, 0x23697257 )
	ROM_LOAD( "7665.rom", 0x68000, 0x8000, 0x12d77607 )
	ROM_LOAD( "7664.rom", 0x70000, 0x8000, 0x0df2cfad )
	ROM_LOAD( "7663.rom", 0x78000, 0x8000, 0x2b0b8f08 )
	ROM_LOAD( "7662.rom", 0x80000, 0x8000, 0xcb0c13c5 )
	ROM_LOAD( "7661.rom", 0x88000, 0x8000, 0xfe93a79b )
	ROM_LOAD( "7660.rom", 0x90000, 0x8000, 0x86dfbb68 )
	ROM_LOAD( "7659.rom", 0x98000, 0x8000, 0x629dc8ce )
	ROM_LOAD( "7658.rom", 0xa0000, 0x8000, 0x1677f24f )
	ROM_LOAD( "7657.rom", 0xa8000, 0x8000, 0x8158839c )
	ROM_LOAD( "7656.rom", 0xb0000, 0x8000, 0x6c741272 )
	ROM_LOAD( "7655.rom", 0xb8000, 0x8000, 0x3433fe7b )
	ROM_LOAD( "7654.rom", 0xc0000, 0x8000, 0x2db6520d )
	ROM_LOAD( "7653.rom", 0xc8000, 0x8000, 0x46a52114 )
	ROM_LOAD( "7652.rom", 0xd0000, 0x8000, 0x2880cfdb )
	ROM_LOAD( "7651.rom", 0xd8000, 0x8000, 0xd7902bad )
	ROM_LOAD( "7650.rom", 0xe0000, 0x8000, 0x642635ec )
	ROM_LOAD( "7649.rom", 0xe8000, 0x8000, 0x4edba14c )
	ROM_LOAD( "7648.rom", 0xf0000, 0x8000, 0x983ea830 )
	ROM_LOAD( "7647.rom", 0xf8000, 0x8000, 0x2e7fbec0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "7682.rom", 0x00000, 0x8000, 0xc4efbf48 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, 0xbc0c4d12 )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, 0x627b3c8c )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, 0x3e07fd32 )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, 0x22f762ab )
	/* alternate version?? */
/*	ROM_LOAD16_BYTE("7634a.rom", 0x0000, 0x8000, 0xaec83731 ) */
/*	ROM_LOAD16_BYTE("7635a.rom", 0x0001, 0x8000, 0xb2fce96f ) */

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, 0x6f146210 )
ROM_END

ROM_START( endurobl )
	ROM_REGION( 0x040000+0x010000+0x040000, REGION_CPU1, 0 ) /* 68000 code + space for RAM + space for decrypted opcodes */
	ROM_LOAD16_BYTE( "7.13j", 0x030000, 0x08000, 0xf1d6b4b7 )
	ROM_CONTINUE (            0x000000, 0x08000 )
	ROM_LOAD16_BYTE( "4.13h", 0x030001, 0x08000, 0x43bff873 )				/* rom de-coded */
	ROM_CONTINUE (            0x000001, 0x08000 )		/* data de-coded */

	ROM_LOAD16_BYTE( "8.14j", 0x010000, 0x08000, 0x2153154a )
	ROM_LOAD16_BYTE( "5.14h", 0x010001, 0x08000, 0x0a97992c )
	ROM_LOAD16_BYTE( "9.15j", 0x020000, 0x08000, 0xdb3bff1c )	/* one byte difference from */
	ROM_LOAD16_BYTE( "6.15h", 0x020001, 0x08000, 0x54b1885a )	/* enduro.a06 / enduro.a09 */

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, 0xe7a4ff90 )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, 0x4caa0095 )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, 0x7e432683 )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "7678.rom", 0x00000, 0x8000, 0x9fb5e656 )
	ROM_LOAD( "7677.rom", 0x08000, 0x8000, 0x7764765b )
	ROM_LOAD( "7676.rom", 0x10000, 0x8000, 0x2e42e0d4 )
	ROM_LOAD( "7675.rom", 0x18000, 0x8000, 0x5cd2d61 )
	ROM_LOAD( "7674.rom", 0x20000, 0x8000, 0x1a129acf )
	ROM_LOAD( "7673.rom", 0x28000, 0x8000, 0x82602394 )
	ROM_LOAD( "7672.rom", 0x30000, 0x8000, 0xd11452f7 )
	ROM_LOAD( "7671.rom", 0x38000, 0x8000, 0xb0c7fdc6 )
	ROM_LOAD( "7670.rom", 0x40000, 0x8000, 0xdbbe2f6e )
	ROM_LOAD( "7669.rom", 0x48000, 0x8000, 0xf9525faa )
	ROM_LOAD( "7668.rom", 0x50000, 0x8000, 0xe115ce33 )
	ROM_LOAD( "7667.rom", 0x58000, 0x8000, 0x923bde9d )
	ROM_LOAD( "7666.rom", 0x60000, 0x8000, 0x23697257 )
	ROM_LOAD( "7665.rom", 0x68000, 0x8000, 0x12d77607 )
	ROM_LOAD( "7664.rom", 0x70000, 0x8000, 0x0df2cfad )
	ROM_LOAD( "7663.rom", 0x78000, 0x8000, 0x2b0b8f08 )
	ROM_LOAD( "7662.rom", 0x80000, 0x8000, 0xcb0c13c5 )
	ROM_LOAD( "7661.rom", 0x88000, 0x8000, 0xfe93a79b )
	ROM_LOAD( "7660.rom", 0x90000, 0x8000, 0x86dfbb68 )
	ROM_LOAD( "7659.rom", 0x98000, 0x8000, 0x629dc8ce )
	ROM_LOAD( "7658.rom", 0xa0000, 0x8000, 0x1677f24f )
	ROM_LOAD( "7657.rom", 0xa8000, 0x8000, 0x8158839c )
	ROM_LOAD( "7656.rom", 0xb0000, 0x8000, 0x6c741272 )
	ROM_LOAD( "7655.rom", 0xb8000, 0x8000, 0x3433fe7b )
	ROM_LOAD( "7654.rom", 0xc0000, 0x8000, 0x2db6520d )
	ROM_LOAD( "7653.rom", 0xc8000, 0x8000, 0x46a52114 )
	ROM_LOAD( "7652.rom", 0xd0000, 0x8000, 0x2880cfdb )
	ROM_LOAD( "7651.rom", 0xd8000, 0x8000, 0xd7902bad )
	ROM_LOAD( "7650.rom", 0xe0000, 0x8000, 0x642635ec )
	ROM_LOAD( "7649.rom", 0xe8000, 0x8000, 0x4edba14c )
	ROM_LOAD( "7648.rom", 0xf0000, 0x8000, 0x983ea830 )
	ROM_LOAD( "7647.rom", 0xf8000, 0x8000, 0x2e7fbec0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "13.16d", 0x00000, 0x004000, 0x81c82fc9 )
	ROM_LOAD( "12.16e", 0x04000, 0x004000, 0x755bfdad )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, 0xbc0c4d12 )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, 0x627b3c8c )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, 0x3e07fd32 )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, 0x22f762ab )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, 0x6f146210 )
ROM_END

ROM_START( endurob2 )
	ROM_REGION( 0x040000+0x010000+0x040000, REGION_CPU1, 0 ) /* 68000 code + space for RAM + space for decrypted opcodes */
	ROM_LOAD16_BYTE( "enduro.a07", 0x000000, 0x08000, 0x259069bc )
	ROM_LOAD16_BYTE( "enduro.a04", 0x000001, 0x08000, 0xf584fbd9 )
	ROM_LOAD16_BYTE( "enduro.a08", 0x010000, 0x08000, 0xd234918c )
	ROM_LOAD16_BYTE( "enduro.a05", 0x010001, 0x08000, 0xa525dd57 )
	ROM_LOAD16_BYTE( "enduro.a09", 0x020000, 0x08000, 0xf6391091 )
	ROM_LOAD16_BYTE( "enduro.a06", 0x020001, 0x08000, 0x79b367d7 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, 0xe7a4ff90 )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, 0x4caa0095 )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, 0x7e432683 )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "7678.rom",		0x00000, 0x8000, 0x9fb5e656 )
	ROM_LOAD( "7677.rom", 		0x08000, 0x8000, 0x7764765b )
	ROM_LOAD( "7676.rom", 		0x10000, 0x8000, 0x2e42e0d4 )
	ROM_LOAD( "enduro.a20", 	0x18000, 0x8000, 0x7c280bc8 )
	ROM_LOAD( "7674.rom", 		0x20000, 0x8000, 0x1a129acf )
	ROM_LOAD( "7673.rom",		0x28000, 0x8000, 0x82602394 )
	ROM_LOAD( "7672.rom", 		0x30000, 0x8000, 0xd11452f7 )
	ROM_LOAD( "7671.rom", 		0x38000, 0x8000, 0xb0c7fdc6 )
	ROM_LOAD( "7670.rom", 		0x40000, 0x8000, 0xdbbe2f6e )
	ROM_LOAD( "7669.rom", 		0x48000, 0x8000, 0xf9525faa )
	ROM_LOAD( "7668.rom", 		0x50000, 0x8000, 0xe115ce33 )
	ROM_LOAD( "enduro.a28", 	0x58000, 0x8000, 0x321f034b )
	ROM_LOAD( "7666.rom", 		0x60000, 0x8000, 0x23697257 )
	ROM_LOAD( "7665.rom", 		0x68000, 0x8000, 0x12d77607 )
	ROM_LOAD( "7664.rom", 		0x70000, 0x8000, 0x0df2cfad )
	ROM_LOAD( "7663.rom", 		0x78000, 0x8000, 0x2b0b8f08 )
	ROM_LOAD( "7662.rom", 		0x80000, 0x8000, 0xcb0c13c5 )
	ROM_LOAD( "enduro.a34", 	0x88000, 0x8000, 0x296454d8 )
	ROM_LOAD( "enduro.a35", 	0x90000, 0x8000, 0x1ebe76df )
	ROM_LOAD( "enduro.a36",		0x98000, 0x8000, 0x243e34e5 )
	ROM_LOAD( "7658.rom", 		0xa0000, 0x8000, 0x1677f24f )
	ROM_LOAD( "7657.rom", 		0xa8000, 0x8000, 0x8158839c )
	ROM_LOAD( "enduro.a39",		0xb0000, 0x8000, 0x1ff3a5e2 )
	ROM_LOAD( "7655.rom", 		0xb8000, 0x8000, 0x3433fe7b )
	ROM_LOAD( "7654.rom", 		0xc0000, 0x8000, 0x2db6520d )
	ROM_LOAD( "7653.rom", 		0xc8000, 0x8000, 0x46a52114 )
	ROM_LOAD( "7652.rom", 		0xd0000, 0x8000, 0x2880cfdb )
	ROM_LOAD( "enduro.a44", 	0xd8000, 0x8000, 0x84bb12a1 )
	ROM_LOAD( "7650.rom", 		0xe0000, 0x8000, 0x642635ec )
	ROM_LOAD( "7649.rom", 		0xe8000, 0x8000, 0x4edba14c )
	ROM_LOAD( "7648.rom", 		0xf0000, 0x8000, 0x983ea830 )
	ROM_LOAD( "7647.rom", 		0xf8000, 0x8000, 0x2e7fbec0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "enduro.a16", 0x00000, 0x8000, 0xd2cb6eb5 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, 0xbc0c4d12 )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, 0x627b3c8c )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, 0x3e07fd32 )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, 0x22f762ab )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, 0x6f146210 )
ROM_END

ROM_START( sharrier )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ic97.bin", 0x000000, 0x8000, 0x7c30a036 )
	ROM_LOAD16_BYTE( "ic84.bin", 0x000001, 0x8000, 0x16deaeb1 )
	ROM_LOAD16_BYTE( "ic98.bin", 0x010000, 0x8000, 0x40b1309f )
	ROM_LOAD16_BYTE( "ic85.bin", 0x010001, 0x8000, 0xce78045c )
	ROM_LOAD16_BYTE( "ic99.bin", 0x020000, 0x8000, 0xf6391091 )
	ROM_LOAD16_BYTE( "ic86.bin", 0x020001, 0x8000, 0x79b367d7 )
	ROM_LOAD16_BYTE( "ic100.bin", 0x030000, 0x8000, 0x6171e9d3 )
	ROM_LOAD16_BYTE( "ic87.bin", 0x030001, 0x8000, 0x70cb72ef )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sic31.bin", 0x00000, 0x08000, 0x347fa325 )
	ROM_LOAD( "sic46.bin", 0x08000, 0x08000, 0x39d98bd1 )
	ROM_LOAD( "sic60.bin", 0x10000, 0x08000, 0x3da3ea6b )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "ic36.bin", 0x00000, 0x8000, 0x93e2d264 )
	ROM_LOAD( "ic35.bin", 0x08000, 0x8000, 0xcd6e7500 )
	ROM_LOAD( "ic34.bin", 0x10000, 0x8000, 0xd5e15e66 )
	ROM_LOAD( "ic33.bin", 0x18000, 0x8000, 0x60d7c1bb )
	ROM_LOAD( "ic32.bin", 0x20000, 0x8000, 0x6d7b5c97 )
	ROM_LOAD( "ic31.bin", 0x28000, 0x8000, 0x5e784271 )
	ROM_LOAD( "ic30.bin", 0x30000, 0x8000, 0xec42c9ef )
	ROM_LOAD( "ic29.bin", 0x38000, 0x8000, 0xed51fdc4 )
	ROM_LOAD( "ic28.bin", 0x40000, 0x8000, 0xedbf5fc3 )
	ROM_LOAD( "ic27.bin", 0x48000, 0x8000, 0x41f25a9c )
	ROM_LOAD( "ic26.bin", 0x50000, 0x8000, 0xac62ae2e )
	ROM_LOAD( "ic25.bin", 0x58000, 0x8000, 0xf6330038 )
	ROM_LOAD( "ic24.bin", 0x60000, 0x8000, 0xcebf797c )
	ROM_LOAD( "ic23.bin", 0x68000, 0x8000, 0x510e5e10 )
	ROM_LOAD( "ic22.bin", 0x70000, 0x8000, 0x6d4a7d7a )
	ROM_LOAD( "ic21.bin", 0x78000, 0x8000, 0xdfe75f3d )
	ROM_LOAD( "ic118.bin",0x80000, 0x8000, 0xe8c537d8 )
	ROM_LOAD( "ic17.bin", 0x88000, 0x8000, 0x5bb09a67 )
	ROM_LOAD( "ic16.bin", 0x90000, 0x8000, 0x9c782295 )
	ROM_LOAD( "ic15.bin", 0x98000, 0x8000, 0x60737b98 )
	ROM_LOAD( "ic14.bin", 0xa0000, 0x8000, 0x24596a8b )
	ROM_LOAD( "ic13.bin", 0xa8000, 0x8000, 0x7a2dad15 )
	ROM_LOAD( "ic12.bin", 0xb0000, 0x8000, 0x0f732717 )
	ROM_LOAD( "ic11.bin", 0xb8000, 0x8000, 0xa2c07741 )
	ROM_LOAD( "ic8.bin",  0xc0000, 0x8000, 0x22844fa4 )
	ROM_LOAD( "ic7.bin",  0xc8000, 0x8000, 0xdcaa2ebf )
	ROM_LOAD( "ic6.bin",  0xd0000, 0x8000, 0x3711105c )
	ROM_LOAD( "ic5.bin",  0xd8000, 0x8000, 0x70fb5ebb )
	ROM_LOAD( "ic4.bin",  0xe0000, 0x8000, 0xb537d082 )
	ROM_LOAD( "ic3.bin",  0xe8000, 0x8000, 0xf5ba4e08 )
	ROM_LOAD( "ic2.bin",  0xf0000, 0x8000, 0xfc3bf8f3 )
	ROM_LOAD( "ic1.bin",  0xf8000, 0x8000, 0xb191e22f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ic73.bin", 0x00000, 0x004000, 0xd6397933 )
	ROM_LOAD( "ic72.bin", 0x04000, 0x004000, 0x504e76d9 )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "snd7231.256", 0x00000, 0x8000, 0x871c6b14 )
	ROM_LOAD( "snd7232.256", 0x08000, 0x8000, 0x4b59340c )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "ic54.bin", 0x0000, 0x8000, 0xd7c535b6 )
	ROM_LOAD16_BYTE( "ic67.bin", 0x0001, 0x8000, 0xa6153af8 )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "pic2.bin", 0x0000, 0x8000, 0xb4740419 )
ROM_END

/***************************************************************************/

INPUT_PORTS_START( hangon )
	PORT_START	/* Steering */
		PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_CENTER , 100, 3, 0x48, 0xb7 )
	PORT_START	/* Accel / Decel */
		PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 0, 0xa2 )
	SYS16_SERVICE
	SYS16_COINAGE
	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
		PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
		PORT_DIPSETTING(    0x04, "Easy" )
		PORT_DIPSETTING(    0x06, "Normal" )
		PORT_DIPSETTING(    0x02, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x18, 0x18, "Time Adj." )
		PORT_DIPSETTING(    0x18, "Normal" )
		PORT_DIPSETTING(    0x10, "Medium" )
		PORT_DIPSETTING(    0x08, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x20, 0x20, "Play Music" )
		PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_START	/* Brake */
		PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 100, 16, 0, 0xa2 )
INPUT_PORTS_END

INPUT_PORTS_START( enduror )
	PORT_START	/* handle right left */
		PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_CENTER, 100, 4, 0x0, 0xff )
	PORT_START	/* Fake Buttons */
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* accel */
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )	/* brake */
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )	/* wheelie */
	PORT_START
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	SYS16_COINAGE
	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
		PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
		PORT_DIPSETTING(    0x01, "Moving" )
		PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
		PORT_DIPSETTING(    0x04, "Easy" )
		PORT_DIPSETTING(    0x06, "Normal" )
		PORT_DIPSETTING(    0x02, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x18, 0x18, "Time Adjust" )
		PORT_DIPSETTING(    0x10, "Easy" )
		PORT_DIPSETTING(    0x18, "Normal" )
		PORT_DIPSETTING(    0x08, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x60, 0x60, "Time Control" )
		PORT_DIPSETTING(    0x40, "Easy" )
		PORT_DIPSETTING(    0x60, "Normal" )
		PORT_DIPSETTING(    0x20, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
		PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/*PORT_START	   Y    */
	/*PORT_ANALOG( 0xff, 0x0, IPT_AD_STICK_Y | IPF_CENTER , 100, 8, 0x0, 0xff ) */
INPUT_PORTS_END

INPUT_PORTS_START( sharrier )
	SYS16_JOY1
	SYS16_JOY2
	PORT_START
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	SYS16_COINAGE
	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
		PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
		PORT_DIPSETTING(    0x01, "Moving" )
		PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
		PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
		PORT_DIPSETTING(    0x08, "2" )
		PORT_DIPSETTING(    0x0c, "3" )
		PORT_DIPSETTING(    0x04, "4" )
		PORT_DIPSETTING(    0x00, "5" )
		PORT_DIPNAME( 0x10, 0x10, "Add Player Score" )
		PORT_DIPSETTING(    0x10, "5000000" )
		PORT_DIPSETTING(    0x00, "7000000" )
		PORT_DIPNAME( 0x20, 0x20, "Trial Time" )
		PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
		PORT_DIPSETTING(    0x80, "Easy" )
		PORT_DIPSETTING(    0xc0, "Normal" )
		PORT_DIPSETTING(    0x40, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_START	/* X */
		PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X |  IPF_REVERSE, 100, 4, 0x20, 0xdf )
	PORT_START	/* Y */
		PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y |  IPF_REVERSE, 100, 4, 0x60, 0x9f )
INPUT_PORTS_END

/***************************************************************************/

GAME( 1985, hangon,   0,        hangon,   hangon,   hangon,   ROT0,         "Sega",    "Hang-On" )
GAME( 1985, sharrier, 0,        sharrier, sharrier, sharrier, ROT0_16BIT,   "Sega",    "Space Harrier" )
GAMEX(1985, enduror,  0,        enduror,  enduror,  enduror,  ROT0,         "Sega",    "Enduro Racer", GAME_NOT_WORKING )
GAME( 1985, endurobl, enduror,  enduror,  enduror,  endurobl, ROT0,         "bootleg", "Enduro Racer (bootleg set 1)" )
GAME( 1985, endurob2, enduror,  endurob2, enduror,  endurob2, ROT0,         "bootleg", "Enduro Racer (bootleg set 2)" )
