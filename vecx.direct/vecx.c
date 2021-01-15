// KARL QUAPPE e.g. does not work well!


/*
 Some rudimentary emulation of VIA is now implemented.
 This was needed to "know" the location a printStr is occuring.
 Without this - it would not be possible to use a shortcut!

 So Y, X position of the current "cursor" is calculated -> DAC
*/


#include <unistd.h> // for usleep
#include <stdint.h>

#include <pitrex/pitrexio-gpio.h>
#include <vectrex/vectrexInterface.h>
#include <vectrex/baremetalUtil.h>

#include "vecx.h"
#include "e6809.h"
#include "bios.i"

#ifdef AVOID_TICKS
unsigned char linuxIntDisabled=0;
unsigned int linuxIntGap=0;
#endif

uint8_t cart[64738];
uint8_t ram[1024];

/* the sound chip registers */
uint16_t delayVia = 0x0000;
uint8_t delayData = 0;
uint32_t cycleCount;
uint32_t mustWait = 0;



// only used for string print intercept
typedef struct
{
	/* the via 6522 registers */
	uint8_t ora, orb;
	uint8_t ddra, ddrb;
	uint8_t t1on;  /* is timer 1 on? */
	uint8_t t1int; /* are timer 1 interrupts allowed? */
	uint16_t t1c;
	uint8_t t1ll;
	uint8_t t1lh;
	uint8_t t1pb7; /* timer 1 controlled version of pb7 */
	uint8_t t2on;  /* is timer 2 on? */
	uint8_t t2int; /* are timer 2 interrupts allowed? */
	uint16_t t2c;
	uint8_t t2ll;
	uint8_t sr;
	uint8_t srb;   /* number of bits shifted so far */
	uint8_t src;   /* shift counter */
	uint8_t srclk;
	uint8_t acr;
	uint8_t pcr;
	uint8_t ifr;
	uint8_t ier;
	uint8_t ca2;
	uint8_t cb2h;  /* basic handshake version of cb2 */
	uint8_t cb2s;  /* version of cb2 controlled by the shift register */
uint8_t alternate;
} VIA6522;
VIA6522 VIA;

enum {
	DAC_MAX_X = 32767,//33000,
	DAC_MAX_Y = 32767,//41000
};

typedef struct
{
	/* analog devices */

	uint16_t rsh;  /* zero ref sample and hold */
	uint16_t xsh;  /* x sample and hold */
	uint16_t ysh;  /* y sample and hold */
	uint16_t zsh;  /* z sample and hold */
	uint16_t jch0;		  /* joystick direction channel 0 */
	uint16_t jch1;		  /* joystick direction channel 1 */
	uint16_t jch2;		  /* joystick direction channel 2 */
	uint16_t jch3;		  /* joystick direction channel 3 */
	uint16_t jsh;  /* joystick sample and hold */

	uint8_t compare;

	int32_t dx;     /* delta x */
	int32_t dy;     /* delta y */
	int32_t curr_x; /* current x position */
	int32_t curr_y; /* current y position */

	uint8_t vectoring; /* are we drawing a vector right now? */
	int32_t vector_x0;
	int32_t vector_y0;
	int32_t vector_x1;
	int32_t vector_y1;
	int32_t vector_dx;
	int32_t vector_dy;
	uint8_t vector_color;
} DACVec;

DACVec DAC;

enum
{
	VECTREX_PDECAY = 30,      /* phosphor decay rate */

	/* number of 6809 cycles before a frame redraw */

	FCYCLES_INIT = VECTREX_MHZ / VECTREX_PDECAY,

	/* max number of possible vectors that maybe on the screen at one time.
	 * one only needs VECTREX_MHZ / VECTREX_PDECAY but we need to also store
	 * deleted vectors in a single table
	 */

	VECTOR_MAX_CNT = VECTREX_MHZ / VECTREX_PDECAY,
};

size_t vector_draw_cnt;
vector_t vectors[VECTOR_MAX_CNT];

int32_t fcycles;

