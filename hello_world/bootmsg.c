#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

void startFrame()
{
  v_WaitRecal();
  v_readButtons();
  v_readJoystick1Analog();
}

void show_ip(unsigned char *IP, int x, int y) {
#ifdef NEVER
    v_directDraw32(x-12000, y-2000,  x-12000, y+1000, 64);
    v_directDraw32(x-12000, y+1000,  x+12000, y+1000, 64);
    v_directDraw32(x+12000, y+1000,  x+12000, y-2000, 64);
    v_directDraw32(x+12000, y-2000,  x-12000, y-2000, 64);
#endif
    // because text is not using the same coordinate system as graphics, we have to
    // make a guess to map the text position to the same location as the graphics...
    v_printStringRaster((x/145)+3-6*strlen(IP),(y/156)-4, IP, strlen(IP)*6, -6, '\0');  // (145 and 156 determined by trial and error)
}

int main(int argc, char **argv) {
  int tick, x, y, vertical_direction, horizontal_direction;
  unsigned char IP[15];

  vectrexinit(1);
  v_init();
  v_setRefresh(60);

  tick = x = y = 0; vertical_direction = horizontal_direction = +1;
  // Bounce around screen to avoid burn-in
  for (;;) {
    if ((tick & 255) == 0) { // this has to be in the display loop because the IP is not always
      int fd;                // set up correctly immediately after booting.  This will ensure changes
      struct ifreq ifr;      // to the IP are reflected in the display.  Also catches DHCP changes.

      /* AF_INET - to define network interface IPv4 */
      fd = socket(AF_INET, SOCK_DGRAM, 0);   /* Create socket for it. */

      /* AF_INET - to define IPv4 Address type. */
      ifr.ifr_addr.sa_family = AF_INET;

      /* define the ifr_name - port name where network attached. */
      memcpy(ifr.ifr_name, "wlan0", 6/*IFNAMSIZ-1*/);

      /* Access network interface information by passing address using ioctl. */
      ioctl(fd, SIOCGIFADDR, &ifr);
      close(fd);

      /* Extract IP Address */
      strcpy(IP,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    }
    tick++;

    startFrame();
    v_setBrightness(60);
    //v_printStringRaster(0,0, "FIXED", strlen(IP)*5, -6, '\0');  // (145 and 156 determined by trial and error)
    show_ip(IP, x, y);

    x += 20*horizontal_direction; y += 50*vertical_direction;

    if (vertical_direction > 0 && y >= +20000) vertical_direction = -1;
    if (vertical_direction < 0 && y <= -19000) vertical_direction = +1;
    if (horizontal_direction > 0 && x >= +5000) horizontal_direction = -1;
    if (horizontal_direction < 0 && x <= -5000) horizontal_direction = +1;

  }
  return 0;
}
