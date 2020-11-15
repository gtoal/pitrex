#ifdef FOURBANKS
// The code that is conditionally included if compiled with -DFOURBANKS
// is *under development* and is intended to support Vectorblade at
// some point...  IT DOES NOT CURRENTLY WORK.

#include <stdio.h> // for debugging bankswitch
#endif

#include <stdint.h>
#include <pitrex/pitrexio-gpio.h>
#include "e6522.h"

#ifdef FOURBANKS
#include "vecx.h" // for bank
#endif

VIA6522 VIA;

uint8_t(*via_read8_port_a) ();
uint8_t(*via_read8_port_b) ();
void(*via_write8_port_a) (uint8_t data);
void(*via_write8_port_b) (uint8_t data);

/* update IRQ and bit-7 of the ifr register after making an adjustment to
 * ifr.  Note that irq_i == (VIA.ifr & 0x80)
 */

inline static void int_update(void)
{
#ifdef FOURBANKS
  int oldbank = bank;
        // This may be where we pick up the IRQ bit for handling 4-bank switching
        // for vectorblade.  Update IRQ bit of 'bank' variable here?
#endif
	if ((VIA.ifr & 0x7f) & (VIA.ier & 0x7f))
	{
		VIA.ifr |= 0x80;
#ifdef FOURBANKS
		bank = (bank & ~2) | 2;
#endif
	}
	else
	{
		VIA.ifr &= 0x7f;
#ifdef FOURBANKS
		bank = (bank & ~2) | 0;
#endif
	}
#ifdef FOURBANKS
	if (bank != oldbank) fprintf(stderr, "IRQ: Switching bank from %d to %d\n", oldbank, bank);
#endif
}

#ifdef FOURBANKS
inline static void setPB6FromVectrex(uint8_t orb, uint8_t ddrb, int init)
{
  uint8_t npb6 = orb & ddrb & 0x40; // all output (0x40)
  uint8_t oldbank = bank;

  if ((ddrb & 0x40) == 0x00) npb6 |= 0x40; // all input (0x40)

  bank = (bank & ~1) | (npb6 ? 1 : 0);

  if (bank != oldbank) fprintf(stderr, "PB6: Switching bank from %d to %d\n", oldbank, bank);

  (void)init; // not sure what jsvecx did with this, if anything
}
#endif

inline uint8_t via_read(uint16_t address)
{
	uint8_t data = 0;

	switch (address & 0xf)
	{
	case 0x0:
		data = via_read8_port_b();
#ifdef FOURBANKS
		// Update PB6 bit of 'bank' variable here?
		// However this is a read.  Do we only do that on a write?
#endif
		break;
	case 0x1:
		/* register 1 also performs handshakes if necessary */
		if ((VIA.pcr & 0x0e) == 0x08)
		{
			/* if ca2 is in pulse mode or handshake mode, then it
			 * goes low whenever ira is read.
			 */
			VIA.ca2 = 0;
		}

		/* fall through */
	case 0xf:
		data = via_read8_port_a();
		break;
	case 0x2:
		data = (uint8_t)VIA.ddrb;
#ifdef FOURBANKS
		// Update PB6 bit of 'bank' variable here?
		// However this is a read.  Do we only do that on a write?
#endif
		break;
	case 0x3:
		data = (uint8_t)VIA.ddra;
		break;
	case 0x4:
		/* T1 low order counter */
		data = (uint8_t)VIA.t1c;
		VIA.ifr &= 0xbf; /* remove timer 1 interrupt flag */

		VIA.t1on = 0; /* timer 1 is stopped */
		VIA.t1int = 0;
		VIA.t1pb7 = 0x80;

		int_update();
		break;
	case 0x5:
		/* T1 high order counter */
		data = (uint8_t)(VIA.t1c >> 8);
		break;
	case 0x6:
		/* T1 low order latch */
		data = (uint8_t)VIA.t1ll;
		break;
	case 0x7:
		/* T1 high order latch */
		data = (uint8_t)VIA.t1lh;
		break;
	case 0x8:
		/* T2 low order counter */
		data = (uint8_t)VIA.t2c;
		VIA.ifr &= 0xdf; /* remove timer 2 interrupt flag */
		VIA.t2on = 0; /* timer 2 is stopped */
		VIA.t2int = 0;

		int_update();
		break;
	case 0x9:
		/* T2 high order counter */
		data = (uint8_t)(VIA.t2c >> 8);
		break;
	case 0xa:
		data = (uint8_t)VIA.sr;
		VIA.alternate = 1;
		VIA.ifr &= 0xfb; /* remove shift register interrupt flag */
		VIA.srb = 0;
		VIA.srclk = 1;

		int_update();
		break;
	case 0xb:
		data = (uint8_t)VIA.acr;
		break;
	case 0xc:
		data = (uint8_t)VIA.pcr;
		break;
	case 0xd:
		/* interrupt flag register */
		data = (uint8_t)VIA.ifr;
		break;
	case 0xe:
		/* interrupt enable register */
		data = (uint8_t)(VIA.ier | 0x80);
		break;
	}
	return data;
}

