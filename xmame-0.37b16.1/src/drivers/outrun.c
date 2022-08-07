/*
**	2xMC68000 + Z80
**	YM2151 + Custom PCM
**
**	Out Run
**	Super Hang-On
**	Super Hang-On Limited Edition
**	Turbo Out Run
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "machine/system16.h"

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

static void set_fg_page1( int data ){
	sys16_fg_page[1] = data>>12;
	sys16_fg_page[0] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;
}

static void set_bg_page1( int data ){
	sys16_bg_page[1] = data>>12;
	sys16_bg_page[0] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;
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

/* hang-on's accel/brake are really both analog controls, but I've added them
as digital as well to see what works better */
/*#define HANGON_DIGITAL_CONTROLS */

static READ16_HANDLER( ho_io_x_r ){ return input_port_0_r( offset ); }
#ifdef HANGON_DIGITAL_CONTROLS
static READ16_HANDLER( ho_io_y_r ){
	int data = input_port_1_r( offset );

	switch(data & 3)
	{
		case 3:	return 0xffff;	/* both */
		case 2:	return 0x00ff;  /* brake */
		case 1:	return 0xff00;  /* accel */
		case 0:	return 0x0000;  /* neither */
	}
	return 0x0000;
}
#else
static READ16_HANDLER( ho_io_y_r ){ return (input_port_1_r( offset ) << 8) + input_port_5_r( offset ); }
#endif

