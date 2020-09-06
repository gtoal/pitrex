#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

#ifndef TRUE
#define TRUE (0==0)
#define FALSE (!TRUE)
#endif

int tx(int x) { // rescale
  return x*50;
}

int ty(int y) {
  return y*50;
}

void line(int xl, int yb, int xr, int yt) {
  v_directDraw32(tx(xl),ty(yb), tx(xr),ty(yt), 64);
}

void DrawFrame(void) {
  int width, height, x, y, step;
  v_WaitRecal();
  //v_doSound();
  v_setBrightness(64);        /* set intensity of vector beam... */
  v_readButtons();
  v_readJoystick1Analog();
  //v_playAllSFX();

  v_setBrightness(60);

  height = 180; width = 80; x = 240;
  for (step = 0; step < 4; step++) {
    x = x - width;
    y = y - height;
    width = width*2/3;
    height = height*2/3;
  }
  line(-240,-180, -x,-height);
  line(x,height, 240,180);
  line(-240,180,  -x,height);
  line(x,-height,  240,-180);
  
  height = 180; width = 80; x = 240;
  for (step = 0; step < 5; step++) {
    line(-x, -height, -x, height);
    line(x, height, x, -height);
    x = x - width;
    y = y - height;
    width = width*2/3;
    height = height*2/3;
  }
}


int main(int argc, char **argv) {
  vectrexinit(1);
  v_init();
  usePipeline = 1;
  v_setRefresh(60);

  for (;;) DrawFrame();

  return 0;
}
