/* Copyright (c) 2016-2017 Wojciech Owczarek,
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
 * @file   ttransport_interface.h
 * @date   Tue Aug 2 34:44 2016
 *
 * @brief  Timestamping Transport API function declarations and macros for inclusion by
 *         timestamping transport imlpementations
 *
 */


/* public interface - implementations must implement all of those */

/* init, shutdown */

static int tTransport_init (TTransport *self, const TTransportConfig *config, CckFdSet *fdSet);
static int tTransport_shutdown (TTransport *self);

/* check if this transport recognises itself as matching the search string (interface, port, etc.) */
static bool isThisMe(TTransport *self, const char* search);
/* test the required configuration */
static bool testConfig(TTransport *self, const TTransportConfig *config);
/* NEW! Now with private healthcare! */
static bool privateHealthCheck(TTransport *self);
/* send message: buffer and destination information; populate TX timestamp if we have one */
static ssize_t sendMessage(TTransport *self, TTransportMessage *message);
/* receive message: buffer and source + destination information; populate RX timestamp if we have one */
static ssize_t receiveMessage(TTransport *self, TTransportMessage *message);
/* get an instnce of the clock driver generating this clock's timestamps, create if not existing */
static ClockDriver *getClockDriver(TTransport *self);
/* perform any periodic checks */
static int monitor(TTransport *self, const int interval, const bool quiet);
/* perform any refresh actions - mcast joins, etc. */
static int refresh(TTransport *self);
/* load any vendor extensions */
static void loadVendorExt(TTransport *self);

/* public interface end */


#define INIT_INTERFACE(var)\
    var->init = tTransport_init;\
    var->shutdown = tTransport_shutdown;\
    var->isThisMe = isThisMe;\
    var->testConfig = testConfig;\
    var->privateHealthCheck = privateHealthCheck;\
    var->sendMessage = sendMessage;\
    var->receiveMessage = receiveMessage;\
    var->getClockDriver = getClockDriver;\
    var->monitor = monitor;\
    var->refresh = refresh;\
    var->loadVendorExt = loadVendorExt;

