#if defined (RASPPI) || defined(BARE_METAL)
/**
 * @file sensorbh1750.cpp
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

#include <stdint.h>
#include <string.h>
#ifndef NDEBUG
 #include <stdio.h>
#endif

#ifndef ALIGNED
 #define ALIGNED __attribute__ ((aligned (4)))
#endif

#include "sensorbh1750.h"
#include "rdm_e120.h"

#include "bh1750.h"

static struct _device_info sDeviceInfo;

SensorBH1750::SensorBH1750(uint8_t nSensor, uint8_t nAddress): RDMSensor(nSensor) {
	SetType(E120_SENS_ILLUMINANCE);
	SetUnit(E120_UNITS_LUX);
	SetPrefix(E120_PREFIX_NONE);
	SetRangeMin(0);
	SetRangeMax(RDM_SENSOR_RANGE_MAX);
	SetNormalMin(0);
	SetNormalMax(RDM_SENSOR_RANGE_MAX);
	SetDescription("Ambient Light");

	memset(&sDeviceInfo, 0, sizeof(struct _device_info));
	sDeviceInfo.slave_address = nAddress;
}

SensorBH1750::~SensorBH1750(void) {
}

bool SensorBH1750::Initialize(void) {
	const bool IsConnected = bh1750_start(&sDeviceInfo);

#ifndef NDEBUG
	printf("%s\tIsConnected=%d\n", __FUNCTION__, (int) IsConnected);
#endif
	return IsConnected;
}

int16_t SensorBH1750::GetValue(void) {
	const int16_t nValue = (int16_t) (bh1750_get_level(&sDeviceInfo) & (uint16_t) 0x7FFF);

#ifndef NDEBUG
	printf("%s\tnValue=%d\n", __FUNCTION__, (int) nValue);
#endif
	return nValue;
}
#endif
