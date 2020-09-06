/**
 * @file detect.c
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

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>

#include "bcm2835.h"

#include "spi_flash.h"

int main(int argc, char **argv) {
	int ret;
	unsigned int cs = 0, max_hz = 0, spi_mode = 0; /* Dummy for now */


	if (getuid() != 0) {
		fprintf(stderr, "Error: Not started with 'root'\n");
		return -1;
	}

	if (bcm2835_init() != 1) {
		fprintf(stderr, "bcm2835_init() failed\n");
		return -2;
	}

	ret = spi_flash_probe(cs, max_hz, spi_mode);

	printf("spi_flash_probe=%d\n", ret);

	if (ret == 0) {
		printf("Detected %s with sector size %d total %d bytes\n", spi_flash_get_name(), spi_flash_get_sector_size(), spi_flash_get_size());
	}

#if 0
	int i;
	uint8_t buffer[16];

	ret = spi_flash_cmd_erase(0, 4096);

	printf("spi_flash_cmd_erase=%d\n", ret);

	ret = spi_flash_cmd_read_fast(0, sizeof(buffer), buffer);

	printf("spi_flash_cmd_read_fast=%d\n", ret);

	for (i = 0; i < sizeof(buffer); i++) {
		printf("%.2x[%c]", buffer[i], isprint(buffer[i]) ? buffer[i] : '.');
	}

	printf("\n");

	ret = spi_flash_cmd_write_multi(0, 6, (const void *)"Arjan");

	printf("spi_flash_cmd_write_multi=%d\n", ret);

	ret = spi_flash_cmd_read_fast(0, sizeof(buffer), buffer);

	printf("spi_flash_cmd_read_fast=%d\n", ret);

	for (i = 0; i < sizeof(buffer); i++) {
		printf("%.2x[%c]", buffer[i], isprint(buffer[i]) ? buffer[i] : '.');
	}
#endif

	printf("\nDone!\n");

	return 0;
}
