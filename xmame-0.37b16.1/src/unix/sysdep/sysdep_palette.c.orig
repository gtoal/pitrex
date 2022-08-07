/* Sysdep palette abstraction and emulation object

   Copyright 1999,2000 Hans de Goede
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
/* Changelog
Version 0.1, November 1999
-initial release (Hans de Goede)
*/
#include <stdio.h>
#include <stdlib.h>
#include <driver.h>
#include <math.h>
#include "sysdep_palette.h"

/* from xmame.h, since in the future we want this to be entirely mame
   independent */
extern struct sysdep_palette_info display_palette_info;
int  sysdep_display_alloc_palette(int writable_colors);
int  sysdep_display_set_pen(int pen, unsigned char red, unsigned char green, unsigned char blue);
extern int widthscale, heightscale;

/* private methods */
static int sysdep_palette_make_pen_from_info(struct sysdep_palette_info
   *info, unsigned char red, unsigned char green, unsigned char blue)
{
   int pen = 0;
   
   /* are the shifts initialised ? */
   if(!info->red_shift)
   {
      for(pen = 1 << (8 * sizeof(pen) - 1); pen && (!(pen & info->red_mask));
         pen >>= 1, info->red_shift++);

      for(pen = 1 << (8 * sizeof(pen) - 1); pen && (!(pen & info->green_mask));
         pen >>= 1, info->green_shift++);

      for(pen = 1 << (8 * sizeof(pen) - 1); pen && (!(pen & info->blue_mask));
         pen >>= 1, info->blue_shift++);
   }
   
   pen  = ((red   << 24) >> info->red_shift)   & info->red_mask;
   pen |= ((green << 24) >> info->green_shift) & info->green_mask;
   pen |= ((blue  << 24) >> info->blue_shift)  & info->blue_mask;
   
   return pen;
}


/* public methods */
struct sysdep_palette_struct *sysdep_palette_create(int depth,
   int writable_colors)
{
   struct sysdep_palette_struct *palette = NULL;
   int lookup_size = 0;
   
   /* verify if the display can handle the requested depth */
   if ( display_palette_info.depth < depth )
   {
      fprintf(stderr,
         "error in sysdep_palette_create: %d bpp requested on a %d bpp display\n",
         depth, display_palette_info.depth);
      return NULL;
   }
   
/* If the display gets recreated this must be done again, but the
   palette can be kept, so now the creator of the display is responsible
   for calling this. Also because there can be 2 palette's for one
   display (the normal and debugger one in xmame for example) */
#if 0
   /* if the display is 8 bpp allocate the nescesarry pens for displays
      with a shared palette like X */
   if (display_palette_info.depth == 8)
      if (sysdep_display_alloc_palette(writable_colors))
         return NULL;
#endif
   
   /* allocate the palette struct */
   if (!(palette = calloc(1, sizeof(struct sysdep_palette_struct))))
   {
      fprintf(stderr,
         "error malloc failed for struct sysdep_palette_struct\n");
      return NULL;
   }
   
   palette->brightness   = 100;
   palette->gamma        = 1.0;
   palette->bright_dirty = 1;
   
   /* allocate the color lookup cache for get_pen */
   if (writable_colors)
   {
      palette->colors = calloc(writable_colors, 3);
      palette->color_dirty = calloc(writable_colors, 1);
      if(!palette->colors || !palette->color_dirty)
      {
         fprintf(stderr, "error malloc failed for color cache\n");
         sysdep_palette_destroy(palette);
         return NULL;
      }
   }
   
   /* can we do our 8 -> 16 bpp speedup hack?
      do not check for the scaleing factors sicne those might change */
   if((depth == 8) && (display_palette_info.depth == 16))
      lookup_size = 65536;
   /* do we need a lookup table? */
   else if ( (display_palette_info.depth != depth) ||
        ( (depth == 16) && writable_colors) )
      lookup_size = writable_colors? writable_colors:32768;
      
   if (lookup_size && !(palette->lookup = calloc(lookup_size, sizeof(int))))
   {
      fprintf(stderr, "error malloc failed for color lookup table\n");
      sysdep_palette_destroy(palette);
      return NULL;
   }
      
   /* do we need to fill the lookup table? */
   if (palette->lookup && !writable_colors)
   {
      int r,g,b;
      
      for(r=0; r<32; r++)
         for(g=0; g<32; g++)
            for(b=0; b<32; b++)
               palette->lookup[r*1024 + g*32 + b] =
                  sysdep_palette_make_pen_from_info(&display_palette_info,
                     r*8, g*8, b*8);
     
      palette->lookup_dirty = 1;
   }
   
   /* build the emulated palette info */
   palette->emulated.writable_colors = writable_colors;
   palette->emulated.depth           = depth;
   /* fill in the masks and shifts if nescesarry */
   if(!writable_colors)
   {
      if(palette->lookup)
      {
         /* if we're emulating a truecolor palette and we use a lookup
            table, we always emulate 555 rgb */
         palette->emulated.red_mask    = 0x7C00;
         palette->emulated.green_mask  = 0x03E0;
         palette->emulated.blue_mask   = 0x001F;
      }
      else
      {
         /* otherwise we're doing direct copy so use the display's values */
         palette->emulated = display_palette_info;
      }
   }
   
   /* and mark the palette dirty so that the lookup table (if needed)
      get's initalised upon the first sysdep_palette_update() */
   sysdep_palette_mark_dirty(palette);

   return palette;
}

