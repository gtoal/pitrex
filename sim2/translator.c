/*
 * translator.c: 6502 to C translator
 *
 * Copyright 1993, 2003 Eric Smith
 *
 * $Id: translator.c,v 1.1 2018/07/31 01:19:45 pi Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "game.h"
#include "memory.h"

char MODE_NONE  [] = "                   ";
char MODE_IMM   [] = "                   ";
char MODE_REL   [] = "                   ";
char MODE_ABS   [] = "TR_ABS(0x%04x);    ";
char MODE_ABS_X [] = "TR_ABS_X(0x%04x);  ";
char MODE_ABS_Y [] = "TR_ABS_Y(0x%04x);  ";
char MODE_ZP    [] = "TR_ZP(0x%02x);       ";
char MODE_ZP_X  [] = "TR_ZP_X(0x%02x);     ";
char MODE_ZP_Y  [] = "TR_ZP_Y(0x%02x);     ";
char MODE_IND_X [] = "TR_IND_X(0x%02x);    ";
char MODE_IND_Y [] = "TR_IND_Y(0x%02x);    ";
char MODE_IND   [] = "TR_IND(0x%04x);    ";

char OP_ADC  [] = "DO_ADC;        ";
char OP_ADCI [] = "DO_ADCI(0x%02x); ";
char OP_AND  [] = "DO_AND;        ";
char OP_ANDI [] = "DO_ANDI(0x%02x); ";
char OP_ASL  [] = "DO_ASL;        ";
char OP_ASLA [] = "DO_ASLA;       ";
char OP_BCC  [] = "TR_BCC(0x%04x);";
char OP_BCS  [] = "TR_BCS(0x%04x);";
char OP_BEQ  [] = "TR_BEQ(0x%04x);";
char OP_BIT  [] = "DO_BIT;        ";
char OP_BMI  [] = "TR_BMI(0x%04x);";
char OP_BNE  [] = "TR_BNE(0x%04x);";
char OP_BPL  [] = "TR_BPL(0x%04x);";
char OP_BRK  [] = "TR_BRK;        ";
char OP_BVC  [] = "TR_BVC(0x%04x);";
char OP_BVS  [] = "TR_BVS(0x%04x);";
char OP_CLC  [] = "DO_CLC;        ";
char OP_CLD  [] = "DO_CLD;        ";
char OP_CLI  [] = "DO_CLI;        ";
char OP_CLV  [] = "DO_CLV;        ";
char OP_CMP  [] = "DO_CMP;        ";
char OP_CMPI [] = "DO_CMPI(0x%02x); ";
char OP_CPX  [] = "DO_CPX;        ";
char OP_CPXI [] = "DO_CPXI(0x%02x); ";
char OP_CPY  [] = "DO_CPY;        ";
char OP_CPYI [] = "DO_CPYI(0x%02x); ";
char OP_DEC  [] = "DO_DEC;        ";
char OP_DEX  [] = "DO_DEX;        ";
char OP_DEY  [] = "DO_DEY;        ";
char OP_EOR  [] = "DO_EOR;        ";
char OP_EORI [] = "DO_EORI(0x%02x); ";
char OP_INC  [] = "DO_INC;        ";
char OP_INX  [] = "DO_INX;        ";
char OP_INY  [] = "DO_INY;        ";
char OP_JMP  [] = "TR_JMP;        ";
char OP_JSR  [] = "TR_JSR(0x%04x);";
char OP_LDA  [] = "DO_LDA;        ";
char OP_LDAI [] = "DO_LDAI(0x%02x); ";
char OP_LDX  [] = "DO_LDX;        ";
char OP_LDXI [] = "DO_LDXI(0x%02x); ";
char OP_LDY  [] = "DO_LDY;        ";
char OP_LDYI [] = "DO_LDYI(0x%02x); ";
char OP_LSR  [] = "DO_LSR;        ";
char OP_LSRA [] = "DO_LSRA;       ";
char OP_NOP  [] = "DO_NOP;        ";
char OP_ORA  [] = "DO_ORA;        ";
char OP_ORAI [] = "DO_ORAI(0x%02x); ";
char OP_PHA  [] = "DO_PHA;        ";
char OP_PHP  [] = "DO_PHP;        ";
char OP_PLA  [] = "DO_PLA;        ";
char OP_PLP  [] = "DO_PLP;        ";
char OP_ROL  [] = "DO_ROL;        ";
char OP_ROLA [] = "DO_ROLA;       ";
char OP_ROR  [] = "DO_ROR;        ";
char OP_RORA [] = "DO_RORA;       ";
char OP_RTI  [] = "TR_RTI;        ";
char OP_RTS  [] = "TR_RTS;        ";
char OP_SBC  [] = "DO_SBC;        ";
char OP_SBCI [] = "DO_SBCI(0x%02x); ";
char OP_SEC  [] = "DO_SEC;        ";
char OP_SED  [] = "DO_SED;        ";
char OP_SEI  [] = "DO_SEI;        ";
char OP_STA  [] = "DO_STA;        ";
char OP_STX  [] = "DO_STX;        ";
char OP_STY  [] = "DO_STY;        ";
char OP_TAX  [] = "DO_TAX;        ";
char OP_TAY  [] = "DO_TAY;        ";
char OP_TSX  [] = "DO_TSX;        ";
char OP_TXA  [] = "DO_TXA;        ";
char OP_TXS  [] = "DO_TXS;        ";
char OP_TYA  [] = "DO_TYA;        ";

typedef struct
{
  char *op;
  char *mode;
  int bytes;
  int cycles;
  int branch;
  int end;
} Optab;

Optab optab [256] =
{
  { /* 0x00 */ OP_BRK,  MODE_NONE,  1, 7, 0, 1 },
  { /* 0x01 */ OP_ORA,  MODE_IND_X, 2, 6, 0, 0 },
  { /* 0x02 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x03 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x04 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x05 */ OP_ORA,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0x06 */ OP_ASL,  MODE_ZP,    2, 5, 0, 0 },
  { /* 0x07 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x08 */ OP_PHP,  MODE_NONE,  1, 3, 0, 0 },
  { /* 0x09 */ OP_ORAI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0x0a */ OP_ASLA, MODE_NONE,  1, 2, 0, 0 },
  { /* 0x0b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x0c */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x0d */ OP_ORA,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0x0e */ OP_ASL,  MODE_ABS,   3, 6, 0, 0 },
  { /* 0x0f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x10 */ OP_BPL,  MODE_REL,   2, 2, 1, 0 },
  { /* 0x11 */ OP_ORA,  MODE_IND_Y, 2, 5, 0, 0 },
  { /* 0x12 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x13 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x14 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x15 */ OP_ORA,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0x16 */ OP_ASL,  MODE_ZP_X,  2, 6, 0, 0 },
  { /* 0x17 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x18 */ OP_CLC,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0x19 */ OP_ORA,  MODE_ABS_Y, 3, 4, 0, 0 },
  { /* 0x1a */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x1b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x1c */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x1d */ OP_ORA,  MODE_ABS_X, 3, 4, 0, 0 },
  { /* 0x1e */ OP_ASL,  MODE_ABS_X, 3, 7, 0, 0 },
  { /* 0x1f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x20 */ OP_JSR,  MODE_ABS,   3, 6, 1, 0 },
  { /* 0x21 */ OP_AND,  MODE_IND_X, 2, 6, 0, 0 },
  { /* 0x22 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x23 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x24 */ OP_BIT,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0x25 */ OP_AND,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0x26 */ OP_ROL,  MODE_ZP,    2, 5, 0, 0 },
  { /* 0x27 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x28 */ OP_PLP,  MODE_NONE,  1, 4, 0, 0 },
  { /* 0x29 */ OP_ANDI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0x2a */ OP_ROLA, MODE_NONE,  1, 2, 0, 0 },
  { /* 0x2b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x2c */ OP_BIT,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0x2d */ OP_AND,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0x2e */ OP_ROL,  MODE_ABS,   3, 6, 0, 0 },
  { /* 0x2f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x30 */ OP_BMI,  MODE_REL,   2, 2, 1, 0 },
  { /* 0x31 */ OP_AND,  MODE_IND_Y, 2, 5, 0, 0 },
  { /* 0x32 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x33 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x34 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x35 */ OP_AND,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0x36 */ OP_ROL,  MODE_ZP_X,  2, 6, 0, 0 },
  { /* 0x37 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x38 */ OP_SEC,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0x39 */ OP_AND,  MODE_ABS_Y, 3, 4, 0, 0 },
  { /* 0x3a */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x3b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x3c */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x3d */ OP_AND,  MODE_ABS_X, 3, 4, 0, 0 },
  { /* 0x3e */ OP_ROL,  MODE_ABS_X, 3, 7, 0, 0 },
  { /* 0x3f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x40 */ OP_RTI,  MODE_NONE,  1, 6, 0, 1 },
  { /* 0x41 */ OP_EOR,  MODE_IND_X, 2, 6, 0, 0 },
  { /* 0x42 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x43 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x44 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x45 */ OP_EOR,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0x46 */ OP_LSR,  MODE_ZP,    2, 5, 0, 0 },
  { /* 0x47 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x48 */ OP_PHA,  MODE_NONE,  1, 3, 0, 0 },
  { /* 0x49 */ OP_EORI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0x4a */ OP_LSRA, MODE_NONE,  1, 2, 0, 0 },
  { /* 0x4b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x4c */ OP_JMP,  MODE_ABS,   3, 3, 1, 1 },
  { /* 0x4d */ OP_EOR,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0x4e */ OP_LSR,  MODE_ABS,   3, 6, 0, 0 },
  { /* 0x4f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x50 */ OP_BVC,  MODE_REL,   2, 2, 1, 0 },
  { /* 0x51 */ OP_EOR,  MODE_IND_Y, 2, 5, 0, 0 },
  { /* 0x52 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x53 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x54 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x55 */ OP_EOR,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0x56 */ OP_LSR,  MODE_ZP_X,  2, 6, 0, 0 },
  { /* 0x57 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x58 */ OP_CLI,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0x59 */ OP_EOR,  MODE_ABS_Y, 3, 4, 0, 0 },
  { /* 0x5a */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x5b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x5c */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x5d */ OP_EOR,  MODE_ABS_X, 3, 4, 0, 0 },
  { /* 0x5e */ OP_LSR,  MODE_ABS_X, 3, 7, 0, 0 },
  { /* 0x5f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x60 */ OP_RTS,  MODE_NONE,  1, 6, 0, 1 },
  { /* 0x61 */ OP_ADC,  MODE_IND_X, 2, 6, 0, 0 },
  { /* 0x62 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x63 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x64 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x65 */ OP_ADC,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0x66 */ OP_ROR,  MODE_ZP,    2, 5, 0, 0 },
  { /* 0x67 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x68 */ OP_PLA,  MODE_NONE,  1, 4, 0, 0 },
  { /* 0x69 */ OP_ADCI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0x6a */ OP_RORA, MODE_NONE,  1, 2, 0, 0 },
  { /* 0x6b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x6c */ OP_JMP,  MODE_IND,   3, 5, 0, 1 },
  { /* 0x6d */ OP_ADC,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0x6e */ OP_ROR,  MODE_ABS,   3, 6, 0, 0 },
  { /* 0x6f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x70 */ OP_BVS,  MODE_REL,   2, 2, 1, 0 },
  { /* 0x71 */ OP_ADC,  MODE_IND_Y, 2, 5, 0, 0 },
  { /* 0x72 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x73 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x74 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x75 */ OP_ADC,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0x76 */ OP_ROR,  MODE_ZP_X,  2, 6, 0, 0 },
  { /* 0x77 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x78 */ OP_SEI,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0x79 */ OP_ADC,  MODE_ABS_Y, 3, 4, 0, 0 },
  { /* 0x7a */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x7b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x7c */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x7d */ OP_ADC,  MODE_ABS_X, 3, 4, 0, 0 },
  { /* 0x7e */ OP_ROR,  MODE_ABS_X, 3, 7, 0, 0 },
  { /* 0x7f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x80 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x81 */ OP_STA,  MODE_IND_X, 2, 6, 0, 0 },
  { /* 0x82 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x83 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x84 */ OP_STY,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0x85 */ OP_STA,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0x86 */ OP_STX,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0x87 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x88 */ OP_DEY,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0x89 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x8a */ OP_TXA,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0x8b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x8c */ OP_STY,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0x8d */ OP_STA,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0x8e */ OP_STX,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0x8f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0x90 */ OP_BCC,  MODE_REL,   2, 2, 1, 0 },
  { /* 0x91 */ OP_STA,  MODE_IND_Y, 2, 6, 0, 0 },
  { /* 0x92 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x93 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x94 */ OP_STY,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0x95 */ OP_STA,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0x96 */ OP_STX,  MODE_ZP_Y,  2, 4, 0, 0 },
  { /* 0x97 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x98 */ OP_TYA,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0x99 */ OP_STA,  MODE_ABS_Y, 3, 5, 0, 0 },
  { /* 0x9a */ OP_TXS,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0x9b */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x9c */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x9d */ OP_STA,  MODE_ABS_X, 3, 5, 0, 0 },
  { /* 0x9e */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0x9f */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0xa0 */ OP_LDYI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0xa1 */ OP_LDA,  MODE_IND_X, 2, 6, 0, 0 },
  { /* 0xa2 */ OP_LDXI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0xa3 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xa4 */ OP_LDY,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0xa5 */ OP_LDA,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0xa6 */ OP_LDX,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0xa7 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xa8 */ OP_TAY,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xa9 */ OP_LDAI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0xaa */ OP_TAX,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xab */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xac */ OP_LDY,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0xad */ OP_LDA,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0xae */ OP_LDX,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0xaf */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0xb0 */ OP_BCS,  MODE_REL,   2, 2, 1, 0 },
  { /* 0xb1 */ OP_LDA,  MODE_IND_Y, 2, 5, 0, 0 },
  { /* 0xb2 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xb3 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xb4 */ OP_LDY,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0xb5 */ OP_LDA,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0xb6 */ OP_LDX,  MODE_ZP_Y,  2, 4, 0, 0 },
  { /* 0xb7 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xb8 */ OP_CLV,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xb9 */ OP_LDA,  MODE_ABS_Y, 3, 4, 0, 0 },
  { /* 0xba */ OP_TSX,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xbb */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xbc */ OP_LDY,  MODE_ABS_X, 3, 4, 0, 0 },
  { /* 0xbd */ OP_LDA,  MODE_ABS_X, 3, 4, 0, 0 },
  { /* 0xbe */ OP_LDX,  MODE_ABS_Y, 3, 4, 0, 0 },
  { /* 0xbf */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0xc0 */ OP_CPYI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0xc1 */ OP_CMP,  MODE_IND_X, 2, 6, 0, 0 },
  { /* 0xc2 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xc3 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xc4 */ OP_CPY,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0xc5 */ OP_CMP,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0xc6 */ OP_DEC,  MODE_ZP,    2, 5, 0, 0 },
  { /* 0xc7 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xc8 */ OP_INY,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xc9 */ OP_CMPI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0xca */ OP_DEX,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xcb */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xcc */ OP_CPY,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0xcd */ OP_CMP,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0xce */ OP_DEC,  MODE_ABS,   3, 6, 0, 0 },
  { /* 0xcf */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0xd0 */ OP_BNE,  MODE_REL,   2, 2, 1, 0 },
  { /* 0xd1 */ OP_CMP,  MODE_IND_Y, 2, 5, 0, 0 },
  { /* 0xd2 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xd3 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xd4 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xd5 */ OP_CMP,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0xd6 */ OP_DEC,  MODE_ZP_X,  2, 6, 0, 0 },
  { /* 0xd7 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xd8 */ OP_CLD,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xd9 */ OP_CMP,  MODE_ABS_Y, 3, 4, 0, 0 },
  { /* 0xda */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xdb */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xdc */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xdd */ OP_CMP,  MODE_ABS_X, 3, 4, 0, 0 },
  { /* 0xde */ OP_DEC,  MODE_ABS_X, 3, 7, 0, 0 },
  { /* 0xdf */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0xe0 */ OP_CPXI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0xe1 */ OP_SBC,  MODE_IND_X, 2, 6, 0, 0 },
  { /* 0xe2 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xe3 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xe4 */ OP_CPX,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0xe5 */ OP_SBC,  MODE_ZP,    2, 3, 0, 0 },
  { /* 0xe6 */ OP_INC,  MODE_ZP,    2, 5, 0, 0 },
  { /* 0xe7 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xe8 */ OP_INX,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xe9 */ OP_SBCI, MODE_IMM,   2, 2, 0, 0 },
  { /* 0xea */ OP_NOP,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xeb */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xec */ OP_CPX,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0xed */ OP_SBC,  MODE_ABS,   3, 4, 0, 0 },
  { /* 0xee */ OP_INC,  MODE_ABS,   3, 6, 0, 0 },
  { /* 0xef */ NULL,    NULL,       0, 0, 0, 0 },

  { /* 0xf0 */ OP_BEQ,  MODE_REL,   2, 2, 1, 0 },
  { /* 0xf1 */ OP_SBC,  MODE_IND_Y, 2, 5, 0, 0 },
  { /* 0xf2 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xf3 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xf4 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xf5 */ OP_SBC,  MODE_ZP_X,  2, 4, 0, 0 },
  { /* 0xf6 */ OP_INC,  MODE_ZP_X,  2, 6, 0, 0 },
  { /* 0xf7 */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xf8 */ OP_SED,  MODE_NONE,  1, 2, 0, 0 },
  { /* 0xf9 */ OP_SBC,  MODE_ABS_Y, 3, 4, 0, 0 },
  { /* 0xfa */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xfb */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xfc */ NULL,    NULL,       0, 0, 0, 0 },
  { /* 0xfd */ OP_SBC,  MODE_ABS_X, 3, 4, 0, 0 },
  { /* 0xfe */ OP_INC,  MODE_ABS_X, 3, 7, 0, 0 },
  { /* 0xff */ NULL,    NULL,       0, 0, 0, 0 }
};

