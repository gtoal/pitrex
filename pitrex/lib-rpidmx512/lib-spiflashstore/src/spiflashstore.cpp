/**
 * @file spiflashstore.cpp
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

#include <stdint.h>
#include <stdio.h>
#include <uuid/uuid.h>
#include <assert.h>

#include "spiflashstore.h"

#include "spi_flash.h"

#include "debug.h"

static const uint8_t s_aSignature[] = {'A', 'v', 'V', 0x10};

#define OFFSET_STORES	((((sizeof(s_aSignature) + 15) / 16) * 16) + 16) // +16 is reserved for UUID

static const uint32_t s_aStorSize[STORE_LAST]  = {96,        144,       32,    64,       96,      32,    64,     32,         480,           64,         32,        96,           48,        32,      768};
#ifndef NDEBUG
static const char s_aStoreName[STORE_LAST][12] = {"Network", "Art-Net3", "DMX", "WS28xx", "E1.31", "LTC", "MIDI", "Art-Net4", "OSC Server", "TLC59711", "USB Pro", "RDM Device", "RConfig", "TCNet", "OSC Client"};
#endif

SpiFlashStore *SpiFlashStore::s_pThis = 0;

SpiFlashStore::SpiFlashStore(void): m_bHaveFlashChip(false), m_bIsNew(false), m_nStartAddress(0), m_nSpiFlashStoreSize(SPI_FLASH_STORE_SIZE), m_tState(STORE_STATE_IDLE) {
	DEBUG_ENTRY

	s_pThis = this;

	if (spi_flash_probe(0, 0, 0) < 0) {
		DEBUG_PUTS("No SPI flash chip");
	} else {
		printf("Detected %s with sector size %d total %d bytes\n", spi_flash_get_name(), spi_flash_get_sector_size(), spi_flash_get_size());
		m_bHaveFlashChip = Init();
	}

	if (m_bHaveFlashChip) {
		m_nSpiFlashStoreSize = OFFSET_STORES;

		for (uint32_t j = 0; j < STORE_LAST; j++) {
			m_nSpiFlashStoreSize += s_aStorSize[j];
		}

		DEBUG_PRINTF("OFFSET_STORES=%d", (int) OFFSET_STORES);
		DEBUG_PRINTF("m_nSpiFlashStoreSize=%d", m_nSpiFlashStoreSize);

		Dump();
	}

	DEBUG_EXIT
}

SpiFlashStore::~SpiFlashStore(void) {
	DEBUG_ENTRY

	while (Flash())
		;

	DEBUG_EXIT
}

bool SpiFlashStore::Init(void) {
	const uint32_t nEraseSize = spi_flash_get_sector_size();
	assert(SPI_FLASH_STORE_SIZE == nEraseSize);

	if (SPI_FLASH_STORE_SIZE != nEraseSize) {
		return false;
	}

	m_nStartAddress = spi_flash_get_size() - nEraseSize;
	assert(!(m_nStartAddress % nEraseSize));

	if (m_nStartAddress % nEraseSize) {
		return false;
	}

	spi_flash_cmd_read_fast(m_nStartAddress, (size_t) SPI_FLASH_STORE_SIZE, (void *) &m_aSpiFlashData);

	bool bSignatureOK = true;

	for (uint32_t i = 0; i < sizeof(s_aSignature); i++) {
		if (s_aSignature[i] != m_aSpiFlashData[i]) {
			m_aSpiFlashData[i] = s_aSignature[i];
			bSignatureOK = false;
		}
	}

	if (__builtin_expect(!bSignatureOK, 0)) {
		DEBUG_PUTS("No signature");

		m_bIsNew = true;

		// Clear bSetList
		for (uint32_t j = 0; j < STORE_LAST; j++) {
			const uint32_t nOffset = GetStoreOffset((enum TStore) j);
			uint32_t k = nOffset;
			m_aSpiFlashData[k++] = 0x00;
			m_aSpiFlashData[k++] = 0x00;
			m_aSpiFlashData[k++] = 0x00;
			m_aSpiFlashData[k++] = 0x00;

			// Clear rest of data
			for (; k < nOffset + s_aStorSize[j]; k++) {
				m_aSpiFlashData[k] = 0xFF;
			}
		}

		m_tState = STORE_STATE_CHANGED;

		return true;
	}

	for (uint32_t j = STORE_E131; j < STORE_LAST; j++) {
		uint8_t *pbSetList = &m_aSpiFlashData[GetStoreOffset((enum TStore) j)];
		if ((pbSetList[0] == 0xFF) && (pbSetList[1] == 0xFF) && (pbSetList[2] == 0xFF) && (pbSetList[3] == 0xFF)) {
			DEBUG_PRINTF("[%s]: bSetList \'FF...FF\'", s_aStoreName[j]);
			// Clear bSetList
			*pbSetList++ = 0x00;
			*pbSetList++ = 0x00;
			*pbSetList++ = 0x00;
			*pbSetList = 0x00;

			m_tState = STORE_STATE_CHANGED;
		}
	}

	return true;
}

uint32_t SpiFlashStore::GetStoreOffset(enum TStore tStore) {
	assert(tStore < STORE_LAST);

	uint32_t nOffset = OFFSET_STORES;

	for (uint32_t i = 0; i < tStore; i++) {
		nOffset += s_aStorSize[i];
	}

	DEBUG_PRINTF("nOffset=%d", nOffset);

	return nOffset;
}

void SpiFlashStore::Update(enum TStore tStore, uint32_t nOffset, void* pData, uint32_t nDataLength, uint32_t bSetList) {
	DEBUG1_ENTRY

	if (__builtin_expect((!m_bHaveFlashChip),0)) {
		return;
	}

	DEBUG_PRINTF("[%s]:%d:%p[%d]:%d-%d", s_aStoreName[tStore], tStore, pData, nOffset, nDataLength, m_tState);

	assert(tStore < STORE_LAST);
	assert(pData != 0);
	assert((nOffset + nDataLength) <= s_aStorSize[tStore]);

	debug_dump(pData, nDataLength);

	bool bIsChanged = false;

	const uint32_t nBase = nOffset + GetStoreOffset(tStore);

	const uint8_t *src = (uint8_t *) pData;
	uint8_t *dst = (uint8_t *) &m_aSpiFlashData[nBase];

	for (uint32_t i = 0; i < nDataLength; i++) {
		if (*src != *dst) {
			bIsChanged = true;
			*dst = *src;
		}
		dst++;
		src++;
	}

	if (bIsChanged && (m_tState != STORE_STATE_ERASED)) {
		m_tState = STORE_STATE_CHANGED;
	}

	if ((0 != nOffset) && (bIsChanged)) {
		assert(bSetList != 0);

		uint32_t *p = (uint32_t *) &m_aSpiFlashData[GetStoreOffset(tStore)];
		*p |= bSetList;
	}

	DEBUG_PRINTF("m_tState=%d", m_tState);

	DEBUG1_EXIT
}

void SpiFlashStore::Copy(enum TStore tStore, void* pData, uint32_t nDataLength) {
	DEBUG1_ENTRY

	if (__builtin_expect((!m_bHaveFlashChip), 0)) {
		DEBUG1_EXIT
		return;
	}

	assert(tStore < STORE_LAST);
	assert(pData != 0);
	assert(nDataLength <= s_aStorSize[tStore]);

	const uint32_t *pSet = (uint32_t *) &m_aSpiFlashData[GetStoreOffset(tStore)];

	DEBUG_PRINTF("*pSet=%d", (int) *pSet);

	if ((__builtin_expect((m_bIsNew), 0)) || (__builtin_expect((*pSet == 0), 0))) {
		Update(tStore, pData, nDataLength);
		DEBUG1_EXIT
		return;
	}

	const uint8_t *src = (uint8_t *) &m_aSpiFlashData[GetStoreOffset(tStore)];
	uint8_t *dst = (uint8_t *) pData;

	for (uint32_t i = 0; i < nDataLength; i++) {
		*dst++ = *src++;
	}

	DEBUG1_EXIT
}

void SpiFlashStore::CopyTo(enum TStore tStore, void* pData, uint32_t& nDataLength) {
	DEBUG1_ENTRY

	if (__builtin_expect(((unsigned) tStore >= (unsigned) STORE_LAST), 0)) {
		nDataLength = 0;
		return;
	}

	nDataLength = s_aStorSize[tStore];

	const uint8_t *src = (uint8_t *) &m_aSpiFlashData[GetStoreOffset(tStore)];
	uint8_t *dst = (uint8_t *) pData;

	for (uint32_t i = 0; i < nDataLength; i++) {
		*dst++ = *src++;
	}

	DEBUG1_EXIT
}

void SpiFlashStore::Dump(void) {
#ifndef NDEBUG
	if (__builtin_expect((!m_bHaveFlashChip), 0)) {
		return;
	}

	debug_dump(m_aSpiFlashData, OFFSET_STORES);
	printf("\n");

	for (uint32_t j = 0; j < STORE_LAST; j++) {
		printf("Store [%s]:%d\n", s_aStoreName[j], j);

		uint8_t *p = (uint8_t *) (&m_aSpiFlashData[GetStoreOffset((enum TStore) j)]);
		debug_dump(p, s_aStorSize[j]);

		printf("\n");
	}

	printf("m_tState=%d\n", m_tState);
#endif
}

bool SpiFlashStore::Flash(void) {
	if (__builtin_expect((m_tState == STORE_STATE_IDLE), 1)) {
		return false;
	}

	DEBUG_PRINTF("m_tState=%d", m_tState);

	assert(m_nStartAddress != 0);

	if (m_nStartAddress == 0) {
		printf("!*! m_nStartAddress == 0 !*!\n");

		return false;
	}

	switch (m_tState) {
		case STORE_STATE_CHANGED:
			spi_flash_cmd_erase(m_nStartAddress, (size_t) SPI_FLASH_STORE_SIZE);
			m_tState = STORE_STATE_ERASED;
			return true;
			break;
		case STORE_STATE_ERASED:
			spi_flash_cmd_write_multi(m_nStartAddress, (size_t) m_nSpiFlashStoreSize, (const void *)&m_aSpiFlashData);
			m_tState = STORE_STATE_IDLE;
			break;
		default:
			break;
	}

#ifndef NDEBUG
	Dump();
#endif

	return false;
}

StoreNetwork *SpiFlashStore::GetStoreNetwork(void) {
	return &m_StoreNetwork;
}

StoreArtNet *SpiFlashStore::GetStoreArtNet(void) {
	return &m_StoreArtNet;
}

StoreDmxSend *SpiFlashStore::GetStoreDmxSend(void) {
	return &m_StoreDmxSend;
}

StoreWS28xxDmx *SpiFlashStore::GetStoreWS28xxDmx(void) {
	return &m_StoreWS28xxDmx;
}

StoreE131 *SpiFlashStore::GetStoreE131(void) {
	return &m_StoreE131;
}

StoreArtNet4 *SpiFlashStore::GetStoreArtNet4(void) {
	return &m_StoreArtNet4;
}

StoreTLC59711* SpiFlashStore::GetStoreTLC59711(void) {
	return &m_StoreTLC59711;
}
