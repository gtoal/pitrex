#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>

#define msleep(ms) usleep((ms)*1000)

// enable and configure an absolute "position" analog channel
static void setup_abs(int fd, unsigned chan, int min, int max) {
  if (ioctl(fd, UI_SET_ABSBIT, chan)) perror("UI_SET_ABSBIT");
  
  struct uinput_abs_setup s = {
    .code = chan,
    .absinfo = { .minimum = min,  .maximum = max },
  };

  if (ioctl(fd, UI_ABS_SETUP, &s)) perror("UI_ABS_SETUP");
}

// Convert Vectrex controller to /dev/js<n> for next free 'n'.
int main(int argc, char **argv) { 
  struct uinput_setup setup = {
     .name = "Vectrex Controller (Right)",
     .id = {
       .bustype = BUS_USB,
       .vendor  = 0x3,
       .product = 0x3,
       .version = 2,
     }
  };
  int VEC_1, VEC_2, VEC_3, VEC_4; // Vectrex button numbers
  int x = 0, y = 0; // current joystick value
  int LEFT_CONTROLLER = 0;
  int fd;

  if ((argc == 2) && ((strcmp(argv[1], "-l") == 0) || (strcmp(argv[1], "--left") == 0))) LEFT_CONTROLLER = 1;
  if ((argc == 2) && ((strcmp(argv[1], "-r") == 0) || (strcmp(argv[1], "--right") == 0))) LEFT_CONTROLLER = 0;
  
  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  
  if (fd < 0) { perror("Cannot open /dev/uinput (You may need to use sudo)"); return 1; }

  ioctl(fd, UI_SET_EVBIT, EV_KEY); // enable button/key handling
  
  ioctl(fd, UI_SET_KEYBIT, BTN_A);
  ioctl(fd, UI_SET_KEYBIT, BTN_B);
  ioctl(fd, UI_SET_KEYBIT, BTN_X);
  ioctl(fd, UI_SET_KEYBIT, BTN_Y);
  
  ioctl(fd, UI_SET_EVBIT, EV_ABS); // enable analog absolute position handling

  // scaled to match standard linux joystick values
  setup_abs(fd, ABS_X,  -32767, 32767);
  setup_abs(fd, ABS_Y,  -32767, 32767);
  
  if (LEFT_CONTROLLER) {
    strcpy(setup.name, "Vectrex Controller (Left)");
  } else {
    strcpy(setup.name, "Vectrex Controller (Right)");
  }

  if (ioctl(fd, UI_DEV_SETUP, &setup)) { perror("UI_DEV_SETUP"); return 1; }
  
  if (ioctl(fd, UI_DEV_CREATE)) { perror("UI_DEV_CREATE"); return 1; }

  // This may unfortunately clash with any application accessing the
  // Vectrex.  Preferably should be ported to the new RPC environment:
  vectrexinit (1);
  v_setName(LEFT_CONTROLLER ? "vecjoy-l" : "vecjoy-r");
  v_init ();
  v_setRefresh (60);

  if (LEFT_CONTROLLER) {
    VEC_1 = 16; VEC_2 = 32; VEC_3 = 64; VEC_4 = 128;
  } else {
    VEC_1 = 1; VEC_2 = 2; VEC_3 = 4; VEC_4 = 8;
  }

  for (;;) {
    struct input_event ev[7];
    memset(&ev, 0, sizeof ev);

    //msleep(1000/60);
    v_WaitRecal ();
    v_readButtons ();
    v_readJoystick1Analog (); // Actually reads both.

    ev[0].type = EV_KEY; ev[0].code = BTN_A; ev[0].value = (currentButtonState&VEC_1) ? !0 : !(0b1000000);
    ev[1].type = EV_KEY; ev[1].code = BTN_B; ev[1].value = (currentButtonState&VEC_2) ? !0 : !(0b0100000);
    ev[2].type = EV_KEY; ev[2].code = BTN_X; ev[2].value = (currentButtonState&VEC_3) ? !0 : !(0b0010000);
    ev[3].type = EV_KEY; ev[3].code = BTN_Y; ev[3].value = (currentButtonState&VEC_4) ? !0 : !(0b0001000);

    ev[4].type = EV_ABS; ev[4].code = ABS_Y;
    if (LEFT_CONTROLLER) {
      ev[4].value = currentJoy2Y<<8;
    } else {
      ev[4].value = currentJoy1Y<<8;
    }
      
    ev[5].type = EV_ABS; ev[5].code = ABS_X;
    if (LEFT_CONTROLLER) {
      ev[5].value = currentJoy2X<<8; // Vectrex returns -128:127, Linux expects -32767:32767
    } else {
      ev[5].value = currentJoy1X<<8;
    }
      
    // sync event tells input layer we're done with a "batch" of updates
    ev[6].type = EV_SYN; ev[6].code = SYN_REPORT; ev[6].value = 0;

    if (write(fd, &ev, sizeof ev) < 0) { perror("write"); return 1; }
  }

  /* NOT REACHED */
  if (ioctl(fd, UI_DEV_DESTROY)) { printf("UI_DEV_DESTROY"); return 1; }

  close(fd);
  return 0;
}

