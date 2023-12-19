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
  if (strcmp(name, "Microsoft Microsoft SideWinder Precision Pro (USB)")!=0) {
    printf("This test program is only for the Microsoft SideWinder Precision Pro,\n");
    printf("so I will exit now. Please use the generic joytest program instead\n");
    printf("and if possible customise it to your specific joystick.\n");
    exit(0);
  }
}

#define MAX_SIDEWINDER_PRO_AXES 6
char *Axis[MAX_SIDEWINDER_PRO_AXES] = { // "RTZ" means "Returns to Zero", i.e. sprung.
  /* 0 */ "Axis 0 (RTZ Big Stick Left:Right Analog)",
  /* 1 */ "Axis 1 (RTZ Big Stick Forward:Back Analog)",
  /* 2 */ "Axis 2 (RTZ Big Stick Rotation Anticlockwise:Clockwise Analog)",
  /* 3 */ "Axis 3 (Unsprung Rotary Knob Clockwise:Anticlockwise Analog)",
  /* 4 */ "Axis 4 (RTZ Thumb Button Left:Right Digital)",
  /* 5 */ "Axis 5 (RTZ Thumb Button Forward:Back Digital)",
};
static int axisval[MAX_SIDEWINDER_PRO_AXES] = {0,0,0,0,0,0};

char *Direction[MAX_SIDEWINDER_PRO_AXES][2] = {
  {"Left","Right"},
  {"Forward","Back"},
  {"Anticlockwise","Clockwise"},
  {"Clockwise","Anticlockwise"},
  {"Left","Right"},
  {"Forward","Back"},
};

#define MAX_SIDEWINDER_PRO_BUTTONS 9
char *Button[MAX_SIDEWINDER_PRO_BUTTONS] = {
  /* 0 */ "Button 0 (Trigger)",
  /* 1 */ "Button 1 (Thumb left)",
  /* 2 */ "Button 2 (Thumb Right Upper)",
  /* 3 */ "Button 3 (Thumb Right Lower)",
  /* 4 */ "Button 4 (A)",
  /* 5 */ "Button 5 (B)",
  /* 6 */ "Button 6 (C)",
  /* 7 */ "Button 7 (D)",
  /* 8 */ "Button 8 (Base =>)",
};

char *Pressed[2] = { "released", "pressed" };

void process_event(struct js_event jse) {
  if (jse.type == 2) {
    printf("%s Value %d %s %s  Delta=%d\n", Axis[jse.number], jse.value,
           jse.value > axisval[jse.number] ? "Increasing" :
           (jse.value < axisval[jse.number] ? "Decreasing" : "Unchanged"),
           Direction[jse.number][jse.value > axisval[jse.number]?1:0],
           jse.value-axisval[jse.number]);
    axisval[jse.number] = jse.value;
  } else if (jse.type == 1) {
    printf("%s %s\n", Button[jse.number], Pressed[jse.value]);
  } else if (jse.type == 130) {
    printf("%s Supported\n", Axis[jse.number]);
  } else if (jse.type == 129) {
    printf("%s Supported\n", Button[jse.number]);
  } else {
    printf("Type=%d Value=%d\n", jse.type, jse.number);
  }
}

int main(int argc, char **argv) {
  int fd;
  char *device_name = "/dev/input/js0";
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

  while (1) {
    while (read(fd, &jse, sizeof(jse)) > 0) {
      process_event(jse);
    }
  }

  return 0;
}
