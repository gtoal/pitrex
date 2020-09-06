/* graphics.h */

extern int IsKeyDown(int key);
extern void Polyline(POINT *pts,int n);
extern void SetPixel(int x,int y,int c);
extern void set_colour(int c);
extern void score_graphics(int level,int score,int lives,int shield,int bomb);
extern void graphics_init(int argc,char *argv[],int *palrgb);
extern void graphics_update(void);
extern void graphics_exit(void);
