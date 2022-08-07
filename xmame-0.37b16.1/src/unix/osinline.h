#ifndef __OSINLINE__
#define __OSINLINE__

/* for uclock() */
#include "sysdep/misc.h"

#define osd_cycles() uclock()

#if defined svgalib || defined x11 || defined ggi || defined openstep || defined SDL
extern unsigned char *dirty_lines;
extern unsigned char **dirty_blocks;

#define osd_mark_vector_dirty(x,y) \
{ \
   dirty_lines[(y)>>3] = 1; \
   dirty_blocks[(y)>>3][(x)>>3] = 1; \
}

#else
#define osd_mark_vector_dirty(x,y)
#endif


#define clip_short _clip_short
#define _clip_short(x) { int sign = x >> 31; if (sign != (x >> 15)) x = sign ^ ((1 << 15) - 1); }

#define clip_short_ret _clip_short_ret
INLINE int _clip_short_ret(int x) { _clip_short(x); return x; }


#define mix_sample _mix_sample
#define _mix_sample(dst,src) \
    __asm__ __volatile__ \
	( " mov %2, %2, asr #1 \n" \
	" add %0, %1, %2 , asr #1 \n" \
	: "=r" (dst) \
	: "r"  (src),"r" (dst) \
	)

#endif /* __OSINLINE__ */
