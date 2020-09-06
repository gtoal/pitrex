// disclaimers see: disclaimer.txt


//.section ".text.startup"

.macro FUNC name
.text
.code 32
.global \name
\name:
.endm

.set _LOADER_START, 0x4000000

@ Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs
.set  MODE_USR, 0x10				@ User Mode
.set  MODE_FIQ, 0x11				@ FIQ Mode
.set  MODE_IRQ, 0x12				@ IRQ Mode
.set  MODE_SVC, 0x13				@ Supervisor Mode
.set  MODE_ABT, 0x17				@ Abort Mode
.set  MODE_UND, 0x1B				@ Undefined Mode
.set  MODE_SYS, 0x1F				@ System Mode

.set  I_BIT, 0x80				@ when I bit is set, IRQ is disabled
.set  F_BIT, 0x40				@ when F bit is set, FIQ is disabled

.set  __und_stack_top,_LOADER_START-0x100000+1*0x08000
.set  __abt_stack_top,_LOADER_START-0x100000+2*0x08000
.set  __fiq_stack_top,_LOADER_START-0x100000+3*0x08000
.set  __irq_stack_top,_LOADER_START-0x100000+4*0x08000
.set  __svc_stack_top,_LOADER_START-0x100000+4*0x08000+0x20000

.section .init
.code 32
.align 2
_preStart:
    @save boot parameter for later use
    mov r5, #0x0080
    str r0, [r5]
    mov r5, #0x0084
    str r1, [r5]
    mov r5, #0x0088
    str r2, [r5]

.align 2
.global _start
_start:

    @ set vectors
    ldr pc, reset_handler
    ldr pc, undefined_handler
    ldr pc, swi_handler
    ldr pc, prefetch_handler
    ldr pc, data_handler
    ldr pc, unused_handler
    ldr pc, irq_handler
    ldr pc, fiq_handler

reset_handler:		.word reset_callee
undefined_handler:	.word undefined_callee
swi_handler:		.word unused_callee
prefetch_handler:	.word abort_prefetch_callee
data_handler:		.word abort_data_callee
unused_handler:		.word unused_callee
irq_handler:		.word irq_callee
fiq_handler:		.word fiq_callee

reset_callee:
    @ Copy vectors to 0x0000, 16 words
    mov r0, #0x0000
    ldr r1, =_start
    ldmia r1!, {r2-r9}
    stmia r0!, {r2-r9}
    ldmia r1!, {r2-r9}
    stmia r0!, {r2-r9}

    // initialize stacks for all modes (overkill!)
    msr CPSR_c,#MODE_ABT|I_BIT|F_BIT 	@ Abort Mode
    ldr r0, =__abt_stack_top
    mov sp, r0

    msr CPSR_c,#MODE_UND|I_BIT|F_BIT 	@ Undefined Mode
    ldr r0, =__und_stack_top
    mov sp, r0

    msr CPSR_c,#MODE_IRQ|I_BIT|F_BIT 	@ IRQ Mode
    ldr r0, =__irq_stack_top
    mov sp, r0

    msr  CPSR_c,#MODE_FIQ|I_BIT|F_BIT	@ FIQ Mode
    ldr r0, =__fiq_stack_top
    mov sp, r0

    msr CPSR_c,#MODE_SVC|I_BIT|F_BIT	@ Supervisor Mode

    ldr r0, =__svc_stack_top
    mov sp, r0
    
    @ clear bss section
    mov   r0, #0
    ldr   r1, =__bss_start
    ldr   r2, =__bss_end__
4:  cmp   r1, r2
    strlo r0, [r1], #4
    blo   4b

    @ C function of the lib-rpidmx512
    @ to setup coprocessor and MMU
    bl vfp_init
    bl mmu_enable @ page table at 0x4000
    
    @ enable unaliogned memory access. Library mmu_enable doesn't do that!
    mrc	p15, 0, r0, c1, c0, 0	@ read SCTLR
    bic	r0, r0, #2		@ A (no unaligned access fault)
    orr	r0, r0, #1 << 22	@ U (v6 unaligned access model)
    mcr	p15, 0, r0, c1, c0, 0	@ write SCTLR	
    
	
    @ "https://www.raspberrypi.org/forums/viewtopic.php?f=63&t=155830"
    @ enable access to cycle counter register
    mov	r0, #1
    MCR p15, 0, r0, C15, C9, 0 @ enable user cycle count access
	
    b      kernelMain

    
FUNC halt
        bl print_halt
	wfe
	b halt

@ following are the actual exception handlers	

FUNC undefined_callee
        bl print_undefined
@	msr   CPSR_c, #MODE_SVC			@ Switch to SVC mode so we can the retrieve the orignal lr
	mov   r0, #0				@ Set type parameter
	sub   r1, lr, #4;			@ Set address parameter
						@ Subtracting 4 adjusts for the instruction queue giving the address of the instruction that caused this exception
   	b    reset_callee 			@ Call the debug_exception function - does not return

FUNC abort_prefetch_callee
        bl print_prefetch
	msr   CPSR_c, #MODE_SVC			@ Switch to SVC mode so we can the retrieve the orignal lr
	mov   r0, #1				@ Set type parameter
	sub   r1, lr, #4;			@ Set address parameter
						@ Subtracting 4 adjusts for the instruction queue giving the address of the instruction that caused this exception
   	b    reset_callee @debug_exception	@ Call the debug_exception function - does not return

FUNC abort_data_callee
        bl print_data
	mov   r0, #2				@ Set type parameter
	sub   r1, lr, #8;			@ Set address parameter
						@ Subtracting 8 adjusts for the instruction queue giving the address of the instruction that caused this exception
   	b    reset_callee @debug_exception	@ Call the debug_exception function - does not return

FUNC irq_callee 
        bl print_irq
        b c_irq_handler				@ void __attribute__((interrupt("IRQ"))) c_irq_handler(void)

FUNC fiq_callee
        bl print_fiq
        b c_fiq_handler				@ void __attribute__((interrupt("FIQ"))) c_fiq_handler(void)

FUNC unused_callee
        bl print_hang
        b unused_callee
    
FUNC _get_stack_pointer
        @ Return the stack pointer value
        str     sp, [sp]
        ldr     r0, [sp]
        mov     pc, lr    @ Return from the function

FUNC _enable_interrupts
        mrs     r0, cpsr
        bic     r0, r0, #0x80
        msr     cpsr_c, r0
        mov     pc, lr     @ Return from the function

/*
    .align 16

.global _kernelLoad
_kernelLoad:    
.INCBIN "linux.img"
*/