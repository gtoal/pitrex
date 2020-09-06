/* 	   BiosKey - Chad Gray 2011		*/
/* 	Display key code of key pressed		*/
/*						*/
/* Part of the VMMenu distriution		*/
/* Use this program to determine the keycodes	*/
/* when customising the keys for the menu	*/

#include <bios.h>
#include <stdio.h>
#include <stdlib.h>

#define  L_CTRL	0x04
#define  L_ALT  0x08

int main(void)
{
	int key, shift;
	int lastshift=0;

	printf("Press a key to show the scancode. q quits.\n");
	while (1)
	{
		shift = _bios_keybrd(0x02);

		/* if shift status changes */
		if ((shift != lastshift) && (shift != 0))
		{
		printf("Keycode = 0x%04x \n", shift);
		//printf("Key = %d\n", shift);
		}

		/* if a key is available */
		if (_bios_keybrd(1))
		{
			/* read the key */
			key = _bios_keybrd(0);
			printf("Keycode = 0x%04x \n", key);
			//printf("Key = %d\n", key);
			if ((key & 0xFF) == 'q')
			break;
		}
		lastshift = shift;
	}
	return 0;
}
