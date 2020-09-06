#include <stdint.h>

#include "edac.h"
#include "e6522.h"

#include "e6809.h"

extern M6809 CPU;
DACVec DAC;

void(*dac_add_line) (int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color);

/* update the various analog values when orb is written. */

void dac_update(void)
{
	switch (VIA.orb & 0x06)
	{
	case 0x00:
		DAC.jsh = DAC.jch0;

		if ((VIA.orb & 0x01) == 0x00)
		{
			/* demultiplexor is on */
			DAC.ysh = DAC.xsh;
		}
		break;
	case 0x02:
		DAC.jsh = DAC.jch1;

		if ((VIA.orb & 0x01) == 0x00)
		{
			/* demultiplexor is on */
			DAC.rsh = DAC.xsh;
		}
		break;
	case 0x04:
		DAC.jsh = DAC.jch2;

		if ((VIA.orb & 0x01) == 0x00)
		{
			/* demultiplexor is on */
			if (DAC.xsh > 0x80)
			{
				DAC.zsh = DAC.xsh - 0x80;
			}
			else
			{
				DAC.zsh = 0;
			}
		}
		break;
	case 0x06:
		/* sound output line */
		DAC.jsh = DAC.jch3;
		break;
	}

	/* compare the current joystick direction with a reference */
	if (DAC.jsh > DAC.xsh)
	{
		DAC.compare = 0x20;
	}
	else
	{
		DAC.compare = 0;
	}

	/* compute the new "deltas" */
	DAC.dx = (int32_t)DAC.xsh - (int32_t)DAC.rsh;
	DAC.dy = (int32_t)DAC.rsh - (int32_t)DAC.ysh;
}

/* perform a single cycle worth of analog emulation */
extern int32_t cycleCount;
int32_t lineStartCycle;

void dac_sstep(void)
{
	int32_t sig_dx=0, sig_dy=0;
	uint8_t sig_ramp=0;
	uint8_t sig_blank=0;

	if ((VIA.acr & 0x10) == 0x10)
	{
		sig_blank = VIA.cb2s;
	}
	else
	{
		sig_blank = VIA.cb2h;
	}

	if (VIA.ca2 == 0)
	{
		/* need to force the current point to the 'orgin' so just
		* calculate distance to origin and use that as dx,dy.
		*/
		sig_dx = DAC_MAX_X / 2 - DAC.curr_x;
		sig_dy = DAC_MAX_Y / 2 - DAC.curr_y;
	}
	else
	{
		if (VIA.acr & 0x80)
		{
			sig_ramp = VIA.t1pb7;
		}
		else
		{
			sig_ramp = VIA.orb & 0x80;
		}

		// no RAMP CHANgES the dx
//if (sig_ramp == 0)
{
		sig_dx = DAC.dx;
		sig_dy = DAC.dy;
}
	}

	if (DAC.vectoring == 0)
	{
		if (sig_blank == 1 && ((DAC.zsh &0x80) ==0) && ((DAC.zsh &0x7f) !=0) &&
			DAC.curr_x >= 0 && DAC.curr_x < DAC_MAX_X &&
			DAC.curr_y >= 0 && DAC.curr_y < DAC_MAX_Y)
		{
			lineStartCycle = cycleCount;
			/* start a new vector */
			DAC.vectoring = 1;
			DAC.vector_x0 = DAC.curr_x;
			DAC.vector_y0 = DAC.curr_y;
			DAC.vector_x1 = DAC.curr_x;
			DAC.vector_y1 = DAC.curr_y;
			DAC.vector_dx = sig_dx;
			DAC.vector_dy = sig_dy;
			DAC.vector_color = (uint8_t)DAC.zsh;
		}
	}
	else
	{
		/* already drawing a vector ... check if we need to turn it off */
		if ((sig_blank == 0)|| ((DAC.zsh &0x80) !=0) || ((DAC.zsh &0x7f) ==0))
		{
			/* blank just went on, vectoring turns off, and we've got a
			* new line.
			*/
			DAC.vectoring = 0;
			// 16 seems to be a drawVLp thing... dunno
				dac_add_line(DAC.vector_x0, DAC.vector_y0, DAC.vector_x1, DAC.vector_y1, DAC.vector_color);
		}
		else if (sig_dx != DAC.vector_dx || sig_dy != DAC.vector_dy || (uint8_t)DAC.zsh != DAC.vector_color)
		{
			/* the parameters of the vectoring processing has changed.
			* so end the current line.
			*/
				dac_add_line(DAC.vector_x0, DAC.vector_y0, DAC.vector_x1, DAC.vector_y1, DAC.vector_color);
			/* we continue vectoring with a new set of parameters if the
			* current point is not out of limits.
			*/
			if (DAC.curr_x >= 0 && DAC.curr_x < DAC_MAX_X && DAC.curr_y >= 0 && DAC.curr_y < DAC_MAX_Y)
			{
  			lineStartCycle = cycleCount;
				DAC.vector_x0 = DAC.curr_x;
				DAC.vector_y0 = DAC.curr_y;
				DAC.vector_x1 = DAC.curr_x;
				DAC.vector_y1 = DAC.curr_y;
				DAC.vector_dx = sig_dx;
				DAC.vector_dy = sig_dy;
				DAC.vector_color = (uint8_t)DAC.zsh;
			}
			else
			{
				DAC.vectoring = 0;
			}
		}
	}
	if ((sig_ramp == 0) || (VIA.ca2 == 0))
	{
		DAC.curr_x += sig_dx;
		DAC.curr_y += sig_dy;
	}


	if (DAC.vectoring == 1 &&
		DAC.curr_x >= 0 && DAC.curr_x < DAC_MAX_X &&
		DAC.curr_y >= 0 && DAC.curr_y < DAC_MAX_Y)
	{

		/* we're vectoring ... current point is still within limits so
		* extend the current vector.
		*/
		DAC.vector_x1 = DAC.curr_x;
		DAC.vector_y1 = DAC.curr_y;
	}
}

void dac_reset(void)
{
	DAC.rsh = 128;
	DAC.xsh = 128;
	DAC.ysh = 128;
	DAC.zsh = 0;
	DAC.jch0 = 128;
	DAC.jch1 = 128;
	DAC.jch2 = 128;
	DAC.jch3 = 128;
	DAC.jsh = 128;

	DAC.compare = 0; /* check this */

	DAC.dx = 0;
	DAC.dy = 0;
	DAC.curr_x = DAC_MAX_X / 2;
	DAC.curr_y = DAC_MAX_Y / 2;

	DAC.vectoring = 0;
}
