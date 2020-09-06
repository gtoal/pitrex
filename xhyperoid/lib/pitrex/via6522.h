#ifndef VIA6522_H
#define VIA6522_H

// Vectrex 6522 Addresses (from: http://playvectrex.com/designit/chrissalo/via3.htm):
#define VIA_port_b           0xD000   //VIA port B data I/O register
/*
*       0 sample/hold (0=enable  mux 1=disable mux)
*       1 mux sel 0
*       2 mux sel 1
*       3 sound BC1
*       4 sound BDIR
*       5 comparator input
*       6 external device (slot pin 35) initialized to input
*       7 /RAMP
*/
#define VIA_port_a           0xD001   //VIA port A data I/O register (handshaking)
#define VIA_DDR_b            0xD002   //VIA port B data direction register (0=input 1=output)
#define VIA_DDR_a            0xD003   //VIA port A data direction register (0=input 1=output)
#define VIA_t1_cnt_lo        0xD004   //VIA timer 1 count register lo (scale factor)
#define VIA_t1_cnt_hi        0xD005   //VIA timer 1 count register hi
#define VIA_t1_lch_lo        0xD006   //VIA timer 1 latch register lo
#define VIA_t1_lch_hi        0xD007   //VIA timer 1 latch register hi
#define VIA_t2_lo            0xD008   //VIA timer 2 count/latch register lo (refresh)
#define VIA_t2_hi            0xD009   //VIA timer 2 count/latch register hi
#define VIA_shift_reg        0xD00A   //VIA shift register
#define VIA_aux_cntl         0xD00B   //VIA auxiliary control register
/*
*       0 PA latch enable
*       1 PB latch enable
*       2 \                     110=output to CB2 under control of phase 2 clock
*       3  > shift register control     (110 is the only mode used by the Vectrex ROM)
*       4 /
*       5 0=t2 one shot                 1=t2 free running
*       6 0=t1 one shot                 1=t1 free running
*       7 0=t1 disable PB7 output       1=t1 enable PB7 output
*/
#define VIA_cntl             0xD00C   //VIA control register
/*
*       0 CA1 control     CA1 -> SW7    0=IRQ on low 1=IRQ on high
*       1 \
*       2  > CA2 control  CA2 -> /ZERO  110=low 111=high
*       3 /
*       4 CB1 control     CB1 -> NC     0=IRQ on low 1=IRQ on high
*       5 \
*       6  > CB2 control  CB2 -> /BLANK 110=low 111=high
*       7 /
*/
#define VIA_int_flags        0xD00D   //VIA interrupt flags register
/*
*               bit                             cleared by
*       0 CA2 interrupt flag            reading or writing port A I/O
*       1 CA1 interrupt flag            reading or writing port A I/O
*       2 shift register interrupt flag reading or writing shift register
*       3 CB2 interrupt flag            reading or writing port B I/O
*       4 CB1 interrupt flag            reading or writing port A I/O
*       5 timer 2 interrupt flag        read t2 low or write t2 high
*       6 timer 1 interrupt flag        read t1 count low or write t1 high
*       7 IRQ status flag               write logic 0 to IER or IFR bit
*/
#define VIA_int_enable       0xD00E   //VIA interrupt enable register
/*
*       0 CA2 interrupt enable
*       1 CA1 interrupt enable
*       2 shift register interrupt enable
*       3 CB2 interrupt enable
*       4 CB1 interrupt enable
*       5 timer 2 interrupt enable
*       6 timer 1 interrupt enable
*       7 IER set/clear control
*/
#define VIA_port_a_nohs      0xD00F   //VIA port A data I/O register (no handshaking)

#endif /* VIA6522_H */
