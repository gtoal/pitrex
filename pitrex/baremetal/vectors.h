/*
 * Names are identical, so that this 'plays nicely with' linux
 * documentation and other associated code.
 */

// see: http://www.ethernut.de/en/documents/arm-exceptions.html

#define PTRACE_R0_idx        0
#define PTRACE_R1_idx        1
#define PTRACE_R2_idx        2
#define PTRACE_R3_idx        3
#define PTRACE_R4_idx        4
#define PTRACE_R5_idx        5
#define PTRACE_R6_idx        6
#define PTRACE_R7_idx        7
#define PTRACE_R8_idx        8
#define PTRACE_R9_idx        9
#define PTRACE_R10_idx       10
#define PTRACE_R11_idx       11
#define PTRACE_R12_idx       12
#define PTRACE_R13_idx       13
#define PTRACE_SP_idx        PTRACE_R13_idx
#define PTRACE_R14_idx       14
#define PTRACE_LR_idx        PTRACE_R14_idx
#define PTRACE_R15_idx       15
#define PTRACE_PC_idx        PTRACE_R15_idx
#define PTRACE_CPSR_idx      16
#define PTRACE_R0_retval_idx 17
#define PTRACE_FRAMETYPE_idx 18
#define PTRACE_SWICODE_idx   19
#define PTRACE_FRAME_size    20 /* in bytes */

#define ARM_CPSR_T_BIT   (0x20)
#define ARM_CPSR_F_BIT   (0x40)
#define ARM_CPSR_I_BIT   (0x80)
#define ARM_CPSR_V_BIT   (1<<28)
#define ARM_CPSR_C_BIT   (1<<29)
#define ARM_CPSR_Z_BIT   (1<<30)
#define ARM_CPSR_N_BIT   (1<<31)
#define ARM_USR_MODE     (0x10) /**< no priv mode */
#define ARM_FIQ_MODE     (0x11) /**< fiq occured */
#define ARM_IRQ_MODE     (0x12) /**< irq occured */
#define ARM_SVC_MODE     (0x13) /**< swi occured */
#define ARM_ABT_MODE     (0x17) /**< pfa or udf  */
#define ARM_UND_MODE     (0x1b) /**< bad opcode  */
#define ARM_SYS_MODE     (0x1f) /**< normal supervisor */
#define ARM_MODE_MASK    (0x1f)




#define PTRACE_FRAME_isirq(X)   ( (X)->uregs[ PTRACE_FRAMETYPE ] < 32 )
#define PTRACE_FRAMETYPE_reset   32 /* in vector order */
#define PTRACE_FRAMETYPE_udf     33
#define PTRACE_FRAMETYPE_swi     34
#define PTRACE_FRAMETYPE_pfa     35
#define PTRACE_FRAMETYPE_da      36
#define PTRACE_FRAMETYPE_notused 37
/* #define PTRACE_FRAMETYPE_irq is above, ie: 0..32 for each irq vector */
#define PTRACE_FRAMETYPE_fiq     38
#define PTRACE_FRAMETYPE_thread  39 /* thread saved state */

struct pt_regs {
  long uregs[ 20 ];
};

void ARM_UDF_Handler( struct pt_regs  *p );
void ARM_SWI_Handler( struct pt_regs  *p );
void ARM_PFA_Handler( struct pt_regs  *p );
void ARM_DA_Handler ( struct pt_regs  *p );
void ARM_IRQ_Handler( struct pt_regs  *p );
void ARM_FIQ_Handler( struct pt_regs  *p );
void ARM_NOTUSED_Handler( struct pt_regs  *p );
void ARM_COMMON_Handler_crash( struct pt_regs  *p, const char *name );







void ptrace_dump_regs( struct pt_regs *p );
void ptrace_stackdump_from( const int *frame_ptr );
void ptrace_stackdump_regs( struct pt_regs *p );
void ptrace_stackdump_here( void );

typedef struct {
  unsigned int _reset;
  unsigned int _undefined;
  unsigned int _swi;
  unsigned int _prefetched;
  unsigned int _data;
  unsigned int _unused;
  unsigned int _irq;
  unsigned int _fiq;

  unsigned int _reset_pointer;
  unsigned int _undefined_pointer;
  unsigned int _swi_pointer;
  unsigned int _prefetched_pointer;
  unsigned int _data_pointer;
  unsigned int _unused_pointer;
  unsigned int _irq_pointer;
  unsigned int _fiq_pointer;
  } pi_vectors;

void print_data(void) __attribute__ ((naked));

