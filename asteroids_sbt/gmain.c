//------------------------------------------------------------------------
//------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <vectrex/osWrapper.h>

#include "dvg.h"
#include "rom.h"

unsigned char stack[256];

unsigned long wcount;

unsigned short lastopadd;

void game ( unsigned short opadd );

typedef unsigned char BYTE;
typedef unsigned short WORD;

BYTE a_reg,x_reg,y_reg,flag_reg,s_reg;
WORD pc_reg = 0;

BYTE RTI;

#define A a_reg
#define X x_reg
#define Y y_reg
#define S s_reg
#define PC pc_reg

unsigned char C;
unsigned char ZN;
unsigned char DEC;
unsigned char V;


unsigned long clockticks;
unsigned long instcount=0;
unsigned long nmi_count=0;

unsigned short temp;
unsigned short temp2;


unsigned char ram[0x8000];

int sum,saveflags;

unsigned char value;

unsigned long kcount;

#define WriteMemory(a,b) (ram[a]=b)

/*
void WriteMemory ( unsigned short add, unsigned char data )
{
    ram[add]=data;
    fprintf(memtrace,"WriteMemory 0x%04X 0x%02X\n",add,data);
    wcount++;
    if(wcount>1000) exit(1);
}
*/

#define ReadMemory(x)  (ram[x])

//------------------------------------------------------------------------
void do_keys ( void )
{
    unsigned long keycode;

//static keymap_entry keymap [] =
//{
//    { XK_1, 0, 0x2403 },        // 1 Player start
//    { XK_2, 0, 0x2404 },        // 2 Player start
//
//    { XK_a, 0, 0x2407 },        // Rotate Left
//    { XK_s, 0, 0x2406 },        // Rotate Right
//    { XK_k, 0, 0x2405 },        // Thrust
//    { XK_l, 0, 0x2004 },        // Fire
//    { XK_space, 0, 0x2003 },    // Hyperspace
//};

//    printf("do_keys\n");
    keycode=readkeypad();
//    printf("0x%08lX\n",keycode);
    if((keycode&0x00001000)||(keycode&0x00008000))
        WriteMemory(0x2407,0x80); else WriteMemory(0x2407,0x00); //A left or Y
    if((keycode&0x00002000)||(keycode&0x00004000))
        WriteMemory(0x2406,0x80); else WriteMemory(0x2406,0x00); //B right or X
    if(keycode&0x00800000)
        WriteMemory(0x2405,0x80); else WriteMemory(0x2405,0x00); //volup thrust
    if(keycode&0x00400000)
        WriteMemory(0x2004,0x80); else WriteMemory(0x2004,0x00); //voldn fire
    if(keycode&0x00000100)
        WriteMemory(0x2403,0x80); else WriteMemory(0x2403,0x00); // start
}
//------------------------------------------------------------------------
void ADC ( unsigned char value )
{
      //saveflags=(P & 0x01);
      sum= ((char) A) + ((char) value) + ( C & 0x01);
      if ((sum>0x7f) || (sum<-0x80)) V=0x40; else V=0x00; //P |= 0x40; else P &= 0xbf;
      sum= A + value + ( C & 0x01);
      if (sum>0xff) C = 0x01; else C = 0x00; //P |= 0x01; else P &= 0xfe;
      A=sum;
//      if (P & 0x08)
      if (DEC & 0x08)
      {
              //P &= 0xfe;
              C = 0x00;
              if ((A & 0x0f)>0x09)
                      A += 0x06;
              if ((A & 0xf0)>0x90)
              {
                      A += 0x60;
                      //P |= 0x01;
                      C = 0x01;
              }
      }
      else
      {
              clockticks++;
      }
      ZN=A; //if (A) P &= 0xfd; else P |= 0x02;  if (A & 0x80) P |= 0x80; else P &= 0x7f;
}
//------------------------------------------------------------------------
void SBC ( unsigned char value )
{
//      value = gameImage[savepc] ^ 0xff;

      value = value ^ 0xFF;
      //saveflags=(P & 0x01);
      sum= ((char) A) + ((char) value) + (( C & 0x01) << 4);
      if ((sum>0x7f) || (sum<-0x80)) V=0x01; else V=0x00; //P |= 0x40; else P &= 0xbf;
      sum= A + value + ( C & 0x01);
      if (sum>0xff) C=0x01; else C=0x00; //P |= 0x01; else P &= 0xfe;
      A=sum;
      //if (P & 0x08)
      if(DEC & 0x08)
      {
              A -= 0x66;
              //P &= 0xfe;
              C = 0x00;
              if ((A & 0x0f)>0x09)
                      A += 0x06;
              if ((A & 0xf0)>0x90)
              {
                      A += 0x60;
                      //P |= 0x01;
                      C = 0x01;
              }
      }
      else
      {
              clockticks++;
      }
      ZN=A; //if (A) P &= 0xfd; else P |= 0x02;  if (A & 0x80) P |= 0x80; else P &= 0x7f;
}
//------------------------------------------------------------------------

void showme ( unsigned short opadd, unsigned char opcode );


//----  ----
#include "game.c"
//----  ----

//------------------------------------------------------------------------
void showme ( unsigned short opadd, unsigned char opcode )
{
    if(clockticks>250)
    {
        clockticks-=250;
        ram [0x2001] ^= 0xff;
        if (++nmi_count == 24)
        {
            nmi_count = 0;
            stack[S--]=(opadd>>8)&0x7F;
            stack[S--]=opadd&0xFF;
            if(ZN) ZN|=0x02;
            stack[S--]=(ZN&0x82)|(DEC&0x08)|(V&0x40)|(C&0x01);
            PCSTART=NMISTART;
            RTI=0;
            while(1)
            {
                game(PCSTART);
                if(RTI)
                {
                    break;
                }
            }
            do_keys();
        }
    }
}

//------------------------------------------------------------------------

int main( void )
{
    textinit();

/*
DSW1
        
2800    SWCOINAGE   Coinage 0 = Free Play, 1 = 1 Coin 2 Credits, 2 = 1 Coin 1 Credit, 3 = 2 Coins 1 Credit
2801    SWCNRMULT   Right Coin Multiplier 0 = 1x, 1 = 4x, 2 = 5x, 3 = 6x
2802    SWCNCMULT   Center Coin Multiplier & Starting Lives 1x & 4, 1 = 1x & 3, 2 = 2x & 4, 3 = 2x & 3
2803    SWLANGUAGE  Language 0 = English, 1 = German, 2 = French, 3 = Spanish 
* 
 */
WriteMemory(0x2802,2); // 4 ships
    
    memcpy(ram,rom,sizeof(rom));

    A=X=Y=0;
    C=0;
    ZN=0;
    V=0;
    S=0xFF;

    clockticks=0;

    kcount=0;

    while(1) game(PCSTART);
}
//------------------------------------------------------------------------
//------------------------------------------------------------------------