/*	outrun: generate_gr_screen(0x200,0x800,0,0,3,0x8000); */
static void generate_gr_screen(
	int w,int bitmap_width,int skip,
	int start_color,int end_color, int source_size )
{
	UINT8 *buf = malloc( source_size );
	if( buf ){
		UINT8 *gr = memory_region(REGION_GFX3);
		UINT8 *grr = NULL;
	    int i,j,k;
	    int center_offset=0;
		sys16_gr_bitmap_width = bitmap_width;

		memcpy(buf,gr,source_size);
		memset(gr,0,256*bitmap_width);

		if (w!=sys16_gr_bitmap_width){
			if (skip>0) /* needs mirrored RHS */
				grr=gr;
			else {
				center_offset= bitmap_width-w;
				gr+=center_offset/2;
			}
		}

		for (i=0; i<256; i++){ /* build gr_bitmap */
			UINT8 last_bit;
			UINT8 color_data[4];
			color_data[0]=start_color;
			color_data[1]=start_color+1;
			color_data[2]=start_color+2;
			color_data[3]=start_color+3;

			last_bit = ((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
			for (j=0; j<w/8; j++){
				for (k=0; k<8; k++){
					UINT8 bit=((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
					if (bit!=last_bit && bit==0 && i>1){ /* color flipped to 0,advance color[0] */
						if (color_data[0]+end_color <= end_color){
							color_data[0]+=end_color;
						}
						else{
							color_data[0]-=end_color;
						}
					}
					*gr++ = color_data[bit];
					last_bit=bit;
					buf[0] <<= 1; buf[0x4000] <<= 1;
				}
				buf++;
			}

			if (grr!=NULL){ /* need mirrored RHS */
				const UINT8 *temp = gr-1-skip;
				for (j=0; j<w-skip; j++){
					*gr++ = *temp--;
				}
				for (j=0; j<skip; j++) *gr++ = 0;
			}
			else {
				gr+=center_offset;
			}
		}

		i=1;
		while ( (1<<i) < sys16_gr_bitmap_width ) i++;
		sys16_gr_bitmap_width=i; /* power of 2 */
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

static WRITE16_HANDLER( sound_command_nmi_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
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

static int sys16_interrupt( void ){
	if(sys16_custom_irq) sys16_custom_irq();
	return 4; /* Interrupt vector 4, used by VBlank */
}

static PORT_READ_START( sound_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
PORT_END

static data16_t *shared_ram;
static READ16_HANDLER( shared_ram_r ){
	return shared_ram[offset];
}
static WRITE16_HANDLER( shared_ram_w ){
	COMBINE_DATA( &shared_ram[offset] );
}

static unsigned char *sound_shared_ram;
static READ16_HANDLER( sound_shared_ram_r )
{
	return (sound_shared_ram[offset*2] << 8) +
			sound_shared_ram[offset*2+1];
}

static WRITE16_HANDLER( sound_shared_ram_w )
{
	if( ACCESSING_LSB ){
		sound_shared_ram[offset*2+1] = data&0xff;
	}
	if( ACCESSING_MSB ){
		sound_shared_ram[offset*2] = data>>8;
	}
}

static READ_HANDLER( sound2_shared_ram_r ){
	return sound_shared_ram[offset];
}
static WRITE_HANDLER( sound2_shared_ram_w ){
	sound_shared_ram[offset] = data;
}


ROM_START( shangon )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code - protected */
	ROM_LOAD16_BYTE( "ic133", 0x000000, 0x10000, 0xe52721fe )
	ROM_LOAD16_BYTE( "ic118", 0x000001, 0x10000, 0x5fee09f6 )
	ROM_LOAD16_BYTE( "ic132", 0x020000, 0x10000, 0x5d55d65f )
	ROM_LOAD16_BYTE( "ic117", 0x020001, 0x10000, 0xb967e8c3 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ic54",        0x00000, 0x08000, 0x260286f9 )
	ROM_LOAD( "ic55",        0x08000, 0x08000, 0xc609ee7b )
	ROM_LOAD( "ic56",        0x10000, 0x08000, 0xb236a403 )

	ROM_REGION( 0x0120000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ic8",	0x000001, 0x010000, 0xd6ac012b )
/*	ROM_RELOAD(     			0x100000, 0x010000 )	// twice? */
	ROM_LOAD16_BYTE( "ic16",  0x000000, 0x010000, 0xd9d83250 )
/*	ROM_RELOAD(              	0x100000, 0x010000 )	// twice? */
	ROM_LOAD16_BYTE( "ic7",   0x020001, 0x010000, 0x25ebf2c5 )
/*	ROM_RELOAD(              	0x0e0000, 0x010000 )	// twice? */
	ROM_LOAD16_BYTE( "ic15",  0x020000, 0x010000, 0x6365d2e9 )
/*	ROM_RELOAD(              	0x0e0000, 0x010000 )	// twice? */
	ROM_LOAD16_BYTE( "ic6",   0x040001, 0x010000, 0x8a57b8d6 )
	ROM_LOAD16_BYTE( "ic14",  0x040000, 0x010000, 0x3aff8910 )
	ROM_LOAD16_BYTE( "ic5",   0x060001, 0x010000, 0xaf473098 )
	ROM_LOAD16_BYTE( "ic13",  0x060000, 0x010000, 0x80bafeef )
	ROM_LOAD16_BYTE( "ic4",   0x080001, 0x010000, 0x03bc4878 )
	ROM_LOAD16_BYTE( "ic12",  0x080000, 0x010000, 0x274b734e )
	ROM_LOAD16_BYTE( "ic3",   0x0a0001, 0x010000, 0x9f0677ed )
	ROM_LOAD16_BYTE( "ic11",  0x0a0000, 0x010000, 0x508a4701 )
	ROM_LOAD16_BYTE( "ic2",   0x0c0001, 0x010000, 0xb176ea72 )
	ROM_LOAD16_BYTE( "ic10",  0x0c0000, 0x010000, 0x42fcd51d )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ic88", 0x0000, 0x08000, 0x1254efa6 )

	ROM_LOAD( "ic66", 0x10000, 0x08000, 0x06f55364 )
	ROM_LOAD( "ic67", 0x18000, 0x08000, 0x731f5cf8 )
	ROM_LOAD( "ic68", 0x20000, 0x08000, 0xa60dabff )
	ROM_LOAD( "ic69", 0x28000, 0x08000, 0x473cc411 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU  - protected */
	ROM_LOAD16_BYTE( "ic76", 0x0000, 0x10000, 0x02be68db )
	ROM_LOAD16_BYTE( "ic58", 0x0001, 0x10000, 0xf13e8bee )
	ROM_LOAD16_BYTE( "ic75", 0x20000, 0x10000, 0x1627c224 )
	ROM_LOAD16_BYTE( "ic57", 0x20001, 0x10000, 0x8cdbcde8 )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "ic47", 0x0000, 0x8000, 0x7836bcc3 )
ROM_END

ROM_START( shangonb )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "s-hangon.30", 0x000000, 0x10000, 0xd95e82fc )
	ROM_LOAD16_BYTE( "s-hangon.32", 0x000001, 0x10000, 0x2ee4b4fb )
	ROM_LOAD16_BYTE( "s-hangon.29", 0x020000, 0x8000, 0x12ee8716 )
	ROM_LOAD16_BYTE( "s-hangon.31", 0x020001, 0x8000, 0x155e0cfd )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ic54",        0x00000, 0x08000, 0x260286f9 )
	ROM_LOAD( "ic55",        0x08000, 0x08000, 0xc609ee7b )
	ROM_LOAD( "ic56",        0x10000, 0x08000, 0xb236a403 )

	ROM_REGION( 0x0120000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ic8",         0x000001, 0x010000, 0xd6ac012b )
/*	ROM_RELOAD(       		          0x100000, 0x010000 )	// twice? */
	ROM_LOAD16_BYTE( "ic16",        0x000000, 0x010000, 0xd9d83250 )
/*	ROM_RELOAD(             		  0x100000, 0x010000 )	// twice? */
	ROM_LOAD16_BYTE( "s-hangon.20", 0x020001, 0x010000, 0xeef23b3d )
/*	ROM_RELOAD(           		      0x0e0000, 0x010000 )	// twice? */
	ROM_LOAD16_BYTE( "s-hangon.14", 0x020000, 0x010000, 0x0f26d131 )
/*	ROM_RELOAD(              		  0x0e0000, 0x010000 )	// twice? */
	ROM_LOAD16_BYTE( "ic6",         0x040001, 0x010000, 0x8a57b8d6 )
	ROM_LOAD16_BYTE( "ic14",        0x040000, 0x010000, 0x3aff8910 )
	ROM_LOAD16_BYTE( "ic5",         0x060001, 0x010000, 0xaf473098 )
	ROM_LOAD16_BYTE( "ic13",        0x060000, 0x010000, 0x80bafeef )
	ROM_LOAD16_BYTE( "ic4",         0x080001, 0x010000, 0x03bc4878 )
	ROM_LOAD16_BYTE( "ic12",        0x080000, 0x010000, 0x274b734e )
	ROM_LOAD16_BYTE( "ic3",         0x0a0001, 0x010000, 0x9f0677ed )
	ROM_LOAD16_BYTE( "ic11",        0x0a0000, 0x010000, 0x508a4701 )
	ROM_LOAD16_BYTE( "ic2",         0x0c0001, 0x010000, 0xb176ea72 )
	ROM_LOAD16_BYTE( "ic10",        0x0c0000, 0x010000, 0x42fcd51d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-hangon.03", 0x0000, 0x08000, 0x83347dc0 )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "s-hangon.02", 0x00000, 0x10000, 0xda08ca2b )
	ROM_LOAD( "s-hangon.01", 0x10000, 0x10000, 0x8b10e601 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "s-hangon.09", 0x00000, 0x10000, 0x070c8059 )
	ROM_LOAD16_BYTE( "s-hangon.05", 0x00001, 0x10000, 0x9916c54b )
	ROM_LOAD16_BYTE( "s-hangon.08", 0x20000, 0x10000, 0x000ad595 )
	ROM_LOAD16_BYTE( "s-hangon.04", 0x20001, 0x10000, 0x8f8f4af0 )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "s-hangon.26", 0x0000, 0x8000, 0x1bbe4fc8 )
ROM_END

/* Outrun hardware */
ROM_START( outrun )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "10380a", 0x000000, 0x10000, 0x434fadbc )
	ROM_LOAD16_BYTE( "10382a", 0x000001, 0x10000, 0x1ddcc04e )
	ROM_LOAD16_BYTE( "10381a", 0x020000, 0x10000, 0xbe8c412b )
	ROM_LOAD16_BYTE( "10383a", 0x020001, 0x10000, 0xdcc586e7 )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, 0x95344b04 )
	ROM_LOAD( "10232", 0x08000, 0x08000, 0x776ba1eb )
	ROM_LOAD( "10267", 0x10000, 0x08000, 0xa85bb823 )
	ROM_LOAD( "10231", 0x18000, 0x08000, 0x8908bcbf )
	ROM_LOAD( "10266", 0x20000, 0x08000, 0x9f6f1a74 )
	ROM_LOAD( "10230", 0x28000, 0x08000, 0x686f5e50 )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "10371", 0x00000, 0x20000, 0x0a1c98de )
	ROM_LOAD( "10372", 0x20000, 0x20000, 0x1640ad1f )
	ROM_LOAD( "10373", 0x40000, 0x20000, 0x339f8e64 )
	ROM_LOAD( "10374", 0x60000, 0x20000, 0x22744340 )
	ROM_LOAD( "10375", 0x80000, 0x20000, 0x62a472bd )
	ROM_LOAD( "10376", 0xa0000, 0x20000, 0x8337ace7 )
	ROM_LOAD( "10377", 0xc0000, 0x20000, 0xc86daecb )
	ROM_LOAD( "10378", 0xe0000, 0x20000, 0x544068fd )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10187",       0x00000, 0x8000, 0xa10abaa9 )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, 0xbcd10dde )
	ROM_RELOAD(              0x30000, 0x8000 ) /* twice?? */
	ROM_LOAD( "10192",       0x08000, 0x8000, 0x770f1270 )
	ROM_LOAD( "10191",       0x10000, 0x8000, 0x20a284ab )
	ROM_LOAD( "10190",       0x18000, 0x8000, 0x7cab70e2 )
	ROM_LOAD( "10189",       0x20000, 0x8000, 0x01366b54 )
	ROM_LOAD( "10188",       0x28000, 0x8000, 0xbad30ad9 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "10327a", 0x00000, 0x10000, 0xe28a5baf )
	ROM_LOAD16_BYTE( "10329a", 0x00001, 0x10000, 0xda131c81 )
	ROM_LOAD16_BYTE( "10328a", 0x20000, 0x10000, 0xd5ec5e5d )
	ROM_LOAD16_BYTE( "10330a", 0x20001, 0x10000, 0xba9ec82a )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "10185", 0x0000, 0x8000, 0x22794426 )
ROM_END

ROM_START( outruna )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "10380b", 0x000000, 0x10000, 0x1f6cadad )
	ROM_LOAD16_BYTE( "10382b", 0x000001, 0x10000, 0xc4c3fa1a )
	ROM_LOAD16_BYTE( "10381a", 0x020000, 0x10000, 0xbe8c412b )
	ROM_LOAD16_BYTE( "10383b", 0x020001, 0x10000, 0x10a2014a )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, 0x95344b04 )
	ROM_LOAD( "10232", 0x08000, 0x08000, 0x776ba1eb )
	ROM_LOAD( "10267", 0x10000, 0x08000, 0xa85bb823 )
	ROM_LOAD( "10231", 0x18000, 0x08000, 0x8908bcbf )
	ROM_LOAD( "10266", 0x20000, 0x08000, 0x9f6f1a74 )
	ROM_LOAD( "10230", 0x28000, 0x08000, 0x686f5e50 )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "10371", 0x00000, 0x20000, 0x0a1c98de )
	ROM_LOAD( "10372", 0x20000, 0x20000, 0x1640ad1f )
	ROM_LOAD( "10373", 0x40000, 0x20000, 0x339f8e64 )
	ROM_LOAD( "10374", 0x60000, 0x20000, 0x22744340 )
	ROM_LOAD( "10375", 0x80000, 0x20000, 0x62a472bd )
	ROM_LOAD( "10376", 0xa0000, 0x20000, 0x8337ace7 )
	ROM_LOAD( "10377", 0xc0000, 0x20000, 0xc86daecb )
	ROM_LOAD( "10378", 0xe0000, 0x20000, 0x544068fd )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10187",       0x00000, 0x8000, 0xa10abaa9 )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, 0xbcd10dde )
	ROM_RELOAD(              0x30000, 0x8000 ) /* twice?? */
	ROM_LOAD( "10192",       0x08000, 0x8000, 0x770f1270 )
	ROM_LOAD( "10191",       0x10000, 0x8000, 0x20a284ab )
	ROM_LOAD( "10190",       0x18000, 0x8000, 0x7cab70e2 )
	ROM_LOAD( "10189",       0x20000, 0x8000, 0x01366b54 )
	ROM_LOAD( "10188",       0x28000, 0x8000, 0xbad30ad9 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "10327a", 0x00000, 0x10000, 0xe28a5baf )
	ROM_LOAD16_BYTE( "10329a", 0x00001, 0x10000, 0xda131c81 )
	ROM_LOAD16_BYTE( "10328a", 0x20000, 0x10000, 0xd5ec5e5d )
	ROM_LOAD16_BYTE( "10330a", 0x20001, 0x10000, 0xba9ec82a )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "10185", 0x0000, 0x8000, 0x22794426 )
ROM_END


ROM_START( outrunb )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "orun_mn.rom", 0x000000, 0x10000, 0xcddceea2 )
	ROM_LOAD16_BYTE( "orun_ml.rom", 0x000001, 0x10000, 0x9cfc07d5 )
	ROM_LOAD16_BYTE( "orun_mm.rom", 0x020000, 0x10000, 0x3092d857 )
	ROM_LOAD16_BYTE( "orun_mk.rom", 0x020001, 0x10000, 0x30a1c496 )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, 0x95344b04 )
	ROM_LOAD( "10232", 0x08000, 0x08000, 0x776ba1eb )
	ROM_LOAD( "10267", 0x10000, 0x08000, 0xa85bb823 )
	ROM_LOAD( "10231", 0x18000, 0x08000, 0x8908bcbf )
	ROM_LOAD( "10266", 0x20000, 0x08000, 0x9f6f1a74 )
	ROM_LOAD( "10230", 0x28000, 0x08000, 0x686f5e50 )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "orun_1.rom", 	0x00000, 0x10000, 0x77377e00 )
	ROM_LOAD( "orun_17.rom", 	0x10000, 0x10000, 0x4f784236 )
	ROM_LOAD( "orun_2.rom", 	0x20000, 0x10000, 0x2c0e7277 )
	ROM_LOAD( "orun_18.rom", 	0x30000, 0x10000, 0x8d459356 )
	ROM_LOAD( "orun_3.rom", 	0x40000, 0x10000, 0x69ecc975 )
	ROM_LOAD( "orun_19.rom", 	0x50000, 0x10000, 0xee4f7154 )
	ROM_LOAD( "orun_4.rom", 	0x60000, 0x10000, 0x54761e57 )
	ROM_LOAD( "orun_20.rom",	0x70000, 0x10000, 0xc2825654 )
	ROM_LOAD( "orun_5.rom", 	0x80000, 0x10000, 0xb6a8d0e2 )
	ROM_LOAD( "orun_21.rom", 	0x90000, 0x10000, 0xe9880aa3 )
	ROM_LOAD( "orun_6.rom", 	0xa0000, 0x10000, 0xa00d0676 )
	ROM_LOAD( "orun_22.rom",	0xb0000, 0x10000, 0xef7d06fe )
	ROM_LOAD( "orun_7.rom", 	0xc0000, 0x10000, 0xd632d8a2 )
	ROM_LOAD( "orun_23.rom", 	0xd0000, 0x10000, 0xdc286dc2 )
	ROM_LOAD( "orun_8.rom", 	0xe0000, 0x10000, 0xda398368 )
	ROM_LOAD( "orun_24.rom",	0xf0000, 0x10000, 0x1222af9f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "orun_ma.rom", 0x00000, 0x8000, 0xa3ff797a )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, 0xbcd10dde )
	ROM_RELOAD(              0x30000, 0x8000 ) /* twice?? */
	ROM_LOAD( "10192",       0x08000, 0x8000, 0x770f1270 )
	ROM_LOAD( "10191",       0x10000, 0x8000, 0x20a284ab )
	ROM_LOAD( "10190",       0x18000, 0x8000, 0x7cab70e2 )
	ROM_LOAD( "10189",       0x20000, 0x8000, 0x01366b54 )
	ROM_LOAD( "10188",       0x28000, 0x8000, 0xbad30ad9 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "orun_mj.rom", 0x00000, 0x10000, 0xd7f5aae0 )
	ROM_LOAD16_BYTE( "orun_mh.rom", 0x00001, 0x10000, 0x88c2e78f )
	ROM_LOAD16_BYTE( "10328a",      0x20000, 0x10000, 0xd5ec5e5d )
	ROM_LOAD16_BYTE( "orun_mg.rom", 0x20001, 0x10000, 0x74c5fbec )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "orun_me.rom", 0x0000, 0x8000, 0x666fe754 )

