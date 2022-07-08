/*------------------------------------------------------------------
  vec4x1.c:

    XINVADERS 3D - 3d Shoot'em up
    Copyright (C) 2000 Don Llopis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "vec4x1.h"

/*================================================================*/

void Vector_init ( VECTOR4 a )
{
   a[0] = a[1] = a[2] = 0.0f;
   a[3] = 1.0f;
}

/*================================================================*/

void Vector_set ( VECTOR4 a, float x, float y, float z )
{
   a[0] = x;
   a[1] = y;
   a[2] = z;
   a[3] = 1.0f;
}

/*================================================================*/

void Vector_copy ( VECTOR4 a, VECTOR4 b )
{
   memcpy ( b, a, sizeof ( VECTOR4 ) );
}

/*================================================================*/

void Vector_print ( VECTOR4 a )
{
   fprintf ( stderr, "%5.5f %5.5f %5.5f\n", a[0], a[1], a[2] );
}

/*================================================================*/

void Vector_negate ( VECTOR4 a )
{
   a[0] *= -1.0f;
   a[1] *= -1.0f;
   a[2] *= -1.0f;
}

/*================================================================*/

void Vector_add ( VECTOR4 a, VECTOR4 b, VECTOR4 c )
{
   c[0] = a[0] + b[0];
   c[1] = a[1] + b[1];
   c[2] = a[2] + b[2];
}

void Vector_addd ( VECTOR4 a, VECTOR4 b )
{
   a[0] = a[0] + b[0];
   a[1] = a[1] + b[1];
   a[2] = a[2] + b[2];
}

/*================================================================*/

void Vector_sub ( VECTOR4 a, VECTOR4 b, VECTOR4 c )
{
   c[0] = a[0] - b[0];
   c[1] = a[1] - b[1];
   c[2] = a[2] - b[2];
}

void Vector_subb ( VECTOR4 a, VECTOR4 b )
{
   a[0] = a[0] - b[0];
   a[1] = a[1] - b[1];
   a[2] = a[2] - b[2];
}

/*================================================================*/

void Vector_cross ( VECTOR4 a, VECTOR4 b, VECTOR4 c )
{
   c[0] = ( a[1] * b[2] ) - ( a[2] * b[1] );
   c[1] = ( a[2] * b[0] ) - ( a[0] * b[2] );
   c[2] = ( a[0] * b[1] ) - ( a[1] * b[0] );
}

/*================================================================*/

float Vector_mag ( VECTOR4 a )
{
   return (float) sqrt ( ( a[0] * a[0] + a[1] * a[1] + a[2] * a[2] ) );
}

float Vector_mag_squared ( VECTOR4 a )
{
   return ( a[0] * a[0] + a[1] * a[1] + a[2] * a[2] );
}


/*================================================================*/

float Vector_dist ( VECTOR4 a, VECTOR4 b )
{
   float x, y, z;

   x = b[0] - a[0];
   y = b[1] - a[1];
   z = b[2] - a[2];

   return (float) sqrt ( (x*x) + (y*y) + (z*z) );
}

/*================================================================*/

float Vector_dist_squared ( VECTOR4 a, VECTOR4 b )
{
   float x, y, z;

   x = b[0] - a[0];
   y = b[1] - a[1];
   z = b[2] - a[2];

   return ( (x*x) + (y*y) + (z*z) );
}

/*================================================================*/

float Vector_dot ( VECTOR4 a, VECTOR4 b )
{
   return ( (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) );
}

/*================================================================*/

void Vector_norm ( VECTOR4 a )
{
   float len;

   len = (float) sqrt ( (a[0] * a[0]) + (a[1] * a[1]) + (a[2] * a[2]) );

   a[0] /= len;
   a[1] /= len;
   a[2] /= len;
}

/*================================================================*/

void Vector_scale ( VECTOR4 a, float s )
{
   a[0] *= s;
   a[1] *= s;
   a[2] *= s;
}

/*================================================================*/
