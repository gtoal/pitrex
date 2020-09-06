/* sod2, a player for polychannel .csf music files.
 * Copyright (C) 1995 Russell Marks. See sod2.c for license details.
 *
 * sod2.h - header for sod2.c
 */


#include "sod2_config.h"


/* note modes */

#define PIANO_MODE	0
#define MULTI_MODE	1
#define RETRIGGER_MODE	2

/* defaults for grokfile.c:
 * You shouldn't change them, as .csf files are likely to depend
 * on these values.
 */

#define DEFAULTMODE	PIANO_MODE
#define DEFAULTVOLUME	35
#define OCTAVEFIX	2


typedef unsigned int fixed;

/* move from int to fixed (x1024) */
#define FIX_UP(x)	((x)<<10)

/* move from double/float to fixed */
#define FIX_DBL(x)	((fixed)((x)*1024.0+0.5))

/* move from fixed to int */
#define FIX_DOWN(x)	((x)>>10)

/* get 'fractional' part */
#define FIX_FRAC(x)	((x)&1023)

/* pattern - a few bars of notes (of size 'blocksize').
 * line - one or more patterns played simultaneously.
 */

struct sample {
  signed char *data;
  int rate;
  int len;
  };

/* NB: all timings in struct note are in fractions of bytes */
struct note {
  int notenum;		/* note number (not used while playing) */
  fixed offset;		/* offset through sample playback */
  fixed gap;		/* next-sample gap along sample */
  int counter;		/* incr'd once per sample gen'd */
  fixed sustain,release; /* for enveloping - offsets from start of note */
  fixed relsus;		/* release minus sustain */
  int pos;		/* stereo position; -16 (left) ... +16 (right) */
  int vol;		/* volume - 0<=vol<=100 */
  int sample;		/* idx in samples[] of sample used by note */
  signed char *data;	/* ptr to sample data */
  fixed len;		/* sample len x1024 */
  int pattern;		/* these are both duplicated here... */
  int mode;		/* ...for quick reference while playing */
  
  /* preconstruct stuff */
  union {
    /* 8-bit version */
    struct {
      signed char *predata; /* preconstructed sample data ptr */
      signed char *premax;  /* pointer to last byte of above */
      signed char *preptr;  /* copy of predata incr'd when playing */
      } s8;
    /* 16-bit version */
    struct {
      signed short *predata;
      signed short *premax;
      signed short *preptr;
      } s16;
    } p;
  };

struct pattern {
  struct note notes[MAX_BLOCKSIZE];
  int mode;
  };


/* this struct is for the pre-resampled samples of certain
 * voice/sample-rate/note-num/envelope/volume.
 */
struct preconstruct {
  int notenum;		/* note number */
  fixed sustain,release; /* enveloping */
  int vol;		/* volume */
  int sample;		/* idx in samples[] of sample used by note */
  union {
    signed char *p8;	/* ptr to pre-resampled 8-bit sample data */
    signed short *p16;	/* or, ptr to pre-resampled 16-bit sample data */
    } data;
  int len;		/* length of above data, in bytes */
  struct note *exnote;	/* 'example' of this note */
  };


extern void die(char *str);


extern int sample_maxval,sample_midval,vsr,psr;
extern int oversample;
extern struct pattern patterns[];
extern struct sample samples[];
extern struct note *active[];
extern int bsize,tempo;
extern int next_sample,last_pattern,last_line;
extern int verbose;
extern int buf_fd[];
extern int realstereo;


/* extra for zblast: save/restore context */

struct sod2_context {
  int bsize,tempo,last_pattern,last_line,slotlen;
  struct sample samples[MAX_SAMPLES];
  struct pattern patterns[MAX_PATTERNS];
  int lines[MAX_LINES][MAX_LINE_DEPTH];
  };

extern void save_sod2_context(struct sod2_context *context);
extern void restore_sod2_context(struct sod2_context *context);

/* need these to allow output via zblast's sound stuff */
extern void play_set_rate(int rate);
extern void play_init(char *fname,void (*output_func)(int c));
extern void play_init_existing();
extern void play_sample();
