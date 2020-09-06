

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *fp;
FILE *show;

#define ROMBASE 0x4800

unsigned short addrmask = 0x7FFF;

unsigned char newline[1024];


//#include "rom.h"
unsigned char rom[0x8000];


#include "hitlist.h"
#include "pcstart.h"
#include "zflag.h"
//unsigned char oplen[256];
#include "oplen.h"
#include "znkeep.h"
#include "ticks.h"


unsigned short sa;

unsigned short temp,temp2;

void tempword ( unsigned short opadd )
{
    temp=rom[opadd+2];
    temp<<=8;
    temp|=rom[opadd+1];
    temp&=addrmask;
}


void checkme ( unsigned short opadd, unsigned short popadd )
{
    int i,j;
    unsigned char plen;

//    printf("    //checkme 0x%04X 0x%04X\n",opadd,popadd);


    plen=popadd-opadd;
    if(plen>3)
    {
        fprintf(stderr,"opadd %02X plen\n",rom[opadd]); abort();
    }
    if(oplen[rom[opadd]]==0)
    {
        fprintf(stderr,"NEW OPLEN %02X\n",rom[opadd]);
        oplen[rom[opadd]]=plen;
    }

    j=0;
    for(i=opadd+1;i<popadd;i++)
    {
        if(hitlist[i])
        {
            fprintf(stderr," HITLIST PROBLEM %04X\n",i);
            j++;
        }
    }
    if(hitlist[popadd])
    {
        return;
    }
    printf("    hitlist(0x%04X);\n",popadd);
    fprintf(stderr,"hitlist %04X\n",popadd);
    for(i=popadd;i<(ROMBASE+1);i++)
    {
        if(hitlist[i]) break;
        printf("    //0x%04X 0x%02X\n",i,rom[i]);
    }

    if(j==0) hitlist[popadd]=1;

}

void bcheckme ( unsigned short popadd )
{

    if(oplen[rom[popadd]]==0)
    {
        fprintf(stderr,"OPLEN %02X NOT DEFINED\n",rom[popadd]);
    }


//printf("    //bcheckme 0x%04X\n",popadd);


    if(pcstart[popadd]==0)
    {
        fprintf(stderr,"new PCSTART 0x%04X\n",popadd);
        pcstart[popadd]=1;
    }

    if(hitlist[popadd]) return;
    printf("    hitlist(0x%04X);\n",popadd);
    fprintf(stderr,"branch hitlist %04X\n",popadd);
    hitlist[popadd]=1;
}