inline void via_write(uint16_t address, uint8_t data)
{
  switch (address & 0xf)
	{
	case 0x0:
		VIA.orb = data;
		via_write8_port_b(data);

		if ((VIA.pcr & 0xe0) == 0x80)
		{
			/* if cb2 is in pulse mode or handshake mode, then it
			 * goes low whenever orb is written.
			 */
			VIA.cb2h = 0;
		}
#ifdef FOURBANKS
	        // See http://vide.malban.de/18th-of-march-2018-vectorblade-iii
		//  - extracting PB6 bit is a little complicated - depends on
		//    whether ddrb is in input or output mode.
                setPB6FromVectrex(data, VIA.ddrb, 1); // update bankswitch
#endif
		break;
	case 0x1:
		/* register 1 also performs handshakes if necessary */
		if ((VIA.pcr & 0x0e) == 0x08)
		{
			/* if ca2 is in pulse mode or handshake mode, then it
			 * goes low whenever ora is written.
			 */

			VIA.ca2 = 0;
		}
		/* fall through */
	case 0xf:
		VIA.ora = data;
		via_write8_port_a(data);
		break;
	case 0x2:
		VIA.ddrb = data;
#ifdef FOURBANKS
                setPB6FromVectrex(VIA.orb, data, 0); // update bankswitch
#endif
		break;
	case 0x3:
		VIA.ddra = data;
		break;
	case 0x4:
		/* T1 low order counter */
		VIA.t1ll = data;
		break;
	case 0x5:
		/* T1 high order counter */
		VIA.t1lh = data;
		VIA.t1c = (VIA.t1lh << 8) | VIA.t1ll;
		VIA.ifr &= 0xbf; /* remove timer 1 interrupt flag */

		VIA.t1on = 1; /* timer 1 starts running */
		VIA.t1int = 1;
		VIA.t1pb7 = 0;

		int_update();
		break;
	case 0x6:
		/* T1 low order latch */
		VIA.t1ll = data;
		break;
	case 0x7:
		/* T1 high order latch */
		VIA.t1lh = data;
		break;
	case 0x8:
		/* T2 low order latch */
		VIA.t2ll = data;
		break;
	case 0x9:
		/* T2 high order latch/counter */
		VIA.t2c = (data << 8) | VIA.t2ll;
		VIA.ifr &= 0xdf;

		VIA.t2on = 1; /* timer 2 starts running */
		VIA.t2int = 1;

		int_update();
		break;
	case 0xa:
		VIA.alternate = 1;
		VIA.sr = data;
		VIA.ifr &= 0xfb; /* remove shift register interrupt flag */
		VIA.srb = 0;
		VIA.srclk = 1;

		int_update();
		break;
	case 0xb:
		VIA.acr = data;
		break;
	case 0xc:
		VIA.pcr = data;

		if ((VIA.pcr & 0x0e) == 0x0c)
		{
			/* ca2 is outputting low */
			VIA.ca2 = 0;
		}
		else
		{
			/* ca2 is disabled or in pulse mode or is
			 * outputting high.
			 */
			VIA.ca2 = 1;
		}

		if ((VIA.pcr & 0xe0) == 0xc0)
		{
			/* cb2 is outputting low */
			VIA.cb2h = 0;
		}
		else
		{
			/* cb2 is disabled or is in pulse mode or is
			 * outputting high.
			 */
			VIA.cb2h = 1;
		}

		break;
	case 0xd:
		/* interrupt flag register */
		VIA.ifr &= ~(data & 0x7f);
		int_update();
		break;
	case 0xe:
		/* interrupt enable register */
		if (data & 0x80)
			VIA.ier |= data & 0x7f;
		else
			VIA.ier &= ~(data & 0x7f);

		int_update();
		break;
	}
}

/* perform a single cycle worth of via emulation.
 * via_sstep0 is the first postion of the emulation.
 */

