#pragma once

#ifdef FREESTANDING
#define isb() asm volatile ("mcr p15, #0, %[zero], c7, c5,  #4" : : [zero] "r" (0) )
#define dsb() asm volatile ("mcr p15, #0, %[zero], c7, c10, #4" : : [zero] "r" (0) )
#define dmb() asm volatile ("mcr p15, #0, %[zero], c7, c10, #5" : : [zero] "r" (0) )

#define invalidate_instruction_cache()	asm volatile ("mcr p15, #0, %[zero], c7, c5,  #0" : : [zero] "r" (0) )
#define flush_prefetch_buffer()		asm volatile ("mcr p15, #0, %[zero], c7, c5,  #4" : : [zero] "r" (0) )
#define flush_branch_target_cache()	asm volatile ("mcr p15, #0, %[zero], c7, c5,  #6" : : [zero] "r" (0) )

#define invalidate_data_cache()		asm volatile ("mcr p15, 0, %0, c7, c6,  0\n" \
						      "mcr p15, 0, %0, c7, c10, 4\n" : : "r" (0) : "memory")

// same as flush_dcache_all
#define clean_data_cache()		asm volatile ("mcr p15, 0, %0, c7, c10, 0\n" \
  						      "mcr p15, 0, %0, c7, c10, 4\n" : : "r" (0) : "memory")


#define ARM_CONTROL_MMU				(1 << 0) // MMU
#define ARM_CONTROL_STRICT_ALIGNMENT		(1 << 1) // Alignment
#define ARM_CONTROL_L1_CACHE			(1 << 2) // D Cache
#define ARM_CONTROL_BRANCH_PREDICTION		(1 << 11)
#define ARM_CONTROL_L1_INSTRUCTION_CACHE 	(1 << 12) /* Icache enable		*/

#define MMU_MODE	(  ARM_CONTROL_MMU			\
			 | ARM_CONTROL_L1_CACHE			\
			 | ARM_CONTROL_L1_INSTRUCTION_CACHE	\
			 | ARM_CONTROL_BRANCH_PREDICTION)


#define CYCLE_COUNTER_ENABLE       (1<<0)  // enable all three counters
#define COUNTER_ZERO               (1<<1)  // reset two count registers
#define CYCLE_COUNTER_RESET        (1<<2)  // reset cycle counter register
#define CYCLE_COUNTER_DIVIDE_64    (1<<3)  // cycle count divider (64)

#define PMNC(v)  asm volatile("mcr p15, 0, %0, c15, c12, 0" :: "r"(v))
#define CCNT(v)  asm volatile("mrc p15, 0, %0, c15, c12, 1" : "=r"(v))

void icache_disable(void);
void dcache_disable(void);

static inline void cache_flush(void)
{
	unsigned long i = 0;
	/* clean entire data cache */
	asm volatile("mcr p15, 0, %0, c7, c10, 0" : : "r" (i));
	/* invalidate both caches and flush btb */
	asm volatile("mcr p15, 0, %0, c7, c7, 0" : : "r" (i));
	/* mem barrier to sync things */
	asm volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (i));
}

// https://www.raspberrypi.org/forums/viewtopic.php?f=63&t=155830
static inline void init_perfcounters (int do_reset, int enable_divider)
{
  // in general enable all counters (including cycle counter)
  int value = CYCLE_COUNTER_ENABLE;

  // peform reset:
  if (do_reset)
  {
    value |= COUNTER_ZERO;     // reset all counters to zero.
    value |= CYCLE_COUNTER_RESET;     // reset cycle counter to zero.
  }

  if (enable_divider)
    value |= CYCLE_COUNTER_DIVIDE_64;     // enable "by 64" divider for CCNT.

  PMNC(value);

}
// http://infocenter.arm.com/help/topic/com.arm.doc.ddi0301h/DDI0301H_arm1176jzfs_r0p7_trm.pdf#page=270
//[6] ECCUsed to enable and disable Cycle Counter interrupt reporting:0 = Disable interrupt, reset value1 = Enable interrupt.

static inline unsigned int get_cyclecount (void)
{
  unsigned int value;
  // Read CCNT Register
  CCNT(value);
  return value;
}

// waiter in arm cycles
// that means in nano seconds
static inline unsigned int waitUntil(unsigned int waiter)
{
  unsigned int value;
  // Read CCNT Register
  do
  {
    CCNT(value);
  } while (value < waiter);
  return value;
}




void mmu_disable(void);
char * getErrorText(int errNo);
void vfp_deinit(void);

int peek(unsigned int address);
unsigned char peekByte(unsigned int address);
void pokeByte(unsigned int address, unsigned char c);
char *getLoadParameter();

void/*__attribute__ ((noinline)) */poke(unsigned int address, unsigned int value);



// returns -1 for "out of radix"
int bm_atoi(char* str, int radix);



///////////////////////////////////////////////////////////
// STRING
///////////////////////////////////////////////////////////

