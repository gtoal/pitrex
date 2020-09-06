/**
 * @file esp8266.h
 *
 */
/* Copyright (C) 2016-2017 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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

#ifndef ESP8266_H_
#define ESP8266_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void esp8266_init(void);

extern const bool esp8266_detect(void);

extern void esp8266_write_4bits(const uint8_t);
extern void esp8266_write_byte(const uint8_t);
extern void esp8266_write_halfword(const uint16_t);
extern void esp8266_write_word(const uint32_t);
extern void esp8266_write_bytes(const uint8_t *, const uint16_t);
extern void esp8266_write_str(const char *);

extern uint8_t esp8266_read_byte(void);
extern void esp8266_read_bytes(/*@out@*/const uint8_t *, const uint16_t);
extern uint16_t esp8266_read_halfword(void);
extern uint32_t esp8266_read_word(void);
extern void esp8266_read_str(/*@out@*/char *, uint16_t *);

#ifdef __cplusplus
}
#endif

#endif /* ESP8266_H_ */
