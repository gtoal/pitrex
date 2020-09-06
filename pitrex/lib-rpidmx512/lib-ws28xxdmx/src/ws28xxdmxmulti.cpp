/**
 * @file ws28xxdmxmulti.h
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

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "ws28xxdmxmulti.h"
#include "ws28xxmulti.h"

#include "ws28xxdmxparams.h"

#include "debug.h"

#ifndef MIN
 #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

WS28xxDmxMulti::WS28xxDmxMulti(TWS28xxDmxMultiSrc tSrc):
	m_tSrc(tSrc),
	m_tLedType(WS28XXMULTI_WS2812B),
	m_nLedCount(170),
	m_nActiveOutputs(WS28XXMULTI_ACTIVE_PORTS_MAX),
	m_pLEDStripe(0),
	m_bIsStarted(false),
	m_bBlackout(false),
	m_nUniverses(1), // -> m_nLedCount(170)
	m_nBeginIndexPortId1(170),
	m_nBeginIndexPortId2(340),
	m_nBeginIndexPortId3(510),
	m_nChannelsPerLed(3),
	m_nPortIdLast(3), // -> (m_nActiveOutputs * m_nUniverses) -1;
	m_bUseSI5351A(false)
{
	DEBUG_ENTRY

	UpdateMembers();

	DEBUG_EXIT
}

WS28xxDmxMulti::~WS28xxDmxMulti(void) {
	Stop(0);

	delete m_pLEDStripe;
	m_pLEDStripe = 0;
}

void WS28xxDmxMulti::Start(uint8_t nPort) {
	DEBUG_PRINTF("%d", (int) nPort);

	if (m_bIsStarted) {
		return;
	}

	m_bIsStarted = true;

	if (m_pLEDStripe == 0) {
		m_pLEDStripe = new WS28xxMulti(m_tLedType, m_nLedCount, m_nActiveOutputs, m_bUseSI5351A);
		assert(m_pLEDStripe != 0);
		m_pLEDStripe->Blackout();
	} else {
		m_pLEDStripe->Update();
	}
}

void WS28xxDmxMulti::Stop(uint8_t nPort) {
	DEBUG_PRINTF("%d", (int) nPort);

	if (!m_bIsStarted) {
		return;
	}

	m_bIsStarted = false;

	if (m_pLEDStripe != 0) {
		m_pLEDStripe->Blackout();
	}
}

void WS28xxDmxMulti::SetData(uint8_t nPortId, const uint8_t* pData, uint16_t nLength) {
	assert(pData != 0);
	assert(nLength <= DMX_MAX_CHANNELS);

	uint32_t i = 0;
	uint32_t beginIndex, endIndex;

	if (__builtin_expect((m_pLEDStripe == 0), 0)) {
		Start(0);
	}

	switch (nPortId & ~(uint8_t)m_nUniverses & (uint8_t)0x03) {
	case 0:
		beginIndex = 0;
		endIndex = MIN(m_nLedCount, (nLength / m_nChannelsPerLed));
		break;
	case 1:
		beginIndex = m_nBeginIndexPortId1;
		endIndex = MIN(m_nLedCount, (beginIndex + (nLength / m_nChannelsPerLed)));
		break;
	case 2:
		beginIndex = m_nBeginIndexPortId2;
		endIndex = MIN(m_nLedCount, (beginIndex + (nLength / m_nChannelsPerLed)));
		break;
	case 3:
		beginIndex = m_nBeginIndexPortId3;
		endIndex = MIN(m_nLedCount, (beginIndex + (nLength / m_nChannelsPerLed)));
		break;
	default:
		__builtin_unreachable();
		break;
	}

	uint32_t nOutIndex;

	if (m_tSrc == WS28XXDMXMULTI_SRC_E131) {
		nOutIndex = nPortId / m_nUniverses;
	} else {
		nOutIndex = nPortId / 4;
	}

	DEBUG_PRINTF("nPort=%d, nLength=%d, nOutIndex=%d, nPortId=%d, beginIndex=%d, endIndex=%d",
			(int ) nPortId, (int ) nLength, (int ) nOutIndex,
			(int )nPortId & ~m_nUniverses & 0x03, (int)beginIndex, (int)endIndex);

	for (uint32_t j = beginIndex; j < endIndex; j++) {
		__builtin_prefetch(&pData[i]);
		if (m_tLedType == WS28XXMULTI_SK6812W) {
			if (i + 3 > nLength) {
				break;
			}
			m_pLEDStripe->SetLED(nOutIndex, j, pData[i], pData[i + 1], pData[i + 2], pData[i + 3]);
			i = i + 4;
		} else {
			if (i + 2 > nLength) {
				break;
			}
			m_pLEDStripe->SetLED(nOutIndex, j, pData[i], pData[i + 1], pData[i + 2]);
			i = i + 3;
		}
	}

	if (nPortId == m_nPortIdLast) {
		m_pLEDStripe->Update();
	}
}

void WS28xxDmxMulti::Blackout(bool bBlackout) {
	m_bBlackout = bBlackout;

	if (bBlackout) {
		m_pLEDStripe->Blackout();
	} else {
		m_pLEDStripe->Update();
	}
}

void WS28xxDmxMulti::SetLEDType(TWS28xxMultiType tWS28xxMultiType) {
	DEBUG_ENTRY

	m_tLedType = tWS28xxMultiType;

	if (tWS28xxMultiType == WS28XXMULTI_SK6812W) {
		m_nBeginIndexPortId1 = 128;
		m_nBeginIndexPortId2 = 256;
		m_nBeginIndexPortId3 = 384;

		m_nChannelsPerLed = 4;
	}

	UpdateMembers();

	DEBUG_EXIT
}

void WS28xxDmxMulti::SetLEDCount(uint16_t nLedCount) {
	DEBUG_ENTRY

	m_nLedCount = nLedCount;

	UpdateMembers();

	DEBUG_EXIT
}

void WS28xxDmxMulti::SetActivePorts(uint8_t nActiveOutputs) {
	DEBUG_ENTRY

	m_nActiveOutputs = nActiveOutputs;

	UpdateMembers();

	DEBUG_EXIT
}

void WS28xxDmxMulti::UpdateMembers(void) {
	m_nUniverses = 1 + (m_nLedCount / (1 + m_nBeginIndexPortId1));

	if (m_tSrc == WS28XXDMXMULTI_SRC_E131) {
		m_nPortIdLast = (m_nActiveOutputs * m_nUniverses)  - 1;
	} else {
		m_nPortIdLast = ((m_nActiveOutputs - 1) * 4) + m_nUniverses - 1;
	}

	DEBUG_PRINTF("m_tLedType=%d, m_nLedCount=%d, m_nUniverses=%d, m_nPortIndexLast=%d", (int)m_tLedType, (int)m_nLedCount, (int)m_nUniverses, (int) m_nPortIdLast);
}

void WS28xxDmxMulti::Print(void) {
	printf("Led parameters\n");
	printf(" Type    : %s [%d]\n", WS28xxDmxParams::GetLedTypeString((TWS28XXType)m_tLedType), m_tLedType);
	printf(" Count   : %d\n", (int) m_nLedCount);
	printf(" Outputs : %d\n", (int) m_nActiveOutputs);
	printf(" SI5351A : %c\n", m_bUseSI5351A ? 'Y' : 'N');
}
