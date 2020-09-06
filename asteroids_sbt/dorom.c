

//------------------------------------------------------------------------
//------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *fp;

unsigned short addrmask = 0x7FFF;

unsigned long ra;

unsigned char rom[0x8000];

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
    return(0);
}
//------------------------------------------------------------------------
//------------------------------------------------------------------------