#define MAXTRACE 5000
int tracearray [MAXTRACE];
int tracecnt = 0;

int is_rom (int addr)
{
  return ((mem [addr].tagr == MEMORY) && 
	  ((mem [addr].tagw == ROMWRT) ||
	   (mem [addr].tagw == IGNWRT)));
}

void push_loc (int addr)
{
  if (! is_rom (addr))
    fprintf (stderr, "Tried to push non-ROM address %04x\n", addr);
  else if (tracecnt == MAXTRACE)
    fprintf (stderr, "Too many locations pushed on trace stack\n");
  else
    tracearray [tracecnt++] = addr;
}

char mark [0x10000];

void clear_marks (void)
{
  int i;
  for (i = 0; i < 0x10000; i++)
    mark [i] = 0;
}

int is_unmarked_rom (int addr)
{
  return (is_rom (addr) && (mark [addr] == 0));
}

void mark_inst (int pc)
{
  int opcode = memrd (pc, 0, 0);
  mark [pc] = 1;
  switch (optab [opcode].bytes)
    {
    case 3:
      mark [pc+2] = 2;
    case 2:
      mark [pc+1] = 2;
    }
}

int valid_inst (int pc)
{
  int opcode;

  if (! is_unmarked_rom (pc))
    return (0);

  opcode = memrd (pc, 0, 0);
  if (opcode == 0)
    return (0);
  switch (optab [opcode].bytes)
    {
    case 0:
      return (0);
    case 1:
      return (1);
    case 2:
      return (is_unmarked_rom (pc+1) ? 2 : 0);
    case 3:
      return ((is_unmarked_rom (pc+1) && is_unmarked_rom (pc+2)) ? 3 : 0);
    default:
      return (0);
    }
}

