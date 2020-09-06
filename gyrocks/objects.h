#ifndef _objects_h_
#define _objects_h_

#include <stdint.h>

#define MAXPOINTS 61

typedef struct
{
	uint8_t count;
	int8_t points[MAXPOINTS*2]; // up to  xy points 
} objects_char_t;

#ifdef __cplusplus
extern "C" {
#endif

extern const objects_char_t gobjects[];

#ifdef __cplusplus
}
#endif

#endif
