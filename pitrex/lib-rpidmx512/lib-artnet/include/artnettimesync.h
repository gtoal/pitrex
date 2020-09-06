/**
 * @file artnettimesync.h
 *
 */
/**
 * Art-Net Designed by and Copyright Artistic Licence Holdings Ltd.
 */
/* Copyright (C) 2017-2018 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

#ifndef ARTTIMESYNC_H_
#define ARTTIMESYNC_H_

#include <stdint.h>

#if  ! defined (PACKED)
#define PACKED __attribute__((packed))
#endif

struct TArtNetTimeSync {
	uint8_t tm_sec;
	uint8_t tm_min;
	uint8_t tm_hour;
	uint8_t tm_mday;
	uint8_t tm_mon;
	uint8_t tm_year_hi;
	uint8_t tm_year_lo;
	uint8_t tm_wday;
	uint8_t tm_isdst;
}PACKED;

class ArtNetTimeSync {
public:
	virtual ~ArtNetTimeSync(void);

	virtual void Handler(const struct TArtNetTimeSync *)= 0;
};

#endif /* ARTTIMESYNC_H_ */