/*	ROM_LOAD( "orun_mf.rom", 0x0000, 0x8000, 0xed5bda9c )	//?? */
ROM_END

/* Turbo Outrun */

ROM_START( toutrun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
/* custom cpu 317-0106 */
	ROM_LOAD16_BYTE( "epr12397.133", 0x000000, 0x10000, 0xe4b57d7d )
	ROM_LOAD16_BYTE( "epr12396.118", 0x000001, 0x10000, 0x5e7115cb )
	ROM_LOAD16_BYTE( "epr12399.132", 0x020000, 0x10000, 0x62c77b1b )
	ROM_LOAD16_BYTE( "epr12398.117", 0x020001, 0x10000, 0x18e34520 )
	ROM_LOAD16_BYTE( "epr12293.131", 0x040000, 0x10000, 0xf4321eea )
	ROM_LOAD16_BYTE( "epr12292.116", 0x040001, 0x10000, 0x51d98af0 )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12323.102", 0x00000, 0x10000, 0x4de43a6f )
	ROM_LOAD( "opr12324.103", 0x10000, 0x10000, 0x24607a55 )
	ROM_LOAD( "opr12325.104", 0x20000, 0x10000, 0x1405137a )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr12307.9",  0x00001, 0x10000, 0x437dcf09 )
	ROM_LOAD16_BYTE( "opr12308.10", 0x00000, 0x10000, 0x0de70cc2 )
	ROM_LOAD16_BYTE( "opr12309.11", 0x20001, 0x10000, 0xdeb8c242 )
	ROM_LOAD16_BYTE( "opr12310.12", 0x20000, 0x10000, 0x45cf157e )
	ROM_LOAD16_BYTE( "opr12311.13", 0x40001, 0x10000, 0xae2bd639 )
	ROM_LOAD16_BYTE( "opr12312.14", 0x40000, 0x10000, 0x626000e7 )
	ROM_LOAD16_BYTE( "opr12313.15", 0x60001, 0x10000, 0x52870c37 )
	ROM_LOAD16_BYTE( "opr12314.16", 0x60000, 0x10000, 0x40c461ea )
	ROM_LOAD16_BYTE( "opr12315.17", 0x80001, 0x10000, 0x3ff9a3a3 )
	ROM_LOAD16_BYTE( "opr12316.18", 0x80000, 0x10000, 0x8a1e6dc8 )
	ROM_LOAD16_BYTE( "opr12317.19", 0xa0001, 0x10000, 0x77e382d4 )
	ROM_LOAD16_BYTE( "opr12318.20", 0xa0000, 0x10000, 0xd1afdea9 )
	ROM_LOAD16_BYTE( "opr12320.22", 0xc0001, 0x10000, 0x7931e446 )
	ROM_LOAD16_BYTE( "opr12321.23", 0xc0000, 0x10000, 0x830bacd4 )
	ROM_LOAD16_BYTE( "opr12322.24", 0xe0001, 0x10000, 0x8b812492 )
	ROM_LOAD16_BYTE( "opr12319.25", 0xe0000, 0x10000, 0xdf23baf9 )

	ROM_REGION( 0x70000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12300.88",	0x00000, 0x10000, 0xe8ff7011 )
	ROM_LOAD( "opr12301.66",    0x10000, 0x10000, 0x6e78ad15 )
	ROM_LOAD( "opr12302.67",    0x20000, 0x10000, 0xe72928af )
	ROM_LOAD( "opr12303.68",    0x30000, 0x10000, 0x8384205c )
	ROM_LOAD( "opr12304.69",    0x40000, 0x10000, 0xe1762ac3 )
	ROM_LOAD( "opr12305.70",    0x50000, 0x10000, 0xba9ce677 )
	ROM_LOAD( "opr12306.71",    0x60000, 0x10000, 0xe49249fd )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "opr12295.76", 0x000000, 0x10000, 0xd43a3a84 )
	ROM_LOAD16_BYTE( "opr12294.58", 0x000001, 0x10000, 0x27cdcfd3 )
	ROM_LOAD16_BYTE( "opr12297.75", 0x020000, 0x10000, 0x1d9b5677 )
	ROM_LOAD16_BYTE( "opr12296.57", 0x020001, 0x10000, 0x0a513671 )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* road */
	ROM_LOAD( "epr12298.11", 0x0, 0x08000, 0xfc9bc41b )
ROM_END


ROM_START( toutruna )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
/* custom cpu 317-0106 */
	ROM_LOAD16_BYTE( "epr12410.133", 0x000000, 0x10000, 0xaa74f3e9 )
	ROM_LOAD16_BYTE( "epr12409.118", 0x000001, 0x10000, 0xc11c8ef7 )
	ROM_LOAD16_BYTE( "epr12412.132", 0x020000, 0x10000, 0xb0534647 )
	ROM_LOAD16_BYTE( "epr12411.117", 0x020001, 0x10000, 0x12bb0d83 )
	ROM_LOAD16_BYTE( "epr12293.131", 0x040000, 0x10000, 0xf4321eea )
	ROM_LOAD16_BYTE( "epr12292.116", 0x040001, 0x10000, 0x51d98af0 )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12323.102", 0x00000, 0x10000, 0x4de43a6f )
	ROM_LOAD( "opr12324.103", 0x10000, 0x10000, 0x24607a55 )
	ROM_LOAD( "opr12325.104", 0x20000, 0x10000, 0x1405137a )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr12307.9",  0x00001, 0x10000, 0x437dcf09 )
	ROM_LOAD16_BYTE( "opr12308.10", 0x00000, 0x10000, 0x0de70cc2 )
	ROM_LOAD16_BYTE( "opr12309.11", 0x20001, 0x10000, 0xdeb8c242 )
	ROM_LOAD16_BYTE( "opr12310.12", 0x20000, 0x10000, 0x45cf157e )
	ROM_LOAD16_BYTE( "opr12311.13", 0x40001, 0x10000, 0xae2bd639 )
	ROM_LOAD16_BYTE( "opr12312.14", 0x40000, 0x10000, 0x626000e7 )
	ROM_LOAD16_BYTE( "opr12313.15", 0x60001, 0x10000, 0x52870c37 )
	ROM_LOAD16_BYTE( "opr12314.16", 0x60000, 0x10000, 0x40c461ea )
	ROM_LOAD16_BYTE( "opr12315.17", 0x80001, 0x10000, 0x3ff9a3a3 )
	ROM_LOAD16_BYTE( "opr12316.18", 0x80000, 0x10000, 0x8a1e6dc8 )
	ROM_LOAD16_BYTE( "opr12317.19", 0xa0001, 0x10000, 0x77e382d4 )
	ROM_LOAD16_BYTE( "opr12318.20", 0xa0000, 0x10000, 0xd1afdea9 )
	ROM_LOAD16_BYTE( "opr12320.22", 0xc0001, 0x10000, 0x7931e446 )
	ROM_LOAD16_BYTE( "opr12321.23", 0xc0000, 0x10000, 0x830bacd4 )
	ROM_LOAD16_BYTE( "opr12322.24", 0xe0001, 0x10000, 0x8b812492 )
	ROM_LOAD16_BYTE( "opr12319.25", 0xe0000, 0x10000, 0xdf23baf9 )

	ROM_REGION( 0x70000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12300.88",	0x00000, 0x10000, 0xe8ff7011 )
	ROM_LOAD( "opr12301.66",    0x10000, 0x10000, 0x6e78ad15 )
	ROM_LOAD( "opr12302.67",    0x20000, 0x10000, 0xe72928af )
	ROM_LOAD( "opr12303.68",    0x30000, 0x10000, 0x8384205c )
	ROM_LOAD( "opr12304.69",    0x40000, 0x10000, 0xe1762ac3 )
	ROM_LOAD( "opr12305.70",    0x50000, 0x10000, 0xba9ce677 )
	ROM_LOAD( "opr12306.71",    0x60000, 0x10000, 0xe49249fd )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "opr12295.76", 0x000000, 0x10000, 0xd43a3a84 )
	ROM_LOAD16_BYTE( "opr12294.58", 0x000001, 0x10000, 0x27cdcfd3 )
	ROM_LOAD16_BYTE( "opr12297.75", 0x020000, 0x10000, 0x1d9b5677 )
	ROM_LOAD16_BYTE( "opr12296.57", 0x020001, 0x10000, 0x0a513671 )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* road */
	ROM_LOAD( "epr12298.11", 0x0, 0x08000, 0xfc9bc41b )
ROM_END

/***************************************************************************/

static READ16_HANDLER( or_io_joy_r ){
	return (input_port_5_r( offset ) << 8) + input_port_6_r( offset );
}

#ifdef HANGON_DIGITAL_CONTROLS
static READ16_HANDLER( or_io_brake_r ){
	int data = input_port_1_r( offset );

	switch(data & 3)
	{
		case 3:	return 0xff00;	/* both */
		case 1:	return 0xff00;  /* brake */
		case 2:	return 0x0000;  /* accel */
		case 0:	return 0x0000;  /* neither */
	}
	return 0x0000;
}

static READ16_HANDLER( or_io_acc_steer_r ){
	int data = input_port_1_r( offset );
	int ret = input_port_0_r( offset ) << 8;

	switch(data & 3)
	{
		case 3:	return 0x00 | ret;	/* both */
		case 1:	return 0x00 | ret;  /* brake */
		case 2:	return 0xff | ret;  /* accel */
		case 0:	return 0x00 | ret ;  /* neither */
	}
	return 0x00 | ret;
}
#else
static READ16_HANDLER( or_io_acc_steer_r ){ return (input_port_0_r( offset ) << 8) + input_port_1_r( offset ); }
static READ16_HANDLER( or_io_brake_r ){ return input_port_5_r( offset ) << 8; }
#endif

static int selected_analog;

static READ16_HANDLER( outrun_analog_r )
{
	switch (selected_analog)
	{
		default:
		case 0: return or_io_acc_steer_r(0,0) >> 8;
		case 1: return or_io_acc_steer_r(0,0) & 0xff;
		case 2: return or_io_brake_r(0,0) >> 8;
		case 3: return or_io_brake_r(0,0) & 0xff;
	}
}

static WRITE16_HANDLER( outrun_analog_select_w )
{
	if ( ACCESSING_LSB )
	{
		selected_analog = (data & 0x0c) >> 2;
	}
}

static int or_gear=0;

static READ16_HANDLER( or_io_service_r )
{
	int ret=input_port_2_r( offset );
	int data=input_port_1_r( offset );
	if(data & 4) or_gear=0;
	else if(data & 8) or_gear=1;

	if(or_gear) ret|=0x10;
	else ret&=0xef;

	return ret;
}

static WRITE16_HANDLER( outrun_sound_write_w )
{
	sound_shared_ram[0]=data&0xff;
}

static WRITE16_HANDLER( outrun_ctrl1_w )
{
	if( ACCESSING_LSB ){
		sys16_refreshenable = data & 0x20;
		/* bit 0 always 1? */
		/* bits 2-3 continuously change: 00-01-10-11; this is the same that
		   gets written to 140030 so is probably input related */
	}
}

static WRITE16_HANDLER( outrun_ctrl2_w )
{
	if( ACCESSING_LSB ){
		/* bit 0 always 1? */
		set_led_status(0,data & 0x04);
		set_led_status(1,data & 0x02);	/* brakes */
		coin_counter_w(0,data & 0x10);
	}
}

static MEMORY_READ16_START( outrun_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x060900, 0x060907, sound_shared_ram_r },		/*??? */
	{ 0x060000, 0x067fff, SYS16_MRA16_EXTRAM2 },

	{ 0x100000, 0x10ffff, SYS16_MRA16_TILERAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_TEXTRAM },

	{ 0x130000, 0x130fff, SYS16_MRA16_SPRITERAM },
	{ 0x120000, 0x121fff, SYS16_MRA16_PALETTERAM },

	{ 0x140010, 0x140011, or_io_service_r },
	{ 0x140014, 0x140015, input_port_3_word_r }, /* dip1 */
	{ 0x140016, 0x140017, input_port_4_word_r }, /* dip2 */
	{ 0x140030, 0x140031, outrun_analog_r },

	{ 0x200000, 0x23ffff, SYS16_CPU3ROM16_r },
	{ 0x260000, 0x267fff, shared_ram_r },
	{ 0xe00000, 0xe00001, SYS16_CPU2_RESET_HACK },
MEMORY_END

static MEMORY_WRITE16_START( outrun_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x060900, 0x060907, sound_shared_ram_w },		/*??? */
	{ 0x060000, 0x067fff, SYS16_MWA16_EXTRAM2 },
	{ 0x100000, 0x10ffff, SYS16_MWA16_TILERAM },
	{ 0x110000, 0x110fff, SYS16_MWA16_TEXTRAM },
	{ 0x130000, 0x130fff, SYS16_MWA16_SPRITERAM },
	{ 0x120000, 0x121fff, SYS16_MWA16_PALETTERAM },
	{ 0x140004, 0x140005, outrun_ctrl1_w },
	{ 0x140020, 0x140021, outrun_ctrl2_w },
	{ 0x140030, 0x140031, outrun_analog_select_w },
	{ 0x200000, 0x23ffff, MWA16_ROM },
	{ 0x260000, 0x267fff, shared_ram_w, &shared_ram },
	{ 0xffff06, 0xffff07, outrun_sound_write_w },
MEMORY_END

static MEMORY_READ16_START( outrun_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x060000, 0x067fff, shared_ram_r },
	{ 0x080000, 0x09ffff, SYS16_MRA16_EXTRAM },		/* gr */
MEMORY_END

static MEMORY_WRITE16_START( outrun_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x060000, 0x067fff, shared_ram_w },
	{ 0x080000, 0x09ffff, SYS16_MWA16_EXTRAM },		/* gr */
MEMORY_END

/* Outrun */

static MEMORY_READ_START( outrun_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_r },
	{ 0xf100, 0xf7ff, MRA_NOP },
	{ 0xf800, 0xf807, sound2_shared_ram_r },
	{ 0xf808, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( outrun_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf0ff, SegaPCM_w },
	{ 0xf100, 0xf7ff, MWA_NOP },
	{ 0xf800, 0xf807, sound2_shared_ram_w,&sound_shared_ram },
	{ 0xf808, 0xffff, MWA_RAM },
MEMORY_END

/***************************************************************************/

static void outrun_update_proc( void ){
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static void outrun_init_machine( void ){
	static int bank[8] = {
		7,0,1,2,
		3,4,5,6
	};
	sys16_obj_bank = bank;
	sys16_spritesystem = sys16_sprite_outrun;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;
	sys16_sprxoffset = -0xc0;

/* hack: cpu 0 reset opcode resets cpu 2 */
	sys16_patch_code(0x7d44,0x4a);
	sys16_patch_code(0x7d45,0x79);
	sys16_patch_code(0x7d46,0x00);
	sys16_patch_code(0x7d47,0xe0);
	sys16_patch_code(0x7d48,0x00);
	sys16_patch_code(0x7d49,0x00);

/* *forced sound cmd */
	sys16_patch_code( 0x55ed, 0x00);

/* rogue tile on music selection screen */
/*	sys16_patch_code( 0x38545, 0x80); */

/* *freeze time */
/*	sys16_patch_code( 0xb6b6, 0x4e); */
/*	sys16_patch_code( 0xb6b7, 0x71); */

	sys16_update_proc = outrun_update_proc;

	sys16_gr_ver = &sys16_extraram[0];
	sys16_gr_hor = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0xc00/2;

	sys16_gr_palette= 0xf00 / 2;
	sys16_gr_palette_default = 0x800 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x06 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_gr_second_road = &sys16_extraram[0x8000];
}

static void outruna_init_machine( void ){
	static int bank[8] = {
		7,0,1,2,
		3,4,5,6
	};
	sys16_obj_bank = bank;
	sys16_spritesystem = sys16_sprite_outrun;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

/* cpu 0 reset opcode resets cpu 2 */
	sys16_patch_code(0x7db8,0x4a);
	sys16_patch_code(0x7db9,0x79);
	sys16_patch_code(0x7dba,0x00);
	sys16_patch_code(0x7dbb,0xe0);
	sys16_patch_code(0x7dbc,0x00);
	sys16_patch_code(0x7dbd,0x00);

/* *forced sound cmd */
	sys16_patch_code( 0x5661, 0x00);

/* rogue tile on music selection screen */
/*	sys16_patch_code( 0x38455, 0x80); */

/* *freeze time */
/*	sys16_patch_code( 0xb6b6, 0x4e); */
/*	sys16_patch_code( 0xb6b7, 0x71); */

	sys16_update_proc = outrun_update_proc;

	sys16_gr_ver = &sys16_extraram[0];
	sys16_gr_hor = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0xc00/2;

	sys16_gr_palette= 0xf00 / 2;
	sys16_gr_palette_default = 0x800 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x06 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_gr_second_road = &sys16_extraram[0x10000];
}

static void init_outrun( void )
{
	sys16_onetime_init_machine();
	sys16_interleave_sprite_data( 0x100000 );
	generate_gr_screen(512,2048,0,0,3,0x8000);
}

static void init_outrunb( void )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	int i;

	sys16_onetime_init_machine();
/*
  Main Processor
	Comparing the bootleg with the custom bootleg, it seems that:-

  if even bytes &0x28 == 0x20 or 0x08 then they are xored with 0x28
  if odd bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0

  ie. data lines are switched.
*/

	for( i=0;i<0x40000;i+=2 ){
		data16_t word = RAM[i/2];
		UINT8 even = word>>8;
		UINT8 odd = word&0xff;

		/* even byte */
		if((even&0x28) == 0x20 || (even&0x28) == 0x08) even^=0x28;

		/* odd byte */
		if((odd&0xc0) == 0x80 || (odd&0xc0) == 0x40) odd^=0xc0;

		RAM[i/2] = (even<<8)+odd;
	}

/*
  Second Processor

  if even bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0
  if odd bytes &0x0c == 0x04 or 0x08 then they are xored with 0x0c
*/
	RAM = (data16_t *)memory_region(REGION_CPU3);
	for(i=0;i<0x40000;i+=2)
	{
		data16_t word = RAM[i/2];
		UINT8 even = word>>8;
		UINT8 odd = word&0xff;

		/* even byte */
		if((even&0xc0) == 0x80 || (even&0xc0) == 0x40) even^=0xc0;

		/* odd byte */
		if((odd&0x0c) == 0x08 || (odd&0x0c) == 0x04) odd^=0x0c;

		RAM[i/2] = (even<<8)+odd;
	}
/*
  Road GFX

	rom orun_me.rom
	if bytes &0x60 == 0x40 or 0x20 then they are xored with 0x60

	rom orun_mf.rom
	if bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0

  I don't know why there's 2 road roms, but I'm using orun_me.rom
*/
	{
		UINT8 *mem = memory_region(REGION_GFX3);
		for(i=0;i<0x8000;i++){
			if( (mem[i]&0x60) == 0x20 || (mem[i]&0x60) == 0x40 ) mem[i]^=0x60;
		}
	}

	generate_gr_screen(512,2048,0,0,3,0x8000);
	sys16_interleave_sprite_data( 0x100000 );

/*
  Z80 Code
	rom orun_ma.rom
	if bytes &0x60 == 0x40 or 0x20 then they are xored with 0x60

*/
	{
		UINT8 *mem = memory_region(REGION_CPU2);
		for(i=0;i<0x8000;i++){
			if( (mem[i]&0x60) == 0x20 || (mem[i]&0x60) == 0x40 ) mem[i]^=0x60;
		}
	}
}

/***************************************************************************/

INPUT_PORTS_START( outrun )
PORT_START	/* Steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 100, 3, 0x48, 0xb8 )
/*	PORT_ANALOG( 0xff, 0x7f, IPT_PADDLE , 70, 3, 0x48, 0xb8 ) */

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )

#else

PORT_START	/* Accel / Decel */
	PORT_ANALOG( 0xff, 0x30, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 0x30, 0x90 )

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
/*	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, "Up Cockpit" )
	PORT_DIPSETTING(    0x01, "Mini Up" )
	PORT_DIPSETTING(    0x03, "Moving" )
/*	PORT_DIPSETTING(    0x00, "No Use" ) */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Time" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, "Enemies" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_ANALOG( 0xff, 0x30, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 100, 16, 0x30, 0x90 )

#endif

INPUT_PORTS_END

/***************************************************************************/
static int or_interrupt( void ){
	int intleft=cpu_getiloops();
	if(intleft!=0) return 2;
	else return 4;
}


#define MACHINE_DRIVER_OUTRUN( GAMENAME,INITMACHINE) \
static struct MachineDriver GAMENAME = \
{ \
	{ \
		{ \
			CPU_M68000, \
			12000000, \
			outrun_readmem,outrun_writemem,0,0, \
			or_interrupt,2 \
		}, \
		{ \
			CPU_Z80 | CPU_AUDIO_CPU, \
			4096000, \
			outrun_sound_readmem,outrun_sound_writemem,sound_readport,sound_writeport, \
			ignore_interrupt,1 \
		}, \
		{ \
			CPU_M68000, \
			12000000, \
			outrun_readmem2,outrun_writemem2,0,0, \
			sys16_interrupt,2 \
		}, \
	}, \
	60, 100 /*DEFAULT_60HZ_VBLANK_DURATION*/, \
	4, /* needed to sync processors */ \
	INITMACHINE, \
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 }, \
	sys16_gfxdecodeinfo, \
	4096*ShadowColorsMultiplier,4096*ShadowColorsMultiplier, \
	0, \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_AFTER_VBLANK, \
	0, \
	sys16_outrun_vh_start, \
	sys16_vh_stop, \
	sys16_outrun_vh_screenrefresh, \
	SOUND_SUPPORTS_STEREO,0,0,0, \
	{ \
		{ \
			SOUND_YM2151, \
			&sys16_ym2151_interface \
		}, \
		{ \
			SOUND_SEGAPCM, \
			&sys16_segapcm_interface_15k, \
		} \
	} \
};

