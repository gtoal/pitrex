/*******************************************************************************

Taito Custom ICs
================

Thanks to Suzuki2go for his videos of Metal Black which made better
emulation of TC0480SCP row and column effects possible.

TODO
----

sizeof() s in the TC0110PCR section are probably unnecessary?

				---

PC080SN
-------
Tilemap generator. Two tilemaps, with gfx data fetched from ROM.
Darius uses 3xPC080SN and has double width tilemaps. (NB: it
has not been verified that Topspeed uses this chip. Possibly
it had a variant with added rowscroll capability.)

Standard memory layout (two 64x64 tilemaps with 8x8 tiles)

0000-3fff BG
4000-41ff BG rowscroll		(only verified to exist on Topspeed)
4200-7fff unknown/unused?
8000-bfff FG	(FG/BG layer order fixed per game; Topspeed has BG on top)
c000-c1ff FG rowscroll		(only verified to exist on Topspeed)
c200-ffff unknown/unused?

Double width memory layout (two 128x64 tilemaps with 8x8 tiles)

0000-7fff BG
8000-ffff FG
(Tile layout is different; tiles and colors are separated:
0x0000-3fff  color / flip words
0x4000-7fff  tile number words)

Control registers

+0x20000 (on from tilemaps)
000-001 BG scroll Y
002-003 FG scroll Y

+0x40000
000-001 BG scroll X
002-003 FG scroll X

+0x50000 control word (written infrequently, only 2 bits used)
	   ---------------x flip screen
	   ----------x----- 0x20 poked here in Topspeed init, followed
	                    by zero (Darius does the same).



TC0080VCO
---------
Combined tilemap and motion object generator. The front tilemap
fetches 3bpp gfx data from ram and only has 8 colors available.
The other tilemaps use ROMs as usual. The same gfx set is used
for both tilemaps and motion objects.

There are two 64x64 tilemaps with 16x16 tiles; the optional
front tilemap is 64x64 with 8x8 tiles.

00000-00fff gfx data for FG0        (lo 2 bits per pixel)
01000-01fff FG0 (64x64)             (two tilenums per word, no color bits)
02000-0bfff chain ram               (sprite tile mapping/colors)
0c000-0dfff BG0 tile numbers (64x64)
0e000-0ffff BG1 tile numbers (64x64)
10000-10fff gfx data for FG0        (hi bit per pixel: Ainferno proves not 4bpp)
11000-11fff unknown / unused ?
12000-1bfff chain ram               (sprite tile mapping/colors)
1c000-1dfff BG0 color / flip bits (64x64)
1e000-1ffff BG1 color / flip bits (64x64)
20000-203ff BG0 rowscroll           (see Dleague title screen *)
20400-207ff spriteram
20800-2080f control registers

[*only used in Dleague AFAIK. Note 0x200 words is not enough for a
64x16 => 0x400 pixel high tilemap. So probably it wraps around and
each rowscroll word affects 2 separate lines. Tacky, but wouldn't be
noticeable unless y zoom more than halved the apparent pixel height
meaning you could see more than half of the total tilemap...]

[There is an oddity with this chip: FG0 areas can be addressed
as chain ram, since the offsets used in spriteram are from the
start of the TC0080VCO address space - not the start of chain ram.
In practice only Dleague seems to do this, for c.10 frames as the
pitcher bowls, and I think it's a coding error. Log it and see.]

[Ainferno and Syvalion are only games using FG0 layer.]

Control registers

000-001 ----xx---------- screen invert
        ------xx-------- unknown (always set)
        ---------------- any others ???

002-003 BG0 scroll X  (0x3ff is the tilemap span)
004-005 BG1 scroll X
006-007 BG0 scroll Y  (0x3ff is the tilemap span)
008-009 BG1 scroll Y
00a-00b unknown (Syvalion - FG0 scroll? - and Recordbr)
00c-00d BG0 zoom (hi byte=X, lo byte=Y *)
00e-00f BG1 zoom (hi byte=X, lo byte=Y *)

[* X zoom normal=0x3f   Y zoom normal=0x7f]

All we know is that as y zoom gets bigger the magnification grows:
this seems to be the only zoom feature actually used in the games.


TC0100SCN
---------
Tilemap generator. The front tilemap fetches gfx data from RAM,
the others use ROMs as usual.

Standard memory layout (three 64x64 tilemaps with 8x8 tiles)

0000-3fff BG0
4000-5fff FG0
6000-6fff gfx data for FG0
7000-7fff unused (probably)
8000-bfff BG1
c000-c3ff BG0 rowscroll (second half unused*)
c400-c7ff BG1 rowscroll (second half unused*)
c800-dfff unused (probably)
e000-e0ff BG0 colscroll [see info below]
e100-ffff unused (probably)

Double width tilemaps memory layout (two 128x64 tilemaps, one 128x32 tilemap)

00000-07fff BG0 (128x64)
08000-0ffff BG1 (128x64)
10000-103ff BG0 rowscroll (second half unused*)
10400-107ff BG1 rowscroll (second half unused*)
10800-108ff BG0 colscroll [evidenced by Warriorb inits from $1634]
10900-10fff unused (probably)
11000-11fff gfx data for FG0
12000-13fff FG0 (128x32)

* Perhaps Taito wanted potential for double height tilemaps on the
  TC0100SCN. The inits state the whole area is "linescroll".

Control registers

000-001 BG0 scroll X
002-003 BG1 scroll X
004-005 FG0 scroll X
006-007 BG0 scroll Y
008-009 BG1 scroll Y
00a-00b FG0 scroll Y
00c-00d ---------------x BG0 disable
        --------------x- BG1 disable
        -------------x-- FG0 disable
        ------------x--- change priority order from BG0-BG1-FG0 to BG1-BG0-FG0
        -----------x---- double width tilemaps + different memory map
                              (cameltru and all the multi-screen games)
        ----------x----- unknown (set in most of the TaitoZ games and Cadash)
00e-00f ---------------x flip screen
        ----------x----- this TC0100SCN is subsidiary [= not the main one]
                              (Multi-screen games only. Could it mean: "write
                               through what is written into main TC0100SCN" ?)
        --x------------- unknown (thunderfox)


Colscroll [standard layout]
=========

The e000-ff area is not divided into two halves, it appears to refer only
to bg0 - the bottommost layer unless bg0/1 are flipped. This would work
for Gunfront, which flips bg layers and has bg0 as clouds on top: the
video shows only the clouds affected.

128 words are available in 0xe0?? area. I think every word scrolls 8
pixels - evidenced in Gunfront. [128 words could scroll 128x8 pixels,
adequate for the double width tilemaps which are available on the
TC0100SCN.]

[The reasoning behind colscroll only affecting bg0 may be that it is
only addressable per column of 8 pixels. This is not very fine, and will
tend to look jagged because you can't individually control each pixel
column. Not a problem if:
(i) you only use steps of 1 up or down between neighbouring columns
(ii) you use rowscroll simultaneously to drown out the jaggedness
(iii) it's the background layer and isn't the most visible thing.]

Growl
-----
This uses column scroll in the boat scene [that's just after you have
disposed of the fat men in fezzes] and in the underground lava cavern
scene.

Boat scene: code from $2eb58 appears to be doing see-saw motion for
water layer under boat. $e08c is highest word written, it oscillates
between fffa and 0005. Going back towards $e000 a middle point is reached
which sticks at zero. By $e000 written values oscillate again.

A total of 80 words are being written to [some below 0xe000, I think those
won't do anything, sloppy coding...]

Cavern scene: code from $3178a moves a sequence of 0s, 1s and 0x1ffs
along. These words equate to 0, +1, -1 so will gently ripple bg 0
up and down adding to the shimmering heat effect.

Ninja Kids
----------
This uses column scroll in the fat flame boss scene [that's the end of
round 2] and in the last round in the final confrontation with Satan scene.

Fat flame boss: code at $8eee moves a sequence of 1s and 0s along. This
is similar to the heat shimmer in Growl cavern scene.

Final boss: code at $a024 moves a sine wave of values 0-4 along. When
you are close to getting him the range of values expands to 0-10.

Gunfront
--------
In demo mode when the boss appears with the clouds, a sequence of 40 words
forming sine wave between 0xffd0 and ffe0 is moved along. Bg0 has been
given priority over bg1 so it's the foreground (clouds) affected.

The 40 words will affect 40 8-pixel columns [rows, as this game is
rotated] i.e. what is visible on screen at any point.

Galmedes
--------
Towards end of first round in empty starfield area, about three big ship
sprites cross the screen (scrolling down with the starfield). 16 starfield
columns [rows, as the game is rotated] scroll across with the ship.
$84fc0 and neighbouring routines poke col scroll area.



TC0280GRD
TC0430GRW
---------
These generate a zooming/rotating tilemap. The TC0280GRD has to be used in
pairs, while the TC0430GRW is a newer, single-chip solution.
Regardless of the hardware differences, the two are functionally identical
except for incxx and incxy, which need to be multiplied by 2 in the TC0280GRD
to bring them to the same scale of the other parameters.

control registers:
000-003 start x
004-005 incxx
006-007 incyx
008-00b start y
00c-00d incxy
00e-00f incyy



TC0360PRI
---------
Priority manager
A higher priority value means higher priority. 0 could mean disable but
I'm not sure. If two inputs have the same priority value, I think the first
one has priority, but I'm not sure of that either.
It seems the chip accepts three inputs from three different sources, and
each one of them can declare to have four different priority levels.

000 unknown. Could it be related to highlight/shadow effects in qcrayon2
    and gunfront?
001 in games with a roz layer, this is the roz palette bank (bottom 6 bits
    affect roz color, top 2 bits affect priority)
002 unknown
003 unknown

004 ----xxxx \       priority level 0 (unused? usually 0, pulirula sets it to F in some places)
    xxxx---- | Input priority level 1 (usually FG0)
005 ----xxxx |   #1  priority level 2 (usually BG0)
    xxxx---- /       priority level 3 (usually BG1)

006 ----xxxx \       priority level 0 (usually sprites with top color bits 00)
    xxxx---- | Input priority level 1 (usually sprites with top color bits 01)
007 ----xxxx |   #2  priority level 2 (usually sprites with top color bits 10)
    xxxx---- /       priority level 3 (usually sprites with top color bits 11)

008 ----xxxx \       priority level 0 (e.g. roz layer if top bits of register 001 are 00)
    xxxx---- | Input priority level 1 (e.g. roz layer if top bits of register 001 are 01)
009 ----xxxx |   #3  priority level 2 (e.g. roz layer if top bits of register 001 are 10)
    xxxx---- /       priority level 3 (e.g. roz layer if top bits of register 001 are 11)

00a unused
00b unused
00c unused
00d unused
00e unused
00f unused



TC0480SCP
---------
Tilemap generator, has four zoomable tilemaps with 16x16 tiles.
It also has a front tilemap with 8x8 tiles which fetches gfx data
from RAM.

BG2 and 3 are "special" layers which have row zoom and source
columnscroll. The selectable layer priority order is a function
of the need to have the "special" layers in particular priority
positions.

Standard memory layout (four 32x32 bg tilemaps, one 64x64 fg tilemap)

0000-0fff BG0
1000-1fff BG1
2000-2fff BG2
3000-3fff BG3
4000-43ff BG0 rowscroll
4400-47ff BG1 rowscroll
4800-4bff BG2 rowscroll
4c00-4fff BG3 rowscroll
5000-53ff BG0 rowscroll low order bytes (see info below)
5400-57ff BG1 rowscroll low order bytes
5800-5bff BG2 rowscroll low order bytes
5c00-5fff BG3 rowscroll low order bytes
6000-63ff BG2 row zoom
6400-67ff BG3 row zoom
6800-6bff BG2 source colscroll
6c00-6fff BG3 source colscroll
7000-bfff unknown/unused?
c000-dfff FG0
e000-ffff gfx data for FG0 (4bpp)

Double width tilemaps memory layout (four 64x32 bg tilemaps, one 64x64 fg tilemap)

0000-1fff BG0
2000-3fff BG1
4000-5fff BG2
6000-7fff BG3
8000-83ff BG0 rowscroll
8400-87ff BG1 rowscroll
8800-8bff BG2 rowscroll
8c00-8fff BG3 rowscroll
9000-93ff BG0 rowscroll low order bytes (used for accuracy with row zoom or layer zoom)
9400-97ff BG1 rowscroll low order bytes [*]
9800-9bff BG2 rowscroll low order bytes
9c00-9fff BG3 rowscroll low order bytes
a000-a3ff BG2 row zoom [+]
a400-a7ff BG3 row zoom
a800-abff BG2 source colscroll
ac00-afff BG3 source colscroll
b000-bfff unknown (Slapshot and Superchs poke in TBA OVER error message in FG0 format)
c000-dfff FG0
e000-ffff gfx data for FG0 (4bpp)

[* Gunbustr suggests that high bytes are irrelevant: it leaves them
all zeroed. Superchs is the only game which uses high bytes that
aren't the low byte of the main rowscroll (Footchmp/Undrfire have
this verified in the code).]

[+ Usual row zoom values are 0 - 0x7f. Gunbustr also uses 0x80-d0
approx. Undrfire keeps to the 0-0x7f range but oddly also uses
the high byte with a mask of 0x3f. Meaning of this high byte is
unknown.]

Bg layers tile word layout

+0x00   %yx..bbbb cccccccc      b=control bits(?) c=color .=unused(?)
+0x02   tilenum
[y=yflip x=xflip b=unknown seen in Metalb]

Control registers

000-001 BG0 x scroll    (layer priority order is definable)
002-003 BG1 x scroll
004-005 BG2 x scroll
006-007 BG3 x scroll
008-009 BG0 y scroll
00a-00b BG1 y scroll
00c-00d BG2 y scroll
00e-00f BG3 y scroll
010-011 BG0 zoom        (high byte = X zoom, low byte = Y zoom,
012-013 BG1 zoom         compression is allowed on Y axis only)
014-015 BG2 zoom
016-017 BG3 zoom
018-019 Text layer x scroll
01a-01b Text layer y scroll
01c-01d Unused (not written)
01e-01f Layer Control register
		x-------	Double width tilemaps (4 bg tilemaps become 64x32, and the
		            memory layout changes). Slapshot changes this on the fly.
		-x------	Flip screen
		--x-----	unknown

				Set in Metalb init by whether a byte in prg ROM $7fffe is zero.
				Subsequently Metalb changes it for some screen layer layouts.
				Footchmp clears it, Hthero sets it [then both leave it alone].
				Deadconx code at $10e2 is interesting, with possible values of:
				0x0, 0x20, 0x40, 0x60 poked in (via ram buffer) to control reg,
				dependent on byte in prg ROM $7fffd and whether screen is flipped.

		---xxx--	BG layer priority order

		...000..	0  1  2  3
		...001..	1  2  3  0  (no evidence of this)
		...010..	2  3  0  1  (no evidence of this)
		...011..	3  0  1  2
		...100..	3  2  1  0
		...101..	2  1  0  3  [Gunbustr attract and Metalb (c) screen]
		...110..	1  0  3  2  (no evidence of this)
		...111..	0  3  2  1

		------x-	BG3 row zoom enable
		-------x	BG2 row zoom enable

020-021 BG0 dx	(provides extra precision to x-scroll, only changed with xscroll)
022-023 BG1 dx
024-025 BG2 dx
026-027 BG3 dx
028-029 BG0 dy	(provides extra precision to y-scroll, only changed with yscroll)
02a-02b BG1 dy
02c-02d BG2 dy
02e-02f BG3 dy

[see code at $1b4a in Slapshot and $xxxxx in Undrfire for evidence of row areas]


TC0110PCR
---------
Interface to palette RAM, and simple tilemap/sprite priority handler. The
priority order seems to be fixed.
The data bus is 16 bits wide.

000  W selects palette RAM address
002 RW read/write palette RAM
004  W unknown, often written to


TC0220IOC
---------
A simple I/O interface with integrated watchdog.
It has four address inputs, which would suggest 16 bytes of addressing space,
but only the first 8 seem to be used.

000 R  IN00-07 (DSA)
000  W watchdog reset
001 R  IN08-15 (DSB)
002 R  IN16-23 (1P)
002  W unknown. Usually written on startup: initialize?
003 R  IN24-31 (2P)
004 RW coin counters and lockout
005  W unknown
006  W unknown
007 R  INB0-7 (coin)


TC0510NIO
---------
Newer version of the I/O chip

000 R  DSWA
000  W watchdog reset
001 R  DSWB
001  W unknown (ssi)
002 R  1P
003 R  2P
003  W unknown (yuyugogo, qzquest and qzchikyu use it a lot)
004 RW coin counters and lockout
005  W unknown
006  W unknown (koshien and pulirula use it a lot)
007 R  coin


TC0640FIO
---------
Newer version of the I/O chip ?


***************************************************************************/

#include "driver.h"
#include "state.h"
#include "taitoic.h"


#define PC080SN_RAM_SIZE 0x10000
#define PC080SN_MAX_CHIPS 2
static int PC080SN_chips;

static data16_t PC080SN_ctrl[PC080SN_MAX_CHIPS][8];

static data16_t *PC080SN_ram[PC080SN_MAX_CHIPS],
				*PC080SN_bg_ram[PC080SN_MAX_CHIPS],
				*PC080SN_fg_ram[PC080SN_MAX_CHIPS],
				*PC080SN_bgscroll_ram[PC080SN_MAX_CHIPS],
				*PC080SN_fgscroll_ram[PC080SN_MAX_CHIPS];

static int PC080SN_bgscrollx[PC080SN_MAX_CHIPS],PC080SN_bgscrolly[PC080SN_MAX_CHIPS],
		PC080SN_fgscrollx[PC080SN_MAX_CHIPS],PC080SN_fgscrolly[PC080SN_MAX_CHIPS];
static struct tilemap *PC080SN_tilemap[PC080SN_MAX_CHIPS][2];
static int PC080SN_bg_gfx[PC080SN_MAX_CHIPS];
static int PC080SN_yinvert,PC080SN_dblwidth;

INLINE void common_get_PC080SN_bg_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	UINT16 code,attr;

	if (!PC080SN_dblwidth)
	{
		code = (ram[2*tile_index + 1] & 0x3fff);
		attr = ram[2*tile_index];
	}
	else
	{
		code = (ram[tile_index + 0x2000] & 0x3fff);
		attr = ram[tile_index];
	}

	SET_TILE_INFO(
			gfxnum,
			code,
			(attr & 0x1ff),
			TILE_FLIPYX((attr & 0xc000) >> 14))
}

INLINE void common_get_PC080SN_fg_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	UINT16 code,attr;

	if (!PC080SN_dblwidth)
	{
		code = (ram[2*tile_index + 1] & 0x3fff);
		attr = ram[2*tile_index];
	}
	else
	{
		code = (ram[tile_index + 0x2000] & 0x3fff);
		attr = ram[tile_index];
	}

	SET_TILE_INFO(
			gfxnum,
			code,
			(attr & 0x1ff),
			TILE_FLIPYX((attr & 0xc000) >> 14))
}

static void PC080SN_get_bg_tile_info_0(int tile_index)
{
	common_get_PC080SN_bg_tile_info(PC080SN_bg_ram[0],PC080SN_bg_gfx[0],tile_index);
}

static void PC080SN_get_fg_tile_info_0(int tile_index)
{
	common_get_PC080SN_fg_tile_info(PC080SN_fg_ram[0],PC080SN_bg_gfx[0],tile_index);
}

static void PC080SN_get_bg_tile_info_1(int tile_index)
{
	common_get_PC080SN_bg_tile_info(PC080SN_bg_ram[1],PC080SN_bg_gfx[1],tile_index);
}

static void PC080SN_get_fg_tile_info_1(int tile_index)
{
	common_get_PC080SN_fg_tile_info(PC080SN_fg_ram[1],PC080SN_bg_gfx[1],tile_index);
}

void (*PC080SN_get_tile_info[PC080SN_MAX_CHIPS][2])(int tile_index) =
{
	{ PC080SN_get_bg_tile_info_0, PC080SN_get_fg_tile_info_0 },
	{ PC080SN_get_bg_tile_info_1, PC080SN_get_fg_tile_info_1 }
};

