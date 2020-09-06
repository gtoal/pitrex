/**
 * @file lightsetgetoutputtype.cpp
 *
 */
/* Copyright (C) 2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

#include "lightset.h"

#ifndef ALIGNED
 #define ALIGNED __attribute__ ((aligned (4)))
#endif

static const char sOutput[LIGHTSET_OUTPUT_TYPE_UNDEFINED][4] ALIGNED = {"dmx", "spi", "mon"};

const char* LightSet::GetOutputType(enum TLightSetOutputType type) {
	assert(type < LIGHTSET_OUTPUT_TYPE_UNDEFINED);

	return sOutput[type];
}

TLightSetOutputType LightSet::GetOutputType(const char* sType) {
	for (uint32_t i = 0; i < sizeof(sOutput) / sizeof(sOutput[0]); i++) {
		if (strncasecmp(sOutput[i], sType, 3) == 0) {
			return (TLightSetOutputType) i;
		}
	}

	return LIGHTSET_OUTPUT_TYPE_DMX;
}
