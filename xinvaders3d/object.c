/*------------------------------------------------------------------
  object.c:

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
#include <sys/types.h>
#include "mat4x4.h"
#include "vec4x1.h"
#include "defines.h"
#include "object.h"

/*================================================================*/

void Objlist_clear ( OBJLIST *lst )
{
   lst->obj_count = 0;
   lst->head = NULL;
   lst->tail = NULL;
}

/*================================================================*/

void Objlist_add ( OBJLIST *lst, OBJECT *obj )
{
   OBJECT *tmp;

   if ( lst->head )
   {
      tmp = lst->tail;
      lst->tail->next = obj;
      /* move tail forward */
      lst->tail = lst->tail->next;
      lst->tail->prev = tmp;
      lst->tail->next = NULL;
   }
   /* empty list so initialize head and tail */
   else
   {
      lst->head = lst->tail = obj;
      lst->head->prev = lst->tail->prev = NULL;
      lst->head->next = lst->tail->next = NULL;
   }

   lst->obj_count++;
}

/*================================================================*/

void Objlist_del ( OBJLIST *lst, OBJECT *obj )
{
   if ( lst->head )
   {
      /* is the object at the head of the list */
      if ( obj->prev == NULL )
      {
         /* is object both at head and tail */
         if ( obj->next == NULL )
         {
            lst->head = lst->tail = NULL;
            obj->next = obj->prev = NULL;
         }
         else
         {
            lst->head = obj->next;
            lst->head->prev = NULL;
         }
      }
      /* is the object at the end of the list */
      else if ( obj->next == NULL )
      {
         lst->tail = obj->prev;
         lst->tail->next = NULL;
      }
      /* otherwise object somewhere in the list */
      else 
      {
         obj->prev->next = obj->next;
         obj->next->prev = obj->prev;
      }

      obj->next = obj->prev = NULL;
      lst->obj_count--;
   }
}

/*================================================================*/

int Objlist_count ( OBJLIST *lst )
{
   return lst->obj_count;
}

/*================================================================*/
OBJECT * Object_new ( void )
{
   OBJECT *obj;

   obj = ( OBJECT * ) malloc ( sizeof ( OBJECT ) );

   return obj;
}
/*================================================================*/

void Object_delete ( OBJECT *obj )
{
   free ( obj );
   obj = NULL;
}

/*================================================================*/

void Object_init ( OBJECT *obj )
{
   obj->active = FALSE;
   obj->frame = 0L;
   obj->delay = 0L;
   obj->max_frame = 0L;

   obj->zone = 0L;
   obj->zheight = 0L;

   Vector_init ( obj->pos );
   Vector_init ( obj->old_pos );
   Vector_init ( obj->dir );
   Vector_init ( obj->thrust );

   obj->actionfn = NULL;
   obj->drawfn   = NULL;

   obj->next = NULL;
   obj->prev = NULL;
}

/*================================================================*/

void Object_set_actionfn ( OBJECT *obj, PFV1 actionfn )
{
   obj->actionfn = actionfn;
}

/*================================================================*/

void Object_set_drawfn ( OBJECT *obj, PFV2 drawfn )
{
   obj->drawfn = drawfn;
}

/*================================================================*/

void Object_set_model ( OBJECT *obj, MODEL *model )
{
   obj->model = model;
}

/*================================================================*/

void Object_set_pos ( OBJECT *obj, VECTOR4 pos )
{
   Vector_copy ( pos, obj->pos );
}

/*================================================================*/
void Object_get_pos ( OBJECT *obj, VECTOR4 pos )
{
   Vector_copy ( obj->pos, pos );
}

/*================================================================*/
void Object_set_dir  ( OBJECT *obj, VECTOR4 dir )
{
   Vector_copy ( dir, obj->dir );
}

/*================================================================*/
void Object_get_dir  ( OBJECT *obj, VECTOR4 dir )
{
   Vector_copy ( obj->dir, dir );
}

/*================================================================*/

void Object_draw ( OBJECT *obj )
{
}

/*================================================================*/
