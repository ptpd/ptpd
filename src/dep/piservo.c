/* Copyright (c) 2015 Wojciech Owczarek,
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
 * @file   piservo.c
 * @date   Sat Jan 9 16:14:10 2015
 *
 * @brief  (new) PI servo implementation
 *
 */

#include "../ptpd.h"
#include "piservo.h"

static double feed (PIservo*, Integer32, double);
static void prime (PIservo *, double);
static void reset (PIservo*);


void
setupPIservo(PIservo *self) {
    self->feed = feed;
    self->prime = prime;
    self->reset = reset;
}

static double
feed (PIservo* self, Integer32 input, double tau) {

    TimeInternal now, delta;

    Boolean runningMaxOutput;

    self->input = input;

    switch(self->tauMethod) {
	case DT_MEASURED:
	    getSystemClock()->getTimeMonotonic(getSystemClock(), &now);
	    if(isTimeZero(&self->lastUpdate)) {
		self->tau = 1.0;
	    } else {
		subTime(&delta, &now, &self->lastUpdate);
		self->tau = delta.seconds + delta.nanoseconds / 1E9;
	    }
	case DT_CONSTANT:
	    self->tau = tau;
	break;

	default:
	    self->tau = 1.0;
    }

    CLAMP(self->integral, self->maxOutput);
    CLAMP(self->output, self->maxOutput);

    if (self->kP < 0.000001)
	    self->kP = 0.000001;
    if (self->kI < 0.000001)
	    self->kI = 0.000001;

    self->integral += self->tau * ((input + 0.0 ) * self->kI);
    self->output = (self->kP * (input + 0.0) ) + self->integral;

    DBGV("Servo dt: %.09f, input (ofm): %d, output(adj): %.09f, accumulator (observed drift): %.09f\n", dt, input, self->output, self->integral);

    runningMaxOutput = (fabs(self->output) >= self->maxOutput);

    if(runningMaxOutput && !self->runningMaxOutput) {
	    WARNING("Clock %s servo now running at maximum output\n", self->controller->name);
    }

    self->runningMaxOutput = runningMaxOutput;

    if(self->tauMethod == DT_MEASURED) {
	self->lastUpdate = now;
    }

    return self->output;

}

static void
prime (PIservo *self, double integral) {

    Boolean runningMaxOutput;

    CLAMP(integral, self->maxOutput);

    self->integral = integral;
    self->output = integral;

    runningMaxOutput = (fabs(self->output) >= self->maxOutput);

    NOTICE("Clock %s servo primed with frequency %.03f ppb\n", self->controller->name, integral);

    if(runningMaxOutput && !self->runningMaxOutput) {
	    WARNING("Clock %s servo now running at maximum output\n", self->controller->name);
    }

    self->runningMaxOutput = runningMaxOutput;

}

static void
reset (PIservo* self) {

    self->input = 0;
    self->output = 0;
    self->integral = 0;

}




