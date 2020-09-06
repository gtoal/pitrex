/**
 * @file networkcircle.h
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
#ifndef NETWORKCIRCLE_H_
#define NETWORKCIRCLE_H_

#include <stdint.h>

#include "circle/net/netsubsystem.h"
#include "circle/net/socket.h"

#include "network.h"

#ifndef HOST_NAME_MAX
 #define HOST_NAME_MAX	64
#endif

class NetworkCircle: public Network {
public:
	NetworkCircle(void);
	~NetworkCircle(void);

	void Init(CNetSubSystem *pNet);

	int32_t Begin(uint16_t nPort);
	int32_t End(uint16_t nPort);

	void MacAddressCopyTo(uint8_t *pMacAddress);

	void JoinGroup(uint32_t nHandle, uint32_t nIp) {
		// Not supported
	}
	void LeaveGroup(uint32_t nHandle, uint32_t nIp) {
		// Not supported
	}

	uint16_t RecvFrom(uint32_t nHandle, uint8_t *pPacket, uint16_t nSize, uint32_t *pFromIp, uint16_t *pFromPort);
	void SendTo(uint32_t nHandle, const uint8_t *pPacket, uint16_t nSize, uint32_t nToIp, uint16_t nRemotePort);

	void SetIp(uint32_t nIp) {
		// Not supported
	}

private:
	CNetSubSystem *m_pNet;
	CSocket *m_pSocket;
};

#endif /* NETWORKCIRCLE_H_ */
