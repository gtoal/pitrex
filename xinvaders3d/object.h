/*------------------------------------------------------------------
  object.h: Model Data, Object, and Object-List managment struct & functions

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
#ifndef OBJECT_HEADER
#define OBJECT_HEADER

typedef struct MODELSTRUCT   MODEL;
typedef struct OBJECTSTRUCT  OBJECT;
typedef struct OBJLISTSTRUCT OBJLIST;

typedef void (*PFV1) ( OBJECT * );
typedef void (*PFV2) ( OBJECT *, MATRIX4 );

/*------------------------------------------------------------------
 * MODEL:
 * Model data management Struct & Funtions 
 *
------------------------------------------------------------------*/

struct MODELSTRUCT
{
   float **vertex;
   int   **edge;
   int   **poly;
   int   num_vertex;
   int   num_polys;
};

/*------------------------------------------------------------------
 * OBJECT:
 * Object Managment Struct & Funtions 
 *
------------------------------------------------------------------*/
struct OBJECTSTRUCT
{
   int active;
  
   long frame;
   long delay;
   long max_frame;
   
   int zone;
   int zheight;
   int vpos;

   float radius;
   float radius_squared;

   double rot;

   VECTOR4 pos;
   VECTOR4 old_pos;
   VECTOR4 dir;
   VECTOR4 thrust;

   PFV1 actionfn;
   PFV2 drawfn;

   MODEL *model;

   OBJECT *next;
   OBJECT *prev;
};

extern OBJECT *  Object_new      ( void );
extern void      Object_delete   ( OBJECT * );
extern void      Object_init     ( OBJECT * );

extern void Object_set_actionfn  ( OBJECT *, PFV1 );
extern void Object_set_drawfn    ( OBJECT *, PFV2 );

extern void Object_set_model     ( OBJECT *, MODEL * );

extern void Object_set_pos       ( OBJECT *, VECTOR4 );
extern void Object_get_pos       ( OBJECT *, VECTOR4 );

extern void Object_set_dir       ( OBJECT *, VECTOR4 );
extern void Object_get_dir       ( OBJECT *, VECTOR4 );

extern void Object_draw          ( OBJECT * );


/*------------------------------------------------------------------
 * OBJLIST:
 * gobject list managment struct & routines
 *
------------------------------------------------------------------*/

struct OBJLISTSTRUCT
{
   int obj_count;
   OBJECT *head;
   OBJECT *tail;
};

extern void Objlist_clear     ( OBJLIST * );
extern void Objlist_add       ( OBJLIST *, OBJECT * );
extern void Objlist_del       ( OBJLIST *, OBJECT * );
extern int  Objlist_count ( OBJLIST * );

#endif