static void PC080SN_restore_scroll(int chip)
{
	int flip;

	PC080SN_bgscrollx[chip] = -PC080SN_ctrl[chip][0];
	PC080SN_fgscrollx[chip] = -PC080SN_ctrl[chip][1];
	PC080SN_bgscrolly[chip] = -PC080SN_ctrl[chip][2];
	PC080SN_fgscrolly[chip] = -PC080SN_ctrl[chip][3];

	flip = (PC080SN_ctrl[chip][4] & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;
	tilemap_set_flip(PC080SN_tilemap[chip][0],flip);
	tilemap_set_flip(PC080SN_tilemap[chip][1],flip);
}

static void PC080SN_restore_scrl_0(void)
{
	PC080SN_restore_scroll(0);
}

static void PC080SN_restore_scrl_1(void)
{
	PC080SN_restore_scroll(1);
}

void (*PC080SN_restore_scrl[PC080SN_MAX_CHIPS])(void) =
{
	PC080SN_restore_scrl_0, PC080SN_restore_scrl_1
};


int PC080SN_vh_start(int chips,int gfxnum,int x_offset,int y_offset,int y_invert,
				int opaque,int dblwidth)
{
	int i;

	if (chips > PC080SN_MAX_CHIPS) return 1;
	PC080SN_chips = chips;

	PC080SN_yinvert = y_invert;
	PC080SN_dblwidth = dblwidth;

	for (i = 0;i < chips;i++)
	{
		int a,b,xd,yd;

		/* Rainbow Islands *has* to have an opaque back tilemap, or the
		   background color is always black */

		a = TILEMAP_TRANSPARENT;
		b = TILEMAP_TRANSPARENT;

		if (opaque==1)
			a = TILEMAP_OPAQUE;

		if (opaque==2)
			b = TILEMAP_OPAQUE;

		if (!PC080SN_dblwidth)	/* standard tilemaps */
		{
			PC080SN_tilemap[i][0] = tilemap_create(PC080SN_get_tile_info[i][0],tilemap_scan_rows,a,8,8,64,64);
			PC080SN_tilemap[i][1] = tilemap_create(PC080SN_get_tile_info[i][1],tilemap_scan_rows,b,8,8,64,64);
		}
		else	/* double width tilemaps */
		{
			PC080SN_tilemap[i][0] = tilemap_create(PC080SN_get_tile_info[i][0],tilemap_scan_rows,a,8,8,128,64);
			PC080SN_tilemap[i][1] = tilemap_create(PC080SN_get_tile_info[i][1],tilemap_scan_rows,b,8,8,128,64);
		}

		PC080SN_ram[i] = malloc(PC080SN_RAM_SIZE);

		if (!PC080SN_ram[i] || !PC080SN_tilemap[i][0] ||
				!PC080SN_tilemap[i][1])
		{
			PC080SN_vh_stop();
			return 1;
		}

		PC080SN_bg_ram[i]       = PC080SN_ram[i] + 0x0000;
		PC080SN_bgscroll_ram[i] = PC080SN_ram[i] + 0x2000;
		PC080SN_fg_ram[i]       = PC080SN_ram[i] + 0x4000;
		PC080SN_fgscroll_ram[i] = PC080SN_ram[i] + 0x6000;
		memset(PC080SN_ram[i],0,PC080SN_RAM_SIZE);

		{
			char buf[20];	/* we need different labels for every item of save data */
			sprintf(buf,"PC080SN-%01x",i);	/* so we add chip # as a suffix */

			state_save_register_UINT16(buf, 0, "memory", PC080SN_ram[i], PC080SN_RAM_SIZE/2);
			state_save_register_UINT16(strcat(buf,"a"), 0, "registers", PC080SN_ctrl[i], 8);
		}

		state_save_register_func_postload(PC080SN_restore_scrl[i]);

		/* use the given gfx set for bg tiles */
		PC080SN_bg_gfx[i] = gfxnum;

		tilemap_set_transparent_pen(PC080SN_tilemap[i][0],0);
		tilemap_set_transparent_pen(PC080SN_tilemap[i][1],0);

		/* I'm setting optional chip #2 with the same offsets (Topspeed) */
		xd = (i == 0) ? -x_offset : -x_offset;
		yd = (i == 0) ? y_offset : y_offset;

		tilemap_set_scrolldx(PC080SN_tilemap[i][0],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(PC080SN_tilemap[i][0],yd,-yd);
		tilemap_set_scrolldx(PC080SN_tilemap[i][1],-16 + xd,-16 - xd);
		tilemap_set_scrolldy(PC080SN_tilemap[i][1],yd,-yd);

		if (!PC080SN_dblwidth)
		{
			tilemap_set_scroll_rows(PC080SN_tilemap[i][0],512);
			tilemap_set_scroll_rows(PC080SN_tilemap[i][1],512);
		}
	}

	return 0;
}

void PC080SN_vh_stop(void)
{
	int i;

	for (i = 0;i < PC080SN_chips;i++)
	{
		free(PC080SN_ram[i]);
		PC080SN_ram[i] = 0;
	}
}

READ16_HANDLER( PC080SN_word_0_r )
{
	return PC080SN_ram[0][offset];
}

READ16_HANDLER( PC080SN_word_1_r )
{
	return PC080SN_ram[1][offset];
}

static void PC080SN_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	int oldword = PC080SN_ram[chip][offset];

	COMBINE_DATA(&PC080SN_ram[chip][offset]);
	if (oldword != PC080SN_ram[chip][offset])
	{
		if (!PC080SN_dblwidth)
		{
			if (offset < 0x2000)
				tilemap_mark_tile_dirty(PC080SN_tilemap[chip][0],offset / 2);
			else if (offset >= 0x4000 && offset < 0x6000)
				tilemap_mark_tile_dirty(PC080SN_tilemap[chip][1],(offset & 0x1fff) / 2);
		}
		else
		{
			if (offset < 0x4000)
				tilemap_mark_tile_dirty(PC080SN_tilemap[chip][0],(offset & 0x1fff));
			else if (offset >= 0x4000 && offset < 0x8000)
				tilemap_mark_tile_dirty(PC080SN_tilemap[chip][1],(offset & 0x1fff));
		}
	}
}

WRITE16_HANDLER( PC080SN_word_0_w )
{
	PC080SN_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_word_1_w )
{
	PC080SN_word_w(1,offset,data,mem_mask);
}

static void PC080SN_xscroll_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	COMBINE_DATA(&PC080SN_ctrl[chip][offset]);

	data = PC080SN_ctrl[chip][offset];

	switch (offset)
	{
		case 0x00:
			PC080SN_bgscrollx[chip] = -data;
			break;

		case 0x01:
			PC080SN_fgscrollx[chip] = -data;
			break;
	}
}

static void PC080SN_yscroll_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	COMBINE_DATA(&PC080SN_ctrl[chip][offset+2]);

	data = PC080SN_ctrl[chip][offset+2];
	if (PC080SN_yinvert)
		data = -data;

	switch (offset)
	{
		case 0x00:
			PC080SN_bgscrolly[chip] = -data;
			break;

		case 0x01:
			PC080SN_fgscrolly[chip] = -data;
			break;
	}
}

static void PC080SN_ctrl_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	COMBINE_DATA(&PC080SN_ctrl[chip][offset+4]);

	data = PC080SN_ctrl[chip][offset+4];

	switch (offset)
	{
		case 0x00:
		{
			int flip = (data & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;

			tilemap_set_flip(PC080SN_tilemap[chip][0],flip);
			tilemap_set_flip(PC080SN_tilemap[chip][1],flip);
			break;
		}
	}
#if 0
	usrintf_showmessage("PC080SN ctrl = %4x",data);
#endif
}

