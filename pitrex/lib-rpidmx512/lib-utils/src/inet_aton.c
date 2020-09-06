#if defined (BARE_METAL)
/**
 * @file inet_aton.c
 *
 */
/* Copyright (C) 2016-2018 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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
#include <ctype.h>

typedef union pcast32 {
	uint32_t u32;
	uint8_t u8[4];
} _pcast32;

int inet_aton(const char *cp, uint32_t *ip_address) {
	const char *b = cp;
	int i, j, k;
	_pcast32 cast32;

	for (i = 0 ; i < 3 ; i++) {
			j = 0;
			k = 0;

			while ((*b != '.') && (*b != (char) 0) && (*b != '\n')) {
				if (j == 3) {
					return 0;
				}

				if (0 == isdigit((int )*b)) {
					return 0;
				}

				j++;
				k = k * 10 + (int) *b - (int) '0';
				b++;
			}

			cast32.u8[i] = k;
			b++;

		}

		j= 0;
		k= 0;

		while ((*b != ' ') && (*b != (char) 0) && (*b != '\n')) {
			if (j == 3) {
				return 0;
			}

			if (0 == isdigit((int )*b)) {
				return 0;
			}

			j++;
			k = k * 10 + (int) *b - (int) '0';
			b++;
		}

		cast32.u8[i] = k;

		if (ip_address != 0) {
			*ip_address = cast32.u32;
		}

		return 1;
}
#endif