inline void via_sstep0(void)
{
	uint8_t t2shift;

	if (VIA.t1on)
	{
		VIA.t1c--;

		if ((VIA.t1c & 0xffff) == 0xffff)
		{
			/* counter just rolled over */
			if (VIA.acr & 0x40)
			{
				/* continuous interrupt mode */
				VIA.ifr |= 0x40;
				int_update();
				VIA.t1pb7 = 0x80 - VIA.t1pb7;

				/* reload counter */
				VIA.t1c = (VIA.t1lh << 8) | VIA.t1ll;
			}
			else
			{
				/* one shot mode */

				if (VIA.t1int)
				{
					VIA.ifr |= 0x40;
					int_update();
					VIA.t1pb7 = 0x80;
					VIA.t1int = 0;
				}
			}
		}
	}

	if (VIA.t2on && (VIA.acr & 0x20) == 0x00)
	{
		VIA.t2c--;

		if ((VIA.t2c & 0xffff) == 0xffff)
		{
			/* one shot mode */
			if (VIA.t2int)
			{
				VIA.ifr |= 0x20;
				int_update();
				VIA.t2int = 0;
			}
		}
	}

	/* shift counter */
	VIA.src--;

	if ((VIA.src & 0xff) == 0xff)
	{
		VIA.src = VIA.t2ll;

		if (VIA.srclk)
		{
			t2shift = 1;
			VIA.srclk = 0;
		}
		else
		{
			t2shift = 0;
			VIA.srclk = 1;
		}
	}
	else
	{
		t2shift = 0;
	}

	if (VIA.srb < 8)
	{
		switch (VIA.acr & 0x1c)
		{
		case 0x00:
			/* disabled */
			break;
		case 0x04:
			/* shift in under control of t2 */
			if (t2shift)
			{
				/* shifting in 0s since cb2 is always an output */

				VIA.sr <<= 1;
				VIA.srb++;
			}
			break;
		case 0x08:
			/* shift in under system clk control */
			VIA.sr <<= 1;
			VIA.srb++;
			break;
		case 0x0c:
			/* shift in under cb1 control */
			break;
		case 0x10:
			/* shift out under t2 control (free run) */
			if (t2shift)
			{
				VIA.cb2s = (VIA.sr >> 7) & 1;

				VIA.sr <<= 1;
				VIA.sr |= VIA.cb2s;
			}
			break;
		case 0x14:
			/* shift out under t2 control */
			if (t2shift)
			{
				VIA.cb2s = (VIA.sr >> 7) & 1;

				VIA.sr <<= 1;
				VIA.sr |= VIA.cb2s;
				VIA.srb++;
			}
			break;
		case 0x18:
			/* shift out under system clock control */
      VIA.alternate = !VIA.alternate;
      if (VIA.alternate)
			{
					VIA.cb2s = (VIA.sr >> 7) & 1;

					VIA.sr <<= 1;
					VIA.sr |= VIA.cb2s;
					VIA.srb++;
				}
			break;
		case 0x1c:
			/* shift out under cb1 control */
			break;
		}

		if (VIA.srb == 8)
		{
			VIA.ifr |= 0x04;
			int_update();
		}
	}
}

/* perform the second part of the via emulation */

inline void via_sstep1(void)
{
	if ((VIA.pcr & 0x0e) == 0x0a)
	{
		/* if ca2 is in pulse mode, then make sure
		 * it gets restored to '1' after the pulse.
		 */
		VIA.ca2 = 1;
	}

	if ((VIA.pcr & 0xe0) == 0xa0)
	{
		/* if cb2 is in pulse mode, then make sure
		 * it gets restored to '1' after the pulse.
		 */
		VIA.cb2h = 1;
	}
}

inline void via_reset(void)
{
#ifdef FOURBANKS
        bank = 0; // wild guess that it is needed here... (assuming IRQ not active)
                  // It's possible that we start up in bank 1 (default of PB6, No IRQ??)
	          // Perhaps this should be either bank = (3&bankmask) or bank = (1&bankmask) ?
#endif
	VIA.ora = 0;
	VIA.orb = 0;
	VIA.ddra = 0;
	VIA.ddrb = 0;
	VIA.t1on = 0;
	VIA.t1int = 0;
	VIA.t1c = 0;
	VIA.t1ll = 0;
	VIA.t1lh = 0;
	VIA.t1pb7 = 0x80;
	VIA.t2on = 0;
	VIA.t2int = 0;
	VIA.t2c = 0;
	VIA.t2ll = 0;
	VIA.sr = 0;
	VIA.srb = 8;
	VIA.src = 0;
	VIA.srclk = 0;
	VIA.acr = 0;
	VIA.pcr = 0;
	VIA.ifr = 0;
	VIA.ier = 0;
	VIA.ca2 = 1;
	VIA.cb2h = 1;
	VIA.cb2s = 0;
	VIA.alternate = 1;
}
