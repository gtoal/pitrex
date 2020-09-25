#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
extern int bufferType; // 0 = none, 1 = double buffer, 2 = auto buffer (if pipeline is empty -> use previous
#include "window.h"

//#define PORTABILITY_HACK1 1
typedef long fp14; // There are some architecture dependencies in this code (long vs long long, signed shifts etc)

//----------------------------------------------
//
// Add and subtract fixed point numbers. :-)
//

#define fpAdd(a1, a2) ((a1) + (a2))
#define fpSub(s1, s2) ((s1) - (s2))

//----------------------------------------------
//
// Convert an integer to n N.14 fixed point
// number and back.
//

#define int2fp(x) ((x) << 14)
#define fp2int(x) ((x) >> 14)

//----------------------------------------------
//
// Get the fractional part of an N.14 fixed point
// number.

#define fpFract(x) ((x) & 0x3fff)

//----------------------------------------------
//
// Convert a N.14 fixed point number to a
// double and back. Handy for printing and for
// those times when you really do want to work
// with floating point values.
//


#ifdef PORTABILITY_HACK1
#define fp2float(x) ( (double) (((double)(x)) / (double)(1 << 14))  )
#define float2fp(x) ((fp14) ((double)(x) * (double)(1 << 14)))
#endif

//----------------------------------------------
//
// Table based fixed point sine and cosine 
// functions.
//

#define fpSin(x) (sine[(x) & 0xff])
#define fpCos(x) (sine[((x)+64) & 0xff])

//----------------------------------------------
//
// 18.14 fixed point sine and cosine tables.
//
// These table assume that there are 256 angles
// in a circle.
//

fp14 sine[256] =
{
     0,    402,    803,   1205,   1605,   2005,   2404,   2801, 
  3196,   3589,   3980,   4369,   4756,   5139,   5519,   5896, 
  6269,   6639,   7005,   7366,   7723,   8075,   8423,   8765, 
  9102,   9434,   9759,  10079,  10393,  10701,  11002,  11297, 
 11585,  11866,  12139,  12406,  12665,  12916,  13159,  13395, 
 13622,  13842,  14053,  14255,  14449,  14634,  14810,  14978, 
 15136,  15286,  15426,  15557,  15678,  15790,  15892,  15985, 
 16069,  16142,  16206,  16260,  16305,  16339,  16364,  16379, 
 16384,  16379,  16364,  16339,  16305,  16260,  16206,  16142, 
 16069,  15985,  15892,  15790,  15678,  15557,  15426,  15286, 
 15136,  14978,  14810,  14634,  14449,  14255,  14053,  13842, 
 13622,  13395,  13159,  12916,  12665,  12406,  12139,  11866, 
 11585,  11297,  11002,  10701,  10393,  10079,   9759,   9434, 
  9102,   8765,   8423,   8075,   7723,   7366,   7005,   6639, 
  6269,   5896,   5519,   5139,   4756,   4369,   3980,   3589, 
  3196,   2801,   2404,   2005,   1605,   1205,    803,    402, 
     0,   -402,   -803,  -1205,  -1605,  -2005,  -2404,  -2801, 
 -3196,  -3589,  -3980,  -4369,  -4756,  -5139,  -5519,  -5896, 
 -6269,  -6639,  -7005,  -7366,  -7723,  -8075,  -8423,  -8765, 
 -9102,  -9434,  -9759, -10079, -10393, -10701, -11002, -11297, 
-11585, -11866, -12139, -12406, -12665, -12916, -13159, -13395, 
-13622, -13842, -14053, -14255, -14449, -14634, -14810, -14978, 
-15136, -15286, -15426, -15557, -15678, -15790, -15892, -15985, 
-16069, -16142, -16206, -16260, -16305, -16339, -16364, -16379, 
-16384, -16379, -16364, -16339, -16305, -16260, -16206, -16142, 
-16069, -15985, -15892, -15790, -15678, -15557, -15426, -15286, 
-15136, -14978, -14810, -14634, -14449, -14255, -14053, -13842, 
-13622, -13395, -13159, -12916, -12665, -12406, -12139, -11866, 
-11585, -11297, -11002, -10701, -10393, -10079,  -9759,  -9434, 
 -9102,  -8765,  -8423,  -8075,  -7723,  -7366,  -7005,  -6639, 
 -6269,  -5896,  -5519,  -5139,  -4756,  -4369,  -3980,  -3589, 
 -3196,  -2801,  -2404,  -2005,  -1605,  -1205,   -803,   -402, 
};