void print_reset()
{
    printf( "Reset EXCEPTION...\r\n" );
    uint32_t progSpace = 0x8000;
    void (*progStart)(void) = (void (*)(void))progSpace;
    progStart();
}
void print_halt()
{
  while (1)
    printf( "ld halt ...\r\n" );
    
}
void print_undefined()
{
  while (1)
    printf( "ld undefined ...\r\n" );
}
void print_prefetch()
{
  while (1)
    printf( "ld prefetched ...\r\n" );
}
void print_data()
{
   struct pt_regs *p;
   u_long *lnk_ptr;

   /* On data abort exception the LR points to PC+8 */
   asm __volatile__ (
      "sub lr, lr, #8\n"
      "mov %0, lr" : "=r" (lnk_ptr)
    );

   asm __volatile__ (
    "mov %0, sp\n"  : "=r" (p) 
       );  
   asm __volatile__ (
    "mov lr, pc\n"  );  
  
    printf( "Data Abort EXCEPTION at %p 0x%08lX\r\n", lnk_ptr, *(lnk_ptr));

    
    
    // following does not work well
    // some bugs in there
    // but since I don't need it right now...
    // I did not debug it!
    ptrace_stackdump_regs( p );    
    for(;;);
   
    
}
void print_hang()
{
  while (1)
    printf( "ld hang ...\r\n" );
}
void print_irq()
{
  while (1)
    printf( "ld irq ...\r\n" );
}
void print_fiq()
{
  while (1)
    printf( "ld fiq ...\r\n" );
}
void print_swi()
{
  while (1)
    printf( "ld swi ...\r\n" );
}
void print_unused()
{
  while (1)
    printf( "ld unsused ...\r\n" );
}

