/* zblast - simple shoot-em-up.
 * Copyright (C) 1993-2003 Russell Marks. See zblast.c for license.
 *
 * hiscore.c - deals with drawing/reading/writing hi-scores.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#ifndef DONT_GETPWUID
#include <pwd.h>
#include <sys/types.h>
#endif
#include "font.h"


/* from zblast.c */
extern int ext_setcol();


/* must be even */
#define NUM_SCORES	20

#define NAME_MAX_LEN	7


struct hiscore
  {
  char name[NAME_MAX_LEN+1];
  int score;
  } hitable[NUM_SCORES];



void gethiscores(void)
{
FILE *in;
static char name[1024],buf[1024];
int score;
int pos,f,g;

for(f=0;f<NUM_SCORES;f++)
  {
  hitable[f].name[0]=0;
  hitable[f].score=0;
  }

if((in=fopen(SCORES_FILE,"r"))==NULL)
  {
  fprintf(stderr,"Couldn't read scores file\n");
  return;
  }

while(fgets(name,sizeof(name),in)!=NULL)
  {
  if(name[0]!='N')
    {
    fprintf(stderr,"Score file looks corrupted\n");
    continue;
    }
  
  name[strlen(name)-1]=0;
  
  /* no more than a certain no. chars... */
  name[2+NAME_MAX_LEN]=0;
  
  if(fgets(buf,sizeof(buf),in)==NULL) break;
  
  if(buf[0]!='S')
    {
    fprintf(stderr,"Score file looks corrupted\n");
    continue;
    }
  
  score=atoi(buf+2);
  
  pos=-1;
  for(f=NUM_SCORES-1;f>=0;f--)
    if(score>hitable[f].score)
      pos=f;
  if(pos!=-1)
    {
    for(g=NUM_SCORES-1;g>pos;g--)
      hitable[g]=hitable[g-1];
    strcpy(hitable[pos].name,name+2);
    hitable[pos].score=score;
    }
  }

fclose(in);
}


void drawhiscores(int bigwindow,int use_x)
{
static char buf[128];
int x,y,siz,xm,ym;
int f,mid;
int zw;

x=bigwindow?10:0;
y=bigwindow?150:(use_x?65:50);
xm=bigwindow?450:210;
ym=bigwindow?20:10;
siz=bigwindow?3:2;

zw=vgatextsize(siz,"0");

gethiscores();

mid=NUM_SCORES/2;

for(f=0;f<NUM_SCORES;f++)
  {
  ext_setcol(14-(f&3));
  sprintf(buf,"%2d %06d %s",f+1,hitable[f].score,hitable[f].name);
  vgadrawtext(x+(f/mid)*xm+zw*(*buf==32),y+(f%mid)*ym,siz,buf+(*buf==32));
  }
}


void writescore(int score)
{
#ifndef DONT_GETPWUID
struct passwd *pd;
#endif
static char name[1024];
char *ptr;
FILE *out;
int capsify=0;

if((ptr=getenv("ZBLAST_NAME"))==NULL)
  {
  capsify=1;
  if((ptr=getenv("USER"))==NULL)
    if((ptr=getenv("LOGNAME"))==NULL)
      {
#ifdef DONT_GETPWUID
      strcpy(name,"someone");
#else
      pd=getpwuid(getuid());
      snprintf(name,sizeof(name),"%s",pd->pw_name);
#endif
      }
  }

if(ptr!=NULL)
  snprintf(name,sizeof(name),"%s",ptr);

if(capsify && islower(name[0]))
  name[0]=toupper(name[0]);

/* no more than a certain no. chars... */
name[NAME_MAX_LEN]=0;
  
if((out=fopen(SCORES_FILE,"a"))==NULL)
  fprintf(stderr,"zblast: couldn't write to scores file!\n");
else
  {
  fprintf(out,"N=%s\nS=%d\n",name,score);
  fclose(out);
  }
}
