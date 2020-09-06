/* zblast - simple shoot-em-up.
 * Copyright (C) 1993-2000 Russell Marks. See zblast.c for license.
 *
 * sod2_config.h is from:
 */
/* sod2, a player for polychannel .csf music files.
 * Copyright (C) 1995-1996 Russell Marks. See sod2.c for license details.
 *
 * config.h - array sizes and configuration.
 *
 * Please contact me if you think any of the limits here are too small.
 * Despite the need for limits, I would like to keep using arrays in
 * general to keep the access speed up.
 */

/************************** PATH SETTINGS ***************************/

/* This path fragment is always checked (last) whether SOD2_SAMPLE_PATH
 * is set or not.
 */
#define MINIMUM_SAMPLE_PATH	SOUNDSDIR


/************************* BUFFER SETTINGS **************************/

/* These are limits on, and configuration of, the buffer used
 * if _ALLOW_BUFFERING is defined. These settings are not used
 * unless buffering is allowed. You shouldn't need to change these.
 */

/* The buffer size to be malloc()'d. */
#define BUFFER_SIZE	2*1024*1024

/* Size of block to write to audio device. If you alter this you must
 * also alter BUF_FRAG_ARG accordingly. (Don't alter it, basically. :-))
 * 
 */
#define BUF_WRITE	8192

/* SNDCTL_DSP_SETFRAGMENT arg appropriate for above setting.
 * This setting gives two 8k fragments.
 * This and the former are doubled/quadrupled for stereo/16-bit
 * as necessary. This means that you must have a minimum sound
 * buffer size (as defined during the kernel's 'make config') of
 * 16k for mono, 32k for stereo or 16-bit, and 64k for 16-bit stereo.
 */
#define BUF_FRAG_ARG	0x2000D


/*********************** ARRAY SIZE SETTINGS ************************/

/* These settings set array sizes used by sod2 for storing patterns,
 * notes, etc. It is unlikely you will need to change these.
 */

/* Max pattern length.
 * Using more than 64 notes in a pattern is a *very* bad idea.
 * You should only use pattern lengths of less than 64 when
 * using different time signatures such as 6/8, 5/4, 7/8 etc.
 * Currently you can only use one pattern length per track,
 * though you can use ';' to kludge around this. See the csf(5)
 * man page for details.
 */
#define MAX_BLOCKSIZE	64

/* Max number of patterns.
 * This should be reasonable.
 */
#define MAX_PATTERNS	256

/* Max number of lines.
 * This may be a limitation:
 * At the default tempo of 125 bpm and default pattern size of 64 notes,
 * this makes the maximum track length about 32 mins 46 sec.
 */
#define MAX_LINES	256

/* Max patterns played at once.
 * This is much the same as a 64-track recorder. :-)
 */
#define MAX_LINE_DEPTH	64

/* Max notes played at once.
 * I doubt this will be much of a limitation either. :-)
 * Decreasing this won't make sod2 faster, as only array entries being
 * used are looked at.
 */
#define MAX_NOTES	2048

/* Max samples that can be loaded/used.
 * This is plenty unless you're doing something very weird.
 */
#define MAX_SAMPLES	256
