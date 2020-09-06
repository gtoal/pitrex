/************************************************************************
 * Print the opcode name to the 'dest' buffer.				*
 *									*
 * Called with:								*
 *    dest    = Pointer to buffer to receive opcode's name.		*
 *    opc     = Opcode's binary value.					*
 *    mode    = Addressing mode used by opcode.				*
 *    useBreg = Flag indicating Acc instruction should use B-reg	*
 ************************************************************************/

void printOpc(char *dest, unsigned int opc, unsigned int mode, unsigned int useBreg)
{
  int	ii;

  strcpy(OpcBfr, OpcodeName[opc]);

  /* If extended instructions being printed, use the 'useBreg' */
  /* to determine Acc used by any Acc instruction. */

  if (Extd_f)
  {
    /* if Accumulator accessed, print it */

    if (mode <= AIRG)
    {
      if (useBreg)
        OpcBfr[3] = 'B';		/* B register accessed */

      else
        OpcBfr[3] = 'A';		/* A register accessed */

      OpcBfr[4] = '\0';			/* terminate string */
    }
  }

  strlwr(OpcBfr);		/* lower case string (just my preference) */

  /* move into destination buffer without trailing '\0' */

  for (ii = 0; OpcBfr[ii] != '\0'; ii++)
    dest[ii] = OpcBfr[ii];
}

/************************************************************************
 * Print parameters, based on the addressing mode of the instruction	*
 * to the 'dest' buffer.						*
 *									*
 * Called with:								*
 *    dest    = Destination buffer of parameters.			*
 *    mode    = Addressing mode of opcode.				*
 *    adr     = Address of opcode (index into 'rom[]').		*
 *    useBreg = Flag indicating Acc instruction should use B-reg	*
 *									*
 * Returns:								*
 *    Length of object code associated with opcode.			*
 ************************************************************************/

U_LONG ImmOpd;

unsigned int printMode(char *dest, unsigned int mode, unsigned int adr, unsigned int useBreg)
{
  unsigned int	ii, jj, size;

  ImmOpd = 0xDEADBEEFUL;

  size = useBreg + 1;		/* initial size of object code */
  adr += useBreg;		/* if using B-reg skip 'USB' instruction */

  switch (mode)
  {
    /* display 4 bit immediate value */

    case AIM4:
    case IM4:
      sprintf(OpcBfr, "#$%lX", ImmOpd = (rom[adr] & 0x0F));
      break;

      /* display 4 bit extended immediate value */

    case AIM4X:	
      sprintf(OpcBfr, "#$%lX", ImmOpd = ((rom[adr] & 0x0F) << 8));
      break;

      /* display 8 bit immediate value */

    case AIM8:
      sprintf(OpcBfr, "#$%02lX", ImmOpd = (rom[adr+1]));
      size++;		/* add 8 bit immediate value to object code size */
      break;

      /* display the '[i]', indicating a indirect through 'I' instruction */

    case AIRG:
    case IRG:
      strcpy(OpcBfr, "[i]");
      break;

      /* *** display special 'extended' 12 bit immediate value */

      /* This is done by reading two consective acc load / add / sub */
      /* instructions and calculating what ends up in the Acc */

    case AIMX:
      ii = (rom[adr] & 0x0F) << 8;	/* get 4 bit extended immediate */
      adr += useBreg + 1;	/* if point to next opcode, skip 'USB's */
      size += useBreg + 1;   /* add to object code size, include 'USB's */
      jj = rom[adr] & 0x0F;		   /* get 4 bit immediate value */

      /* check if the next opcode is an 'ADD' instruction (0x20-0x2F) */

      if ((rom[adr] & 0xF0) == 0x20)
      {
        /* if 4 bit immediate value is 0, use 8 bit immediate value	*/

        if (jj == 0)
        {
          ii += rom[adr+1];	/* add 8 bit immediate value */
          size++;		/* add to oject code size */
        }
        else
          ii += jj;		/* else, just add 4 bit immediate value */
      }

      /* if not followed by an 'ADD' instruction, then must be a 'SUB'  */

      else
      {
        /* if 4 bit immediate value is 0, use 8 bit immediate value	 */

        if (jj == 0)
        {
          ii -= rom[adr+1];	/* sub 8 bit immediate value */
          size++;		/* sub to oject code size */
        }
        else
          ii -= jj;		/* else, just sub 4 bit immediate value */
      }
      ii &= 0xFFF;		/* remove any overflow */
      sprintf(OpcBfr, "#$%03lX", ImmOpd = (ii));	/* print to temperary buffer */
      break;

      /* display no parameters */

    case ACC:
    case AXLT:
    case IMP:
      OpcBfr[0] = '\0';					/* no parameters */
      break;

      /* display 12 bit immediate value */

    case IM12:
      sprintf(OpcBfr, "#$%03lX",
        ImmOpd = ((rom[adr] & 0x0F) +
        (rom[adr+1] & 0xF0) +
        ((rom[adr+1] & 0x0F) << 8))
      );
      size++;
      break;

      /* display address of direct addressing mode */

    case ADIR:
    case DIR:
      sprintf(OpcBfr, "$%X", rom[adr] & 0x0F);
      break;

      /* indicate a jump through the 'J' register */

    case JUMP:
/*		strcpy(OpcBfr, "[j]"); */
      OpcBfr[0] = '\0';					/* no parameters */
      break;

      /* *** special extended jump */
      /* This address mode can be used to display a 'LDJ #nnn' instruction */
      /* followed immediatly be a 'JMP' instruction. */

    case JUMPX:
      size += 2;      /* add immediate value and 'JMP' inst to object size */

      /* use 12 bit value of 'LDJ' instruction as parameter */

      sprintf(OpcBfr, "$%03lX",
        ImmOpd = ((rom[adr] & 0x0F) +
        (rom[adr+1] & 0xF0) +
        ((rom[adr+1] & 0x0F) << 8))
      );
      break;
  }

  /* move into destination buffer without trailing '\0' */

  for (ii = 0; OpcBfr[ii] != '\0'; ii++)
    dest[ii] = OpcBfr[ii];

  return (size);			/* return size of opcode */
}

