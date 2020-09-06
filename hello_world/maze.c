#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

#ifndef TRUE
#define TRUE (0==0)
#define FALSE (!TRUE)
#endif

static unsigned char current_intensity = 127;

int tx(int x) { // rescale
  return x*50;
}
int ty(int y) {
  return y*50;
}

void line(int xl, int yb, int xr, int yt) {
  v_directDraw32(tx(xl),ty(yb), tx(xr),ty(yt), current_intensity);
}

// original author 'CatOnTreeStudio' at scratch.mit.edu
// hacked by gtoal@gtoal.com for Vectrex

// To do: use perspective spacing for walls as in
// https://scratch.mit.edu/projects/157841571/
// (Currently spacing is linear and doesn't look quite right)

#define KEY_RIGHT 0
#define KEY_LEFT 1
#define KEY_UP 2
#define KEY_DOWN 3
#define KEY_ANY 4

int key(int code) {
  int bit[5] = { 0x88, 0x44, 0x22, 0x11, 0xFF };
  return currentButtonState & bit[code];
}

#define FADE 8
void intensity(int i) {
  v_setBrightness(current_intensity = i);
}

static int
  dist, rotation=1, stepX=0, stepY=0, KeyPressOn=0,
  Size_X,
  Size_Y,
  HeroPosY,
  HeroPosX,
  ScreenSizeX,
  ScreenSizeY,
  ScreenPosX,
  ScreenPosY;

// The original scratch code supported coloured blocks and the different
// codes (1:7) below represent different coloured blocks (walls).
// (9 is a corridor space and I think 8 was for an object in a space)
// If the maze is pre-built like this, using a large array is OK as it
// is const and therefore in ROM space.  However if you need dynamic
// dungeon generation then the array could be shrunk considerably by
// replacing each byte with a single bit (since the vectrex only
// supports one colour), and packing 8 bits per byte.

// (the fenceposts are because Scratch's arrays are based at 1 rather than 0.
// - easier to add an exra element than to adjust all the indexes)
const char *Level[] = {
  "1111111111111111111111"
  "1111111111111111111111",
  "1199919119991199991111", // <-- starts in northwest corner (2,2)
  "1191119999199994491111",
  "1191111119194444491111",
  "1191111119199999499911",
  "1199999989111944444911",
  "1191111939111999494911",
  "1191199939991119994911",
  "1111193333391119199911",
  "1199199933399919111111",
  "1191111933333919999111",
  "1199999939933919119911",
  "1191111933933919111911",
  "1191111933933911111911",
  "1191191993999999199911",
  "1199991193933919991911",
  "1191191993933911111911",
  "1191191933933911999911",
  "1191191993999911111211",
  "1111111113333111111111",
  "1111111111111111111111",
};

// left and right wall code could be merged into one procedure at the expense of clarity.
// I'm OK with the redundancy for now.

