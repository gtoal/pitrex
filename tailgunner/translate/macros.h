// look for '//' comments for recent changes
#include <assert.h>
#define NOTUSED FALSE

//#ifdef DUALCPU
#define return_or_continue_or_break "return"
//#else
//#define return_or_continue_or_break "continue"
//#endif


/*
INP_A is inputs, INP B is switches
*/
CINESTATE opINP_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opINP_A_AA (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  fprintf(codefile, "%scmp_old = flag_C = acc_a0 = register_A;\n", cur_tabs);
  fprintf(codefile, "#ifdef RAWIO\n");
  fprintf(codefile, "%sregister_A = cmp_new = get_io_bit(0x%01x);\n", cur_tabs, opcode & 0x0F);
  fprintf(codefile, "#else\n");
  if ((opcode&0x0F) == 1) {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_moveright();\n", cur_tabs);
  } else if ((opcode&0x0F) == 2) {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_moveleft();\n", cur_tabs);
  } else if ((opcode&0x0F) == 3) {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_moveup();\n", cur_tabs);
  } else if ((opcode&0x0F) == 4) {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_movedown();\n", cur_tabs);
  } else if ((opcode&0x0F) == 5) {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_fire();\n", cur_tabs);
  } else if ((opcode&0x0F) == 6) {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_shields();\n", cur_tabs);
  } else if ((opcode&0x0F) == 7) {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_startbutton();\n", cur_tabs);
  } else if ((opcode&0x0F) == 13) {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_laserstyle();\n", cur_tabs);
  } else {
    fprintf(codefile, "%sregister_A = cmp_new = get_io_bit(0x%01x);\n", cur_tabs, opcode & 0x0F);
  }
  fprintf(codefile, "#endif\n");
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opINP_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opINP_B_AA (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B; /* save old accB */\n");

  fprintf(codefile, "#ifdef RAWIO\n");
  fprintf(codefile, "%sregister_B = cmp_new = (( ioSwitches >> 0x%01x ) & 0x01);\n", cur_tabs,  rom [register_PC] & 0x07 );
  fprintf(codefile, "#else\n");

  if ((opcode&7) == 7 /*docs say 7 on input, 5 on output? */) {
    fprintf(codefile, "%sregister_B = cmp_new = get_coin_state(); /* apparently not used in tailgunner??? */\n", cur_tabs);
  } else if ((opcode&7) == 0) {
    fprintf(codefile, "%sregister_B = cmp_new = get_quarters_per_game(); /* 1 => 1q/game, 0 => 2q/game */ \n", cur_tabs);
  } else if ((opcode&7) == 1) {
    fprintf(codefile, "%sregister_B = cmp_new = get_shield_bit2();\n", cur_tabs);
  } else if ((opcode&7) == 4) {
    fprintf(codefile, "%sregister_B = cmp_new = get_shield_bit1();\n", cur_tabs);
  } else if ((opcode&7) == 5) {
    fprintf(codefile, "%sregister_B = cmp_new = get_shield_bit0();\n", cur_tabs);
  } else {
    fprintf(codefile, "%sregister_B = cmp_new = get_switch_bit(0x%01x);\n", cur_tabs, opcode&7);
  }

  fprintf(codefile, "#endif\n");
  register_B = 0xDEADBEEFUL;

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opOUTsnd_A (int opcode)
{
  internal_test(opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUTsnd_A (%02x) */\n", cur_tabs, opcode);
  if (register_A != 0xDEADBEEFUL) {
    if ((opcode & 0x07) == 5) {
      fprintf(codefile, "%sreset_coin_counter(%0ld);\n", cur_tabs, register_A&1);
    } else if ((opcode & 0x07) == 3) {
      fprintf(codefile, "%sset_sound_data(%0ld);\n", cur_tabs, register_A&1);
    } else if ((opcode & 0x07) == 0) {
      fprintf(codefile, "%sset_sound_addr_A(%0ld);\n", cur_tabs, register_A&1);
    } else if ((opcode & 0x07) == 1) {
      fprintf(codefile, "%sset_sound_addr_B(%0ld);\n", cur_tabs, register_A&1);
    } else if ((opcode & 0x07) == 2) {
      fprintf(codefile, "%sset_sound_addr_C(%0ld);\n", cur_tabs, register_A&1);
    } else if ((opcode & 0x07) == 4) {
      if (register_A&1) {
        fprintf(codefile, "%sstrobe_sound_on();\n", cur_tabs);
      } else {
        fprintf(codefile, "%sstrobe_sound_off();\n", cur_tabs);
      }
    } else {
      fprintf(codefile, "%sput_io_bit(/*bitno*/0x%0x, /*set or clr*/0x%01lx);\n", cur_tabs, opcode & 0x07, register_A&1);
    }
  } else {
    if ((opcode & 0x07) == 5) {
      fprintf(codefile, "%s%s", cur_tabs, "reset_coin_counter(register_A&1);\n");
    } else if ((opcode & 0x07) == 3) {
      fprintf(codefile, "%sset_sound_data(register_A&1);\n", cur_tabs);
    } else if ((opcode & 0x07) == 0) {
      fprintf(codefile, "%sset_sound_addr_A(register_A&1);\n", cur_tabs);
    } else if ((opcode & 0x07) == 1) {
      fprintf(codefile, "%sset_sound_addr_B(register_A&1);\n", cur_tabs);
    } else if ((opcode & 0x07) == 2) {
      fprintf(codefile, "%sset_sound_addr_C(register_A&1);\n", cur_tabs);
    } else if ((opcode & 0x07) == 4) {
        fprintf(codefile, "%sif (register_A&1) strobe_sound_on(); else strobe_sound_off();\n", cur_tabs);
    } else {
      fprintf(codefile, "%sput_io_bit(/*bitno*/0x%01x, /*set or clr*/register_A&1);\n", cur_tabs, opcode & 0x07);
    }
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opOUTbi_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUTbi_A_A (%02x) */\n", cur_tabs, opcode);

  if ((opcode & 0x07) != 6) {
    return state = opOUTsnd_A (opcode); /* Macro expand */
  }

  if (register_A != 0xDEADBEEFUL) {
    fprintf(codefile, "%svgColour = 0x%02x;\n", cur_tabs, (register_A & 0x01 ? 0x0f: 0x07));
  } else {
    fprintf(codefile, "%s%s", cur_tabs, "vgColour = register_A & 0x01 ? 0x0f: 0x07;\n");
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opOUT16_A_A (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUT16_A_A (%02x) */\n", cur_tabs, opcode);

  if ((opcode & 0x07) != 6)
    return opOUTsnd_A (opcode);

  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);

  fprintf(codefile, "%s%s", cur_tabs, "if ((register_A & 0x1) != 1)\n");
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s%s", cur_tabs, "		vgColour = FromX & 0x0F;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "		if (!vgColour)\n");
  fprintf(codefile, "%s%s", cur_tabs, "			vgColour = 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opOUT64_A_A (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUT64_A_A (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opOUTWW_A_A (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUTWW_A_A (%02x) */\n", cur_tabs, opcode);

  if ((opcode & 0x07) != 6)
    return opOUTsnd_A (opcode);

  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);

  fprintf(codefile, "%s%s", cur_tabs, "if ((register_A & 0x1) == 1) {\n");
  push_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "if (!(~FromX & 0x0FFF)) {/* black */\n");
  fprintf(codefile, "%s%s", cur_tabs, "	vgColour = 0;\n");
  fprintf(codefile, "%s%s", cur_tabs, "} else { /* non-black */\n");
  fprintf(codefile, "%s%s", cur_tabs, "	if ((~FromX & 0x0FFF) & 0x0888) /* bright */ vgColour = (((~FromX & 0x0FFF) >> 1) & 0x04) | (((~FromX & 0x0FFF) >> 6) & 0x02) | (((~FromX & 0x0FFF) >> 11) & 0x01) | 0x08;\n");
  fprintf(codefile, "%s%s", cur_tabs, "	else if ((~FromX & 0x0FFF) & 0x0444) /* dim bits */ vgColour = ((~FromX & 0x0FFF) & 0x04) | (((~FromX & 0x0FFF) >> 5) & 0x02) | (((~FromX & 0x0FFF) >> 10) & 0x01);\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  pop_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "} /* colour change? == 1 */\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opOUTsnd_B (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUTsnd_B (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opOUTbi_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUTbi_B_BB (%02x) */\n", cur_tabs, opcode);

  if ((opcode & 0x07) != 0x06) return opOUTsnd_B (opcode);

  fprintf(codefile, "%s%s", cur_tabs, "vgColour = ((register_B & 0x01) << 3) | 0x07;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opOUT16_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUT16_B_BB (%02x) */\n", cur_tabs, opcode);

  if ((opcode & 0x07) - 0x06)
    return opOUTsnd_B (opcode);

  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);

  fprintf(codefile, "%s%s", cur_tabs, "if ((register_B & 0xFF) != 1)\n");
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s%s", cur_tabs, "		vgColour = FromX & 0x0F;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "		if (!vgColour)\n");
  fprintf(codefile, "%s%s", cur_tabs, "			vgColour = 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opOUT64_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUT64_B_BB (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opOUTWW_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opOUTWW_B_BB (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}


CINESTATE opLDAimm_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDAimm_A_AA (%02x) */\n", cur_tabs, opcode);
  if (opcode == 0 /* CLR */) {
    fprintf(codefile, "%s%s", cur_tabs, "cmp_old = flag_C = acc_a0 = register_A;\n");
    fprintf(codefile, "%sregister_A = cmp_new = 0;\n", cur_tabs);
    register_A = 0;
  } else if (ImmOpd != 0xDEADBEEFUL) {
/* Here is the first major bug - I was only assigning register_A, I forgot
   about all the other crud.  MUST CHECK the other instructions for similar
   loss of side effects! */
    fprintf(codefile, "%s%s", cur_tabs, "cmp_old = flag_C = acc_a0 = register_A;\n");
    fprintf(codefile, "%sregister_A = cmp_new = 0x%04lx;\n", cur_tabs, register_A = ImmOpd);
  } else {
    fprintf(codefile, "%s%s", cur_tabs, "cmp_old = flag_C = acc_a0 = register_A;\n");
    fprintf(codefile, "%sregister_A = cmp_new = 0x%04lx; /* pick up immediate value */\n", cur_tabs, register_A = ((opcode & 0x0F) << 8));
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA; /* swap state and end opcode */\n");
  return state = state_AA;
}

CINESTATE opLDAimm_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDAimm_B_AA (%02x) */\n", cur_tabs, opcode);
  if (opcode == 0 /* CLR */) {
    fprintf(codefile, "%s%s", cur_tabs, "flag_C = acc_a0 = register_A;\n");
    fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B; /* step back cmp flag */\n");
    fprintf(codefile, "%sregister_B = cmp_new = 0;\n", cur_tabs);
    register_B = 0;
  } else if (ImmOpd != 0xDEADBEEFUL) {
/* Same bug, fixed */
    fprintf(codefile, "%s%s", cur_tabs, "flag_C = acc_a0 = register_A;\n");
    fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B; /* step back cmp flag */\n");
    fprintf(codefile, "%sregister_B = cmp_new = 0x%04lx;\n", cur_tabs, register_B = ImmOpd);
  } else {
    fprintf(codefile, "%s%s", cur_tabs, "flag_C = acc_a0 = register_A;\n");
    fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B; /* step back cmp flag */\n");
    fprintf(codefile, "%sregister_B = cmp_new = 0x%04lx; /* pick up immediate value */\n", cur_tabs, register_B = ((opcode & 0x0F) << 8));
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLDAdir_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDAdir_A_AA (%02x) */\n", cur_tabs, opcode);

  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = flag_C = acc_a0 = register_A; /* store old acc */\n");
  if (register_P != 0xDEADBEEFUL) {
    fprintf(codefile, "%sregister_A = cmp_new = ram[register_I = 0x%02lx]; /* set I register */\n", cur_tabs, register_I = (register_P << 4) + (opcode & 0x0f));
  } else {
    fprintf(codefile, "%sregister_A = cmp_new = ram[register_I = (register_P << 4) + 0x%02x]; /* set I register */\n", cur_tabs, (opcode & 0x0f));
    register_I = 0xDEADBEEFUL;
  }
  register_A = 0xDEADBEEFUL;
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLDAdir_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDAdir_B_AA (%02x) */\n", cur_tabs, opcode);

/* remember to finish compressing the source code */

  fprintf(codefile, "%sflag_C = acc_a0 = register_A;\n", cur_tabs);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B; /* store old acc */\n");
  if (register_P != 0xDEADBEEFUL) {
    fprintf(codefile, "%sregister_B = cmp_new = ram[register_I = (register_P << 4) + 0x%02x]; /* new acc value */\n", cur_tabs, opcode & 0x0F);
    register_I = ((register_P << 4) + (opcode & 0x0F));
  } else {
    fprintf(codefile, "%sregister_B = cmp_new = ram[register_I = (register_P << 4) + 0x%02x]; /* new acc value */\n", cur_tabs, opcode & 0x0F);
    register_I = 0xDEADBEEFUL;
  }
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLDAirg_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDAirg_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%scmp_old = flag_C = acc_a0 = register_A;\n", cur_tabs);
  /* BUG FIX - add cmp_new */
  fprintf(codefile, "%s%s", cur_tabs, "register_A = cmp_new = ram[register_I];\n");
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLDAirg_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDAirg_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B = cmp_new = ram[register_I];\n");
  register_B = 0xDEADBEEFUL;
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}


CINESTATE opADDimm_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opADDimm_A_AA (%02x) */\n", cur_tabs, opcode);
  if (register_A != 0xDEADBEEFUL) {
    register_A += (opcode & 0x0f);
    fprintf(codefile, "%scmp_new = 0x%01x; cmp_old = acc_a0 = register_A; ", cur_tabs, opcode&0x0f);
    if ((register_A & 0xf000) != 0) { /* Carry */
      fprintf(codefile, "register_A = 0x%03lx; flag_C = 0x1000;\n", register_A & 0xfff);
    } else { /* No carry */
      fprintf(codefile, "register_A = flag_C = 0x%04lx;\n", register_A);
    }
  } else {
    fprintf(codefile, "%sregister_A = (flag_C = ((cmp_old = acc_a0 = register_A) + (cmp_new = 0x%01x))) & 0xFFF; /* add values, save carry */\n", cur_tabs, (opcode & 0x0f));
  }
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opADDimm_B_AA (int opcode)
{
  internal_test(opcode);
/* Still to optimise this one as above */
  if (disp_opcodes) fprintf(codefile, "%s/* opADDimm_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sacc_a0 = register_A; /* save old accA bit0 */\n", cur_tabs);
  fprintf(codefile, "%scmp_old = register_B; /* store old acc for later */\n", cur_tabs);
  fprintf(codefile, "%sregister_B = (flag_C = (register_B + (cmp_new = 0x%02x))) & 0xFFF; /* add values */\n", cur_tabs, (opcode & 0x0F));
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opADDimmX_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opADDimmX_A_AA (%02x) */\n", cur_tabs, opcode);
  if (ImmOpd == 0xDEADBEEFUL) {
    ImmOpd = rom[register_PC+1]; /* Why wasn't this picked up in the opcode decoding? */
  }
  if (register_A != 0xDEADBEEFUL) {
    fprintf(codefile, "%scmp_old = register_A; cmp_new = 0x%02lx;\n", cur_tabs, ImmOpd); /* BUG FIX */
    if (((ImmOpd + register_A) >> 12) != 0) {
      fprintf(codefile, "%sregister_A = (flag_C = (acc_a0 = 0x%04lx)) & 0xFFF;\n", cur_tabs, register_A = (ImmOpd+register_A));
      register_A &= 0xFFF;
    } else {
      fprintf(codefile, "%sregister_A = flag_C = acc_a0 = 0x%04lx;\n", cur_tabs, register_A = (ImmOpd+register_A));
    }
  } else {
    fprintf(codefile, "%sregister_A = (flag_C = ((acc_a0 = cmp_old = register_A) + (cmp_new = 0x%04lx))) & 0xFFF; /* add values */\n", cur_tabs, ImmOpd);
    register_A = 0xDEADBEEFUL;                        /* IS THIS BRACKETING CORRECT? */
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opADDimmX_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opADDimmX_B_AA (%02x) */\n", cur_tabs, opcode);

  fprintf(codefile, "%scmp_old = register_B; acc_a0 = register_A; /* save old accA bit0 */\n", cur_tabs);

  if (ImmOpd == 0xDEADBEEFUL) {
    ImmOpd = rom[register_PC+1]; /* Why wasn't this picked up in the opcode decoding? */
  }
  if (register_B != 0xDEADBEEFUL) {
    fprintf(codefile, "%scmp_new = 0x%02lx;\n", cur_tabs, ImmOpd); /* I assume the bug fix above is needed here too? */
    if (((ImmOpd + register_B) >> 12) != 0) {
      fprintf(codefile, "%sregister_B = (flag_C = 0x%04lx) & 0xFFF; cmp_new = 0x%02lx\n", cur_tabs, register_B = (ImmOpd+register_B), ImmOpd);
      register_B &= 0xFFF;
    } else {
      fprintf(codefile, "%sregister_B = flag_C = 0x%04lx; cmp_new = 0x%02lx; /* No carry */\n", cur_tabs, register_B = (ImmOpd+register_B), ImmOpd);
    }
  } else {
    fprintf(codefile, "%sregister_B = (flag_C = (register_B + (cmp_new = 0x%02lx))) & 0xFFF; /* add values */\n", cur_tabs, ImmOpd);
                                              /* Is this bracketing correct? */
    register_B = 0xDEADBEEFUL;
  }

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opADDdir_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opADDdir_A_AA (%02x) */\n", cur_tabs, opcode);
  if (register_P != 0xDEADBEEFUL) {
    fprintf(codefile, "%sregister_A = (flag_C = ((cmp_old = acc_a0 = register_A) + (cmp_new = ram[register_I = 0x%02lx]))) & 0xFFF; /* do acc operation */\n", cur_tabs, register_I = (register_P << 4) + (opcode & 0x0F));
  } else {
    fprintf(codefile, "%sregister_A = (flag_C = ((cmp_old = acc_a0 = register_A) + (cmp_new = ram[register_I = (register_P << 4) + 0x%01x]))) & 0xFFF; /* do acc operation */\n", cur_tabs, (opcode & 0x0F));
    register_I = 0xDEADBEEFUL;
  }
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opADDdir_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opADDdir_B_AA (%02x) */\n", cur_tabs, opcode);

  fprintf(codefile, "%s%s", cur_tabs, "acc_a0 = register_A; /* store old acc value */\n");
  if (register_P != 0xDEADBEEFUL) {
    fprintf(codefile, "%sregister_B = (flag_C = ((cmp_old = register_B) + (cmp_new = ram[register_I = 0x%02lx]))) & 0xFFF; /* do acc operation */\n", cur_tabs, register_I = ((register_P << 4) + (opcode & 0x0F)));
  } else {
    fprintf(codefile, "%sregister_B = (flag_C = ((cmp_old = register_B) + (cmp_new = ram[register_I = (register_P << 4) + 0x%02x]))) & 0xFFF; /* do acc operation */\n", cur_tabs, (opcode & 0x0F));
    register_I = 0xDEADBEEFUL;
  }
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opAWDirg_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opAWDirg_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = acc_a0 = register_A;\n");
  if (register_I != 0xDEADBEEFUL) {
    fprintf(codefile, "%sregister_A = (flag_C = (register_A + (cmp_new = ram[0x%02lx]))) & 0xFFF;\n", cur_tabs, register_I);
  } else {
    fprintf(codefile, "%s%s", cur_tabs, "register_A = (flag_C = (register_A + (cmp_new = ram[register_I]))) & 0xFFF;\n");
  }
  fprintf(codefile, "%s%s", cur_tabs, "set_watchdog();\n");
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opAWDirg_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opAWDirg_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B = (flag_C = (register_B + (cmp_new = ram[register_I]))) & 0xFFF;\n");
  register_B = 0xDEADBEEFUL;
/* does this one also need to set the watchdog??? - original version of code didn't,
   and I have added it without checking too closely... */
  fprintf(codefile, "%s%s", cur_tabs, "set_watchdog();\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opSUBimm_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSUBimm_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = acc_a0 = register_A;\n");
  fprintf(codefile, "%sregister_A = (flag_C = (register_A + (((cmp_new = 0x%01x) ^ 0xFFF) + 1))) & 0xFFF; /* 1's-comp add */\n", cur_tabs, (opcode & 0x0F));
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opSUBimm_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSUBimm_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sacc_a0 = register_A;\n", cur_tabs);
  fprintf(codefile, "%sregister_B = (flag_C = ((cmp_old = register_B) + ((cmp_new = 0x%01x) ^ 0xFFF) + 1)) & 0xFFF; /* 1's-comp add */\n", cur_tabs, (opcode & 0x0F));
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opSUBimmX_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSUBimmX_A_AA (%02x) */\n", cur_tabs, opcode);
/* Question: is this the byte FOLLOWING the instruction? */
  if (ImmOpd == 0xDEADBEEFUL) {
    ImmOpd = rom[register_PC+1]; /* Why wasn't this picked up in the opcode decoding? */
  }

  fprintf(codefile, "%scmp_old = acc_a0 = register_A; /* back up regA */\n", cur_tabs);
  fprintf(codefile, "%sregister_A = (flag_C = (register_A + (((cmp_new = 0x%02lx) ^ 0xFFF) + 1))) & 0xFFF; /* add */\n", cur_tabs, ImmOpd);

  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opSUBimmX_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSUBimmX_B_AA (%02x) */\n", cur_tabs, opcode);
  if (ImmOpd == 0xDEADBEEFUL) {
    ImmOpd = rom[register_PC+1]; /* Why wasn't this picked up in the opcode decoding? */
  }
  fprintf(codefile, "%sacc_a0 = register_A;\n", cur_tabs);
  fprintf(codefile, "%sregister_B = (flag_C = ((cmp_old = register_B) + (((cmp_new = 0x%02lx) ^ 0xFFF) + 1))) & 0xFFF; /* add */\n", cur_tabs, ImmOpd);

  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opSUBdir_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSUBdir_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = acc_a0 = register_A;\n");

  if (register_P != 0xDEADBEEFUL) {
    fprintf(codefile, "%sregister_A = (flag_C = (register_A + ((cmp_new = ram[register_I = 0x%02lx]) ^ 0xFFF) + 1)) & 0xFFF; /* set regI addr */\n", cur_tabs, register_I = (register_P << 4) + (opcode & 0x0F));
  } else {
    fprintf(codefile, "%sregister_A = (flag_C = (register_A + ((cmp_new = ram[register_I = (register_P << 4) + 0x%01x]) ^ 0xFFF) + 1)) & 0xFFF; /* set regI addr */\n", cur_tabs, (opcode & 0x0F));
    register_I = 0xDEADBEEFUL;
  }
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opSUBdir_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSUBdir_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sacc_a0 = register_A;\n", cur_tabs);
  fprintf(codefile, "%sregister_B = (flag_C = ((cmp_old = register_B) + ((cmp_new = ram[register_I = (register_P << 4) + 0x%02x]) ^ 0xFFF) + 1)) & 0xFFF; /* ones compliment */\n", cur_tabs, (opcode & 0x0F));
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opSUBirg_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSUBirg_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "register_A = (flag_C = ((cmp_old = acc_a0 = register_A) + ((cmp_new = ram[register_I]) ^ 0xFFF) + 1)) & 0xFFF; /* ones compliment */\n");
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opSUBirg_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSUBirg_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sacc_a0 = register_A;\n", cur_tabs);
  fprintf(codefile, "%s%s", cur_tabs, "register_B = (flag_C = ((cmp_old = register_B) + ((cmp_new = ram[register_I]) ^ 0xFFF) + 1)) & 0xFFF; /* ones compliment */\n");
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}


CINESTATE opCMPdir_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opCMPdir_A_AA (%02x) */\n", cur_tabs, opcode);
  if (comments) {
    fprintf(codefile, "%s%s", cur_tabs, "/*\n");
    fprintf(codefile, "%s%s", cur_tabs, " * compare direct mode; don't modify regs, just set carry flag or not.\n");
    fprintf(codefile, "%s%s", cur_tabs, " */\n");
  }
  fprintf(codefile, "%scmp_old = acc_a0 = register_A; /* backup old acc */\n", cur_tabs);
  if (register_P == 0xDEADBEEFUL) {
    fprintf(codefile, "%sflag_C = (((cmp_new = ram[register_I = (register_P << 4) + 0x%01x]) ^ 0xFFF) + 1 + register_A);\n", cur_tabs, (opcode & 0x0F));
    register_I = 0xDEADBEEFUL;
  } else {
#ifdef NEVER
    fprintf(codefile, "%sif (register_P != 0x%01x) ERROR(\"register_P is faulty\\n\"); /* Optimisation assertion test */\n", cur_tabs, register_P);
#endif
    fprintf(codefile, "%sflag_C = (((cmp_new = ram[register_I = 0x%02lx]) ^ 0xFFF) + 1 + register_A);\n", cur_tabs, register_I = ((register_P << 4) + (opcode & 0x0f)));
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opCMPdir_B_AA (int opcode)
{
  internal_test(opcode);
  /* SEE ABOVE */
  if (disp_opcodes) fprintf(codefile, "%s/* opCMPdir_B_AA (%02x) */\n", cur_tabs, opcode);

  fprintf(codefile, "%s%s", cur_tabs, "acc_a0 = register_A;\n");
  fprintf(codefile, "%sflag_C = ((((cmp_new = ram[register_I = (register_P << 4) + 0x%01x]) ^ 0xFFF) + 1) + (cmp_old = register_B)); /* ones compliment */\n", cur_tabs, (opcode & 0x0F));

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");

  return state = state_AA;
}


CINESTATE opANDirg_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opANDirg_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = flag_C = acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A &= (cmp_new = ram[register_I]);\n");
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opANDirg_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opANDirg_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= (cmp_new = ram[register_I]);\n");
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}


CINESTATE opLDJimm_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDJimm_A_A (%02x) */\n", cur_tabs, opcode);

  if (ImmOpd == 0xDEADBEEFUL) {
    CINEWORD temp_byte = rom[register_PC+1];
    temp_byte = ((temp_byte << 4) | ((temp_byte >> 4)&0xF)) & 0xFF;
    ImmOpd = (opcode & 0x0F) | (temp_byte << 4);
  }
  /* A DANGEROUS OPTIMISATION WOULD BE TO NOT OUTPUT THE NEXT LINE: !!! */
  register_J = ImmOpd;
#ifndef SUPEROPTIMISE
  fprintf(codefile, "%sregister_J = 0x%04lx;\n", cur_tabs, register_J);
#endif
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opLDJimm_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDJimm_B_BB (%02x) */\n", cur_tabs, opcode);

  if (ImmOpd == 0xDEADBEEFUL) {
    CINEWORD temp_byte = rom[register_PC+1];
    temp_byte = ((temp_byte << 4) | ((temp_byte >> 4)&0xF)) & 0xFF;
    ImmOpd = (opcode & 0x0F) | (temp_byte << 4);
  }
  /* A DANGEROUS OPTIMISATION WOULD BE TO NOT OUTPUT THE NEXT LINE: !!! */
  register_J = ImmOpd;
#ifndef SUPEROPTIMISE
  fprintf(codefile, "%sregister_J = 0x%04lx;\n", cur_tabs, register_J);
#endif

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}


CINESTATE opLDJirg_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDJirg_A_A (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
/*fprintf(stderr, "MANUAL ASSISTANCE: Look at dynamic jump through table at 0x%04x\n", register_PC);*/
  if (pass == PASS_1) fprintf(codefile, "%s/* WARNING: DYNAMIC JUMP TABLE.  NEEDS HAND-TWEAKING */\n", cur_tabs);
  fprintf(codefile, "%s%s", cur_tabs, "/* load J reg from value at last dir addr */\n");
  if (register_I != 0xDEADBEEFUL) {
    fprintf(codefile, "%sregister_J = ram[0x%02lx];\n", cur_tabs, register_I);
  } else {
    fprintf(codefile, "%s%s", cur_tabs, "register_J = ram[register_I];\n");
  }
//fprintf(codefile, "%s%s", cur_tabs, "if ((register_J & (~0xfff)) != 0) ERROR(\"Ram value must have been > 12 bits!\\n\");\n");
  if (debug) fprintf(codefile, "%sif (debug) fprintf(stderr, \"%%04x J set to 0x%%03x from ram[0x%%02x]\\n\", register_PC, register_J, register_I);\n", cur_tabs);
  register_J = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opLDJirg_B_BB (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDJirg_B_BB (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
/*fprintf(stderr, "MANUAL ASSISTANCE: Look at dynamic jump through table at 0x%04x\n", register_PC);*/
  if (pass == PASS_1) fprintf(codefile, "%s/* WARNING: DYNAMIC JUMP TABLE.  NEEDS HAND-TWEAKING */\n", cur_tabs);
  fprintf(codefile, "%s%s", cur_tabs, "/* load J reg from value at last dir addr */\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_J = ram[register_I] & 0xFFF;\n");
  if (debug) fprintf(codefile, "%sif (debug) fprintf(stderr, \"%%04x J set to 0x%%03x from ram[0x%%02x]\\n\", register_PC, register_J, register_I);\n", cur_tabs);
  register_J = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}


CINESTATE opLDPimm_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDPimm_A_A (%02x) */\n", cur_tabs, opcode);
  if (ImmOpd != 0xDEADBEEFUL) {
    assert(ImmOpd < 16);
    assert(ImmOpd >= 0);
    if (register_P != ImmOpd) fprintf(codefile, "%sregister_P = 0x%01lx; /* set page register */\n", cur_tabs, ImmOpd);
    register_P = ImmOpd;
  } else {
    if (register_P != (opcode&0x0f)) fprintf(codefile, "%sregister_P = 0x%01x; /* set page register */\n", cur_tabs, opcode & 0x0F);
    /* ERROR - can't see why it would ever get here */
    exit(1);
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opLDPimm_B_BB (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDPimm_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "/* load page register from immediate */\n");
  if (register_P != (opcode&0x0f)) fprintf(codefile, "%sregister_P = 0x%01x; /* set page register */\n", cur_tabs, opcode & 0x0F);
  register_P = opcode&0x0f;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}


CINESTATE opLDIdir_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDIdir_A_A (%02x) */\n", cur_tabs, opcode);
  if (register_P != 0xDEADBEEFUL) {
    fprintf(codefile, "%sregister_I = ram[0x%02lx]&0xff; /* set new register_I (8 bits) */\n", cur_tabs, (register_P << 4) + (opcode & 0x0f));
    register_I = 0xDEADBEEFUL; /* since we don't know what's in ram */
  } else {
    fprintf(codefile, "%sregister_I = ram[(register_P << 4) + 0x%02x]&0xff; /* set/mask new register_I */\n", cur_tabs, (opcode & 0x0F));
    register_I = 0xDEADBEEFUL;
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opLDIdir_B_BB (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLDIdir_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s%s", cur_tabs, "CINEBYTE temp_byte = (register_P << 4) + /* get ram page ... */\n");
  fprintf(codefile, "%s         (0x%02x & 0x0F); /* and imm half of ram addr.. */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_I = ram[temp_byte] & 0xFF; /* set/mask new register_I */\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  register_I = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}


CINESTATE opSTAdir_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSTAdir_A_A (%02x) */\n", cur_tabs, opcode);

  if (register_P != 0xDEADBEEFUL) {
    assert(register_P < 16); assert(register_P >= 0);
    register_I = (register_P << 4) + (opcode & 0x0F); /* snag imm value */;
    fprintf(codefile, "%sram[register_I = 0x%02lx] = ", cur_tabs, register_I);
  } else {
    fprintf(codefile, "%sram[register_I = (register_P << 4) + 0x%1x] = ", cur_tabs, (opcode & 0x0F));
    register_I = 0xDEADBEEFUL;
  }

  if (register_A != 0xDEADBEEFUL) {
    assert((register_A & (~0xFFF)) == 0);
    fprintf(codefile, "0x%03lx; /* store acc to RAM */\n", register_A);
  } else {
    fprintf(codefile, "register_A; /* store acc to RAM */\n");
  }

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opSTAdir_B_BB (int opcode)
{
  internal_test(opcode);

/* to be optimised as above */

  if (disp_opcodes) fprintf(codefile, "%s/* opSTAdir_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sram[register_I = (register_P << 4) + 0x%01x] = register_B; /* set I register and store B to ram */\n", cur_tabs, (opcode & 0x0F));
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  register_I = 0xDEADBEEFUL;
  return state = state_BB;
}


CINESTATE opSTAirg_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSTAirg_A_A (%02x) */\n", cur_tabs, opcode);
  if (register_I != 0xDEADBEEFUL) {
    if (register_A != 0xDEADBEEFUL) {
      fprintf(codefile, "%sram[0x%02lx] = 0x%03lx; /* store acc */\n", cur_tabs, register_I, register_A);
    } else {
      fprintf(codefile, "%sram[0x%02lx] = register_A; /* store acc */\n", cur_tabs, register_I);
    }
  } else {
    fprintf(codefile, "%s%s", cur_tabs, "ram[register_I] = register_A; /* store acc */\n");
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opSTAirg_B_BB (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opSTAirg_B_BB (%02x) */\n", cur_tabs, opcode);
  if (register_I != 0xDEADBEEFUL) {
    if (register_B != 0xDEADBEEFUL) {
      fprintf(codefile, "%sram[0x%02lx] = 0x%03lx; /* store acc */\n", cur_tabs, register_I, register_B);
    } else {
//fprintf(codefile, "%sif (register_I != 0x%02lx) ERROR(\"Error in I\\n\");\n", cur_tabs, register_I);
      fprintf(codefile, "%sram[0x%02lx] = register_B; /* store acc */\n", cur_tabs, register_I);
    }
  } else {
    fprintf(codefile, "%s%s", cur_tabs, "ram[register_I] = register_B; /* store acc */\n");
  }
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}


CINESTATE opXLT_A_AA (int opcode)
{
  internal_test(opcode);
  /* We should check the next opcode to see that it is a NOP.
     If not, need to look at this more closely */

  if (disp_opcodes) fprintf(codefile, "%s/* opXLT_A_AA (%02x) */\n", cur_tabs, opcode);
  if (comments) {
    fprintf(codefile, "%s%s", cur_tabs, "/*\n");
    fprintf(codefile, "%s%s", cur_tabs, " * XLT is weird; it loads the current accumulator with the bytevalue\n");
    fprintf(codefile, "%s%s", cur_tabs, " * at ROM location pointed to by the accumulator; this allows the\n");
    fprintf(codefile, "%s%s", cur_tabs, " * program to read the program itself..\n");
    fprintf(codefile, "%s%s", cur_tabs, " *		NOTE! Next opcode is *IGNORED!* because of a twisted side-effect\n");
    fprintf(codefile, "%s%s", cur_tabs, " */\n");
  }
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);

register_A = 0xDEADBEEFUL;
register_I = 0xDEADBEEFUL;
register_P = 0xDEADBEEFUL;

  if (register_A != 0xDEADBEEFUL) {
    fprintf(codefile, "%scmp_old = register_A; register_A = cmp_new = 0x%02lx; /* new acc value */\n", cur_tabs, register_A = rom[(register_PC & 0xF000) | register_A]);
    if (debug) fprintf(codefile, "%sif (debug) fprintf(stderr, \"%%04x A set using XLT but target known at compile time?\\n\");\n", cur_tabs);
  } else {
    if (debug) fprintf(codefile, "%sif (debug) fprintf(stderr, \"%%04x A set to 0x%%03x from rom[0x%%04x]\\n\", register_PC, rom[(register_PC & 0xF000) | register_A], (register_PC & 0xF000) | register_A);\n", cur_tabs);
    if ((register_PC & 0xF000) != 0) {
      fprintf(codefile, "%scmp_old = register_A; register_A = cmp_new = rom[0x%04lx | register_A]; /* new acc value */\n", cur_tabs, register_PC & 0xF000);
    } else {
      fprintf(codefile, "%scmp_old = register_A; register_A = cmp_new = rom[register_A]; /* new acc value */\n", cur_tabs);
    }
  }
  if (rom[register_PC+1] == 0x5f /*NOP*/) {
    /* No point in making an explicit jump to the next instruction! */
    if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
    /* This note_state stuff is screwy.  get rid of it. */
  } else {
    fprintf(codefile, "%sregister_PC = 0x%04lx; /* bump PC twice because XLT is fucked up */\n", cur_tabs, register_PC = register_PC+2);
    romFlags[register_PC] |= TAG_JUMPTARGET_A;
    register_A = 0xDEADBEEFUL; /* FOR NOW, UNLESS BETTER OPTIMISED */
    fprintf(codefile, "%s%s;\n", cur_tabs, Jump());
  }
  return state = state_AA;
}

CINESTATE opXLT_B_AA (int opcode)
{
  assert(NOTUSED); /* Just as well, as this looks broken... */
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opXLT_B_AA (%02x) */\n", cur_tabs, opcode);

  fprintf(codefile, "%s%s", cur_tabs, "flag_C = acc_a0 = register_A; /* back up acc */\n");
  fprintf(codefile, "%sregister_B = cmp_new = rom[(register_PC & 0xF000) | (cmp_old = register_B)]; /* new acc value */\n", cur_tabs);
  fprintf(codefile, "%sregister_PC = 0x%04lx; /* bump PC twice because XLT is fucked up */\n", cur_tabs, register_PC = register_PC+2);
  romFlags[register_PC] |= TAG_JUMPTARGET_AA;

  register_B = 0xDEADBEEFUL; /* FOR NOW, UNLESS BETTER OPTIMISED */
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}


CINESTATE opMULirg_A_AA (int opcode)
{

/* WARNING: check the SAR and <<, >> logic here.
   I am 99% sure it is assuming 16 bit integers and THAT THIS PROCEDURE IS BUGGY!!!!!
   Unfortunately it's too complicated for me to correct without advice...
 */

  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opMULirg_A_AA (%02x) */\n", cur_tabs, opcode);

#ifdef NEVER // we're embedduing now
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = ram[register_I];\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B <<= 4; /* get sign bit 15 */\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B |= (register_A >> 8); /* bring in A high nibble */\n");
  fprintf(codefile, "%sregister_A = ((register_A & 0xFF) << 8) | (0x%02x); /* pick up opcode */\n", cur_tabs, opcode & 0xFF);
  fprintf(codefile, "%s%s", cur_tabs, "if (register_A & 0x100) /* 1bit shifted out? */ {\n");
  push_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "acc_a0 = register_A = (register_A >> 8) | ((register_B & 0xFF) << 8);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A >>= 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A &= 0xFFF;\n");
  fprintf(codefile, "%sregister_B = %s;\n", cur_tabs, SAR("register_B","4"));
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B & 0x0F;\n");
  fprintf(codefile, "%sregister_B = %s;\n", cur_tabs, SAR("register_B","1"));
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= 0xFFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = (register_B += cmp_new);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= 0xFFF;\n");
  pop_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "} else {\n");
  push_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "register_A = (register_A >> 8) | /* Bhigh | Alow */ ((register_B & 0xFF) << 8);\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = acc_a0 = register_A & 0xFFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = (cmp_old + cmp_new);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A >>= 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A &= 0xFFF;\n");
  fprintf(codefile, "%sregister_B = %s;\n", cur_tabs, SAR("register_B","5"));
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= 0xFFF;\n");
  pop_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
#else
  fprintf(codefile, "%s%s", "\t", "MUL();\n");
#endif
  register_A = 0xDEADBEEFUL;
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opMULirg_B_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);

  if (disp_opcodes) fprintf(codefile, "%s/* opMULirg_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = ram[register_I]; cmp_old = register_B; acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B = SEX(register_B);\n");
  fprintf(codefile, "%sregister_B = %s;\n", cur_tabs, SAR("register_B","1"));
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = (register_B + temp_word);\n");
  fprintf(codefile, "%s%s", cur_tabs, "if (register_A & 0x01) register_B = flag_C;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= 0xFFF;\n");
  register_A = 0xDEADBEEFUL;
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}


CINESTATE opLSRe_A_AA (int opcode)
{
  internal_test(opcode);

  if (disp_opcodes) fprintf(codefile, "%s/* opLSRe_A_AA (%02x) */\n", cur_tabs, opcode);
  if (comments) {
    fprintf(codefile, "%s%s", cur_tabs, "/*\n");
    fprintf(codefile, "%s%s", cur_tabs, " * EB; right shift pure; fill new bit with zero.\n");
    fprintf(codefile, "%s%s", cur_tabs, " */\n");
    fprintf(codefile, "%s%s", cur_tabs, "\n");
  }
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = 0x0BEB; cmp_old = acc_a0 = register_A; flag_C = (0x0BEB + register_A);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A >>= 1;\n");
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSRe_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSRe_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = 0x0BEB; acc_a0 = register_A; cmp_old = register_B; flag_C = (0x0BEB + register_B);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B >>= 1;\n");
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  return state = state_AA;
}

CINESTATE opLSRf_A_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSRf_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opLSRf 1\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSRf_B_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSRf_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opLSRf 2\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSLe_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSLe_A_AA (%02x) */\n", cur_tabs, opcode);
  if (comments) {
    fprintf(codefile, "%s%s", cur_tabs, "/*\n");
    fprintf(codefile, "%s%s", cur_tabs, " * EC; left shift pure; fill new bit with zero *\n");
    fprintf(codefile, "%s%s", cur_tabs, " */\n");
  }
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = 0x0CEC;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = (0x0CEC + register_A);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A = (register_A << 1) & 0x0FFF;\n");
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSLe_B_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSLe_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = 0x0CEC; acc_a0 = register_A; cmp_old = register_B; flag_C = (0x0CEC + register_B);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B = (register_B << 1) & 0xFFF;\n");
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSLf_A_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSLf_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opLSLf 1\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSLf_B_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSLf_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opLSLf 2\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opASRe_A_AA (int opcode)
{

/* WARNING: check the SAR and <<, >> logic here.  It may be assuming
   16 bit integers */

  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opASRe_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = 0xDED; cmp_old = flag_C = acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A = SEX(register_A); /* make signed */\n");
  fprintf(codefile, "%sregister_A = (%s) & 0xFFF;\n", cur_tabs, SAR("register_A","1"));
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opASRe_B_AA (int opcode)
{

/* WARNING: check the SAR and <<, >> logic here.  It may be assuming
   16 bit integers */

  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opASRe_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = 0x0DED;\n");
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B = SEX(register_B);\n");
  fprintf(codefile, "%sregister_B = (%s)&0xFFF;\n", cur_tabs, SAR("register_B","1"));
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opASRf_A_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opASRf_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opASRf 1\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opASRf_B_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opASRf_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opASRf 2\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opASRDe_A_AA (int opcode)
{

/* WARNING: check the SAR and <<, >> logic here.  It may be assuming
   16 bit integers */

  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opASRDe_A_AA (%02x) */\n", cur_tabs, opcode);
  if (comments) {
    fprintf(codefile, "%s%s", cur_tabs, "/*\n");
    fprintf(codefile, "%s%s", cur_tabs, " * Arithmetic shift right of D (A+B) .. B is high (sign bits).\n");
    fprintf(codefile, "%s%s", cur_tabs, " * divide by 2, but leave the sign bit the same. (ie: 1010 -> 1001)\n");
    fprintf(codefile, "%s%s", cur_tabs, " */\n");
  }
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = 0x0EEE; cmp_old = acc_a0 = register_A; flag_C = (0x0EEE + register_A);\n");
  /* My version */
  fprintf(codefile, "%s%s", cur_tabs, "register_A = (register_A >> 1) | ((register_B & 1) << 11);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B = (register_B >> 1) | (register_B & 0x800);\n");

  register_A = 0xDEADBEEFUL;
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opASRDe_B_AA (int opcode)
{
  assert(NOTUSED);
/* WARNING: check the SAR and <<, >> logic here.  It may be assuming
   16 bit integers */

  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opASRDe_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s%s", cur_tabs, "CINEWORD temp_word = 0x0EEE;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = temp_word;\n");
  fprintf(codefile, "%s%s", cur_tabs, "acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = (temp_word += register_B);\n");
#ifdef BUGGYORIGINAL
  fprintf(codefile, "%s%s", cur_tabs, "register_B <<= 4;\n");
  fprintf(codefile, "%sregister_B = %s;\n", cur_tabs, SAR("register_B","5"));
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= 0x0FFF;\n");
#else
  fprintf(codefile, "%s%s", cur_tabs, "register_B = SEX(register_B);\n");
  fprintf(codefile, "%sregister_B = %s;\n", cur_tabs, SAR("register_B","1"));
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= 0x0FFF;\n");
#endif
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  return state = state_AA;
}

CINESTATE opASRDf_A_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opASRDf_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opASRDf 1\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opASRDf_B_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opASRDf_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opASRDf 2\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSLDe_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSLDe_A_AA (%02x) */\n", cur_tabs, opcode);
  if (comments) {
    fprintf(codefile, "%s%s", cur_tabs, "/* LSLDe -- Left shift through both accumulators; lossy in middle. */\n");
  }
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = 0x0FEF; cmp_old = acc_a0 = register_A; flag_C = (0x0FEF + register_A);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A = (register_A << 1) & 0xFFF; register_B = (register_B << 1) & 0xFFF;\n");
  register_A = 0xDEADBEEFUL;
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSLDe_B_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSLDe_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opLSLD 1\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opLSLDf_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSLDf_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s%s", cur_tabs, "/* LSLDf */\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "CINEWORD temp_word = 0xFFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = temp_word;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = (temp_word + register_A);\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A <<= 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A &= 0x0FFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B <<= 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= 0x0FFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  register_A = 0xDEADBEEFUL;
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  return state = state_AA;
}

CINESTATE opLSLDf_B_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLSLDf_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s%s", cur_tabs, "/* not 'the same' as the A->AA version above */\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "CINEWORD temp_word = 0xFFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_new = temp_word;\n");
  fprintf(codefile, "%s%s", cur_tabs, "acc_a0 = register_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "cmp_old = register_B;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "flag_C = (temp_word + register_B);\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B <<= 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_B &= 0x0FFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  register_A = 0xDEADBEEFUL;
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  return state = state_AA;
}

CINESTATE opJMP_A_A (int opcode)
{
  internal_test(opcode);

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJMP_A_A (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s;\n", cur_tabs, Jump());

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");

  /* After a jump, next code is entirely unknown.
     We ought to handle this by setting flags on first pass, but
     for now, brute force and ignorance rules... */

        register_P = 0xDEADBEEFUL;
        register_I = 0xDEADBEEFUL;
        register_A = 0xDEADBEEFUL;
        register_B = 0xDEADBEEFUL;

  return state = state_A;
}

CINESTATE opJMP_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);

  romFlags[register_PC] |= TAG_JUMPINST;

  /* Todo: steal code from opJMP_A_A */
  if (disp_opcodes) fprintf(codefile, "%s/* opJMP_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s;\n", cur_tabs, Jump());
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJEI_A_A (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJEI_A_A (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
#ifdef NEVER
  fprintf(codefile, "%s%s", cur_tabs, "FromX = SEX(FromX);\n");
  fprintf(codefile, "%s%s", cur_tabs, "if (!(CCPU_READPORT (CCPU_PORT_IOOUTPUTS) & 0x80))\n");
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s  if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKY) - (CINESWORD)FromX) < 0x800) %s /* Should this be '> 0' ? */\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "} else {\n");
  fprintf(codefile, "%s  if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKX) - (CINESWORD)FromX) < 0x800) %s /* > 0 ? */\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
#else
  /* I DO NOT KNOW HOW THIS IS USED.  It appears to be used only once,
     and has no effect whether the branch is take or not..? */
#endif
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opJEI_B_BB (int opcode)
{
  int savedpc = register_PC;
  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJEI_B_BB (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  fprintf(codefile, "%s%s", cur_tabs, "FromX = SEX(FromX);\n");
  fprintf(codefile, "%s%s", cur_tabs, "if (!(CCPU_READPORT (CCPU_PORT_IOOUTPUTS) & 0x80))\n");
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s  if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKY) - (CINESWORD)FromX) < 0x800) %s /* > 0 ? */\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "} else {\n");
  fprintf(codefile, "%s  if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKX) - (CINESWORD)FromX) < 0x800) %s /* > 0 ? */\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJEI_A_B (int opcode)
{
  int savedpc = register_PC;

  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJEI_A_B (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  fprintf(codefile, "%s%s", cur_tabs, "FromX = SEX(FromX);\n");
  fprintf(codefile, "%s%s", cur_tabs, "if (!(CCPU_READPORT (CCPU_PORT_IOOUTPUTS) & 0x80)) {\n");
  fprintf(codefile, "%s  if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKY) - (CINESWORD)FromX) < 0x800) %s /* > 0? */\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "} else {\n");
  fprintf(codefile, "%s  if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKX) - (CINESWORD)FromX) < 0x800) %s /* > 0 ? */\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

/* WARNING: THESE ARE THE FUNNY ONES SPECIFICALLY FOR TAILGUNNER */

  /* These instructions require the saved 'cmp_new' from a previous
     instruction.  Therefore on the first pass, we can tag that
     previous instruction so that it remembers the cmp_new value.
     Actually a better thing is to tag this instruction, and have the
     previous instruction do a lookahead test when it generates
     its code */

  /* Not we have not yet optimised the saving of cmp_* so it's not
     yet necessary to set these flags. */

CINESTATE opJMI_A_A (int opcode)
{
  int savedpc = register_PC;

  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;
  
  romFlags[register_PC] |= TAG_JUMPINST;

  if (pass == PASS_1) {
    romFlags[register_PC] |= (TAG_REQUIRES_CMP_OLD | TAG_REQUIRES_CMP_NEW);
  }

  if (disp_opcodes) fprintf(codefile, "%s/* opJMI_A_A (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "/*\n");
  fprintf(codefile, "%s%s", cur_tabs, " * previous instruction was not an ACC instruction, nor was the\n");
  fprintf(codefile, "%s%s", cur_tabs, " * instruction twice back a USB, therefore minus flag test the\n");
  fprintf(codefile, "%s%s", cur_tabs, " * current A-reg\n");
  fprintf(codefile, "%s%s", cur_tabs, " */\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "/* negative acc? */\n");
  fprintf(codefile, "%sif (register_A & 0x800) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opJMI_AA_A (int opcode)
{
  int savedpc = register_PC;

  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (pass == PASS_1) {
    romFlags[register_PC] |= (TAG_REQUIRES_CMP_OLD | TAG_REQUIRES_CMP_NEW);
  }

  if (disp_opcodes) fprintf(codefile, "%s/* opJMI_AA_A (%02x) */\n", cur_tabs, opcode);
  if (comments) fprintf(codefile, "%s%s", cur_tabs, "/* previous acc negative? Jump if so... */\n");
  fprintf(codefile, "%sif (cmp_old & 0x800) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opJMI_BB_A (int opcode)
{
  int savedpc = register_PC;

  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (pass == PASS_1) {
    romFlags[register_PC] |= (TAG_REQUIRES_CMP_OLD | TAG_REQUIRES_CMP_NEW);
  }

  if (disp_opcodes) fprintf(codefile, "%s/* opJMI_BB_A (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (register_B & 0x800) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opJMI_B_BB (int opcode)
{
  int savedpc = register_PC;

  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (pass == PASS_1) {
    romFlags[register_PC] |= (TAG_REQUIRES_CMP_OLD | TAG_REQUIRES_CMP_NEW);
  }

  if (disp_opcodes) fprintf(codefile, "%s/* opJMI_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (register_A & 0x800) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

/* I am changing this from signed to unsigned as a test. */
CINESTATE opJLT_A_A (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (pass == PASS_1) {
    romFlags[register_PC] |= (TAG_REQUIRES_CMP_OLD | TAG_REQUIRES_CMP_NEW);
  }

  if (disp_opcodes) fprintf(codefile, "%s/* opJLT_A_A (%02x) */\n", cur_tabs, opcode);
  if (comments) fprintf(codefile, "%s%s", cur_tabs, "/* jump if old acc less than new acc */\n");
  /*fprintf(codefile, "%sif (((cmp_new|cmp_old)>>12) != 0) ERROR(\"cmp_new/old out of range\");\n", cur_tabs);*/
  fprintf(codefile, "%sif (cmp_new < cmp_old) %s;\n", cur_tabs, Jump());
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opJLT_B_BB (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (pass == PASS_1) {
    romFlags[register_PC] |= (TAG_REQUIRES_CMP_OLD | TAG_REQUIRES_CMP_NEW);
  }

  if (disp_opcodes) fprintf(codefile, "%s/* opJLT_B_BB (%02x) */\n", cur_tabs, opcode);
/*  fprintf(codefile, "%s%s", cur_tabs, "if (SEX(cmp_new) < SEX(cmp_old))\n");*/
  /*fprintf(codefile, "%sif (((cmp_new|cmp_old)>>12) != 0) ERROR(\"cmp_new/old out of range\");\n", cur_tabs);*/
  fprintf(codefile, "%sif (cmp_new < cmp_old) %s;\n", cur_tabs, Jump());
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJEQ_A_A (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (pass == PASS_1) {
    romFlags[register_PC] |= (TAG_REQUIRES_CMP_OLD | TAG_REQUIRES_CMP_NEW);
  }

  if (disp_opcodes) fprintf(codefile, "%s/* opJEQ_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (cmp_new == cmp_old) %s;\n", cur_tabs, Jump());
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opJEQ_B_BB (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (pass == PASS_1) {
    romFlags[register_PC] |= (TAG_REQUIRES_CMP_OLD | TAG_REQUIRES_CMP_NEW);
  }

  if (disp_opcodes) fprintf(codefile, "%s/* opJEQ_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (cmp_new == cmp_old) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJA0_A_A (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  /* I am not convinced that we need this extra acc_a0 variable
     *as well as* an extra carry-flag remembering variable  */

  if (disp_opcodes) fprintf(codefile, "%s/* opJA0_A_A (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (acc_a0 & 0x01) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opJA0_B_BB (int opcode)
{
  int savedpc = register_PC;

  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJA0_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (acc_a0 & 0x01) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJNC_A_A (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

/* BUG: explore changes *all* the registers :-( */
  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJNC_A_AA (%02x) */\n", cur_tabs, opcode);
  /* Use this opcode to test new carry and new jump code */
  fprintf(codefile, "%sif (!(flag_C & CARRYBIT)) %s;\n", cur_tabs, Jump());
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");

  return state = state_A;
}

CINESTATE opJNC_B_BB (int opcode)
{
  int savedpc = register_PC;

  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJNC_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (!(flag_C & CARRYBIT)) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJDR_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opJDR_A_A (%02x) */\n", cur_tabs, opcode);
  if (comments) fprintf(codefile, "%s%s", cur_tabs, "/* We don't need to simulate this too accurately */\n");
  register_A = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opJDR_B_BB (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opJDR_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "/* We don't need to simulate this too accurately */\n");
  register_B = 0xDEADBEEFUL;
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opNOP_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opNOP_A_A (%02x) */\n", cur_tabs, opcode);
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opNOP_B_BB (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opNOP_B_BB (%02x) */\n", cur_tabs, opcode);
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJPP32_A_B (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);

  romFlags[register_PC] |= TAG_JUMPINST;

  /* AHA!  Is this the *only* way we can jump between rom pages?
     If so, then can surely clean up the code of all the other jumps */

  if (disp_opcodes) fprintf(codefile, "%s/* opJPP32_A_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "/*\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 00 = Offset 0000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 01 = Offset 1000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 02 = Offset 2000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 03 = Offset 3000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 04 = Offset 4000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 05 = Offset 5000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 06 = Offset 6000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 07 = Offset 7000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " */\n");

/* Need to inline and set tags */

  fprintf(codefile, "%s%s", cur_tabs, "register_PC = register_J + ((register_P & 0x07) << 12);\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJPP32_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJPP32_B_BB (%02x) */\n", cur_tabs, opcode);

/* Need to inline and set tags */

  fprintf(codefile, "%s%s", cur_tabs, "register_PC = register_J + ((register_P & 0x07) << 12);\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJPP16_A_B (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJPP16_A_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "/*\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 00 = Offset 0000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 01 = Offset 1000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 02 = Offset 2000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " * 03 = Offset 3000h\n");
  fprintf(codefile, "%s%s", cur_tabs, " */\n");

/* Need to inline and set tags */

  fprintf(codefile, "%s%s", cur_tabs, "register_PC = register_J + ((register_P & 0x03) << 12);\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJPP16_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJPP16_B_BB (%02x) */\n", cur_tabs, opcode);

/* Need to inline and set tags */

  fprintf(codefile, "%s%s", cur_tabs, "register_PC = register_J + ((register_P & 0x03) << 12);\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJMP_A_B (int opcode)
{
  internal_test(opcode);

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJMP_A_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s;\n", cur_tabs, Jump());
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJPP8_A_B (int opcode)
{
  int savedpc = register_PC;

  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJPP8_A_B (%02x) */\n", cur_tabs, opcode);
  if (comments) {
    fprintf(codefile, "%s%s", cur_tabs, "/*\n");
    fprintf(codefile, "%s%s", cur_tabs, " * \"long jump\"; combine P and J to jump to a new far location (that can\n");
    fprintf(codefile, "%s%s", cur_tabs, " *	be more than 12 bits in address). After this jump, further jumps\n");
    fprintf(codefile, "%s%s", cur_tabs, " * are local to this new page.\n");
    fprintf(codefile, "%s%s", cur_tabs, " */\n");
  }
  if (register_P != 0xDEADBEEFUL) {
    if ((register_P != 1) && (register_P != 2)) {
      fprintf(stderr, "%04lx: Translated jump to a wrong page: P = 0x%02lx\n",
        register_PC, register_P);
      exit(1);
    }
    if (register_J != 0xDEADBEEFUL) {
      fprintf(codefile, "%sregister_PC = 0x%04lx; /* rom offset */\n", cur_tabs,
        register_PC = (register_J + (((register_P & 0x03) - 1) << 12)) & 0xFFFF);
fprintf(stderr, "Setting TAG_JUMPTARGET_B bit on 0x%04lx\n", register_PC);
      romFlags[register_PC] |= TAG_JUMPTARGET_B;
      explore(register_PC, endAdr);
    } else {
      fprintf(codefile, "%sregister_PC = register_J + 0x%04lx; /* rom offset */\n", cur_tabs, ((register_P & 0x03) - 1) << 12);
      register_PC = 0xDEADBEEFUL;                                                                             /* WHY -1 ??????????? */
      fprintf(codefile, "%s%s", cur_tabs, "/* WARNING: UNKNOWN JUMP DESTINATION - MAY FOUL UP CODE OPTIMISATIONS */\n");
    }
  } else {
    fprintf(codefile, "%s%s", cur_tabs, "register_PC = register_J + (((register_P & 0x03) - 1) << 12);\n");
  }
#ifdef NEVER /* Bug? */
if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B; /* Does this hold true after a jump? */\n");
#endif

  fprintf(codefile, "%s%s;\n", cur_tabs, Jump());

/* BUG fix. Or bodge? */
  return state = state; /* Should be state_unknown */
  /* SAME 'FIX' NEEDS TO BE APPLIED TO ALL UNCONDITIONAL JUMPS!!!! */
  /*return state = state_B;*/
}

CINESTATE opJPP8_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJPP8_B_BB (%02x) */\n", cur_tabs, opcode);

/* Need to inline and set tags */

  if (register_P != 0xDEADBEEFUL) {
    if ((register_P != 1) && (register_P != 2)) {
      fprintf(stderr, "Translated jump to a wrong page: P = 0x%02lx\n", register_P);
      exit(1);
    }
    /* insert optimised expansion here and explore() it! */
  }
  fprintf(codefile, "%s%s", cur_tabs, "register_PC = register_J + (((register_P & 0x03) - 1) << 12);\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opJMI_A_B (int opcode)
{
  int savedpc = register_PC;
  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJMI_A_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (register_A & 0x800) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJMI_AA_B (int opcode)
{
  int savedpc = register_PC;
  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJMI_AA_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opJMI 3\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJMI_BB_B (int opcode)
{
  int savedpc = register_PC;
  assert(NOTUSED);
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJMI_BB_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opJMI 4\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJLT_A_B (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJLT_A_B (%02x) */\n", cur_tabs, opcode);
  /* fprintf(codefile, "%s%s", cur_tabs, "if (SEX(cmp_new) < SEX(cmp_old))\n"); */
  /*fprintf(codefile, "%sif (((cmp_new|cmp_old)>>12) != 0) ERROR(\"cmp_new/old out of range\");\n", cur_tabs);*/
  fprintf(codefile, "%sif (cmp_new < cmp_old) %s;\n", cur_tabs, Jump()); /* Pretty sure this is a bugfix */
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJEQ_A_B (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJEQ_A_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (cmp_new == cmp_old) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJA0_A_B (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJA0_A_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (acc_a0 & A0BIT) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJNC_A_B (int opcode)
{
  int savedpc = register_PC;
  internal_test(opcode);

  explore(register_PC+1, endAdr); register_PC = savedpc;

  romFlags[register_PC] |= TAG_JUMPINST;

  if (disp_opcodes) fprintf(codefile, "%s/* opJNC_A_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%sif (!(flag_C & CARRYBIT)) %s;\n", cur_tabs, Jump());
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opJDR_A_B (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opJDR_A_B (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "/* register_PC++; */\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}


CINESTATE opNOP_A_B (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opNOP_A_B (%02x) */\n", cur_tabs, opcode);
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_B;\n");
  return state = state_B;
}

CINESTATE opLLT_A_AA (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLLT_A_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "{CINEBYTE temp_byte = 0;\n");
  push_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "for (;;) {\n");
  push_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "if (   (((register_A >> 8) & 0x0A) && (((register_A >> 8) & 0x0A) ^ 0x0A))\n");
  fprintf(codefile, "%s%s", cur_tabs, "  ||   (((register_B >> 8) & 0x0A) && (((register_B >> 8) & 0x0A) ^ 0x0A))  ) break;\n");
  fprintf(codefile, "%s%s", cur_tabs, "register_A <<= 1; register_B <<= 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "if (!(++temp_byte)) break /* This may not be correct */;\n");
  pop_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  fprintf(codefile, "%s%s", cur_tabs, "vgShiftLength = temp_byte & 0xfff; register_A &= 0x0FFF; register_B &= 0x0FFF;\n");

  register_A = 0xDEADBEEFUL;
  register_B = 0xDEADBEEFUL;

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  pop_indent("  ");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  return state = state_AA;
}

CINESTATE opLLT_B_AA (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opLLT_B_AA (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opLLT 1\");\n");

  register_A = 0xDEADBEEFUL;
  register_B = 0xDEADBEEFUL;

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_AA;\n");
  return state = state_AA;
}

CINESTATE opVIN_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opVIN_A_A (%02x) */\n", cur_tabs, opcode);
  if (comments) fprintf(codefile, "%s%s", cur_tabs, "/* set the starting address of a vector */\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "FromX = register_A & 0xFFF; /* regA goes to x-coord */\n");
  fprintf(codefile, "%s%s", cur_tabs, "FromY = register_B & 0xFFF; /* regB goes to y-coord */\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opVIN_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opVIN_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "FromX = register_A & 0xFFF; /* regA goes to x-coord */\n");
  fprintf(codefile, "%s%s", cur_tabs, "FromY = register_B & 0xFFF; /* regB goes to y-coord */\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE opWAI_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opWAI_A_A (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "/* wait for a tick on the watchdog */\n");
  if (debug) fprintf(codefile, "%s%s", cur_tabs, "fprintf(stderr, \"%04x ClearScreen();\\n\", register_PC);\n");
  fprintf(codefile, "#ifndef DUALCPU\n");
  fprintf(codefile, "%s%s", cur_tabs, "CinemaClearScreen();\n");
  fprintf(codefile, "%s%s", cur_tabs, "bNewFrame = 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "bailOut = TRUE;\n");
  fprintf(codefile, "#endif\n");
/* some dodgy code here */

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");

  fprintf(codefile, "%sregister_PC = 0x%04lx;\n", cur_tabs, register_PC = register_PC+1);
  romFlags[register_PC] |= TAG_JUMPTARGET_A;

  fprintf(codefile, "%s%s; /* NOT REALLY A JUMP - ACTUALLY FOR GETTING BACK TO POLLING LOOP - NEEDS WORK */\n", cur_tabs, return_or_continue_or_break);

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  return state = state_A;
}

CINESTATE opWAI_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opWAI_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "CinemaClearScreen();\n");
  fprintf(codefile, "%s%s", cur_tabs, "bNewFrame = 1;\n");
  fprintf(codefile, "%s%s", cur_tabs, "bailOut = TRUE;\n");

/* some more dodgy code here */

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state;\n");

  fprintf(codefile, "%sregister_PC = 0x%04lx;\n", cur_tabs, register_PC = register_PC+1);
  romFlags[register_PC] |= TAG_JUMPTARGET_A;

  fprintf(codefile, "%s%s; /* NOT REALLY A JUMP - ACTUALLY FOR GETTING BACK TO POLLING LOOP - NEEDS WORK */\n", cur_tabs, return_or_continue_or_break);

  return state = state_BB;
}

CINESTATE opVDR_A_A (int opcode)
{
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opVDR_A_A (%02x) */\n", cur_tabs, opcode);
  if (slave_pc) fprintf(codefile, "%sregister_PC = 0x%04lx /* Force consistency */;\n", cur_tabs, register_PC);
  fprintf(codefile, "%s%s", cur_tabs, "{\n");
  fprintf(codefile, "%s%s", cur_tabs, "/* set ending points and draw the vector, or buffer for a later draw. */\n");
  fprintf(codefile, "%s%s", cur_tabs, "int ToX = register_A & 0xFFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "int ToY = register_B & 0xFFF;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");

  fprintf(codefile, "%s%s", cur_tabs, "/* Sign extend from 20 bit CCPU to 32bit target machine */\n");
  fprintf(codefile, "%s%s", cur_tabs, "FromX = SEX(FromX);\n");
  fprintf(codefile, "%s%s", cur_tabs, "ToX = SEX(ToX);\n");
  fprintf(codefile, "%s%s", cur_tabs, "FromY = SEX(FromY);\n");
  fprintf(codefile, "%s%s", cur_tabs, "ToY = SEX(ToY);\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");

  fprintf(codefile, "%s%s", cur_tabs, "/* figure out the vector */\n");
  fprintf(codefile, "%s%s", cur_tabs, "ToX -= FromX;\n");
  fprintf(codefile, "%sToX = %s;\n", cur_tabs, SAR("ToX","vgShiftLength"));
  fprintf(codefile, "%s%s", cur_tabs, "ToX += FromX;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "ToY -= FromY;\n");
  fprintf(codefile, "%sToY = %s;\n", cur_tabs, SAR("ToY","vgShiftLength"));
  fprintf(codefile, "%s%s", cur_tabs, "ToY += FromY;\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  fprintf(codefile, "%s%s", cur_tabs, "/* render the line */\n");
  fprintf(codefile, "#ifndef DUALCPU\n");
  fprintf(codefile, "%s%s", cur_tabs, "CinemaVectorData (FromX, FromY, ToX, ToY, vgColour);\n");
  fprintf(codefile, "#endif\n");
  fprintf(codefile, "%s%s", cur_tabs, "\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_A;\n");
  fprintf(codefile, "%s%s", cur_tabs, "}\n");
  return state = state_A;
}

CINESTATE opVDR_B_BB (int opcode)
{
  assert(NOTUSED);
  internal_test(opcode);
  if (disp_opcodes) fprintf(codefile, "%s/* opVDR_B_BB (%02x) */\n", cur_tabs, opcode);
  fprintf(codefile, "%s%s", cur_tabs, "UNFINISHED (\"opVDR B 1\");\n");
  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "state = state_BB;\n");
  return state = state_BB;
}

CINESTATE tJPP_A_B (int opcode)
{
  internal_test(opcode);

  switch (ccpu_msize) {

  case CCPU_MEMSIZE_4K:
  case CCPU_MEMSIZE_8K:
    return opJPP8_A_B (opcode);

  case CCPU_MEMSIZE_16K:
    return opJPP16_A_B (opcode);

  case CCPU_MEMSIZE_32K:
    return opJPP32_A_B (opcode);

  default:
    fprintf(codefile, "%s%s", cur_tabs, "fprintf (stderr, \"Out of range JPP!\\n\");\n");
    return opJPP32_A_B (opcode);

  }

}

CINESTATE tJPP_B_BB (int opcode)
{
  internal_test(opcode);

  switch (ccpu_msize)
  {
    case CCPU_MEMSIZE_4K:
    case CCPU_MEMSIZE_8K:
      return opJPP8_B_BB (opcode);
    case CCPU_MEMSIZE_16K:
      return opJPP16_B_BB (opcode);
    case CCPU_MEMSIZE_32K:
      return opJPP32_B_BB (opcode);
    default:
      fprintf(stderr, "Out of range JPP!\n");
  }

  if (require_note_state(register_PC)) fprintf(codefile, "%s%s", cur_tabs, "/* state = state; */\n");
  return state = state;
}



CINESTATE tJMI_A_B (int opcode)
{
  internal_test(opcode);
  /* again, all compile-time tests */

  return (ccpu_jmi_dip) ? opJMI_A_B (opcode) : opJEI_A_B (opcode);
}

CINESTATE tJMI_A_A (int opcode)
{
  internal_test(opcode);

  return (ccpu_jmi_dip) ? opJMI_A_A (opcode) : opJEI_A_A (opcode);
}

CINESTATE tJMI_AA_B (int opcode)
{
  internal_test(opcode);

  return (ccpu_jmi_dip) ? opJMI_AA_B (opcode) : opJEI_AA_B (opcode);
}

CINESTATE tJMI_AA_A (int opcode)
{
  internal_test(opcode);

  return (ccpu_jmi_dip) ? opJMI_AA_A (opcode) : opJEI_A_A (opcode);
}

CINESTATE tJMI_B_BB1 (int opcode)
{
  internal_test(opcode);

  return (ccpu_jmi_dip) ? opJMI_B_BB (opcode) : opJEI_B_BB (opcode);
}

CINESTATE tJMI_BB_B (int opcode)
{
  internal_test(opcode);

  return (ccpu_jmi_dip) ? opJMI_BB_B (opcode) : opJEI_A_B (opcode);
}

CINESTATE tJMI_BB_A (int opcode)
{
  internal_test(opcode);

  return (ccpu_jmi_dip) ? opJMI_BB_A (opcode) : opJEI_A_A (opcode);
}


CINESTATE tOUT_A_A (int opcode)
{
  internal_test(opcode);


  switch (ccpu_monitor) { /* Known at translate time */

  case CCPU_MONITOR_16LEV:
    return opOUT16_A_A (opcode);

  case CCPU_MONITOR_64LEV:
    return opOUT64_A_A (opcode);

  case CCPU_MONITOR_WOWCOL:
    return opOUTWW_A_A (opcode);

  default: /* This for tailgunner */
    return opOUTbi_A_A (opcode);
  }
}

CINESTATE tOUT_B_BB (int opcode)
{
  internal_test(opcode);
  /* see above */


  switch (ccpu_monitor)
  {

    /* These three are in error if tailgunner */
    case CCPU_MONITOR_16LEV:
      return opOUT16_B_BB (opcode);
    case CCPU_MONITOR_64LEV:
      return opOUT64_B_BB (opcode);
    case CCPU_MONITOR_WOWCOL:
      return opOUTWW_B_BB (opcode);

    default: /* Tailgunner */
      return opOUTbi_B_BB (opcode);
  }
}