static inline int isdigit(int c) {
  return (c >= (int) '0' && c <= (int) '9') ? 1 : 0;
}

static inline int isxdigit(int c) {
  return ((isdigit(c) != 0) || (((unsigned) c | 32) - (int) 'a' < 6)) ? 1 : 0;
}

static inline int isprint(int c) {
  return ((c >= (int) ' ' && c <= (int) '~')) ? 1 : 0;
}

static inline int isupper(int c) {
  return (c >= (int) 'A' && c <= (int) 'Z') ? 1 : 0;
}

static inline int islower(int c) {
  return (c >= (int) 'a' && c <= (int) 'z') ? 1 : 0;
}

static inline int isalpha(int c) {
  return ((isupper(c) != 0) || (islower(c) != 0)) ? 1 : 0;
}

static inline int tolower(int c) {
  return ((isupper(c) != 0) ? (c + 32) : c);
}

static inline int toupper(int c) {
  return ((islower(c) != 0) ? (c - 32) : c);
}

static inline int memcmp(const void *s1, const void *s2, size_t n) {
  unsigned char u1, u2;
  unsigned char *t1, *t2;

  t1 = (unsigned char *) s1;
  t2 = (unsigned char *) s2;

  for (; n-- != (size_t) 0; t1++, t2++) {
    u1 = *t1;
    u2 = *t2;
    if (u1 != u2) {
      return (int) (u1 - u2);
    }
  }

  return 0;
}

static inline void *memcpy(/*@only@*/void *dest, const void *src, size_t n) {
  char *dp = (char *) dest;
  const char *sp = (const char *) src;

  while (n-- != (size_t) 0) {
    *dp++ = *sp++;
  }

  return dest;
}

static inline void *memmove(/*@only@*/void *dst, const void *src, size_t n) {
  char *dp = (char *) dst;
  const char *sp = (const char *) src;

  if (dp < sp) {
    while (n-- != (size_t) 0) {
      *dp++ = *sp++;
    }
  } else {
    sp += n;
    dp += n;
    while (n-- != (size_t) 0) {
      *--dp = *--sp;
    }
  }

  return dst;
}

static inline void *memset(/*@only@*/void *dest, int c, size_t n) {
  char *dp = (char *) dest;

  while (n-- != (size_t) 0) {
    *dp++ = (char) c;
  }

  return dest;
}

static inline size_t strlen(const char *s) {
  const char *p = s;

  while (*s != (char) 0) {
    ++s;
  }

  return (size_t) (s - p);
}

static inline char *strcpy(/*@only@*/char *s1, const char *s2) {
  char *s = s1;

  while ((*s++ = *s2++) != '\0')
    ;
  return s1;
}

static inline char *strncpy(/*@only@*/char *s1, const char *s2, size_t n) {
  char *s = s1;

  while (n > 0 && *s2 != '\0') {
    *s++ = *s2++;
    --n;
  }

  while (n > 0) {
    *s++ = '\0';
    --n;
  }

  return s1;
}

static inline int strcmp(const char *s1, const char *s2) {
  unsigned char *p1 = (unsigned char *) s1;
  unsigned char *p2 = (unsigned char *) s2;

  for (; *p1 == *p2; p1++, p2++) {
    if (*p1 == (unsigned char) '\0') {
      return 0;
    }
  }

  return (int) (*p1 - *p2);
}

static inline int strncmp(const char *s1, const char *s2, size_t n) {
  unsigned char *p1 = (unsigned char *) s1;
  unsigned char *p2 = (unsigned char *) s2;

  for (; n > 0; p1++, p2++, --n) {
    if (*p1 != *p2) {
      return (int) (*p1 - *p2);
    } else if (*p1 == (unsigned char) '\0') {
      return 0;
    }
  }

  return 0;
}

static inline int strcasecmp(const char *s1, const char *s2) {
  unsigned char *p1 = (unsigned char *) s1;
  unsigned char *p2 = (unsigned char *) s2;

  for (; tolower((int) *p1) == tolower((int) *p2); p1++, p2++) {
    if (*p1 == (unsigned char) '\0') {
      return 0;
    }
  }

  return (int) (tolower((int) *p1) - tolower((int) *p2));
}

static inline int strncasecmp(const char *s1, const char *s2, size_t n)
{
  unsigned char *p1 = (unsigned char *) s1;
  unsigned char *p2 = (unsigned char *) s2;

  for (; n > 0; p1++, p2++, --n) {
    if (tolower((int) *p1) != tolower((int) *p2)) {
      return (int) (tolower((int) *p1) - tolower((int) *p2));
    } else if (*p1 == (unsigned char) '\0') {
      return 0;
    }
  }

  return 0;
}
/*
int isdigit(int c);
int isxdigit(int c);
int isprint(int c);
int isupper(int c);
int islower(int c);
int isalpha(int c);
int tolower(int c);
int toupper(int c);
int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmovevoid *dst, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
size_t strlen(const char *s);
char *strcpy(char *s1, const char *s2);
char *strncpy(char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
*/
#endif
