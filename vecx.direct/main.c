#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h> // for usleep
#include <errno.h>
#ifndef FREESTANDING
#include <dirent.h> // this is actually not supported by the crosscompiler!
#endif

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include <vectrex/osWrapper.h>
#include <pitrex/bcm2835.h>
#include "e6809.h"
#include "vecx.h"

enum
{
	EMU_TIMER = 20, /* the emulators heart beats at 20 milliseconds */

	DEFAULT_WIDTH = 495,
	DEFAULT_HEIGHT = 615
};

static int32_t scl_factor;

static void quit(void);

static void render(void)
{
return;
}

static void resize(void)
{
	int sclx, scly;
	int screenx, screeny;
	int width, height;
return;
}

static int readevents(void)
{
	return 0;
}

#define GET_SYSTEM_TIMER bcm2835_st_read()


extern uint32_t cycleCount;
uint32_t cycleCount_mark;

static void emuloop(void)
{
	vecx_reset();
	cycleCount_mark = 0;
	uint32_t mark = (uint32_t)(GET_SYSTEM_TIMER&0xffffffff);
    setMarkStart();
	for (;;)
	{
		vecx_emu(1000);
		uint32_t mark2 = (uint32_t)(GET_SYSTEM_TIMER&0xffffffff);

		if ( mark2 >= mark +1000000) // 1 second
		{
		  mark = mark2;
		  uint32_t percent = ((cycleCount-cycleCount_mark)*100) / (1500000);
		  printf("Emulation percent: %i\n\r", percent);
		  cycleCount_mark = cycleCount;
		}
		v_resetDetection();
	}
}

static void load_overlay()
{

}

//#define SETTINGS_SIZE 1024
//unsigned char settingsBlob[SETTINGS_SIZE];
static int init(void)
{
	vectrexinit(1);
	v_init();
        //v_loadSettings("vecxDirect", settingsBlob, SETTINGS_SIZE);
	v_setName("vecxDirect"); // not sure if this does anything with the direct emulator!
        return 1;
}

static void quit(void)
{
  vectrexclose ();
	exit(0);
}
void initEmulator();

void main()
{

	if (!init()) quit();
	initEmulator();
	resize();
	emuloop();
	quit();
    while (1)
    {
    }
}

char name1[40];
char name2[40];
char name3[40];
char name4[40];

char *names[] =
{
  name1,
  name2,
  name3,
  name4,
};
int NAME_COUNT = 4;
void translateName(char *from, char *to);
int fillDirNames(int startWith);
void loadSelected(int selected);


int maxInDir = 0;
void initEmulator()
{
#ifdef FREESTANDING
    char *vectrexDir = "vectrex";
#else
    char *vectrexDir = "/opt/pitrex/roms/vectrex";
#endif
    if (chdir(vectrexDir)<0) // this isn't an ideal mechanism, it was expedient because bare metal library did not support paths
    {
	    printf("NO vectrex directory found...!\r\n");
	    return;
    }
    // see if there is a directory "vectrex"
    int startWith = 0;
    int nameResult = fillDirNames(startWith);
    if (nameResult<0) return;
    int selectedMenu = 0;
    int selectionMade=0;
    while(1)
    {
	v_WaitRecal();
	v_readButtons();
	v_readJoystick1Analog();
	v_setBrightness(64);        /* set intensity of vector beam... */
	if ((currentButtonState&0x04) == (0x04))
	{
      chdir("..");
	  loadSelected(selectedMenu);
	  return;
	}
	if ((currentJoy1Y>50) && (selectionMade==0))
	{
		selectedMenu--;
		if (selectedMenu<0) selectedMenu=0;
		selectionMade = 1;
	}
	if ((currentJoy1Y<-50) && (selectionMade==0))
	{
		selectedMenu++;
		if (selectedMenu>=NAME_COUNT) selectedMenu=NAME_COUNT-1;
		selectionMade = 1;
	}
	if ((currentJoy1X>50) && (selectionMade==0))
	{
	  if (nameResult == NAME_COUNT)
	    startWith+=4;
	    nameResult = fillDirNames(startWith);
	  selectionMade = 1;
	}
	if ((currentJoy1X<-50) && (selectionMade==0))
	{
	  if (startWith>=4)
	    startWith-=4;
	    nameResult = fillDirNames(startWith);
	  selectionMade = 1;
	}
	if ((ABS(currentJoy1Y)<20) && (ABS(currentJoy1X)<20)) selectionMade =0;

	int x = -40;
	int y = 80;
	char *selected = ">";
	for (int i=0;i<NAME_COUNT;i++)
	{
	    char translatesName[40];
	    translateName(names[i],translatesName);

	    v_printStringRaster(x, y, translatesName, 40, -5, 0);
	    if (i == selectedMenu)
	    {
	      v_printString(-60, y-7, selected, 5, 70);
	    }
	    y-=20;
	}
    }
}
static int filelength(FILE *fd)        // also used in inifile.c so not static
{
   int fsize;
   fseek (fd, 0, SEEK_END);
// my book says return 0 on success!
   fsize = ftell(fd); 
   (void) fseek (fd, 0, SEEK_SET);
   return fsize;
}

