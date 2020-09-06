/**
 * @file spiflashinstall.h
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

#ifndef SPIFLASHINSTALL_H_
#define SPIFLASHINSTALL_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

class SpiFlashInstall {
public:
	SpiFlashInstall(void);
	~SpiFlashInstall(void);

	bool WriteFirmware(const uint8_t *pBuffer, uint32_t nSize);

private:
	bool Open(const char *pFileName);
	void Close(void);
	bool BuffesCompare(uint32_t nSize);
	bool Diff(uint32_t nOffset);
	void Write(uint32_t nOffset);
	void Process(const char *pFileName, uint32_t nOffset);

public:
	static SpiFlashInstall* Get(void) {
		return s_pThis;
	}

private:
	static SpiFlashInstall *s_pThis;

private:
	bool m_bHaveFlashChip;
	uint32_t m_nEraseSize;
	uint32_t m_nFlashSize;
	alignas(uint32_t) uint8_t *m_pFileBuffer;
	alignas(uint32_t) uint8_t *m_pFlashBuffer;
	FILE *m_pFile;
};

#endif /* SPIFLASHINSTALL_H_ */
