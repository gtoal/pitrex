/**
 * @file led.c
 *
 */
/* Copyright (C) 2018-2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>

#include "h3.h"

#include "c/hardware.h"

static uint32_t ticks_per_second = 1000000 / 2;
static uint32_t led_counter = 0;
static uint32_t micros_previous = 0;

void led_set_ticks_per_second(uint32_t ticks) {
	ticks_per_second = ticks;
}

void led_blink(void) {
	if (__builtin_expect (ticks_per_second == 0, 0)) {
		return;
	}

	const uint32_t micros_now = H3_TIMER->AVS_CNT1;

	if (__builtin_expect ((micros_now - micros_previous < ticks_per_second), 0)) {
		return;
	}

	micros_previous = micros_now;

	led_counter ^= 0x1;
	hardware_led_set((int) led_counter);
}
