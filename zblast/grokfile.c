/* zblast - simple shoot-em-up.
 * Copyright (C) 1993-2000 Russell Marks. See zblast.c for license.
 *
 * sod2.c/grokfile.c are from:
 */
/* sod2, a player for polychannel .csf music files.
 * Copyright (C) 1995 Russell Marks. See sod2.c for license details.
 *
 * grokfile.c - parse a .csf file (derived from makesod.c).
 */

/* Note that in zblast, this can only be run on the installed csf
 * files, which are known-valid csf files. I've tried to fix/avoid any
 * obvious buffer overruns etc. here, but if any still exist in this
 * particular file it's not the end of the world, as there's no
 * potential for user-supplied data to screw things up.
 */

#ifdef MUSIC_SUPPORT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "sod2.h"


char sample_path[1024],sample_file[1024];

static int curline=0;


/* this is a bit unpleasant, but it won't work otherwise */
extern int lines[MAX_NOTES][MAX_LINE_DEPTH];


/* (some) function prototypes */
void makecsffilename(char *new,char *org,int newsize);
int sod2_getline(char *buf,FILE *in);
void check_linelen(char *buf,int bsize,int has_comma);
void check_samplenum(int samplenum);
void processcsf(FILE *in);
void addline(int num,int mode,
	char *notes,char *octaves,char *sample,
        char *stereo1,char *stereo2,char *relvol,
	int sod_time,int sod_down,int sod_pos);
void load_sample(char *filename);
void sign(void);
void padline(char *str,int n);
void interp2(void);
void interp4(void);


void grokfile(char *filename)
{
FILE *in;
char inname[256];

makecsffilename(inname,filename,sizeof(inname));
if((in=fopen(inname,"r"))==NULL)
  {
  fprintf(stderr,"zblast: couldn't open file `%s'.\n",filename);
  exit(1);
  }
else
  {
  processcsf(in);
  fclose(in);
  }
}


void makecsffilename(char *new,char *org,int newsize)
{
char *ptr;

if((ptr=strrchr(org,'.'))!=NULL)
  snprintf(new,newsize,"%s",org);
else
  snprintf(new,newsize,"%s.csf",org);
}


int sod2_getline(char *buf,FILE *in)
{
char *ptr;

buf[0]=0;
do
  ptr=fgets(buf,256,in);
while(buf[0]=='#' && !feof(in));
		     
if(ptr!=NULL)
  {
  if(buf[0]!=0 && buf[strlen(buf)-1]=='\n') buf[strlen(buf)-1]=0;
  if(buf[0]!=0 && buf[strlen(buf)-1]=='\r') buf[strlen(buf)-1]=0;
  }
else
  buf[0]=0;
return((ptr==NULL)?-1:0);
}


/* check line len, and fix it if too long (giving warning) */
void check_linelen(char *buf,int bsize,int has_comma)
{
char *cptr=NULL;

/* if we allow a comma, see if we have one or not. */
if(has_comma)
  cptr=strchr(buf,',');

if((cptr==NULL && strlen(buf)>bsize) ||
   (cptr!=NULL && (cptr-buf)>bsize))
  {
  buf[bsize]=0;
  fprintf(stderr,"zblast: line %d too long.\n",curline);
  exit(1);
  }
}


/* check sample number is ok - exit if not. */
void check_samplenum(int samplenum)
{
if(samplenum<=0 || samplenum>=next_sample)
  {
  fprintf(stderr,"zblast: bad sample number %d on line %d!\n",samplenum,curline);
  exit(1);
  }
}



