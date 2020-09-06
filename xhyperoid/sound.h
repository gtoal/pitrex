/* sound.h */

/* virtual sound channels, used for mixing.
 * each can play at most one sample at a time.
 */
#define PSHOT_CHANNEL	0	/* player shot */
#define PTHRUST_CHANNEL 1
#define ASTEROID_CHANNEL 2	/* asteroid being hit */
#define BADDIE_CHANNEL	3	/* baddie being hit */
#define BSHOT_CHANNEL	4	/* baddie shot channel */
#define EFFECT_CHANNEL  5	/* effects like level end noises */

#define NUM_CHANNELS	6

/* other signals to sound player */
#define TURN_SOUND_ON	128
#define TURN_SOUND_OFF	129

#define KILL_SNDSERV	255	/* signal to sndserv to die */

/* sample offsets in sample[] */
#define PSHOT_SAMPLE		0
#define PTHRUST_SAMPLE		1
#define EXPLODE_SAMPLE		2
#define EXPLODE2_SAMPLE		3
#define BSHOT_SAMPLE		4
#define PHIT_SAMPLE		5
#define TITLE_SAMPLE		6
#define NEWBONUS_SAMPLE		7
#define NEWHUNT_SAMPLE		8
#define NEWSWARM_SAMPLE		NEWHUNT_SAMPLE
#define NEWSPIN_SAMPLE		NEWHUNT_SAMPLE
#define BONUSGOT_SAMPLE		9
#define BONUSSHOT_SAMPLE	EXPLODE_SAMPLE
#define BONUSTIMEOUT_SAMPLE	EXPLODE_SAMPLE
#define HUNTEXPLODE_SAMPLE	EXPLODE2_SAMPLE
#define SPINEXPLODE_SAMPLE	EXPLODE_SAMPLE
#define ROIDSPLIT_SAMPLE	EXPLODE_SAMPLE
#define ROIDNOSPLIT_SAMPLE	EXPLODE_SAMPLE
#define BADDIEWOUND_SAMPLE	10
#define SWARMSPLIT_SAMPLE	11
#define EXTRALIFE_SAMPLE	BONUSGOT_SAMPLE
#define NUM_SAMPLES		12


extern void queuesam(int chan,int sam);
extern void start_sound(void);
