#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "e8910.h"

/***************************************************************************

  ay8910.c


  Emulation of the AY-3-8910 / YM2149 sound chip.

  Based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.

***************************************************************************/

#define STEP2 length
#define STEP  2

AY8910 PSG;

uint16_t vol_table[32] = { 0, 23, 27, 33, 39, 46, 55, 65, 77, 92, 109, 129, 154, 183, 217, 258, 307,365, 434, 516, 613, 728, 865, 1029, 1223, 1453, 1727, 2052, 2439, 2899, 3446, 4095 };

enum
{
	SOUND_FREQ = 22050,
	SOUND_SAMPLE = 1024,
	TUNEA = 1, /* tuning muliplayer */
	TUNEB = 2, /* tuning divider */

	MAX_OUTPUT = 0x0fff,

	/* register id's */
	AY_AFINE = 0,
	AY_ACOARSE = 1,
	AY_BFINE = 2,
	AY_BCOARSE = 3,
	AY_CFINE = 4,
	AY_CCOARSE = 5,
	AY_NOISEPER = 6,
	AY_ENABLE = 7,
	AY_AVOL = 8,
	AY_BVOL = 9,
	AY_CVOL = 10,
	AY_EFINE = 11,
	AY_ECOARSE = 12,
	AY_ESHAPE = 13,

	AY_PORTA = 14,
	AY_PORTB = 15
};

