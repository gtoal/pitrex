/*------------------------------------------------------------------
  mat4x4.h:

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
#ifndef MATRIX44x4_DEF
#define MATRIX44x4_DEF

#include "vec4x1.h"


typedef float MATRIX4 [4][4];

extern void Matrix_id        ( MATRIX4 );
extern void Matrix_zero      ( MATRIX4 );
extern void Matrix_copy      ( MATRIX4 , MATRIX4 );
extern void Matrix_print     ( MATRIX4 );
extern void Matrix_mult      ( MATRIX4 , MATRIX4, MATRIX4 );
extern void Matrix_transpose ( MATRIX4 , MATRIX4 );
extern void Matrix_vec_mult  ( MATRIX4 , VECTOR4, VECTOR4 );
extern void Matrix_vec_multn ( MATRIX4 , VECTOR4 [], VECTOR4 [], int );

extern void Matrix_x_rot     ( MATRIX4, float );
extern void Matrix_y_rot     ( MATRIX4, float );
extern void Matrix_z_rot     ( MATRIX4, float );

extern void Matrix_set_xrow  ( MATRIX4, VECTOR4 );
extern void Matrix_set_yrow  ( MATRIX4, VECTOR4 );
extern void Matrix_set_zrow  ( MATRIX4, VECTOR4 );

extern void Matrix_set_xcol  ( MATRIX4, VECTOR4 );
extern void Matrix_set_ycol  ( MATRIX4, VECTOR4 );
extern void Matrix_set_zcol  ( MATRIX4, VECTOR4 );

extern void Matrix_set_trans ( MATRIX4, VECTOR4 );
#endif
