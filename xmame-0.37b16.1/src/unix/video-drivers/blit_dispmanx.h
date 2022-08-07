/* this routine is the generic blit routine used in many cases, trough a number
   of defines it can be customised for specific cases.
   Currently recognised defines:
   DEST		ptr of type DEST_PIXEL to which should be blitted, if this is
		not defined only PUT_IMAGE is called if defined.
   DEST_PIXEL	type of the buffer to which is blitted, only needed if
                DEST is defined.
   DEST_WIDTH   Width of the destination buffer in pixels! Only needed if
                DEST is defined.
   SRC_PIXEL    type of the buffer from which is blitted, currently
                8 bpp (unsigned char) and 16 bpp (unsigned short) are supported.
   PUT_IMAGE    This function is called to update the parts of the screen
		which need updating. This is only called if defined.
   INDIRECT     This needs to be defined if DEST_PIXEL != unsigned char,
                this is a ptr to a list of pixels/colormappings for the
                colordepth conversion.
   BLIT_16BPP_HACK This one speaks for itself, it's a speedup hack for 8bpp
                to 16bpp blits.
   PACK_BITS    Write to packed 24bit pixels, DEST_PIXEL must be 32bits and
                INDIRECT must be on.
   DOUBLEBUFFER First copy each line to a buffer called doublebuffer_buffer,
                then do a memcpy to the real destination. This speeds up
                scaling when writing directly to framebuffer since it
                tremendously speeds up the reads done to copy one line to
                the next.
   
   This routines use long copy's so everything should always be long aligned.
*/

// For maximum speed, Raspberry Pi only does 1x1 processing and no scanlines

//switch (heightscale | (widthscale << 8) | (use_scanlines << 16))
{
/* 1x1 */

/*
 * Define the COPY routine
 * 
 * Can being either:
 *   INDIRECT;			 lookup 8-bit index and store 16-bit pixel
 *   DIRECT;			 copy buffer verbatim
 */

#ifdef INDIRECT

#define COPY_LINE2(SRC, END, DST) \
   SRC_PIXEL  *src = SRC; \
   SRC_PIXEL  *end = END; \
   DEST_PIXEL *dst = DST; \
   for(;src<end;src+=8,dst+=8) \
   { \
      *(dst  ) = INDIRECT[*(src  )]; \
      *(dst+1) = INDIRECT[*(src+1)]; \
      *(dst+2) = INDIRECT[*(src+2)]; \
      *(dst+3) = INDIRECT[*(src+3)]; \
      *(dst+4) = INDIRECT[*(src+4)]; \
      *(dst+5) = INDIRECT[*(src+5)]; \
      *(dst+6) = INDIRECT[*(src+6)]; \
      *(dst+7) = INDIRECT[*(src+7)]; \
   }
#else  /* not indirect */
#define COPY_LINE2(SRC, END, DST) \
   memcpy(DST, SRC, ((END)-(SRC))*sizeof(DEST_PIXEL));
#endif /* indirect */

#define SCALE_X(X) (X)
#define SCALE_X_8(X) ((X)<<3)
#define SCALE_Y(Y) (Y)
#define SCALE_Y_8(Y) ((Y)<<3)

/* 1x1 we don't do scanlines with 1x1 */
#define COPY_LINE(SRC, END, DST) { COPY_LINE2(SRC, END, DST) }
#include "blit_core.h"
#undef COPY_LINE

#undef SCALE_Y_8
#undef SCALE_Y
#undef SCALE_X_8
#undef SCALE_X

#undef COPY_LINE2

}

