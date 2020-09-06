/**
 * @file mcp4902.c
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

/*
 * MCP4902: Dual 12-Bit Voltage Output DAC
 */

#include <stdint.h>
#include <stdbool.h>

#include "bob.h"

#include "mcp4902.h"
#include "mcp49x2.h"

#define MCP4902_DATA(x)			((uint16_t)((uint8_t)(x) << 4))

bool mcp4902_start(device_info_t *device_info) {

	if (device_info->speed_hz == 0) {
		device_info->speed_hz = MCP49X2_SPI_SPEED_DEFAULT_HZ;
	} else if (device_info->speed_hz > (uint32_t) MCP49X2_SPI_SPEED_MAX_HZ) {
		device_info->speed_hz = MCP49X2_SPI_SPEED_MAX_HZ;
	}

	if (device_info->chip_select >= SPI_CS2) {
		device_info->chip_select = SPI_CS2;
		lib_bcm2835_aux_spi_begin();
		device_info->internal.clk_div = lib_bcm2835_aux_spi_CalcClockDivider(device_info->speed_hz);
	} else {
		FUNC_PREFIX(spi_begin());;
	}

	return true;
}

void mcp4902_write_a(const device_info_t *device_info, uint8_t data) {
	data = MCP4902_DATA(data) | 0x3000 | MCP49X2_WRITE_DAC_A;

	if (device_info->chip_select == SPI_CS2) {
		lib_bcm2835_aux_spi_setClockDivider(device_info->internal.clk_div);
		lib_bcm2835_aux_spi_write(data);
	} else {
		FUNC_PREFIX(spi_set_speed_hz(device_info->speed_hz));
		FUNC_PREFIX(spi_chipSelect(device_info->chip_select));
		FUNC_PREFIX(spi_write(data));
	}
}

void mcp4902_write_b(const device_info_t *device_info, uint8_t data) {
	data = MCP4902_DATA(data) | 0x3000 | MCP49X2_WRITE_DAC_B;

	if (device_info->chip_select == SPI_CS2) {
		lib_bcm2835_aux_spi_setClockDivider(device_info->internal.clk_div);
		lib_bcm2835_aux_spi_write(data);
	} else {
		FUNC_PREFIX(spi_set_speed_hz(device_info->speed_hz));
		FUNC_PREFIX(spi_chipSelect(device_info->chip_select));
		FUNC_PREFIX(spi_setDataMode(SPI_MODE0));
		FUNC_PREFIX(spi_write(data));
	}

}

void mcp4902_write_ab(const device_info_t *device_info, uint8_t data_a, uint8_t data_b) {
	data_a = MCP4902_DATA(data_a) | 0x3000 | MCP49X2_WRITE_DAC_A;
	data_b = MCP4902_DATA(data_b) | 0x3000 | MCP49X2_WRITE_DAC_B;

	if (device_info->chip_select == SPI_CS2) {
		lib_bcm2835_aux_spi_setClockDivider(device_info->internal.clk_div);
		lib_bcm2835_aux_spi_write(data_a);
		lib_bcm2835_aux_spi_write(data_b);
	} else {
		FUNC_PREFIX(spi_set_speed_hz(device_info->speed_hz));
		FUNC_PREFIX(spi_chipSelect(device_info->chip_select));
		FUNC_PREFIX(spi_write(data_a));
		FUNC_PREFIX(spi_write(data_b));
	}

}