WRITE16_HANDLER( PC080SN_xscroll_word_0_w )
{
	PC080SN_xscroll_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_xscroll_word_1_w )
{
	PC080SN_xscroll_word_w(1,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_yscroll_word_0_w )
{
	PC080SN_yscroll_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_yscroll_word_1_w )
{
	PC080SN_yscroll_word_w(1,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_ctrl_word_0_w )
{
	PC080SN_ctrl_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( PC080SN_ctrl_word_1_w )
{
	PC080SN_ctrl_word_w(1,offset,data,mem_mask);
}

/* This routine is needed as an override by Jumping, which
   doesn't set proper scroll values for foreground tilemap */

void PC080SN_set_scroll(int chip,int tilemap_num,int scrollx,int scrolly)
{
	tilemap_set_scrollx(PC080SN_tilemap[chip][tilemap_num],0,scrollx);
	tilemap_set_scrolly(PC080SN_tilemap[chip][tilemap_num],0,scrolly);
}

/* This routine is needed as an override by Jumping */

void PC080SN_set_trans_pen(int chip,int tilemap_num,int pen)
{
	tilemap_set_transparent_pen(PC080SN_tilemap[chip][tilemap_num],pen);
}


void PC080SN_tilemap_update(void)
{
	int chip,j;

	for (chip = 0;chip < PC080SN_chips;chip++)
	{
		tilemap_set_scrolly(PC080SN_tilemap[chip][0],0,PC080SN_bgscrolly[chip]);
		tilemap_set_scrolly(PC080SN_tilemap[chip][1],0,PC080SN_fgscrolly[chip]);

		if (!PC080SN_dblwidth)
		{
			for (j = 0;j < 256;j++)
				tilemap_set_scrollx(PC080SN_tilemap[chip][0],
						(j + PC080SN_bgscrolly[chip]) & 0x1ff,
						PC080SN_bgscrollx[chip] - PC080SN_bgscroll_ram[chip][j]);
			for (j = 0;j < 256;j++)
				tilemap_set_scrollx(PC080SN_tilemap[chip][1],
						(j + PC080SN_fgscrolly[chip]) & 0x1ff,
						PC080SN_fgscrollx[chip] - PC080SN_fgscroll_ram[chip][j]);
		}
		else
		{
			tilemap_set_scrollx(PC080SN_tilemap[chip][0],0,PC080SN_bgscrollx[chip]);
			tilemap_set_scrollx(PC080SN_tilemap[chip][1],0,PC080SN_fgscrollx[chip]);
		}

		tilemap_update(PC080SN_tilemap[chip][0]);
		tilemap_update(PC080SN_tilemap[chip][1]);
	}
}

void PC080SN_tilemap_draw(struct osd_bitmap *bitmap,int chip,int layer,int flags,UINT32 priority)
{
	switch (layer)
	{
		case 0:
			tilemap_draw(bitmap,PC080SN_tilemap[chip][0],flags,priority);
			break;
		case 1:
			tilemap_draw(bitmap,PC080SN_tilemap[chip][1],flags,priority);
			break;
	}
}





/***************************************************************************/


#define TC0080VCO_RAM_SIZE 0x21000
#define TC0080VCO_CHAR_RAM_SIZE 0x2000
#define TC0080VCO_TOTAL_CHARS 256

static data16_t *TC0080VCO_ram,
				*TC0080VCO_bg0_ram_0, *TC0080VCO_bg0_ram_1,
				*TC0080VCO_bg1_ram_0, *TC0080VCO_bg1_ram_1,
				*TC0080VCO_tx_ram_0,  *TC0080VCO_tx_ram_1,
				*TC0080VCO_char_ram,  *TC0080VCO_bgscroll_ram;

/* This sprite related stuff still needs to be accessed in
   vidhrdw/taito_h */
data16_t *TC0080VCO_chain_ram_0, *TC0080VCO_chain_ram_1,
				*TC0080VCO_spriteram, *TC0080VCO_scroll_ram;

static data16_t TC0080VCO_bg0_scrollx,TC0080VCO_bg0_scrolly,
		TC0080VCO_bg1_scrollx,TC0080VCO_bg1_scrolly;

static struct tilemap *TC0080VCO_tilemap[3];

static UINT8 *TC0080VCO_char_dirty;
static int TC0080VCO_chars_dirty;
static int TC0080VCO_bg_gfx,TC0080VCO_tx_gfx;
static int TC0080VCO_bg_xoffs,TC0080VCO_bg_yoffs;
static int TC0080VCO_bg_flip_yoffs;

int TC0080VCO_flipscreen = 0,TC0080VCO_has_tx;


#if 0
static int TC0080VCO_zoomy_conv_table[] =
{
/* 		These are hand-tuned values... 		*/
/*    +0   +1   +2   +3   +4   +5   +6   +7    +8   +9   +a   +b   +c   +d   +e   +f */
	0x00,0x01,0x01,0x02,0x02,0x03,0x04,0x05, 0x06,0x06,0x07,0x08,0x09,0x0a,0x0a,0x0b,	/* 0x00 */
	0x0b,0x0c,0x0c,0x0d,0x0e,0x0e,0x0f,0x10, 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x16,
	0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e, 0x1f,0x20,0x21,0x22,0x24,0x25,0x26,0x27,
	0x28,0x2a,0x2b,0x2c,0x2e,0x30,0x31,0x32, 0x34,0x36,0x37,0x38,0x3a,0x3c,0x3e,0x3f,

	0x40,0x41,0x42,0x42,0x43,0x43,0x44,0x44, 0x45,0x45,0x46,0x46,0x47,0x47,0x48,0x49,	/* 0x40 */
	0x4a,0x4a,0x4b,0x4b,0x4c,0x4d,0x4e,0x4f, 0x4f,0x50,0x51,0x51,0x52,0x53,0x54,0x55,
	0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d, 0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x66,
	0x67,0x68,0x6a,0x6b,0x6c,0x6e,0x6f,0x71, 0x72,0x74,0x76,0x78,0x80,0x7b,0x7d,0x7f
};
#endif


static void TC0080VCO_get_bg0_tile_info_0(int tile_index)
{
	int color, tile;

	color = TC0080VCO_bg0_ram_1[ tile_index ] & 0x001f;
	tile  = TC0080VCO_bg0_ram_0[ tile_index ] & 0x7fff;

	tile_info.priority = 0;

	SET_TILE_INFO(
			TC0080VCO_bg_gfx,
			tile,
			color,
			TILE_FLIPYX((TC0080VCO_bg0_ram_1[tile_index] & 0x00c0) >> 6))
}

static void TC0080VCO_get_bg1_tile_info_0(int tile_index)
{
	int color, tile;

	color = TC0080VCO_bg1_ram_1[ tile_index ] & 0x001f;
	tile  = TC0080VCO_bg1_ram_0[ tile_index ] & 0x7fff;

	tile_info.priority = 0;

	SET_TILE_INFO(
			TC0080VCO_bg_gfx,
			tile,
			color,
			TILE_FLIPYX((TC0080VCO_bg1_ram_1[tile_index] & 0x00c0) >> 6))
}

static void TC0080VCO_get_tx_tile_info(int tile_index)
{
	int tile;

	if (!TC0080VCO_flipscreen)
	{
		if ( (tile_index & 1) )
			tile = (TC0080VCO_tx_ram_0[tile_index >> 1] & 0x00ff);
		else
			tile = (TC0080VCO_tx_ram_0[tile_index >> 1] & 0xff00) >> 8;
		tile_info.priority = 0;
	}
	else
	{
		if ( (tile_index & 1) )
			tile = (TC0080VCO_tx_ram_0[tile_index >> 1] & 0xff00) >> 8;
		else
			tile = (TC0080VCO_tx_ram_0[tile_index >> 1] & 0x00ff);
		tile_info.priority = 0;
	}

	SET_TILE_INFO(
			TC0080VCO_tx_gfx,
			tile,
			0x40,
			0)		/* 0x20<<1 as 3bpp */
}


/* Is this endian-correct ??? */

static struct GfxLayout TC0080VCO_charlayout =
{
	8, 8,	/* 8x8 pixels */
	256,	/* 256 chars */

/* can't be 4bpp as it becomes opaque in Ainferno... */
/*	4,		   4 bits per pixel    */
/*#ifdef LSB_FIRST */
/*	{ 0x10000*8 + 8, 0x10000*8, 8, 0 }, */
/*#else */
/*	{ 0x10000*8, 0x10000*8 + 8, 0, 8 }, */
/*#endif */

	3,		/* 3 bits per pixel */
#ifdef LSB_FIRST
	{ 0x10000*8, 8, 0 },
#else
	{ 0x10000*8 + 8, 0, 8 },
#endif
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7 },
	16*8
};


void TC0080VCO_set_layer_ptrs(void)
{
	TC0080VCO_char_ram      = TC0080VCO_ram + 0x00000/2;	/* continues at +0x10000 */
	TC0080VCO_tx_ram_0      = TC0080VCO_ram + 0x01000/2;
	TC0080VCO_chain_ram_0   = TC0080VCO_ram + 0x00000/2;	/* only used from +0x2000 */

	TC0080VCO_bg0_ram_0     = TC0080VCO_ram + 0x0c000/2;
	TC0080VCO_bg1_ram_0     = TC0080VCO_ram + 0x0e000/2;

	TC0080VCO_tx_ram_1      = TC0080VCO_ram + 0x11000/2;
	TC0080VCO_chain_ram_1   = TC0080VCO_ram + 0x10000/2;	/* only used from +0x12000 */

	TC0080VCO_bg0_ram_1     = TC0080VCO_ram + 0x1c000/2;
	TC0080VCO_bg1_ram_1     = TC0080VCO_ram + 0x1e000/2;
	TC0080VCO_bgscroll_ram  = TC0080VCO_ram + 0x20000/2;
	TC0080VCO_spriteram     = TC0080VCO_ram + 0x20400/2;
	TC0080VCO_scroll_ram    = TC0080VCO_ram + 0x20800/2;
}

void TC0080VCO_dirty_chars(void)
{
	memset(TC0080VCO_char_dirty,1,TC0080VCO_TOTAL_CHARS);
	TC0080VCO_chars_dirty = 1;
}

void TC0080VCO_dirty_tilemaps(void)
{
	tilemap_mark_all_tiles_dirty(TC0080VCO_tilemap[0]);
	tilemap_mark_all_tiles_dirty(TC0080VCO_tilemap[1]);
	tilemap_mark_all_tiles_dirty(TC0080VCO_tilemap[2]);
}

void TC0080VCO_restore_scroll(void)
{
	TC0080VCO_flipscreen = TC0080VCO_scroll_ram[0] & 0x0c00;

	tilemap_set_flip( TC0080VCO_tilemap[0], TC0080VCO_flipscreen ? TILEMAP_FLIPX | TILEMAP_FLIPY : 0 );
	tilemap_set_flip( TC0080VCO_tilemap[1], TC0080VCO_flipscreen ? TILEMAP_FLIPX | TILEMAP_FLIPY : 0 );
	tilemap_set_flip( TC0080VCO_tilemap[2], TC0080VCO_flipscreen ? TILEMAP_FLIPX | TILEMAP_FLIPY : 0 );

	TC0080VCO_bg0_scrollx = TC0080VCO_scroll_ram[1] &0x03ff;
	TC0080VCO_bg1_scrollx = TC0080VCO_scroll_ram[2] &0x03ff;
	TC0080VCO_bg0_scrolly = TC0080VCO_scroll_ram[3] &0x03ff;
	TC0080VCO_bg1_scrolly = TC0080VCO_scroll_ram[4] &0x03ff;
}


int TC0080VCO_vh_start(int gfxnum,int has_fg0,int bg_xoffs,int bg_yoffs,int bg_flip_yoffs)
{
	int gfx_index=0;

	TC0080VCO_bg_xoffs = bg_xoffs;	/* usually 1 */
	TC0080VCO_bg_yoffs = bg_yoffs;	/* usually 1 */
	TC0080VCO_bg_flip_yoffs = bg_flip_yoffs;	/* usually -2 */
	TC0080VCO_has_tx = has_fg0;	/* for debugging only */

	TC0080VCO_tilemap[0] = tilemap_create(TC0080VCO_get_bg0_tile_info_0,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64);
	TC0080VCO_tilemap[1] = tilemap_create(TC0080VCO_get_bg1_tile_info_0,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64);
	TC0080VCO_ram = malloc(TC0080VCO_RAM_SIZE);

	if ( !TC0080VCO_ram || !TC0080VCO_tilemap[0] || !TC0080VCO_tilemap[1])
	{
		TC0080VCO_vh_stop();
		return 1;
	}

	memset( TC0080VCO_ram,0,TC0080VCO_RAM_SIZE );
	TC0080VCO_set_layer_ptrs();

	/* use the given gfx set for bg tiles*/
	TC0080VCO_bg_gfx = gfxnum;

	tilemap_set_transparent_pen( TC0080VCO_tilemap[0],0 );
	tilemap_set_transparent_pen( TC0080VCO_tilemap[1],0 );

	tilemap_set_scrolldx(TC0080VCO_tilemap[0],TC0080VCO_bg_xoffs,512);
	tilemap_set_scrolldx(TC0080VCO_tilemap[1],TC0080VCO_bg_xoffs,512);
	tilemap_set_scrolldy(TC0080VCO_tilemap[0],TC0080VCO_bg_yoffs,TC0080VCO_bg_flip_yoffs);
	tilemap_set_scrolldy(TC0080VCO_tilemap[1],TC0080VCO_bg_yoffs,TC0080VCO_bg_flip_yoffs);

	state_save_register_UINT16("TC0080VCOa", 0, "memory", TC0080VCO_ram, TC0080VCO_RAM_SIZE/2);
	state_save_register_int   ("TC0080VCOb", 0, "registers", &TC0080VCO_has_tx);
	state_save_register_func_postload(TC0080VCO_set_layer_ptrs);

	/* Perform extra initialisations for text layer */
	{
		TC0080VCO_tilemap[2] = tilemap_create(TC0080VCO_get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
		TC0080VCO_char_dirty = malloc(TC0080VCO_TOTAL_CHARS);

		if (!TC0080VCO_char_dirty || !TC0080VCO_tilemap[2])
		{
			TC0080VCO_vh_stop();
			return 1;
		}

		TC0080VCO_dirty_chars();
		state_save_register_func_postload(TC0080VCO_dirty_chars);

	 	/* find first empty slot to decode gfx */
		for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
			if (Machine->gfx[gfx_index] == 0)
				break;
		if (gfx_index == MAX_GFX_ELEMENTS)
		{
			TC0080VCO_vh_stop();
			return 1;
		}

		/* create the char set (gfx will then be updated dynamically from RAM) */
		Machine->gfx[gfx_index] = decodegfx((UINT8 *)TC0080VCO_char_ram,&TC0080VCO_charlayout);
		if (!Machine->gfx[gfx_index])
			return 1;

		/* set the color information */
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = 64;	/* is this correct ? */
		TC0080VCO_tx_gfx = gfx_index;

		tilemap_set_scrolldx(TC0080VCO_tilemap[2],0,0);
		tilemap_set_scrolldy(TC0080VCO_tilemap[2],48,-448);

		tilemap_set_transparent_pen( TC0080VCO_tilemap[2],0 );
	}

	state_save_register_func_postload(TC0080VCO_dirty_tilemaps);	/* unnecessary ? */
	state_save_register_func_postload(TC0080VCO_restore_scroll);

	/* bg0 tilemap scrollable per pixel row */
	tilemap_set_scroll_rows(TC0080VCO_tilemap[0],512);

	return 0;
}

void TC0080VCO_vh_stop(void)
{
	free( TC0080VCO_ram );
	TC0080VCO_ram = 0;

	free( TC0080VCO_char_dirty );
	TC0080VCO_char_dirty = 0;

	return;
}


static WRITE16_HANDLER( TC0080VCO_scrollram_w )
{
	switch ( offset )
	{
		case 0x00:			/* screen invert control */
			TC0080VCO_restore_scroll();
			break;

		case 0x01:			/* BG0 scroll X */
			TC0080VCO_bg0_scrollx = data &0x03ff;
			break;

		case 0x02:			/* BG1 scroll X */
			TC0080VCO_bg1_scrollx = data &0x03ff;
			break;

		case 0x03:			/* BG0 scroll Y */
			TC0080VCO_bg0_scrolly = data &0x03ff;
			break;

		case 0x04:			/* BG1 scroll Y */
			TC0080VCO_bg1_scrolly = data &0x03ff;
			break;

		default:
			break;
	}
}

READ16_HANDLER( TC0080VCO_word_r )
{
	return TC0080VCO_ram[offset];
}

WRITE16_HANDLER( TC0080VCO_word_w )
{
	int oldword = TC0080VCO_ram[offset];
	COMBINE_DATA(&TC0080VCO_ram[offset]);

	/* A lot of TC0080VCO writes require no action... */

	if (oldword != TC0080VCO_ram[offset])
	{
		if (offset < 0x1000/2)
		{
			TC0080VCO_char_dirty[offset / 8] = 1;
			TC0080VCO_chars_dirty = 1;
#if 0
			if (!TC0080VCO_has_tx)
			{
				if (TC0080VCO_ram[offset])
				usrintf_showmessage_secs(7,"Write non-zero to TC0080VCO char ram\nPlease report to MAMEDEV");
			}
#endif
		}
		else if (offset < 0x2000/2)	/* fg0 (text layer) */
		{
			tilemap_mark_tile_dirty( TC0080VCO_tilemap[2],(offset &0x07ff) * 2 );
			tilemap_mark_tile_dirty( TC0080VCO_tilemap[2],(offset &0x07ff) * 2 + 1 );
#if 0
			if (!TC0080VCO_has_tx)
			{
				if (TC0080VCO_ram[offset])
				usrintf_showmessage_secs(7,"Write non-zero to TC0080VCO fg0\nPlease report to MAMEDEV");
			}
#endif
		}
		else if (offset < 0xc000/2)	/* chain ram */
		{}
		else if (offset < 0xe000/2)	/* bg0 (0) */
			tilemap_mark_tile_dirty(TC0080VCO_tilemap[0],(offset & 0xfff));

		else if (offset < 0x10000/2)	/* bg1 (0) */
			tilemap_mark_tile_dirty(TC0080VCO_tilemap[1],(offset & 0xfff));

		else if (offset < 0x11000/2)
		{
			TC0080VCO_char_dirty[(offset - 0x10000/2) / 8] = 1;
			TC0080VCO_chars_dirty = 1;
#if 0
			if (!TC0080VCO_has_tx)
			{
				if (TC0080VCO_ram[offset])
				usrintf_showmessage_secs(7,"Write non-zero to TC0080VCO char-hi ram\nPlease report to MAMEDEV");
			}
#endif
		}
		else if (offset < 0x12000/2)	/* unknown/unused */
		{
#if 1
			if (TC0080VCO_ram[offset])
			usrintf_showmessage_secs(7,"Write non-zero to mystery TC0080VCO area\nPlease report to MAMEDEV");
#endif
		}
		else if (offset < 0x1c000/2)	/* chain ram */
		{}
		else if (offset < 0x1e000/2)	/* bg0 (1) */
			tilemap_mark_tile_dirty(TC0080VCO_tilemap[0],(offset & 0xfff));

		else if (offset < 0x20000/2)	/* bg1 (1) */
			tilemap_mark_tile_dirty(TC0080VCO_tilemap[1],(offset & 0xfff));

		else if (offset < 0x20400/2)	/* bg0 rowscroll */
		{}
		else if (offset < 0x20800/2)	/* sprite ram */
		{}
		else if (offset < 0x20fff/2)
			TC0080VCO_scrollram_w(offset-(0x20800/2),TC0080VCO_ram[offset],mem_mask);
	}
}


void TC0080VCO_tilemap_update(void)
{
	int j;

	if (!TC0080VCO_flipscreen)
	{
		for (j = 0;j < 0x400;j++)
			tilemap_set_scrollx(TC0080VCO_tilemap[0],(j+0) & 0x3ff,
				-TC0080VCO_bg0_scrollx - TC0080VCO_bgscroll_ram[j &0x1ff]);
	}
	else
	{
		for (j = 0;j < 0x400;j++)
			tilemap_set_scrollx(TC0080VCO_tilemap[0],(j+0) & 0x3ff,
				-TC0080VCO_bg0_scrollx + TC0080VCO_bgscroll_ram[j &0x1ff]);
	}

	tilemap_set_scrolly(TC0080VCO_tilemap[0],0, TC0080VCO_bg0_scrolly);
	tilemap_set_scrollx(TC0080VCO_tilemap[1],0,-TC0080VCO_bg1_scrollx);
	tilemap_set_scrolly(TC0080VCO_tilemap[1],0, TC0080VCO_bg1_scrolly);
	tilemap_set_scrollx(TC0080VCO_tilemap[2],0,0);	/* no scroll (maybe) */
	tilemap_set_scrolly(TC0080VCO_tilemap[2],0,0);

	/* Decode any characters that have changed */

	if (TC0080VCO_chars_dirty)
	{
		int tile_index;

		for (tile_index = 0;tile_index < 64*64;tile_index++)
		{
			int attr = TC0080VCO_tx_ram_0[tile_index >> 1];

			/* should this be reversed in flipscreen ?? */
			if (tile_index & 1)	/* each word has 2 chars */
			{
				attr = (attr &0xff);
			}
			else
			{
				attr = (attr &0xff00) >> 8;
			}

			if (TC0080VCO_char_dirty[attr])
				tilemap_mark_tile_dirty(TC0080VCO_tilemap[2],tile_index);
		}

		for (j = 0;j < TC0080VCO_TOTAL_CHARS;j++)
		{
			if (TC0080VCO_char_dirty[j])
				decodechar(Machine->gfx[TC0080VCO_tx_gfx],j,
					(UINT8 *)TC0080VCO_char_ram,&TC0080VCO_charlayout);
			TC0080VCO_char_dirty[j] = 0;
		}
		TC0080VCO_chars_dirty = 0;
	}

	tilemap_update(TC0080VCO_tilemap[0]);
	tilemap_update(TC0080VCO_tilemap[1]);
	tilemap_update(TC0080VCO_tilemap[2]);
}


/* NB: orientation_flipx code in following routine has not been tested */

static void TC0080VCO_bg0_tilemap_draw(struct osd_bitmap *bitmap,int flags,UINT32 priority)
{
	UINT16 zoom = TC0080VCO_scroll_ram[6];
	int zx, zy;

	zx = (zoom & 0xff00) >> 8;
	zy =  zoom & 0x00ff;

	if (zx == 0x3f && zy == 0x7f)		/* normal size */
	{
		tilemap_draw(bitmap,TC0080VCO_tilemap[0],flags,priority);
	}
	else		/* zoom + rowscroll = custom draw routine */
	{
		UINT8  *dst8, *src8;
		UINT16 *dst16,*src16;
/*		UINT32 *dst32,*src32;		   in future add 24/32 bit color support    */
		UINT8  scanline8[512];
		UINT16 scanline16[512];

		int sx,zoomx,zoomy;
		int dx,ex,dy,ey;
		int y,y_index,src_y_index,row_index;
		int x_index,x_step,x_max;

		int flip = TC0080VCO_flipscreen;
		int rot=Machine->orientation;
		int machine_flip = 0;	/* for  ROT 180 ? */

		int min_x = Machine->visible_area.min_x;
		int max_x = Machine->visible_area.max_x;
		int min_y = Machine->visible_area.min_y;
		int max_y = Machine->visible_area.max_y;
		int screen_width = max_x - min_x + 1;
		int width_mask=0x3ff;	/* underlying tilemap */

		struct osd_bitmap *srcbitmap = tilemap_get_pixmap(TC0080VCO_tilemap[0]);

#if 0
{
	char buf[100];
	sprintf(buf,"xmin= %04x xmax= %04x ymin= %04x ymax= %04x",min_x,max_x,min_y,max_y);
	usrintf_showmessage(buf);
}
#endif

		if (zx < 63)
		{
			/* no evidence for these calcs? */
			dx = 16 - (zx + 2) / 8;
			ex = (zx + 2) % 8;
			zoomx = ((dx << 3) - ex) << 10;
		}
		else
		{
			/* 256 is speculative, haven't found a game using zoomx yet */
			zoomx = 0x10000 - ((zx - 0x3f) * 256);
		}

		if (zy < 127)
		{
			/* no evidence for these calcs? */
			dy = 16 - (zy + 2) / 16;
			ey = (zy + 2) % 16;
			zoomy = ((dy << 4) - ey) << 9;
		}
		else
		{
			/* confirmed with real board */
			zoomy = 0x10000 - ((zy - 0x7f) * 512);
		}

		if (!flip)
		{
			sx =       (-TC0080VCO_scroll_ram[1] - 1) << 16;
			y_index = (( TC0080VCO_scroll_ram[3] - 1) << 16) + min_y * zoomy;
		}
		else
		{
			/* adjustment for zx is entirely speculative */
			sx =  (( 0x200 + TC0080VCO_scroll_ram[1]) << 16)
					- (max_x + min_x) * (zoomx-0x10000);

			/* 0x130 correct for Dleague. Syvalion correct with 0x1f0.
			   min_y is 0x20 and 0x30; max_y is 0x10f and 0x1bf;
			   max_y + min_y seems a good bet... */

			y_index = ((-TC0080VCO_scroll_ram[3] - 2) << 16)
					+ min_y * zoomy - (max_y + min_y) * (zoomy-0x10000);
		}

		if (!machine_flip) y = min_y; else y = max_y;

		if (Machine->scrbitmap->depth == 8)
		{
			do
			{
				src_y_index = (y_index>>16) &0x3ff;	/* tilemaps are 1024 px up/down */

				/* row areas are the same in flipscreen, so we must read in reverse */
				row_index = (y_index >> 16) &0x1ff;
				if (flip)	row_index = 0x1ff - row_index;

				if ((rot &ORIENTATION_FLIP_X)==0)
				{
					x_index = sx - ((TC0080VCO_bgscroll_ram[row_index] << 16));
				}
				else	/* Orientation flip X */
				{
					x_index = sx + ((TC0080VCO_bgscroll_ram[row_index] << 16));
				}

				src8 = (UINT8 *)srcbitmap->line[src_y_index];
				dst8 = scanline8;

				x_step = zoomx;

				x_max = x_index + screen_width * x_step;

				while (x_index<x_max)
				{
					*dst8++ = src8[(x_index >> 16) &width_mask];
					x_index += x_step;
				}

				if ((rot &ORIENTATION_FLIP_X)!=0)
					pdraw_scanline8(bitmap,512-(screen_width/2),y,screen_width,
						scanline8,0,palette_transparent_pen,rot,priority);
				else
					pdraw_scanline8(bitmap,0,y,screen_width,
						scanline8,0,palette_transparent_pen,rot,priority);

				y_index += zoomy;
				if (!machine_flip) y++; else y--;
			}
			while ( (!machine_flip && y <= max_y) || (machine_flip && y >= min_y) );
		}
		else if (Machine->scrbitmap->depth == 16)
		{
			do
			{
				src_y_index = (y_index>>16) &0x3ff;	/* tilemaps are 1024 px up/down */

				/* row areas are the same in flipscreen, so we must read in reverse */
				row_index = (src_y_index &0x1ff);
				if (flip)	row_index = 0x1ff - row_index;

				if ((rot &ORIENTATION_FLIP_X)==0)
				{
					x_index = sx - ((TC0080VCO_bgscroll_ram[row_index] << 16));
				}
				else	/* Orientation flip X */
				{
					x_index = sx + ((TC0080VCO_bgscroll_ram[row_index] << 16));
				}

				src16 = (UINT16 *)srcbitmap->line[src_y_index];
				dst16 = scanline16;

				x_step = zoomx;

				x_max = x_index + screen_width * x_step;

				while (x_index<x_max)
				{
					*dst16++ = src16[(x_index >> 16) &width_mask];
					x_index += x_step;
				}

				if ((rot &ORIENTATION_FLIP_X)!=0)
					pdraw_scanline16(bitmap,512-(screen_width/2),y,screen_width,
						scanline16,0,palette_transparent_pen,rot,priority);
				else
					pdraw_scanline16(bitmap,0,y,screen_width,
						scanline16,0,palette_transparent_pen,rot,priority);

				y_index += zoomy;
				if (!machine_flip) y++; else y--;
			}
			while ( (!machine_flip && y <= max_y) || (machine_flip && y >= min_y) );
		}
	}
}


static void TC0080VCO_bg1_tilemap_draw(struct osd_bitmap *bitmap,int flags,UINT32 priority)
{
	UINT8 layer=1;
	UINT16 zoom = TC0080VCO_scroll_ram[6+layer];
	int min_x = Machine->visible_area.min_x;
	int max_x = Machine->visible_area.max_x;
	int min_y = Machine->visible_area.min_y;
	int max_y = Machine->visible_area.max_y;
	int zoomx, zoomy;

	zoomx = (zoom & 0xff00) >> 8;
	zoomy =  zoom & 0x00ff;

	if (zoomx == 0x3f && zoomy == 0x7f)		/* normal size */
	{
		tilemap_draw(bitmap,TC0080VCO_tilemap[layer],flags,priority);
	}
	else		/* zoomed */
	{
		int zx, zy, dx, dy, ex, ey;
		int sx,sy;

		/* shouldn't we set no_clip before doing this (see TC0480SCP) ? */
		struct osd_bitmap *srcbitmap = tilemap_get_pixmap(TC0080VCO_tilemap[layer]);

		if (zoomx < 63)
		{
			/* no evidence for these calcs? */
			dx = 16 - (zoomx + 2) / 8;
			ex = (zoomx + 2) % 8;
			zx = ((dx << 3) - ex) << 10;
		}
		else
		{
			/* 256 is speculative, haven't found a game using zoomx yet */
			zx = 0x10000 - ((zoomx - 0x3f) * 256);
		}

		if (zoomy < 127)
		{
			/* no evidence for these calcs? */
			dy = 16 - (zoomy + 2) / 16;
			ey = (zoomy + 2) % 16;
			zy = ((dy << 4) - ey) << 9;
		}
		else
		{
			/* confirmed with real board */
			zy = 0x10000 - ((zoomy - 0x7f) * 512);
		}

		if (!TC0080VCO_flipscreen)
		{
			sx = (-TC0080VCO_scroll_ram[layer+1] - 1) << 16;
			sy = ( TC0080VCO_scroll_ram[layer+3] - 1) << 16;
		}
		else
		{
			/* adjustment for zx is entirely speculative */
			sx =  (( 0x200 + TC0080VCO_scroll_ram[layer+1]) << 16)
					- (max_x + min_x) * (zx-0x10000);

			sy =  (( 0x3fe - TC0080VCO_scroll_ram[layer+3]) << 16)
					- (max_y + min_y) * (zy-0x10000);
		}

		copyrozbitmap(bitmap,srcbitmap,
			sx, sy,
			zx, 0, 0, zy,
			0,					/* why no wraparound ?? */
			&Machine->visible_area,
			TRANSPARENCY_COLOR, 0, priority);
	}
}


void TC0080VCO_tilemap_draw(struct osd_bitmap *bitmap,int layer,int flags,UINT32 priority)
{
	int disable = 0x00;	/* possibly layer disable bits do exist ?? */

#if 0
	usrintf_showmessage("layer disable = %x",disable);
#endif

	switch (layer)
	{
		case 0:
			if (disable & 0x01) return;
			TC0080VCO_bg0_tilemap_draw(bitmap,flags,priority);
			break;
		case 1:
			if (disable & 0x02) return;
			TC0080VCO_bg1_tilemap_draw(bitmap,flags,priority);
			break;
		case 2:
			if (disable & 0x04) return;
			tilemap_draw(bitmap,TC0080VCO_tilemap[2],flags,priority);
			break;
	}
}



/***************************************************************************/


#define TC0100SCN_RAM_SIZE 0x14000	/* enough for double-width tilemaps */
#define TC0100SCN_TOTAL_CHARS 256
#define TC0100SCN_MAX_CHIPS 3
static int TC0100SCN_chips;
static struct rectangle myclip;

static data16_t TC0100SCN_ctrl[TC0100SCN_MAX_CHIPS][8];

static data16_t *TC0100SCN_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_bg_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_fg_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_tx_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_char_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_bgscroll_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_fgscroll_ram[TC0100SCN_MAX_CHIPS],
				*TC0100SCN_colscroll_ram[TC0100SCN_MAX_CHIPS];

static int TC0100SCN_bgscrollx[TC0100SCN_MAX_CHIPS],TC0100SCN_bgscrolly[TC0100SCN_MAX_CHIPS],
		TC0100SCN_fgscrollx[TC0100SCN_MAX_CHIPS],TC0100SCN_fgscrolly[TC0100SCN_MAX_CHIPS];

/* We keep two tilemaps for each of the 3 actual tilemaps: one at standard width, one double */
static struct tilemap *TC0100SCN_tilemap[TC0100SCN_MAX_CHIPS][3][2];

static char *TC0100SCN_char_dirty[TC0100SCN_MAX_CHIPS];
static int TC0100SCN_chars_dirty[TC0100SCN_MAX_CHIPS];
static int TC0100SCN_bg_gfx[TC0100SCN_MAX_CHIPS],TC0100SCN_tx_gfx[TC0100SCN_MAX_CHIPS];
static int TC0100SCN_bg_col_mult,TC0100SCN_bg_tilemask,TC0100SCN_tx_col_mult;
static int TC0100SCN_gfxbank,TC0100SCN_chip_colbank[3],TC0100SCN_colbank[3];
static int TC0100SCN_dblwidth[TC0100SCN_MAX_CHIPS];


INLINE void common_get_bg0_tile_info(data16_t *ram,int gfxnum,int tile_index,int colbank,int dblwidth)
{
	int code,attr;

	if (!dblwidth)
	{
		/* Mahjong Quest (F2 system) inexplicably has a banking feature */
		code = (ram[2*tile_index + 1] & TC0100SCN_bg_tilemask) + (TC0100SCN_gfxbank << 15);
		attr = ram[2*tile_index];
	}
	else
	{
		code = ram[2*tile_index + 1] & TC0100SCN_bg_tilemask;
		attr = ram[2*tile_index];
	}
	SET_TILE_INFO(
			gfxnum,
			code,
			(((attr * TC0100SCN_bg_col_mult) + TC0100SCN_colbank[0]) & 0xff) + colbank,
			TILE_FLIPYX((attr & 0xc000) >> 14))
}

INLINE void common_get_bg1_tile_info(data16_t *ram,int gfxnum,int tile_index,int colbank,int dblwidth)
{
	int code,attr;

	if (!dblwidth)
	{
		/* Mahjong Quest (F2 system) inexplicably has a banking feature */
		code = (ram[2*tile_index + 1] & TC0100SCN_bg_tilemask) + (TC0100SCN_gfxbank << 15);
		attr = ram[2*tile_index];
	}
	else
	{
		code = ram[2*tile_index + 1] & TC0100SCN_bg_tilemask;
		attr = ram[2*tile_index];
	}
	SET_TILE_INFO(
			gfxnum,
			code,
			(((attr * TC0100SCN_bg_col_mult) + TC0100SCN_colbank[1]) & 0xff) + colbank,
			TILE_FLIPYX((attr & 0xc000) >> 14))
}

INLINE void common_get_tx_tile_info(data16_t *ram,int gfxnum,int tile_index,int colbank,int dblwidth)
{
	int attr = ram[tile_index];

	SET_TILE_INFO(
			gfxnum,
			attr & 0xff,
			((((attr >> 6) &0xfc) * TC0100SCN_tx_col_mult + (TC0100SCN_colbank[2] << 2)) &0x3ff) + colbank*4,
			TILE_FLIPYX((attr & 0xc000) >> 14))
}

static void TC0100SCN_get_bg_tile_info_0(int tile_index)
{
	common_get_bg0_tile_info(TC0100SCN_bg_ram[0],TC0100SCN_bg_gfx[0],tile_index,
			TC0100SCN_chip_colbank[0],TC0100SCN_dblwidth[0]);
}

static void TC0100SCN_get_fg_tile_info_0(int tile_index)
{
	common_get_bg1_tile_info(TC0100SCN_fg_ram[0],TC0100SCN_bg_gfx[0],tile_index,
			TC0100SCN_chip_colbank[0],TC0100SCN_dblwidth[0]);
}

static void TC0100SCN_get_tx_tile_info_0(int tile_index)
{
	common_get_tx_tile_info(TC0100SCN_tx_ram[0],TC0100SCN_tx_gfx[0],tile_index,
			TC0100SCN_chip_colbank[0],TC0100SCN_dblwidth[0]);
}

static void TC0100SCN_get_bg_tile_info_1(int tile_index)
{
	common_get_bg0_tile_info(TC0100SCN_bg_ram[1],TC0100SCN_bg_gfx[1],tile_index,
			TC0100SCN_chip_colbank[1],TC0100SCN_dblwidth[1]);
}

static void TC0100SCN_get_fg_tile_info_1(int tile_index)
{
	common_get_bg1_tile_info(TC0100SCN_fg_ram[1],TC0100SCN_bg_gfx[1],tile_index,
			TC0100SCN_chip_colbank[1],TC0100SCN_dblwidth[1]);
}

static void TC0100SCN_get_tx_tile_info_1(int tile_index)
{
	common_get_tx_tile_info(TC0100SCN_tx_ram[1],TC0100SCN_tx_gfx[1],tile_index,
			TC0100SCN_chip_colbank[1],TC0100SCN_dblwidth[1]);
}

static void TC0100SCN_get_bg_tile_info_2(int tile_index)
{
	common_get_bg0_tile_info(TC0100SCN_bg_ram[2],TC0100SCN_bg_gfx[2],tile_index,
			TC0100SCN_chip_colbank[2],TC0100SCN_dblwidth[2]);
}

static void TC0100SCN_get_fg_tile_info_2(int tile_index)
{
	common_get_bg1_tile_info(TC0100SCN_fg_ram[2],TC0100SCN_bg_gfx[2],tile_index,
			TC0100SCN_chip_colbank[2],TC0100SCN_dblwidth[2]);
}

static void TC0100SCN_get_tx_tile_info_2(int tile_index)
{
	common_get_tx_tile_info(TC0100SCN_tx_ram[2],TC0100SCN_tx_gfx[2],tile_index,
			TC0100SCN_chip_colbank[2],TC0100SCN_dblwidth[2]);
}

/* This array changes with TC0100SCN_MAX_CHIPS */

void (*TC0100SCN_get_tile_info[TC0100SCN_MAX_CHIPS][3])(int tile_index) =
{
	{ TC0100SCN_get_bg_tile_info_0, TC0100SCN_get_fg_tile_info_0, TC0100SCN_get_tx_tile_info_0 },
	{ TC0100SCN_get_bg_tile_info_1, TC0100SCN_get_fg_tile_info_1, TC0100SCN_get_tx_tile_info_1 },
	{ TC0100SCN_get_bg_tile_info_2, TC0100SCN_get_fg_tile_info_2, TC0100SCN_get_tx_tile_info_2 }
};


static struct GfxLayout TC0100SCN_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
#ifdef LSB_FIRST
	{ 8, 0 },
#else
	{ 0, 8 },
#endif
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


void TC0100SCN_set_chip_colbanks(int chip0,int chip1,int chip2)
{
	TC0100SCN_chip_colbank[0] = chip0;	/* palette area for chip 0 */
	TC0100SCN_chip_colbank[1] = chip1;
	TC0100SCN_chip_colbank[2] = chip2;
}

void TC0100SCN_set_colbanks(int bg0,int bg1,int fg)
{
	TC0100SCN_colbank[0] = bg0;
	TC0100SCN_colbank[1] = bg1;
	TC0100SCN_colbank[2] = fg;	/* text */
}

void TC0100SCN_set_bg_tilemask(int mask)
{
	TC0100SCN_bg_tilemask = mask;
}

WRITE16_HANDLER( TC0100SCN_gfxbank_w )   /* Mjnquest banks its 2 sets of scr tiles */
{
    TC0100SCN_gfxbank = (data & 0x1);
}

void TC0100SCN_set_layer_ptrs(int i)
{
	if (!TC0100SCN_dblwidth[i])
	{
		TC0100SCN_bg_ram[i]        = TC0100SCN_ram[i] + 0x0;
		TC0100SCN_tx_ram[i]        = TC0100SCN_ram[i] + 0x4000 /2;
		TC0100SCN_char_ram[i]      = TC0100SCN_ram[i] + 0x6000 /2;
		TC0100SCN_fg_ram[i]        = TC0100SCN_ram[i] + 0x8000 /2;
		TC0100SCN_bgscroll_ram[i]  = TC0100SCN_ram[i] + 0xc000 /2;
		TC0100SCN_fgscroll_ram[i]  = TC0100SCN_ram[i] + 0xc400 /2;
		TC0100SCN_colscroll_ram[i] = TC0100SCN_ram[i] + 0xe000 /2;
	}
	else
	{
		TC0100SCN_bg_ram[i]        = TC0100SCN_ram[i] + 0x0;
		TC0100SCN_fg_ram[i]        = TC0100SCN_ram[i] + 0x08000 /2;
		TC0100SCN_bgscroll_ram[i]  = TC0100SCN_ram[i] + 0x10000 /2;
		TC0100SCN_fgscroll_ram[i]  = TC0100SCN_ram[i] + 0x10400 /2;
		TC0100SCN_colscroll_ram[i] = TC0100SCN_ram[i] + 0x10800 /2;
		TC0100SCN_char_ram[i]      = TC0100SCN_ram[i] + 0x11000 /2;
		TC0100SCN_tx_ram[i]        = TC0100SCN_ram[i] + 0x12000 /2;
	}
}

/* As we can't pass function calls with params in set...func_postload() calls
   in the vh_start, this slightly obnoxious method is used */

static void TC0100SCN_layer_ptr_0(void)
{
	TC0100SCN_set_layer_ptrs(0);
}

static void TC0100SCN_layer_ptr_1(void)
{
	TC0100SCN_set_layer_ptrs(1);
}

static void TC0100SCN_layer_ptr_2(void)
{
	TC0100SCN_set_layer_ptrs(2);
}

void (*TC0100SCN_layer_ptr[TC0100SCN_MAX_CHIPS])(void) =
{
	TC0100SCN_layer_ptr_0, TC0100SCN_layer_ptr_1, TC0100SCN_layer_ptr_2
};

void TC0100SCN_dirty_tilemaps(int chip)
{
	tilemap_mark_all_tiles_dirty(TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth[chip]]);
	tilemap_mark_all_tiles_dirty(TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth[chip]]);
	tilemap_mark_all_tiles_dirty(TC0100SCN_tilemap[chip][2][TC0100SCN_dblwidth[chip]]);
}

static void TC0100SCN_dirty_t_0(void)
{
	TC0100SCN_dirty_tilemaps(0);
}

static void TC0100SCN_dirty_t_1(void)
{
	TC0100SCN_dirty_tilemaps(1);
}

static void TC0100SCN_dirty_t_2(void)
{
	TC0100SCN_dirty_tilemaps(2);
}

void (*TC0100SCN_dirty_t[TC0100SCN_MAX_CHIPS])(void) =
{
	TC0100SCN_dirty_t_0, TC0100SCN_dirty_t_1, TC0100SCN_dirty_t_2
};

void TC0100SCN_dirty_chars(int chip)
{
	memset(TC0100SCN_char_dirty[chip],1,TC0100SCN_TOTAL_CHARS);
	TC0100SCN_chars_dirty[chip] = 1;
}

static void TC0100SCN_dirty_c_0(void)
{
	TC0100SCN_dirty_chars(0);
}

static void TC0100SCN_dirty_c_1(void)
{
	TC0100SCN_dirty_chars(1);
}

static void TC0100SCN_dirty_c_2(void)
{
	TC0100SCN_dirty_chars(2);
}

void (*TC0100SCN_dirty_c[TC0100SCN_MAX_CHIPS])(void) =
{
	TC0100SCN_dirty_c_0, TC0100SCN_dirty_c_1, TC0100SCN_dirty_c_2
};

static void TC0100SCN_restore_scroll(int chip)
{
	int flip;

	TC0100SCN_bgscrollx[chip] = -TC0100SCN_ctrl[chip][0];
	TC0100SCN_fgscrollx[chip] = -TC0100SCN_ctrl[chip][1];
	tilemap_set_scrollx(TC0100SCN_tilemap[chip][2][0],0,-TC0100SCN_ctrl[chip][2]);
	tilemap_set_scrollx(TC0100SCN_tilemap[chip][2][1],0,-TC0100SCN_ctrl[chip][2]);

	TC0100SCN_bgscrolly[chip] = -TC0100SCN_ctrl[chip][3];
	TC0100SCN_fgscrolly[chip] = -TC0100SCN_ctrl[chip][4];
	tilemap_set_scrolly(TC0100SCN_tilemap[chip][2][0],0,-TC0100SCN_ctrl[chip][5]);
	tilemap_set_scrolly(TC0100SCN_tilemap[chip][2][1],0,-TC0100SCN_ctrl[chip][5]);

	flip = (TC0100SCN_ctrl[chip][7] & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;
	tilemap_set_flip(TC0100SCN_tilemap[chip][0][0],flip);
	tilemap_set_flip(TC0100SCN_tilemap[chip][1][0],flip);
	tilemap_set_flip(TC0100SCN_tilemap[chip][2][0],flip);
	tilemap_set_flip(TC0100SCN_tilemap[chip][0][1],flip);
	tilemap_set_flip(TC0100SCN_tilemap[chip][1][1],flip);
	tilemap_set_flip(TC0100SCN_tilemap[chip][2][1],flip);
}

static void TC0100SCN_restore_scrl_0(void)
{
	TC0100SCN_restore_scroll(0);
}

static void TC0100SCN_restore_scrl_1(void)
{
	TC0100SCN_restore_scroll(1);
}

static void TC0100SCN_restore_scrl_2(void)
{
	TC0100SCN_restore_scroll(2);
}

static void (*TC0100SCN_restore_scrl[TC0100SCN_MAX_CHIPS])(void) =
{
	TC0100SCN_restore_scrl_0, TC0100SCN_restore_scrl_1, TC0100SCN_restore_scrl_2
};


int TC0100SCN_vh_start(int chips,int gfxnum,int x_offset,int y_offset,int flip_xoffs,
		int flip_yoffs,int flip_text_xoffs,int flip_text_yoffs,int multiscrn_xoffs)
{
	int gfx_index,gfxset_offs,i;


	if (chips > TC0100SCN_MAX_CHIPS) return 1;

	TC0100SCN_chips = chips;

	for (i = 0;i < chips;i++)
	{
		int xd,yd;
		TC0100SCN_dblwidth[i]=0;

		/* Single width versions */
		TC0100SCN_tilemap[i][0][0] = tilemap_create(TC0100SCN_get_tile_info[i][0],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
		TC0100SCN_tilemap[i][1][0] = tilemap_create(TC0100SCN_get_tile_info[i][1],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
		TC0100SCN_tilemap[i][2][0] = tilemap_create(TC0100SCN_get_tile_info[i][2],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

		/* Double width versions */
		TC0100SCN_tilemap[i][0][1] = tilemap_create(TC0100SCN_get_tile_info[i][0],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);
		TC0100SCN_tilemap[i][1][1] = tilemap_create(TC0100SCN_get_tile_info[i][1],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);
		TC0100SCN_tilemap[i][2][1] = tilemap_create(TC0100SCN_get_tile_info[i][2],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,32);

		/* Set up clipping for multi-TC0100SCN games. We assume
		   this code won't ever affect single screen games:
		   Thundfox is the only one of those with two chips, and
		   we're safe as it uses single width tilemaps. */

		if (chips==2)	/* Dual screen */
		{
			myclip.min_x = (320*i);
			myclip.min_y = 16;
			myclip.max_x = 320*(i+1) - 1;
			myclip.max_y = 256;
		}

		if (chips==3)	/* Triple screen */
		{
			myclip.min_x = (288*i);
			myclip.min_y = 16;
			myclip.max_x = 288*(i+1) - 1;
			myclip.max_y = 256;
		}

		if (chips>1)	/* Single screen games (Cameltru) don't need clipping */
		{
			tilemap_set_clip(TC0100SCN_tilemap[i][0][1],&myclip);
			tilemap_set_clip(TC0100SCN_tilemap[i][1][1],&myclip);
			tilemap_set_clip(TC0100SCN_tilemap[i][2][1],&myclip);
		}

		TC0100SCN_ram[i] = malloc(TC0100SCN_RAM_SIZE);
		TC0100SCN_char_dirty[i] = malloc(TC0100SCN_TOTAL_CHARS);

		if (!TC0100SCN_ram[i] || !TC0100SCN_char_dirty[i] ||
				!TC0100SCN_tilemap[i][0][0] || !TC0100SCN_tilemap[i][0][1] ||
				!TC0100SCN_tilemap[i][1][0] || !TC0100SCN_tilemap[i][1][1] ||
				!TC0100SCN_tilemap[i][2][0] || !TC0100SCN_tilemap[i][2][1] )
		{
			TC0100SCN_vh_stop();
			return 1;
		}

		TC0100SCN_set_layer_ptrs(i);
		TC0100SCN_dirty_chars(i);
		memset(TC0100SCN_ram[i],0,TC0100SCN_RAM_SIZE);

		{
			char buf[20];	/* we need different labels for every item of save data */
			sprintf(buf,"TC0100SCN-%01x",i);	/* so we add chip # as a suffix */
			state_save_register_UINT16(strcat(buf,"a"), 0, "memory", TC0100SCN_ram[i], TC0100SCN_RAM_SIZE/2);
			sprintf(buf,"TC0100SCN-%01x",i);
			state_save_register_UINT16(strcat(buf,"b"), 0, "registers", TC0100SCN_ctrl[i], 8);
			sprintf(buf,"TC0100SCN-%01x",i);
			state_save_register_int   (strcat(buf,"c"), 0, "registers", &TC0100SCN_dblwidth[i]);
		}

		state_save_register_func_postload(TC0100SCN_layer_ptr[i]);
		state_save_register_func_postload(TC0100SCN_dirty_c[i]);
		state_save_register_func_postload(TC0100SCN_dirty_t[i]);	/* unnecessary ? */
		state_save_register_func_postload(TC0100SCN_restore_scrl[i]);

		/* find first empty slot to decode gfx */
		for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
			if (Machine->gfx[gfx_index] == 0)
				break;
		if (gfx_index == MAX_GFX_ELEMENTS)
		{
			TC0100SCN_vh_stop();
			return 1;
		}

		/* create the char set (gfx will then be updated dynamically from RAM) */
		Machine->gfx[gfx_index] = decodegfx((UINT8 *)TC0100SCN_char_ram[i],&TC0100SCN_charlayout);
		if (!Machine->gfx[gfx_index])
			return 1;

		/* set the color information */
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = 64;

		TC0100SCN_tx_gfx[i] = gfx_index;

		/* use the given gfx set for bg tiles; 2nd/3rd chips will
		   use the same gfx set */
		gfxset_offs = i;
		if (i > 1)
			gfxset_offs = 1;
		TC0100SCN_bg_gfx[i] = gfxnum + gfxset_offs;

		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][0][0],0);
		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][1][0],0);
		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][2][0],0);

		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][0][1],0);
		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][1][1],0);
		tilemap_set_transparent_pen(TC0100SCN_tilemap[i][2][1],0);

		/* Standard width tilemaps. I'm setting the optional chip #2
		   7 bits higher and 2 pixels to the left than chip #1 because
		   that's how thundfox wants it. */

		xd = (i == 0) ?  (-x_offset) : (-x_offset-2);
		yd = (i == 0) ? (8-y_offset) : (1-y_offset);

		tilemap_set_scrolldx(TC0100SCN_tilemap[i][0][0], xd-16, -xd-16);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][0][0], yd,    -yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][1][0], xd-16, -xd-16);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][1][0], yd,    -yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][2][0], xd-16, -xd-16-7);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][2][0], yd,    -yd);

		/* Double width tilemaps. We must correct offsets for
		   extra chips, as MAME sees offsets from LHS of whole
		   display not from the edges of individual screens.
		   NB flipscreen tilemap offsets are speculative. */

		xd = -x_offset;
		yd = 8-y_offset;

		if (chips==2)	/* Dual screen */
		{
			if (i==1)  xd += (320-multiscrn_xoffs);
		}
		if (chips==3)	/* Triple screen */
		{
			if (i==1)  xd += (286-multiscrn_xoffs);
			if (i==2)  xd += (572-multiscrn_xoffs*2);
		}
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][0][1], xd-16, -xd-16);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][0][1], yd,    -yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][1][1], xd-16, -xd-16);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][1][1], yd,    -yd);
		tilemap_set_scrolldx(TC0100SCN_tilemap[i][2][1], xd-16, -xd-16-7);
		tilemap_set_scrolldy(TC0100SCN_tilemap[i][2][1], yd,    -yd);

		tilemap_set_scroll_rows(TC0100SCN_tilemap[i][0][0],512);
		tilemap_set_scroll_rows(TC0100SCN_tilemap[i][1][0],512);
		tilemap_set_scroll_rows(TC0100SCN_tilemap[i][0][1],512);
		tilemap_set_scroll_rows(TC0100SCN_tilemap[i][1][1],512);

		/* Default is for all used chips to access the same palette area */
		TC0100SCN_chip_colbank[i]=0;
	}

	TC0100SCN_gfxbank= 0;	/* Mjnquest uniquely banks tiles */
	state_save_register_int ("TC100SCN_bank", 0, "control", &TC0100SCN_gfxbank);

	TC0100SCN_bg_tilemask = 0xffff;	/* Mjnquest has 0x7fff tilemask */

	TC0100SCN_bg_col_mult = 1;	/* multiplier for when bg gfx != 4bpp */
	TC0100SCN_tx_col_mult = 1;	/* multiplier needed when bg gfx is 6bpp */

	if (Machine->gfx[gfxnum]->color_granularity == 2)	/* Yuyugogo, Yesnoj */
		TC0100SCN_bg_col_mult = 8;

	if (Machine->gfx[gfxnum]->color_granularity == 0x40)	/* Undrfire */
		TC0100SCN_tx_col_mult = 4;