MACHINE_DRIVER_OUTRUN(machine_driver_outrun,outrun_init_machine)
MACHINE_DRIVER_OUTRUN(machine_driver_outruna,outruna_init_machine)


static data16_t *shared_ram2;
static READ16_HANDLER( shared_ram2_r ){
	return shared_ram2[offset];
}
static WRITE16_HANDLER( shared_ram2_w ){
	COMBINE_DATA(&shared_ram2[offset]);
}

static MEMORY_READ16_START( shangon_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x20c640, 0x20c647, sound_shared_ram_r },
	{ 0x20c000, 0x20ffff, SYS16_MRA16_EXTRAM2 },
	{ 0x400000, 0x40ffff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x600000, 0x600fff, SYS16_MRA16_SPRITERAM },
	{ 0xa00000, 0xa00fff, SYS16_MRA16_PALETTERAM },
	{ 0xc68000, 0xc68fff, shared_ram_r },
	{ 0xc7c000, 0xc7ffff, shared_ram2_r },
	{ 0xe00002, 0xe00003, sys16_coinctrl_r },
	{ 0xe01000, 0xe01001, input_port_2_word_r }, /* service */
	{ 0xe0100c, 0xe0100d, input_port_4_word_r }, /* dip2 */
	{ 0xe0100a, 0xe0100b, input_port_3_word_r }, /* dip1 */
	{ 0xe030f8, 0xe030f9, ho_io_x_r },
	{ 0xe030fa, 0xe030fb, ho_io_y_r },