int valid_n_inst (int pc, int n)
{
  int i;
  int len;

  for (i = 0; i < n; i++)
    {
      len = valid_inst (pc);
      if (len == 0)
	return (0);
      pc += len;
    }

  return (1);
}

int sign_ext (int v)
{
  if (v & 0x80)
    return (v | 0xff00);
  else
    return (v);
}

int rel_addr (int p, int o)
{
  return ((p + 2 + sign_ext (o)) & 0xffff);
}

void decode_inst (int *pc, int *operand, int *branch, int *end)
{
  int opcode;

  opcode = memrd (*pc, 0, 0);
  switch (optab [opcode].bytes)
    {
    case 0:
      *branch = 0;
      *end = 1;
      break;
    case 1:
      *branch = 0;
      *end = optab [opcode].end;
      (*pc)++;
      break;
    case 2:
      *branch = optab [opcode].branch;
      *end = optab [opcode].end;
      if (*branch)
	*operand = rel_addr ((*pc), memrd ((*pc)+1, 0, 0));
      else
	*operand = memrd ((*pc)+1, 0, 0);
      (*pc)+=2;
      break;
    case 3:
      *branch = optab [opcode].branch;
      *end = optab [opcode].end;
      *operand = memrdwd ((*pc)+1, 0, 0);
      (*pc)+=3;
      break;
    }
}

