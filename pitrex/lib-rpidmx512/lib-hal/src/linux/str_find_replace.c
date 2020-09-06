/**
 * @file str_find_replace.h
 *
 */
/* Copyright (C) 2018 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

#include <string.h>
#include <assert.h>

char *str_find_replace(char *str, const char *find, const char *replace) {
	assert(strlen(replace) <= strlen(find));

	unsigned i, j, k, n, m;

	 i = j = m = n = 0;

	while (str[i] != '\0') {
		if (str[m] == find[n]) {
			m++;
			n++;
			if (find[n] == '\0') {
				for (k = 0; replace[k] != '\0'; k++, j++) {
					str[j] = replace[k];
				}
				n = 0;
				i = m;
			}
		} else {
			str[j] = str[i];
			j++;
			i++;
			m = i;
			n = 0;
		}
	}

	for (; j < i; j++) {
		str[j] = '\0';
	}

	return str;
}