MEMORY_END

static MEMORY_WRITE16_START( shangon_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x20c640, 0x20c647, sound_shared_ram_w },
	{ 0x20c000, 0x20ffff, SYS16_MWA16_EXTRAM2 },
	{ 0x400000, 0x40ffff, SYS16_MWA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM },
	{ 0x600000, 0x600fff, SYS16_MWA16_SPRITERAM },
	{ 0xa00000, 0xa00fff, SYS16_MWA16_PALETTERAM },
	{ 0xc68000, 0xc68fff, shared_ram_w, &shared_ram },
	{ 0xc7c000, 0xc7ffff, shared_ram2_w, &shared_ram2 },
	{ 0xe00002, 0xe00003, sys16_3d_coinctrl_w },
MEMORY_END

static MEMORY_READ16_START( shangon_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x454000, 0x45401f, SYS16_MRA16_EXTRAM3 },
	{ 0x7e8000, 0x7e8fff, shared_ram_r },
	{ 0x7fc000, 0x7ffbff, shared_ram2_r },
	{ 0x7ffc00, 0x7fffff, SYS16_MRA16_EXTRAM },
MEMORY_END

static MEMORY_WRITE16_START( shangon_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x454000, 0x45401f, SYS16_MWA16_EXTRAM3 },
	{ 0x7e8000, 0x7e8fff, shared_ram_w },
	{ 0x7fc000, 0x7ffbff, shared_ram2_w },
	{ 0x7ffc00, 0x7fffff, SYS16_MWA16_EXTRAM },
