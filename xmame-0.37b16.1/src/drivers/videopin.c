/***************************************************************************

Atari Video Pinball Driver

Done by Sebastien Monassa
Questions, comments, etc to smonassa@mail.dotcom.fr

TODO: In a logical order
	  o Debug variable force plunger and false ball movement
	  o Is nudge effect working ?
      o Sound (hum...)
      o LEDs (possible ?, depends on backdrop for exact coordinates)
	  o High score saving/loading
-	  o Get a good and final Artwork backdrop to adjust display and leds finally

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"

/* vidhrdw/videopin.c */
WRITE_HANDLER( videopin_videoram_w );
extern int videopin_vh_start(void);
extern void videopin_vh_stop(void);
extern void videopin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern struct artwork_info *videopin_backdrop;


/* machine/videopin.c */
extern int videopin_interrupt(void);
WRITE_HANDLER( videopin_out1_w );
WRITE_HANDLER( videopin_out2_w );
WRITE_HANDLER( videopin_led_w );
WRITE_HANDLER( videopin_watchdog_w );
WRITE_HANDLER( videopin_ball_position_w );
WRITE_HANDLER( videopin_note_dvslrd_w );
READ_HANDLER( videopin_in0_r );
READ_HANDLER( videopin_in1_r );
READ_HANDLER( videopin_in2_r );




static MEMORY_READ_START( videopin_readmem )
	{ 0x0000, 0x01ff, MRA_RAM },        /* working   RAM 512  bytes */
	{ 0x0200, 0x07ff, MRA_RAM },        /* playfield RAM 1,5 Kbytes */
	{ 0x0800, 0x0800, videopin_in2_r }, /* VBLANK, NUDGE, PLUNGER1, PLUNGER2 */
	{ 0x1000, 0x1000, videopin_in0_r }, /* IN0 Switches */
	{ 0x1800, 0x1800, videopin_in1_r }, /* IN1 DSW */
	{ 0x2000, 0x3fff, MRA_ROM },        /* PROM */
	{ 0xfff0, 0xffff, MRA_ROM },        /* PROM for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( videopin_writemem )
	{ 0x0000, 0x01ff, MWA_RAM },                  /* working RAM */
	{ 0x0200, 0x07ff, videoram_w, &videoram, &videoram_size },
	                                              /* playfield RAM */
	{ 0x0800, 0x0800, videopin_note_dvslrd_w },   /* No sound yet, audio frequency load (NOTE DVSRLD) */
	{ 0x0801, 0x0801, videopin_led_w },           /* LED write (LED WR) */
	{ 0x0802, 0x0802, watchdog_reset_w },         /* watchdog counter clear (WATCHDOG) */
	{ 0x0804, 0x0804, videopin_ball_position_w }, /* video ball position (BALL POSITION) */
	{ 0x0805, 0x0805, videopin_out1_w },          /* NMI MASK, lockout coil, audio frequency (OUT 1) */
	{ 0x0806, 0x0806, videopin_out2_w },          /* audio disable during attract, bell audio gen enable,
                                                     bong audio gen enable, coin counter,
													 audio volume select (OUT 2) */
	{ 0x2000, 0x3fff, MWA_ROM }, /* PROM */
MEMORY_END



INPUT_PORTS_START( videopin )
	PORT_START		/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )   /* Right coin 1 */
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )   /* Left coin 2 */
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* Right flipper */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* Left flipper */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START1 )  /* No start2 button */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* What the hell is Slam switch ??? */
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )

	PORT_START		/* IN1/DSW */
	PORT_DIPNAME( 0xC0, 0x80, "Coin Mode" )
	PORT_DIPSETTING(	0xC0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x30, 0x00, "Language" )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x10, "German" )
	PORT_DIPSETTING(	0x20, "French" )
	PORT_DIPSETTING(	0x30, "Spanish" )
	PORT_DIPNAME( 0x08, 0x08, "Balls" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPNAME( 0x04, 0x00, "Replay" )
	PORT_DIPSETTING(	0x00, "Special 1 Replay" )
	PORT_DIPSETTING(	0x04, "80000 Points" )
	PORT_DIPNAME( 0x01, 0x01, "Replay Points" )
	PORT_DIPSETTING(	0x00, "3b 180000/5b 300000" )
	PORT_DIPSETTING(	0x01, "3b 210000/5b 350000" )
	PORT_DIPNAME( 0x02, 0x00, "Extra Ball" )
	PORT_DIPSETTING(	0x00, "Extra Ball" )
	PORT_DIPSETTING(	0x02, "50000 Points" )

	PORT_START		/* IN2 VBLANK, NUDGE, PLUNGER1, PLUNGER2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* PLUNGER1 */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* PLUNGER2 */
											  /* Ball shooter force = PLUNGER2 - PLUNGER1 */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) /* NUDGE */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK ) /* VBLANK */

	PORT_START		/* FAKE to read the plunger button */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 ) /* PLUNGER1 */
INPUT_PORTS_END


static struct GfxLayout videopin_charlayout =
{
	8,8,	/* 8*8 characters */
	64, 	/* 64 characters */
	1,		/* 1 bit per pixel */
	{ 0 },	/* no separation in 1 bpp */
/*	{ 56, 48, 40, 32, 24, 16, 8, 0 }, */
	{  4, 5, 6, 7, 0x200*8 + 4, 0x200*8 + 5, 0x200*8 + 6, 0x200*8 + 7},
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxLayout videopin_balllayout =
{
	8,8,	/* 8*8 characters */
	4, 		/* 4 characters */
	1,		/* 1 bit per pixel */
	{ 0 },	/* no separation in 1 bpp */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo videopin_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &videopin_charlayout,  0x00, 0x01 }, /* offset into colors, # of colors */
	{ 1, 0x0400, &videopin_balllayout, 0x00, 0x01 }, /* offset into colors, # of colors */
	{ -1 } /* end of array */
};