int translate_inst (int *pc, char *s)
{
  int opcode;
  int operand = 0;

  opcode = memrd (*pc, 0, 0);
  switch (optab [opcode].bytes)
    {
    case 0:
      return (0);
    case 1:
      break;
    case 2:
      operand = memrd ((*pc)+1, 0, 0);
      break;
    case 3:
      operand = memrdwd ((*pc)+1, 0, 0);
      break;
    }

  if (s)
    {
      sprintf (s, "case 0x%04x: /* %02x", *pc, opcode);
      s += strlen (s);

      switch (optab [opcode].bytes)
	{
	case 1:
	  sprintf (s, "      */  ");
	  s += strlen (s);
	  break;
	case 2:
	  sprintf (s, " %02x   */  ", operand);
	  s += strlen (s);
	  break;
	case 3:
	  sprintf (s, " %04x */  ", operand);
	  s += strlen (s);
	  break;
	}

      sprintf (s, "B(%d);  C(%d);  ", optab [opcode].bytes,
	       optab [opcode].cycles);
      s += strlen (s);

      if ((optab [opcode].bytes == 1) || 
	  (optab [opcode].mode == MODE_REL) ||
	  (optab [opcode].mode == MODE_IMM))
	sprintf (s, optab [opcode].mode);
      else
	sprintf (s, optab [opcode].mode, operand);
      s += strlen (s);

      if (optab [opcode].op == OP_JSR)
	sprintf (s, optab [opcode].op, *pc);
      else if (optab [opcode].mode == MODE_REL)
	sprintf (s, optab [opcode].op, rel_addr (*pc, operand));
      else if (optab [opcode].mode == MODE_IMM)
	sprintf (s, optab [opcode].op, operand);
      else
	sprintf (s, optab [opcode].op);
      s += strlen (s);
    }

  (*pc) += optab [opcode].bytes;

  return (1);
}