fp14
fpDiv(fp14 d1, fp14 d2)
{
#ifdef PORTABILITY_HACK1
  return float2fp(fp2float(d1)/fp2float(d2)); // alternative portable version
#else
  // tested on LCC on windows and GCC on 64 bit linux, but still may not be portable.  Signedness is an issue.
  return ((fp14)((((long long)d1<<14LL) / (long long)d2)<<14LL));
#endif
}

fp14
fpMul(fp14 m1, fp14 m2)
{
#ifdef PORTABILITY_HACK1
  return float2fp(fp2float(m1)*fp2float(m2)); // alternative portable version
#else
  // see above
  return ((fp14)(((long long)m1 * (long long)m2)>>14LL));
#endif
}

typedef struct VPoint
{
        int x,y,z;
} VPoint;

// for convenience and shorter code on Vectrex - need to do more of this...
// in GCC many of these short functions could be 'static inline' ...
int mult_sin(int coord, int angle)
{
  return fp2int(fpMul(int2fp(coord), fpSin(angle)));
}

int mult_cos(int coord, int angle)
{
  return fp2int(fpMul(int2fp(coord), fpCos(angle)));
}

// x,y,z - 'real' world (1024x1024x1024)
// xangle,yangle,zangle - rotate object around its own axis (note: need quaternions to avoid gimbal lock)
// screenx, screeny, screenz - 'virtual pixel' offset in screen coords (1280x1024)

