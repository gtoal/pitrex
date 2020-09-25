#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include <math.h>

void
startFrame ()
{
  v_WaitRecal ();
  v_readButtons ();
  v_readJoystick1Analog ();
}

//
// 3D Cube by Jason Wright (C)opyright Pyrofer 2006
//
// This code is free, there is no charge for it nor should anybody else charge
// for the code, its distribution or derivitives of this code.
//
// You may use this code, modify and change and improve upon it on the following conditions,
//
// 1> You send me the modified source code and where possible make it public
// 2> I am credited for the original work in all derivitives of this code
// 3> You do not charge for this code or code derived from this code
// 4> You comment the code you change! 
//
// Basically, use this to learn! I couldnt find anything this simple when I started so I hope
// that this helps others to learn. You should be able to change the LCD routines to drive almost 
// any graphics device. The base resolution is 128x128 for this display. Simply set the X and Y offset
// to half the screen resolution for each axis.
// Then adjust the Z offset to make the object fit nicely in the screen.

#define OFFSETX 0		// offset for screen wont change unless
#define OFFSETY 0		// i use different screen! so its kinda fixed
#define OFFSETZ 128

const signed int aa[8] = { 10, -10, -10, 10, 10, -10, -10, 10 };	// x data for shape vertex
const signed int bb[8] = { 10, 10, -10, -10, 10, 10, -10, -10 };	// y data for shape vertex
const signed int cc[8] = { -10, -10, -10, -10, 10, 10, 10, 10 };	// z data for shape vertex

const int ff[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4 };	// start vertex for lines
const int gg[12] = { 2, 3, 4, 1, 6, 7, 8, 5, 5, 6, 7, 8 };	// end vertex for lines

int sx, sy, ex, ey;		// define global vars for calling graphics subroutines

int
main (int argc, char **argv)
{
  int newx[8];			// translated screen x co-ordinates for vertex
  int newy[8];			// translated screen y co-ordinates for vertex
  int i, loop;			// temp variable for loops
  float xt, yt, zt,
        x, y, z,
        sinax, cosax, sinay,
        cosay, sinaz, cosaz;	// lots of work variables - could convert to fixed point for native vectrex...
  float xpos = 0;		// position for object in 3d space, in x
  float ypos = 0;		// y
  float zpos = 0;		// and z values
  float rotx = 0;		// starting amount of x rotation
  float roty = 0;		// starting amount of y rotation
  float rotz = 0;		// starting amount of z rotation

  vectrexinit (1);
  v_setName("cube");
  v_init ();
  v_setRefresh (60);
  for (;;)
    {
      startFrame ();

      xpos = xpos + 0.0;	// move the object
      ypos = ypos + 0.0;	// it would wander off screen
      zpos = zpos + 0.0;	// really quick, so leave it centered

      rotx = rotx + 0.01;	// rotate the cube on X axis
      roty = roty + 0.01;	// and on its y axis
      rotz = rotz + 0.0;	// dont bother with z or it gets confusing

      sinax = sin (rotx);	// precalculate the sin and cos values
      cosax = cos (rotx);	// for the rotation as this saves a 

      sinay = sin (roty);	// little time when running as we
      cosay = cos (roty);	// call sin and cos less often

      sinaz = sin (rotz);	// they are slow routines
      cosaz = cos (rotz);	// and we dont want slow!

      for (i = 0; i < 8; i++)	// translate 3d vertex position to 2d screen position
	{
	  x = aa[i];		// get x for vertex i
	  y = bb[i];		// get y for vertex i
	  z = cc[i];		// get z for vertex i

	  yt = y * cosax - z * sinax;	// rotate around the x axis
	  zt = y * sinax + z * cosax;	// using the Y and Z for the rotation
	  y = yt;
	  z = zt;

	  xt = x * cosay - z * sinay;	// rotate around the Y axis
	  zt = x * sinay + z * cosay;	// using X and Z
	  x = xt;
	  z = zt;

	  xt = x * cosaz - y * sinaz;	// finaly rotate around the Z axis
	  yt = x * sinaz + y * cosaz;	// using X and Y
	  x = xt;
	  y = yt;

	  x = x + xpos;		// add the object position offset
	  y = y + ypos;		// for both x and y
	  z = z + OFFSETZ - zpos;	// as well as Z

	  newx[i] = ((x*512) / z) + OFFSETX;	// translate 3d to 2d coordinates for screen
	  newy[i] = ((y*512) / z) + OFFSETY;	// drawing so we can see the cube
	}

      for (i = 0; i < 12; i++)	// draw the lines that make up the object
	{
	  int vertex;
	  vertex = ff[i] - 1;	// temp = start vertex for this line
	  sx = newx[vertex];	// set line start x to vertex[i] x position
	  sy = newy[vertex];	// set line start y to vertex[i] y position
	  vertex = gg[i] - 1;	// temp = end vertex for this line
	  ex = newx[vertex];	// set line end x to vertex[i+1] x position
	  ey = newy[vertex];	// set line end y to vertex[i+1] y position

	  v_directDraw32 (sx<<8, sy<<8, ex<<8, ey<<8, 64);
	}

      v_printString (-10, 0, "HELLO", 7, 64);
      v_setBrightness (60);
      v_printStringRaster (-8, -4, "WORLD", 5 * 8, -7, '\0');
    }
}
