/**
 * @file hal_spi.h
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

#ifndef HAL_SPI_H_
#define HAL_SPI_H_

#if defined(__linux__)
 #include "bcm2835.h"
#elif defined(H3)
 #include "h3_spi.h"
#else
 #include "bcm2835_spi.h"
#endif

#if defined(H3)
 #define SPI_BIT_ORDER_MSBFIRST	H3_SPI_BIT_ORDER_MSBFIRST
 #define SPI_MODE0				H3_SPI_MODE0
 #define SPI_MODE3				H3_SPI_MODE3
 #define SPI_CS0				H3_SPI_CS0
 #define SPI_CS_NONE			H3_SPI_CS_NONE
#else
 #define SPI_BIT_ORDER_MSBFIRST	BCM2835_SPI_BIT_ORDER_MSBFIRST
 #define SPI_MODE0				BCM2835_SPI_MODE0
 #define SPI_MODE3				BCM2835_SPI_MODE3
 #define SPI_CS0				BCM2835_SPI_CS0
 #define SPI_CS_NONE			BCM2835_SPI_CS_NONE
#endif

#if defined(H3)
 #define FUNC_PREFIX(x) h3_##x
#else
 #define FUNC_PREFIX(x) lib_bcm2835_##x
#endif

#endif /* HAL_SPI_H_ */
