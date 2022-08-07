#ifndef __AVGDVG__
#define __AVGDVG__

/* vector engine types, passed to vg_init */

#define AVGDVG_MIN          1
#define USE_DVG             1
#define USE_AVG_RBARON      2
#define USE_AVG_BZONE       3
#define USE_AVG             4
#define USE_AVG_TEMPEST     5
#define USE_AVG_MHAVOC      6
#define USE_AVG_ALPHAONE    7
#define USE_AVG_SWARS       8
#define USE_AVG_QUANTUM     9
#define AVGDVG_MAX          10

int avgdvg_done(void);
WRITE_HANDLER( avgdvg_go_w );
WRITE_HANDLER( avgdvg_reset_w );
WRITE16_HANDLER( avgdvg_go_word_w );
WRITE16_HANDLER( avgdvg_reset_word_w );
int avgdvg_init(int vgType);

/* Tempest and Quantum use this capability */
void avg_set_flip_x(int flip);
void avg_set_flip_y(int flip);

/* Apart from the color mentioned below, the vector games will make additional
 * entries for translucency/antialiasing and for backdrop/overlay artwork */

/* ---orig.--- */
/* Black and White vector colors for Asteroids, Lunar Lander, Omega Race */
/*PALETTE_INIT( avg_white ); */
/* Basic 8 rgb vector colors for Tempest, Gravitar, Major Havoc etc. */
/*PALETTE_INIT( avg_multi ); */
/* ---end orig.--- */
/* Black and White vector colors for Asteroids, Lunar Lander, Omega Race */
void avg_init_palette_white (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
/* Monochrome Aqua vector colors for Red Baron */
void avg_init_palette_aqua  (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
/* Red and Green vector colors for Battlezone */
void avg_init_palette_bzone (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
/* Basic 8 rgb vector colors for Tempest, Gravitar, Major Havoc etc. */
void avg_init_palette_multi (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
/* Special case for Star Wars and Empire strikes back */
void avg_init_palette_swars (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
/* Monochrome Aqua vector colors for Asteroids Deluxe */
void avg_init_palette_astdelux  (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

/* Some games use a colorram. This is not handled via the Mame core functions
 * right now, but in src/vidhrdw/avgdvg.c itself. */
WRITE_HANDLER( tempest_colorram_w );
WRITE_HANDLER( mhavoc_colorram_w );
WRITE16_HANDLER( quantum_colorram_w );

/* ---orig.--- */
/*
VIDEO_START( dvg );
VIDEO_START( avg );
VIDEO_START( avg_tempest );
VIDEO_START( avg_mhavoc );
VIDEO_START( avg_alphaone );
VIDEO_START( avg_starwars );
VIDEO_START( avg_quantum );
VIDEO_START( avg_bzone );
VIDEO_START( avg_redbaron );
*/
/* ---end orig.--- */
int dvg_start(void);
int avg_start(void);
int avg_start_tempest(void);
int avg_start_mhavoc(void);
int avg_start_starwars(void);
int avg_start_quantum(void);
int avg_start_bzone(void);
int avg_start_redbaron(void);
void dvg_stop(void);
void avg_stop(void);

#endif