/*logerror("TC0100SCN bg gfx granularity %04x: multiplier %04x\n", */
/*Machine->gfx[gfxnum]->color_granularity,TC0100SCN_tx_col_mult); */

	TC0100SCN_set_colbanks(0,0,0);	/* standard values, only Wgp changes them */

	return 0;
}

void TC0100SCN_vh_stop(void)
{
	int i;

	for (i = 0;i < TC0100SCN_chips;i++)
	{
		free(TC0100SCN_ram[i]);
		TC0100SCN_ram[i] = 0;
		free(TC0100SCN_char_dirty[i]);
		TC0100SCN_char_dirty[i] = 0;
	}
}


READ16_HANDLER( TC0100SCN_word_0_r )
{
	return TC0100SCN_ram[0][offset];
}

READ16_HANDLER( TC0100SCN_word_1_r )
{
	return TC0100SCN_ram[1][offset];
}

READ16_HANDLER( TC0100SCN_word_2_r )
{
	return TC0100SCN_ram[2][offset];
}

static void TC0100SCN_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	int oldword = TC0100SCN_ram[chip][offset];

	COMBINE_DATA(&TC0100SCN_ram[chip][offset]);
	if (oldword != TC0100SCN_ram[chip][offset])
	{
		if (!TC0100SCN_dblwidth[chip])
		{
			if (offset < 0x2000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][0][0],offset / 2);
			else if (offset < 0x3000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][2][0],(offset & 0x0fff));
			else if (offset < 0x3800)
			{
				TC0100SCN_char_dirty[chip][(offset - 0x3000) / 8] = 1;
				TC0100SCN_chars_dirty[chip] = 1;
			}
			else if (offset >= 0x4000 && offset < 0x6000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][1][0],(offset & 0x1fff) / 2);
		}
		else	/* Double-width tilemaps have a different memory map */
		{
			if (offset < 0x4000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][0][1],offset / 2);
			else if (offset >= 0x4000 && offset < 0x8000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][1][1],(offset & 0x3fff) / 2);
			else if (offset >= 0x8800 && offset < 0x9000)
			{
				TC0100SCN_char_dirty[chip][(offset - 0x8800) / 8] = 1;
				TC0100SCN_chars_dirty[chip] = 1;
			}
			else if (offset >= 0x9000)
				tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][2][1],(offset & 0x0fff));
		}
	}
}