static unsigned char videopin_palette[] =
{
	0x00,0x00,0x00, /* BLACK (transparent) */
	0xff,0xff,0xff  /* WHITE */
};


static unsigned short videopin_colortable[] =
{
	0x00, 0x01
};

static void init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
{
	memcpy(game_palette,videopin_palette,sizeof(videopin_palette));
	memcpy(game_colortable,videopin_colortable,sizeof(videopin_colortable));

	/* Get Artwork */
	artwork_load(&videopin_backdrop, "videopin.png", 2, Machine->drv->total_colors - 2);
	if (videopin_backdrop)
	{
		memcpy (game_palette+3*videopin_backdrop->start_pen, videopin_backdrop->orig_palette,
				3*videopin_backdrop->num_pens_used);
	}
}

/*
static struct DACinterface dac_interface =
{
	3,
	{ 100, 100, 100 } // ???
};
*/


static struct MachineDriver machine_driver_videopin =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			/*12096000/8, */
			750000, 	   /* 12000000/16 = 12MHz/16 = 750 KHz not sure about the frequency ??? */
			videopin_readmem,videopin_writemem,0,0,
			videopin_interrupt,8
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	45*8, 39*8, { 0*8, 45*8-1, 0*8, 39*8-1 }, /* 360x312 */
	/*48*8, 32*8, { 0*8, 48*8-1, 0*8, 32*8-1 },    384x256    */
	videopin_gfxdecodeinfo,
	256, 2, /* Game is black & white, extra 254 colors for the backdrop */
	init_palette,

	VIDEO_TYPE_RASTER,
	0,
	videopin_vh_start,
	videopin_vh_stop,
    videopin_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			/*SOUND_DAC, */
			/*&dac_interface */
			0,0 /* No sound yet */
		}
	}
};




/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( videopin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64Kb code roms, the ROMs are nibble-wide */
    ROM_LOAD_NIB_LOW ( "34242-01.e0", 0x2000, 0x0400, 0xc6a83795 ) /*ROM0 */
	ROM_LOAD_NIB_HIGH( "34237-01.k0", 0x2000, 0x0400, 0x9b5ef087 ) /*ROM0 */
	ROM_LOAD_NIB_LOW ( "34243-01.d0", 0x2400, 0x0400, 0xdc87d023 ) /*ROM4 */
	ROM_LOAD_NIB_HIGH( "34238-01.j0", 0x2400, 0x0400, 0x280d9e67 ) /*ROM4 */
	ROM_LOAD_NIB_LOW ( "34250-01.h1", 0x2800, 0x0400, 0x26fdd5a3 ) /*ROM1 */
	ROM_LOAD_NIB_HIGH( "34249-01.h1", 0x2800, 0x0400, 0x923b3609 ) /*ROM1 */
	ROM_LOAD_NIB_LOW ( "34244-01.c0", 0x2c00, 0x0400, 0x4c12a4b1 ) /*ROM5 */
	ROM_LOAD_NIB_HIGH( "34240-01.h0", 0x2c00, 0x0400, 0xd487eff5 ) /*ROM5 */
	ROM_LOAD_NIB_LOW ( "34252-01.e1", 0x3000, 0x0400, 0x4858d87a ) /*ROM2 */
	ROM_LOAD_NIB_HIGH( "34247-01.k1", 0x3000, 0x0400, 0xd3083368 ) /*ROM2 */
	ROM_LOAD_NIB_LOW ( "34246-01.a0", 0x3400, 0x0400, 0x39ff2d49 ) /*ROM6 */
	ROM_LOAD_NIB_HIGH( "34239-01.h0", 0x3400, 0x0400, 0x692de455 ) /*ROM6 */
	ROM_LOAD_NIB_LOW ( "34251-01.f1", 0x3800, 0x0400, 0x5d416efc ) /*ROM3 */
	ROM_LOAD_NIB_HIGH( "34248-01.j1", 0x3800, 0x0400, 0x9f120e95 ) /*ROM3 */
	ROM_LOAD_NIB_LOW ( "34245-01.b0", 0x3c00, 0x0400, 0xda02c194 ) /*ROM7 */
	ROM_RELOAD(                       0xfc00, 0x0400 ) /* for 6502 vectors */
	ROM_LOAD_NIB_HIGH( "34241-01.f0", 0x3c00, 0x0400, 0x5bfb83da ) /*ROM7 */
	ROM_RELOAD(                       0xfc00, 0x0400 ) /* for 6502 vectors */

	ROM_REGION(0x520, REGION_GFX1, ROMREGION_DISPOSE )	  /* 1k for graphics: temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "34258-01.c5", 0x0000, 0x0200, 0x91a5f117 )
	ROM_LOAD( "34259-01.d5", 0x0200, 0x0200, 0x6cd98c06 )
	ROM_LOAD( "34257-01.m1", 0x0400, 0x0020, 0x50245866 ) /* 32bytes 16x16 space for 8x8 ball pix */
	ROM_LOAD( "9402-01.h4",  0x0420, 0x0100, 0xb8094b4c ) /* sync (not used) */
ROM_END


GAMEX( 1979, videopin, 0, videopin, videopin, 0, ROT270, "Atari", "Video Pinball", GAME_NOT_WORKING | GAME_NO_SOUND )