void processcsf(FILE *in)
{
char buf[256],buf2[256],buf3[256],vbuf[256],sbuf[256],sbuf2[256],*ptr;
int f,g,i,mode,len;
int sod_time,sod_down,sod_pos;

/* look for '*samples' (and possibly '*tempo', '*bsize' before that) */
sod2_getline(buf,in);
if(!strncmp(buf,"*tempo",6))
  {
  tempo=atoi(buf+6);
  sod2_getline(buf,in);
  }

if(!strncmp(buf,"*bsize",6))
  {
  bsize=atoi(buf+6);
  sod2_getline(buf,in);
  if(bsize>64)
    {
    fprintf(stderr,"zblast: maximum blocksize allowed is 64.\n");
    exit(1);
    }
  }

if(strcmp(buf,"*samples"))
  {
  fprintf(stderr,"zblast: missing `*samples' block (must be first).\n");
  exit(1);
  }

/* add all the sample descriptions */
next_sample=1;
sod2_getline(buf,in);
while(strcmp(buf,"*blocklist"))
  {
  while((ptr=strchr(buf,'\t'))!=NULL) *ptr=32;
  if((ptr=strchr(buf,' '))==NULL)
    {
    fprintf(stderr,"zblast: couldn't find sample rate on line %d.\n",curline);
    exit(1);
    }

  samples[next_sample].rate=atoi(ptr);
  *ptr=0;

  load_sample(buf);

  len=strlen(buf);
  if(len>4 && strcmp(buf+len-4,".ami")!=0)
    sign();
  
  next_sample++;
  if(sod2_getline(buf,in)==-1)
    {
    fprintf(stderr,"zblast: missing `*blocklist' section.\n");
    exit(1);
    }
  }

g=1;
sod2_getline(buf,in);
while(strcmp(buf,"*blocks"))
  {
  f=0;
  ptr=strrchr(buf,',');
  if(ptr!=NULL)
    {
    do
      {
      *ptr++=0;
      lines[g][f]=atoi(ptr);
      f++;
      }
    while((ptr=strrchr(buf,','))!=NULL);
    }
  lines[g][f]=atoi(buf);
    
  g++;    
  if(sod2_getline(buf,in)==-1)
    {
    fprintf(stderr,"zblast: missing `*blocks' section.\n");
    exit(1);
    }
  }

if(g==0)
  {
  fprintf(stderr,"zblast: must be some patterns in the block list!\n");
  exit(1);
  }

last_line=g-1;


/* now interpret the patterns, and write them as we go along. */
f=1;
while(sod2_getline(buf,in)!=-1)
  {
  check_linelen(buf,bsize,0);
  
  for(i=0;i<bsize;i++) vbuf[i]='9';	/* max relative vol. */
  vbuf[bsize]=0;
  for(i=0;i<bsize;i++) sbuf[i]=' ';
  sbuf[bsize]=0;
  for(i=0;i<bsize;i++) sbuf2[i]=' ';	/* space=use sod_pos */
  sbuf2[bsize]=0;
  
  sod_time=10000; sod_down=10; sod_pos=0;
  mode=DEFAULTMODE;
  while(buf[0]=='*')
    {
    switch(buf[1])
      {
      case 'p':
        mode=PIANO_MODE; break;
      case 'm':
        mode=MULTI_MODE; break;
      case 'r':
        mode=RETRIGGER_MODE; break;
      case 'e':
        ptr=strchr(buf,',');
        *ptr=0;
        sod_time=atoi(buf+3);
        ptr++;
        sod_down=atoi(ptr);
        break;
      case 'v':
        sod2_getline(vbuf,in);
        check_linelen(vbuf,bsize,0);
        padline(vbuf,bsize);
        break;
      case 's':		/* stereo position */
        sod_pos=atoi(buf+3);
        if(sod_pos<-16) sod_pos=-16;
        if(sod_pos>16) sod_pos=16;
        break;
      case 'S':		/* stereo position like *v */
        sod2_getline(sbuf,in);
        check_linelen(sbuf,bsize,0);
        padline(sbuf,bsize);
        sod2_getline(sbuf2,in);
        check_linelen(sbuf2,bsize,0);
        padline(sbuf2,bsize);
        break;
      }
    sod2_getline(buf,in);
    check_linelen(buf,bsize,0);
    }

  padline(buf ,bsize);
  sod2_getline(buf2,in); check_linelen(buf2,bsize,0); padline(buf2,bsize);
  sod2_getline(buf3,in); check_linelen(buf3,bsize,1);
  addline(f,mode,buf,buf2,buf3,sbuf,sbuf2,vbuf,sod_time,sod_down,sod_pos);
  f++;
  }

last_pattern=f-1;
}


