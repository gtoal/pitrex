//#include <vectrex/graphics_lib.h>
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for exit()
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open_joystick(char *device_name) {
int fd = -1;

 if (device_name == NULL) {
   return fd;
 }

 fd = open(device_name, O_RDONLY | O_NONBLOCK);

 return fd;
}

int axes=0, buttons=0;
void print_device_info(int fd) {
  char name[128];

  ioctl(fd, JSIOCGAXES, &axes);
  ioctl(fd, JSIOCGBUTTONS, &buttons);
  ioctl(fd, JSIOCGNAME(sizeof(name)), &name);

  printf("Your joystick is a \"%s\"\nIt supports %d Axes and %d Buttons\n", name, axes, buttons);
}

char *Pressed[2] = { "released", "pressed" };

static int LinJoyX = 0, LinJoyY = 0, LinButtons = 0;

void process_event(struct js_event jse) {
  if (jse.type == 2) {
    printf("Axis %d Value %d\n", jse.number, jse.value);
    if (jse.number == 0) LinJoyX = jse.value>>8;
    else if (jse.number == 1) LinJoyY = jse.value>>8;
  } else if (jse.type == 1) {
    printf("Button %d %s\n", jse.number, Pressed[jse.value]);
    if (jse.value == 0) {
      if ((0 <= jse.number) && (jse.number <= 3)) LinButtons &= ~(1<<jse.number);
      else if ((4 <= jse.number) && (jse.number <= 7)) LinButtons &= ~(1<<(jse.number-4));
    } else {
      if ((0 <= jse.number) && (jse.number <= 3)) LinButtons |= 1<<jse.number;
      else if ((4 <= jse.number) && (jse.number <= 7)) LinButtons |= 1<<(jse.number-4);
    }
  } else if (jse.type == 130) {
    printf("Axis %d Supported\n", jse.number);
  } else if (jse.type == 129) {
    printf("Button %d Supported\n", jse.number);
  } else {
    printf("Type=%d Value=%d\n", jse.type, jse.number);
  }
}

void startFrame (void) {
  v_WaitRecal ();
  //v_readButtons ();
  //v_readJoystick1Analog ();
}

int main(int argc, char **argv) {
  int tick = 0;
  int x, y;
  int fd;
  char *device_name = "/dev/input/js0"; // Sidewinder pro for now on my machine. Unfortunately button numbers are different from Vectrex controller unless remapped
  struct js_event jse;

  if (strrchr(argv[0], '/')) argv[0] = strrchr(argv[0], '/')+1;
  if (argc == 2) device_name = argv[1];
  if (argc > 2) {
    printf("syntax: %s {optional joystick device such as /dev/input/js0}\n", argv[0]);
    exit(1);
  }
  fd = open_joystick(device_name);
  if (fd < 0) {
    printf("Could not locate joystick device %s\n", device_name);
    exit(1);
  }

  print_device_info(fd);

  vectrexinit (1);
  v_setName("linjoy");
  v_init ();
  v_setRefresh (60);

  for (;;) {
    // read joysticks, buttons ...
    while (read(fd, &jse, sizeof(jse)) > 0) {
      process_event(jse);
    }

    startFrame();

    //if (tick++ == 2000) { // temporary failsafe to exit if I mess up with menu actions
    //  exit(0);
    //}

    // Buttons not tested yet.  To do.
    
    x = LinJoyX;//currentJoy1X;
    y = LinJoyY;//currentJoy1Y;
    fprintf(stderr, "x=%d y=%d\n", x, y);
    
    x *= 200; y *= 150;
    
    v_directDraw32(x-500,    y, x+500,    y, 127);
    v_directDraw32(   x, y-500,    x, y+500, 127);

    v_directDraw32(-10000, -10000,  -10000,  10000, 64);
    v_directDraw32(-10000,  10000,   10000,  10000, 64);
    v_directDraw32( 10000,  10000,   10000, -10000, 64);
    v_directDraw32( 10000, -10000,  -10000, -10000, 64);
    v_printString(-10,0, "HELLO", 7, 64);
    v_setBrightness(60);
    v_printStringRaster(-8,-4, "WORLD", 5*8, -7, '\0');
    
#ifdef NEVER
    v_directDraw32Patterned(xl, yb, xr, yt, pattern, bri);
#endif

  }
  exit(0);
  return 0;
}

