/* zblast - simple shoot-em-up.
 * Copyright (C) 1993-2000 Russell Marks. See zblast.c for license.
 *
 * sod2.c/grokfile.c are from:
 */
/* sod2, a player for polychannel .csf music files.
 * Copyright (C) 1995, 1996 Russell Marks.
 *
 * Based on `sodman' by Graham Richards.
 *
 * hacked for zblast 960329
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * sod2.c - the main player code.
 */

#ifdef MUSIC_SUPPORT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#ifdef linux
#include <sys/soundcard.h>
#endif
#include "sod2.h"
#include "grokfile.h"


#define SOD2_VER	"2.0"

#if 0
/* remove 'inline' if the compiler doesn't like it */
#ifndef _ALLOW_INLINE
#define inline
#endif
#endif

/* global data */

int bsize=64;			/* pattern length being used */
int tempo=125;
int last_pattern,last_line;
int slotlen;			/* length of a notespace in samples */

/* output environment */
/* output is generated internally at 'vsr', but output at 'psr'. */

int sample_maxval=255;		/* defaults to 8-bit output */
int sample_midval=128;		/* 'middle' value - approx half of above */
int vsr=8000;			/* virtual (subsampled) sample rate */
int psr=8000;			/* physical (actual) sample rate */

/* flags */

#ifdef linux
int ioctlfile=1;
#else
int ioctlfile=0;
#endif
int preconstruct=1;		/* resample in advance, before playing */
int startline=1;

int realstereo=0;
int rstereobuf_l[1024],rstereobuf_r[1024];
int rstereopos;


int verbose=0;


int next_sample=0;
int sampletick=0,oldstick=0;
struct timeval oldtv,newtv;
int minhz=(1<<30);
int oldval;
int maxchan;

struct sample samples[MAX_SAMPLES];	/* ptrs to 8-bit unsigned samples */

struct pattern patterns[MAX_PATTERNS];

/* entries in line[x][] are indicies into patterns[]. */
int lines[MAX_LINES][MAX_LINE_DEPTH];

/* active notes array - an entry is NULL if unused, otherwise it points
 * to a struct note in a struct pattern somewhere.
 */
struct note *active[MAX_NOTES];
int last_active;

int play_done;

/* these (from original play()) now must be global */
int line,notesp,noteslot,count,note,pos;
int done=0;
struct note **aptr,*nptr;





/* function prototypes */
void clear_data(void);
void runtime_fixup(void);
struct preconstruct *find_precon(struct note *nptr,
	struct preconstruct *preconarr,int preconlen);
void play(void);
int addnotes(int line,int noteslot);
static inline int gensample(int note,struct note *nptr);
static inline void gen_stereo(int pos,int val);
void output_sample(int count);
#ifdef linux
void ioctl_dsp(int fd);
#endif

void (*write_sample)(int c);


/* code starts here */


/* set sample rate - call this before any others */
void play_set_rate(int rate)
{
vsr=psr=rate;
}


void play_init_existing()
{
last_active=-1;
oldval=sample_midval;
maxchan=0;
if(startline>last_line || startline<0) startline=last_line;

line=startline;
noteslot=0;
notesp=0;
play_done=done=0;
}


void play_init(char *fname,void (*output_func)(int c))
{
int f;

write_sample=output_func;

if(realstereo)
  {
  for(f=0;f<1024;f++)
    rstereobuf_l[rstereopos]=rstereobuf_r[rstereopos]=0;
  rstereopos=0;
  }

vsr=psr;

clear_data();			/* zero out arrays and general init */
grokfile(fname);		/* read in file */

runtime_fixup();	/* fix runtime parts of arrays ready for playing */

play_init_existing();
}


void play_uninit()
{
/* XXX this should free memory etc. and generally reset */
}


void clear_data()
{
int f,g;

for(f=0;f<MAX_NOTES;f++)	active[f]=NULL;
for(f=0;f<MAX_SAMPLES;f++)	samples[f].data=NULL;
for(f=0;f<MAX_PATTERNS;f++)	patterns[f].mode=-1;
for(f=0;f<MAX_LINES;f++)
  for(g=0;g<MAX_LINE_DEPTH;g++)
    lines[f][g]=0;
}


