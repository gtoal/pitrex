#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "vectors.h"
#include "rpi-gpio.h"
#include "rpi-aux.h"
#include "bcm2835_vc.h"

extern int __bss_start__;
extern int __bss_end__;
extern void main();


/** Main function - we'll never return from here */
void  kernelMain( unsigned int r0, unsigned int r1, unsigned int atags )
{
    tweakVectors();
    int32_t arm_clock = lib_bcm2835_vc_get_clock_rate(BCM2835_VC_CLOCK_ID_ARM);
#ifdef MHZ1000
    if (arm_clock != 1000000000)
    {
      lib_bcm2835_vc_set_clock_rate(BCM2835_VC_CLOCK_ID_ARM, 1000000000);
      RPI_AuxMiniUartInit( 115200, 8, 400000000);
    }
#else
    if (arm_clock != 700000000)
    {
       lib_bcm2835_vc_set_clock_rate(BCM2835_VC_CLOCK_ID_ARM, 700000000);
       RPI_AuxMiniUartInit( 115200, 8, 250000000);
    }
#endif    
    /* Print to the UART using the standard libc functions */
    printf("PiTrex starting...\r\n" );
    printf("BSS start: %X, end: %X\r\n", &__bss_start__, &__bss_end__);
    
    arm_clock = lib_bcm2835_vc_get_clock_rate(BCM2835_VC_CLOCK_ID_ARM);
    printf("ARM CLOCK  : %d MHz\r\n", (int) arm_clock);
    int32_t vc_clock = lib_bcm2835_vc_get_clock_rate(BCM2835_VC_CLOCK_ID_CORE);
    printf("GPU CLOCK  : %d MHz\r\n", (int) vc_clock);
    int32_t uart_clock = lib_bcm2835_vc_get_clock_rate(BCM2835_VC_CLOCK_ID_UART);
    printf("UART CLOCK : %d MHz\r\n", (int) uart_clock);
    
    /* Never exit as there is no OS to exit to! */
    while(1)
    {
      main(); // starting the "normal" main
    }
}