/* destructor */
void sysdep_palette_destroy(struct sysdep_palette_struct *palette)
{
   if(palette->colors)
      free(palette->colors);
   if(palette->color_dirty)
      free(palette->color_dirty);
   if(palette->lookup)
      free(palette->lookup);
   free(palette);
}

/* for pseudo color modes */   
int sysdep_palette_set_pen(struct sysdep_palette_struct *palette, int pen,
   unsigned char red, unsigned char green, unsigned char blue)
{
   if(pen >= palette->emulated.writable_colors)
   {
      fprintf(stderr,
         "error in sysdep_palette_set_pen: pen (%d) out of range for writable_colors (%d)\n",
         pen, palette->emulated.writable_colors);
      return -1;
   }
  
   if ( (palette->colors[pen*3]   == red)   &&
        (palette->colors[pen*3+1] == green) &&
        (palette->colors[pen*3+2] == blue) )
      return 0;
     
   palette->colors[pen*3]   = red;
   palette->colors[pen*3+1] = green;
   palette->colors[pen*3+2] = blue;
   
   palette->color_dirty[pen] = 1;
   palette->dirty = 1;
   return 0;
}

int sysdep_palette_get_pen(struct sysdep_palette_struct *palette, int pen,
   unsigned char *red, unsigned char *green, unsigned char *blue)
{
   if (palette->emulated.writable_colors)
   {
      if(pen >= palette->emulated.writable_colors)
      {
         fprintf(stderr,
            "error in sysdep_palette_get_pen: pen (%d) out of range for writable_colors (%d)\n",
            pen, palette->emulated.writable_colors);
         return -1;
      }
      *red   = palette->colors[pen*3];
      *green = palette->colors[pen*3+1];
      *blue  = palette->colors[pen*3+2];
   }
   else
   {
      *red   = ((pen & palette->emulated.red_mask) <<
         palette->emulated.red_shift) >> 24;
      *green = ((pen & palette->emulated.green_mask) <<
         palette->emulated.green_shift) >> 24;
      *blue  = ((pen & palette->emulated.blue_mask) <<
         palette->emulated.blue_shift) >> 24;
   }
   
   return 0;
}


/* for true color modes */   
int sysdep_palette_make_pen(struct sysdep_palette_struct *palette,
   unsigned char red, unsigned char green, unsigned char blue)
{
   if(palette->emulated.writable_colors)
   {
      fprintf(stderr,
         "error sysdep_palette_make_pen called for writable palette\n");
      return -1;
   }
   
   return sysdep_palette_make_pen_from_info(&palette->emulated, red, green,
      blue);
}