/* pass 1 - analysis */
void pass1 (void)
{
  int PC;
  int operand;
  int branch;
  int end;

  while (tracecnt)
    {
      /* trace */
      PC = tracearray [--tracecnt];
      while (valid_inst (PC))
	{
	  mark_inst (PC);
	  decode_inst (& PC, & operand, & branch, & end);
	  if (branch)
	    push_loc (operand);
	  if (end)
	    break;
	}

#if 0
      /* if there are no more pending traces, try to find more code */
      if (! tracecnt)
	{
	  for (PC = 0; PC < 0x10000; PC++)
	    {
	      if (valid_n_inst (PC, 3))
		push_loc (PC);
	    }
	}
#endif
    }
}

/* pass 2 - output translation */
void pass2 (void)
{
  int PC = 0;
  char buf [80];

  while (PC < 0x10000)
    {
      while ((PC < 0x10000) && (mark [PC] != 1))
	PC++;
      while ((PC < 0x10000) && (mark [PC] == 1))
	{
	  translate_inst (& PC, buf);
	  fprintf (stdout, "%s\n", buf);
	}
      fprintf (stdout, "             PC = 0x%04x;  continue;\n", PC);
    }
}

int main (int argc, char *argv[])
{
  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <game>\n", argv [0]); 
      fprintf (stderr, "The following games are supported:\n"
	       "    keyword       title\n"
	       "    ----------    ------------------------------\n");
      show_games ();
      exit (1);
    }

  game = pick_game (argv [1]);

  if (game == 0)
    {
      fprintf (stderr, "A game must be specified\n");
      exit (1);
    }

  mem = (elem *) calloc(65536L,sizeof(elem));

  if(!mem)
    {
      fprintf(stderr, "Cannot get memory for translation\n");
      exit(1);
    }

  setup_game ();

  clear_marks ();

  push_loc (memrdwd (0xfffa, 0, 0)); /* NMI vector */
  push_loc (memrdwd (0xfffc, 0, 0)); /* RESET vector */
  push_loc (memrdwd (0xfffe, 0, 0)); /* IRQ vector */

  pass1 ();
  pass2 ();

  return (0);
}


long irq_cycle = 0;