/************************************************************************
 * Print the object code in hexidecimal.				*
 *									*
 * Called with:								*
 *    dest = Destination buffer used to receive data.			*
 *    adr  = Address of object code (index into 'rom[]').		*
 *    size = Size of object code.					*
 ************************************************************************/

void printObj(char *dest, unsigned int adr, unsigned int size)
{
  unsigned int	ii;

  sprintf(OpcBfr, "%04X:", adr);		/* print address */

  /* print however many bytes contained in oject code */

  for (ii = 0; ii < size; ii++)
    sprintf(OpcBfr+((ii * 3) + 5), " %02X", rom[adr+ii]);

  /* move into destination buffer without trailing '\0' */

  for (ii = 0; OpcBfr[ii] != '\0'; ii++)
    dest[ii] = OpcBfr[ii];
}

/**************************************************************************
 * Fully disassemble one opcode.					  *
 *									  *
 * Called with:								  *
 *    dest    = Destination buffer to receive ASCII data.		  *
 *    adr     = Address of opcode (index into rom[]).		  *
 *    objSize = Pointer to (unsigned int) to receive object code size.		  *
 *    brkFlg  = Pointer to a flag indicating a break in instruction flow. *
 **************************************************************************/

uchar	opCode;

void dissOpcode(char *dest, unsigned int adr, unsigned int *objSize, unsigned int *brkFlg)
{
  uchar	/*opCode,*/ nextOpc;
  unsigned int	useBreg;

  /* NOTE: This will need some care with multiple instruction opcodes: */
  register_PC = adr;

  opCode = rom[adr];			/* get opcode */

  /* check for break in code flow */

  if (DecodeTbl[opCode].name == JMPA || DecodeTbl[opCode].name == JPPB)
    *brkFlg = 1;				/* indicate code break */

  else
    *brkFlg = 0;				/* indicate no code break */

  /* Check to see if extended instructions are being disassembled. */

  if (Extd_f)
  {

    /* start by check for special prefixes */

    /* *** Use B-reg? */

    if (DecodeTbl[opCode].name == NOPB)
    {
            nextOpc = rom[adr+1];

            /* does next opcode use Acc addressing? */

            if (DecodeTbl[nextOpc].mode > AIRG)
            {
                    /* No, disassemble opcode NOPB */

                    printOpc(dest+TAB_OPCODE, DecodeTbl[opCode].name,
                      DecodeTbl[opCode].mode, 0);

                    *objSize = printMode(dest+TAB_PARAM,
                      DecodeTbl[opCode].mode, adr, 0);

                    printObj(dest, adr, *objSize);
                    return;
            }
            else
            {	useBreg = 1;				/* set to B-reg */
                    opCode = nextOpc;			/* get next opcode */
            }
    }
    else
      useBreg = 0;

    /* check for extended LDA instructions */

    if ((opCode & 0xF0) == 0x00)
    {
            if (useBreg == 0 || (useBreg == 1 && rom[adr+2] == 0x57))
            {
                    nextOpc = rom[adr+useBreg+useBreg+1];

                    if ((nextOpc & 0xE0) == 0x20)
                    {
                              /* disasemble extended LDA instruction */

                              printOpc(dest+TAB_OPCODE, LDA, AIMX, useBreg);

                              *objSize = printMode(
                                dest+TAB_PARAM, AIMX, adr, useBreg);

                              printObj(dest, adr, *objSize);
                              return;
                    }
            }
    }

    /* check for extended JUMP instructions */

    if (DecodeTbl[opCode].name == LDJ && opCode != 0xE1)
    {
            nextOpc = rom[adr+2];

            if (DecodeTbl[nextOpc].mode == JUMP)
            {
                    /* Disassemble extended JUMP */

                    if (DecodeTbl[nextOpc].name == JMPA ||
                              DecodeTbl[nextOpc].name == JPPB)
                      *brkFlg = 1;
                    /* set break in instruction flow flag */

                    printOpc(dest+TAB_OPCODE, DecodeTbl[nextOpc].name,
                      DecodeTbl[nextOpc].mode, useBreg);

                    *objSize = printMode(
                      dest+TAB_PARAM, JUMPX, adr, useBreg);

                    printObj(dest, adr, *objSize);
                    return;
            }
    }
  }
  else
    useBreg = 0;		/* no extended instructions allowed */

  /* Disassemble the opcode */

  printOpc(dest+TAB_OPCODE, DecodeTbl[opCode].name,
    DecodeTbl[opCode].mode, useBreg);

  *objSize = printMode(dest+TAB_PARAM, DecodeTbl[opCode].mode, adr, useBreg);

  printObj(dest, adr, *objSize);
  return;
}

