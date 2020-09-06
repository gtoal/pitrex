/**
 * @file devicesparamsconst.h
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

#ifndef DEVICESPARAMSCONST_H_
#define DEVICESPARAMSCONST_H_

#include <stdint.h>

class DevicesParamsConst {
public:
	alignas(uint32_t) static const char FILE_NAME[];
	alignas(uint32_t) static const char LED_TYPE[];
	alignas(uint32_t) static const char LED_COUNT[];

	alignas(uint32_t) static const char ACTIVE_OUT[];
	alignas(uint32_t) static const char USE_SI5351A[];

	alignas(uint32_t) static const char DMX_START_ADDRESS[];
	alignas(uint32_t) static const char SPI_SPEED_HZ[];

	alignas(uint32_t) static const char LED_GROUPING[];
	alignas(uint32_t) static const char LED_GROUP_COUNT[];

	alignas(uint32_t) static const char GLOBAL_BRIGHTNESS[];
};

#endif /* DEVICESPARAMSCONST_H_ */