void DrawShip(int ship, int x, int y, int z, int xangle, int yangle, int zangle, int screenx, int screeny)
{
int i;
// Ships hand-coded by Peter Hirschberger.  Great work, Peter!
// Original data was given to me as doubles (eg lines of 1.5 length) but I've scaled everything to ints
#define nNumVectors0 20
	VPoint vectors0[nNumVectors0*2] = 
	{
            { 0, 0, -100 },
            { 40, 0, -60 },
            { 40, 0, -60 },
            { 0, 0, -20 },
            { 0, 0, -20 },
            { -40, 0, -60 },
            { -40, 0, -60 },
            { 0, 0, -100 },
            { 0, 0, -100 },
            { 0, 40, -60 },
            { 0, 40, -60 },
            { 0, 0, -20 },
            { 0, 0, -20 },
            { 0, -40, -60 },
            { 0, -40, -60 },
            { 0, 0, -100 },
            { 0, 0, -20 },
            { 30, 0, 120 },
            { 0, 0, -20 },
            { 20, 20, 120 },
            { 0, 0, -20 },
            { -20, 20, 120 },
            { 0, 0, -20 },
            { -30, 0, 120 },
            { 30, 0, 120 },
            { 20, 20, 120 },
            { 20, 20, 120 },
            { -20, 20, 120 },
            { -20, 20, 120 },
            { -30, 0, 120 },
            { -30, 0, 120 },
            { 30, 0, 120 },
            { 30, 0, 120 },
            { 100, -40, 80 },
            { 100, -40, 80 },
            { 0, 0, -20 },
            { -30, 0, 120 },
            { -100, -40, 80 },
            { -100, -40, 80 },
            { 0, 0, -20 },
	};
#define nNumVectors1 21
	VPoint vectors1[nNumVectors1*2] = 
	{
            { 0, 0, -90 },
            { 45, 0, -60 },
            { 45, 0, -60 },
            { 45, 0, 60 },
            { 45, 0, 60 },
            { 0, 0, 90 },
            { 0, 0, 90 },
            { -45, 0, 60 },
            { -45, 0, 60 },
            { -45, 0, -60 },
            { -45, 0, -60 },
            { 0, 0, -90 },
            { 45, 0, 60 },
            { 60, 24, 60 },
            { 60, 24, 60 },
            { 60, 24, 150 },
            { 60, 24, 150 },
            { 45, 0, 150 },
            { 45, 0, 150 },
            { 45, 0, 60 },
            { -45, 0, 60 },
            { -60, 24, 60 },
            { -60, 24, 60 },
            { -60, 24, 150 },
            { -60, 24, 150 },
            { -45, 0, 150 },
            { -45, 0, 150 },
            { -45, 0, 60 },
            { -45, 0, 60 },
            { -60, -27, -90 },
            { -60, -27, -90 },
            { 0, -34, -129 },
            { 0, -34, -129 },
            { 60, -27, -90 },
            { 60, -27, -90 },
            { 45, 0, 60 },
            { 0, 0, -90 },
            { 0, -34, -129 },
            { 45, 0, -60 },
            { 60, -27, -90 },
            { -45, 0, -60 },
            { -60, -27, -90 },
	};
#define nNumVectors2 17
	VPoint vectors2[nNumVectors2*2] = 
	{
            { 0, 0, -100 },
            { 60, 0, -40 },
            { 60, 0, -40 },
            { 0, 0, 20 },
            { 0, 0, 20 },
            { -60, 0, -40 },
            { -60, 0, -40 },
            { 0, 0, -100 },
            { 0, 0, -100 },
            { 0, 40, -40 },
            { 0, 40, -40 },
            { 0, 0, 20 },
            { 60, 0, -40 },
            { 0, 40, -40 },
            { 0, 40, -40 },
            { -60, 0, -40 },
            { 0, 0, 20 },
            { 0, 80, 100 },
            { 0, 80, 100 },
            { 0, 40, 100 },
            { 0, 40, 100 },
            { 0, 0, 20 },
            { 0, 0, 20 },
            { 100, -40, 100 },
            { 100, -40, 100 },
            { 40, -20, 100 },
            { 40, -20, 100 },
            { 0, 0, 20 },
            { 0, 0, 20 },
            { -100, -40, 100 },
            { -100, -40, 100 },
            { -40, -20, 100 },
            { -40, -20, 100 },
            { 0, 0, 20 },
	};
#define nNumVectors3 23
	VPoint vectors3[nNumVectors3*2] = 
	{
            { 0, 2, -92 },
            { 30, 2, -70 },
            { 30, 2, -70 },
            { 0, 10, -20 },
            { 0, 10, -20 },
            { -30, 2, -70 },
            { -30, 2, -70 },
            { 0, 2, -92 },
            { 0, 2, -92 },
            { 0, -25, -45 },
            { 0, -25, -45 },
            { 0, 10, -20 },
            { 30, 2, -70 },
            { 22, 15, -60 },
            { -30, 2, -70 },
            { -22, 15, -60 },
            { 30, 2, -70 },
            { 0, -25, -45 },
            { -30, 2, -70 },
            { 0, -25, -45 },
            { 22, 15, -60 },
            { -22, 15, -60 },
            { 22, 15, -60 },
            { 0, 10, -20 },
            { -22, 15, -60 },
            { 0, 10, -20 },
            { 30, 2, -70 },
            { -30, 2, -70 },
            { 0, 10, -20 },
            { -20, -10, 110 },
            { 0, 10, -20 },
            { 20, -10, 110 },
            { -20, -10, 110 },
            { 20, -10, 110 },
            { 0, -25, -45 },
            { -20, -10, 110 },
            { 0, -25, -45 },
            { 20, -10, 110 },
            { 0, 10, -20 },
            { -90, 20, 90 },
            { -90, 20, 90 },
            { -20, -10, 110 },
            { 0, 10, -20 },
            { 90, 20, 90 },
            { 90, 20, 90 },
            { 20, -10, 110 },
	};
        
int     nvec[4] = {nNumVectors0,nNumVectors1,nNumVectors2,nNumVectors3}; 
        // The pvec array initialisation below caused LCC to crash:
        // so rather than have 'ifdef's for one compiler, I rewrote it as an explicit initialisation.
VPoint *pvec[4] = {vectors0, vectors1, vectors2, vectors3};
int     nNumVectors = nvec[ship];

//pvec[0] = vectors0;
//pvec[1] = vectors1;
//pvec[2] = vectors2;
//pvec[3] = vectors3;
 
	for (i=0; i<nNumVectors*2; i+=2)
	{
          int x0, y0, z0, x1, y1, z1, tx0, ty0, tz0, tx1, ty1, tz1, xa, ya, xb, yb;
          x0 = pvec[ship][i].x*2/3; // last minute rescaling as the ships looked a little
          y0 = pvec[ship][i].y*2/3; // too large when entering space on the back plane.
          z0 = pvec[ship][i].z*2/3; // remove this later... rescale the data or fix it when we move to OpenGL matrices

          x1 = pvec[ship][i+1].x*2/3;
          y1 = pvec[ship][i+1].y*2/3;
          z1 = pvec[ship][i+1].z*2/3;

          /*
             The matrix code below is not ideal for plotting
             a pausible flight path (the 'Gimbal lock' problem).
             It would be much better to handle pitch/yaw/roll
             using quaternions.  I'm hacking it by restricting
             the degrees of freedom when a ship switches from
             a 2D path on the rear face to a 3D path approaching
             the viewport.

             THIS IS VERY HACKY CODE.  best solution would be to punt to OpenGL in a more major rewrite...

           */

          // rotate around z
          tx0 = mult_cos(x0, zangle)-mult_sin(y0, zangle);
          ty0 = mult_sin(x0, zangle)+mult_cos(y0, zangle);
          tz0 = z0;

          tx1 = mult_cos(x1, zangle)-mult_sin(y1, zangle);
          ty1 = mult_sin(x1, zangle)+mult_cos(y1, zangle);
          tz1 = z1;
          x0=tx0;y0=ty0;z0=tz0; x1=tx1;y1=ty1;z1=tz1;

          // rotate around y
          tz0 = mult_cos(z0, yangle)-mult_sin(x0, yangle);
          tx0 = mult_sin(z0, yangle)+mult_cos(x0, yangle);
          ty0 = y0;

          tz1 = mult_cos(z1, yangle)-mult_sin(x1, yangle);
          tx1 = mult_sin(z1, yangle)+mult_cos(x1, yangle);
          ty1 = y1;
          x0=tx0;y0=ty0;z0=tz0; x1=tx1;y1=ty1;z1=tz1;

          // rotate around x
          ty0 = mult_cos(y0, xangle)-mult_sin(z0, xangle);
          tz0 = mult_sin(y0, xangle)+mult_cos(z0, xangle);
          tx0 = x0;

          ty1 = mult_cos(y1, xangle)-mult_sin(z1, xangle);
          tz1 = mult_sin(y1, xangle)+mult_cos(z1, xangle);
          tx1 = x1;
          tx0 = tx0+x; ty0 = ty0+y; tx1 = tx1+x; ty1 = ty1+y;

          // treat 512,340^H^H^H400 as 0,0 for perspective purposes.  ANOTHER HACKY ALGORITHM!
	  // Good old 8-bit micro style coding...
#define TWEAK 2048 /* was 1024 */
          xa = ((TWEAK*tx0) / (TWEAK+tz0));
          ya = ((TWEAK*ty0) / (TWEAK+tz0));
          xb = ((TWEAK*tx1) / (TWEAK+tz1));
          yb = ((TWEAK*ty1) / (TWEAK+tz1));
//#undef TWEAK

          // scale by depth  ">>10" is short for "/ZDEPTH"
          xa = (xa*(z+200))>>10; ya=(ya*(z+200))>>10; xb=(xb*(z+200))>>10; yb=(yb*(z+200))>>10;

          // then re-correct back to center of screen at 512,400
          //CinemaVectorData(xa+512+screenx,ya+400+screeny,xb+512+screenx,yb+400+screeny, 7);
          v_line(400+xa+screenx,400+ya+screeny,400+xb+screenx,400+yb+screeny, 64);
          //fprintf(stdout, "%d,%d -> %d,%d (%d)\n",512+xa+screenx,400+ya+screeny,512+xb+screenx,400+yb+screeny, 64);
	}
#undef nNumVectors0
#undef nNumVectors1
#undef nNumVectors2
#undef nNumVectors3
}