void tweakVectors()
{
  pi_vectors *pv ;
  // circumvent gcc UDF
  asm("ldr %[result],= 0\n\t" : [result]"=r" (pv) :: "cc");
  /*
  int mem = 8*4;
  printf("Pointers\r\n");
  printf("mem: %i: %x\r\n", (int)mem, peek(mem));mem+=4;
  printf("mem: %i: %x\r\n", (int)mem, peek(mem));mem+=4;
  printf("mem: %i: %x\r\n", (int)mem, peek(mem));mem+=4;
  printf("mem: %i: %x\r\n", (int)mem, peek(mem));mem+=4;
  printf("mem: %i: %x\r\n", (int)mem, peek(mem));mem+=4;
  printf("mem: %i: %x\r\n", (int)mem, peek(mem));mem+=4;
  printf("mem: %i: %x\r\n", (int)mem, peek(mem));mem+=4;
  printf("mem: %i: %x\r\n", (int)mem, peek(mem));mem+=4;

  printf("---\r\n");
  */
  pv->_reset_pointer = (unsigned int)print_reset;
  pv->_undefined_pointer = (unsigned int)print_undefined;
  pv->_swi_pointer = (unsigned int)print_swi;
  pv->_prefetched_pointer = (unsigned int)print_prefetch;
  pv->_data_pointer = (unsigned int)print_data;
  pv->_unused_pointer = (unsigned int)print_unused;
  pv->_irq_pointer = (unsigned int)print_irq;
  pv->_fiq_pointer = (unsigned int)print_fiq;
  /*
  printf("vector:%i: %x\r\n", &(pv->_reset_pointer), pv->_reset_pointer);
  printf("vector:%i: %x\r\n", &(pv->_undefined_pointer), pv->_undefined_pointer);
  printf("vector:%i: %x\r\n", &(pv->_swi_pointer), pv->_swi_pointer);
  printf("vector:%i: %x\r\n", &(pv->_prefetched_pointer), pv->_prefetched_pointer);
  printf("vector:%i: %x\r\n", &(pv->_data_pointer), pv->_data_pointer);
  printf("vector:%i: %x\r\n", &(pv->_unused_pointer), pv->_unused_pointer);
  printf("vector:%i: %x\r\n", &(pv->_irq_pointer), pv->_irq_pointer);
  printf("vector:%i: %x\r\n", &(pv->_fiq_pointer), pv->_fiq_pointer);
  */
}
void ptrace_dump_regs( struct pt_regs *p )
{
  int x;
  printf("\r\n======================================\r\n");
  printf("R0 : %08X\r\n", p->uregs[ PTRACE_R0_idx ] );
  printf("R1 : %08X\r\n", p->uregs[ PTRACE_R1_idx ] );
  printf("R2 : %08X\r\n", p->uregs[ PTRACE_R2_idx ] );
  printf("R3 : %08X\r\n", p->uregs[ PTRACE_R3_idx ] );
  printf("R4 : %08X\r\n", p->uregs[ PTRACE_R4_idx ] );
  printf("R5 : %08X\r\n", p->uregs[ PTRACE_R5_idx ] );
  printf("R6 : %08X\r\n", p->uregs[ PTRACE_R6_idx ] );
  printf("R7 : %08X\r\n", p->uregs[ PTRACE_R7_idx ] );
  printf("R8 : %08X\r\n", p->uregs[ PTRACE_R8_idx ] );
  printf("R9 : %08X\r\n", p->uregs[ PTRACE_R9_idx ] );
  printf("R1 : %08X\r\n", p->uregs[ PTRACE_R10_idx ] );
  printf("R11: %08X\r\n", p->uregs[ PTRACE_R11_idx ] );
  printf("R12: %08X\r\n", p->uregs[ PTRACE_R12_idx ] );
  printf("R13: %08X\r\n", p->uregs[ PTRACE_R13_idx ] );
  printf("R14: %08X\r\n", p->uregs[ PTRACE_R14_idx ] );
  printf("R15: %08X\r\n", p->uregs[ PTRACE_R15_idx ] );
  printf("PSW: ");
  printf("%08X\r\n", p->uregs[ PTRACE_CPSR_idx ] );
  /* now decode the flags */
  x = p->uregs[ PTRACE_CPSR_idx ];
  printf("  ");
  printf("%c", ( x & ARM_CPSR_N_BIT ) ? 'N' : 'n' );
  printf("%c", ( x & ARM_CPSR_Z_BIT ) ? 'Z' : 'z' );
  printf("%c", ( x & ARM_CPSR_C_BIT ) ? 'C' : 'c' );
  printf("%c", ( x & ARM_CPSR_V_BIT ) ? 'V' : 'v' );
  printf("...");
  printf("%c", ( x & ARM_CPSR_F_BIT ) ? 'F' : 'f' );
  printf("%c", ( x & ARM_CPSR_I_BIT ) ? 'I' : 'i' );
  printf("%c", ( x & ARM_CPSR_T_BIT ) ? 'T' : 't' );
  switch( ARM_MODE_MASK & x ){
  case ARM_USR_MODE:
    printf("user-mode\r\n");
    break;
  case ARM_FIQ_MODE:
    printf("user-mode\r\n");
    break;
  case ARM_IRQ_MODE:
    printf("irq-mode\r\n");
    break;
  case ARM_SVC_MODE:
    printf("svc-mode\r\n");
    break;
  case ARM_ABT_MODE:
    if( p->uregs[ PTRACE_FRAMETYPE_idx ] == PTRACE_FRAMETYPE_pfa ){
      printf("pfa-mode\r\n");
    } else if( p->uregs[ PTRACE_FRAMETYPE_idx ] == PTRACE_FRAMETYPE_da  ){
      printf("da-mode\r\n");
    } else {
      printf("unknown-abort-mode\r\n");
    }
    break;
  case ARM_UND_MODE:
    printf("und-mode\r\n");
    break;
  case ARM_SYS_MODE:
    printf("sys-mode\r\n");
    break;
  }
  printf("\r\n======================================\r\n");
}

void
ptrace_stackdump_from( const int *fp )
{
  int depth;
  const int *newfp;
  char buf[5];
  int  pc;

  depth = 0;
  while( fp != NULL ){
    pc = fp[ - 1 ];
    buf[0] = (depth / 10);
    if( buf[0] ){
      buf[0] += 0x30;
    } else {
      buf[0]  = 0x20;
    }
    buf[1]  = (depth % 10);
    buf[2]  = ')';
    buf[3]  = ' ';
    buf[4]  = 0;
    printf( "%s%08X\r\n",buf, pc );

    newfp = (const int *)fp[-3];
    /* is new frame pointer some what valid? */
    if( (newfp <= fp) || (newfp > &(fp[1024])) ){
      /* no, then stop */
      break;
    }
    fp = newfp;
  }
}

void
ptrace_stackdump_regs( struct pt_regs *p )
{
  ptrace_dump_regs( p );
  ptrace_stackdump_from( (int *)(p->uregs[ PTRACE_R13_idx ]) );
}

void
ptrace_stackdump_here( void )
{
  const int *fp;
  asm volatile ("mov %0,fp\n" : "=r" (fp) );

  ptrace_stackdump_from( fp );
}