void DrawLeftWall (int WallPosXLeft, int WallPosYUp, int WallPosYDown, int stepX, int stepY) {
  if (Level[(HeroPosY + dist * stepY - stepX)][(HeroPosX + dist * stepX + stepY)] > '7') {
    if (Level[(HeroPosY + (dist + 1) * stepY - stepX)][(HeroPosX + (dist + 1) * stepX + stepY)] > '7') {
      if (WallPosYUp - (240 + (dist + 1) * Size_X) * 3/8 != 0) {
        line(WallPosXLeft + dist * Size_X, WallPosYUp - (240 + dist * Size_X) * 3/8,
             WallPosXLeft + (dist + 1) * Size_X, WallPosYUp - (240 + (dist + 1) * Size_X) * 3/8);
      }
      if (WallPosYDown + (dist + 1) * Size_Y != 0) {
        line(WallPosXLeft + (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y,
             WallPosXLeft + (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y);
        line(WallPosXLeft + (dist + 1) * Size_X, WallPosYDown + (240 + (dist + 1) * Size_X) * 3/8,
             WallPosXLeft + dist * Size_X, WallPosYDown + (240 + dist * Size_X) * 3/8);
      }
    } else {
      if (WallPosYUp - (dist + 1) * Size_Y != 0) {
        line(WallPosXLeft + dist * Size_X, WallPosYUp - (dist + 1) * Size_Y,
             WallPosXLeft + (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y);
        line(WallPosXLeft + (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y,
             WallPosXLeft + (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y);
        line(WallPosXLeft + (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y,
             WallPosXLeft + dist * Size_X, WallPosYDown + (dist + 1) * Size_Y);
      }
    }
  } else {
    if (WallPosYDown + (dist + 1) * Size_Y != 0) {
      line(WallPosXLeft + dist * Size_X, WallPosYUp - dist * Size_Y,
           WallPosXLeft + (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y);
      line(WallPosXLeft + (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y,
           WallPosXLeft + (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y);
      line(WallPosXLeft + (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y,
           WallPosXLeft + dist * Size_X, WallPosYDown + dist * Size_Y);
    }
  }
}

void DrawRightWall (int WallPosXRight, int WallPosYUp, int WallPosYDown, int stepX, int stepY) {
  if (Level[(HeroPosY + dist * stepY + stepX)][(HeroPosX + dist * stepX - stepY)] > '7') {
    if (Level[(HeroPosY + (dist + 1) * stepY + stepX)][(HeroPosX + (dist + 1) * stepX - stepY)] > '7') {
      if (WallPosYUp - (dist + 1) * Size_Y != 0) {
        line(WallPosXRight - dist * Size_X, WallPosYUp - (240 + dist * Size_X) * 3/8,
             WallPosXRight - (dist + 1) * Size_X, WallPosYUp - (240 + (dist + 1) * Size_X) * 3/8);
        line(WallPosXRight - (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y,
             WallPosXRight - (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y);
        line(WallPosXRight - (dist + 1) * Size_X, WallPosYDown + (240 + (dist + 1) * Size_X) * 3/8,
             WallPosXRight - dist * Size_X, WallPosYDown + (240 + dist * Size_X) * 3/8);
      }
    } else {
      if (WallPosYUp - (dist + 1) * Size_Y != 0) {
        line(WallPosXRight - dist * Size_X, WallPosYUp - (dist + 1) * Size_Y,
             WallPosXRight - (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y);
        line(WallPosXRight - (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y,
             WallPosXRight - (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y);
        line(WallPosXRight - (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y,
             WallPosXRight - dist * Size_X, WallPosYDown + (dist + 1) * Size_Y);
      }
    }
  } else {
    if (WallPosYDown + (dist + 1) * Size_Y != 0) {
      line(WallPosXRight - dist * Size_X, WallPosYDown + dist * Size_Y,
           WallPosXRight - dist * Size_X, WallPosYUp - dist * Size_Y);
      line(WallPosXRight - dist * Size_X, WallPosYUp - dist * Size_Y,
           WallPosXRight - (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y);
      line(WallPosXRight - (dist + 1) * Size_X, WallPosYUp - (dist + 1) * Size_Y,
           WallPosXRight - (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y);
      line(WallPosXRight - (dist + 1) * Size_X, WallPosYDown + (dist + 1) * Size_Y,
           WallPosXRight - dist * Size_X, WallPosYDown + dist * Size_Y);
    }
  }
}

void BuildBackgroundWall (int WallPosXLeft, int WallPosXRight, int WallPosYDown, int WallPosYUp, int stepX, int stepY) {
  dist = 0;
  intensity (64 - dist * FADE);
  while (!( (Level[(HeroPosY + dist * stepY)][(HeroPosX + dist * stepX)] < '8') || (dist > 4) )) {
    DrawLeftWall (WallPosXLeft, WallPosYUp, WallPosYDown, stepX, stepY);
    DrawRightWall (WallPosXRight, WallPosYUp, WallPosYDown, stepX, stepY);
    dist += 1;
    intensity (64 - dist * FADE);
  }
  intensity (64 - (dist - 1) * FADE);
  if (WallPosYDown + dist * Size_Y != 0) {
    line(WallPosXLeft + dist * Size_X, WallPosYDown + dist * Size_Y,
         WallPosXRight - dist * Size_X, WallPosYDown + dist * Size_Y);
    line(WallPosXRight - dist * Size_X, WallPosYUp - dist * Size_Y,
         WallPosXLeft + dist * Size_X, WallPosYUp - dist * Size_Y);
  }
}

void DrawFrame(void) {

  v_WaitRecal();
  //v_doSound();
  v_setBrightness(64);        /* set intensity of vector beam... */
  v_readButtons();
  v_readJoystick1Analog();
  //v_playAllSFX();

  v_setBrightness(60);
  v_printStringRaster(-60,-120, "BACK  FORWARD  LEFT  RIGHT", 30, -7, '\0');

  if (rotation == 0) {
    stepX = 1; stepY = 0;
  } else if (rotation == 1) {
    stepX = 0; stepY = 1;
  } else if (rotation == 2) {
    stepX = -1; stepY = 0;
  } else /* rotation == 3 */ {
    stepX = 0; stepY = -1;
  }

  if (KeyPressOn && !key(KEY_ANY)) {
    KeyPressOn = 0;
  } else if (key(KEY_LEFT)) {
    if (KeyPressOn == 0) { rotation += 3; KeyPressOn = 1; }
  } else if (key(KEY_RIGHT)) {
    if (KeyPressOn == 0) { rotation += 1; KeyPressOn = 1; }
  } else if (key(KEY_UP)) {
    if (KeyPressOn == 0) {
      if (Level[(HeroPosY + stepY)][(HeroPosX + stepX)] > '7') { HeroPosX += stepX; HeroPosY += stepY; }
      KeyPressOn = 1;
    }
  } else if (key(KEY_DOWN)) {
    if (KeyPressOn == 0) {
      if (Level[(HeroPosY - stepY)][(HeroPosX - stepX)] > '7') { HeroPosX -= stepX; HeroPosY -= stepY; }
      KeyPressOn = 1;
    }
  }
  rotation &= 3;

  BuildBackgroundWall (ScreenPosX, ScreenPosX + ScreenSizeX, ScreenPosY, ScreenPosY + ScreenSizeY,
		       ((rotation^1)&1) * -((rotation&2)-1), (rotation&1) * -((rotation&2)-1));
}


int main(int argc, char **argv) {
  HeroPosY = 2; HeroPosX = 2; // initial square
  ScreenSizeX = 480; ScreenSizeY = 360; // size of scratch display
  ScreenPosX = -240; ScreenPosY = -180; // center
  Size_X = ScreenSizeX / 10; Size_Y = ScreenSizeY / 10;
  vectrexinit(1);
  v_init();
  usePipeline = 1;
  v_setRefresh(60);

  for (;;) DrawFrame();

  return 0;
}