void addline(int num,int mode,
	char *notes,char *octaves,char *sample,
        char *stereo1,char *stereo2,char *relvol,
	int sod_time,int sod_down,int sod_pos)
{
int note=0,f,gotnote,samplenum,vol;
char *ptr;
int tmp;

if(sod_pos<-16) sod_pos=-16;
if(sod_pos> 16) sod_pos= 16;

ptr=strchr(sample,',');
if(ptr==NULL)
  vol=DEFAULTVOLUME;
else
  {
  *ptr=0;
  ptr++;
  vol=atoi(ptr);
  }

if((strlen(sample)>10)||(sample[strlen(sample)-1]=='*'))
  {
  if(sample[strlen(sample)-1]=='*')
    sample[strlen(sample)-1]=0;
  samplenum=-1;
  }
else
  {
  tmp=sample[0];
  if(tmp>'Z') tmp-=32;
  tmp-='0';
  if(tmp>9) tmp-=7;
  
  samplenum=tmp;
  check_samplenum(samplenum);
  }

patterns[num].mode=mode;

if(strlen(notes)==bsize)
  {
  for(f=0;f<bsize;f++)
    {
    gotnote=1;
    switch(notes[f])
      {
      case 'c': note= 0; break;
      case 'C': note= 1; break;
      case 'd': note= 2; break;
      case 'D': note= 3; break;
      case 'e': note= 4; break;
      case 'f': note= 5; break;
      case 'F': note= 6; break;
      case 'g': note= 7; break;
      case 'G': note= 8; break;
      case 'a': note= 9; break;
      case 'A': note=10; break;
      case 'b': note=11; break;
      default:  gotnote=0;
      }

    if(gotnote)
      {
      patterns[num].notes[f].notenum=note+12*(octaves[f]-'0'-OCTAVEFIX);
      patterns[num].notes[f].vol=vol*(relvol[f]-48)/9;
      
      /* now work out stereo pos */
      tmp=sod_pos;
      if((stereo2[f]>='0' && stereo2[f]<='9') ||
         (stereo2[f]>='A' && stereo2[f]<='G') ||
         (stereo2[f]>='a' && stereo2[f]<='g'))
        {
        tmp=stereo2[f];
        if(tmp>'Z') tmp-=32;
        tmp-='0';
        if(tmp>9) tmp-=7;
        if(stereo1[f]=='-' || stereo1[f]=='l' || stereo1[f]=='L')
          tmp=-tmp;	/* i.e. left instead of right */
        }
      patterns[num].notes[f].pos=tmp;
      
      if(samplenum==-1)
        {
        tmp=sample[f];
        if(tmp>'Z') tmp-=32;
        tmp-='0';
        if(tmp>9) tmp-=7;
        check_samplenum(tmp);
        patterns[num].notes[f].sample=tmp;
        }
      else
        patterns[num].notes[f].sample=samplenum;
      }
    else
      {
      patterns[num].notes[f].vol=0;	/* no note */
      if(notes[f]==';') patterns[num].notes[f].vol=-1;
      }

    patterns[num].notes[f].pattern=num;
    patterns[num].notes[f].mode=mode;
    patterns[num].notes[f].sustain=sod_time;
    patterns[num].notes[f].release=sod_down;
    }
  }
}


/* well, it works :) */
/* NB: n chars plus NUL must fit into str */
void padline(char *str,int n)
{
while(strlen(str)<n)
  strcat(str," ");
}


void load_sample(char *filename)
{
FILE *in;
int len,lastone;
char *path,*pathptr,*ptr;

/* find out where the sample actually is. */

if((path=getenv("SOD2_SAMPLE_PATH"))!=NULL)
  snprintf(sample_path,sizeof(sample_path),"%s:%s",path,MINIMUM_SAMPLE_PATH);
else
  snprintf(sample_path,sizeof(sample_path),"%s",MINIMUM_SAMPLE_PATH);

pathptr=sample_path;
lastone=0;

do
  {
  if((ptr=strchr(pathptr,':'))!=NULL) 
    *ptr=0;
  else
    lastone=1;

  snprintf(sample_file,sizeof(sample_file),"%s/%s",pathptr,filename);
  
  pathptr+=strlen(pathptr)+1;
  }
while((in=fopen(sample_file,"rb"))==NULL && lastone==0);

if(in==NULL)
  {
  fprintf(stderr,"zblast: couldn't open sample file `%s'.\n",filename);
  exit(1);
  }

/* find out how big the sample is */

if(fseek(in,0,SEEK_END)==-1)	die("fseek on sample file");
if((len=ftell(in))==-1)		die("ftell on sample file");
rewind(in);

/* load the sample */

/* need an extra blank byte in case '-i' is used */
if((samples[next_sample].data=malloc(len+1))==NULL) die("malloc");
samples[next_sample].data[len]=0;
if(fread(samples[next_sample].data,1,len,in)!=len) die("fread");
fclose(in);
samples[next_sample].len=len;

if(verbose)
  fprintf(stderr,"%s, %d bytes\n",sample_file,len);
}


/* only 8-bit input samples are allowed, so this is ok for now */
void sign()
{
int f;
unsigned char *ptr;

ptr=samples[next_sample].data;
for(f=0;f<samples[next_sample].len;f++,ptr++)
  *ptr-=128;
}


#endif /* MUSIC_SUPPORT */
