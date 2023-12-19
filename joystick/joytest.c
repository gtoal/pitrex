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

void process_event(struct js_event jse) {
  if (jse.type == 2) {
    printf("Axis %d Value %d\n", jse.number, jse.value);
  } else if (jse.type == 1) {
    printf("Button %d %s\n", jse.number, Pressed[jse.value]);
  } else if (jse.type == 130) {
    printf("Axis %d Supported\n", jse.number);
  } else if (jse.type == 129) {
    printf("Button %d Supported\n", jse.number);
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
