/* convxpm.c - XPM to bytemap convertor, based on zgv's readxpm.c.
 *
 * This has been cut down to simplify things, since we know (and
 * require) certain things of the XPMs we'll be converting.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "convxpm.h"


#define MAX_CPP	1

static struct colchars_tag	/* this is used to store colour char codes */
  {
  int idx;
  unsigned char r,g,b;
  unsigned char name[MAX_CPP+1];
  } *colchars=NULL;


/* prototypes */
unsigned char *next_token(unsigned char *ptr);
void get_token(unsigned char *token,int tokenmax,unsigned char *buf);




/* xpm_data is xpm array, palrgb is int array of r,g,b in range 0..255 */
unsigned char *xpm2bytemap(char *xpm_data[],int *palrgb)
{
static unsigned char smallcol2idx[256];
static unsigned char buf[128]; /* only used for magic/tokens so can be short */
static unsigned char colname[256];	/* temp. used to store colour name */
unsigned char *inpline;
int w,h,f,x,y,ncols,cpp;	/* cpp=chars/pixel */
unsigned char *bmap,*cptr,*ptr;
int cur_string=0;

/* get width/height etc. */
inpline=xpm_data[cur_string++];

if(sscanf(inpline,"%d %d %d %d",&w,&h,&ncols,&cpp)!=4)
  return(NULL);

if(w==0 || h==0 || cpp!=1)
  return(NULL);

/* colchars will usually be NULL at this point, but an error could
 * well have left it hanging around - nuke it if so.
 */
if(colchars!=NULL) free(colchars);

/* alloc colchars array */
if((colchars=malloc(ncols*sizeof(struct colchars_tag)))==NULL)
  return(NULL);


/* read in colours. This is REALLY FUN. :-(
 */

for(f=0;f<ncols;f++)
  {
  int r,g,b;
  
  inpline=xpm_data[cur_string++];
  
  memset(colchars[f].name,0,MAX_CPP+1);
  memcpy(colchars[f].name,inpline,cpp);
  colchars[f].idx=f;
  
  /* look for "c" token */
  cptr=inpline+cpp; *buf=0;
  while(*cptr && strcmp(buf,"c")!=0)
    {
    cptr=next_token(cptr);
    get_token(buf,sizeof(buf),cptr);
    }

  if(strcmp(buf,"c")!=0)
    return(NULL);
  
  cptr=next_token(cptr);
  
  /* so now cptr points to a colour name/number.
   * we assume it's a number.
   */
  get_token(colname,sizeof(colname),cptr);
  
  /* now work out what we've got. if it starts with a `#' it's RGB,
   * and if it's "None" it's the transparent `colour', otherwise it's a
   * colour name from rgb.txt.
   *
   * However, we also tolerate the old-style "#Transparent" for "None".
   */
  if(*colname=='#' && isxdigit(colname[1]))
    {
    /* RGB - length of rest must be 3, 6, 9, or 12. */
    int rgb;
    
    switch(strlen(colname+1))
      {
      case 3:
        rgb=strtol(colname+1,NULL,16);
        r=(rgb>>8)*16+(rgb>>8);
        g=((rgb>>4)&15)*16+((rgb>>4)&15);
        b=(rgb&15)*16+(rgb&15);
        break;
      
      case 6:
        rgb=strtol(colname+1,NULL,16);
        r=(rgb>>16);
        g=((rgb>>8)&255);
        b=(rgb&255);
        break;
      
      case 9:
        /* 0123456789
         * #rr.gg.bb.
         */
        colname[3]=colname[6]=colname[9]=0;
        r=strtol(colname+1,NULL,16);
        g=strtol(colname+4,NULL,16);
        b=strtol(colname+7,NULL,16);
        break;
      
      case 12:
        /* 0123456789012
         * #rr..gg..bb..
         */
        colname[3]=colname[7]=colname[11]=0;
        r=strtol(colname+1,NULL,16);
        g=strtol(colname+5,NULL,16);
        b=strtol(colname+9,NULL,16);
        break;
      
      default:
        return(NULL);
      }
    }
  else		/* not RGB number... */
    if(strcasecmp(colname,"None")==0 || strcasecmp(colname,"#Transparent")==0)
      {
      /* transparent `colour' */
      r=g=b=0;
      }
    else
      {
      /* lookup the colour name - unsupported. */
      return(NULL);
      }
  
  /* write colour to colchars[] in case we're producing 24-bit output */
  colchars[f].r=r;
  colchars[f].g=g;
  colchars[f].b=b;
  }


/* make code -> index lookup table. the disadvantage of
 * this is that we don't spot invalid colour codes, but it's so
 * much quicker this is probably worth it. The memsets do at least
 * ensure we give a consistent result (invalid codes give the
 * background colour (i.e. colour index 0)).
 */
memset(smallcol2idx,0,256);
for(f=0;f<ncols;f++)
  {
  int g;
  
  for(g=0;g<15;g++)
    if(colchars[f].r==palrgb[g*3] &&
       colchars[f].g==palrgb[g*3+1] &&
       colchars[f].b==palrgb[g*3+2])
      smallcol2idx[colchars[f].name[0]]=g;
  
  if(g==16)
    return(NULL);
  }


/* phew. finally dealt with the colours, let's try (gasp) reading the
 * bloody *picture*! :-)
 */

/* extra lines are in case we're dithering. */
if((bmap=malloc(w*h))==NULL)
  return(NULL);

ptr=bmap;

for(y=0;y<h;y++)
  {
  cptr=xpm_data[cur_string++];
  
  /* loop over line directly indexing in smallcol2idx */
  for(x=0;x<w;x++)
    *ptr++=smallcol2idx[*cptr++];
  }

free(colchars);
colchars=NULL;
  
return(bmap);
}


unsigned char *next_token(unsigned char *ptr)
{
while(*ptr!=0 && *ptr!=' ' && *ptr!='\t')
  ptr++;
while(*ptr==' ' || *ptr=='\t')
  ptr++;
return(ptr);
}


void get_token(unsigned char *token,int tokenmax,unsigned char *buf)
{
int n=0;

while(*buf!=0 && *buf!=' ' && *buf!='\t' && n<tokenmax-1)
  *token++=*buf++,n++;
*token=0;
}