WRITE16_HANDLER( TC0100SCN_word_0_w )
{
	TC0100SCN_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_word_1_w )
{
	TC0100SCN_word_w(1,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_word_2_w )
{
	TC0100SCN_word_w(2,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_dual_screen_w )
{
	TC0100SCN_word_0_w(offset,data,mem_mask);
	TC0100SCN_word_1_w(offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_triple_screen_w )
{
	TC0100SCN_word_0_w(offset,data,mem_mask);
	TC0100SCN_word_1_w(offset,data,mem_mask);
	TC0100SCN_word_2_w(offset,data,mem_mask);
}


READ16_HANDLER( TC0100SCN_ctrl_word_0_r )
{
	return TC0100SCN_ctrl[0][offset];
}

READ16_HANDLER( TC0100SCN_ctrl_word_1_r )
{
	return TC0100SCN_ctrl[1][offset];
}

READ16_HANDLER( TC0100SCN_ctrl_word_2_r )
{
	return TC0100SCN_ctrl[2][offset];
}


static void TC0100SCN_ctrl_word_w(int chip,offs_t offset,data16_t data,UINT32 mem_mask)
{
	COMBINE_DATA(&TC0100SCN_ctrl[chip][offset]);

	data = TC0100SCN_ctrl[chip][offset];

	switch (offset)
	{
		case 0x00:
			TC0100SCN_bgscrollx[chip] = -data;
			break;

		case 0x01:
			TC0100SCN_fgscrollx[chip] = -data;
			break;

		case 0x02:
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][2][0],0,-data);
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][2][1],0,-data);
			break;

		case 0x03:
			TC0100SCN_bgscrolly[chip] = -data;
			break;

		case 0x04:
			TC0100SCN_fgscrolly[chip] = -data;
			break;

		case 0x05:
			tilemap_set_scrolly(TC0100SCN_tilemap[chip][2][0],0,-data);
			tilemap_set_scrolly(TC0100SCN_tilemap[chip][2][1],0,-data);
			break;

		case 0x06:
		{
			int old_width = TC0100SCN_dblwidth[chip];
			TC0100SCN_dblwidth[chip] = (data &0x10) >> 4;

			if (TC0100SCN_dblwidth[chip] != old_width)	/* tilemap width is changing */
			{
				/* Reinitialise layer pointers */
				TC0100SCN_set_layer_ptrs(chip);

				/* and ensure full redraw of the tilemaps */
				TC0100SCN_dirty_tilemaps(chip);
			}

			break;
		}

		case 0x07:
		{
			int flip = (data & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;

			tilemap_set_flip(TC0100SCN_tilemap[chip][0][0],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][1][0],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][2][0],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][0][1],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][1][1],flip);
			tilemap_set_flip(TC0100SCN_tilemap[chip][2][1],flip);

			break;
		}
	}
}

WRITE16_HANDLER( TC0100SCN_ctrl_word_0_w )
{
	TC0100SCN_ctrl_word_w(0,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_ctrl_word_1_w )
{
	TC0100SCN_ctrl_word_w(1,offset,data,mem_mask);
}

WRITE16_HANDLER( TC0100SCN_ctrl_word_2_w )
{
	TC0100SCN_ctrl_word_w(2,offset,data,mem_mask);
}


READ32_HANDLER( TC0100SCN_ctrl_long_r )
{
	return (TC0100SCN_ctrl_word_0_r(offset*2,0)<<16)|TC0100SCN_ctrl_word_0_r(offset*2+1,0);
}

WRITE32_HANDLER( TC0100SCN_ctrl_long_w )
{
	if (ACCESSING_MSW32) TC0100SCN_ctrl_word_w(0,offset*2,data>>16,mem_mask>>16);
	if (ACCESSING_LSW32) TC0100SCN_ctrl_word_w(0,(offset*2)+1,data&0xffff,mem_mask&0xffff);
}

READ32_HANDLER( TC0100SCN_long_r )
{
	return (TC0100SCN_word_0_r(offset*2,0)<<16)|TC0100SCN_word_0_r(offset*2+1,0);
}

WRITE32_HANDLER( TC0100SCN_long_w )
{
	if (((mem_mask & 0xff000000) == 0) || ((mem_mask & 0x00ff0000) == 0))
	{
		int oldword = TC0100SCN_word_0_r(offset*2,0);
		int newword = data>>16;
		if ((mem_mask & 0x00ff0000) != 0)
			newword |= (oldword &0x00ff);
		if ((mem_mask & 0xff000000) != 0)
			newword |= (oldword &0xff00);
		TC0100SCN_word_0_w(offset*2,newword,0);
	}
	if (((mem_mask & 0x0000ff00) == 0) || ((mem_mask & 0x000000ff) == 0))
	{
		int oldword = TC0100SCN_word_0_r((offset*2)+1,0);
		int newword = data&0xffff;
		if ((mem_mask & 0x000000ff) != 0)
			newword |= (oldword &0x00ff);
		if ((mem_mask & 0x0000ff00) != 0)
			newword |= (oldword &0xff00);
		TC0100SCN_word_0_w((offset*2)+1,newword,0);
	}
}


void TC0100SCN_tilemap_update(void)
{
	int chip,j;

	for (chip = 0;chip < TC0100SCN_chips;chip++)
	{
		tilemap_set_scrolly(TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth[chip]],0,TC0100SCN_bgscrolly[chip]);
		tilemap_set_scrolly(TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth[chip]],0,TC0100SCN_fgscrolly[chip]);

		for (j = 0;j < 256;j++)
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth[chip]],
					(j + TC0100SCN_bgscrolly[chip]) & 0x1ff,
					TC0100SCN_bgscrollx[chip] - TC0100SCN_bgscroll_ram[chip][j]);
		for (j = 0;j < 256;j++)
			tilemap_set_scrollx(TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth[chip]],
					(j + TC0100SCN_fgscrolly[chip]) & 0x1ff,
					TC0100SCN_fgscrollx[chip] - TC0100SCN_fgscroll_ram[chip][j]);

		/* Decode any characters that have changed */
		if (TC0100SCN_chars_dirty[chip])
		{
			int tile_index;


			for (tile_index = 0;tile_index < 64*64;tile_index++)
			{
				int attr = TC0100SCN_tx_ram[chip][tile_index];
				if (TC0100SCN_char_dirty[chip][attr & 0xff])
					tilemap_mark_tile_dirty(TC0100SCN_tilemap[chip][2][TC0100SCN_dblwidth[chip]],tile_index);
			}

			for (j = 0;j < TC0100SCN_TOTAL_CHARS;j++)
			{
				if (TC0100SCN_char_dirty[chip][j])
					decodechar(Machine->gfx[TC0100SCN_tx_gfx[chip]],j,
					(UINT8 *)TC0100SCN_char_ram[chip],&TC0100SCN_charlayout);
				TC0100SCN_char_dirty[chip][j] = 0;
			}
			TC0100SCN_chars_dirty[chip] = 0;
		}

		tilemap_update(TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth[chip]]);
		tilemap_update(TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth[chip]]);
		tilemap_update(TC0100SCN_tilemap[chip][2][TC0100SCN_dblwidth[chip]]);
	}
}

void TC0100SCN_tilemap_draw(struct osd_bitmap *bitmap,int chip,int layer,int flags,UINT32 priority)
{
	int disable = TC0100SCN_ctrl[chip][6] & 0xf7;

#if 0
if (disable != 0 && disable != 3 && disable != 7)
	usrintf_showmessage("layer disable = %x",disable);
#endif

	switch (layer)
	{
		case 0:
			if (disable & 0x01) return;
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][0][TC0100SCN_dblwidth[chip]],flags,priority);
			break;
		case 1:
			if (disable & 0x02) return;
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][1][TC0100SCN_dblwidth[chip]],flags,priority);
			break;
		case 2:
			if (disable & 0x04) return;
			tilemap_draw(bitmap,TC0100SCN_tilemap[chip][2][TC0100SCN_dblwidth[chip]],flags,priority);
			break;
	}
}

int TC0100SCN_bottomlayer(int chip)
{
	return (TC0100SCN_ctrl[chip][6] & 0x8) >> 3;
}


/***************************************************************************/

#define TC0280GRD_RAM_SIZE 0x2000
static data16_t TC0280GRD_ctrl[8];
static data16_t *TC0280GRD_ram;
static struct tilemap *TC0280GRD_tilemap;
static int TC0280GRD_gfxnum,TC0280GRD_base_color;


static void TC0280GRD_get_tile_info(int tile_index)
{
	int attr = TC0280GRD_ram[tile_index];
	SET_TILE_INFO(
			TC0280GRD_gfxnum,
			attr & 0x3fff,
			((attr & 0xc000) >> 14) + TC0280GRD_base_color,
			0)
}


int TC0280GRD_vh_start(int gfxnum)
{
	TC0280GRD_ram = malloc(TC0280GRD_RAM_SIZE);
	TC0280GRD_tilemap = tilemap_create(TC0280GRD_get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,64);

	if (!TC0280GRD_ram || !TC0280GRD_tilemap)
	{
		TC0280GRD_vh_stop();
		return 1;
	}

	state_save_register_UINT16("TC0280GRDa", 0, "memory", TC0280GRD_ram, TC0280GRD_RAM_SIZE/2);
	state_save_register_UINT16("TC0280GRDb", 0, "registers", TC0280GRD_ctrl, 8);

	tilemap_set_clip(TC0280GRD_tilemap,0);

	TC0280GRD_gfxnum = gfxnum;

	return 0;
}

int TC0430GRW_vh_start(int gfxnum)
{
	return TC0280GRD_vh_start(gfxnum);
}

void TC0280GRD_vh_stop(void)
{
	free(TC0280GRD_ram);
	TC0280GRD_ram = 0;
}

void TC0430GRW_vh_stop(void)
{
	TC0280GRD_vh_stop();
}

READ16_HANDLER( TC0280GRD_word_r )
{
	return TC0280GRD_ram[offset];
}

READ16_HANDLER( TC0430GRW_word_r )
{
	return TC0280GRD_word_r(offset,mem_mask);
}

WRITE16_HANDLER( TC0280GRD_word_w )
{
	int oldword = TC0280GRD_ram[offset];

	COMBINE_DATA(&TC0280GRD_ram[offset]);
	if (oldword != TC0280GRD_ram[offset])
	{
		tilemap_mark_tile_dirty(TC0280GRD_tilemap,offset);
	}
}

WRITE16_HANDLER( TC0430GRW_word_w )
{
	TC0280GRD_word_w(offset,data,mem_mask);
}

WRITE16_HANDLER( TC0280GRD_ctrl_word_w )
{
	COMBINE_DATA(&TC0280GRD_ctrl[offset]);
}

WRITE16_HANDLER( TC0430GRW_ctrl_word_w )
{
	TC0280GRD_ctrl_word_w(offset,data,mem_mask);
}

void TC0280GRD_tilemap_update(int base_color)
{
	if (TC0280GRD_base_color != base_color)
	{
		TC0280GRD_base_color = base_color;
		tilemap_mark_all_tiles_dirty(TC0280GRD_tilemap);
	}

	tilemap_update(TC0280GRD_tilemap);
}

void TC0430GRW_tilemap_update(int base_color)
{
	TC0280GRD_tilemap_update(base_color);
}

static void zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority,int xmultiply)
{
	UINT32 startx,starty;
	int incxx,incxy,incyx,incyy;
	struct osd_bitmap *srcbitmap = tilemap_get_pixmap(TC0280GRD_tilemap);

	/* 24-bit signed */
	startx = ((TC0280GRD_ctrl[0] & 0xff) << 16) + TC0280GRD_ctrl[1];
	if (startx & 0x800000) startx -= 0x1000000;
	incxx = (INT16)TC0280GRD_ctrl[2];
	incxx *= xmultiply;
	incyx = (INT16)TC0280GRD_ctrl[3];
	/* 24-bit signed */
	starty = ((TC0280GRD_ctrl[4] & 0xff) << 16) + TC0280GRD_ctrl[5];
	if (starty & 0x800000) starty -= 0x1000000;
	incxy = (INT16)TC0280GRD_ctrl[6];
	incxy *= xmultiply;
	incyy = (INT16)TC0280GRD_ctrl[7];

	startx -= xoffset * incxx + yoffset * incyx;
	starty -= xoffset * incxy + yoffset * incyy;

	copyrozbitmap(bitmap,srcbitmap,startx << 4,starty << 4,
			incxx << 4,incxy << 4,incyx << 4,incyy << 4,
			1,	/* copy with wraparound */
			&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen,priority);
}

void TC0280GRD_zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority)
{
	zoom_draw(bitmap,xoffset,yoffset,priority,2);
}

void TC0430GRW_zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority)
{
	zoom_draw(bitmap,xoffset,yoffset,priority,1);
}


/***************************************************************************/

UINT8 TC0360PRI_regs[16];

int TC0360PRI_vh_start(void)
{
	state_save_register_UINT8("TC0360PRI", 0, "registers", TC0360PRI_regs, 16);
	return 0;
}

WRITE_HANDLER( TC0360PRI_w )
{
	TC0360PRI_regs[offset] = data;

if (offset >= 0x0a)
	usrintf_showmessage("write %02x to unused TC0360PRI reg %x",data,offset);
#if 0
#define regs TC0360PRI_regs
	usrintf_showmessage("%02x %02x  %02x %02x  %02x %02x %02x %02x %02x %02x",
		regs[0x00],regs[0x01],regs[0x02],regs[0x03],
		regs[0x04],regs[0x05],regs[0x06],regs[0x07],
		regs[0x08],regs[0x09]);
#endif
}

WRITE16_HANDLER( TC0360PRI_halfword_w )
{
	if (ACCESSING_LSB)
	{
		TC0360PRI_w(offset,data & 0xff);
#if 0
if (data & 0xff00)
{ logerror("CPU #0 PC %06x: warning - write %02x to MSB of TC0360PRI address %02x\n",cpu_get_pc(),data,offset); }
	else
{ logerror("CPU #0 PC %06x: warning - write %02x to MSB of TC0360PRI address %02x\n",cpu_get_pc(),data,offset); }
#endif
	}
}

WRITE16_HANDLER( TC0360PRI_halfword_swap_w )
{
	if (ACCESSING_MSB)
	{
		TC0360PRI_w(offset,(data >> 8) & 0xff);
#if 0
if (data & 0xff)
{ logerror("CPU #0 PC %06x: warning - write %02x to LSB of TC0360PRI address %02x\n",cpu_get_pc(),data,offset); }
	else
{ logerror("CPU #0 PC %06x: warning - write %02x to LSB of TC0360PRI address %02x\n",cpu_get_pc(),data,offset); }
#endif
	}
}


/***************************************************************************/


#define TC0480SCP_RAM_SIZE 0x10000
#define TC0480SCP_TOTAL_CHARS 256
static data16_t TC0480SCP_ctrl[0x18];
static data16_t *TC0480SCP_ram,
		*TC0480SCP_bg_ram[4],
		*TC0480SCP_tx_ram,
		*TC0480SCP_char_ram,
		*TC0480SCP_bgscroll_ram[4],
		*TC0480SCP_rowzoom_ram[4],
		*TC0480SCP_bgcolumn_ram[4];
static int TC0480SCP_bgscrollx[4];
static int TC0480SCP_bgscrolly[4];

/* We keep two tilemaps for each of the 5 actual tilemaps: one at standard width, one double */
static struct tilemap *TC0480SCP_tilemap[5][2];
static char *TC0480SCP_char_dirty;
static int TC0480SCP_chars_dirty;
static int TC0480SCP_bg_gfx,TC0480SCP_tx_gfx;
static int TC0480SCP_tile_colbase,TC0480SCP_dblwidth;
static int TC0480SCP_x_offs,TC0480SCP_y_offs;
static int TC0480SCP_text_xoffs,TC0480SCP_text_yoffs;
static int TC0480SCP_flip_xoffs,TC0480SCP_flip_yoffs;

int TC0480SCP_pri_reg;   // read externally in vidhrdw\taito_f2.c

INLINE void common_get_tc0480bg_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	int code = ram[2*tile_index + 1] & 0x7fff;
	int attr = ram[2*tile_index];
	SET_TILE_INFO(
			gfxnum,
			code,
			(attr & 0xff) + TC0480SCP_tile_colbase,
			TILE_FLIPYX((attr & 0xc000) >> 14))
}

INLINE void common_get_tc0480tx_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	int attr = ram[tile_index];
	SET_TILE_INFO(
			gfxnum,
			attr & 0xff,
			((attr & 0x3f00) >> 8) + TC0480SCP_tile_colbase,
			TILE_FLIPYX((attr & 0xc000) >> 14))
}

static void TC0480SCP_get_bg0_tile_info(int tile_index)
{
	common_get_tc0480bg_tile_info(TC0480SCP_bg_ram[0],TC0480SCP_bg_gfx,tile_index);
}

static void TC0480SCP_get_bg1_tile_info(int tile_index)
{
	common_get_tc0480bg_tile_info(TC0480SCP_bg_ram[1],TC0480SCP_bg_gfx,tile_index);
}

static void TC0480SCP_get_bg2_tile_info(int tile_index)
{
	common_get_tc0480bg_tile_info(TC0480SCP_bg_ram[2],TC0480SCP_bg_gfx,tile_index);
}

static void TC0480SCP_get_bg3_tile_info(int tile_index)
{
	common_get_tc0480bg_tile_info(TC0480SCP_bg_ram[3],TC0480SCP_bg_gfx,tile_index);
}

static void TC0480SCP_get_tx_tile_info(int tile_index)
{
	common_get_tc0480tx_tile_info(TC0480SCP_tx_ram,TC0480SCP_tx_gfx,tile_index);
}

void (*tc480_get_tile_info[5])(int tile_index) =
{
	TC0480SCP_get_bg0_tile_info, TC0480SCP_get_bg1_tile_info,
	TC0480SCP_get_bg2_tile_info, TC0480SCP_get_bg3_tile_info,
	TC0480SCP_get_tx_tile_info
};


static struct GfxLayout TC0480SCP_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
#ifdef LSB_FIRST
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
#else
	{ 3*4, 2*4, 1*4, 0*4, 7*4, 6*4, 5*4, 4*4 },
#endif
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

void TC0480SCP_set_layer_ptrs(void)
{
	if (!TC0480SCP_dblwidth)
	{
		TC0480SCP_bg_ram[0]	  = TC0480SCP_ram + 0x0000; /*0000 */
		TC0480SCP_bg_ram[1]	  = TC0480SCP_ram + 0x0800; /*1000 */
		TC0480SCP_bg_ram[2]	  = TC0480SCP_ram + 0x1000; /*2000 */
		TC0480SCP_bg_ram[3]	  = TC0480SCP_ram + 0x1800; /*3000 */
		TC0480SCP_bgscroll_ram[0] = TC0480SCP_ram + 0x2000; /*4000 */
		TC0480SCP_bgscroll_ram[1] = TC0480SCP_ram + 0x2200; /*4400 */
		TC0480SCP_bgscroll_ram[2] = TC0480SCP_ram + 0x2400; /*4800 */
		TC0480SCP_bgscroll_ram[3] = TC0480SCP_ram + 0x2600; /*4c00 */
		TC0480SCP_rowzoom_ram[2]  = TC0480SCP_ram + 0x3000; /*6000 */
		TC0480SCP_rowzoom_ram[3]  = TC0480SCP_ram + 0x3200; /*6400 */
		TC0480SCP_bgcolumn_ram[2] = TC0480SCP_ram + 0x3400; /*6800 */
		TC0480SCP_bgcolumn_ram[3] = TC0480SCP_ram + 0x3600; /*6c00 */
		TC0480SCP_tx_ram		  = TC0480SCP_ram + 0x6000; /*c000 */
		TC0480SCP_char_ram	  = TC0480SCP_ram + 0x7000; /*e000 */
	}
	else
	{
		TC0480SCP_bg_ram[0]	  = TC0480SCP_ram + 0x0000; /*0000 */
		TC0480SCP_bg_ram[1]	  = TC0480SCP_ram + 0x1000; /*2000 */
		TC0480SCP_bg_ram[2]	  = TC0480SCP_ram + 0x2000; /*4000 */
		TC0480SCP_bg_ram[3]	  = TC0480SCP_ram + 0x3000; /*6000 */
		TC0480SCP_bgscroll_ram[0] = TC0480SCP_ram + 0x4000; /*8000 */
		TC0480SCP_bgscroll_ram[1] = TC0480SCP_ram + 0x4200; /*8400 */
		TC0480SCP_bgscroll_ram[2] = TC0480SCP_ram + 0x4400; /*8800 */
		TC0480SCP_bgscroll_ram[3] = TC0480SCP_ram + 0x4600; /*8c00 */
		TC0480SCP_rowzoom_ram[2]  = TC0480SCP_ram + 0x5000; /*a000 */
		TC0480SCP_rowzoom_ram[3]  = TC0480SCP_ram + 0x5200; /*a400 */
		TC0480SCP_bgcolumn_ram[2] = TC0480SCP_ram + 0x5400; /*a800 */
		TC0480SCP_bgcolumn_ram[3] = TC0480SCP_ram + 0x5600; /*ac00 */
		TC0480SCP_tx_ram		  = TC0480SCP_ram + 0x6000; /*c000 */
		TC0480SCP_char_ram	  = TC0480SCP_ram + 0x7000; /*e000 */
	}
}