/* update the snd chips internal registers when VIA.ora/VIA.orb changes */
inline void dac_update(void)
{
	switch (VIA.orb & 0x06)
	{
	  case 0x00:
		  if ((VIA.orb & 0x01) == 0x00)
		  {
			  /* demultiplexor is on */
			  DAC.ysh = DAC.xsh;
		  }
		  break;
	  case 0x02:
		  if ((VIA.orb & 0x01) == 0x00)
		  {
			  /* demultiplexor is on */
			  DAC.rsh = DAC.xsh;
		  }
		  break;
	}

	/* compute the new "deltas" */
	DAC.dx = (int32_t)DAC.xsh - (int32_t)DAC.rsh;
	DAC.dy = (int32_t)DAC.rsh - (int32_t)DAC.ysh;
}

/* perform a single cycle worth of analog emulation */
inline void dac_sstep(void)
{
	int32_t sig_dx=0, sig_dy=0;
	uint8_t sig_ramp=0;

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
		sig_dx = DAC.dx;
		sig_dy = DAC.dy;
	}
	if ((sig_ramp == 0) || (VIA.ca2 == 0))
	{
		mustWait = 1;

		DAC.curr_x += sig_dx;
		DAC.curr_y += sig_dy;
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

inline void snd_update(void)
{
}

inline void int_update(void)
{
	if ((VIA.ifr & 0x7f) & (VIA.ier & 0x7f))
	{
		VIA.ifr |= 0x80;
	}
	else
	{
		VIA.ifr &= 0x7f;
	}
}
inline void write8_port_a(uint8_t data)
{
	/* output of port a feeds directly into the dac which then
	* feeds the x axis sample and hold.
	*/
	DAC.xsh = data ^ 0x80;
	dac_update();
}

inline void write8_port_b(uint8_t data)
{
	(void)data;
	dac_update();
}

inline void via_write(uint16_t address, uint8_t data)
{
  delayVia =address;
  delayData = data;
  mustWait = 1;

  switch (address & 0xf)
	{
	case 0x0:
		VIA.orb = data;
		write8_port_b(data);

		if ((VIA.pcr & 0xe0) == 0x80)
		{
			/* if cb2 is in pulse mode or handshake mode, then it
			 * goes low whenever orb is written.
			 */
			VIA.cb2h = 0;
		}
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
		write8_port_a(data);
		break;
	case 0x2:
		VIA.ddrb = data;
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
#ifdef AVOID_TICKS
		if (linuxIntGap == 0)
		 linuxIntGap = (VIA.t1ll + ST_GAP_END) * 0.6;//(VIA.t1c + SCALETOTAL_OFFSET) * DELAY_PI_CYCLE_EQUIVALENT;
#endif
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
#ifdef AVOID_TICKS
		if (data == 0)
		 linuxIntGap = 0;
#endif
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

static uint8_t read8(uint16_t address)
{
	uint8_t data = 0xff;

	if ((address & 0xe000) == 0xe000)
	{
		/* rom */
		data = rom[address & 0x1fff];
	}
	else if ((address & 0xe000) == 0xc000)
	{
		if (address & 0x800)
		{
			/* ram */
			data = ram[address & 0x3ff];
		}
		else if (address & 0x1000)
		{
		    /* io */
		    // theoretically insert a delay here
		    // up to the current emulated cycles in this
		    // CPU instruction
                    data =  GET(address);
                }

	}
	else if (address < 0x8000)
	{
		/* cartridge */
		data = cart[address];
	}

	return data;
}



static void write8(uint16_t address, uint8_t data)
{
	if ((address & 0xe000) == 0xe000)
	{
		/* rom */
	}
	else if ((address & 0xe000) == 0xc000)
	{
		/* it is possible for both ram and io to be written at the same! */

		if (address & 0x800)
		{
			ram[address & 0x3ff] = data;
		}

		if (address & 0x1000)
		{
		  via_write(address, data);
		}
	}
	else if (address < 0x8000)
	{
		/* cartridge */
	}
}

inline void via_sstep0(void)
{
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


void vecx_reset(void)
{
	/* ram */
	for (int r = 0; r < 1024; r++)
		ram[r] = (uint8_t)42;

	vector_draw_cnt = 0;
	fcycles = FCYCLES_INIT;

	e6809_read8 = read8;
	e6809_write8 = write8;

	e6809_reset();
	dac_reset();

	delayVia = 0x0000;
	delayData = 0;
	cycleCount = 0;
}

static char printbuffer[40];
void vecx_emu(int32_t cycles)
{
	while (cycles > 0)
	{
// 		mustWait = 0;
		uint16_t icycles;
		delayVia = 0x0000;
		delayData = 0;
		if (CPU.reg_pc == 0xF495) // printstr
		{
			int x = -1+ (DAC.curr_x-32768/2)/128;
			int y = -(1+ (DAC.curr_y-32768/2)/128);

			int address = CPU.reg_u;
			int counter=0;
			unsigned char c = read8(address++);
			while ((c & 0x80) != 0x80)
			{
			  if (counter < 39)
			    printbuffer[counter++] = c;
			  c = read8(address++);
			}
			printbuffer[counter++] = 0x80;

			//Vec_Text_Height EQU     $C82A   ;Default text height
			//Vec_Text_Width  EQU     $C82B   ;Default text width
			int cyclesemulated	=  v_printStringRaster(x, y, printbuffer, read8(0xC82B), read8(0xC82A), 0x80);

			cycles -= cyclesemulated;
			cycleCount += cyclesemulated;

			// Reset all VIA settings, so next postitions can be calculated ...
			SET (VIA_aux_cntl, 0x98); //Shift Reg. Enabled, T1 PB7 Enabled
			DELAY_CYCLES(2);
			SET(VIA_t1_cnt_lo, VIA.t1ll);
			DELAY_CYCLES(2);
			v_setScale(VIA.t1ll);

			DAC.curr_x = DAC_MAX_X / 2; // Origin!
			DAC.curr_y = DAC_MAX_Y / 2;
			VIA.pcr = 0xcc; //Shift Reg. Enabled, T1 PB7 Enabled
			VIA.ora = 0;
			VIA.orb = 1;
			DAC.ysh = DAC.xsh = 0;
			VIA.acr = 0x98;
			VIA.ca2 = 0;

			CPU.reg_u = address;
			CPU.reg_pc = 0xF36A; // any RTS
			icycles = 0;
		}
		else
		{
//		  unsigned int reg_pc = CPU.reg_pc;
			if (((CPU.reg_cc / FLAG_I) & 1) == 0)
				icycles = e6809_sstep(GET(0xD00D) & 0x80, 0);
			else
				icycles = e6809_sstep(0, 0);
			// wait for vectrex cycles in nano seconds

			for (uint16_t c = 0; c < icycles; c++)
			{
				via_sstep0();
				dac_sstep();
				via_sstep1();
			}
#ifdef FREESTANDING
			unsigned int waitCycles = icycles*666;
			unsigned int waited  = 0;

// somehow mix a READY wait in here?
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360f/BIHCGFCF.html
			if (mustWait)
			{
				waited  = waitUntil(waitCycles);
			}

			// reset cycle counter
			PMNC(CYCLE_COUNTER_ENABLE|CYCLE_COUNTER_RESET|COUNTER_ZERO);

//if ((reg_pc>=0x1F4A5) && (reg_pc<0xF50A))
//  printf("Waited %x: (%i): %i - %i\r\n", reg_pc, icycles, waited, (waited -(666*icycles)));
#else
 DELAY_CYCLES(icycles);
#endif


			if (delayVia!=0)
			{
//  PMNC(CYCLE_COUNTER_ENABLE|CYCLE_COUNTER_RESET|COUNTER_ZERO);
#ifdef AVOID_TICKS
			  if (linuxIntGap == 0 && linuxIntDisabled > 0)
			  {
			   enableLinuxInterrupts();
			   linuxIntDisabled = 0;
			  }
			  else if (linuxIntDisabled == 0)
			  {
			   disableLinuxInterrupts(linuxIntGap);
			   linuxIntDisabled = 1;
			  }
#endif
			  SET(delayVia, delayData);
//			unsigned int waited  = get_cyclecount();
//  printf("VIA ACCESS: %i \r\n", waited);
			}


			cycles -= icycles;
			cycleCount += icycles;
		}

	}
#ifdef AVOID_TICKS
/* Make sure that Linux interrupts are enabled before exit when Vectrex reset pressed
 * This probably does allow some glitches through.
 * - Actually this doesn't work, but I don't know what else to do :(
 */
if (linuxIntDisabled > 0)
{
 enableLinuxInterrupts();
 linuxIntDisabled = 0;
}
#endif
}
