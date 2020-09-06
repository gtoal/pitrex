///////////////////////////////////////////////////////////

/* This file is loaded as a kernel image to 0x8000
   and reloads the main "loader" - and positions it at 
   0x1f000000 -> where it should not interfere with anything.
   
   If another program is loaded -> it will be loaded to 0x8000 and act as 
   normal kernel.
   
   The new "normal" kernel must do all setups again (mmu, stacks, handlers).
   
   But with this kind of indirection, any sort of kernel can be (re) started - even
   a normal raspbian!
*/

#include <stdio.h>

#include <baremetal/rpi-gpio.h>
#include <baremetal/rpi-aux.h>
#include <pitrex/pitrexio-gpio.h>
#include <ff.h> 

static FATFS fat_fs;		/* File system object */
#define MAX_LOAD (1024*1024*100) // 100 MB



void main()
{

  
  // correct start registers and jump to 8000
	FRESULT result = f_mount(&fat_fs, (const TCHAR *) "", (BYTE) 1);
	if (result != FR_OK) 
	{
		printf("NO filesystem...!\r\n");
		printf("f_mount failed! %d\r\n", (int) result);
	}
	else
	{
	  printf("FAT filesystem found!\r\n");
	}
	
	
	// 0x1f000000 does not work, since for some reason
	// mailboxes can not communicate when called from there ????
	// mailboxes are important for e.g. sd cart init
	uint32_t progSpace = LOADER_START;
	
	
	// todo copy from "r2" to 0x3e00000
	// for e.g. 100000 bytes
	// and set r2 to that value
	
	static const TCHAR FILE_NAME_PARAMS[] = "loader.pit";		///< Parameters file name
	FRESULT rc_rd = FR_DISK_ERR;
	FIL file_object_rd;
	rc_rd = f_open(&file_object_rd, FILE_NAME_PARAMS, (BYTE) FA_READ);

	if (rc_rd != FR_OK)
	{
		printf("Could not open file %s (%i) \r\n", FILE_NAME_PARAMS, rc_rd);
	}
	else
	{
		unsigned int fsize = MAX_LOAD;
		rc_rd = f_read(&file_object_rd, (void *)progSpace, fsize, &fsize);
		if ( rc_rd!= FR_OK)
		{
			printf("File not loaded (size got = %i)\r\n", rc_rd);
			f_close(&file_object_rd);
		}
		else
		{
			f_close(&file_object_rd);
			// file is loaded
			printf("Starting loader...\r\n");
			void (*progStart)(void) = (void (*)(void))progSpace;
			progStart();
		}
	}
        while(1)
        {
	}
}