static void startFrame()
{
  v_WaitRecal();
  //v_doSound();
  v_setBrightness(64);        /* set intensity of vector beam... */
  v_readButtons();
  v_readJoystick1Analog();
  //v_playAllSFX();
}

int main(int argc, char **argv) {
  //#define SETTINGS_SIZE 1024
  //static unsigned char settingsBlob[SETTINGS_SIZE];
  int x,y,z, dist;
  char *progname, *p;

  progname = argv[0];
  p = strrchr(progname, '/'); if (p) progname = p+1;

  vectrexinit(1);
  v_setName(progname);
  v_init();
  v_set_hardware_orientation(VECTREX_DEFAULT);

  //v_loadSettings(progname, settingsBlob, SETTINGS_SIZE);

  usePipeline = 1;   // should create procedures for these rather than use global variables.
                     // doesn't matter for now but should be cleaned up before we release
  bufferType = 2;
  
  v_setRefresh(60);
  v_window(0,0, 1024*3/4,1024, NO_CLIP); // clipping currently broken except in default vertical orientation
  // using a window with negative lower-left did not seem to work in this code.  More debugging needed.

  x = 0; y = 0; z = 0; dist = 200;
  for (;;) {
    if (random()&1) {x += 1; y &= 255;}
    if (random()&1) {y += 1; y &= 255;}
    if (random()&1) {z += 1; z &= 255;}
    dist += 1; dist &= 1023;
    startFrame();
    DrawShip(0, /* AT */ -256+96,0, (dist+300)&1023, /* ROT */ x,y,z, /* SCREEN */ 0,0);
    DrawShip(1, /* AT */ 96,0,(dist+800)&1023, /* ROT */ y,z,x, /* SCREEN */ 0,0);
    DrawShip(2, /* AT */ -256+96,256,dist, /* ROT */ z,x,y, /* SCREEN */ 0,0);
    DrawShip(3, /* AT */ 96,256,(500-dist)&1023, /* ROT */ x,z,y, /* SCREEN */ 0,0);
  }

  return 0;
}
