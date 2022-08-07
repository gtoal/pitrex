void gles2_draw(void * screen, int width, int height, int depth, unsigned short *palette);
void gles2_create(int display_width, int display_height, int bitmap_width, int bitmap_height, int depth);
void gles2_destroy();
void gles2_palette_changed();
extern EGLDisplay display;
extern EGLSurface surface;