void e8910_write(uint8_t r, uint8_t v)
{
	int32_t old;

	PSG.regs[r] = v;

	/* A note about the period of tones, noise and envelope: for speed reasons,*/
	/* we count down from the period to 0, but careful studies of the chip     */
	/* output prove that it instead counts up from 0 until the counter becomes */
	/* greater or equal to the period. This is an important difference when the*/
	/* program is rapidly changing the period to modulate the sound.           */
	/* To compensate for the difference, when the period is changed we adjust  */
	/* our internal counter.                                                   */
	/* Also, note that period = 0 is the same as period = 1. This is mentioned */
	/* in the YM2203 data sheets. However, this does NOT apply to the Envelope */
	/* period. In that case, period = 0 is half as period = 1. */
	switch (r)
	{
	case AY_AFINE:
	case AY_ACOARSE:
		PSG.regs[AY_ACOARSE] &= 0x0f;
		old = PSG.per_a;
		PSG.per_a = ((PSG.regs[AY_AFINE] + 256 * PSG.regs[AY_ACOARSE]) * TUNEA) / TUNEB;
		if (PSG.per_a == 0) PSG.per_a = 1;
		PSG.cnt_a += PSG.per_a - old;
		if (PSG.cnt_a <= 0) PSG.cnt_a = 1;
		break;
	case AY_BFINE:
	case AY_BCOARSE:
		PSG.regs[AY_BCOARSE] &= 0x0f;
		old = PSG.per_b;
		PSG.per_b = ((PSG.regs[AY_BFINE] + 256 * PSG.regs[AY_BCOARSE]) * TUNEA) / TUNEB;
		if (PSG.per_b == 0) PSG.per_b = 1;
		PSG.cnt_b += PSG.per_b - old;
		if (PSG.cnt_b <= 0) PSG.cnt_b = 1;
		break;
	case AY_CFINE:
	case AY_CCOARSE:
		PSG.regs[AY_CCOARSE] &= 0x0f;
		old = PSG.per_c;
		PSG.per_c = ((PSG.regs[AY_CFINE] + 256 * PSG.regs[AY_CCOARSE]) * TUNEA) / TUNEB;
		if (PSG.per_c == 0) PSG.per_c = 1;
		PSG.cnt_c += PSG.per_c - old;
		if (PSG.cnt_c <= 0) PSG.cnt_c = 1;
		break;
	case AY_NOISEPER:
		PSG.regs[AY_NOISEPER] &= 0x1f;
		old = PSG.per_n;
		PSG.per_n = (PSG.regs[AY_NOISEPER] * TUNEA) / TUNEB;
		if (PSG.per_n == 0) PSG.per_n = 1;
		PSG.cnt_n += PSG.per_n - old;
		if (PSG.cnt_n <= 0) PSG.cnt_n = 1;
		break;
	case AY_ENABLE:
		break;
	case AY_AVOL:
		PSG.regs[AY_AVOL] &= 0x1f;
		PSG.env_a = PSG.regs[AY_AVOL] & 0x10;
		PSG.vol_a = PSG.env_a ? PSG.vol_e : vol_table[PSG.regs[AY_AVOL] ? PSG.regs[AY_AVOL] * 2 + 1 : 0];
		break;
	case AY_BVOL:
		PSG.regs[AY_BVOL] &= 0x1f;
		PSG.env_b = PSG.regs[AY_BVOL] & 0x10;
		PSG.vol_b = PSG.env_b ? PSG.vol_e : vol_table[PSG.regs[AY_BVOL] ? PSG.regs[AY_BVOL] * 2 + 1 : 0];
		break;
	case AY_CVOL:
		PSG.regs[AY_CVOL] &= 0x1f;
		PSG.env_c = PSG.regs[AY_CVOL] & 0x10;
		PSG.vol_c = PSG.env_c ? PSG.vol_e : vol_table[PSG.regs[AY_CVOL] ? PSG.regs[AY_CVOL] * 2 + 1 : 0];
		break;
	case AY_EFINE:
	case AY_ECOARSE:
		old = PSG.per_e;
		PSG.per_e = ((PSG.regs[AY_EFINE] + 256 * PSG.regs[AY_ECOARSE])) * 1;
		if (PSG.per_e == 0) PSG.per_e = 1;
		PSG.cnt_e += PSG.per_e - old;
		if (PSG.cnt_e <= 0) PSG.cnt_e = 1;
		break;
	case AY_ESHAPE:
		/* envelope shapes:
		C AtAlH
		0 0 x x  \___

		0 1 x x  /___

		1 0 0 0  \\\\

		1 0 0 1  \___

		1 0 1 0  \/\/
				  ___
		1 0 1 1  \

		1 1 0 0  ////
				  ___
		1 1 0 1  /

		1 1 1 0  /\/\

		1 1 1 1  /___

		The envelope counter on the AY-3-8910 has 16 steps. On the YM2149 it
		has twice the steps, happening twice as fast. Since the end result is
		just a smoother curve, we always use the YM2149 behaviour.
		*/
		PSG.regs[AY_ESHAPE] &= 0x0f;
		PSG.attack = (PSG.regs[AY_ESHAPE] & 0x04) ? 0x1f : 0x00;
		if ((PSG.regs[AY_ESHAPE] & 0x08) == 0)
		{
			/* if Continue = 0, map the shape to the equivalent one which has Continue = 1 */
			PSG.hold = 1;
			PSG.alternate = PSG.attack;
		}
		else
		{
			PSG.hold = PSG.regs[AY_ESHAPE] & 0x01;
			PSG.alternate = PSG.regs[AY_ESHAPE] & 0x02;
		}
		PSG.cnt_e = PSG.per_e;
		PSG.cnt_env = 0x1f;
		PSG.holding = 0;
		PSG.vol_e = vol_table[PSG.cnt_env ^ PSG.attack];
		if (PSG.env_a) PSG.vol_a = PSG.vol_e;
		if (PSG.env_b) PSG.vol_b = PSG.vol_e;
		if (PSG.env_c) PSG.vol_c = PSG.vol_e;
		break;
	case AY_PORTA:
		break;
	case AY_PORTB:
		break;
	}
}


void e8910_reset(void)
{
	for (uint8_t r = 0; r < 16; r++)
		e8910_write(r, 0);

	/* input buttons */
	e8910_write(14, 0xff);
}

void e8910_init(void)
{
	PSG.RNG = 1;
	PSG.out_a = 0;
	PSG.out_b = 0;
	PSG.out_c = 0;
	PSG.out_n = 0xff;



}

void e8910_done(void)
{
}
