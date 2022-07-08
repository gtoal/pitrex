/*------------------------------------------------------------------
  mat4x4.c:

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

#include "mat4x4.h"


/*================================================================*/

void Matrix_id ( MATRIX4 a )
{
   memset ( a, 0, sizeof ( MATRIX4 ) );
   a[0][0] = a[1][1] = a[2][2] = a[3][3] = 1.0f;
}

/*================================================================*/

void Matrix_zero ( MATRIX4 a )
{
   memset ( a, 0, sizeof (MATRIX4 ) );
}

/*================================================================*/

void Matrix_copy ( MATRIX4 a, MATRIX4 b )
{
   memcpy ( b, a, sizeof ( MATRIX4 ) );
}

/*================================================================*/

void Matrix_print ( MATRIX4 a )
{
   int i, j;

   for ( i=0; i<4; i++ )
   {
      for (j=0; j<4; j++ )
         fprintf ( stderr, "%5.5f ", a[i][j] );

      fprintf ( stderr, "\n" );
   }
}

/*================================================================*/

void Matrix_mult ( MATRIX4 a, MATRIX4 b, MATRIX4 c )
{
   int i;

   for ( i=0; i<4; i++ )
   {
      c[i][0] = ( a[i][0] * b[0][0] ) +
                  ( a[i][1] * b[1][0] ) +
                  ( a[i][2] * b[2][0] ) +
                  ( a[i][3] * b[3][0] );
      
      c[i][1] = ( a[i][0] * b[0][1] ) +
                  ( a[i][1] * b[1][1] ) +
                  ( a[i][2] * b[2][1] ) +
                  ( a[i][3] * b[3][1] );
      
      c[i][2] = ( a[i][0] * b[0][2] ) +
                  ( a[i][1] * b[1][2] ) +
                  ( a[i][2] * b[2][2] ) +
                  ( a[i][3] * b[3][2] );
      
      c[i][3] = ( a[i][0] * b[0][3] ) +
                  ( a[i][1] * b[1][3] ) +
                  ( a[i][2] * b[2][3] ) +
                  ( a[i][3] * b[3][3] );
   }
}

/*================================================================*/

void Matrix_transpose ( MATRIX4 a, MATRIX4 b )
{
   b[0][0] = a[0][0];
   b[0][1] = a[1][0];
   b[0][2] = a[2][0];
   b[0][3] = b[3][0];
   
   b[1][0] = a[0][1];
   b[1][1] = a[1][1];
   b[1][2] = a[2][1];
   b[1][3] = b[3][1];
   
   b[2][0] = a[0][2];
   b[2][1] = a[1][2];
   b[2][2] = a[2][2];
   b[2][3] = b[3][2];
   
   b[3][0] = a[0][3];
   b[3][1] = a[1][3];
   b[3][2] = a[2][3];
   b[3][3] = b[3][3];
}

/*================================================================*/

void Matrix_vec_mult ( MATRIX4 a, VECTOR4 b, VECTOR4 c )
{
   c[0] = ( a[0][0] * b[0] ) +
          ( a[0][1] * b[1] ) +
          ( a[0][2] * b[2] ) +
          ( a[0][3] * b[3] );

   c[1] = ( a[1][0] * b[0] ) +
          ( a[1][1] * b[1] ) +
          ( a[1][2] * b[2] ) +
          ( a[1][3] * b[3] );

   c[2] = ( a[2][0] * b[0] ) +
          ( a[2][1] * b[1] ) +
          ( a[2][2] * b[2] ) +
          ( a[2][3] * b[3] );
   
   c[3] = ( a[3][0] * b[0] ) +
          ( a[3][1] * b[1] ) +
          ( a[3][2] * b[2] ) +
          ( a[3][3] * b[3] );
}

/*================================================================*/

void Matrix_vec_multn ( MATRIX4 a, VECTOR4 b[], VECTOR4 c[], int num )
{
   int i;

   for ( i=0; i<num; i++ )
   {
      c[i][0] = ( a[0][0] * b[i][0] ) +
                ( a[0][1] * b[i][1] ) +
                ( a[0][2] * b[i][2] ) +
                ( a[0][3] * b[i][3] );
      
      c[i][1] = ( a[1][0] * b[i][0] ) +
                ( a[1][1] * b[i][1] ) +
                ( a[1][2] * b[i][2] ) +
                ( a[1][3] * b[i][3] );
     
      c[i][2] = ( a[2][0] * b[i][0] ) +
                ( a[2][1] * b[i][1] ) +
                ( a[2][2] * b[i][2] ) +
                ( a[2][3] * b[i][3] );
    
      c[i][3] = ( a[3][0] * b[i][0] ) +
                ( a[3][1] * b[i][1] ) +
                ( a[3][2] * b[i][2] ) +
                ( a[3][3] * b[i][3] );
   }
}

/*================================================================*/

void Matrix_x_rot ( MATRIX4 a, float rad )
{
   float sine, cosine;

   sine = (float) sin ( rad );
   cosine = (float) cos ( rad );

   Matrix_id ( a );
   a[1][1] = cosine; a[1][2] = -sine;
   a[2][1] = sine;   a[2][2] = cosine;
}

/*================================================================*/

void Matrix_y_rot ( MATRIX4 a, float rad )
{
   float sine, cosine;

   sine = (float) sin ( rad );
   cosine = (float) cos ( rad );

   Matrix_id ( a );
   a[0][0] = cosine; a[0][2] = sine;
   a[2][0] = -sine;  a[2][2] = cosine;
}

/*================================================================*/

void Matrix_z_rot ( MATRIX4 a, float rad )
{
   float sine, cosine;

   sine = sin ( rad );
   cosine = cos ( rad );

   Matrix_id ( a );
   a[0][0] = cosine; a[0][1] = -sine;
   a[1][0] = sine;   a[1][1] = cosine;
}

/*================================================================*/

void Matrix_set_xrow ( MATRIX4 a, VECTOR4 b )
{
   a[0][0] = b[0];
   a[0][1] = b[1];
   a[0][2] = b[2];
}

/*================================================================*/

void Matrix_set_yrow ( MATRIX4 a, VECTOR4 b )
{
   a[1][0] = b[0];
   a[1][1] = b[1];
   a[1][2] = b[2];
}

/*================================================================*/

void Matrix_set_zrow ( MATRIX4 a, VECTOR4 b )
{
   a[2][0] = b[0];
   a[2][1] = b[1];
   a[2][2] = b[2];
} 

/*================================================================*/

void Matrix_set_xcol ( MATRIX4 a, VECTOR4 b )
{
   a[0][0] = b[0];
   a[1][0] = b[1];
   a[2][0] = b[2];
}

/*================================================================*/

void Matrix_set_ycol ( MATRIX4 a, VECTOR4 b )
{
   a[0][1] = b[0];
   a[1][1] = b[1];
   a[2][1] = b[2];
}

/*================================================================*/

void Matrix_set_zcol ( MATRIX4 a, VECTOR4 b )
{
   a[0][2] = b[0];
   a[1][2] = b[1];
   a[2][2] = b[2];
} 

/*================================================================*/

void Matrix_set_trans ( MATRIX4 a, VECTOR4 b )
{
   a[0][3] = b[0];
   a[1][3] = b[1];
   a[2][3] = b[2];
}

/*================================================================*/
