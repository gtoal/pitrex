/* sound.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef DO_SOUND
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#endif

#include "sound.h"


/* use 256 byte frags */
#define BASE_SOUND_FRAG_PWR	8
#define BASE_SOUND_BUFSIZ	(1<<BASE_SOUND_FRAG_PWR)

int soundfd=-1;
int parentpid;
int sndpipe[2];
int sound_bufsiz;


struct channel_tag
  {
  struct sample_tag *sample;	/* pointer to sample struct, NULL if none */
  int offset;			/* position in sample */
  } channel[NUM_CHANNELS];

/* sample filenames */
char *samplename[]=
  {
  "pshot.ub",
  "thrust.ub",
  "explode.ub",
  "explode2.ub",
  "bshot.ub",
  "phit.ub",
  "title.ub",
  "newbonus.ub",
  "newbad.ub",
  "bonusgot.ub",
  "bwound.ub",
  "swarmsplit.ub"
  };

/* for in-memory samples */
struct sample_tag
  {
  unsigned char *data;		/* pointer to sample, NULL if none */
  int length;			/* length of sample */
  } sample[NUM_SAMPLES];



#ifdef DO_SOUND

#define BYTEFIX(x)	(((x)<0)?0:(((x)>255)?255:(x)))

/* mix and play a chunk of sound to /dev/dsp. */
void snd_playchunk(void)
{
int f,g,v;
struct channel_tag *cptr;
static unsigned char soundbuf[1024];

if(soundfd==-1)
  {
  usleep(50000);
  return;
  }

for(f=0;f<sound_bufsiz;f++)
  {
  v=0;
  for(g=0,cptr=&(channel[0]);g<NUM_CHANNELS;g++,cptr++)
    if(cptr->sample!=NULL)
      {
      v+=(int)cptr->sample->data[cptr->offset++];
      if(cptr->offset>=cptr->sample->length)
        cptr->sample=NULL;
      }
    else
      v+=128;	/* make sure it doesn't click! */
  
  /* kludge to up the volume of sound effects - mmm, lovely distortion :-) */
  v-=128*NUM_CHANNELS;
  v=128+(v*3)/(2*NUM_CHANNELS);
  v=BYTEFIX(v);
  
  soundbuf[f]=(unsigned char)v;
  }

write(soundfd,soundbuf,sound_bufsiz);
}


void snd_main(void)
{
unsigned char buf[2];
int live=1,ret;

close(sndpipe[1]);

while(live)
  {
  while((ret=read(sndpipe[0],buf,2))>0)
    {
    switch(*buf)
      {
      case KILL_SNDSERV:
        live=0;
        break;
      
      default:
        channel[buf[0]].sample=&(sample[buf[1]]);
        channel[buf[0]].offset=0;
      }
    }
  
  /* stop if pipe was broken */
  if(ret==0) break;
  
  snd_playchunk();
  }

exit(0);
}


void start_sound(void)
{
int f;

for(f=0;f<NUM_CHANNELS;f++) channel[f].sample=NULL;

for(f=0;f<NUM_SAMPLES;f++) sample[f].data=NULL;

pipe(sndpipe);
fcntl(sndpipe[0],F_SETFL,O_NONBLOCK);
fcntl(sndpipe[1],F_SETFL,O_NONBLOCK);

parentpid=getpid();

if(fork())
  return;	/* parent */

/* child - sound player */

sound_bufsiz=BASE_SOUND_BUFSIZ;
#ifndef SNDCTL_DSP_SETFRAGMENT
soundfd=-1;
#else
if((soundfd=open("/dev/dsp",O_WRONLY))<0)
  soundfd=-1;
else
  {
  int frag;
  FILE *in;
  static char buf[1024];

  frag=(0x20000|BASE_SOUND_FRAG_PWR);
  ioctl(soundfd,SNDCTL_DSP_SETFRAGMENT,&frag);
  
  /* load in the samples */
  for(f=0;f<NUM_SAMPLES;f++)
    {
    sprintf(buf,"%s/%s",SOUNDSDIR,samplename[f]);
    if((in=fopen(buf,"rb"))!=NULL)
      {
      fseek(in,0,SEEK_END);
      sample[f].length=ftell(in);
      if((sample[f].data=malloc(sample[f].length))==NULL)
        break;
      rewind(in);
      fread(sample[f].data,1,sample[f].length,in);
      fclose(in);
      }
    }
  }
#endif

snd_main();
}


/* setup a new sample to be played on a given channel. */
void queuesam(int chan,int sam)
{
unsigned char buf[2];

buf[0]=chan;
buf[1]=sam;
write(sndpipe[1],buf,2);
}

#else	/* !DO_SOUND */

/* stubs */
void queuesam(int chan,int sam)
{
}

void start_sound(void)
{
}

#endif	/* !DO_SOUND */
