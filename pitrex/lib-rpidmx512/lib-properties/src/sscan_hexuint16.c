/**
 * @file sscan_hexuint16.c
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
#include <stddef.h>
#include <ctype.h>
#include <assert.h>

#include "sscan.h"

extern char *get_name(const char *buf, const char *name);

int sscan_hexuint16(const char *buf, const char *name, uint16_t *uint16) {
	assert(buf != NULL);
	assert(name != NULL);
	assert(uint16 != NULL);

	char *b;
	int k = 0;
	uint8_t nibble;
	uint16_t tmp;

	if ((b = get_name(buf, name)) == NULL) {
		return SSCAN_NAME_ERROR;
	}

	k = 0;
	tmp = 0;

	while ((*b != (char) '\0') && (k < 4)) {
		if (isxdigit((int) *b) == 0) {
			return SSCAN_NAME_ERROR;
		}
		nibble = *b > '9' ? ((uint8_t) *b | (uint8_t) 0x20) - (uint8_t) 'a' + (uint8_t) 10 : (uint8_t) (*b - '0');
		tmp = (tmp << 4) | nibble;
		k++;
		b++;
	}

	if ((*b != (char) '\0') && (*b != (char) ' ')) {
		return SSCAN_NAME_ERROR;
	}

	if (k != 4) {
		return SSCAN_VALUE_ERROR;
	}

	*uint16 = tmp;

	return SSCAN_OK;
}