void runtime_fixup()
{
int f,g,rate;
int maxgap=0,mingap=(1<<30);

/* the general idea here is to put in all the timing information
 * in the note structs.
 */

slotlen=15*vsr/tempo;

for(f=1;f<=last_pattern;f++) if(patterns[f].mode!=-1)
  for(g=0;g<bsize;g++) if(patterns[f].notes[g].vol>0)
    {
    rate=samples[patterns[f].notes[g].sample].rate;

    /* notenum -> next-sample gap */
    /* time to wheel out the magic numbers */
    patterns[f].notes[g].gap=FIX_DBL(((double)rate*
    	pow(1.059463,(double)patterns[f].notes[g].notenum)/(double)vsr));
    
    if(patterns[f].notes[g].gap>maxgap) maxgap=patterns[f].notes[g].gap;
    if(patterns[f].notes[g].gap<mingap) mingap=patterns[f].notes[g].gap;

    /* fix enveloping stuff */
    patterns[f].notes[g].sustain*=slotlen/8;
    patterns[f].notes[g].release*=slotlen/8;
    patterns[f].notes[g].relsus=patterns[f].notes[g].release;
    patterns[f].notes[g].release+=patterns[f].notes[g].sustain;

    patterns[f].notes[g].data=samples[patterns[f].notes[g].sample].data;
    patterns[f].notes[g].len=FIX_UP(samples[patterns[f].notes[g].sample].len);
    
    /* scale up (or down) stereo positioning
     * it's currently correct for 8kHz
     */
    patterns[f].notes[g].pos=(patterns[f].notes[g].pos*vsr)/8000;
    }

/* the remainder of the routine is for preconstruct mode only */

if(preconstruct)
  {
  /* ok, resample everything now rather than later */
  struct note *noteptr;
  int pattern,noteslot;
  int total_notes=0;
  int preconlen=0,preconsize=16;
  int mem=0;
  struct preconstruct *preptr;
  struct preconstruct *preconarr=
  		malloc(preconsize*sizeof(struct preconstruct));
  
  
  /* the theory behind this is that patterns and notes will almost
   * certainly be repeated many times, and if we generate all the notes
   * that will be needed *in advance*, this will probably save a
   * lot of CPU time when we start playing. The disadvantages are
   * that it delays the start of playback, and it takes a lot more
   * memory.
   */

  if(preconarr==NULL) die("allocate memory");

  /* we first find all the unique notes */
  for(pattern=1;pattern<=last_pattern;pattern++)
    for(noteslot=0;noteslot<bsize;noteslot++)
      {
      noteptr=&(patterns[pattern].notes[noteslot]);
      
      /* don't look at the rest of a pattern after ';' */
      if(noteptr->vol==-1) break;
      
      /* skip any notespaces without actual notes */
      if(noteptr->vol==0) continue;
      
      /* add the note if it hasn't been seen before */
      if(find_precon(noteptr,preconarr,preconlen)==NULL)
        {
        preconlen++;
        if(preconlen*sizeof(struct preconstruct)>preconsize)
          {
          preconsize+=128*sizeof(struct preconstruct);
          if((preconarr=realloc(preconarr,preconsize))==NULL)
            die("allocate memory");
          }
        
        preconarr[preconlen-1].notenum=noteptr->notenum;
        preconarr[preconlen-1].sustain=noteptr->sustain;
        preconarr[preconlen-1].release=noteptr->release;
        preconarr[preconlen-1].vol    =noteptr->vol;
        preconarr[preconlen-1].sample =noteptr->sample;
        preconarr[preconlen-1].notenum=noteptr->notenum;
        preconarr[preconlen-1].exnote =noteptr;
        }
      
      total_notes++;
      }
  
  /* now do the resampling */
  last_active=0;
  for(f=0;f<preconlen;f++)
    {
    int onesec=vsr;
    int size=onesec;
    int len=0;
    signed char *data=malloc(size);
    
    if(data==NULL) die("allocate memory");
    
    active[0]=preconarr[f].exnote;
    active[0]->offset=0;
    active[0]->counter=0;
    
    while(active[0]!=NULL)
      {
      g=gensample(0,active[0]);
      if(len>=size)
        if((data=realloc(data,size+=onesec))==NULL)
          die("allocate memory");
      if(g<-128) g=-128; if(g>127) g=127;
      data[len++]=g;
      }
    
    preconarr[f].data.p8=data;
    
    preconarr[f].len=len;
    mem+=len;
    }

  /* now go back and fill in the 'predata' and 'prelen' fields
   * in all the note structs.
   */
  for(pattern=1;pattern<=last_pattern;pattern++)
    for(noteslot=0;noteslot<bsize;noteslot++)
      {
      noteptr=&(patterns[pattern].notes[noteslot]);
      if(noteptr->vol==-1) break;
      if(noteptr->vol==0) continue;
      
      if((preptr=find_precon(noteptr,preconarr,preconlen))==NULL)
        die("find resampled note (can't happen!)");
      else
        {
        noteptr->p.s8.predata=preptr->data.p8;
        noteptr->p.s8.premax =preptr->data.p8+preptr->len-1;
        }
      }

  /* we don't need preconarr any more, so ditch it */
  free(preconarr);
  }	/* end of if(preconstruct) */
}