void loadSelected(int selected)
{
  char *selectedName = names[selected];
  uint8_t *loadMem = cart;


  // prepareName
  char loadName[50];
  int c=0;
//  loadName[c++]='/';
  loadName[c++]='v';
  loadName[c++]='e';
  loadName[c++]='c';
  loadName[c++]='t';
  loadName[c++]='r';
  loadName[c++]='e';
  loadName[c++]='x';
  loadName[c++]='/';
  for (int ii=0;c<50;ii++)
  {
    loadName[c++]=selectedName[ii];
    if (selectedName[ii]==0) break;
  }



  printf("Loading: %s \r\n", loadName);
  
  FILE *f;
  f = fopen(loadName, "r");
  if (f == 0)
  {
            printf("Could not open file %s (%i) \r\n", loadName, errno);
  }
  else
  {
    unsigned int fsize = filelength(f);
    int sizeRead = fread(loadMem, 1,fsize , f);
    if ( sizeRead== 0)
    {
        fclose(f);
        printf("File not loaded\r\n");
    }
    else
    {
        fclose(f);
        // file is loaded
        printf("Starting loaded file...\r\n");
    }
  }
}


// to upper case :-)
void translateName(char *from, char *to)
{
  int i=0;
  char c=1;
  while (c!=0)
  {
    c = *from++;
    if ((c>'a') && (c<'z'))
    {
      c = c-('a'-'A');
    }
    *to++=c;
  }

}

// return 1 on error
// otherwise the count of "names" filled
int fillDirNames(int startWith)
{
    char buf[256];
    if (getcwd (buf,256)==0)
    {
      printf("f_getcwd failed (%i) \r\n", errno);
      chdir("..");
      return -1;
    }
    printf("Current directory: %s \r\n", buf);
    DIR *dp;
    dp = opendir (buf);
    if (dp == 0)
    {
      printf("opendir failed (%i) \r\n", errno);
      chdir("..");
      return -2;
    }
    printf("Contents: \r\n");
    printf("--------- \r\n");
    dirent *finfo;
    int currentCounter=0;
    maxInDir = 0;
    int selected = 0;
    do
    {
      finfo =  readdir(dp);
      if (finfo != 0)
      {
      if (currentCounter>=startWith)
      {
				if (currentCounter<startWith+NAME_COUNT)
				{
				  int i = currentCounter-startWith;
				  char *nameToFill=names[i];
				  int c=0;
				  for (; c<39; c++)
				  {
				    nameToFill[c]=finfo->d_name[c];
				    if (nameToFill[c]==0) break;
				  }
				  nameToFill[c]=0;
				  selected++;
				}
      }
      printf("\t %s\r\n",finfo->d_name);
      maxInDir++;
      currentCounter++;
      }
    } while (finfo != 0);
    printf("\r\n");
    closedir (dp);
    return selected;
}
