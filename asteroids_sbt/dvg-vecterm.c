/*
 * display.c: Atari DVG and AVG simulators
 *
 * Copyright 1991, 1992, 1996, 2003 Eric Smith
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: display.c 2 2003-08-20 01:51:05Z eric $


 2006-10-31 modified by dwelch

 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "../vecterm/vecterm_communication.h"

#define MAXSTACK 4

#define VCTR 0
#define HALT 1
#define SVEC 2
#define STAT 3
#define CNTR 4
#define JSRL 5
#define RTSL 6
#define JMPL 7
#define SCAL 8

#define DVCTR 0x01
#define DLABS 0x0a
#define DHALT 0x0b
#define DJSRL 0x0c
#define DRTSL 0x0d
#define DJMPL 0x0e
#define DSVEC 0x0f

#define twos_comp_val(num,bits) ((num&(1<<(bits-1)))?(num|~((1<<bits)-1)):(num&((1<<bits)-1)))

extern unsigned char ram[0x8000];
#define ReadMemory(x)  (ram[x])

static framegen_data *init_framegen_data(void) {
    static framegen_data frame;
    frame.options.use_ay_regs           = false;
    frame.options.use_led               = false;
    frame.options.use_led_dimmed        = false;
    frame.options.use_points            = false;

    frame.options.screen_swap_x_y       = false;
    frame.options.screen_flip_x         = false;
    frame.options.screen_flip_y         = false;
    frame.options.aspect_ratio_adjust   = false;
    switch (vecterm_orientation()) {
        case 3:
            frame.options.screen_swap_x_y       = true;
            frame.options.screen_flip_x         = true;
            frame.options.aspect_ratio_adjust   = true;
            break;
        case 2:
            frame.options.screen_flip_x         = true;
            frame.options.screen_flip_y         = true;
            break;
        case 1:
            break;
        default:
            frame.options.screen_swap_x_y       = true;
            frame.options.screen_flip_y         = true;
            frame.options.aspect_ratio_adjust   = true;
            break;
    }


    frame.options.aspect_ratio          = 1.0;//3.0/5.0;
    //frame.options.aspect_ratio          = 4.0/5.0; // this hack is compensated for by a counter-hack where SCREEN_H is changed from 800 to 1023 :-/
    
    frame.options.use_opt_1             = true;
    frame.options.use_opt_2             = true;
    frame.options.use_opt_point_offset  = false;
    frame.options.use_opt_point_char_detect  = false;
    frame.options.continue_distance     = 1;
    frame.options.point_size            = 8;
    
    // frame.measurement_flags             = VECTERM_MEASUREMENT_FLAGS_BUTTONS  | VECTERM_MEASUREMENT_FLAGS_DIGITAL_X | VECTERM_MEASUREMENT_FLAGS_DIGITAL_Y;
    frame.measurement_flags             = VECTERM_MEASUREMENT_FLAGS_BUTTONS  | VECTERM_MEASUREMENT_FLAGS_ANALOG_X | VECTERM_MEASUREMENT_FLAGS_ANALOG_Y;
    
    static float tmp_mem[5*16*1024];
    frame.max_line_count = 16*1024;
    frame.ptr = tmp_mem;
    return (&frame);
}

framegen_data *frame;

int textinit ( void )
{
  frame = init_framegen_data();
  if (!vecterm_open_connection(0)) {
    printf("error: opening vecterm connection failed\n");
    return (-1);
  }
  frame->line_count = 0;
  return 0;
}


#define SCREEN_H 1024
#define SCREEN_W 1024

void draw_line ( long oldx, long oldy, long currentx, long currenty, int xyz, int z)
{
    if(z==0) return;

    oldy=SCREEN_H-oldy-250;
    currenty=SCREEN_H-currenty-250;

    //fprintf(stdout, "line(%d,%d  %d,%d   %d   %d)\n", oldx,oldy, currentx,currenty,  xyz, z);
    // lineBresenham(oldx/4,oldy/4,currentx/4,currenty/4);

    if (frame->line_count < frame->max_line_count) {
      float *fptr = frame->ptr + 5*frame->line_count++;
      fptr[0] = (float)oldx/(float)SCREEN_W;
      fptr[1] = 1.0-(float)(oldy+50)/(float)SCREEN_H; // +50 to move everything up a little
      fptr[2] = (float)currentx/(float)SCREEN_W;
      fptr[3] = 1.0-(float)(currenty+50)/(float)SCREEN_H;
      fptr[4] = (float)(z&15)/15.0;
    }
}

long keycode = 0L, tmp_btns;

unsigned long readkeypad ( void )
{
  return keycode;
}

void poll_buttons(void) {
  keycode = 0L; tmp_btns = vecterm_get_buttons(); // READ ONCE PER FRAME
  
  //mx = vecterm_get_joystick_analog_x_controller_0(); // actually should use digital for rotate left/right
  //my = vecterm_get_joystick_analog_y_controller_0();

  // or mx < -32
  if (tmp_btns & (VECTERM_BUTTON_0_1 | VECTERM_BUTTON_1_1)) keycode |= 0x00001000; // ROTATE LEFT
  // or mx > 32
  if (tmp_btns & (VECTERM_BUTTON_0_2 | VECTERM_BUTTON_1_2)) keycode |= 0x00002000; // ROTATE RIGHT
  if (tmp_btns & (VECTERM_BUTTON_0_3 | VECTERM_BUTTON_1_3)) keycode |= 0x00800000; // THRUST
  if (tmp_btns & (VECTERM_BUTTON_0_4 | VECTERM_BUTTON_1_4)) keycode |= 0x00400000; // FIRE  (+ 2 player start)
  if ((keycode&0x00C00000) == 0x00C00000) keycode = 0x00000100; // START/HYPERSPACE when THRUST+FIRE both pressed (+ 1 player start)
  // passed back to game via readkeypad above
}

void dvg_draw_screen (void)
{
    int pc;
    int sp;
    int stack [MAXSTACK];

    long scale;
    int statz;

    long currentx;
    long currenty;

    int done = 0;

    int firstwd, secondwd;
    int opcode;

    long x, y;
    int z, temp;
    int a;

    long oldx, oldy;
    long deltax, deltay;

    pc = 0;
    sp = 0;
    scale = 0;
    statz = 0;
    currentx = 1023 * 8192;
    currenty = 512 * 8192;
    //currentx = 512 * 8192;
    //currenty = 1023 * 8192;

    while (!done)
    {
        firstwd = ReadMemory(0x4000+(pc<<1)+1); firstwd<<=8; firstwd|=ReadMemory(0x4000+(pc<<1)+0);
        opcode = firstwd >> 12;
        pc++;
        if ((opcode >= 0 /* DVCTR */) && (opcode <= DLABS))
        {
            secondwd = ReadMemory(0x4000+(pc<<1)+1); secondwd<<=8; secondwd|=ReadMemory(0x4000+(pc<<1)+0);
            pc++;
        }


        switch (opcode)
        {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                y = firstwd & 0x03ff; if (firstwd & 0x0400) y = -y;
                x = secondwd & 0x03ff;if (secondwd & 0x400) x = -x;
                z = secondwd >> 12;
                oldx = currentx; oldy = currenty;
                temp = (scale + opcode) & 0x0f;
                if (temp > 9) temp = -1;
                deltax = (x << 21) >> (30 - temp);
                deltay = (y << 21) >> (30 - temp);
                currentx += deltax;
                currenty -= deltay;
                draw_line (oldx, oldy, currentx, currenty, 7, z);
                break;

            case DLABS:
                x = twos_comp_val (secondwd, 12);
                y = twos_comp_val (firstwd, 12);
                /*
                x = secondwd & 0x07ff;
                if (secondwd & 0x0800)
                x = x - 0x1000;
                y = firstwd & 0x07ff;
                if (firstwd & 0x0800)
                y = y - 0x1000;
                */
                scale = secondwd >> 12;
                currentx = x;
                currenty = (896 - y);
                break;

            case DHALT:
                done = 1;
                break;

            case DJSRL:
                a = firstwd & 0x0fff;
                stack [sp] = pc;
                if (sp == (MAXSTACK - 1))
                {
                    printf ("\n*** Vector generator stack overflow! ***\n");
                    done = 1;
                    sp = 0;
                }
                else
                    sp++;
                pc = a;
                break;

            case DRTSL:
                if (sp == 0)
                {
                    printf ("\n*** Vector generator stack underflow! ***\n");
                    done = 1;
                    sp = MAXSTACK - 1;
                }
                else
                    sp--;
                pc = stack [sp];
                break;

            case DJMPL:
                a = firstwd & 0x0fff;
                pc = a;
                break;

            case DSVEC:
                y = firstwd & 0x0300; if (firstwd & 0x0400) y = -y;
                x = (firstwd & 0x03) << 8; if (firstwd & 0x04) x = -x;
                z = (firstwd >> 4) & 0x0f;
                temp = 2 + ((firstwd >> 2) & 0x02) + ((firstwd >> 11) & 0x01);
                oldx = currentx; oldy = currenty;
                temp = (scale + temp) & 0x0f;
                if (temp > 9) temp = -1;
                deltax = (x << 21) >> (30 - temp);
                deltay = (y << 21) >> (30 - temp);
                currentx += deltax;
                currenty -= deltay;
                draw_line (oldx, oldy, currentx, currenty, 7, z);
                break;

            default:
                printf("internal error\n");
                done = 1;
        }
    }
    // page_flip();
    // should wait for 50hz timer here
    vecterm_wait_for_elapsed_frame();  
    vecterm_new_frame(frame);
    frame->line_count = 0;
    poll_buttons();
}