void die(char *str)
{
fprintf(stderr,"zblast: couldn't %s.\n",str);
exit(1);
}


struct preconstruct *
find_precon(struct note *nptr,struct preconstruct *preconarr,int preconlen)
{
int f;

for(f=0;f<preconlen;f++)
  if(preconarr[f].notenum==nptr->notenum && 
     preconarr[f].sustain==nptr->sustain && 
     preconarr[f].release==nptr->release && 
     preconarr[f].vol    ==nptr->vol && 
     preconarr[f].sample ==nptr->sample) return(preconarr+f);

return(NULL);	/* couldn't find it */
}


/* finally, play the music.
 *
 * I originally had play(), addnotes(), gensample() and output_sample()
 * as one big function, but it became a bit confusing to work on.
 *
 * Since gensample() is a stunningly major hotspot (tens of millions of
 * calls are likely), you should compile with -finline-functions (assuming
 * gcc is being used).
 */
void play_sample()
{
if(notesp==0) addnotes(line,noteslot);

      for(note=0;note<=last_active;)
        if(active[note]==NULL)
          memmove(active+note,active+note+1,
            sizeof(active[0])*(last_active---note));	/* wahay :-) */
        else
          note++;

      count=0;
      
      if(!realstereo)
        {
        /* mono or pseudostereo is "real-time", i.e. generate then play */
        for(note=0,aptr=active;note<=last_active;note++,aptr++)
          {
          count+=*((nptr=*aptr)->p.s8.preptr);
          if(++(nptr->p.s8.preptr)>nptr->p.s8.premax) *aptr=NULL;
          }
  
        if(line==last_line+1 && last_active==-1) done=1;
        output_sample(count);
        }
      else
        {
        /* real stereo is tricky, as it works by delaying sounds
         * on the left or right channels to model the delay you get
         * in the real world when sounds originate at different places.
         * this delay is up to 16*vsr/8000. We need to incrementally write
         * (once for each gensample!) to a buffer at least this long.
         */
        for(note=0,aptr=active;note<=last_active;note++,aptr++)
          {
          pos=(nptr=*aptr)->pos;
          count=*(nptr->p.s8.preptr);
          gen_stereo(pos,count);
          if(++(nptr->p.s8.preptr)>nptr->p.s8.premax) *aptr=NULL;
          }
        
        if(line==last_line+1 && last_active==-1) done=1;
        
        output_sample(rstereobuf_l[rstereopos]);	/* left */
        output_sample(rstereobuf_r[rstereopos]);	/* right */
        
        rstereobuf_l[rstereopos]=rstereobuf_r[rstereopos]=0;
        rstereopos++; if(rstereopos>=1024) rstereopos=0;
        }
      
      sampletick++;
      if(last_active>maxchan) maxchan=last_active;

/* advance (what were) loops, etc. */
notesp++;
if(notesp>=slotlen)
  {
  notesp=0;
  noteslot++;
  if(noteslot>=bsize || done!=0)
    {
    noteslot=0;
    line++;
    if(line>last_line+1 || done!=0)
      play_done=1;
    }
  }
}