void TC0480SCP_dirty_tilemaps(void)
{
	tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[0][TC0480SCP_dblwidth]);
	tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[1][TC0480SCP_dblwidth]);
	tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[2][TC0480SCP_dblwidth]);
	tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[3][TC0480SCP_dblwidth]);
	tilemap_mark_all_tiles_dirty(TC0480SCP_tilemap[4][TC0480SCP_dblwidth]);
}

void TC0480SCP_dirty_chars(void)
{
	memset(TC0480SCP_char_dirty,1,TC0480SCP_TOTAL_CHARS);
	TC0480SCP_chars_dirty = 1;
}

static void TC0480SCP_restore_scroll(void)
{
	int reg;
	int flip = TC0480SCP_ctrl[0xf] & 0x40;

	tilemap_set_flip(TC0480SCP_tilemap[0][0],flip);
	tilemap_set_flip(TC0480SCP_tilemap[1][0],flip);
	tilemap_set_flip(TC0480SCP_tilemap[2][0],flip);
	tilemap_set_flip(TC0480SCP_tilemap[3][0],flip);
	tilemap_set_flip(TC0480SCP_tilemap[4][0],flip);

	tilemap_set_flip(TC0480SCP_tilemap[0][1],flip);
	tilemap_set_flip(TC0480SCP_tilemap[1][1],flip);
	tilemap_set_flip(TC0480SCP_tilemap[2][1],flip);
	tilemap_set_flip(TC0480SCP_tilemap[3][1],flip);
	tilemap_set_flip(TC0480SCP_tilemap[4][1],flip);

	reg = TC0480SCP_ctrl[0];
	if (!flip)  reg = -reg;
	TC0480SCP_bgscrollx[0] = reg;

	reg = TC0480SCP_ctrl[1] + 4;
	if (!flip)  reg = -reg;
	TC0480SCP_bgscrollx[1] = reg;

	reg = TC0480SCP_ctrl[2] + 8;
	if (!flip)  reg = -reg;
	TC0480SCP_bgscrollx[2] = reg;

	reg = TC0480SCP_ctrl[3] + 12;
	if (!flip)  reg = -reg;
	TC0480SCP_bgscrollx[3] = reg;

	reg = TC0480SCP_ctrl[4];
	if (!flip)  reg = -reg;
	TC0480SCP_bgscrolly[0] = reg;

	reg = TC0480SCP_ctrl[5];
	if (!flip)  reg = -reg;
	TC0480SCP_bgscrolly[1] = reg;

	reg = TC0480SCP_ctrl[6];
	if (!flip)  reg = -reg;
	TC0480SCP_bgscrolly[2] = reg;

	reg = TC0480SCP_ctrl[7];
	if (!flip)  reg = -reg;
	TC0480SCP_bgscrolly[3] = reg;

	reg = TC0480SCP_ctrl[0x0c];
	if (!flip)	reg -= TC0480SCP_text_xoffs;
	if (flip)	reg += TC0480SCP_text_xoffs;
	tilemap_set_scrollx(TC0480SCP_tilemap[4][0], 0, -reg);
	tilemap_set_scrollx(TC0480SCP_tilemap[4][1], 0, -reg);

	reg = TC0480SCP_ctrl[0x0d];
	if (!flip)	reg -= TC0480SCP_text_yoffs;
	if (flip)	reg += TC0480SCP_text_yoffs;
	tilemap_set_scrolly(TC0480SCP_tilemap[4][0], 0, -reg);
	tilemap_set_scrolly(TC0480SCP_tilemap[4][1], 0, -reg);
}


int TC0480SCP_vh_start(int gfxnum,int pixels,int x_offset,int y_offset,int text_xoffs,
				int text_yoffs,int flip_xoffs,int flip_yoffs,int col_base)
{
	int gfx_index;

		int i,xd,yd;
		TC0480SCP_tile_colbase = col_base;
		TC0480SCP_text_xoffs = text_xoffs;
		TC0480SCP_text_yoffs = text_yoffs;
		TC0480SCP_flip_xoffs = flip_xoffs;	/* for most games (-1,0) */
		TC0480SCP_flip_yoffs = flip_yoffs;
		TC0480SCP_dblwidth=0;

		/* Single width versions */
		TC0480SCP_tilemap[0][0] = tilemap_create(tc480_get_tile_info[0],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		TC0480SCP_tilemap[1][0] = tilemap_create(tc480_get_tile_info[1],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		TC0480SCP_tilemap[2][0] = tilemap_create(tc480_get_tile_info[2],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		TC0480SCP_tilemap[3][0] = tilemap_create(tc480_get_tile_info[3],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		TC0480SCP_tilemap[4][0] = tilemap_create(tc480_get_tile_info[4],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

		/* Double width versions */
		TC0480SCP_tilemap[0][1] = tilemap_create(tc480_get_tile_info[0],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		TC0480SCP_tilemap[1][1] = tilemap_create(tc480_get_tile_info[1],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		TC0480SCP_tilemap[2][1] = tilemap_create(tc480_get_tile_info[2],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		TC0480SCP_tilemap[3][1] = tilemap_create(tc480_get_tile_info[3],tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		TC0480SCP_tilemap[4][1] = tilemap_create(tc480_get_tile_info[4],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

		TC0480SCP_ram = malloc(TC0480SCP_RAM_SIZE);
		TC0480SCP_char_dirty = malloc(TC0480SCP_TOTAL_CHARS);

		if (!TC0480SCP_ram || !TC0480SCP_char_dirty ||
				!TC0480SCP_tilemap[0][0] || !TC0480SCP_tilemap[0][1] ||
				!TC0480SCP_tilemap[1][0] || !TC0480SCP_tilemap[1][1] ||
				!TC0480SCP_tilemap[2][0] || !TC0480SCP_tilemap[2][1] ||
				!TC0480SCP_tilemap[3][0] || !TC0480SCP_tilemap[3][1] ||
				!TC0480SCP_tilemap[4][0] || !TC0480SCP_tilemap[4][1])
		{
			TC0480SCP_vh_stop();
			return 1;
		}

		TC0480SCP_set_layer_ptrs();
		TC0480SCP_dirty_chars();
		memset(TC0480SCP_ram,0,TC0480SCP_RAM_SIZE);

		state_save_register_UINT16("TC0480SCPa", 0, "memory", TC0480SCP_ram, TC0480SCP_RAM_SIZE/2);
		state_save_register_UINT16("TC0480SCPb", 0, "registers", TC0480SCP_ctrl, 0x18);
		state_save_register_int   ("TC0480SCPc", 0, "registers", &TC0480SCP_dblwidth);
		state_save_register_func_postload(TC0480SCP_set_layer_ptrs);
		state_save_register_func_postload(TC0480SCP_dirty_chars);
		state_save_register_func_postload(TC0480SCP_dirty_tilemaps);	/* unnecessary ? */
		state_save_register_func_postload(TC0480SCP_restore_scroll);

		/* find first empty slot to decode gfx */
		for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
			if (Machine->gfx[gfx_index] == 0)
				break;
		if (gfx_index == MAX_GFX_ELEMENTS)
		{
			TC0480SCP_vh_stop();
			return 1;
		}

		/* create the char set (gfx will then be updated dynamically from RAM) */
		Machine->gfx[gfx_index] = decodegfx((UINT8 *)TC0480SCP_char_ram,&TC0480SCP_charlayout);
		if (!Machine->gfx[gfx_index])
			return 1;

		/* set the color information */
		Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[gfx_index]->total_colors = 64;

		TC0480SCP_tx_gfx = gfx_index;

		/* use the given gfx set for bg tiles */
		TC0480SCP_bg_gfx = gfxnum;

		for (i=0;i<2;i++)
		{
			tilemap_set_transparent_pen(TC0480SCP_tilemap[0][i],0);
			tilemap_set_transparent_pen(TC0480SCP_tilemap[1][i],0);
			tilemap_set_transparent_pen(TC0480SCP_tilemap[2][i],0);
			tilemap_set_transparent_pen(TC0480SCP_tilemap[3][i],0);
			tilemap_set_transparent_pen(TC0480SCP_tilemap[4][i],0);
		}

		TC0480SCP_x_offs = x_offset + pixels;
		TC0480SCP_y_offs = y_offset;

		xd = -TC0480SCP_x_offs;
		yd =  TC0480SCP_y_offs;

		/* Metalb and Deadconx have minor screenflip issues: blue planet
		   is off on x axis by 1 and in Deadconx the dark blue screen
		   between stages also seems off by 1 pixel. */

		/* It's not possible to get the text scrolldx calculations
		   harmonised with the other layers: xd-2, 315-xd is the
		   next valid pair:- the numbers diverge from xd, 319-xd */

		/* Single width offsets */
		tilemap_set_scrolldx(TC0480SCP_tilemap[0][0], xd,   320-xd +TC0480SCP_flip_xoffs );
		tilemap_set_scrolldy(TC0480SCP_tilemap[0][0], yd,   256-yd +TC0480SCP_flip_yoffs );
		tilemap_set_scrolldx(TC0480SCP_tilemap[1][0], xd,   320-xd +TC0480SCP_flip_xoffs );
		tilemap_set_scrolldy(TC0480SCP_tilemap[1][0], yd,   256-yd +TC0480SCP_flip_yoffs );
		tilemap_set_scrolldx(TC0480SCP_tilemap[2][0], xd,   320-xd +TC0480SCP_flip_xoffs );
		tilemap_set_scrolldy(TC0480SCP_tilemap[2][0], yd,   256-yd +TC0480SCP_flip_yoffs );
		tilemap_set_scrolldx(TC0480SCP_tilemap[3][0], xd,   320-xd +TC0480SCP_flip_xoffs );
		tilemap_set_scrolldy(TC0480SCP_tilemap[3][0], yd,   256-yd +TC0480SCP_flip_yoffs );
		tilemap_set_scrolldx(TC0480SCP_tilemap[4][0], xd-3, 316-xd);	/* text layer */
		tilemap_set_scrolldy(TC0480SCP_tilemap[4][0], yd,   256-yd);	/* text layer */

		/* Double width offsets */
		tilemap_set_scrolldx(TC0480SCP_tilemap[0][1], xd,   320-xd +TC0480SCP_flip_xoffs );
		tilemap_set_scrolldy(TC0480SCP_tilemap[0][1], yd,   256-yd +TC0480SCP_flip_yoffs );
		tilemap_set_scrolldx(TC0480SCP_tilemap[1][1], xd,   320-xd +TC0480SCP_flip_xoffs );
		tilemap_set_scrolldy(TC0480SCP_tilemap[1][1], yd,   256-yd +TC0480SCP_flip_yoffs );
		tilemap_set_scrolldx(TC0480SCP_tilemap[2][1], xd,   320-xd +TC0480SCP_flip_xoffs );
		tilemap_set_scrolldy(TC0480SCP_tilemap[2][1], yd,   256-yd +TC0480SCP_flip_yoffs );
		tilemap_set_scrolldx(TC0480SCP_tilemap[3][1], xd,   320-xd +TC0480SCP_flip_xoffs );
		tilemap_set_scrolldy(TC0480SCP_tilemap[3][1], yd,   256-yd +TC0480SCP_flip_yoffs );
		tilemap_set_scrolldx(TC0480SCP_tilemap[4][1], xd-3, 317-xd);	/* text layer */
		tilemap_set_scrolldy(TC0480SCP_tilemap[4][1], yd,   256-yd);	/* text layer */

		for (i=0;i<2;i++)
		{
			/* Both sets of bg tilemaps scrollable per pixel row */
			tilemap_set_scroll_rows(TC0480SCP_tilemap[0][i],512);
			tilemap_set_scroll_rows(TC0480SCP_tilemap[1][i],512);
			tilemap_set_scroll_rows(TC0480SCP_tilemap[2][i],512);
			tilemap_set_scroll_rows(TC0480SCP_tilemap[3][i],512);
		}

	return 0;
}

void TC0480SCP_vh_stop(void)
{
	free(TC0480SCP_ram);
	TC0480SCP_ram = 0;
	free(TC0480SCP_char_dirty);
	TC0480SCP_char_dirty = 0;
}

READ32_HANDLER( TC0480SCP_ctrl_long_r )
{
	return (TC0480SCP_ctrl_word_r(offset*2,0)<<16)|TC0480SCP_ctrl_word_r(offset*2+1,0);
}

/* TODO: byte access ? */

WRITE32_HANDLER( TC0480SCP_ctrl_long_w )
{
	if (ACCESSING_MSW32) TC0480SCP_ctrl_word_w(offset*2,data>>16,mem_mask>>16);
	if (ACCESSING_LSW32) TC0480SCP_ctrl_word_w((offset*2)+1,data&0xffff,mem_mask&0xffff);
}

READ32_HANDLER( TC0480SCP_long_r )
{
	return (TC0480SCP_word_r(offset*2,0)<<16)|TC0480SCP_word_r(offset*2+1,0);
}

WRITE32_HANDLER( TC0480SCP_long_w )
{
	if (((mem_mask & 0xff000000) == 0) || ((mem_mask & 0x00ff0000) == 0))
	{
		int oldword = TC0480SCP_word_r(offset*2,0);
		int newword = data>>16;
		if ((mem_mask & 0x00ff0000) != 0)
			newword |= (oldword &0x00ff);
		if ((mem_mask & 0xff000000) != 0)
			newword |= (oldword &0xff00);
		TC0480SCP_word_w(offset*2,newword,0);
	}
	if (((mem_mask & 0x0000ff00) == 0) || ((mem_mask & 0x000000ff) == 0))
	{
		int oldword = TC0480SCP_word_r((offset*2)+1,0);
		int newword = data&0xffff;
		if ((mem_mask & 0x000000ff) != 0)
			newword |= (oldword &0x00ff);
		if ((mem_mask & 0x0000ff00) != 0)
			newword |= (oldword &0xff00);
		TC0480SCP_word_w((offset*2)+1,newword,0);
	}
}

READ16_HANDLER( TC0480SCP_word_r )
{
	return TC0480SCP_ram[offset];
}

static void TC0480SCP_word_write(offs_t offset,data16_t data,UINT32 mem_mask)
{
	int oldword = TC0480SCP_ram[offset];
	COMBINE_DATA(&TC0480SCP_ram[offset]);

	if (oldword != TC0480SCP_ram[offset])
	{
		if (!TC0480SCP_dblwidth)
		{
			if (offset < 0x2000)
			{
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[(offset /
					0x800)][TC0480SCP_dblwidth],((offset % 0x800) / 2));
			}
			else if (offset < 0x6000)
			{   /* do nothing */
			}
			else if (offset < 0x7000)
			{
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[4][TC0480SCP_dblwidth],
					(offset - 0x6000));
			}
			else if (offset <= 0x7fff)
			{
				TC0480SCP_char_dirty[(offset - 0x7000) / 16] = 1;
				TC0480SCP_chars_dirty = 1;
			}
		}
		else
		{
			if (offset < 0x4000)
			{
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[(offset /
					0x1000)][TC0480SCP_dblwidth],((offset % 0x1000) / 2));
			}
			else if (offset < 0x6000)
			{   /* do nothing */
			}
			else if (offset < 0x7000)
			{
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[4][TC0480SCP_dblwidth],
					(offset - 0x6000));
			}
			else if (offset <= 0x7fff)
			{
				TC0480SCP_char_dirty[(offset - 0x7000) / 16] = 1;
				TC0480SCP_chars_dirty = 1;
			}
		}
	}
}

WRITE16_HANDLER( TC0480SCP_word_w )
{
	TC0480SCP_word_write(offset,data,mem_mask);
}

READ16_HANDLER( TC0480SCP_ctrl_word_r )
{
	return TC0480SCP_ctrl[offset];
}

static void TC0480SCP_ctrl_word_write(offs_t offset,data16_t data,UINT32 mem_mask)
{
	int flip = TC0480SCP_pri_reg & 0x40;

	COMBINE_DATA(&TC0480SCP_ctrl[offset]);
	data = TC0480SCP_ctrl[offset];

	switch( offset )
	{
		/* The x offsets of the four bg layers are staggered by intervals of 4 pixels */

		case 0x00:   /* bg0 x */
			if (!flip)  data = -data;
			TC0480SCP_bgscrollx[0] = data;
			break;

		case 0x01:   /* bg1 x */
			data += 4;
			if (!flip)  data = -data;
			TC0480SCP_bgscrollx[1] = data;
			break;

		case 0x02:   /* bg2 x */
			data += 8;
			if (!flip)  data = -data;
			TC0480SCP_bgscrollx[2] = data;
			break;

		case 0x03:   /* bg3 x */
			data += 12;
			if (!flip)  data = -data;
			TC0480SCP_bgscrollx[3] = data;
			break;

		case 0x04:   /* bg0 y */
			if (flip)  data = -data;
			TC0480SCP_bgscrolly[0] = data;
			break;

		case 0x05:   /* bg1 y */
			if (flip)  data = -data;
			TC0480SCP_bgscrolly[1] = data;
			break;

		case 0x06:   /* bg2 y */
			if (flip)  data = -data;
			TC0480SCP_bgscrolly[2] = data;
			break;

		case 0x07:   /* bg3 y */
			if (flip)  data = -data;
			TC0480SCP_bgscrolly[3] = data;
			break;

		case 0x08:   /* bg0 zoom */
		case 0x09:   /* bg1 zoom */
		case 0x0a:   /* bg2 zoom */
		case 0x0b:   /* bg3 zoom */
			break;

		case 0x0c:   /* fg (text) x */

			/* Text layer can be offset from bg0 (e.g. Metalb) */
			if (!flip)	data -= TC0480SCP_text_xoffs;
			if (flip)	data += TC0480SCP_text_xoffs;

			tilemap_set_scrollx(TC0480SCP_tilemap[4][0], 0, -data);
			tilemap_set_scrollx(TC0480SCP_tilemap[4][1], 0, -data);
			break;

		case 0x0d:   /* fg (text) y */

			/* Text layer can be offset from bg0 (e.g. Slapshot) */
			if (!flip)	data -= TC0480SCP_text_yoffs;
			if (flip)	data += TC0480SCP_text_yoffs;

			tilemap_set_scrolly(TC0480SCP_tilemap[4][0], 0, -data);
			tilemap_set_scrolly(TC0480SCP_tilemap[4][1], 0, -data);
			break;

		/* offset 0x0e unused */

		case 0x0f:   /* control register */
		{
			int old_width = (TC0480SCP_pri_reg &0x80) >> 7;
			flip = (data & 0x40) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;
			TC0480SCP_pri_reg = data;

			tilemap_set_flip(TC0480SCP_tilemap[0][0],flip);
			tilemap_set_flip(TC0480SCP_tilemap[1][0],flip);
			tilemap_set_flip(TC0480SCP_tilemap[2][0],flip);
			tilemap_set_flip(TC0480SCP_tilemap[3][0],flip);
			tilemap_set_flip(TC0480SCP_tilemap[4][0],flip);

			tilemap_set_flip(TC0480SCP_tilemap[0][1],flip);
			tilemap_set_flip(TC0480SCP_tilemap[1][1],flip);
			tilemap_set_flip(TC0480SCP_tilemap[2][1],flip);
			tilemap_set_flip(TC0480SCP_tilemap[3][1],flip);
			tilemap_set_flip(TC0480SCP_tilemap[4][1],flip);

			TC0480SCP_dblwidth = (TC0480SCP_pri_reg &0x80) >> 7;

			if (TC0480SCP_dblwidth != old_width)	/* tilemap width is changing */
			{
				/* Reinitialise layer pointers */
				TC0480SCP_set_layer_ptrs();

				/* and ensure full redraw of tilemaps */
				TC0480SCP_dirty_tilemaps();
			}

			break;
		}

		/* Rest are layer specific delta x and y, used while scrolling that layer */
	}
}

void TC0480SCP_mark_transparent_colors(int opaque_layer)
{
	int i,layer;

	/* We really only need to mark the two custom layers */
	for (layer=0; layer<4; layer++)
	{
		if (layer==opaque_layer) continue; /* Don't mark opaque layer colors */

		for (i=0x800*(TC0480SCP_dblwidth+1)*layer; i<(0x800*(TC0480SCP_dblwidth+1)*layer) +
			(TC0480SCP_dblwidth+1)*0x800; i+=2)
		{
			data16_t color = TC0480SCP_ram[i] & 0xff;
			palette_used_colors[color*16] = PALETTE_COLOR_TRANSPARENT;
		}
	}
}

WRITE16_HANDLER( TC0480SCP_ctrl_word_w )
{
	TC0480SCP_ctrl_word_write(offset,data,mem_mask);
}

void TC0480SCP_tilemap_update(void)
{
	int layer, zoom, i, j;
	int flip = TC0480SCP_pri_reg & 0x40;

	for (layer = 0; layer < 4; layer++)
	{
		tilemap_set_scrolly(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],0,TC0480SCP_bgscrolly[layer]);
		zoom = 0x10000 + 0x7f - TC0480SCP_ctrl[0x08 + layer];

		if (zoom != 0x10000)	/* can't use scroll rows when zooming */
		{
			tilemap_set_scrollx(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],
					0, TC0480SCP_bgscrollx[layer]);
		}
		else
		{
			for (j = 0;j < 512;j++)
			{
				i = TC0480SCP_bgscroll_ram[layer][j];

				if (!flip)
				tilemap_set_scrollx(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],
						j & 0x1ff,
						TC0480SCP_bgscrollx[layer] - i);
				if (flip)
				tilemap_set_scrollx(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],
						j & 0x1ff,
						TC0480SCP_bgscrollx[layer] + i);
			}
		}
	}

	/* Decode any characters that have changed */
	if (TC0480SCP_chars_dirty)
	{
		int tile_index;

		for (tile_index = 0;tile_index < 64*64;tile_index++)
		{
			int attr = TC0480SCP_tx_ram[tile_index];
			if (TC0480SCP_char_dirty[attr & 0xff])
				tilemap_mark_tile_dirty(TC0480SCP_tilemap[4][TC0480SCP_dblwidth],tile_index);
		}

		for (j = 0;j < TC0480SCP_TOTAL_CHARS;j++)
		{
			if (TC0480SCP_char_dirty[j])
				decodechar(Machine->gfx[TC0480SCP_tx_gfx],j,
					(UINT8 *)TC0480SCP_char_ram,&TC0480SCP_charlayout);
			TC0480SCP_char_dirty[j] = 0;
		}
		TC0480SCP_chars_dirty = 0;
	}

	tilemap_update(TC0480SCP_tilemap[0][TC0480SCP_dblwidth]);
	tilemap_update(TC0480SCP_tilemap[1][TC0480SCP_dblwidth]);
	tilemap_update(TC0480SCP_tilemap[2][TC0480SCP_dblwidth]);
	tilemap_update(TC0480SCP_tilemap[3][TC0480SCP_dblwidth]);
	tilemap_update(TC0480SCP_tilemap[4][TC0480SCP_dblwidth]);
}


/*********************************************************************
				BG0,1 LAYER DRAW

TODO
----

Broken for any rotation except ROT0. ROT180 support could probably
be added without too much difficulty: machine_flip is there as a
place-holder for this purpose.

Wouldn't work if y needs to be > 255 (i.e. if some game uses a
bigger than usual vertical visible area). Refer to TC0080VCO
custom draw routine for an example of dealing with this.


Historical Issues
-----------------

1) bg layers got too far left and down, the greater the magnification.
   Largely fixed by adding offsets (to sx&y) which get bigger as we
   zoom in (why we have *zoomx and *zoomy in the calculations).

2) Hthero and Footchmp bg layers behaved differently when zoomed.
   Fixed by bringing tc0480scp_x&y_offs into calculations.

3) Metalb "TAITO" text in attract too far to the right. Fixed by
   bringing (layer*4) into offset calculations. But might be possible
   to avoid this by stepping the scroll deltas for the four layers -
   currently they are the same, and we have to kludge the offsets in
   TC0480SCP_ctrl_word_write.

4) Zoom movement was jagged: improved by bringing in scroll delta
   values... but the results are noticably imperfect.

**********************************************************************/

static void TC0480SCP_bg01_draw(struct osd_bitmap *bitmap,int layer,int flags,UINT32 priority)
{
	/* X-axis zoom offers expansion only: 0 = no zoom, 0xff = max
	   Y-axis zoom offers expansion/compression: 0x7f = no zoom, 0xff = max
	   (0x1a in Footchmp hiscore = shrunk) */

	int zoomx = 0x10000 - (TC0480SCP_ctrl[0x08 + layer] &0xff00);
	int zoomy = 0x10000 - (((TC0480SCP_ctrl[0x08 + layer] &0xff) - 0x7f) * 512);

	if ((zoomx == 0x10000) && (zoomy == 0x10000))	/* no zoom, simple */
	{
		/* Prevent bad things */
		tilemap_set_clip(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],&Machine->visible_area);

		tilemap_draw(bitmap,TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],flags,priority);
	}
	else	/* zoom */
	{
		UINT8  *dst8, *src8;
		UINT16 *dst16,*src16;
/*		UINT32 *dst32,*src32;		   in future add 24/32 bit color support    */
		UINT8  scanline8[512];
		UINT16 scanline16[512];
		UINT32 sx;
		struct osd_bitmap *srcbitmap = tilemap_get_pixmap(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth]);
		int flip = TC0480SCP_pri_reg & 0x40;
		int y,y_index,src_y_index,row_index;
		int x_index,x_step,x_max;
		int rot=Machine->orientation;
		int machine_flip = 0;	/* for  ROT 180 ? */

		UINT16 screen_width = Machine->visible_area.max_x -
							Machine->visible_area.min_x + 1;
/*		UINT16 min_y = Machine->visible_area.min_y; */
/*		UINT16 max_y = Machine->visible_area.max_y; */

		int width_mask=0x1ff;
		if (TC0480SCP_dblwidth)	width_mask=0x3ff;

		tilemap_set_clip(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth],0);


		if (!flip)
		{
			sx = ((TC0480SCP_bgscrollx[layer] + 15 + layer*4) << 16)
				+ ((255-(TC0480SCP_ctrl[0x10 + layer] & 0xff)) << 8);
			sx += (TC0480SCP_x_offs - 15 - layer*4) * zoomx;

			if (rot &ORIENTATION_FLIP_X)	/* orientation flip X (Gunbustr) */
				sx = -sx -((screen_width + TC0480SCP_flip_xoffs) * zoomx);

			y_index = (TC0480SCP_bgscrolly[layer] << 16)
				+ ((TC0480SCP_ctrl[0x14 + layer] & 0xff) << 8);
			y_index -= (TC0480SCP_y_offs) * zoomy;
		}
		else	/* TC0480SCP tiles flipscreen */
		{
			sx = ((-TC0480SCP_bgscrollx[layer] + 15 + layer*4 + TC0480SCP_flip_xoffs ) << 16)
				+ ((255-(TC0480SCP_ctrl[0x10 + layer] & 0xff)) << 8);
			sx += (TC0480SCP_x_offs - 15 - layer*4) * zoomx;

			if (rot &ORIENTATION_FLIP_X)	/* orientation flip X (untested) */
				sx = -sx -((screen_width + TC0480SCP_flip_xoffs) * zoomx);

			y_index = ((-TC0480SCP_bgscrolly[layer] + TC0480SCP_flip_yoffs) << 16)
				+ ((TC0480SCP_ctrl[0x14 + layer] & 0xff) << 8);
			y_index -= (TC0480SCP_y_offs) * zoomy;
		}


		if (!machine_flip) y=0; else y=255;

		if (Machine->scrbitmap->depth == 8)
		{
			do
			{
				src_y_index = (y_index>>16) &0x1ff;

				/* row areas are the same in flipscreen, so we must read in reverse */
				row_index = src_y_index;
				if (flip)	row_index = 0x1ff - row_index;

				if ((rot &ORIENTATION_FLIP_X)==0)
				{
					x_index = sx - ((TC0480SCP_bgscroll_ram[layer][row_index] << 16))
						- ((TC0480SCP_bgscroll_ram[layer][row_index+0x800] << 8) &0xffff);
				}
				else	/* Orientation flip X (Gunbustr) */
				{
					x_index = sx + ((TC0480SCP_bgscroll_ram[layer][row_index] << 16))
						+ ((TC0480SCP_bgscroll_ram[layer][row_index+0x800] << 8) &0xffff);
				}

				src8 = (UINT8 *)srcbitmap->line[src_y_index];
				dst8 = scanline8;

				x_step = zoomx;

				x_max = x_index + screen_width * x_step;

				while (x_index<x_max)
				{
					*dst8++ = src8[(x_index >> 16) &width_mask];
					x_index += x_step;
				}

				if ((rot &ORIENTATION_FLIP_X)!=0)
					pdraw_scanline8(bitmap,512 - screen_width/2,y,screen_width,
						scanline8,0,palette_transparent_pen,rot,priority);
				else
					pdraw_scanline8(bitmap,0,y,screen_width,
						scanline8,0,palette_transparent_pen,rot,priority);

				y_index += zoomy;
				if (!machine_flip) y++; else y--;
			}
			while ( (!machine_flip && y<256) || (machine_flip && y>=0) );
		}
		else if (Machine->scrbitmap->depth == 16)
		{
			do
			{
				src_y_index = (y_index>>16) &0x1ff;

				/* row areas are the same in flipscreen, so we must read in reverse */
				row_index = src_y_index;
				if (flip)	row_index = 0x1ff - row_index;

				if ((rot &ORIENTATION_FLIP_X)==0)
				{
					x_index = sx - ((TC0480SCP_bgscroll_ram[layer][row_index] << 16))
						- ((TC0480SCP_bgscroll_ram[layer][row_index+0x800] << 8) &0xffff);
				}
				else	/* Orientation flip X (Gunbustr) */
				{
					x_index = sx + ((TC0480SCP_bgscroll_ram[layer][row_index] << 16))
						+ ((TC0480SCP_bgscroll_ram[layer][row_index+0x800] << 8) &0xffff);
				}

				src16 = (UINT16 *)srcbitmap->line[src_y_index];
				dst16 = scanline16;

				x_step = zoomx;

				x_max = x_index + screen_width * x_step;

				while (x_index<x_max)
				{
					*dst16++ = src16[(x_index >> 16) &width_mask];
					x_index += x_step;
				}

				if ((rot &ORIENTATION_FLIP_X)!=0)
					pdraw_scanline16(bitmap,512 - screen_width/2,y,screen_width,
						scanline16,0,palette_transparent_pen,rot,priority);
				else
					pdraw_scanline16(bitmap,0,y,screen_width,
						scanline16,0,palette_transparent_pen,rot,priority);

				y_index += zoomy;
				if (!machine_flip) y++; else y--;
			}
			while ( (!machine_flip && y<256) || (machine_flip && y>=0) );
		}
	}
}


