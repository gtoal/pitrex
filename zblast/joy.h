/* zblast - simple shoot-em-up.
 * Copyright (C) 1993-2000 Russell Marks. See zblast.c for license.
 *
 * joy.h
 */

/* these maximums implicit from ioctl defs */
#define MAX_AXES	256
#define MAX_BUTS	256

struct joystate_tag
  {
  unsigned char num_axes;
  unsigned char num_buts;
  int axis[MAX_AXES];
  int digital_axis[MAX_AXES];	/* these are -1, 0, or 1 */
  int but[MAX_BUTS];
  };

extern int joy_init(void);
extern struct joystate_tag *joy_update(void);
extern void joy_close(void);
