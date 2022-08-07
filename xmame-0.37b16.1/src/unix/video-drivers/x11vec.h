/*****************************************************************

  X11 vector routines

  For PiTrex. Based on fxvec.c, glvec.c, and vidhrdw/vector.c
  (and yet I'm still confused).
  
  Rewritten into format of Glide vector driver (fxvec.c) based on
  XMAME 0.106 version.

*****************************************************************/

int vector_vh_start (void);
void vector_vh_stop (void);
void vector_set_shift (int shift);
void vector_add_point(int x, int y, int color, int intensity);
void vector_add_clip (int x1, int yy1, int x2, int y2);
void vector_clear_list(void);
void vector_vh_update(struct osd_bitmap *bitmap);
void vector_set_flip_x (int flip);
void vector_set_flip_y (int flip);
void vector_set_swap_xy (int swap);
void vector_set_flicker(float _flicker);
float vector_get_flicker(void);
void vector_set_gamma(float gamma);
float vector_get_gamma(void);
void vector_set_intensity(float _intensity);
float vector_get_intensity(void);
void vector_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
