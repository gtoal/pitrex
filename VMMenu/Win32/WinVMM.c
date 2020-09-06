// Windows specific functions for the Vector Mame Menu

#include "vmmstddef.h"
#include "WinVMM.h"
#include <winioctl.h>
#include <stdio.h>

#define IOCTL_KEYBOARD_SET_INDICATORS     CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define IOCTL_KEYBOARD_QUERY_INDICATORS   CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

int LEDstate=0;

/******************************************************************
	Write to keyboard LEDs value held in global variable LEDstate
 - only if state has changed
*******************************************************************/
void setLEDs(int leds)
{
   uint32_t input = 0;
   DWORD len;
   HANDLE kbd;

   if (LEDstate != leds)
   {
      //printf("LEDS: %i\n", leds);
      // KeyboardClass0 should be the first keyboard, on my laptop an external keyboard was KeyboardClass3
      DefineDosDevice(DDD_RAW_TARGET_PATH, "keyboard", "\\Device\\KeyboardClass0");
      kbd = CreateFile("\\\\.\\keyboard", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING,  FILE_ATTRIBUTE_NORMAL,  NULL);
      input |= (leds << 16);
      if (!DeviceIoControl(kbd, IOCTL_KEYBOARD_SET_INDICATORS, &input, sizeof(input), NULL, 0, &len, NULL))
      {
         printf("Error writing to LEDs!\n");
      } 
      DefineDosDevice(DDD_REMOVE_DEFINITION, "keyboard", NULL);
      CloseHandle(kbd);
      LEDstate=leds;
   }
}
