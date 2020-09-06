/**
 * @file remoteconfigconst.cpp
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

#include <stdio.h>

#include "remoteconfigconst.h"

alignas(uint32_t) const char RemoteConfigConst::PARAMS_FILE_NAME[] = "rconfig.txt";
alignas(uint32_t) const char RemoteConfigConst::PARAMS_DISABLE[] = "disable";
alignas(uint32_t) const char RemoteConfigConst::PARAMS_DISABLE_WRITE[] = "disable_write";
alignas(uint32_t) const char RemoteConfigConst::PARAMS_ENABLE_REBOOT[] = "enable_reboot";
alignas(uint32_t) const char RemoteConfigConst::PARAMS_ENABLE_UPTIME[] = "enable_uptime";
alignas(uint32_t) const char RemoteConfigConst::PARAMS_DISPLAY_NAME[] = "display_name";
