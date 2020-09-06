/*
#include <stdint.h>
#include <stdio.h>

#include "ser.h"
#include "emu/e6522.h"
#include "emu/e6809.h"
#include "emu/e8910.h"
#include "emu/edac.h"
#include "emu/vecx.h"

static void e6809_load (FILE *file)
{
	fread(&CPU.reg_x, sizeof(CPU.reg_x), 1, file);
	fread(&CPU.reg_y, sizeof(CPU.reg_y), 1, file);
	fread(&CPU.reg_u, sizeof(CPU.reg_u), 1, file);
	fread(&CPU.reg_s, sizeof(CPU.reg_s), 1, file);
	fread(&CPU.reg_pc, sizeof(CPU.reg_pc), 1, file);
	fread(&CPU.reg_a, sizeof(CPU.reg_a), 1, file);
	fread(&CPU.reg_b, sizeof(CPU.reg_b), 1, file);
	fread(&CPU.reg_dp, sizeof(CPU.reg_dp), 1, file);
	fread(&CPU.reg_cc, sizeof(CPU.reg_cc), 1, file);
	fread(&CPU.irq_status, sizeof(CPU.irq_status), 1, file);
}

static void e6809_save (FILE *file)
{
	fwrite(&CPU.reg_x, sizeof(CPU.reg_x), 1, file);
	fwrite(&CPU.reg_y, sizeof(CPU.reg_y), 1, file);
	fwrite(&CPU.reg_u, sizeof(CPU.reg_u), 1, file);
	fwrite(&CPU.reg_s, sizeof(CPU.reg_s), 1, file);
	fwrite(&CPU.reg_pc, sizeof(CPU.reg_pc), 1, file);
	fwrite(&CPU.reg_a, sizeof(CPU.reg_a), 1, file);
	fwrite(&CPU.reg_b, sizeof(CPU.reg_b), 1, file);
	fwrite(&CPU.reg_dp, sizeof(CPU.reg_dp), 1, file);
	fwrite(&CPU.reg_cc, sizeof(CPU.reg_cc), 1, file);
	fwrite(&CPU.irq_status, sizeof(CPU.irq_status), 1, file);
}

static void via_load (FILE *file)
{
	fread(&VIA.ora, sizeof(VIA.ora), 1, file);
	fread(&VIA.orb, sizeof(VIA.orb), 1, file);
	fread(&VIA.ddra, sizeof(VIA.ddra), 1, file);
	fread(&VIA.ddrb, sizeof(VIA.ddrb), 1, file);
	fread(&VIA.t1on, sizeof(VIA.t1on), 1, file);
	fread(&VIA.t1int, sizeof(VIA.t1int), 1, file);
	fread(&VIA.t1c, sizeof(VIA.t1c), 1, file);
	fread(&VIA.t1ll, sizeof(VIA.t1ll), 1, file);
	fread(&VIA.t1lh, sizeof(VIA.t1lh), 1, file);
	fread(&VIA.t1pb7, sizeof(VIA.t1pb7), 1, file);
	fread(&VIA.t2on, sizeof(VIA.t2on), 1, file);
	fread(&VIA.t2int, sizeof(VIA.t2int), 1, file);
	fread(&VIA.t2c, sizeof(VIA.t2c), 1, file);
	fread(&VIA.t2ll, sizeof(VIA.t2ll), 1, file);
	fread(&VIA.sr, sizeof(VIA.sr), 1, file);
	fread(&VIA.srb, sizeof(VIA.srb), 1, file);
	fread(&VIA.src, sizeof(VIA.src), 1, file);
	fread(&VIA.srclk, sizeof(VIA.srclk), 1, file);
	fread(&VIA.acr, sizeof(VIA.acr), 1, file);
	fread(&VIA.pcr, sizeof(VIA.pcr), 1, file);
	fread(&VIA.ifr, sizeof(VIA.ifr), 1, file);
	fread(&VIA.ier, sizeof(VIA.ier), 1, file);
	fread(&VIA.ca2, sizeof(VIA.ca2), 1, file);
	fread(&VIA.cb2h, sizeof(VIA.cb2h), 1, file);
	fread(&VIA.cb2s, sizeof(VIA.cb2s), 1, file);
}

static void via_save (FILE *file)
{
	fwrite(&VIA.ora, sizeof(VIA.ora), 1, file);
	fwrite(&VIA.orb, sizeof(VIA.orb), 1, file);
	fwrite(&VIA.ddra, sizeof(VIA.ddra), 1, file);
	fwrite(&VIA.ddrb, sizeof(VIA.ddrb), 1, file);
	fwrite(&VIA.t1on, sizeof(VIA.t1on), 1, file);
	fwrite(&VIA.t1int, sizeof(VIA.t1int), 1, file);
	fwrite(&VIA.t1c, sizeof(VIA.t1c), 1, file);
	fwrite(&VIA.t1ll, sizeof(VIA.t1ll), 1, file);
	fwrite(&VIA.t1lh, sizeof(VIA.t1lh), 1, file);
	fwrite(&VIA.t1pb7, sizeof(VIA.t1pb7), 1, file);
	fwrite(&VIA.t2on, sizeof(VIA.t2on), 1, file);
	fwrite(&VIA.t2int, sizeof(VIA.t2int), 1, file);
	fwrite(&VIA.t2c, sizeof(VIA.t2c), 1, file);
	fwrite(&VIA.t2ll, sizeof(VIA.t2ll), 1, file);
	fwrite(&VIA.sr, sizeof(VIA.sr), 1, file);
	fwrite(&VIA.srb, sizeof(VIA.srb), 1, file);
	fwrite(&VIA.src, sizeof(VIA.src), 1, file);
	fwrite(&VIA.srclk, sizeof(VIA.srclk), 1, file);
	fwrite(&VIA.acr, sizeof(VIA.acr), 1, file);
	fwrite(&VIA.pcr, sizeof(VIA.pcr), 1, file);
	fwrite(&VIA.ifr, sizeof(VIA.ifr), 1, file);
	fwrite(&VIA.ier, sizeof(VIA.ier), 1, file);
	fwrite(&VIA.ca2, sizeof(VIA.ca2), 1, file);
	fwrite(&VIA.cb2h, sizeof(VIA.cb2h), 1, file);
	fwrite(&VIA.cb2s, sizeof(VIA.cb2s), 1, file);
}

static void e8910_load (FILE *file)
{
	fread(PSG.regs, sizeof(PSG.regs[0]), 16, file);
	fread(&PSG.per_a, sizeof(PSG.per_a), 1, file);
	fread(&PSG.per_b, sizeof(PSG.per_b), 1, file);
	fread(&PSG.per_c, sizeof(PSG.per_c), 1, file);
	fread(&PSG.per_n, sizeof(PSG.per_n), 1, file);
	fread(&PSG.per_e, sizeof(PSG.per_e), 1, file);
	fread(&PSG.cnt_a, sizeof(PSG.cnt_a), 1, file);
	fread(&PSG.cnt_b, sizeof(PSG.cnt_b), 1, file);
	fread(&PSG.cnt_c, sizeof(PSG.cnt_c), 1, file);
	fread(&PSG.cnt_n, sizeof(PSG.cnt_n), 1, file);
	fread(&PSG.cnt_e, sizeof(PSG.cnt_e), 1, file);
	fread(&PSG.vol_a, sizeof(PSG.vol_a), 1, file);
	fread(&PSG.vol_b, sizeof(PSG.vol_b), 1, file);
	fread(&PSG.vol_c, sizeof(PSG.vol_c), 1, file);
	fread(&PSG.vol_e, sizeof(PSG.vol_e), 1, file);
	fread(&PSG.env_a, sizeof(PSG.env_a), 1, file);
	fread(&PSG.env_b, sizeof(PSG.env_b), 1, file);
	fread(&PSG.env_c, sizeof(PSG.env_c), 1, file);
	fread(&PSG.out_a, sizeof(PSG.out_a), 1, file);
	fread(&PSG.out_b, sizeof(PSG.out_b), 1, file);
	fread(&PSG.out_c, sizeof(PSG.out_c), 1, file);
	fread(&PSG.out_n, sizeof(PSG.out_n), 1, file);
	fread(&PSG.cnt_env, sizeof(PSG.cnt_env), 1, file);
	fread(&PSG.hold, sizeof(PSG.hold), 1, file);
	fread(&PSG.alternate, sizeof(PSG.alternate), 1, file);
	fread(&PSG.attack, sizeof(PSG.attack), 1, file);
	fread(&PSG.holding, sizeof(PSG.holding), 1, file);
	fread(&PSG.RNG, sizeof(PSG.RNG), 1, file);
}

static void e8910_save (FILE *file)
{
	fwrite(PSG.regs, sizeof(PSG.regs[0]), 16, file);
	fwrite(&PSG.per_a, sizeof(PSG.per_a), 1, file);
	fwrite(&PSG.per_b, sizeof(PSG.per_b), 1, file);
	fwrite(&PSG.per_c, sizeof(PSG.per_c), 1, file);
	fwrite(&PSG.per_n, sizeof(PSG.per_n), 1, file);
	fwrite(&PSG.per_e, sizeof(PSG.per_e), 1, file);
	fwrite(&PSG.cnt_a, sizeof(PSG.cnt_a), 1, file);
	fwrite(&PSG.cnt_b, sizeof(PSG.cnt_b), 1, file);
	fwrite(&PSG.cnt_c, sizeof(PSG.cnt_c), 1, file);
	fwrite(&PSG.cnt_n, sizeof(PSG.cnt_n), 1, file);
	fwrite(&PSG.cnt_e, sizeof(PSG.cnt_e), 1, file);
	fwrite(&PSG.vol_a, sizeof(PSG.vol_a), 1, file);
	fwrite(&PSG.vol_b, sizeof(PSG.vol_b), 1, file);
	fwrite(&PSG.vol_c, sizeof(PSG.vol_c), 1, file);
	fwrite(&PSG.vol_e, sizeof(PSG.vol_e), 1, file);
	fwrite(&PSG.env_a, sizeof(PSG.env_a), 1, file);
	fwrite(&PSG.env_b, sizeof(PSG.env_b), 1, file);
	fwrite(&PSG.env_c, sizeof(PSG.env_c), 1, file);
	fwrite(&PSG.out_a, sizeof(PSG.out_a), 1, file);
	fwrite(&PSG.out_b, sizeof(PSG.out_b), 1, file);
	fwrite(&PSG.out_c, sizeof(PSG.out_c), 1, file);
	fwrite(&PSG.out_n, sizeof(PSG.out_n), 1, file);
	fwrite(&PSG.cnt_env, sizeof(PSG.cnt_env), 1, file);
	fwrite(&PSG.hold, sizeof(PSG.hold), 1, file);
	fwrite(&PSG.alternate, sizeof(PSG.alternate), 1, file);
	fwrite(&PSG.attack, sizeof(PSG.attack), 1, file);
	fwrite(&PSG.holding, sizeof(PSG.holding), 1, file);
	fwrite(&PSG.RNG, sizeof(PSG.RNG), 1, file);
}

void dac_load(FILE *file)
{
	fread(&DAC.rsh, sizeof(DAC.rsh), 1, file);
	fread(&DAC.xsh, sizeof(DAC.xsh), 1, file);
	fread(&DAC.ysh, sizeof(DAC.ysh), 1, file);
	fread(&DAC.zsh, sizeof(DAC.zsh), 1, file);
	fread(&DAC.jch0, sizeof(DAC.jch0), 1, file);
	fread(&DAC.jch1, sizeof(DAC.jch1), 1, file);
	fread(&DAC.jch2, sizeof(DAC.jch2), 1, file);
	fread(&DAC.jch3, sizeof(DAC.jch3), 1, file);
	fread(&DAC.jsh, sizeof(DAC.jsh), 1, file);
	fread(&DAC.compare, sizeof(DAC.compare), 1, file);
	fread(&DAC.dx, sizeof(DAC.dx), 1, file);
	fread(&DAC.dy, sizeof(DAC.dy), 1, file);
	fread(&DAC.curr_x, sizeof(DAC.curr_x), 1, file);
	fread(&DAC.curr_y, sizeof(DAC.curr_y), 1, file);
	fread(&DAC.vectoring, sizeof(DAC.vectoring), 1, file);
	fread(&DAC.vector_x0, sizeof(DAC.vector_x0), 1, file);
	fread(&DAC.vector_y0, sizeof(DAC.vector_y0), 1, file);
	fread(&DAC.vector_x1, sizeof(DAC.vector_x1), 1, file);
	fread(&DAC.vector_y1, sizeof(DAC.vector_y1), 1, file);
	fread(&DAC.vector_dx, sizeof(DAC.vector_dx), 1, file);
	fread(&DAC.vector_dy, sizeof(DAC.vector_dy), 1, file);
	fread(&DAC.vector_color, sizeof(DAC.vector_color), 1, file);
}

void dac_save(FILE* file)
{
	fwrite(&DAC.rsh, sizeof(DAC.rsh), 1, file);
	fwrite(&DAC.xsh, sizeof(DAC.xsh), 1, file);
	fwrite(&DAC.ysh, sizeof(DAC.ysh), 1, file);
	fwrite(&DAC.zsh, sizeof(DAC.zsh), 1, file);
	fwrite(&DAC.jch0, sizeof(DAC.jch0), 1, file);
	fwrite(&DAC.jch1, sizeof(DAC.jch1), 1, file);
	fwrite(&DAC.jch2, sizeof(DAC.jch2), 1, file);
	fwrite(&DAC.jch3, sizeof(DAC.jch3), 1, file);
	fwrite(&DAC.jsh, sizeof(DAC.jsh), 1, file);
	fwrite(&DAC.compare, sizeof(DAC.compare), 1, file);
	fwrite(&DAC.dx, sizeof(DAC.dx), 1, file);
	fwrite(&DAC.dy, sizeof(DAC.dy), 1, file);
	fwrite(&DAC.curr_x, sizeof(DAC.curr_x), 1, file);
	fwrite(&DAC.curr_y, sizeof(DAC.curr_y), 1, file);
	fwrite(&DAC.vectoring, sizeof(DAC.vectoring), 1, file);
	fwrite(&DAC.vector_x0, sizeof(DAC.vector_x0), 1, file);
	fwrite(&DAC.vector_y0, sizeof(DAC.vector_y0), 1, file);
	fwrite(&DAC.vector_x1, sizeof(DAC.vector_x1), 1, file);
	fwrite(&DAC.vector_y1, sizeof(DAC.vector_y1), 1, file);
	fwrite(&DAC.vector_dx, sizeof(DAC.vector_dx), 1, file);
	fwrite(&DAC.vector_dy, sizeof(DAC.vector_dy), 1, file);
	fwrite(&DAC.vector_color, sizeof(DAC.vector_color), 1, file);
}

void vecx_load(char *name)
{
	FILE *f;
	if (!(f = fopen(name, "rb")))
	{
		perror(name);
		return;
	}
	fread(ram, sizeof(ram[0]), 1024, f);
	fread(&snd_select, sizeof(snd_select), 1, f);
	e6809_load(f);
	via_load(f);
	e8910_load(f);
	dac_load(f);
	fclose(f);
}

void vecx_save(char *name)
{
	FILE *f;
	if (!(f = fopen(name, "wb")))
	{
		perror(name);
		return;
	}
	fwrite(ram, sizeof(ram[0]), 1024, f);
	fwrite(&snd_select, sizeof(snd_select), 1, f);
	e6809_save(f);
	via_save(f);
	e8910_save(f);
	dac_save(f);
	fclose(f);
}
*/
