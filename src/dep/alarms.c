/*-
 * Copyright (c) 2015 Wojciech Owczarek
 *
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    alarms.c
 * @authors Wojciech Owczarek
 * @date   Wed Dec 9 19:13:10 2015
 * This souece file contains the implementations of functions
 * handling raising and clearing alarms.
 */

#include "../ptpd.h"

AlarmEntry * getAlarms() {

    static AlarmEntry alarms[] = {

    { "STATE", "PORT_STATE", "Port state different to expected value", { ALRM_PORT_STATE }},
    { "OFS", "OFM_THRESHOLD", "Offset From Master outside threshold", { ALRM_OFM_THRESHOLD }},
    { "OFSL", "OFM_SECONDS", "Offset From Master above 1 second", { ALRM_OFM_SECONDS }},
    { "STEP", "CLOCK_STEP", "Clock was stepped", { ALRM_CLOCK_STEP}}

    };

    return alarms;

}