/****************************************************************
				BG2,3 LAYER DRAW

TODO
----

Broken for any rotation except ROT0. ROT180 support could probably
be added without too much difficulty: machine_flip is there as a
place-holder for this purpose.

Wouldn't work if y needs to be > 255 (i.e. if some game uses a
bigger than usual vertical visible area). Refer to TC0080VCO
custom draw routine for an example of dealing with this.

Low order words for overall layer zoom are not really understood.
In Metalbj initial text screen zoom you can see they ARE words
(not separate bytes); however, I just use the low byte to smooth
the zooming sequences. This is noticeably imperfect on the Y axis.

Verify behaviour of Taito logo (Gunbustr) against real machine
to perfect the row zoom emulation.

What do high bytes of row zoom do - if anything - in UndrFire?
There is still jaggedness to the road in this game and Superchs.


Historical Issues
-----------------

Sometimes BG2/3 were misaligned by 1 pixel horizontally: this
was due to low order byte of 0 causing different (sx >> 16) than
when it was 1-255. To prevent this we use (255-byte) so
(sx >> 16) no longer depends on the low order byte.

In flipscreen we have to bring in extra offsets, since various
games don't have exactly (320-,256-) tilemap scroll deltas in
flipscreen.

****************************************************************/

static void TC0480SCP_bg23_draw(struct osd_bitmap *bitmap,int layer,int flags,UINT32 priority)
{
	struct osd_bitmap *srcbitmap = tilemap_get_pixmap(TC0480SCP_tilemap[layer][TC0480SCP_dblwidth]);
	UINT8  *dst8, *src8;
	UINT16 *dst16,*src16;
/*	UINT32 *dst32,*src32;		   in future add 24/32 bit color support    */
	int y,y_index,src_y_index,row_index,row_zoom;
	int sx,x_index,x_step,x_max;
	UINT32 zoomx,zoomy,rot=Machine->orientation;
	UINT8 scanline8[512];
	UINT16 scanline[512];
	int flipscreen = TC0480SCP_pri_reg & 0x40;
	int machine_flip = 0;	/* for  ROT 180 ? */

	UINT16 screen_width = Machine->visible_area.max_x -
							Machine->visible_area.min_x + 1;
/*	UINT16 min_y = Machine->visible_area.min_y; */
/*	UINT16 max_y = Machine->visible_area.max_y; */

	int width_mask=0x1ff;
	if (TC0480SCP_dblwidth)	width_mask=0x3ff;

	/* X-axis zoom offers expansion only: 0 = no zoom, 0xff = max
	   Y-axis zoom offers expansion/compression: 0x7f = no zoom, 0xff = max
	   (0x1a in Footchmp hiscore = shrunk) */

	zoomx = 0x10000 - (TC0480SCP_ctrl[0x08 + layer] &0xff00);
	zoomy = 0x10000 - (((TC0480SCP_ctrl[0x08 + layer] &0xff) - 0x7f) * 512);

	if (!flipscreen)
	{
		sx = ((TC0480SCP_bgscrollx[layer] + 15 + layer*4) << 16)
			+ ((255-(TC0480SCP_ctrl[0x10 + layer] & 0xff)) << 8);
		sx += (TC0480SCP_x_offs - 15 - layer*4) * zoomx;

		if (rot &ORIENTATION_FLIP_X)	/* orientation flip X (Gunbustr) */
			sx = -sx -((screen_width + TC0480SCP_flip_xoffs) * zoomx);

		y_index = (TC0480SCP_bgscrolly[layer] << 16)
			+ ((TC0480SCP_ctrl[0x14 + layer] & 0xff) << 8);
		y_index -= (TC0480SCP_y_offs) * zoomy;
	}
	else	/* TC0480SCP tiles flipscreen */
	{
		sx = ((-TC0480SCP_bgscrollx[layer] + 15 + layer*4 + TC0480SCP_flip_xoffs ) << 16)
			+ ((255-(TC0480SCP_ctrl[0x10 + layer] & 0xff)) << 8);
		sx += (TC0480SCP_x_offs - 15 - layer*4) * zoomx;

		if (rot &ORIENTATION_FLIP_X)	/* orientation flip X (untested) */
			sx = -sx -((screen_width + TC0480SCP_flip_xoffs) * zoomx);

		y_index = ((-TC0480SCP_bgscrolly[layer] + TC0480SCP_flip_yoffs) << 16)
			+ ((TC0480SCP_ctrl[0x14 + layer] & 0xff) << 8);
		y_index -= (TC0480SCP_y_offs) * zoomy;
	}


	if (!machine_flip) y=0; else y=255;

	if (Machine->scrbitmap->depth == 8)
	{
		do
		{
			if (!flipscreen)
				src_y_index = ((y_index>>16) + TC0480SCP_bgcolumn_ram[layer][(y -
							TC0480SCP_y_offs) &0x1ff]) &0x1ff;
			else	/* colscroll area is back to front in flipscreen */
				src_y_index = ((y_index>>16) + TC0480SCP_bgcolumn_ram[layer][0x1ff -
							((y - TC0480SCP_y_offs) &0x1ff)]) &0x1ff;

			/* row areas are the same in flipscreen, so we must read in reverse */
			row_index = src_y_index;
			if (flipscreen)	row_index = 0x1ff - row_index;

			if (TC0480SCP_pri_reg & (layer-1))	/* bit0 enables for BG2, bit1 for BG3 */
				row_zoom = TC0480SCP_rowzoom_ram[layer][row_index];
			else
				row_zoom = 0;

			if ((rot &ORIENTATION_FLIP_X)==0)
			{
				x_index = sx - ((TC0480SCP_bgscroll_ram[layer][row_index] << 16))
					- ((TC0480SCP_bgscroll_ram[layer][row_index+0x800] << 8) &0xffff);

				/* flawed calc ?? */
				x_index -= (TC0480SCP_x_offs - 0x1f + layer*4) * ((row_zoom &0xff) << 8);
			}
			else	/* Orientation flip X (Gunbustr) */
			{
				x_index = sx + ((TC0480SCP_bgscroll_ram[layer][row_index] << 16))
					+ ((TC0480SCP_bgscroll_ram[layer][row_index+0x800] << 8) &0xffff);

				/* flawed calc ?? */
				x_index += (TC0480SCP_x_offs - 0x1f + layer*4) * ((row_zoom &0xff) << 8);
			}

			x_step = zoomx;
			if (row_zoom)	/* need to reduce x_step */
			{
				if (!(row_zoom &0xff00))
					x_step -= ((row_zoom * 256) &0xffff);
				else	/* Undrfire uses the hi byte, why? */
					x_step -= (((row_zoom &0xff) * 256) &0xffff);

				if ((rot &ORIENTATION_FLIP_X)!=0)
				{
					x_index += (screen_width + TC0480SCP_flip_xoffs) *
						((row_zoom * 256) &0xffff);
				}
			}

			x_max = x_index + screen_width * x_step;
			src8 = (UINT8 *)srcbitmap->line[src_y_index];
			dst8 = scanline8;

			while (x_index<x_max)
			{
				*dst8++ = src8[(x_index >> 16) &width_mask];
				x_index += x_step;
			}

			if ((rot &ORIENTATION_FLIP_X)!=0)
				pdraw_scanline8(bitmap,512 - screen_width/2,y,screen_width,
					scanline8,0,palette_transparent_pen,rot,priority);
			else
				pdraw_scanline8(bitmap,0,y,screen_width,
					scanline8,0,palette_transparent_pen,rot,priority);

			y_index += zoomy;
			if (!machine_flip) y++; else y--;
		}
		while ( (!machine_flip && y<256) || (machine_flip && y>=0) );
	}
	else if (Machine->scrbitmap->depth == 16)
	{
		do
		{
			if (!flipscreen)
				src_y_index = ((y_index>>16) + TC0480SCP_bgcolumn_ram[layer][(y -
							TC0480SCP_y_offs) &0x1ff]) &0x1ff;
			else	/* colscroll area is back to front in flipscreen */
				src_y_index = ((y_index>>16) + TC0480SCP_bgcolumn_ram[layer][0x1ff -
							((y - TC0480SCP_y_offs) &0x1ff)]) &0x1ff;

			/* row areas are the same in flipscreen, so we must read in reverse */
			row_index = src_y_index;
			if (flipscreen)	row_index = 0x1ff - row_index;

			if (TC0480SCP_pri_reg & (layer-1))	/* bit0 enables for BG2, bit1 for BG3 */
				row_zoom = TC0480SCP_rowzoom_ram[layer][row_index];
			else
				row_zoom = 0;

			if ((rot &ORIENTATION_FLIP_X)==0)
			{
				x_index = sx - ((TC0480SCP_bgscroll_ram[layer][row_index] << 16))
					- ((TC0480SCP_bgscroll_ram[layer][row_index+0x800] << 8) &0xffff);

				/* flawed calc ?? */
				x_index -= (TC0480SCP_x_offs - 0x1f + layer*4) * ((row_zoom &0xff) << 8);
			}
			else	/* Orientation flip X (Gunbustr) */
			{
				x_index = sx + ((TC0480SCP_bgscroll_ram[layer][row_index] << 16))
					+ ((TC0480SCP_bgscroll_ram[layer][row_index+0x800] << 8) &0xffff);

				/* flawed calc ?? */
				x_index += (TC0480SCP_x_offs - 0x1f + layer*4) * ((row_zoom &0xff) << 8);
			}

/* We used to kludge 270 multiply factor, before adjusting x_index instead */

			x_step = zoomx;
			if (row_zoom)	/* need to reduce x_step */
			{
				if (!(row_zoom &0xff00))
					x_step -= ((row_zoom * 256) &0xffff);
				else	/* Undrfire uses the hi byte, why? */
					x_step -= (((row_zoom &0xff) * 256) &0xffff);

				if ((rot &ORIENTATION_FLIP_X)!=0)
				{
					x_index += (screen_width + TC0480SCP_flip_xoffs) *
						((row_zoom * 256) &0xffff);
				}
			}

			x_max = x_index + screen_width * x_step;
			src16 = (UINT16 *)srcbitmap->line[src_y_index];
			dst16 = scanline;

			while (x_index<x_max)
			{
				*dst16++ = src16[(x_index >> 16) &width_mask];
				x_index += x_step;
			}

			if ((rot &ORIENTATION_FLIP_X)!=0)
				pdraw_scanline16(bitmap,512 - screen_width/2,y,screen_width,
					scanline,0,palette_transparent_pen,rot,priority);
			else
				pdraw_scanline16(bitmap,0,y,screen_width,
					scanline,0,palette_transparent_pen,rot,priority);

			y_index += zoomy;
			if (!machine_flip) y++; else y--;
		}
		while ( (!machine_flip && y<256) || (machine_flip && y>=0) );
	}
}



