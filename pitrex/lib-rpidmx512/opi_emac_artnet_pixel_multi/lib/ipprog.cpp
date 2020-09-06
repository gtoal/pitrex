/**
 * @file ipprog.cpp
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "artnetipprog.h"
#include "artnet.h"

#include "ipprog.h"

#include "network.h"

#include "display.h"

#if defined(ORANGE_PI)
 #include "spiflashstore.h"
#endif

union uip {
	uint32_t u32;
	uint8_t u8[4];
} static ip_union;

IpProg::IpProg(void) {
#if defined(ORANGE_PI)
	assert(SpiFlashStore::Get() != 0);
#endif
}

IpProg::~IpProg(void) {

}

void IpProg::Handler(const struct TArtNetIpProg *pArtNetIpProg, struct TArtNetIpProgReply *pArtNetIpProgReply) {
	pArtNetIpProgReply->ProgPortHi = (uint8_t)(ARTNET_UDP_PORT >> 8);
	pArtNetIpProgReply->ProgPortLo = ARTNET_UDP_PORT & 0xFF;

#ifndef NDEBUG
	printf("IpProg::Handler, Command = %d\n", pArtNetIpProg->Command);
	printf("\tIP : " IPSTR "\n", IP2STR(Network::Get()->GetIp()));
	printf("\tNetmask : " IPSTR "\n", IP2STR(Network::Get()->GetNetmask()));
#endif

	if (pArtNetIpProg->Command == 0) {
		ip_union.u32 = Network::Get()->GetIp();
		memcpy((void *) &pArtNetIpProgReply->ProgIpHi, (void *) ip_union.u8, ARTNET_IP_SIZE);
		ip_union.u32 = Network::Get()->GetNetmask();
		memcpy((void *) &pArtNetIpProgReply->ProgSmHi, (void *) ip_union.u8, ARTNET_IP_SIZE);
	} else if ((pArtNetIpProg->Command & IPPROG_COMMAND_PROGRAM_IPADDRESS) == IPPROG_COMMAND_PROGRAM_IPADDRESS) {
		// Get IPAddress from IpProg
		memcpy((void *) ip_union.u8, (void *) &pArtNetIpProg->ProgIpHi, ARTNET_IP_SIZE);

		Network::Get()->SetIp(ip_union.u32);
#if defined(ORANGE_PI)
		SpiFlashStore::Get()->GetStoreNetwork()->UpdateIp(ip_union.u32);
#endif
		Display::Get()->Printf(3, "IP: " IPSTR " S", IP2STR(ip_union.u32));

#ifndef NDEBUG
		printf("\tIP : " IPSTR "\n", IP2STR(Network::Get()->GetIp()));
		printf("\tNetmask : " IPSTR "\n", IP2STR(Network::Get()->GetNetmask()));
#endif
		// Set IPAddress in IpProgReply
		memcpy(&pArtNetIpProgReply->ProgIpHi, &pArtNetIpProg->ProgIpHi, ARTNET_IP_SIZE);
		pArtNetIpProgReply->Status = 0; // DHCP Disabled;
	}
}

