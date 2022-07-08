/*------------------------------------------------------------------
  vec4x1.h:

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
#ifndef VECTOR44x1_DEF
#define VECTOR44x1_DEF

typedef float VECTOR4 [4];

extern void   Vector_init    ( VECTOR4 );
extern void   Vector_set     ( VECTOR4, float, float, float );
extern void   Vector_copy    ( VECTOR4, VECTOR4 );
extern void   Vector_print   ( VECTOR4 );
extern void   Vector_negate  ( VECTOR4 );
extern void   Vector_add     ( VECTOR4, VECTOR4, VECTOR4 );
extern void   Vector_addd    ( VECTOR4, VECTOR4 );
extern void   Vector_sub     ( VECTOR4, VECTOR4, VECTOR4 );
extern void   Vector_subb    ( VECTOR4, VECTOR4 );
extern void   Vector_cross   ( VECTOR4, VECTOR4, VECTOR4 );
extern float  Vector_mag     ( VECTOR4 );
extern float  Vector_mag_squared ( VECTOR4 );
extern float  Vector_dist    ( VECTOR4, VECTOR4 );
extern float  Vector_dist_squared ( VECTOR4, VECTOR4 );
extern float  Vector_dot     ( VECTOR4, VECTOR4 );
extern void   Vector_norm    ( VECTOR4 );
extern void   Vector_scale   ( VECTOR4, float );

#endif