void sysdep_palette_update(struct sysdep_palette_struct *palette)
{
   int i=0;
   
   palette->lookup_dirty = 0;
   
   if (palette->bright_dirty)
   {
      for (i=0; i<256; i++)
         palette->bright_lookup[i] = (float)palette->brightness *
            pow(i / 255.0, 1 / palette->gamma) * 2.55 + 0.5;
      sysdep_palette_mark_dirty(palette);
      palette->bright_dirty = 0;
   }
   
   if (palette->dirty)
   {
      for (i=0; i<palette->emulated.writable_colors; i++)
      {
         if(palette->color_dirty[i])
         {
            unsigned char red, green, blue;

            if (i != Machine->uifont->colortable[1])
            {
               red = palette->bright_lookup[palette->colors[i*3]];
               green = palette->bright_lookup[palette->colors[i*3+1]];
               blue = palette->bright_lookup[palette->colors[i*3+2]];
            }
            else
            {
               red = palette->colors[i*3];
               green = palette->colors[i*3+1];
               blue = palette->colors[i*3+2];
            }
            
            if(!palette->lookup)
            {
               sysdep_display_set_pen(i, red, green, blue);
            }
            else
            {
               int color = sysdep_palette_make_pen_from_info(
                  &display_palette_info, red, green, blue);
               
               /* can we do our 8 -> 16 bpp speedup hack? */
               if((palette->emulated.depth == 8) &&
                  (display_palette_info.depth == 16) &&
                  (widthscale == 1) &&
                  (heightscale <= 2))
               {
                  int n;
                  for(n=0;n<256;n++)
                  {
                     palette->lookup[(n<<8) | i] &= 0xffff0000;
                     palette->lookup[(n<<8) | i] |= color;
                     
                     palette->lookup[(i<<8) | n] &= 0x0000ffff;
                     palette->lookup[(i<<8) | n] |= color << 16;
                  }
               }
               else
                  palette->lookup[i] = color;
               
               palette->lookup_dirty = 1;
            }
            palette->color_dirty[i] = 0;
         }
      }
      palette->dirty = 0;
   }
}

/* This is broken, and for now is no longer used, instead
   sysdep_palette_marked dirty should be used,
   and display_alloc_palette palette must be called every time a dispay is
   created. So also on recreation! */
int sysdep_palette_change_display(struct sysdep_palette_struct **palette)
{
   int i;
   struct sysdep_palette_struct *new_palette = NULL;
   
   if(!(new_palette=sysdep_palette_create((*palette)->emulated.depth,
      (*palette)->emulated.writable_colors)))
      return -1;
   
   if ((*palette)->emulated.writable_colors)
   {
      /* copy all the old colors */
      for(i=0; i<(*palette)->emulated.writable_colors; i++)
         sysdep_palette_set_pen(new_palette, i, (*palette)->colors[i*3],
            (*palette)->colors[i*3+1], (*palette)->colors[i*3+2]);
   }
   else
   {
      /* check that the color masks of the new palette are the same as the old
         colormasks, otherwise barf for now, we could emulate them later on
         as follows:
         - close new_palette
         - recreate new_palette writable
         - set all the pens of new_palette so that they match
           the old masks.
         - modify new_palette->emulated so that it becomes non-wrtiable,
           with the masks of the old palette.
         However if we're going this way, we might just as well
         add the possibility to sysdep_palette_create to force specific 
         colormasks, which we might want in the future anyway
      */
      if ( ((*palette)->emulated.red_mask   != new_palette->emulated.red_mask) ||
           ((*palette)->emulated.green_mask != new_palette->emulated.green_mask) ||
           ((*palette)->emulated.blue_mask  != new_palette->emulated.blue_mask))
      {
         fprintf(stderr, "error recreating palette, colormasks don't match!\n");
         sysdep_palette_destroy(new_palette);
         return -1;
      }
   }
   
   sysdep_palette_set_brightness(new_palette, (*palette)->brightness);
   sysdep_palette_set_gamma(new_palette, (*palette)->gamma);
   
   sysdep_palette_destroy(*palette);
   *palette = new_palette;
   return 0;
}

void sysdep_palette_mark_dirty(struct sysdep_palette_struct *palette)
{
   int i;
   
   if(palette->emulated.writable_colors)
   {
      for (i=0; i<palette->emulated.writable_colors; i++)
         palette->color_dirty[i] = 1;
      
      palette->dirty = 1;
   }
}

/* brightness = percentage 0-100% */
int sysdep_palette_set_brightness(struct sysdep_palette_struct *palette,
   int brightness)
{
   if (palette->brightness != brightness)
   {
      palette->brightness = brightness;
         
      /* failing isn't such a disaster thus we do it quietly */
      if(!palette->emulated.writable_colors)
         return -1;
      
      palette->bright_dirty = 1;
   }

   return 0;
}

int sysdep_palette_get_brightness(struct sysdep_palette_struct *palette)
{
   return palette->brightness;
}


int sysdep_palette_set_gamma(struct sysdep_palette_struct *palette,
   float gamma)
{
   if (palette->gamma != gamma)
   {
      palette->gamma = gamma;

      /* failing isn't such a disaster thus we do it quietly */
      if(!palette->emulated.writable_colors)
         return -1;

      palette->bright_dirty = 1;
   }

   return 0;
}

float sysdep_palette_get_gamma(struct sysdep_palette_struct *palette)
{
   return palette->gamma;
}
