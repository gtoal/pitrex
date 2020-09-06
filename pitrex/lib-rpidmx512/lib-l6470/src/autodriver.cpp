/**
 * @file autodriver.cpp
 *
 */
/*
 * Based on https://github.com/sparkfun/L6470-AutoDriver/tree/master/Libraries/Arduino
 */
/* Copyright (C) 2017-2019 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
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
#include <assert.h>

#include "hal_spi.h"
#include "hal_gpio.h"

#include "autodriver.h"

#include "l6470constants.h"

#include "debug.h"

#define BUSY_PIN_NOT_USED	0xFF

uint8_t AutoDriver::m_nNumBoards[2];

AutoDriver::AutoDriver(uint8_t nPosition, uint8_t nSpiChipSelect, uint8_t nResetPin, uint8_t nBusyPin) :
	m_nSpiChipSelect(nSpiChipSelect),
	m_nResetPin(nResetPin),
	m_nBusyPin(nBusyPin),
	m_nPosition(nPosition),
	m_bIsBusy(false),
	m_bIsConnected(false)
{
	DEBUG_ENTRY

	m_nNumBoards[nSpiChipSelect]++;

	if (getParam(L6470_PARAM_CONFIG) == 0x2e88) {
		m_bIsConnected = true;
	}

	DEBUG_PRINTF("m_bIsConnected=%d", (int) m_bIsConnected);
	DEBUG_EXIT
}

AutoDriver::AutoDriver(uint8_t nPosition, uint8_t nSpiChipSelect, uint8_t nResetPin) :
	m_nSpiChipSelect(nSpiChipSelect),
	m_nResetPin(nResetPin),
	m_nBusyPin(BUSY_PIN_NOT_USED),
	m_nPosition(nPosition),
	m_bIsBusy(false),
	m_bIsConnected(false)
{
	DEBUG_ENTRY

	m_nNumBoards[nSpiChipSelect]++;

	if (getParam(L6470_PARAM_CONFIG) == 0x2e88) {
		m_bIsConnected = true;
	}

	DEBUG_PRINTF("m_bIsConnected=%d", (int) m_bIsConnected);
	DEBUG_EXIT
}

AutoDriver::~AutoDriver(void) {
	hardHiZ();
	m_bIsBusy = false;
	m_bIsConnected = false;
	m_nNumBoards[m_nSpiChipSelect]--;
}

int AutoDriver::busyCheck(void) {
	if (m_nBusyPin == BUSY_PIN_NOT_USED) {
		if (getParam(L6470_PARAM_STATUS) & L6470_STATUS_BUSY) {
			return 0;
		} else {
			return 1;
		}
	} else {
		if (!m_bIsBusy) {
			if (getParam(L6470_PARAM_STATUS) & L6470_STATUS_BUSY) {
				return 0;
			} else {
				m_bIsBusy = true;
				return 1;
			}
		}
		// By default, the BUSY pin is forced low when the device is performing a command
		if (FUNC_PREFIX(gpio_lev(m_nBusyPin)) == HIGH) {
			m_bIsBusy = false;
			return 0;
		} else {
			return 1;
		}
	}
}

uint8_t AutoDriver::SPIXfer(uint8_t data) {
	uint8_t dataPacket[m_nNumBoards[m_nSpiChipSelect]];

	for (int i = 0; i < m_nNumBoards[m_nSpiChipSelect]; i++) {
		dataPacket[i] = 0;
	}

	dataPacket[m_nPosition] = data;

	FUNC_PREFIX(spi_chipSelect(m_nSpiChipSelect));
	FUNC_PREFIX(spi_set_speed_hz(4000000));
	FUNC_PREFIX(spi_setDataMode(SPI_MODE3));
	FUNC_PREFIX(spi_transfern((char *) dataPacket, m_nNumBoards[m_nSpiChipSelect]));

	return dataPacket[m_nPosition];
}

uint16_t AutoDriver::getNumBoards(void) {
	int n = 0;
	for (int i = 0; i < (int) (sizeof(m_nNumBoards) / sizeof(m_nNumBoards[0])); i++) {
		n += m_nNumBoards[i];
	}
	return n;
}

uint8_t AutoDriver::getNumBoards(int cs) {
	if (cs < (int) (sizeof(m_nNumBoards) / sizeof(m_nNumBoards[0]))) {
		return m_nNumBoards[cs];
	} else {
		return 0;
	}
}