int addnotes(int line,int noteslot)
{
int lp,pat,nf,nf2;
int tmp,tmp2;

for(lp=0;lp<MAX_LINE_DEPTH;lp++)
  if(patterns[lines[line][lp]].notes[noteslot].vol==-1) return(1);

for(lp=0;lp<MAX_LINE_DEPTH;lp++)
  {
  if((pat=lines[line][lp])!=0)
    {
    /* add all new notes to be played at this notespace */
    if(patterns[pat].notes[noteslot].vol!=0)
      {
      if(last_active>=MAX_NOTES)
        fprintf(stderr,"zblast: too many simultaneous notes!\n");
      else
        {
        nf=++last_active;
        active[nf]=&patterns[pat].notes[noteslot];
        active[nf]->offset=0;
        active[nf]->counter=0;
        if(preconstruct)
          active[nf]->p.s8.preptr=active[nf]->p.s8.predata;

        switch(active[nf]->mode)
          {
          case PIANO_MODE:
            /* turn off prev. notes of this pattern which have = notenum */
            tmp=active[nf]->notenum;
            tmp2=active[nf]->pattern;
            for(nf2=0;nf2<last_active;nf2++)
              if(active[nf2]!=NULL &&
                 active[nf2]->notenum==tmp && active[nf2]->pattern==tmp2)
                active[nf2]=NULL;
            break;
          
          case RETRIGGER_MODE:
            /* turn off prev. notes of this pattern no matter what */
            tmp2=active[nf]->pattern;
            for(nf2=0;nf2<last_active;nf2++)
              if(active[nf2]!=NULL && active[nf2]->pattern==tmp2)
                active[nf2]=NULL;

/*          default: */
            /* nothing needs doing for multi mode */
          }
        }
      }
    }
  }

return(0);
}


/* the big hotspot.
 * suggestions for improving the speed of this routine are most welcome.
 */
static inline int gensample(int note,struct note *nptr)
{
int val;
int sub=FIX_FRAC(nptr->offset);
signed char *ptr=(nptr->data+FIX_DOWN(nptr->offset));
val=((*ptr)*(1024-sub)+(*(ptr+1))*sub)*nptr->vol/102400;

if(nptr->counter>=nptr->sustain)
  {
  /* scale to account for the decay */
  val*=1024-FIX_UP(nptr->counter-nptr->sustain)/nptr->relsus;
  val/=1024;
  }

nptr->offset+=nptr->gap;
nptr->counter++;

if(nptr->offset>=nptr->len || nptr->counter>=nptr->release)
  active[note]=NULL;

return(val);
}


/* add val to stereo buffers */
static inline void gen_stereo(int pos,int val)
{
/* add val, correctly delayed on either left or right buffer,
 * to add the stereo positioning.
 */

if(pos<0)
  {
  rstereobuf_l[rstereopos]+=val;
  rstereobuf_r[(rstereopos-pos)%1024]+=val;
  }
else
  {
  rstereobuf_l[(rstereopos+pos)%1024]+=val;
  rstereobuf_r[rstereopos]+=val;
  }
}


void output_sample(int count)
{
count+=sample_midval;
if(count>sample_maxval) count=sample_maxval;
if(count<0) count=0;

/* 8-bit output */
write_sample(count);
}


void save_sod2_context(struct sod2_context *context)
{
context->bsize=bsize;
context->tempo=tempo;
context->last_pattern=last_pattern;
context->last_line=last_line;
context->slotlen=slotlen;

/* XXX this probably assumes all kinds of things about non-aligned
 * access, so change for portability at some point
 */
memcpy(context->samples,samples,sizeof(samples));
memcpy(context->patterns,patterns,sizeof(patterns));
memcpy(context->lines,lines,sizeof(lines));
}


void restore_sod2_context(struct sod2_context *context)
{
bsize=context->bsize;
tempo=context->tempo;
last_pattern=context->last_pattern;
last_line=context->last_line;
slotlen=context->slotlen;

/* XXX this probably assumes all kinds of things about non-aligned
 * access, so change for portability at some point
 */
memcpy(samples,context->samples,sizeof(samples));
memcpy(patterns,context->patterns,sizeof(patterns));
memcpy(lines,context->lines,sizeof(lines));
}

#endif /* MUSIC_SUPPORT */
