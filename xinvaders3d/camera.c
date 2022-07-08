/*------------------------------------------------------------------
  camera.c:

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
#include <math.h>

#include "mat4x4.h"
#include "vec4x1.h"
#include "camera.h"

/* local variables */
static float hfov, vfov, hpc, vpc;
static float screen_width, screen_height, half_screen_width,
             half_screen_height;
static float aspect_ratio;
static float hadjust, vadjust;
static MATRIX4 r_2_l; 


void Camera_init ( unsigned int width, unsigned int height, float fov )
{
  screen_width       = (float) width;
  screen_height      = (float) height;
  half_screen_width  = screen_width / 2.0f;
  half_screen_height = screen_height / 2.0f;
  
  /* calculate horizontal & vertical field of view */

  /* horizontal compensate = HALF_SCREEN_WIDTH / tan ( degrees / 2 )
     vertical compensate = HALF_SCREEN_HEIGHT / tan ( degrees / 2 )

     see "Building 3d Game Engine" by Brain Hook
  */

  hfov = vfov = fov;

  hpc = half_screen_width / tan ( hfov/1.5f );
  vpc = half_screen_height / tan ( vfov/1.5f );

  /* adjust aspect ratio if screen not square ( ie. 4/3 ratio )*/
  aspect_ratio = screen_height / screen_height;


  /* calculate perspective adjustment values */
  hadjust = hpc;
  vadjust = (vpc * aspect_ratio);

  /* initialize coordinate system conversion mat */
  Matrix_id ( r_2_l );
  r_2_l[2][2] = -1.0f;
  
}

void Camera_project_point ( VECTOR4 a, int b[2] )
{
   if ( a[2] < 0.0 ) {
      b[0] = b[1] = 0;
      return;
   }
  b[0] = (int) ( a[0] * hadjust / a[2] + half_screen_width );
  b[1] = (int) ( -a[1] * vadjust / a[2] + half_screen_height );
}

void Camera_project_points ( VECTOR4 a[], int b[], int n )
{
   int i, j;
  
   for ( i=0, j=0; i<n; i++, j+=2 )
   {
     b[j] = (int) ( a[i][0] * hadjust / a[i][2] + half_screen_width );
     b[j+1] = (int) ( -a[i][1] * vadjust / a[i][2] + half_screen_height );
   }
}

void Camera_transform ( MATRIX4 r, VECTOR4 up, VECTOR4 from, VECTOR4 at )
{
  VECTOR4 x, y, z, tmp, pos;
  MATRIX4 tmp_mat;

  /* initialize transformation mat */
  Matrix_id ( tmp_mat );

  /* calculate view plane normal -- or view-z-axis */
  Vector_sub ( from, at, z );
  Vector_norm ( z );

  /* calculate view-x-axis */
  Vector_cross ( up, z, x );
  Vector_norm ( x );

  /* calculate view-y-axis */
  Vector_cross ( z, x, y );

  /* place axis-vecs into transformation mat
     To understand how this works see chapter 5
     and 6 of "Computer Graphics Principles & Practice"
   */
  Matrix_set_xrow ( tmp_mat, x );
  Matrix_set_yrow ( tmp_mat, y );
  Matrix_set_zrow ( tmp_mat, z );

  /* get camera's world position and rotate it
     into camera coordinate system. This becomes
     the new translation which is to be applied
     to an object's vertices inorder to make its
     position relative to the camera coordinate system
     see chapter 6
  */
  Vector_copy ( from, tmp );
  Vector_negate ( tmp );
  Matrix_vec_mult ( tmp_mat, tmp, pos );
  Matrix_set_trans ( tmp_mat, pos );

  /* now change from right hand coordinate system
     to left hand coordinate system
     leaving the result in 'r'
  */
  Matrix_mult ( r_2_l, tmp_mat, r );
}
