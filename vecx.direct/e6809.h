#ifndef __E6809_H
#define __E6809_H

typedef struct
{
	uint16_t reg_x, reg_y; /* index registers */
	uint16_t reg_u; /* user stack pointer */
	uint16_t reg_s; /* hardware stack pointer */
	uint16_t reg_pc; /* program counter */
	uint8_t reg_a, reg_b; /* accumulators */
	uint8_t reg_dp; /* direct page register */
	uint8_t reg_cc; /* condition codes */
	uint16_t irq_status; /* flag to see if interrupts should be handled (sync/cwai). */
	uint16_t *rptr_xyus[4];

} M6809;

extern M6809 CPU;

enum
{
	FLAG_E = 0x80,
	FLAG_F = 0x40,
	FLAG_H = 0x20,
	FLAG_I = 0x10,
	FLAG_N = 0x08,
	FLAG_Z = 0x04,
	FLAG_V = 0x02,
	FLAG_C = 0x01,

	IRQ_NORMAL = 0,
	IRQ_SYNC = 1,
	IRQ_CWAI = 2
};

/* user defined read and write functions */

extern uint8_t(*e6809_read8) (uint16_t address);
extern void(*e6809_write8) (uint16_t address, uint8_t data);

void e6809_reset(void);
uint16_t e6809_sstep(uint16_t irq_i, uint16_t irq_f);

#endif