MEMORY_END

static MEMORY_READ_START( shangon_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf7ff, SegaPCM_r },
	{ 0xf800, 0xf807, sound2_shared_ram_r },
	{ 0xf808, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( shangon_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf7ff, SegaPCM_w },
	{ 0xf800, 0xf807, sound2_shared_ram_w,&sound_shared_ram },
	{ 0xf808, 0xffff, MWA_RAM },
MEMORY_END

/***************************************************************************/

static void shangon_update_proc( void ){
	set_bg_page1( sys16_textram[0x74e] );
	set_fg_page1( sys16_textram[0x74f] );
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static void shangon_init_machine( void ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_hangon;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_patch_code( 0x65bd, 0xf9);
	sys16_patch_code( 0x6677, 0xfa);
	sys16_patch_code( 0x66d5, 0xfb);
	sys16_patch_code( 0x9621, 0xfb);

	sys16_update_proc = shangon_update_proc;

	sys16_gr_ver = shared_ram;
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

static void init_shangon( void ){
	sys16_onetime_init_machine();
	generate_gr_screen(512,1024,0,0,4,0x8000);

	sys16_patch_z80code( 0x1087, 0x20);
	sys16_patch_z80code( 0x1088, 0x01);
}

static void init_shangonb( void ){
	sys16_onetime_init_machine();
	generate_gr_screen(512,1024,8,0,4,0x8000);
}
/***************************************************************************/

INPUT_PORTS_START( shangon )
PORT_START	/* Steering */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_CENTER , 100, 3, 0x42, 0xbd )

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )

#else

PORT_START	/* Accel / Decel */
	PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 1, 0xa2 )

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

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
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Play Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 100, 16, 1, 0xa2 )

