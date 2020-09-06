/**
 * @file ltcgenerator.h
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

#ifndef H3_LTCGENERATOR_H_
#define H3_LTCGENERATOR_H_

#include <stdint.h>

#include "ltc.h"

#include "artnetnode.h"

class LtcGenerator {
public:
	LtcGenerator(ArtNetNode* pNode, const struct TLtcTimeCode *pStartLtcTimeCode, const struct TLtcTimeCode *pStopLtcTimeCode, struct TLtcDisabledOutputs *pLtcDisabledOutputs);
	~LtcGenerator(void);

	void Start(void);
	void Stop(void);

	void Run(void);

private:
	void HandleAction(void);

	void ActionStart(void);
	void ActionStop(void);
	void ActionResume(void);

	void Update(void);
	void Increment(void);

private:
	struct TLtcTimeCode *m_pStartLtcTimeCode;
	struct TLtcTimeCode *m_pStopLtcTimeCode;
	uint8_t m_nFps;
	char m_aTimeCode[TC_CODE_MAX_LENGTH];
	uint32_t m_nTimer0Interval;
	int m_nHandle;
	uint8_t m_Buffer[64];
	int m_nBytesReceived;
	bool m_bIncrement;
};

#endif /* H3_LTCGENERATOR_H_ */
