/* PiTrex Linux Menu 
   Based on bare-metal menu
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#define BINPATH "/opt/pitrex/"
#define SOUNDPATH "/opt/pitrex/"

void loadAndStart(char FILE_NAME[], char *parameter)
{
    int result;
    
    v_noSound();
    pid_t pID = fork();
    if (pID < 0)
      printf("Failed to fork\r\n");

    if ( pID == 0 )
    {
      result = execlp(FILE_NAME, FILE_NAME, parameter, NULL);
      if (result < 0)
      {
        printf("Program failed to load\r\n");
      }
    }
    else
    {
      wait(NULL);
      usleep(500000); //Should just disable 1+2+3+4 exit function for 1s instead.
    }
}

int selectedMenu;
int selectionMade;
int selectionStart;


// menu 
// todo read from disk...
void bootMenu(void)
{
  typedef struct MenuItem {
    char *DISPLAYNAME;
    char *img;
    char *param;
  } MenuItem;
  MenuItem menuItems[] = {
    {"REBOOT", "reboot", ""},
    {"ZBLAST", BINPATH "zblast", ""},
    {"HYPEROID", BINPATH "hyperoid", ""},
  };
  
  char *selected = ">";
  int max = (sizeof(menuItems)/sizeof(menuItems[0]))-1;
  
  int x = -50;
  int SPREAD = 15;
  int brightnesses[]={40,50,70,90,70,50,40};
  int brightnessCounter=0;
  for (int y=20; y<20+7*SPREAD;y+=SPREAD)
  {
    if (!(brightnessCounter+selectionStart<0) && (!(brightnessCounter+selectionStart>max)))
      v_printString(x,y, menuItems[brightnessCounter+selectionStart].DISPLAYNAME, 5, brightnesses[brightnessCounter]);
    if (brightnesses[brightnessCounter] == 90) selectedMenu = brightnessCounter+selectionStart;
    brightnessCounter++;
  }
  
  
  v_printString(-60, 20+3*SPREAD, selected, 5, 90);
 
  
  if ((currentJoy1Y>50) && (selectionMade==0))
  {
    selectionStart++;
    if (selectionStart>max-3)
    {
      selectionStart=max-3;
    }
    selectionMade = 1;
  }
  if ((currentJoy1Y<-50) && (selectionMade==0))
  {
    selectionStart--;
    if (selectionStart<0-3) selectionStart=0-3;
    selectionMade = 1;
  }
  if (ABS(currentJoy1Y)<20) selectionMade =0;

  if ((currentButtonState&0x0f) == (0x04)) // exactly button 3
  {
    int i;
//    loaderSettings.lastSelection = (unsigned char) selectedMenu; // remember last started game

    for (i = 0; i <= max; i++)
    {
      if (selectedMenu==i)
      {
       usleep(200000);
       loadAndStart(menuItems[i].img, menuItems[i].param);
      }
    }
  }
  
}


#include "ymStuff.i"
#include "playRaw.i"

/** Main function - we'll never return from here */
void main()
{
    printf("Loader starting...\r\n"); 

    selectionStart = 0;
    selectionMade = 0;

    vectrexinit(1); // pitrex
    v_init(); // vectrex interface

 
//    selectionStart

    loadAndPlayRAW();
    v_init(); // vectrex interface
    usePipeline = 1;
    
    int ymloaded = loadYM();

    int s = 0;
    int bDir = 1;
    int b = 30;
    char *ss[] = {"PITREX LINUX GAMES"};
    while(1)
    {
        v_WaitRecal();
        v_doSound();
        v_readButtons();
        v_readJoystick1Analog();
        b=b+bDir;
        if (b==70) bDir = -1;
        if (b==20) 
        {
          bDir = 1;
          s++;
          if (s==5) s=0;
        }
        
        v_printString(-50, -100, ss[s], 5, b);
        bootMenu();
	if (ymloaded) v_playYM();      
    }
}
