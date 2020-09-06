/**
 * @file sscan_uin32_t.c
 *
 */
/* Copyright (C) 2016-2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

int sscan_uint32_t(const char *buf, const char *name, uint32_t *value) {
	assert(buf != NULL);
	assert(name != NULL);
	assert(value != NULL);

	int64_t k;
	char *b;

	if ((b = get_name(buf, name)) == NULL) {
		return SSCAN_NAME_ERROR;
	}

	k = 0;

	do {
		if (isdigit((int) *b) == 0) {
			return SSCAN_VALUE_ERROR;
		}
		k = k * 10 + (int64_t) *b - (int64_t) '0';
		b++;
	} while ((*b != ' ') && (*b != (char) 0));

	if (k > (int64_t) ((uint32_t) ~0)) {
		return SSCAN_VALUE_ERROR;
	}

	*value = (uint32_t) k;

	return SSCAN_OK;
}