void translate ( unsigned short opadd, unsigned char opcode )
{
    unsigned short nextopcode;
    unsigned char dozflag;

    nextopcode=oplen[opcode];
    if(nextopcode==0)
    {
        fprintf(stderr,"OPLEN NOT DEFINED %02X\n",opcode);
        abort();
    }
    nextopcode=rom[opadd+nextopcode];

    dozflag=0;
    if(zflag[opcode])
    {
        if(zflag[nextopcode])
        {
            if(!pcstart[opadd]) dozflag=1;
        }
    }



//**********************************
dozflag=1; if(znkeep[opadd]) dozflag=0;
//**********************************



if(pcstart[opadd]==0)
{
    printf("//"); //suppress the case statement
}
else
{
    printf("\n\n//L_%04X:\n",opadd);
}



    printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);

    if(pcstart[opadd]) printf("clockticks+=%u; //SUM\n",ticks[opadd]);







    switch(opcode)
    {



        //      ticks[0x00]=7; instruction[0x00]=brk6502; adrmode[0x00]=implied6502;
        case 0x00: //brk6502
            //      ticks[0x00]=7; instruction[0x00]=brk6502; adrmode[0x00]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=7;\n");
            //void implied6502()
            //{
            //}
            //void brk6502()
            //{
            //      PC++;
            //      put6502memory(0x0100+S--,(BYTE)(PC>>8));
            //      put6502memory(0x0100+S--,(BYTE)(PC & 0xff));
            //      put6502memory(0x0100+S--,P);
            //      P |= 0x14;
            //      PC = gameImage[0xfffe & addrmask] + (gameImage[0xffff & addrmask] << 8);
            //}
            printf("    crash(0x%04X,0x%02X);\n",opadd,opcode);

            printf("    showme(0x%04X,0x%02X);\n",opadd,opcode);
            temp2=opadd+1;
//            printf("    WriteMemory((unsigned short)(0x0100+S--),0x%02X);\n",(temp2>>8)&0x7F);
  //          printf("    WriteMemory((unsigned short)(0x0100+S--),0x%02X);\n",temp2&0xFF);
  //          printf("    WriteMemory((unsigned short)(0x0100+S--),P);\n");
            temp=rom[0x7FFF]; temp<<=8; temp|=rom[0x7FFE];
            printf("    P|=0x14;\n");
            printf("    PCSTART=0x%04X;\n",temp);
bcheckme(opadd);
bcheckme(temp);
            printf("    /**/return;\n");
            printf("    //goto L_%04X;\n",temp);
           // checkme(opadd,opadd+1);
            break;
        //      ticks[0x01]=6; instruction[0x01]=ora6502; adrmode[0x01]=indx6502;
        case 0x01: //ora6502
            //      ticks[0x01]=6; instruction[0x01]=ora6502; adrmode[0x01]=indx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indx6502()
            //{
            //      value = gameImage[PC++]+X;
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    crash(0x%04X,0x%02X);\n",opadd,opcode);
            break;
        //      ticks[0x02]=2; instruction[0x02]=nop6502; adrmode[0x02]=implied6502;
        case 0x02: //nop6502
            //      ticks[0x02]=2; instruction[0x02]=nop6502; adrmode[0x02]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x03]=2; instruction[0x03]=nop6502; adrmode[0x03]=implied6502;
        case 0x03: //nop6502
            //      ticks[0x03]=2; instruction[0x03]=nop6502; adrmode[0x03]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x04]=3; instruction[0x04]=tsb6502; adrmode[0x04]=zp6502;
        case 0x04: //tsb6502
            //      ticks[0x04]=3; instruction[0x04]=tsb6502; adrmode[0x04]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void tsb6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc] |= A;
            //      if (gameImage[savepc]) P &= 0xfd; else P |= 0x02;
            //}
            printf("    crash(0x%04X,0x%02X); //65C02 instruction\n",opadd,opcode);
            break;
        //      ticks[0x05]=3; instruction[0x05]=ora6502; adrmode[0x05]=zp6502;
        case 0x05: //ora6502
            //      ticks[0x05]=3; instruction[0x05]=ora6502; adrmode[0x05]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}

            printf("    A |= ReadMemory(0x%02X);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x06]=5; instruction[0x06]=asl6502; adrmode[0x06]=zp6502;
        case 0x06: //asl6502
            //      ticks[0x06]=5; instruction[0x06]=asl6502; adrmode[0x06]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void asl6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      P= (P & 0xfe) | ((value >>7) & 0x01);
            //      value = value << 1;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value = ReadMemory(0x%02X);\n",temp);
            //printf("    P= (P & 0xfe) | ((value >>7) & 0x01);\n");
            // save the carry bit and or it with value;
            printf("    C = value >> 7;\n");
            printf("    value = value << 1;\n");
            printf("    WriteMemory(0x%02X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");

            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x07]=2; instruction[0x07]=nop6502; adrmode[0x07]=implied6502;
        case 0x07: //nop6502
            //      ticks[0x07]=2; instruction[0x07]=nop6502; adrmode[0x07]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x08]=3; instruction[0x08]=php6502; adrmode[0x08]=implied6502;
        case 0x08: //php6502
            //      ticks[0x08]=3; instruction[0x08]=php6502; adrmode[0x08]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void implied6502()
            //{
            //}
            //void php6502()
            //{
            //      gameImage[0x100+S--] = P;
            //}
            //printf("    temp=0x100; temp+=S--;\n");
            //printf("    WriteMemory(temp,P);\n");
            printf("    if(ZN) ZN|=0x02;\n");
            printf("    stack[S--]=(ZN&0x82)|(DEC&0x08)|(V&0x40)|(C&0x01);\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x09]=3; instruction[0x09]=ora6502; adrmode[0x09]=immediate6502;
        case 0x09: //ora6502
            //      ticks[0x09]=3; instruction[0x09]=ora6502; adrmode[0x09]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A |= 0x%02X;\n",rom[opadd+1]);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x0a]=2; instruction[0x0a]=asla6502; adrmode[0x0a]=implied6502;
        case 0x0A: //asla6502
            //      ticks[0x0a]=2; instruction[0x0a]=asla6502; adrmode[0x0a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void asla6502()
            //{
            //      P= (P & 0xfe) | ((A >>7) & 0x01);
            //      A = A << 1;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    P= (P & 0xfe) | ((A >>7) & 0x01); \n");
            //carry bit
            printf("    C = (A >> 7);\n");
            printf("    A = A << 1;\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x0b]=2; instruction[0x0b]=nop6502; adrmode[0x0b]=implied6502;
        case 0x0B: //nop6502
            //      ticks[0x0b]=2; instruction[0x0b]=nop6502; adrmode[0x0b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x0c]=4; instruction[0x0c]=tsb6502; adrmode[0x0c]=abs6502;
        case 0x0C: //tsb6502
            //      ticks[0x0c]=4; instruction[0x0c]=tsb6502; adrmode[0x0c]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void tsb6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc] |= A;
            //      if (gameImage[savepc]) P &= 0xfd; else P |= 0x02;
            //}
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            break;
        //      ticks[0x0d]=4; instruction[0x0d]=ora6502; adrmode[0x0d]=abs6502;
        case 0x0D: //ora6502
            //      ticks[0x0d]=4; instruction[0x0d]=ora6502; adrmode[0x0d]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}

            if(temp>=ROMBASE) printf("    A |= 0x%02X;\n",rom[temp]);
            else              printf("    A |= ReadMemory(0x%04X);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x0e]=6; instruction[0x0e]=asl6502; adrmode[0x0e]=abs6502;
        case 0x0E: //asl6502
            //      ticks[0x0e]=6; instruction[0x0e]=asl6502; adrmode[0x0e]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void asl6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      P= (P & 0xfe) | ((value >>7) & 0x01);
            //      value = value << 1;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value = ReadMemory(0x%04X);\n",temp);
            //printf("    P= (P & 0xfe) | ((value >>7) & 0x01);\n");
            printf("    C = value >> 7;\n");
            printf("    value = value << 1;\n");
            printf("    WriteMemory(0x%04X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x0f]=2; instruction[0x0f]=nop6502; adrmode[0x0f]=implied6502;
        case 0x0F: //nop6502
            //      ticks[0x0f]=2; instruction[0x0f]=nop6502; adrmode[0x0f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x10]=2; instruction[0x10]=bpl6502; adrmode[0x10]=relative6502;
        case 0x10: //bpl6502
            //      ticks[0x10]=2; instruction[0x10]=bpl6502; adrmode[0x10]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void relative6502()
            //{
            //      savepc = gameImage[PC++];
            //      if (savepc & 0x80) savepc -= 0x100;
            //      if ((savepc>>8) != (PC>>8))
            //              clockticks6502++;
            //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void bpl6502()
            //{
            //      if ((P & 0x80)==0)
            //      {
            //              adrmode[opcode]();
            //              PC += savepc;
            //              clockticks6502++;
            //      }
            //      else
            //              value=gameImage[PC++];
            //

                temp2=(opadd+temp+2)&addrmask;

            //printf("    if ((P & 0x80)==0)\n");
            printf("    if((ZN&0x80)==0)\n");
            printf("    {\n");
            printf("        clockticks++;\n");
            printf("        showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("        PCSTART=0x%04X;\n",temp2);
bcheckme(opadd);
bcheckme(temp2);
            printf("        /**/return;\n");
            printf("        //goto L_%04X;\n",temp2);
            printf("    }\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x11]=5; instruction[0x11]=ora6502; adrmode[0x11]=indy6502;
        case 0x11: //ora6502
            //      ticks[0x11]=5; instruction[0x11]=ora6502; adrmode[0x11]=indy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void indy6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //      if (ticks[opcode]==5)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    temp=temp+Y;\n");
            printf("    A |= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x12]=3; instruction[0x12]=ora6502; adrmode[0x12]=indzp6502;
        case 0x12: //ora6502
            //      ticks[0x12]=3; instruction[0x12]=ora6502; adrmode[0x12]=indzp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void indzp6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value + 1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x13]=2; instruction[0x13]=nop6502; adrmode[0x13]=implied6502;
        case 0x13: //nop6502
            //      ticks[0x13]=2; instruction[0x13]=nop6502; adrmode[0x13]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x14]=3; instruction[0x14]=trb6502; adrmode[0x14]=zp6502;
        case 0x14: //trb6502
            //      ticks[0x14]=3; instruction[0x14]=trb6502; adrmode[0x14]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void trb6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc] = gameImage[savepc] & (A ^ 0xff);
            //      if (gameImage[savepc]) P &= 0xfd; else P |= 0x02;
            //}
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            break;
        //      ticks[0x15]=4; instruction[0x15]=ora6502; adrmode[0x15]=zpx6502;
        case 0x15: //ora6502
            //      ticks[0x15]=4; instruction[0x15]=ora6502; adrmode[0x15]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("   temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("   A |= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("   if (A) P &= 0xfd; else P |= 0x02; ");            printf("   if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            printf("   //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x16]=6; instruction[0x16]=asl6502; adrmode[0x16]=zpx6502;
        case 0x16: //asl6502
            //      ticks[0x16]=6; instruction[0x16]=asl6502; adrmode[0x16]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void asl6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      P= (P & 0xfe) | ((value >>7) & 0x01);
            //      value = value << 1;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("    value = ReadMemory(temp);\n");
            //printf("    P= (P & 0xfe) | ((value >>7) & 0x01);\n");
            printf("    C = value >> 7;\n");
            printf("    value = value << 1;\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x17]=2; instruction[0x17]=nop6502; adrmode[0x17]=implied6502;
        case 0x17: //nop6502
            //      ticks[0x17]=2; instruction[0x17]=nop6502; adrmode[0x17]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x18]=2; instruction[0x18]=clc6502; adrmode[0x18]=implied6502;
        case 0x18: //clc6502
            //      ticks[0x18]=2; instruction[0x18]=clc6502; adrmode[0x18]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void clc6502()
            //{
            //      P &= 0xfe;
            //}
            //printf("    P&= 0xfe;\n");
            printf("    C = 0;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x19]=4; instruction[0x19]=ora6502; adrmode[0x19]=absy6502;
        case 0x19: //ora6502
            //      ticks[0x19]=4; instruction[0x19]=ora6502; adrmode[0x19]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=Y;\n",temp);  //addrmask?
            printf("    A |= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x1a]=2; instruction[0x1a]=ina6502; adrmode[0x1a]=implied6502;
        case 0x1A: //ina6502
            //      ticks[0x1a]=2; instruction[0x1a]=ina6502; adrmode[0x1a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void ina6502()
            //{
            //      A++;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A++; \n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x1b]=2; instruction[0x1b]=nop6502; adrmode[0x1b]=implied6502;
        case 0x1B: //nop6502
            //      ticks[0x1b]=2; instruction[0x1b]=nop6502; adrmode[0x1b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x1c]=4; instruction[0x1c]=trb6502; adrmode[0x1c]=abs6502;
        case 0x1C: //trb6502
            //      ticks[0x1c]=4; instruction[0x1c]=trb6502; adrmode[0x1c]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void trb6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc] = gameImage[savepc] & (A ^ 0xff);
            //      if (gameImage[savepc]) P &= 0xfd; else P |= 0x02;
            //}
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            break;
        //      ticks[0x1d]=4; instruction[0x1d]=ora6502; adrmode[0x1d]=absx6502;
        case 0x1D: //ora6502
            //      ticks[0x1d]=4; instruction[0x1d]=ora6502; adrmode[0x1d]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void ora6502()
            //{
            //      adrmode[opcode]();
            //      A |= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    A |= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x1e]=7; instruction[0x1e]=asl6502; adrmode[0x1e]=absx6502;
        case 0x1E: //asl6502
            //      ticks[0x1e]=7; instruction[0x1e]=asl6502; adrmode[0x1e]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=7;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void asl6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      P= (P & 0xfe) | ((value >>7) & 0x01);
            //      value = value << 1;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp); //what about addrmask?
            printf("    value = ReadMemory(temp);\n");
            //printf("    P= (P & 0xfe) | ((value >>7) & 0x01);\n");
            printf("    C = value >> 7;\n");
            printf("    value = value << 1;\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x1f]=2; instruction[0x1f]=nop6502; adrmode[0x1f]=implied6502;
        case 0x1F: //nop6502
            //      ticks[0x1f]=2; instruction[0x1f]=nop6502; adrmode[0x1f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x20]=6; instruction[0x20]=jsr6502; adrmode[0x20]=abs6502;
        case 0x20: //jsr6502
            //      ticks[0x20]=6; instruction[0x20]=jsr6502; adrmode[0x20]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void jsr6502()
            //{
            //      PC++;
            //      put6502memory(0x0100+S--,(BYTE)(PC >> 8));
            //      put6502memory(0x0100+S--,(BYTE)(PC & 0xff));
            //      PC--;
            //      adrmode[opcode]();
            //      PC=savepc;
            //}
            temp2=(opadd+3)&addrmask;

            //printf("    temp=0x100; temp+=S--;\n");
            //printf("    WriteMemory(temp,0x%02X);\n",(temp2>>8)&0xFF); //addrmask above
            //printf("    temp=0x100; temp+=S--;\n");
            //printf("    WriteMemory(temp,0x%02X);\n",temp2&0xFF);
            printf("    stack[S--]=0x%02X;\n",(temp2>>8)&0xFF);
            printf("    stack[S--]=0x%02X;\n",temp2&0xFF);
bcheckme(temp2);
            temp2--;
            printf("    showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("    PCSTART=0x%04X; //jsr\n",temp);
bcheckme(opadd);
bcheckme(temp);
            printf("    /**/return;\n");
            printf("    //goto L_%04X;\n",temp);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x21]=6; instruction[0x21]=and6502; adrmode[0x21]=indx6502;
        case 0x21: //and6502
            //      ticks[0x21]=6; instruction[0x21]=and6502; adrmode[0x21]=indx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indx6502()
            //{
            //      value = gameImage[PC++]+X;
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=0x%02X; value+=X;\n",temp); //this should clip
            printf("    temp=ReadMemory(value+1);\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    A &= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x22]=2; instruction[0x22]=nop6502; adrmode[0x22]=implied6502;
        case 0x22: //nop6502
            //      ticks[0x22]=2; instruction[0x22]=nop6502; adrmode[0x22]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //65C02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x23]=2; instruction[0x23]=nop6502; adrmode[0x23]=implied6502;
        case 0x23: //nop6502
            //      ticks[0x23]=2; instruction[0x23]=nop6502; adrmode[0x23]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x24]=3; instruction[0x24]=bit6502; adrmode[0x24]=zp6502;
        case 0x24: //bit6502
            //      ticks[0x24]=3; instruction[0x24]=bit6502; adrmode[0x24]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void bit6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* non-destrucive logically And between value and the accumulator
            //       * and set zero flag */
            //      if (value & A) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative and overflow flags from value */
            //      P = (P & 0x3f) | (value & 0xc0);
            //}
            printf("    value=ReadMemory(0x%02X);\n",rom[opadd+1]);
            printf("    V = value;\n");
            if(dozflag) printf("//"); printf("  ZN = value & 0x80;\n");
            if(dozflag) printf("//"); printf("  if((value & A)==0) ZN |= 0x02;\n");
            //printf("    //showme(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x25]=3; instruction[0x25]=and6502; adrmode[0x25]=zp6502;
        case 0x25: //and6502
            //      ticks[0x25]=3; instruction[0x25]=and6502; adrmode[0x25]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A &= ReadMemory(0x%02X);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x26]=5; instruction[0x26]=rol6502; adrmode[0x26]=zp6502;
        case 0x26: //rol6502
            //      ticks[0x26]=5; instruction[0x26]=rol6502; adrmode[0x26]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void rol6502()
            //{
            //      saveflags=(P & 0x01);
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      P= (P & 0xfe) | ((value >>7) & 0x01);
            //      value = value << 1;
            //      value |= saveflags;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            printf("    value = ReadMemory(0x%02X);\n",temp);
            //printf("    P= (P & 0xfe) | ((value >>7) & 0x01);\n");
            printf("    C = value >> 7;\n");
            printf("    value = value << 1;\n");
            printf("    value |= (saveflags & 0x01);\n");
            printf("    WriteMemory(0x%02X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x27]=2; instruction[0x27]=nop6502; adrmode[0x27]=implied6502;
        case 0x27: //nop6502
            //      ticks[0x27]=2; instruction[0x27]=nop6502; adrmode[0x27]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x28]=4; instruction[0x28]=plp6502; adrmode[0x28]=implied6502;
        case 0x28: //plp6502
            //      ticks[0x28]=4; instruction[0x28]=plp6502; adrmode[0x28]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void implied6502()
            //{
            //}
            //void plp6502()
            //{
            //      P=gameImage[++S+0x100] | 0x20;
            //}
            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    P=ReadMemory(temp) | 0x20;\n");
            //printf("    P=stack[++S] | 0x20;\n");
            printf("    ZN=C=DEC=V=stack[++S]; // | 0x20;\n");
            printf("    ZN&=0x82;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
                        checkme(opadd,opadd+1);
            break;
        //      ticks[0x29]=3; instruction[0x29]=and6502; adrmode[0x29]=immediate6502;
        case 0x29: //and6502
            //      ticks[0x29]=3; instruction[0x29]=and6502; adrmode[0x29]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A &= 0x%02X;\n",rom[opadd+1]);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x2a]=2; instruction[0x2a]=rola6502; adrmode[0x2a]=implied6502;
        case 0x2A: //rola6502
            //      ticks[0x2a]=2; instruction[0x2a]=rola6502; adrmode[0x2a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void rola6502()
            //{
            //      saveflags=(P & 0x01);
            //      P= (P & 0xfe) | ((A >>7) & 0x01);
            //      A = A << 1;
            //      A |= saveflags;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            //printf("    P= (P & 0xfe) | ((A >>7) & 0x01);\n");
            printf("    C = A >> 7;\n");
            printf("    A = A << 1;\n");
            printf("    A |= (saveflags & 0x01);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x2b]=2; instruction[0x2b]=nop6502; adrmode[0x2b]=implied6502;
        case 0x2B: //nop6502
            //      ticks[0x2b]=2; instruction[0x2b]=nop6502; adrmode[0x2b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x2c]=4; instruction[0x2c]=bit6502; adrmode[0x2c]=abs6502;
        case 0x2C: //bit6502
            //      ticks[0x2c]=4; instruction[0x2c]=bit6502; adrmode[0x2c]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void bit6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* non-destrucive logically And between value and the accumulator
            //       * and set zero flag */
            //      if (value & A) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative and overflow flags from value */
            //      P = (P & 0x3f) | (value & 0xc0);
            //}
            if(temp<ROMBASE) printf("    value=ReadMemory(0x%04X);\n",temp);
            else             printf("    value=0x%02X;\n",rom[temp]);
            printf("    V = value;\n");
            if(dozflag) printf("//"); printf("  ZN = value & 0x80;\n");
            if(dozflag) printf("//"); printf("  if((value&A)==0) ZN |= 0x02;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x2d]=4; instruction[0x2d]=and6502; adrmode[0x2d]=abs6502;
        case 0x2D: //and6502
            //      ticks[0x2d]=4; instruction[0x2d]=and6502; adrmode[0x2d]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}

            if(temp<ROMBASE) printf("    A &= ReadMemory(0x%04X);\n",temp);
            else             printf("    A &= 0x%02X;\n",rom[temp]);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x2e]=6; instruction[0x2e]=rol6502; adrmode[0x2e]=abs6502;
        case 0x2E: //rol6502
            //      ticks[0x2e]=6; instruction[0x2e]=rol6502; adrmode[0x2e]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void rol6502()
            //{
            //      saveflags=(P & 0x01);
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      P= (P & 0xfe) | ((value >>7) & 0x01);
            //      value = value << 1;
            //      value |= saveflags;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            printf("    value = ReadMemory(0x%04X);\n",temp);
            //printf("    P= (P & 0xfe) | ((value >>7) & 0x01);\n");
            printf("    C = value >> 7;\n");
            printf("    value = value << 1;\n");
            printf("    value |= (saveflags & 0x01);\n");
            printf("    WriteMemory(0x%04X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x2f]=2; instruction[0x2f]=nop6502; adrmode[0x2f]=implied6502;
        case 0x2F: //nop6502
            //      ticks[0x2f]=2; instruction[0x2f]=nop6502; adrmode[0x2f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x30]=2; instruction[0x30]=bmi6502; adrmode[0x30]=relative6502;
        case 0x30: //bmi6502
            //      ticks[0x30]=2; instruction[0x30]=bmi6502; adrmode[0x30]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void relative6502()
            //{
            //      savepc = gameImage[PC++];
            //      if (savepc & 0x80) savepc -= 0x100;
            //      if ((savepc>>8) != (PC>>8))
            //              clockticks6502++;
            //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void bmi6502()
            //{
            //      if (P & 0x80)
            //      {
            //              adrmode[opcode]();
            //              PC += savepc;
            //              clockticks6502++;
            //      }
            //      else
            //              value=gameImage[PC++];
            //}
            temp2=(opadd+temp+2)&addrmask;
            //printf("    if (P & 0x80)\n");
            printf("    if(ZN&0x80)\n");
            printf("    {\n");
            printf("        clockticks++; \n");
            printf("        showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("        PCSTART=0x%04X;\n",temp2);
bcheckme(opadd);
bcheckme(temp2);
            printf("        /**/return;\n");
            printf("        //goto L_%04X;\n",temp2);
            printf("    }\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x31]=5; instruction[0x31]=and6502; adrmode[0x31]=indy6502;
        case 0x31: //and6502
            //      ticks[0x31]=5; instruction[0x31]=and6502; adrmode[0x31]=indy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void indy6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //      if (ticks[opcode]==5)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    temp=temp+Y;\n");
            printf("    A &= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x32]=3; instruction[0x32]=and6502; adrmode[0x32]=indzp6502;
        case 0x32: //and6502
            //      ticks[0x32]=3; instruction[0x32]=and6502; adrmode[0x32]=indzp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void indzp6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value + 1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}

            printf("    value=ReadMemory(0x%02X);\n",rom[opadd+1]);
            printf("    temp=ReadMemory((unsigned short)(value+1));\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");

            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x33]=2; instruction[0x33]=nop6502; adrmode[0x33]=implied6502;
        case 0x33: //nop6502
            //      ticks[0x33]=2; instruction[0x33]=nop6502; adrmode[0x33]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x34]=4; instruction[0x34]=bit6502; adrmode[0x34]=zpx6502;
        case 0x34: //bit6502
            //      ticks[0x34]=4; instruction[0x34]=bit6502; adrmode[0x34]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void bit6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* non-destrucive logically And between value and the accumulator
            //       * and set zero flag */
            //      if (value & A) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative and overflow flags from value */
            //      P = (P & 0x3f) | (value & 0xc0);
            //}
            printf("   temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("   value=ReadMemory(temp);\n");
            //if(dozflag) printf("//"); printf("  ZN=;\n"); // printf("//"); printf("   if (value & A) P &= 0xfd; else P |= 0x02; \n");
            printf("   if(value&A) ZN&=0xFD; else ZN|=0x02;\n");
            //printf("   P = (P & 0x3f) | (value & 0xc0);\n");
            printf("   ZN |= value & 0x80;\n");
            printf("   V = value;\n");
            printf("   crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x35]=4; instruction[0x35]=and6502; adrmode[0x35]=zpx6502;
        case 0x35: //and6502
            //      ticks[0x35]=4; instruction[0x35]=and6502; adrmode[0x35]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("    A &= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x36]=6; instruction[0x36]=rol6502; adrmode[0x36]=zpx6502;
        case 0x36: //rol6502
            //      ticks[0x36]=6; instruction[0x36]=rol6502; adrmode[0x36]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void rol6502()
            //{
            //      saveflags=(P & 0x01);
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      P= (P & 0xfe) | ((value >>7) & 0x01);
            //      value = value << 1;
            //      value |= saveflags;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            printf("    value = ReadMemory(temp);\n");
            //printf("    P= (P & 0xfe) | ((value >>7) & 0x01);\n");
            printf("    C = value >> 7;\n");
            printf("    value = value << 1;\n");
            printf("    value |= (saveflags & 0x01);\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x37]=2; instruction[0x37]=nop6502; adrmode[0x37]=implied6502;
        case 0x37: //nop6502
            //      ticks[0x37]=2; instruction[0x37]=nop6502; adrmode[0x37]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x38]=2; instruction[0x38]=sec6502; adrmode[0x38]=implied6502;
        case 0x38: //sec6502
            //      ticks[0x38]=2; instruction[0x38]=sec6502; adrmode[0x38]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void sec6502()
            //{
            //      P |= 0x01;
            //}
            //printf("    P |= 0x01;\n");
            printf("    C = 0x01;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x39]=4; instruction[0x39]=and6502; adrmode[0x39]=absy6502;
        case 0x39: //and6502
            //      ticks[0x39]=4; instruction[0x39]=and6502; adrmode[0x39]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=Y;\n",temp); //no addrmask!?
            printf("    A &= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x3a]=2; instruction[0x3a]=dea6502; adrmode[0x3a]=implied6502;
        case 0x3A: //dea6502
            //      ticks[0x3a]=2; instruction[0x3a]=dea6502; adrmode[0x3a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void dea6502()
            //{
            //      A--;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A--; \n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            printf("    crash(0x%04X,0x%02X); //65C02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x3b]=2; instruction[0x3b]=nop6502; adrmode[0x3b]=implied6502;
        case 0x3B: //nop6502
            //      ticks[0x3b]=2; instruction[0x3b]=nop6502; adrmode[0x3b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x3c]=4; instruction[0x3c]=bit6502; adrmode[0x3c]=absx6502;
        case 0x3C: //bit6502
            //      ticks[0x3c]=4; instruction[0x3c]=bit6502; adrmode[0x3c]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void bit6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* non-destrucive logically And between value and the accumulator
            //       * and set zero flag */
            //      if (value & A) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative and overflow flags from value */
            //      P = (P & 0x3f) | (value & 0xc0);
            //}
            printf("    crash(0x%04X,0x%02X); //65C02 instruction\n",opadd,opcode);
            break;
        //      ticks[0x3d]=4; instruction[0x3d]=and6502; adrmode[0x3d]=absx6502;
        case 0x3D: //and6502
            //      ticks[0x3d]=4; instruction[0x3d]=and6502; adrmode[0x3d]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void and6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      A &= value;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    A &= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x3e]=7; instruction[0x3e]=rol6502; adrmode[0x3e]=absx6502;
        case 0x3E: //rol6502
            //      ticks[0x3e]=7; instruction[0x3e]=rol6502; adrmode[0x3e]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=7;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void rol6502()
            //{
            //      saveflags=(P & 0x01);
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      P= (P & 0xfe) | ((value >>7) & 0x01);
            //      value = value << 1;
            //      value |= saveflags;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            printf("    temp=0x%04X; temp+=X;\n",temp);  //addrmask?
            printf("    value = ReadMemory(temp);\n");
            //printf("    P= (P & 0xfe) | ((value >>7) & 0x01);\n");
            printf("    C = value >> 7;\n");
            printf("    value = value << 1;\n");
            printf("    value |= (saveflags & 0x01);\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x3f]=2; instruction[0x3f]=nop6502; adrmode[0x3f]=implied6502;
        case 0x3F: //nop6502
            //      ticks[0x3f]=2; instruction[0x3f]=nop6502; adrmode[0x3f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x40]=6; instruction[0x40]=rti6502; adrmode[0x40]=implied6502;
        case 0x40: //rti6502
            //      ticks[0x40]=6; instruction[0x40]=rti6502; adrmode[0x40]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void implied6502()
            //{
            //}
            //void rti6502()
            //{
            //      P=gameImage[++S+0x100] | 0x20;
            //      PC=gameImage[++S+0x100];
            //      PC |= (gameImage[++S+0x100] << 8);
            //}
            printf("    { unsigned short savepcstart;\n");
            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    P=ReadMemory(temp) | 0x20;\n");
            //printf("    P=stack[++S]|0x20;\n");
            printf("    ZN=V=C=DEC=stack[++S];\n");
//            printf("    ZN&=0x82;\n");

            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    savepcstart=ReadMemory(temp);\n");
            printf("    savepcstart=stack[++S];\n");

            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    temp=ReadMemory(temp); temp<<=8;\n");
            printf("    temp=stack[++S]; temp<<=8;\n");

            printf("    savepcstart|=temp;\n");
            printf("    showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("    PCSTART=savepcstart;//gotta have PCSTART after //showme\n");
            printf("    }\n");
            printf("    RTI=1;\n");
            printf("    return;\n");
bcheckme(opadd);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x41]=6; instruction[0x41]=eor6502; adrmode[0x41]=indx6502;
        case 0x41: //eor6502
            //      ticks[0x41]=6; instruction[0x41]=eor6502; adrmode[0x41]=indx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indx6502()
            //{
            //      value = gameImage[PC++]+X;
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=0x%02X; value+=X;\n",temp); //want to clip?
            printf("    temp=ReadMemory(value+1);\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    A ^= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x42]=2; instruction[0x42]=nop6502; adrmode[0x42]=implied6502;
        case 0x42: //nop6502
            //      ticks[0x42]=2; instruction[0x42]=nop6502; adrmode[0x42]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x43]=2; instruction[0x43]=nop6502; adrmode[0x43]=implied6502;
        case 0x43: //nop6502
            //      ticks[0x43]=2; instruction[0x43]=nop6502; adrmode[0x43]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x44]=2; instruction[0x44]=nop6502; adrmode[0x44]=implied6502;
        case 0x44: //nop6502
            //      ticks[0x44]=2; instruction[0x44]=nop6502; adrmode[0x44]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x45]=3; instruction[0x45]=eor6502; adrmode[0x45]=zp6502;
        case 0x45: //eor6502
            //      ticks[0x45]=3; instruction[0x45]=eor6502; adrmode[0x45]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A ^= ReadMemory(0x%02X);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x46]=5; instruction[0x46]=lsr6502; adrmode[0x46]=zp6502;
        case 0x46: //lsr6502
            //      ticks[0x46]=5; instruction[0x46]=lsr6502; adrmode[0x46]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void lsr6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* set carry flag if shifting right causes a bit to be lost */
            //      P= (P & 0xfe) | (value & 0x01);
            //
            //      value = value >>1;
            //      put6502memory(savepc,value);
            //
            //      /* set zero flag if value is zero */
            //      if (value != 0) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative flag if bit 8 set??? can this happen on an LSR? */
            //      if ((value & 0x80) == 0x80)
            //         P |= 0x80;
            //      else
            //         P &= 0x7f;
            //}


//J.W=Op6502(pc.w++);
//I=Rd6502(j.w);
//P&=~C_FLAG;
//P|=Rg&C_FLAG;
//Rg>>=1;
//M_FL(Rg)
//Wr6502(j.w,I);

            printf("    value=ReadMemory(0x%02X);\n",temp);
            //printf("    P= (P & 0xfe) | (value & 0x01);\n");
            printf("    C = value;\n");
            printf("    value = value >> 1;\n");
            printf("    WriteMemory(0x%02X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x47]=2; instruction[0x47]=nop6502; adrmode[0x47]=implied6502;
        case 0x47: //nop6502
            //      ticks[0x47]=2; instruction[0x47]=nop6502; adrmode[0x47]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x48]=3; instruction[0x48]=pha6502; adrmode[0x48]=implied6502;
        case 0x48: //pha6502
            //      ticks[0x48]=3; instruction[0x48]=pha6502; adrmode[0x48]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void implied6502()
            //{
            //}
            //void pha6502()
            //{
            //      gameImage[0x100+S--] = A;
            //}
            //printf("    temp=0x100; temp+=S--;\n");
            //printf("    WriteMemory(temp,A);\n");
            printf("    stack[S--]=A;\n");

            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x49]=3; instruction[0x49]=eor6502; adrmode[0x49]=immediate6502;
        case 0x49: //eor6502
            //      ticks[0x49]=3; instruction[0x49]=eor6502; adrmode[0x49]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A ^= 0x%02X;\n",rom[opadd+1]);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x4a]=2; instruction[0x4a]=lsra6502; adrmode[0x4a]=implied6502;
        case 0x4A: //lsra6502
            //      ticks[0x4a]=2; instruction[0x4a]=lsra6502; adrmode[0x4a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void lsra6502()
            //{
            //      P= (P & 0xfe) | (A & 0x01);
            //      A = A >>1;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    P= (P & 0xfe) | (A & 0x01); \n");
            printf("    C = A;\n");
            printf("    A = A >> 1; \n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x4b]=2; instruction[0x4b]=nop6502; adrmode[0x4b]=implied6502;
        case 0x4B: //nop6502
            //      ticks[0x4b]=2; instruction[0x4b]=nop6502; adrmode[0x4b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x4c]=3; instruction[0x4c]=jmp6502; adrmode[0x4c]=abs6502;
        case 0x4C: //jmp6502
            //      ticks[0x4c]=3; instruction[0x4c]=jmp6502; adrmode[0x4c]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void jmp6502()
            //{
            //      adrmode[opcode]();
            //      PC=savepc;
            //}
            printf("    showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("    PCSTART=0x%04X;\n",temp);
bcheckme(opadd);
bcheckme(temp);
            printf("    /**/return;\n");
            printf("    //goto L_%04X;\n",temp);
            //checkme(opadd,opadd+3);
            break;
        //      ticks[0x4d]=4; instruction[0x4d]=eor6502; adrmode[0x4d]=abs6502;
        case 0x4D: //eor6502
            //      ticks[0x4d]=4; instruction[0x4d]=eor6502; adrmode[0x4d]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp<ROMBASE) printf("    A ^= ReadMemory(0x%04X);\n",temp);
            else             printf("    A ^= 0x%02X);\n",rom[temp]);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x4e]=6; instruction[0x4e]=lsr6502; adrmode[0x4e]=abs6502;
        case 0x4E: //lsr6502
            //      ticks[0x4e]=6; instruction[0x4e]=lsr6502; adrmode[0x4e]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void lsr6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* set carry flag if shifting right causes a bit to be lost */
            //      P= (P & 0xfe) | (value & 0x01);
            //
            //      value = value >>1;
            //      put6502memory(savepc,value);
            //
            //      /* set zero flag if value is zero */
            //      if (value != 0) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative flag if bit 8 set??? can this happen on an LSR? */
            //      if ((value & 0x80) == 0x80)
            //         P |= 0x80;
            //      else
            //         P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%04X);\n",temp);
           // printf("    P= (P & 0xfe) | (value & 0x01);\n");
            printf("    C = value;\n");
            printf("    value = value >> 1;\n");
            printf("    WriteMemory(0x%04X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x4f]=2; instruction[0x4f]=nop6502; adrmode[0x4f]=implied6502;
        case 0x4F: //nop6502
            //      ticks[0x4f]=2; instruction[0x4f]=nop6502; adrmode[0x4f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x50]=2; instruction[0x50]=bvc6502; adrmode[0x50]=relative6502;
        case 0x50: //bvc6502
            //      ticks[0x50]=2; instruction[0x50]=bvc6502; adrmode[0x50]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void relative6502()
            //{
            //      savepc = gameImage[PC++];
            //      if (savepc & 0x80) savepc -= 0x100;
            //      if ((savepc>>8) != (PC>>8))
            //              clockticks6502++;
            //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void bvc6502()
            //{
            //      if ((P & 0x40)==0)
            //      {
            //              adrmode[opcode]();
            //              PC += savepc;
            //              clockticks6502++;
            //      }
            //      else
            //              value=gameImage[PC++];
            //}
            temp2=(opadd+temp+2)&addrmask;
            //printf("    if ((P & 0x40)==0)\n");
            printf("    if((V & 0x40)==0)\n");
            printf("    {\n");
            printf("        clockticks++; \n");
            printf("        showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("        PCSTART=0x%04X;\n",temp2);
bcheckme(opadd);
bcheckme(temp2);
            printf("        /**/return;\n");
            printf("        //goto L_%04X;\n",temp2);
            printf("    }\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x51]=5; instruction[0x51]=eor6502; adrmode[0x51]=indy6502;
        case 0x51: //eor6502
            //      ticks[0x51]=5; instruction[0x51]=eor6502; adrmode[0x51]=indy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void indy6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //      if (ticks[opcode]==5)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    temp+=Y;\n");
            printf("    A ^= ReadMemory(temp); \n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x52]=3; instruction[0x52]=eor6502; adrmode[0x52]=indzp6502;
        case 0x52: //eor6502
            //      ticks[0x52]=3; instruction[0x52]=eor6502; adrmode[0x52]=indzp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void indzp6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value + 1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%02X);\n",rom[opadd+1]);
            printf("    temp=ReadMemory((unsigned short)(value+1));\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x53]=2; instruction[0x53]=nop6502; adrmode[0x53]=implied6502;
        case 0x53: //nop6502
            //      ticks[0x53]=2; instruction[0x53]=nop6502; adrmode[0x53]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x54]=2; instruction[0x54]=nop6502; adrmode[0x54]=implied6502;
        case 0x54: //nop6502
            //      ticks[0x54]=2; instruction[0x54]=nop6502; adrmode[0x54]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x55]=4; instruction[0x55]=eor6502; adrmode[0x55]=zpx6502;
        case 0x55: //eor6502
            //      ticks[0x55]=4; instruction[0x55]=eor6502; adrmode[0x55]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("    A ^= ReadMemory(temp); \n",temp);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x56]=6; instruction[0x56]=lsr6502; adrmode[0x56]=zpx6502;
        case 0x56: //lsr6502
            //      ticks[0x56]=6; instruction[0x56]=lsr6502; adrmode[0x56]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void lsr6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* set carry flag if shifting right causes a bit to be lost */
            //      P= (P & 0xfe) | (value & 0x01);
            //
            //      value = value >>1;
            //      put6502memory(savepc,value);
            //
            //      /* set zero flag if value is zero */
            //      if (value != 0) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative flag if bit 8 set??? can this happen on an LSR? */
            //      if ((value & 0x80) == 0x80)
            //         P |= 0x80;
            //      else
            //         P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("    value=ReadMemory(temp);\n");
            //printf("    P= (P & 0xfe) | (value & 0x01);\n");
            printf("    C = value;\n");
            printf("    value = value >> 1;\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x57]=2; instruction[0x57]=nop6502; adrmode[0x57]=implied6502;
        case 0x57: //nop6502
            //      ticks[0x57]=2; instruction[0x57]=nop6502; adrmode[0x57]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x58]=2; instruction[0x58]=cli6502; adrmode[0x58]=implied6502;
        case 0x58: //cli6502
            //      ticks[0x58]=2; instruction[0x58]=cli6502; adrmode[0x58]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void cli6502()
            //{
            //      P &= 0xfb;
            //}
            //printf("    P &= 0xfb;\n");
            printf("    INTERRUPTS?\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x59]=4; instruction[0x59]=eor6502; adrmode[0x59]=absy6502;
        case 0x59: //eor6502
            //      ticks[0x59]=4; instruction[0x59]=eor6502; adrmode[0x59]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=Y;\n",temp);
            printf("    A ^= ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x5a]=3; instruction[0x5a]=phy6502; adrmode[0x5a]=implied6502;
        case 0x5A: //phy6502
            //      ticks[0x5a]=3; instruction[0x5a]=phy6502; adrmode[0x5a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void implied6502()
            //{
            //}
            //void phy6502()
            //{
            //      put6502memory(0x100+S--,Y);
            //}
            //printf("    temp=0x100; temp+=S--;\n");
            //printf("    WriteMemory(temp,Y);\n");
            printf("    stack[S--]=Y;\n");
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x5b]=2; instruction[0x5b]=nop6502; adrmode[0x5b]=implied6502;
        case 0x5B: //nop6502
            //      ticks[0x5b]=2; instruction[0x5b]=nop6502; adrmode[0x5b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x5c]=2; instruction[0x5c]=nop6502; adrmode[0x5c]=implied6502;
        case 0x5C: //nop6502
            //      ticks[0x5c]=2; instruction[0x5c]=nop6502; adrmode[0x5c]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x5d]=4; instruction[0x5d]=eor6502; adrmode[0x5d]=absx6502;
        case 0x5D: //eor6502
            //      ticks[0x5d]=4; instruction[0x5d]=eor6502; adrmode[0x5d]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void eor6502()
            //{
            //      adrmode[opcode]();
            //      A ^= gameImage[savepc];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    A ^= ReadMemory(temp);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x5e]=7; instruction[0x5e]=lsr6502; adrmode[0x5e]=absx6502;
        case 0x5E: //lsr6502
            //      ticks[0x5e]=7; instruction[0x5e]=lsr6502; adrmode[0x5e]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=7;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void lsr6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* set carry flag if shifting right causes a bit to be lost */
            //      P= (P & 0xfe) | (value & 0x01);
            //
            //      value = value >>1;
            //      put6502memory(savepc,value);
            //
            //      /* set zero flag if value is zero */
            //      if (value != 0) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative flag if bit 8 set??? can this happen on an LSR? */
            //      if ((value & 0x80) == 0x80)
            //         P |= 0x80;
            //      else
            //         P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    value=ReadMemory(temp);\n",temp);
            //printf("    P= (P & 0xfe) | (value & 0x01);\n");
            printf("    C = value;\n");
            printf("    value = value >> 1;\n");
            printf("    WriteMemory(temp,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x5f]=2; instruction[0x5f]=nop6502; adrmode[0x5f]=implied6502;
        case 0x5F: //nop6502
            //      ticks[0x5f]=2; instruction[0x5f]=nop6502; adrmode[0x5f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x60]=6; instruction[0x60]=rts6502; adrmode[0x60]=implied6502;
        case 0x60: //rts6502
            //      ticks[0x60]=6; instruction[0x60]=rts6502; adrmode[0x60]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void implied6502()
            //{
            //}
            //void rts6502()
            //{
            //      PC=gameImage[++S+0x100];
            //      PC |= (gameImage[++S+0x100] << 8);
            //      PC++;
            //}
            printf("    { unsigned short SAVEPCSTART;\n");
            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    SAVEPCSTART=ReadMemory(temp);\n");
            printf("    SAVEPCSTART=stack[++S];\n");

            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    temp=ReadMemory(temp); temp<<=8;\n");
            printf("    temp=stack[++S]; temp<<=8;\n");

            printf("    SAVEPCSTART|=temp;\n");
            printf("    showme(0x%04X,0x%04X);\n",opadd,opcode);
            printf("    PCSTART=SAVEPCSTART;\n");
            printf("    }\n");
            printf("    return;\n");
bcheckme(opadd);
            //no checkme?
            break;
        //      ticks[0x61]=6; instruction[0x61]=adc6502; adrmode[0x61]=indx6502;
        case 0x61: //adc6502
            //      ticks[0x61]=6; instruction[0x61]=adc6502; adrmode[0x61]=indx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indx6502()
            //{
            //      value = gameImage[PC++]+X;
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=0x%02X; value+=X;\n",temp); //this should clip
            printf("    temp=ReadMemory(value+1);\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    ADC(ReadMemory(temp));\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x62]=2; instruction[0x62]=nop6502; adrmode[0x62]=implied6502;
        case 0x62: //nop6502
            //      ticks[0x62]=2; instruction[0x62]=nop6502; adrmode[0x62]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x63]=2; instruction[0x63]=nop6502; adrmode[0x63]=implied6502;
        case 0x63: //nop6502
            //      ticks[0x63]=2; instruction[0x63]=nop6502; adrmode[0x63]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x64]=3; instruction[0x64]=stz6502; adrmode[0x64]=zp6502;
        case 0x64: //stz6502
            //      ticks[0x64]=3; instruction[0x64]=stz6502; adrmode[0x64]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void stz6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,0);
            //}
            printf("    WriteMemory(0x%02X,0);\n",temp);
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x65]=3; instruction[0x65]=adc6502; adrmode[0x65]=zp6502;
        case 0x65: //adc6502
            //      ticks[0x65]=3; instruction[0x65]=adc6502; adrmode[0x65]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    ADC(ReadMemory(0x%02X));\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x66]=5; instruction[0x66]=ror6502; adrmode[0x66]=zp6502;
        case 0x66: //ror6502
            //      ticks[0x66]=5; instruction[0x66]=ror6502; adrmode[0x66]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void ror6502()
            //{
            //      saveflags=(P & 0x01);
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //      P= (P & 0xfe) | (value & 0x01);
            //      value = value >>1;
            //      if (saveflags) value |= 0x80;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            printf("    value=ReadMemory(0x%02X);\n",temp);
            //printf("    P= (P & 0xfe) | (value & 0x01);\n");
            printf("    C = value;\n");
            printf("    value = value >> 1;\n");
            printf("    if ( saveflags & 0x01 ) value |= 0x80;\n");
            printf("    WriteMemory(0x%02X,value);\n",temp);
            if(dozflag) printf("//"); printf("    ZN=value;\n");//       printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x67]=2; instruction[0x67]=nop6502; adrmode[0x67]=implied6502;
        case 0x67: //nop6502
            //      ticks[0x67]=2; instruction[0x67]=nop6502; adrmode[0x67]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x68]=4; instruction[0x68]=pla6502; adrmode[0x68]=implied6502;
        case 0x68: //pla6502
            //      ticks[0x68]=4; instruction[0x68]=pla6502; adrmode[0x68]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void implied6502()
            //{
            //}
            //void pla6502()
            //{
            //      A=gameImage[++S+0x100];
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    A=ReadMemory(temp); \n");
            printf("    A=stack[++S];\n");

            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x69]=3; instruction[0x69]=adc6502; adrmode[0x69]=immediate6502;
        case 0x69: //adc6502
            //      ticks[0x69]=3; instruction[0x69]=adc6502; adrmode[0x69]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    ADC(0x%02X);\n",rom[opadd+1]);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x6a]=2; instruction[0x6a]=rora6502; adrmode[0x6a]=implied6502;
        case 0x6A: //rora6502
            //      ticks[0x6a]=2; instruction[0x6a]=rora6502; adrmode[0x6a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void rora6502()
            //{
            //      saveflags=(P & 0x01);
            //      P= (P & 0xfe) | (A & 0x01);
            //      A = A >>1;
            //      if (saveflags) A |= 0x80;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}

            //printf("    saveflags=(P & 0x01); \n");
            printf("    saveflags = C; \n");
            //printf("    P= (P & 0xfe) | (A & 0x01);\n");
            printf("    C = A;\n");
            printf("    A = A >> 1;\n");
            printf("    if ( saveflags & 0x01 ) A |= 0x80;\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x6b]=2; instruction[0x6b]=nop6502; adrmode[0x6b]=implied6502;
        case 0x6B: //nop6502
            //      ticks[0x6b]=2; instruction[0x6b]=nop6502; adrmode[0x6b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x6c]=5; instruction[0x6c]=jmp6502; adrmode[0x6c]=indirect6502;
        case 0x6C: //jmp6502
            //      ticks[0x6c]=5; instruction[0x6c]=jmp6502; adrmode[0x6c]=indirect6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void indirect6502()
            //{
            //      help = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      savepc = gameImage[help] + (gameImage[help + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void jmp6502()
            //{
            //      adrmode[opcode]();
            //      PC=savepc;
            //}
            printf("    showme(0x%04X,0x%02X);\n",opadd,opcode);
            if(temp<ROMBASE)
            {
                    printf("    temp2=0x%04X;\n",temp);
                    printf("    temp=ReadMemory(temp2+1); temp<<=8;\n");
                    printf("    temp|=ReadMemory(temp2);\n");
                    printf("    PCSTART=temp;\n");
//cant bcheckme
            }
            else
            {
                tempword(temp-1);
                printf("    PCSTART=0x%04X;\n",temp);
bcheckme(temp);
            }
            printf("    return;\n");
            //no checkme
            break;
        //      ticks[0x6d]=4; instruction[0x6d]=adc6502; adrmode[0x6d]=abs6502;
        case 0x6D: //adc6502
            //      ticks[0x6d]=4; instruction[0x6d]=adc6502; adrmode[0x6d]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp<ROMBASE) printf("    ADC(ReadMemory(0x%04X));\n",temp);
            else             printf("    ADC(0x%02X);\n",rom[temp]);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x6e]=6; instruction[0x6e]=ror6502; adrmode[0x6e]=abs6502;
        case 0x6E: //ror6502
            //      ticks[0x6e]=6; instruction[0x6e]=ror6502; adrmode[0x6e]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void ror6502()
            //{
            //      saveflags=(P & 0x01);
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //      P= (P & 0xfe) | (value & 0x01);
            //      value = value >>1;
            //      if (saveflags) value |= 0x80;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            printf("    value=ReadMemory(0x%04X);\n",temp);
            //printf("    P= (P & 0xfe) | (value & 0x01);\n");
            printf("    C = value;\n");
            printf("    value = value >> 1;\n");
            printf("    if ( saveflags & 0x01 ) value |= 0x80;\n");
            printf("    WriteMemory(0x%04X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x6f]=2; instruction[0x6f]=nop6502; adrmode[0x6f]=implied6502;
        case 0x6F: //nop6502
            //      ticks[0x6f]=2; instruction[0x6f]=nop6502; adrmode[0x6f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x70]=2; instruction[0x70]=bvs6502; adrmode[0x70]=relative6502;
        case 0x70: //bvs6502
            //      ticks[0x70]=2; instruction[0x70]=bvs6502; adrmode[0x70]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void relative6502()
            //{
            //      savepc = gameImage[PC++];
            //      if (savepc & 0x80) savepc -= 0x100;
            //      if ((savepc>>8) != (PC>>8))
            //              clockticks6502++;
            //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void bvs6502()
            //{
            //      if (P & 0x40)
            //      {
            //              adrmode[opcode]();
            //              PC += savepc;
            //              clockticks6502++;
            //      }
            //      else
            //              value=gameImage[PC++];
            //}
                temp2=(opadd+temp+2)&addrmask;
            //printf("    if (P & 0x40)\n");
            printf("    if(V & 0x40)\n");
            printf("    {\n");
            printf("        clockticks++;\n");
            printf("        showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("        PCSTART=0x%04X;\n",temp2);
bcheckme(opadd);
bcheckme(temp2);
            printf("        /**/return;\n");
            printf("        //goto L_%04X;\n",temp2);
            printf("    }\n");
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x71]=5; instruction[0x71]=adc6502; adrmode[0x71]=indy6502;
        case 0x71: //adc6502
            //      ticks[0x71]=5; instruction[0x71]=adc6502; adrmode[0x71]=indy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void indy6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //      if (ticks[opcode]==5)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    temp=temp+Y;\n");
            printf("    ADC(ReadMemory(temp));\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x72]=3; instruction[0x72]=adc6502; adrmode[0x72]=indzp6502;
        case 0x72: //adc6502
            //      ticks[0x72]=3; instruction[0x72]=adc6502; adrmode[0x72]=indzp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void indzp6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value + 1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%02X);\n",rom[opadd+1]);
            printf("    temp=ReadMemory((unsigned short)(value+1));\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x73]=2; instruction[0x73]=nop6502; adrmode[0x73]=implied6502;
        case 0x73: //nop6502
            //      ticks[0x73]=2; instruction[0x73]=nop6502; adrmode[0x73]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x74]=4; instruction[0x74]=stz6502; adrmode[0x74]=zpx6502;
        case 0x74: //stz6502
            //      ticks[0x74]=4; instruction[0x74]=stz6502; adrmode[0x74]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void stz6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,0);
            //}
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            break;
        //      ticks[0x75]=4; instruction[0x75]=adc6502; adrmode[0x75]=zpx6502;
        case 0x75: //adc6502
            //      ticks[0x75]=4; instruction[0x75]=adc6502; adrmode[0x75]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("    ADC(ReadMemory(temp));\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x76]=6; instruction[0x76]=ror6502; adrmode[0x76]=zpx6502;
        case 0x76: //ror6502
            //      ticks[0x76]=6; instruction[0x76]=ror6502; adrmode[0x76]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void ror6502()
            //{
            //      saveflags=(P & 0x01);
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //      P= (P & 0xfe) | (value & 0x01);
            //      value = value >>1;
            //      if (saveflags) value |= 0x80;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            printf("    value=ReadMemory(temp);\n");
            //printf("    P= (P & 0xfe) | (value & 0x01);\n");
            printf("    C = value;\n");
            printf("    value = value >> 1;\n");
            printf("    if ( saveflags & 0x01 ) value |= 0x80;\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x77]=2; instruction[0x77]=nop6502; adrmode[0x77]=implied6502;
        case 0x77: //nop6502
            //      ticks[0x77]=2; instruction[0x77]=nop6502; adrmode[0x77]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x78]=2; instruction[0x78]=sei6502; adrmode[0x78]=implied6502;
        case 0x78: //sei6502
            //      ticks[0x78]=2; instruction[0x78]=sei6502; adrmode[0x78]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void sei6502()
            //{
            //      P |= 0x04;
            //}
            //printf("    P |= 0x04;\n");
            printf("    INTERRUPTS?\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x79]=4; instruction[0x79]=adc6502; adrmode[0x79]=absy6502;
        case 0x79: //adc6502
            //      ticks[0x79]=4; instruction[0x79]=adc6502; adrmode[0x79]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=Y;\n",temp);
            printf("    ADC(ReadMemory(temp));\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x7a]=4; instruction[0x7a]=ply6502; adrmode[0x7a]=implied6502;
        case 0x7A: //ply6502
            //      ticks[0x7a]=4; instruction[0x7a]=ply6502; adrmode[0x7a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void implied6502()
            //{
            //}
            //void ply6502()
            //{
            //      Y=gameImage[++S+0x100];
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    Y=ReadMemory(temp); \n");
            printf("    Y=stack[++S];\n");

            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f; \n");
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x7b]=2; instruction[0x7b]=nop6502; adrmode[0x7b]=implied6502;
        case 0x7B: //nop6502
            //      ticks[0x7b]=2; instruction[0x7b]=nop6502; adrmode[0x7b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x7c]=6; instruction[0x7c]=jmp6502; adrmode[0x7c]=indabsx6502;
        case 0x7C: //jmp6502
            //      ticks[0x7c]=6; instruction[0x7c]=jmp6502; adrmode[0x7c]=indabsx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indabsx6502()
            //{
            //      help = gameImage[PC] + (gameImage[PC + 1] << 8) + X;
            //      savepc = gameImage[help] + (gameImage[help + 1] << 8);
            //}
            tempword(opadd);
            // MORE STUFF
            //void jmp6502()
            //{
            //      adrmode[opcode]();
            //      PC=savepc;
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            break;
        //      ticks[0x7d]=4; instruction[0x7d]=adc6502; adrmode[0x7d]=absx6502;
        case 0x7D: //adc6502
            //      ticks[0x7d]=4; instruction[0x7d]=adc6502; adrmode[0x7d]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void adc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + saveflags;
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    ADC(ReadMemory(temp));\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x7e]=7; instruction[0x7e]=ror6502; adrmode[0x7e]=absx6502;
        case 0x7E: //ror6502
            //      ticks[0x7e]=7; instruction[0x7e]=ror6502; adrmode[0x7e]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=7;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void ror6502()
            //{
            //      saveflags=(P & 0x01);
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //      P= (P & 0xfe) | (value & 0x01);
            //      value = value >>1;
            //      if (saveflags) value |= 0x80;
            //      put6502memory(savepc,value);
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04x; temp+=X;\n",temp);
            //printf("    saveflags=(P & 0x01);\n");
            printf("    saveflags = C;\n");
            printf("    value=ReadMemory(temp);\n");
            //printf("    P= (P & 0xfe) | (value & 0x01);\n");
            printf("    C = value;\n");
            printf("    value = value >> 1;\n");
            printf("    if ( saveflags & 0x01 ) value |= 0x80;\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x7f]=2; instruction[0x7f]=nop6502; adrmode[0x7f]=implied6502;
        case 0x7F: //nop6502
            //      ticks[0x7f]=2; instruction[0x7f]=nop6502; adrmode[0x7f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    checkme(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x80]=2; instruction[0x80]=bra6502; adrmode[0x80]=relative6502;
        case 0x80: //bra6502
            //      ticks[0x80]=2; instruction[0x80]=bra6502; adrmode[0x80]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
                //void relative6502()
                //{
                //      savepc = gameImage[PC++];
                //      if (savepc & 0x80) savepc -= 0x100;
                //      if ((savepc>>8) != (PC>>8))
                //              clockticks6502++;
                //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void bra6502()
            //{
            //      adrmode[opcode]();
            //      PC += savepc;
            //      clockticks6502++;
            //}

            // I WASNT SURE ON THIS ONE, i HAD OPADD+TEMP+1 which looks wrong
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            printf("    PCSTART=0x%04X\n",(opadd+temp+2)&0x7FFF);
bcheckme(opadd);
bcheckme((opadd+temp+2)&0x7FFF);
            printf("    return;\n");
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x81]=6; instruction[0x81]=sta6502; adrmode[0x81]=indx6502;
        case 0x81: //sta6502
            //      ticks[0x81]=6; instruction[0x81]=sta6502; adrmode[0x81]=indx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indx6502()
            //{
            //      value = gameImage[PC++]+X;
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void sta6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,A);
            //}
            printf("    value=0x%02X; value+=X;\n",temp); //this should clip
            printf("    temp=ReadMemory(value+1);\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    WriteMemory(temp,A);\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x82]=2; instruction[0x82]=nop6502; adrmode[0x82]=implied6502;
        case 0x82: //nop6502
            //      ticks[0x82]=2; instruction[0x82]=nop6502; adrmode[0x82]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x83]=2; instruction[0x83]=nop6502; adrmode[0x83]=implied6502;
        case 0x83: //nop6502
            //      ticks[0x83]=2; instruction[0x83]=nop6502; adrmode[0x83]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x84]=2; instruction[0x84]=sty6502; adrmode[0x84]=zp6502;
        case 0x84: //sty6502
            //      ticks[0x84]=2; instruction[0x84]=sty6502; adrmode[0x84]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void sty6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,Y);
            //}
            printf("    WriteMemory(0x%02X,Y);\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x85]=2; instruction[0x85]=sta6502; adrmode[0x85]=zp6502;
        case 0x85: //sta6502
            //      ticks[0x85]=2; instruction[0x85]=sta6502; adrmode[0x85]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void sta6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,A);
            //}
            printf("    WriteMemory(0x%02X,A);\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x86]=2; instruction[0x86]=stx6502; adrmode[0x86]=zp6502;
        case 0x86: //stx6502
            //      ticks[0x86]=2; instruction[0x86]=stx6502; adrmode[0x86]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void stx6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,X);
            //}
            printf("    WriteMemory(0x%02X,X);\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x87]=2; instruction[0x87]=nop6502; adrmode[0x87]=implied6502;
        case 0x87: //nop6502
            //      ticks[0x87]=2; instruction[0x87]=nop6502; adrmode[0x87]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x88]=2; instruction[0x88]=dey6502; adrmode[0x88]=implied6502;
        case 0x88: //dey6502
            //      ticks[0x88]=2; instruction[0x88]=dey6502; adrmode[0x88]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void dey6502()
            //{
            //      Y--;
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    Y--;\n");
            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x89]=2; instruction[0x89]=bit6502; adrmode[0x89]=immediate6502;
        case 0x89: //bit6502
            //      ticks[0x89]=2; instruction[0x89]=bit6502; adrmode[0x89]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void bit6502()
            //{
            //      adrmode[opcode]();
            //      value=gameImage[savepc];
            //
            //      /* non-destrucive logically And between value and the accumulator
            //       * and set zero flag */
            //      if (value & A) P &= 0xfd; else P |= 0x02;
            //
            //      /* set negative and overflow flags from value */
            //      P = (P & 0x3f) | (value & 0xc0);
            //}
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            break;
        //      ticks[0x8a]=2; instruction[0x8a]=txa6502; adrmode[0x8a]=implied6502;
        case 0x8A: //txa6502
            //      ticks[0x8a]=2; instruction[0x8a]=txa6502; adrmode[0x8a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void txa6502()
            //{
            //      A=X;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A = X; \n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x8b]=2; instruction[0x8b]=nop6502; adrmode[0x8b]=implied6502;
        case 0x8B: //nop6502
            //      ticks[0x8b]=2; instruction[0x8b]=nop6502; adrmode[0x8b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x8c]=4; instruction[0x8c]=sty6502; adrmode[0x8c]=abs6502;
        case 0x8C: //sty6502
            //      ticks[0x8c]=4; instruction[0x8c]=sty6502; adrmode[0x8c]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void sty6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,Y);
            //}
            printf("    WriteMemory(0x%04X,Y);\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x8d]=4; instruction[0x8d]=sta6502; adrmode[0x8d]=abs6502;
        case 0x8D: //sta6502
            //      ticks[0x8d]=4; instruction[0x8d]=sta6502; adrmode[0x8d]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void sta6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,A);
            //}

            if(temp==0x3000) //this is Asteroids specific
            {
                printf("   dvg_draw_screen();\n");
            }
            else
            {
                printf("    WriteMemory(0x%04X,A);\n",temp);
            }
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x8e]=4; instruction[0x8e]=stx6502; adrmode[0x8e]=abs6502;
        case 0x8E: //stx6502
            //      ticks[0x8e]=4; instruction[0x8e]=stx6502; adrmode[0x8e]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void stx6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,X);
            //}
            printf("    WriteMemory(0x%04X,X);\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x8f]=2; instruction[0x8f]=nop6502; adrmode[0x8f]=implied6502;
        case 0x8F: //nop6502
            //      ticks[0x8f]=2; instruction[0x8f]=nop6502; adrmode[0x8f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x90]=2; instruction[0x90]=bcc6502; adrmode[0x90]=relative6502;
        case 0x90: //bcc6502
            //      ticks[0x90]=2; instruction[0x90]=bcc6502; adrmode[0x90]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void relative6502()
            //{
            //      savepc = gameImage[PC++];
            //      if (savepc & 0x80) savepc -= 0x100;
            //      if ((savepc>>8) != (PC>>8))
            //              clockticks6502++;
            //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void bcc6502()
            //{
            //      if ((P & 0x01)==0)
            //      {
            //              adrmode[opcode]();
            //              PC += savepc;
            //              clockticks6502++;
            //      }
            //      else
            //              value = gameImage[PC++];
            //}
            temp2=(opadd+temp+2)&addrmask;
            //printf("    if ((P & 0x01)==0) \n");
            printf("    if ((C & 0x01)==0) \n");
            printf("    { \n");
            printf("        clockticks++;  \n");
            printf("        showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("        PCSTART=0x%04X;\n",temp2);
bcheckme(opadd);
bcheckme(temp2);
            printf("        /**/return;\n");
            printf("        //goto L_%04X;\n",temp2);
            printf("    } \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x91]=6; instruction[0x91]=sta6502; adrmode[0x91]=indy6502;
        case 0x91: //sta6502
            //      ticks[0x91]=6; instruction[0x91]=sta6502; adrmode[0x91]=indy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indy6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //      if (ticks[opcode]==5)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void sta6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,A);
            //}
            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    temp+=Y;\n");
            printf("    WriteMemory(temp,A);\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x92]=3; instruction[0x92]=sta6502; adrmode[0x92]=indzp6502;
        case 0x92: //sta6502
            //      ticks[0x92]=3; instruction[0x92]=sta6502; adrmode[0x92]=indzp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void indzp6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value + 1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void sta6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,A);
            //}

            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    WriteMemory(temp,A);\n");
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x93]=2; instruction[0x93]=nop6502; adrmode[0x93]=implied6502;
        case 0x93: //nop6502
            //      ticks[0x93]=2; instruction[0x93]=nop6502; adrmode[0x93]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x94]=4; instruction[0x94]=sty6502; adrmode[0x94]=zpx6502;
        case 0x94: //sty6502
            //      ticks[0x94]=4; instruction[0x94]=sty6502; adrmode[0x94]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void sty6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,Y);
            //}
            if(temp==0)
            {
                printf("    WriteMemory((unsigned short)X,Y);\n");
            }
            else
            {
                printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
                printf("    WriteMemory(temp,Y);\n");
            }
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x95]=4; instruction[0x95]=sta6502; adrmode[0x95]=zpx6502;
        case 0x95: //sta6502
            //      ticks[0x95]=4; instruction[0x95]=sta6502; adrmode[0x95]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void sta6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,A);
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("    WriteMemory(temp,A);\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x96]=4; instruction[0x96]=stx6502; adrmode[0x96]=zpy6502;
        case 0x96: //stx6502
            //      ticks[0x96]=4; instruction[0x96]=stx6502; adrmode[0x96]=zpy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpy6502()
            //{
            //      savepc=gameImage[PC++]+Y;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=Y)&0xFF
            //void stx6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,X);
            //}
            printf("    temp=0x%04X; temp+=Y; temp&=0xFF;\n",temp);
            printf("    WriteMemory(temp,X);\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x97]=2; instruction[0x97]=nop6502; adrmode[0x97]=implied6502;
        case 0x97: //nop6502
            //      ticks[0x97]=2; instruction[0x97]=nop6502; adrmode[0x97]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x98]=2; instruction[0x98]=tya6502; adrmode[0x98]=implied6502;
        case 0x98: //tya6502
            //      ticks[0x98]=2; instruction[0x98]=tya6502; adrmode[0x98]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void tya6502()
            //{
            //      A=Y;
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A = Y;\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x99]=5; instruction[0x99]=sta6502; adrmode[0x99]=absy6502;
        case 0x99: //sta6502
            //      ticks[0x99]=5; instruction[0x99]=sta6502; adrmode[0x99]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void sta6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,A);
            //}

            printf("    temp=0x%04X; temp+=Y;\n",temp);
            printf("    WriteMemory(temp,A);\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x9a]=2; instruction[0x9a]=txs6502; adrmode[0x9a]=implied6502;
        case 0x9A: //txs6502
            //      ticks[0x9a]=2; instruction[0x9a]=txs6502; adrmode[0x9a]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void txs6502()
            //{
            //      S=X;
            //}
            printf("    S = X;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0x9b]=2; instruction[0x9b]=nop6502; adrmode[0x9b]=implied6502;
        case 0x9B: //nop6502
            //      ticks[0x9b]=2; instruction[0x9b]=nop6502; adrmode[0x9b]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0x9c]=4; instruction[0x9c]=stz6502; adrmode[0x9c]=abs6502;
        case 0x9C: //stz6502
            //      ticks[0x9c]=4; instruction[0x9c]=stz6502; adrmode[0x9c]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void stz6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,0);
            //}
            printf("    WriteMemory(0x%04X,0);\n",temp);
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x9d]=5; instruction[0x9d]=sta6502; adrmode[0x9d]=absx6502;
        case 0x9D: //sta6502
            //      ticks[0x9d]=5; instruction[0x9d]=sta6502; adrmode[0x9d]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void sta6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,A);
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    WriteMemory(temp,A);\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x9e]=5; instruction[0x9e]=stz6502; adrmode[0x9e]=absx6502;
        case 0x9E: //stz6502
            //      ticks[0x9e]=5; instruction[0x9e]=stz6502; adrmode[0x9e]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void stz6502()
            //{
            //      adrmode[opcode]();
            //      put6502memory(savepc,0);
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    WriteMemory(temp,0);\n");
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0x9f]=2; instruction[0x9f]=nop6502; adrmode[0x9f]=implied6502;
        case 0x9F: //nop6502
            //      ticks[0x9f]=2; instruction[0x9f]=nop6502; adrmode[0x9f]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xa0]=3; instruction[0xa0]=ldy6502; adrmode[0xa0]=immediate6502;
        case 0xA0: //ldy6502
            //      ticks[0xa0]=3; instruction[0xa0]=ldy6502; adrmode[0xa0]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void ldy6502()
            //{
            //      adrmode[opcode]();
            //      Y=gameImage[savepc];
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    Y = 0x%02X; \n",rom[opadd+1]);
            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xa1]=6; instruction[0xa1]=lda6502; adrmode[0xa1]=indx6502;
        case 0xA1: //lda6502
            //      ticks[0xa1]=6; instruction[0xa1]=lda6502; adrmode[0xa1]=indx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indx6502()
            //{
            //      value = gameImage[PC++]+X;
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=0x%02X; value+=X;\n",temp); //this should clip
            printf("    temp=ReadMemory(value+1);\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    A=ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xa2]=3; instruction[0xa2]=ldx6502; adrmode[0xa2]=immediate6502;
        case 0xA2: //ldx6502
            //      ticks[0xa2]=3; instruction[0xa2]=ldx6502; adrmode[0xa2]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void ldx6502()
            //{
            //      adrmode[opcode]();
            //      X=gameImage[savepc];
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    X=0x%02X;\n",rom[opadd+1]);
            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xa3]=2; instruction[0xa3]=nop6502; adrmode[0xa3]=implied6502;
        case 0xA3: //nop6502
            //      ticks[0xa3]=2; instruction[0xa3]=nop6502; adrmode[0xa3]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xa4]=3; instruction[0xa4]=ldy6502; adrmode[0xa4]=zp6502;
        case 0xA4: //ldy6502
            //      ticks[0xa4]=3; instruction[0xa4]=ldy6502; adrmode[0xa4]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void ldy6502()
            //{
            //      adrmode[opcode]();
            //      Y=gameImage[savepc];
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    Y=ReadMemory(0x%02X);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xa5]=3; instruction[0xa5]=lda6502; adrmode[0xa5]=zp6502;
        case 0xA5: //lda6502
            //      ticks[0xa5]=3; instruction[0xa5]=lda6502; adrmode[0xa5]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A=ReadMemory(0x%02X); \n",temp);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xa6]=3; instruction[0xa6]=ldx6502; adrmode[0xa6]=zp6502;
        case 0xA6: //ldx6502
            //      ticks[0xa6]=3; instruction[0xa6]=ldx6502; adrmode[0xa6]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void ldx6502()
            //{
            //      adrmode[opcode]();
            //      X=gameImage[savepc];
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    X=ReadMemory(0x%02X); \n",temp);
            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xa7]=2; instruction[0xa7]=nop6502; adrmode[0xa7]=implied6502;
        case 0xA7: //nop6502
            //      ticks[0xa7]=2; instruction[0xa7]=nop6502; adrmode[0xa7]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xa8]=2; instruction[0xa8]=tay6502; adrmode[0xa8]=implied6502;
        case 0xA8: //tay6502
            //      ticks[0xa8]=2; instruction[0xa8]=tay6502; adrmode[0xa8]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void tay6502()
            //{
            //      Y=A;
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    Y=A;\n");
            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xa9]=3; instruction[0xa9]=lda6502; adrmode[0xa9]=immediate6502;
        case 0xA9: //lda6502
            //      ticks[0xa9]=3; instruction[0xa9]=lda6502; adrmode[0xa9]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    A=0x%02X;\n",rom[opadd+1]);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xaa]=2; instruction[0xaa]=tax6502; adrmode[0xaa]=implied6502;
        case 0xAA: //tax6502
            //      ticks[0xaa]=2; instruction[0xaa]=tax6502; adrmode[0xaa]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void tax6502()
            //{
            //      X=A;
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    X=A;\n");
            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xab]=2; instruction[0xab]=nop6502; adrmode[0xab]=implied6502;
        case 0xAB: //nop6502
            //      ticks[0xab]=2; instruction[0xab]=nop6502; adrmode[0xab]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xac]=4; instruction[0xac]=ldy6502; adrmode[0xac]=abs6502;
        case 0xAC: //ldy6502
            //      ticks[0xac]=4; instruction[0xac]=ldy6502; adrmode[0xac]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void ldy6502()
            //{
            //      adrmode[opcode]();
            //      Y=gameImage[savepc];
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp<ROMBASE) printf("    Y=ReadMemory(0x%04X);\n",temp);
            else             printf("    Y=0x%02X;\n",rom[temp]);
            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xad]=4; instruction[0xad]=lda6502; adrmode[0xad]=abs6502;
        case 0xAD: //lda6502
            //      ticks[0xad]=4; instruction[0xad]=lda6502; adrmode[0xad]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp>=ROMBASE) printf("    A=0x%02X; \n",rom[temp]);
            else             printf("    A=ReadMemory(0x%04X); \n",temp);
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//");  printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xae]=4; instruction[0xae]=ldx6502; adrmode[0xae]=abs6502;
        case 0xAE: //ldx6502
            //      ticks[0xae]=4; instruction[0xae]=ldx6502; adrmode[0xae]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void ldx6502()
            //{
            //      adrmode[opcode]();
            //      X=gameImage[savepc];
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp>=ROMBASE) printf("    X=0x%02X;\n",rom[temp]);
            else              printf("    X=ReadMemory(0x%04X);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xaf]=2; instruction[0xaf]=nop6502; adrmode[0xaf]=implied6502;
        case 0xAF: //nop6502
            //      ticks[0xaf]=2; instruction[0xaf]=nop6502; adrmode[0xaf]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xb0]=2; instruction[0xb0]=bcs6502; adrmode[0xb0]=relative6502;
        case 0xB0: //bcs6502
            //      ticks[0xb0]=2; instruction[0xb0]=bcs6502; adrmode[0xb0]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void relative6502()
            //{
            //      savepc = gameImage[PC++];
            //      if (savepc & 0x80) savepc -= 0x100;
            //      if ((savepc>>8) != (PC>>8))
            //              clockticks6502++;
            //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void bcs6502()
            //{
            //      if (P & 0x01)
            //      {
            //              adrmode[opcode]();
            //              PC += savepc;
            //              clockticks6502++;
            //      }
            //      else
            //              value=gameImage[PC++];
            //}
            temp2=(opadd+temp+2)&addrmask;
            //printf("    if (P & 0x01) \n");
            printf("    if(C & 0x01)\n");
            printf("    { \n");
            printf("        clockticks++;  \n");
            printf("        showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("        PCSTART=0x%04X;\n",temp2);
bcheckme(opadd);
bcheckme(temp2);
            printf("        /**/return;\n");
            printf("        //goto L_%04X;\n",temp2);
            printf("    } \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xb1]=5; instruction[0xb1]=lda6502; adrmode[0xb1]=indy6502;
        case 0xB1: //lda6502
            //      ticks[0xb1]=5; instruction[0xb1]=lda6502; adrmode[0xb1]=indy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void indy6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //      if (ticks[opcode]==5)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}

            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    temp=temp+Y;\n");
            printf("    A=ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);

            break;
        //      ticks[0xb2]=3; instruction[0xb2]=lda6502; adrmode[0xb2]=indzp6502;
        case 0xB2: //lda6502
            //      ticks[0xb2]=3; instruction[0xb2]=lda6502; adrmode[0xb2]=indzp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void indzp6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value + 1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);

            printf("    A=ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xb3]=2; instruction[0xb3]=nop6502; adrmode[0xb3]=implied6502;
        case 0xB3: //nop6502
            //      ticks[0xb3]=2; instruction[0xb3]=nop6502; adrmode[0xb3]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xb4]=4; instruction[0xb4]=ldy6502; adrmode[0xb4]=zpx6502;
        case 0xB4: //ldy6502
            //      ticks[0xb4]=4; instruction[0xb4]=ldy6502; adrmode[0xb4]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void ldy6502()
            //{
            //      adrmode[opcode]();
            //      Y=gameImage[savepc];
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=X; temp&=0xFF;\n",temp);
            printf("    Y=ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xb5]=4; instruction[0xb5]=lda6502; adrmode[0xb5]=zpx6502;
        case 0xB5: //lda6502
            //      ticks[0xb5]=4; instruction[0xb5]=lda6502; adrmode[0xb5]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X; temp&=0xFF;\n",temp);
            printf("    A = ReadMemory(temp); \n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f; \n");

            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xb6]=4; instruction[0xb6]=ldx6502; adrmode[0xb6]=zpy6502;
        case 0xB6: //ldx6502
            //      ticks[0xb6]=4; instruction[0xb6]=ldx6502; adrmode[0xb6]=zpy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpy6502()
            //{
            //      savepc=gameImage[PC++]+Y;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=Y)&0xFF
            //void ldx6502()
            //{
            //      adrmode[opcode]();
            //      X=gameImage[savepc];
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%02X; temp+=Y; temp&=0xFF;\n",temp);
            printf("    X=ReadMemory(temp&0xFF);\n");
            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xb7]=2; instruction[0xb7]=nop6502; adrmode[0xb7]=implied6502;
        case 0xB7: //nop6502
            //      ticks[0xb7]=2; instruction[0xb7]=nop6502; adrmode[0xb7]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xb8]=2; instruction[0xb8]=clv6502; adrmode[0xb8]=implied6502;
        case 0xB8: //clv6502
            //      ticks[0xb8]=2; instruction[0xb8]=clv6502; adrmode[0xb8]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void clv6502()
            //{
            //      P &= 0xbf;
            //}
            //printf("    P &= 0xbf;\n");
            printf("    V = 0;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xb9]=4; instruction[0xb9]=lda6502; adrmode[0xb9]=absy6502;
        case 0xB9: //lda6502
            //      ticks[0xb9]=4; instruction[0xb9]=lda6502; adrmode[0xb9]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=Y;\n",temp);
            printf("    A=ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//"); printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xba]=2; instruction[0xba]=tsx6502; adrmode[0xba]=implied6502;
        case 0xBA: //tsx6502
            //      ticks[0xba]=2; instruction[0xba]=tsx6502; adrmode[0xba]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void tsx6502()
            //{
            //      X=S;
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    X=S;\n");
            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xbb]=2; instruction[0xbb]=nop6502; adrmode[0xbb]=implied6502;
        case 0xBB: //nop6502
            //      ticks[0xbb]=2; instruction[0xbb]=nop6502; adrmode[0xbb]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xbc]=4; instruction[0xbc]=ldy6502; adrmode[0xbc]=absx6502;
        case 0xBC: //ldy6502
            //      ticks[0xbc]=4; instruction[0xbc]=ldy6502; adrmode[0xbc]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void ldy6502()
            //{
            //      adrmode[opcode]();
            //      Y=gameImage[savepc];
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    Y=ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xbd]=4; instruction[0xbd]=lda6502; adrmode[0xbd]=absx6502;
        case 0xBD: //lda6502
            //      ticks[0xbd]=4; instruction[0xbd]=lda6502; adrmode[0xbd]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void lda6502()
            //{
            //      adrmode[opcode]();
            //      A=gameImage[savepc];
            //      // set the zero flag
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      // set the negative flag
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    A=ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=A;\n"); // printf("//");  printf("    if (A) P &= 0xfd; else P |= 0x02; ");            printf("    if (A & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xbe]=4; instruction[0xbe]=ldx6502; adrmode[0xbe]=absy6502;
        case 0xBE: //ldx6502
            //      ticks[0xbe]=4; instruction[0xbe]=ldx6502; adrmode[0xbe]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void ldx6502()
            //{
            //      adrmode[opcode]();
            //      X=gameImage[savepc];
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=Y;\n",temp);
            printf("    X=ReadMemory(temp);\n");
            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xbf]=2; instruction[0xbf]=nop6502; adrmode[0xbf]=implied6502;
        case 0xBF: //nop6502
            //      ticks[0xbf]=2; instruction[0xbf]=nop6502; adrmode[0xbf]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xc0]=3; instruction[0xc0]=cpy6502; adrmode[0xc0]=immediate6502;
        case 0xC0: //cpy6502
            //      ticks[0xc0]=3; instruction[0xc0]=cpy6502; adrmode[0xc0]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void cpy6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (Y+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=Y+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=Y; temp+=0x100; temp-=0x%02X;\n",rom[opadd+1]);
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
           if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//");  printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xc1]=6; instruction[0xc1]=cmp6502; adrmode[0xc1]=indx6502;
        case 0xC1: //cmp6502
            //      ticks[0xc1]=6; instruction[0xc1]=cmp6502; adrmode[0xc1]=indx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indx6502()
            //{
            //      value = gameImage[PC++]+X;
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=0x%02X; value+=X;\n",temp); //this should clip
            printf("    temp=ReadMemory(value+1);\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    value = ReadMemory(temp);\n");
            printf("    temp=A; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xc2]=2; instruction[0xc2]=nop6502; adrmode[0xc2]=implied6502;
        case 0xC2: //nop6502
            //      ticks[0xc2]=2; instruction[0xc2]=nop6502; adrmode[0xc2]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xc3]=2; instruction[0xc3]=nop6502; adrmode[0xc3]=implied6502;
        case 0xC3: //nop6502
            //      ticks[0xc3]=2; instruction[0xc3]=nop6502; adrmode[0xc3]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xc4]=3; instruction[0xc4]=cpy6502; adrmode[0xc4]=zp6502;
        case 0xC4: //cpy6502
            //      ticks[0xc4]=3; instruction[0xc4]=cpy6502; adrmode[0xc4]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void cpy6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (Y+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=Y+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%02X);\n",temp);
            printf("    temp=Y; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
           if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//");  printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xc5]=3; instruction[0xc5]=cmp6502; adrmode[0xc5]=zp6502;
        case 0xC5: //cmp6502
            //      ticks[0xc5]=3; instruction[0xc5]=cmp6502; adrmode[0xc5]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%02X);\n",temp);
            printf("    temp=A; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xc6]=5; instruction[0xc6]=dec6502; adrmode[0xc6]=zp6502;
        case 0xC6: //dec6502
            //      ticks[0xc6]=5; instruction[0xc6]=dec6502; adrmode[0xc6]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void dec6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc]--;
            //      value = gameImage[savepc];
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%02X);\n",temp);
            printf("    value--;\n");
            printf("    WriteMemory(0x%02X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xc7]=2; instruction[0xc7]=nop6502; adrmode[0xc7]=implied6502;
        case 0xC7: //nop6502
            //      ticks[0xc7]=2; instruction[0xc7]=nop6502; adrmode[0xc7]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xc8]=2; instruction[0xc8]=iny6502; adrmode[0xc8]=implied6502;
        case 0xC8: //iny6502
            //      ticks[0xc8]=2; instruction[0xc8]=iny6502; adrmode[0xc8]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void iny6502()
            //{
            //      Y++;
            //      if (Y) P &= 0xfd; else P |= 0x02;
            //      if (Y & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    Y++; \n");
            if(dozflag) printf("//"); printf("  ZN=Y;\n"); // printf("//"); printf("    if (Y) P &= 0xfd; else P |= 0x02; ");            printf("    if (Y & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xc9]=3; instruction[0xc9]=cmp6502; adrmode[0xc9]=immediate6502;
        case 0xC9: //cmp6502
            //      ticks[0xc9]=3; instruction[0xc9]=cmp6502; adrmode[0xc9]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=A; temp+=0x100; temp-=0x%02X;\n",rom[opadd+1]);
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe; \n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF; \n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xca]=2; instruction[0xca]=dex6502; adrmode[0xca]=implied6502;
        case 0xCA: //dex6502
            //      ticks[0xca]=2; instruction[0xca]=dex6502; adrmode[0xca]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void dex6502()
            //{
            //      X--;
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    X--; \n");
            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xcb]=2; instruction[0xcb]=nop6502; adrmode[0xcb]=implied6502;
        case 0xCB: //nop6502
            //      ticks[0xcb]=2; instruction[0xcb]=nop6502; adrmode[0xcb]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xcc]=4; instruction[0xcc]=cpy6502; adrmode[0xcc]=abs6502;
        case 0xCC: //cpy6502
            //      ticks[0xcc]=4; instruction[0xcc]=cpy6502; adrmode[0xcc]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void cpy6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (Y+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=Y+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp<ROMBASE) printf("    value=ReadMemory(0x%04X);\n",temp);
            else             printf("    value=0x%02X;\n",rom[temp]);
            printf("    temp=Y; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            break;
        //      ticks[0xcd]=4; instruction[0xcd]=cmp6502; adrmode[0xcd]=abs6502;
        case 0xCD: //cmp6502
            //      ticks[0xcd]=4; instruction[0xcd]=cmp6502; adrmode[0xcd]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp>=ROMBASE) printf("    value = 0x%02X;\n",rom[temp]);
            else              printf("    value = ReadMemory(0x%04X); \n",temp);
            printf("    temp=A; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe; \n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF; \n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xce]=6; instruction[0xce]=dec6502; adrmode[0xce]=abs6502;
        case 0xCE: //dec6502
            //      ticks[0xce]=6; instruction[0xce]=dec6502; adrmode[0xce]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void dec6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc]--;
            //      value = gameImage[savepc];
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%04X);\n",temp);
            printf("    value--;\n");
            printf("    WriteMemory(0x%04X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xcf]=2; instruction[0xcf]=nop6502; adrmode[0xcf]=implied6502;
        case 0xCF: //nop6502
            //      ticks[0xcf]=2; instruction[0xcf]=nop6502; adrmode[0xcf]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xd0]=2; instruction[0xd0]=bne6502; adrmode[0xd0]=relative6502;
        case 0xD0: //bne6502
            //      ticks[0xd0]=2; instruction[0xd0]=bne6502; adrmode[0xd0]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void relative6502()
            //{
            //      savepc = gameImage[PC++];
            //      if (savepc & 0x80) savepc -= 0x100;
            //      if ((savepc>>8) != (PC>>8))
            //              clockticks6502++;
            //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void bne6502()
            //{
            //      if ((P & 0x02)==0)
            //      {
            //              adrmode[opcode]();
            //              PC += savepc;
            //              clockticks6502++;
            //      }
            //      else
            //              value=gameImage[PC++];
            //}
            //            if((temp>>8)!=((opadd+1)>>8)) printf("    \n");
            temp2=(temp+opadd+2)&addrmask;
            //printf("    if ((P & 0x02)==0)\n");
            //printf("    if ((ZN & 0x02)==0)\n"); IF NOT ZERO
            printf("    if(ZN) //if not zero\n");
            printf("    {\n");
            printf("        clockticks++; \n");
            printf("        showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("        PCSTART=0x%04X;\n",temp2);
bcheckme(opadd);
bcheckme(temp2);
            printf("        /**/return;\n");
            printf("        //goto L_%04X;\n",temp2);
            printf("    }\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xd1]=5; instruction[0xd1]=cmp6502; adrmode[0xd1]=indy6502;
        case 0xD1: //cmp6502
            //      ticks[0xd1]=5; instruction[0xd1]=cmp6502; adrmode[0xd1]=indy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void indy6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //      if (ticks[opcode]==5)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    temp=temp+Y;\n");
            printf("    value = ReadMemory(temp);\n");
            printf("    temp=A; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            break;
        //      ticks[0xd2]=3; instruction[0xd2]=cmp6502; adrmode[0xd2]=indzp6502;
        case 0xD2: //cmp6502
            //      ticks[0xd2]=3; instruction[0xd2]=cmp6502; adrmode[0xd2]=indzp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void indzp6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value + 1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            break;
        //      ticks[0xd3]=2; instruction[0xd3]=nop6502; adrmode[0xd3]=implied6502;
        case 0xD3: //nop6502
            //      ticks[0xd3]=2; instruction[0xd3]=nop6502; adrmode[0xd3]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            break;
        //      ticks[0xd4]=2; instruction[0xd4]=nop6502; adrmode[0xd4]=implied6502;
        case 0xD4: //nop6502
            //      ticks[0xd4]=2; instruction[0xd4]=nop6502; adrmode[0xd4]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xd5]=4; instruction[0xd5]=cmp6502; adrmode[0xd5]=zpx6502;
        case 0xD5: //cmp6502
            //      ticks[0xd5]=4; instruction[0xd5]=cmp6502; adrmode[0xd5]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X; temp&=0xFF;\n",temp);
            printf("    value = ReadMemory(temp);\n");
            printf("    temp=A; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xd6]=6; instruction[0xd6]=dec6502; adrmode[0xd6]=zpx6502;
        case 0xD6: //dec6502
            //      ticks[0xd6]=6; instruction[0xd6]=dec6502; adrmode[0xd6]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void dec6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc]--;
            //      value = gameImage[savepc];
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X; temp&=0xFF;\n",temp);
            printf("    value=ReadMemory(temp);\n");
            printf("    value--;\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xd7]=2; instruction[0xd7]=nop6502; adrmode[0xd7]=implied6502;
        case 0xD7: //nop6502
            //      ticks[0xd7]=2; instruction[0xd7]=nop6502; adrmode[0xd7]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            break;
        //      ticks[0xd8]=2; instruction[0xd8]=cld6502; adrmode[0xd8]=implied6502;
        case 0xD8: //cld6502
            //      ticks[0xd8]=2; instruction[0xd8]=cld6502; adrmode[0xd8]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void cld6502()
            //{
            //      P &= 0xf7;
            //}
            //printf("    P &= 0xf7;\n");
            printf("    DEC = 0;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xd9]=4; instruction[0xd9]=cmp6502; adrmode[0xd9]=absy6502;
        case 0xD9: //cmp6502
            //      ticks[0xd9]=4; instruction[0xd9]=cmp6502; adrmode[0xd9]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=Y;            \n",temp);
            printf("    value = ReadMemory(temp);\n");
            printf("    temp=A; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xda]=3; instruction[0xda]=phx6502; adrmode[0xda]=implied6502;
        case 0xDA: //phx6502
            //      ticks[0xda]=3; instruction[0xda]=phx6502; adrmode[0xda]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void implied6502()
            //{
            //}
            //void phx6502()
            //{
            //      put6502memory(0x100+S--,X);
            //}
            //printf("    temp=0x100; temp+=S--;\n");
            //printf("    WriteMemory(temp,X);\n");
            printf("    stack[S--]=X;\n");

            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xdb]=2; instruction[0xdb]=nop6502; adrmode[0xdb]=implied6502;
        case 0xDB: //nop6502
            //      ticks[0xdb]=2; instruction[0xdb]=nop6502; adrmode[0xdb]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xdc]=2; instruction[0xdc]=nop6502; adrmode[0xdc]=implied6502;
        case 0xDC: //nop6502
            //      ticks[0xdc]=2; instruction[0xdc]=nop6502; adrmode[0xdc]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xdd]=4; instruction[0xdd]=cmp6502; adrmode[0xdd]=absx6502;
        case 0xDD: //cmp6502
            //      ticks[0xdd]=4; instruction[0xdd]=cmp6502; adrmode[0xdd]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void cmp6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (A+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=A+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    value = ReadMemory(temp);\n");
            printf("    temp=A; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xde]=7; instruction[0xde]=dec6502; adrmode[0xde]=absx6502;
        case 0xDE: //dec6502
            //      ticks[0xde]=7; instruction[0xde]=dec6502; adrmode[0xde]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=7;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void dec6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc]--;
            //      value = gameImage[savepc];
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    value=ReadMemory(temp);\n");
            printf("    value--;\n");
            printf("    WriteMemory(temp,value);\n");
           if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//");  printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xdf]=2; instruction[0xdf]=nop6502; adrmode[0xdf]=implied6502;
        case 0xDF: //nop6502
            //      ticks[0xdf]=2; instruction[0xdf]=nop6502; adrmode[0xdf]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            //printf("    //showme(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xe0]=3; instruction[0xe0]=cpx6502; adrmode[0xe0]=immediate6502;
        case 0xE0: //cpx6502
            //      ticks[0xe0]=3; instruction[0xe0]=cpx6502; adrmode[0xe0]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void cpx6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (X+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=X+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value = 0x%02X;\n",rom[opadd+1]);
            printf("    temp=X; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xe1]=6; instruction[0xe1]=sbc6502; adrmode[0xe1]=indx6502;
        case 0xE1: //sbc6502
            //      ticks[0xe1]=6; instruction[0xe1]=sbc6502; adrmode[0xe1]=indx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void indx6502()
            //{
            //      value = gameImage[PC++]+X;
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=0x%02X; value+=X;\n",temp); //this should clip
            printf("    temp=ReadMemory(value+1);\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");
            printf("    SBC(ReadMemory(temp));\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xe2]=2; instruction[0xe2]=nop6502; adrmode[0xe2]=implied6502;
        case 0xE2: //nop6502
            //      ticks[0xe2]=2; instruction[0xe2]=nop6502; adrmode[0xe2]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xe3]=2; instruction[0xe3]=nop6502; adrmode[0xe3]=implied6502;
        case 0xE3: //nop6502
            //      ticks[0xe3]=2; instruction[0xe3]=nop6502; adrmode[0xe3]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xe4]=3; instruction[0xe4]=cpx6502; adrmode[0xe4]=zp6502;
        case 0xE4: //cpx6502
            //      ticks[0xe4]=3; instruction[0xe4]=cpx6502; adrmode[0xe4]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void cpx6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (X+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=X+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=X; temp+=0x100; temp-=ReadMemory(0x%02X);\n",temp);
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xe5]=3; instruction[0xe5]=sbc6502; adrmode[0xe5]=zp6502;
        case 0xE5: //sbc6502
            //      ticks[0xe5]=3; instruction[0xe5]=sbc6502; adrmode[0xe5]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    SBC(ReadMemory(0x%02X));\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xe6]=5; instruction[0xe6]=inc6502; adrmode[0xe6]=zp6502;
        case 0xE6: //inc6502
            //      ticks[0xe6]=5; instruction[0xe6]=inc6502; adrmode[0xe6]=zp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void zp6502()
            //{
            //      savepc=gameImage[PC++];
            //}
            temp=rom[opadd+1];
            //void inc6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc]++;
            //      value = gameImage[savepc];
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%02X);\n",temp);
            printf("    value++;\n");
            printf("    WriteMemory(0x%02X,value);\n",temp);
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xe7]=2; instruction[0xe7]=nop6502; adrmode[0xe7]=implied6502;
        case 0xE7: //nop6502
            //      ticks[0xe7]=2; instruction[0xe7]=nop6502; adrmode[0xe7]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xe8]=2; instruction[0xe8]=inx6502; adrmode[0xe8]=implied6502;
        case 0xE8: //inx6502
            //      ticks[0xe8]=2; instruction[0xe8]=inx6502; adrmode[0xe8]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void inx6502()
            //{
            //      X++;
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    X++; \n");
           if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//");  printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f; \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xe9]=3; instruction[0xe9]=sbc6502; adrmode[0xe9]=immediate6502;
        case 0xE9: //sbc6502
            //      ticks[0xe9]=3; instruction[0xe9]=sbc6502; adrmode[0xe9]=immediate6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void immediate6502()
            //{
            //      savepc=PC++;
            //}
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    SBC(0x%02X);\n",rom[opadd+1]);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xea]=2; instruction[0xea]=nop6502; adrmode[0xea]=implied6502;
        case 0xEA: //nop6502
            //      ticks[0xea]=2; instruction[0xea]=nop6502; adrmode[0xea]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xeb]=2; instruction[0xeb]=nop6502; adrmode[0xeb]=implied6502;
        case 0xEB: //nop6502
            //      ticks[0xeb]=2; instruction[0xeb]=nop6502; adrmode[0xeb]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xec]=4; instruction[0xec]=cpx6502; adrmode[0xec]=abs6502;
        case 0xEC: //cpx6502
            //      ticks[0xec]=4; instruction[0xec]=cpx6502; adrmode[0xec]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void cpx6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc];
            //      if (X+0x100-value>0xff) P |= 0x01; else P &= 0xfe;
            //      value=X+0x100-value;
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp>=ROMBASE) printf("    value = 0x%02X;\n",rom[temp]);
            else              printf("    value = ReadMemory(0x%04X);\n",temp);
            printf("    temp=X; temp+=0x100; temp-=value;\n");
            //printf("    if (temp>0xFF) P |= 0x01; else P &= 0xfe;\n");
            printf("    if (temp>0xFF) C = 0x01; else C = 0x00;\n");
            printf("    value=temp&0xFF;\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xed]=4; instruction[0xed]=sbc6502; adrmode[0xed]=abs6502;
        case 0xED: //sbc6502
            //      ticks[0xed]=4; instruction[0xed]=sbc6502; adrmode[0xed]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            if(temp>=ROMBASE) printf("    SBC(0x%02X);\n",rom[temp]);
            else              printf("    SBC(ReadMemory(0x%04X));\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xee]=6; instruction[0xee]=inc6502; adrmode[0xee]=abs6502;
        case 0xEE: //inc6502
            //      ticks[0xee]=6; instruction[0xee]=inc6502; adrmode[0xee]=abs6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void abs6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //}
            tempword(opadd);
            //void inc6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc]++;
            //      value = gameImage[savepc];
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%04X);\n",temp);
            printf("    value++;\n");
            printf("    WriteMemory(0x%04X,value);\n",temp);
           if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//");  printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xef]=2; instruction[0xef]=nop6502; adrmode[0xef]=implied6502;
        case 0xEF: //nop6502
            //      ticks[0xef]=2; instruction[0xef]=nop6502; adrmode[0xef]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xf0]=2; instruction[0xf0]=beq6502; adrmode[0xf0]=relative6502;
        case 0xF0: //beq6502
            //      ticks[0xf0]=2; instruction[0xf0]=beq6502; adrmode[0xf0]=relative6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void relative6502()
            //{
            //      savepc = gameImage[PC++];
            //      if (savepc & 0x80) savepc -= 0x100;
            //      if ((savepc>>8) != (PC>>8))
            //              clockticks6502++;
            //}
            temp=rom[opadd+1];
            if(temp&0x80) temp-=0x100;
            //void beq6502()
            //{
            //      if (P & 0x02)
            //      {
            //              adrmode[opcode]();
            //              PC += savepc;
            //              clockticks6502++;
            //      }
            //      else
            //              value=gameImage[PC++];
            //}
            temp2=(opadd+temp+2)&addrmask;
            //printf("    if (P & 0x02) \n");
            //printf("    if(ZN & 0x02)\n");
            printf("    if(ZN==0)//if zero\n");
            printf("    { \n");
            printf("        clockticks++;  \n");
            printf("        showme(0x%04X,0x%02X);\n",opadd,opcode);
            printf("        PCSTART=0x%04X;\n",temp2);
bcheckme(opadd);
bcheckme(temp2);
            printf("        /**/return;\n");
            printf("        //goto L_%04X;\n",temp2);
            printf("    } \n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xf1]=5; instruction[0xf1]=sbc6502; adrmode[0xf1]=indy6502;
        case 0xF1: //sbc6502
            //      ticks[0xf1]=5; instruction[0xf1]=sbc6502; adrmode[0xf1]=indy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=5;\n");
            //void indy6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value+1] << 8);
            //      if (ticks[opcode]==5)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=ReadMemory(0x%02X);\n",temp+1);
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(0x%02X);\n",temp);
            printf("    temp=temp+Y;\n");
            printf("    SBC(ReadMemory(temp));\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xf2]=3; instruction[0xf2]=sbc6502; adrmode[0xf2]=indzp6502;
        case 0xF2: //sbc6502
            //      ticks[0xf2]=3; instruction[0xf2]=sbc6502; adrmode[0xf2]=indzp6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=3;\n");
            //void indzp6502()
            //{
            //      value = gameImage[PC++];
            //      savepc = gameImage[value] + (gameImage[value + 1] << 8);
            //}
            temp=rom[opadd+1];
            // MORE STUFF
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    value=ReadMemory(0x%02X);\n",rom[opadd+1]);
            printf("    temp=ReadMemory((unsigned short)(value+1));\n");
            printf("    temp<<=8;\n");
            printf("    temp|=ReadMemory(value);\n");


            printf("    crash(0x%04X,0x%02X); //65c02\n",opadd,opcode);
            break;
        //      ticks[0xf3]=2; instruction[0xf3]=nop6502; adrmode[0xf3]=implied6502;
        case 0xF3: //nop6502
            //      ticks[0xf3]=2; instruction[0xf3]=nop6502; adrmode[0xf3]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xf4]=2; instruction[0xf4]=nop6502; adrmode[0xf4]=implied6502;
        case 0xF4: //nop6502
            //      ticks[0xf4]=2; instruction[0xf4]=nop6502; adrmode[0xf4]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xf5]=4; instruction[0xf5]=sbc6502; adrmode[0xf5]=zpx6502;
        case 0xF5: //sbc6502
            //      ticks[0xf5]=4; instruction[0xf5]=sbc6502; adrmode[0xf5]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X; temp&=0xFF;\n",temp);
            printf("    SBC(ReadMemory(temp));\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xf6]=6; instruction[0xf6]=inc6502; adrmode[0xf6]=zpx6502;
        case 0xF6: //inc6502
            //      ticks[0xf6]=6; instruction[0xf6]=inc6502; adrmode[0xf6]=zpx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=6;\n");
            //void zpx6502()
            //{
            //      savepc=gameImage[PC++]+X;
            //      savepc &= 0x00ff;
            //}
            temp=rom[opadd+1];
            //(temp+=X)&0xFF
            //void inc6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc]++;
            //      value = gameImage[savepc];
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X; temp&=0xFF;\n",temp);
            printf("    value=ReadMemory(temp);\n");
            printf("    value++;\n");
            printf("    WriteMemory(temp,value);\n");
            if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//"); printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+2);
            break;
        //      ticks[0xf7]=2; instruction[0xf7]=nop6502; adrmode[0xf7]=implied6502;
        case 0xF7: //nop6502
            //      ticks[0xf7]=2; instruction[0xf7]=nop6502; adrmode[0xf7]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xf8]=2; instruction[0xf8]=sed6502; adrmode[0xf8]=implied6502;
        case 0xF8: //sed6502
            //      ticks[0xf8]=2; instruction[0xf8]=sed6502; adrmode[0xf8]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void sed6502()
            //{
            //      P |= 0x08;
            //}
            //printf("    P |= 0x08;\n");
            printf("    DEC = 0x08;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xf9]=4; instruction[0xf9]=sbc6502; adrmode[0xf9]=absy6502;
        case 0xF9: //sbc6502
            //      ticks[0xf9]=4; instruction[0xf9]=sbc6502; adrmode[0xf9]=absy6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absy6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+Y)>>8))
            //                      clockticks6502++;
            //      savepc += Y;
            //}
            tempword(opadd);
          //add Y!
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=Y;\n",temp);
            printf("    SBC(ReadMemory(temp));\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xfa]=4; instruction[0xfa]=plx6502; adrmode[0xfa]=implied6502;
        case 0xFA: //plx6502
            //      ticks[0xfa]=4; instruction[0xfa]=plx6502; adrmode[0xfa]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void implied6502()
            //{
            //}
            //void plx6502()
            //{
            //      X=gameImage[++S+0x100];
            //      if (X) P &= 0xfd; else P |= 0x02;
            //      if (X & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            //printf("    temp=++S; temp+=0x100;\n");
            //printf("    X=ReadMemory(temp);\n");
            printf("    X=stack[++S];\n");

            if(dozflag) printf("//"); printf("  ZN=X;\n"); // printf("//"); printf("    if (X) P &= 0xfd; else P |= 0x02; ");            printf("    if (X & 0x80) P |= 0x80; else P &= 0x7f;\n");
            printf("    crash(0x%04X,0x%02X); //65c02 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xfb]=2; instruction[0xfb]=nop6502; adrmode[0xfb]=implied6502;
        case 0xFB: //nop6502
            //      ticks[0xfb]=2; instruction[0xfb]=nop6502; adrmode[0xfb]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xfc]=2; instruction[0xfc]=nop6502; adrmode[0xfc]=implied6502;
        case 0xFC: //nop6502
            //      ticks[0xfc]=2; instruction[0xfc]=nop6502; adrmode[0xfc]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;
        //      ticks[0xfd]=4; instruction[0xfd]=sbc6502; adrmode[0xfd]=absx6502;
        case 0xFD: //sbc6502
            //      ticks[0xfd]=4; instruction[0xfd]=sbc6502; adrmode[0xfd]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=4;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void sbc6502()
            //{
            //      adrmode[opcode]();
            //      value = gameImage[savepc] ^ 0xff;
            //      saveflags=(P & 0x01);
            //      sum= ((char) A) + ((char) value) + (saveflags << 4);
            //      if ((sum>0x7f) || (sum<-0x80)) P |= 0x40; else P &= 0xbf;
            //      sum= A + value + saveflags;
            //      if (sum>0xff) P |= 0x01; else P &= 0xfe;
            //      A=sum;
            //      if (P & 0x08)
            //      {
            //              A -= 0x66;
            //              P &= 0xfe;
            //              if ((A & 0x0f)>0x09)
            //                      A += 0x06;
            //              if ((A & 0xf0)>0x90)
            //              {
            //                      A += 0x60;
            //                      P |= 0x01;
            //              }
            //      }
            //      else
            //      {
            //              clockticks6502++;
            //      }
            //      if (A) P &= 0xfd; else P |= 0x02;
            //      if (A & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    SBC(ReadMemory(temp));\n",temp);
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            checkme(opadd,opadd+3);
            break;
        //      ticks[0xfe]=7; instruction[0xfe]=inc6502; adrmode[0xfe]=absx6502;
        case 0xFE: //inc6502
            //      ticks[0xfe]=7; instruction[0xfe]=inc6502; adrmode[0xfe]=absx6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=7;\n");
            //void absx6502()
            //{
            //      savepc = gameImage[PC] + (gameImage[PC + 1] << 8);
            //      PC++;
            //      PC++;
            //      if (ticks[opcode]==4)
            //              if ((savepc>>8) != ((savepc+X)>>8))
            //                      clockticks6502++;
            //      savepc += X;
            //}
            tempword(opadd);
          //add X!
            //void inc6502()
            //{
            //      adrmode[opcode]();
            //      gameImage[savepc]++;
            //      value = gameImage[savepc];
            //      if (value) P &= 0xfd; else P |= 0x02;
            //      if (value & 0x80) P |= 0x80; else P &= 0x7f;
            //}
            printf("    temp=0x%04X; temp+=X;\n",temp);
            printf("    value = ReadMemory(temp);\n");
            printf("    value++;\n");
            printf("    WriteMemory(temp,value);\n");
           if(dozflag) printf("//"); printf("  ZN=value;\n"); // printf("//");  printf("    if (value) P &= 0xfd; else P |= 0x02; ");            printf("    if (value & 0x80) P |= 0x80; else P &= 0x7f;\n");
            //printf("    //showme(0x%04X,0x%02X);\n",opadd,opcode);
            break;
        //      ticks[0xff]=2; instruction[0xff]=nop6502; adrmode[0xff]=implied6502;
        case 0xFF: //nop6502
            //      ticks[0xff]=2; instruction[0xff]=nop6502; adrmode[0xff]=implied6502;
            //printf("case 0x%04X: //%02X %02X %02X\n",opadd,opcode,rom[opadd+1],rom[opadd+2]);
            printf("    //clockticks+=2;\n");
            //void implied6502()
            //{
            //}
            //void nop6502()
            //{
            //}
            printf("    crash(0x%04X,0x%02X); //not a 6502 instruction\n",opadd,opcode);
            checkme(opadd,opadd+1);
            break;

    }
}



//------------------------------------------------------------------------
void do_rom ( char *filename, unsigned short base )
{
    FILE *fp;


    fp=fopen(filename,"rb");
    if(fp==NULL)
    {
        printf("Error opening rom [%s]\n",filename);
        exit(1);
    }
    fread(rom+base,1,0x800,fp);
    fclose(fp);
}
//------------------------------------------------------------------------

int savehitlist ( void )
{
    FILE *fp;
    unsigned long ra;

    fp=fopen("hitlist.h","wt");
    if(fp==NULL) return(1);
    fprintf(fp,"\nunsigned char hitlist[    0x8000]=\n{\n");
    for(ra=0;ra<0x8000;ra++)
    {
        fprintf(fp,"  %u, //0x%04X 0x%02X\n",hitlist[ra],ra,rom[ra]);
    }
    fprintf(fp,"};\n\n");
    fclose(fp);


    return(0);
}

int savepcstart ( void )
{
    FILE *fp;
    unsigned long ra;

    fp=fopen("pcstart.h","wt");
    if(fp==NULL) return(1);
    fprintf(fp,"\nunsigned char pcstart[0x8000]=\n{\n");
    for(ra=0;ra<0x8000;ra++)
    {
        fprintf(fp,"  %u, //0x%04X 0x%02X\n",pcstart[ra],ra,rom[ra]);
    }
    fprintf(fp,"};\n\n");
    fclose(fp);

    return(0);
}

int savezflag ( void )
{
    FILE *fp;
    unsigned long ra;

    fp=fopen("zflag.h","wt");
    if(fp==NULL) return(1);
    fprintf(fp,"\nunsigned char zflag[256]=\n{\n");
    for(ra=0;ra<256;ra++)
    {
        fprintf(fp,"  %u, //0x%02X\n",zflag[ra],ra);
    }
    fprintf(fp,"};\n\n");
    fclose(fp);

    return(0);
}

int saveoplen ( void )
{
    FILE *fp;
    unsigned long ra;

    fp=fopen("oplen.h","wt");
    if(fp==NULL) return(1);
    fprintf(fp,"\nunsigned char oplen[256]=\n{\n");
    for(ra=0;ra<256;ra++)
    {
        fprintf(fp,"  %u, //0x%02X\n",oplen[ra],ra);
    }
    fprintf(fp,"};\n\n");
    fclose(fp);

    return(0);
}


int main ( void )
{
    memset(rom,0,sizeof(rom));

    do_rom ("035145.02", 0x6800);
    do_rom ("035144.02", 0x7000);
    do_rom ("035143.02", 0x7800);
    do_rom ("035127.02", 0x5000);

    rom[0x2000]=0x7F;
    rom[0x2001]=0x7F;
    rom[0x2002]=0x7F;
    rom[0x2003]=0x7F;
    rom[0x2004]=0x7F;
    rom[0x2005]=0x7F;
    rom[0x2006]=0x7F;
    rom[0x2007]=0x7F;
    rom[0x2400]=0x7F;
    rom[0x2401]=0x7F;
    rom[0x2402]=0x7F;
    rom[0x2403]=0x7F;
    rom[0x2404]=0x7F;
    rom[0x2405]=0x7F;
    rom[0x2406]=0x7F;
    rom[0x2407]=0x7F;
    rom[0x2800]=0xFC;
    rom[0x2801]=0xFC;
    rom[0x2802]=0xFD;
    rom[0x2803]=0xFC;
    rom[0x2C08]=0x9D;
    rom[0x2C09]=0x09;
    rom[0x2C0A]=0xFF;
    rom[0x2C0E]=0xFF;
    rom[0x2C0F]=0xFF;


    { unsigned long ra;

    fp=fopen("rom.h","wt");
    if(fp==NULL)
    {
        printf("Error creating file rom.h\n");
        return(1);
    }
    fprintf(fp,"\nunsigned char rom[0x8000]=\n{");
    for(ra=0;ra<0x8000;ra++)
    {
        fprintf(fp," 0x%02X, //%04X\n",rom[ra],ra);
    }
    fprintf(fp,"};\n\n");
    fclose(fp);

    }


//   memset(oplen,0,sizeof(oplen));

    temp=rom[0x7ffd]; temp<<=8; temp|=rom[0x7ffc];
    bcheckme(temp);
    printf("\n\nunsigned short PCSTART=0x%04X;\n",temp);
    temp=rom[0x7ffb]; temp<<=8; temp|=rom[0x7ffa];
    bcheckme(temp);
    printf("unsigned short NMISTART=0x%04X;\n",temp);


    printf("void game ( unsigned short opadd )\n");
    printf("{\n");
//    printf("    casecount[opadd]++;\n");
    printf("\n");
    printf("\n");
    printf("    switch(opadd)\n");
    printf("    {\n");
    printf("\n");
    printf("        default:\n");
    printf("            printf(\"CASE %%04X not defined\\n\",opadd);\n");
//    printf("            printf(\"%%lu\\n\",instcount);\n");
    printf("            exit(0);\n");

    for(sa=0;sa<0x8000;sa++)
    {
        if(hitlist[sa]==0)
        {
           // printf("//case 0x%04X: //%02X %02X %02X --SKIPPED--\n",sa,rom[sa+0],rom[sa+1],rom[sa+2]);
            continue;
        }
        translate(sa,rom[sa]);
    }


    printf("    }\n");
    printf("}\n");
    printf("\n");


    savehitlist();

  //  memset(pcstart,0,sizeof(pcstart));

    savepcstart();

    savezflag();
    saveoplen();

    return(0);
}