#endif
INPUT_PORTS_END

/***************************************************************************/
static struct MachineDriver machine_driver_shangon =
{
	{
		{
			CPU_M68000,
			10000000,
			shangon_readmem,shangon_writemem,0,0,
			sys16_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4096000,
			shangon_sound_readmem,shangon_sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,1
		},
		{
			CPU_M68000,
			10000000,
			shangon_readmem2,shangon_writemem2,0,0,
			sys16_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	shangon_init_machine,
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
			SOUND_YM2151,
			&sys16_ym2151_interface
		},
		{
			SOUND_SEGAPCM,
			&sys16_segapcm_interface_15k_512,
		}
	}
};

GAMEX(1992, shangon,  0,        shangon,  shangon,  shangon,  ROT0,         "Sega",    "Super Hang-On", GAME_NOT_WORKING )
GAME( 1992, shangonb, shangon,  shangon,  shangon,  shangonb, ROT0,         "bootleg", "Super Hang-On (bootleg)" )

GAME( 1986, outrun,   0,        outrun,   outrun,   outrun,   ROT0,         "Sega",    "Out Run (set 1)" )
GAME( 1986, outruna,  outrun,   outruna,  outrun,   outrun,   ROT0,         "Sega",    "Out Run (set 2)" )
GAME( 1986, outrunb,  outrun,   outruna,  outrun,   outrunb,  ROT0,         "Sega",    "Out Run (set 3)" )
GAMEX(19??, toutrun,  0,        outrun,   outrun,   outrun,   ROT0,         "Sega", "Turbo Outrun (set 1)", GAME_NOT_WORKING )
GAMEX(19??, toutruna, toutrun,  outrun,   outrun,   outrun,   ROT0,         "Sega", "Turbo Outrun (set 2)", GAME_NOT_WORKING )