void TC0480SCP_tilemap_draw(struct osd_bitmap *bitmap,int layer,int flags,UINT32 priority)
{
	/* no layer disable bits */

	switch (layer)
	{
		case 0:
			TC0480SCP_bg01_draw(bitmap,0,flags,priority);
			break;
		case 1:
			TC0480SCP_bg01_draw(bitmap,1,flags,priority);
			break;
		case 2:
			TC0480SCP_bg23_draw(bitmap,2,flags,priority);
			break;
		case 3:
			TC0480SCP_bg23_draw(bitmap,3,flags,priority);
			break;
		case 4:
			tilemap_draw(bitmap,TC0480SCP_tilemap[4][TC0480SCP_dblwidth],flags,priority);
			break;
	}
}

/***************************************************************

Old TC0480SCP bg layer priority table (kept for reference)

	mb = seen during metal black game
	ss = seen during slap shot game
	{ 0, 1, 2, 3, },	// 0x00  00000  mb ss [text screens, but Deadconx confirms layer order]
	{ 0, 1, 2, 3, },	// 0x01  00001
	{ 0, 1, 2, 3, },	// 0x02  00010
	{ 0, 1, 2, 3, },	// 0x03  00011  mb    [all boss dies]
	{ 0, 1, 2, 3, },	// 0x04  00100
	{ 0, 1, 2, 3, },	// 0x05  00101
	{ 0, 1, 2, 3, },	// 0x06  00110
	{ 0, 1, 2, 3, },	// 0x07  00111
	{ 0, 1, 2, 3, },	// 0x08  01000
	{ 0, 1, 2, 3, },	// 0x09  01001
	{ 0, 1, 2, 3, },	// 0x0a  01010
	{ 0, 1, 2, 3, },	// 0x0b  01011
	{ 3, 0, 1, 2, },	// 0x0c  01100  mb    [round3 boss; bg0 undefined (was 0312)]
	{ 3, 0, 1, 2, },	// 0x0d  01101
	{ 3, 0, 1, 2, },	// 0x0e  01110
	{ 3, 0, 1, 2, },	// 0x0f  01111  mb    [second part of round5]
	{ 3, 2, 1, 0, },	// 0x10  10000  mb ss [round 6 start (was 3012; 0/1 or 1/0?)]
	{ 3, 2, 1, 0, },	// 0x11  10001     ss (1/0 or 0/1?)
	{ 3, 2, 1, 0, },	// 0x12  10010  mb    [round1 start]
	{ 3, 2, 1, 0, },	// 0x13  10011  mb    [final boss, round2/4 start & demo bonus] (2/1 then 0, bg3 undefined)
	{ 0, 1, 2, 3, },	// 0x14  10100  mb    [copyright screen, bg0 undefined]
	{ 0, 1, 2, 3, },	// 0x15  10101
	{ 0, 1, 2, 3, },	// 0x16  10110
	{ 0, 1, 2, 3, },	// 0x17  10111  mb    [round4 boss; bg0/1 undefined]
	{ 0, 1, 2, 3, },	// 0x18  11000
	{ 0, 1, 2, 3, },	// 0x19  11001
	{ 0, 1, 2, 3, },	// 0x1a  11010
	{ 0, 1, 2, 3, },	// 0x1b  11011
	{ 0, 3, 2, 1, },	// 0x1c  11100  mb    [late round 3; bg1/2/3 posns interchangeable (was 0123)]
	{ 0, 3, 2, 1, },	// 0x1d  11101
	{ 0, 3, 2, 1, },	// 0x1e  11110  mb    [first part of round5]
	{ 0, 3, 2, 1, },	// 0x1f  11111  mb    [first part of round3]
};

**************************************************************/

static UINT16 TC0480SCP_bg_pri_lookup[8] =
{
	0x0123,
	0x1230,
	0x2301,
	0x3012,
	0x3210,
	0x2103,
	0x1032,
	0x0321
};

int TC0480SCP_get_bg_priority(void)
{
	return TC0480SCP_bg_pri_lookup[(TC0480SCP_pri_reg &0x1c) >> 2];
}


/***************************************************************************/


static int TC0110PCR_type = 0;
static int TC0110PCR_addr[3];
static data16_t *TC0110PCR_ram[3];
#define TC0110PCR_RAM_SIZE 0x2000


void TC0110PCR_restore_colors(int chip)
{
	int i,color,r=0,g=0,b=0;

	for (i=0; i<(256*16); i++)
	{
		color = TC0110PCR_ram[chip][i];

		switch (TC0110PCR_type)
		{

			case 0x00:
			{
				r = (color >>  0) & 0x1f;
				g = (color >>  5) & 0x1f;
				b = (color >> 10) & 0x1f;

				r = (r << 3) | (r >> 2);
				g = (g << 3) | (g >> 2);
				b = (b << 3) | (b >> 2);
				break;
			}

			case 0x01:
			{
				b = (color >>  0) & 0x1f;
				g = (color >>  5) & 0x1f;
				r = (color >> 10) & 0x1f;

				r = (r << 3) | (r >> 2);
				g = (g << 3) | (g >> 2);
				b = (b << 3) | (b >> 2);
				break;
			}

			case 0x02:
			{
				r = (color >> 0) & 0xf;
				g = (color >> 4) & 0xf;
				b = (color >> 8) & 0xf;

				r = (r << 4) | r;
				g = (g << 4) | g;
				b = (b << 4) | b;
			}
		}

		palette_change_color( i + (chip << 12),r,g,b);
	}
}

static void TC0110PCR_restore_cols_0(void)
{
	TC0110PCR_restore_colors(0);
}

static void TC0110PCR_restore_cols_1(void)
{
	TC0110PCR_restore_colors(1);
}

static void TC0110PCR_restore_cols_2(void)
{
	TC0110PCR_restore_colors(2);
}


int TC0110PCR_vh_start(void)
{
	TC0110PCR_ram[0] = malloc(TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_ram[0]));

	if (!TC0110PCR_ram[0]) return 1;

	state_save_register_UINT16("TC0110PCR-0", 0, "memory", TC0110PCR_ram[0],
		TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_ram[0]) / 2);
	state_save_register_func_postload(TC0110PCR_restore_cols_0);

	TC0110PCR_type = 0;	/* default, xBBBBBGGGGGRRRRR */

	return 0;
}

int TC0110PCR_1_vh_start(void)
{
	TC0110PCR_ram[1] = malloc(TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_ram[1]));

	if (!TC0110PCR_ram[1]) return 1;

	state_save_register_UINT16("TC0110PCR-1", 0, "memory", TC0110PCR_ram[1],
		TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_ram[1])/2);
	state_save_register_func_postload(TC0110PCR_restore_cols_1);

	return 0;
}

int TC0110PCR_2_vh_start(void)
{
	TC0110PCR_ram[2] = malloc(TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_ram[2]));

	if (!TC0110PCR_ram[2]) return 1;

	state_save_register_UINT16("TC0110PCR-2", 0, "memory", TC0110PCR_ram[2],
		TC0110PCR_RAM_SIZE * sizeof(*TC0110PCR_ram[2])/2);
	state_save_register_func_postload(TC0110PCR_restore_cols_2);

	return 0;
}

void TC0110PCR_vh_stop(void)
{
	free(TC0110PCR_ram[0]);
	TC0110PCR_ram[0] = 0;
}

void TC0110PCR_1_vh_stop(void)
{
	free(TC0110PCR_ram[1]);
	TC0110PCR_ram[1] = 0;
}

void TC0110PCR_2_vh_stop(void)
{
	free(TC0110PCR_ram[2]);
	TC0110PCR_ram[2] = 0;
}

READ16_HANDLER( TC0110PCR_word_r )
{
	switch (offset)
	{
		case 1:
			return TC0110PCR_ram[0][(TC0110PCR_addr[0])];

		default:
logerror("PC %06x: warning - read TC0110PCR address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

READ16_HANDLER( TC0110PCR_word_1_r )
{
	switch (offset)
	{
		case 1:
			return TC0110PCR_ram[1][(TC0110PCR_addr[1])];

		default:
logerror("PC %06x: warning - read second TC0110PCR address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

READ16_HANDLER( TC0110PCR_word_2_r )
{
	switch (offset)
	{
		case 1:
			return TC0110PCR_ram[2][(TC0110PCR_addr[2])];

		default:
logerror("PC %06x: warning - read third TC0110PCR address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE16_HANDLER( TC0110PCR_word_w )
{
	switch (offset)
	{
		case 0:
			/* In test mode game writes to odd register number so (data>>1) */
			TC0110PCR_addr[0] = (data >> 1) & 0xfff;
			if (data>0x1fff) logerror ("Write to palette index > 0x1fff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette BGR value */

			TC0110PCR_ram[0][(TC0110PCR_addr[0])] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			palette_change_color(TC0110PCR_addr[0],r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_word_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_addr[0] = data & 0xfff;
			if (data>0xfff) logerror ("Write to palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette BGR value */

			TC0110PCR_ram[0][(TC0110PCR_addr[0])] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			palette_change_color(TC0110PCR_addr[0],r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_word_1_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_addr[1] = data & 0xfff;
			if (data>0xfff) logerror ("Write to second TC0110PCR palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette RGB value */

			TC0110PCR_ram[1][(TC0110PCR_addr[1])] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			/* change a color in the second color area (4096-8191) */
			palette_change_color(TC0110PCR_addr[1] + 4096,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to second TC0110PCR offset %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_word_2_w )
{
	switch (offset)
	{
		case 0:
			TC0110PCR_addr[2] = data & 0xfff;
			if (data>0xfff) logerror ("Write to third TC0110PCR palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette RGB value */

			TC0110PCR_ram[2][(TC0110PCR_addr[2])] = data & 0xffff;

			r = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			b = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			/* change a color in the second color area (8192-12288) */
			palette_change_color(TC0110PCR_addr[2] + 8192,r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to third TC0110PCR offset %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_rbswap_word_w )
{
	TC0110PCR_type = 1;	/* xRRRRRGGGGGBBBBB */

	switch (offset)
	{
		case 0:
			TC0110PCR_addr[0] = data & 0xfff;
			if (data>0xfff) logerror ("Write to palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette RGB value */

			TC0110PCR_ram[0][(TC0110PCR_addr[0])] = data & 0xffff;

			b = (data >>  0) & 0x1f;
			g = (data >>  5) & 0x1f;
			r = (data >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			palette_change_color(TC0110PCR_addr[0],r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR offset %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

WRITE16_HANDLER( TC0110PCR_step1_4bpg_word_w )
{
	TC0110PCR_type = 2;	/* xxxxBBBBGGGGRRRR */

	switch (offset)
	{
		case 0:
			TC0110PCR_addr[0] = data & 0xfff;
			if (data>0xfff) logerror ("Write to palette index > 0xfff\n");
			break;

		case 1:
		{
			int r,g,b;   /* data = palette BGR value */

			TC0110PCR_ram[0][(TC0110PCR_addr[0])] = data & 0xffff;

			r = (data >> 0) & 0xf;
			g = (data >> 4) & 0xf;
			b = (data >> 8) & 0xf;

			r = (r << 4) | r;
			g = (g << 4) | g;
			b = (b << 4) | b;

			palette_change_color(TC0110PCR_addr[0],r,g,b);
			break;
		}

		default:
logerror("PC %06x: warning - write %04x to TC0110PCR address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

/***************************************************************************/


static data8_t TC0220IOC_regs[8];
static data8_t TC0220IOC_port;

READ_HANDLER( TC0220IOC_r )
{
	switch (offset)
	{
		case 0x00:	/* IN00-07 (DSA) */
			return input_port_0_r(0);

		case 0x01:	/* IN08-15 (DSB) */
			return input_port_1_r(0);

		case 0x02:	/* IN16-23 (1P) */
			return input_port_2_r(0);

		case 0x03:	/* IN24-31 (2P) */
			return input_port_3_r(0);

		case 0x04:	/* coin counters and lockout */
			return TC0220IOC_regs[4];

		case 0x07:	/* INB0-7 (coin) */
			return input_port_4_r(0);

		default:
logerror("PC %06x: warning - read TC0220IOC address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE_HANDLER( TC0220IOC_w )
{
	TC0220IOC_regs[offset] = data;

	switch (offset)
	{
		case 0x00:
			watchdog_reset_w(offset,data);
			break;

		case 0x04:	/* coin counters and lockout, hi nibble irrelevant */
			coin_lockout_w(0,~data & 0x01);
			coin_lockout_w(1,~data & 0x02);
			coin_counter_w(0,data & 0x04);
			coin_counter_w(1,data & 0x08);

/*if (data &0xf0) */
/*logerror("PC %06x: warning - write %02x to TC0220IOC address %02x\n",cpu_get_pc(),data,offset); */

			break;

		default:
logerror("PC %06x: warning - write %02x to TC0220IOC address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

READ_HANDLER( TC0220IOC_port_r )
{
	return TC0220IOC_port;
}

WRITE_HANDLER( TC0220IOC_port_w )
{
	TC0220IOC_port = data;
}

READ_HANDLER( TC0220IOC_portreg_r )
{
	return TC0220IOC_r(TC0220IOC_port);
}

WRITE_HANDLER( TC0220IOC_portreg_w )
{
	TC0220IOC_w(TC0220IOC_port, data);
}

READ16_HANDLER( TC0220IOC_halfword_port_r )
{
	return TC0220IOC_port_r( offset );
}

WRITE16_HANDLER( TC0220IOC_halfword_port_w )
{
	if (ACCESSING_LSB)
		TC0220IOC_port_w( offset, data & 0xff );
}

READ16_HANDLER( TC0220IOC_halfword_portreg_r )
{
	return TC0220IOC_portreg_r( offset );
}

WRITE16_HANDLER( TC0220IOC_halfword_portreg_w )
{
	if (ACCESSING_LSB)
		TC0220IOC_portreg_w( offset, data & 0xff );
}

READ16_HANDLER( TC0220IOC_halfword_byteswap_port_r )
{
	return TC0220IOC_port_r( offset ) << 8;
}

WRITE16_HANDLER( TC0220IOC_halfword_byteswap_port_w )
{
	if (ACCESSING_MSB)
		TC0220IOC_port_w( offset, (data>>8) & 0xff );
}

READ16_HANDLER( TC0220IOC_halfword_byteswap_portreg_r )
{
	return TC0220IOC_portreg_r( offset )<<8;
}

WRITE16_HANDLER( TC0220IOC_halfword_byteswap_portreg_w )
{
	if (ACCESSING_MSB)
		TC0220IOC_portreg_w( offset, (data>>8) & 0xff );
}

READ16_HANDLER( TC0220IOC_halfword_r )
{
	return TC0220IOC_r(offset);
}

WRITE16_HANDLER( TC0220IOC_halfword_w )
{
	if (ACCESSING_LSB)
		TC0220IOC_w(offset,data & 0xff);
	else
	{
		/* qtorimon writes here the coin counters - bug? */
		TC0220IOC_w(offset,(data >> 8) & 0xff);

		if (offset)		/* ainferno writes watchdog in msb */
logerror("CPU #0 PC %06x: warning - write to MSB of TC0220IOC address %02x\n",cpu_get_pc(),offset);
	}
}

READ16_HANDLER( TC0220IOC_halfword_byteswap_r )
{
	return TC0220IOC_halfword_r(offset,mem_mask) << 8;
}

WRITE16_HANDLER( TC0220IOC_halfword_byteswap_w )
{
	if (ACCESSING_MSB)
		TC0220IOC_w(offset,(data >> 8) & 0xff);
	else
	{
		TC0220IOC_w(offset,data & 0xff);

logerror("CPU #0 PC %06x: warning - write to LSB of TC0220IOC address %02x\n",cpu_get_pc(),offset);
	}
}


/***************************************************************************/


static data8_t TC0510NIO_regs[8];

READ_HANDLER( TC0510NIO_r )
{
	switch (offset)
	{
		case 0x00:	/* DSA */
			return input_port_0_r(0);

		case 0x01:	/* DSB */
			return input_port_1_r(0);

		case 0x02:	/* 1P */
			return input_port_2_r(0);

		case 0x03:	/* 2P */
			return input_port_3_r(0);

		case 0x04:	/* coin counters and lockout */
			return TC0510NIO_regs[4];

		case 0x07:	/* coin */
			return input_port_4_r(0);

		default:
logerror("PC %06x: warning - read TC0510NIO address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE_HANDLER( TC0510NIO_w )
{
	TC0510NIO_regs[offset] = data;

	switch (offset)
	{
		case 0x00:
			watchdog_reset_w(offset,data);
			break;

		case 0x04:	/* coin counters and lockout */
			coin_lockout_w(0,~data & 0x01);
			coin_lockout_w(1,~data & 0x02);
			coin_counter_w(0,data & 0x04);
			coin_counter_w(1,data & 0x08);
			break;

		default:
logerror("PC %06x: warning - write %02x to TC0510NIO address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

READ16_HANDLER( TC0510NIO_halfword_r )
{
	return TC0510NIO_r(offset);
}

WRITE16_HANDLER( TC0510NIO_halfword_w )
{
	if (ACCESSING_LSB)
		TC0510NIO_w(offset,data & 0xff);
	else
	{
		/* driftout writes the coin counters here - bug? */
logerror("CPU #0 PC %06x: warning - write to MSB of TC0510NIO address %02x\n",cpu_get_pc(),offset);
		TC0510NIO_w(offset,(data >> 8) & 0xff);
	}
}

READ16_HANDLER( TC0510NIO_halfword_wordswap_r )
{
	return TC0510NIO_halfword_r(offset ^ 1,mem_mask);
}

WRITE16_HANDLER( TC0510NIO_halfword_wordswap_w )
{
	TC0510NIO_halfword_w(offset ^ 1,data,mem_mask);
}


/***************************************************************************/

static data8_t TC0640FIO_regs[8];

READ_HANDLER( TC0640FIO_r )
{
	switch (offset)
	{
		case 0x00:	/* DSA */
			return input_port_0_r(0);

		case 0x01:	/* DSB */
			return input_port_1_r(0);

		case 0x02:	/* 1P */
			return input_port_2_r(0);

		case 0x03:	/* 2P */
			return input_port_3_r(0);

		case 0x04:	/* coin counters and lockout */
			return TC0640FIO_regs[4];

		case 0x07:	/* coin */
			return input_port_4_r(0);

		default:
logerror("PC %06x: warning - read TC0640FIO address %02x\n",cpu_get_pc(),offset);
			return 0xff;
	}
}

WRITE_HANDLER( TC0640FIO_w )
{
	TC0640FIO_regs[offset] = data;

	switch (offset)
	{
		case 0x00:
			watchdog_reset_w(offset,data);
			break;

		case 0x04:	/* coin counters and lockout */
			coin_lockout_w(0,~data & 0x01);
			coin_lockout_w(1,~data & 0x02);
			coin_counter_w(0,data & 0x04);
			coin_counter_w(1,data & 0x08);
			break;

		default:
logerror("PC %06x: warning - write %02x to TC0640FIO address %02x\n",cpu_get_pc(),data,offset);
			break;
	}
}

READ16_HANDLER( TC0640FIO_halfword_r )
{
	return TC0640FIO_r(offset);
}

WRITE16_HANDLER( TC0640FIO_halfword_w )
{
	if (ACCESSING_LSB)
		TC0640FIO_w(offset,data & 0xff);
	else
	{
		TC0640FIO_w(offset,(data >> 8) & 0xff);
logerror("CPU #0 PC %06x: warning - write to MSB of TC0640FIO address %02x\n",cpu_get_pc(),offset);
	}
}

READ16_HANDLER( TC0640FIO_halfword_byteswap_r )
{
	return TC0640FIO_halfword_r(offset,mem_mask) << 8;
}

WRITE16_HANDLER( TC0640FIO_halfword_byteswap_w )
{
	if (ACCESSING_MSB)
		TC0640FIO_w(offset,(data >> 8) & 0xff);
	else
	{
		TC0640FIO_w(offset,data & 0xff);
logerror("CPU #0 PC %06x: warning - write to LSB of TC0640FIO address %02x\n",cpu_get_pc(),offset);
	}
}


