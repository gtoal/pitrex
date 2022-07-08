/*------------------------------------------------------------------
  camera.h:

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
#ifndef CAMERA
#define CAMERA


#ifdef NEW_CAMERA 

typedef struct CAMERASTRUCT CAMERA;
struct CAMERASTRUCT
{
   MATRIX4 local;
   MATRIX4 world;

   VECTOR4 up;
   VECTOR4 from;
   VECTOR4 at;

   VECTOR4 follow_offset;

   float scr_width;
   float scr_height;
   float fov;

   OBJECT *follow_obj;
};

/* new camera interface */

extern CAMERA * Camera_new ( void );
extern void Camera_delete ( CAMERA *cam );
extern void Camera_init ( CAMERA *cam, float width, float height, float fov );
extern void Camera_project_point ( CAMERA *cam, long pt[2] );
extern void Camera_project_points ( CAMERA *cam, long pts[], int n );
extern void Camera_transform ( CAMERA *cam, VECTOR4 up, VECTOR4 from, VECTOR4 at );
extern void Camera_follow_obj ( CAMERA *cam, GOBJECT *obj );
extern void Camera_set_follow_offset ( CAMERA *cam, VECTOR4 from );
extern void Camera_update ( CAMERA *cam );

#endif

/* old camera interface */

extern void Camera_init ( unsigned int width, unsigned int height, float fov );
extern void Camera_project_point ( VECTOR4 a, int b[2] );
extern void Camera_project_points ( VECTOR4 a[], int b[], int n );
extern void Camera_transform ( MATRIX4 r, VECTOR4 up, VECTOR4 from, VECTOR4 at );

#endif
