/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "StopWatch.h"

#include <cassert>
#include <ctime>


#define USEC_PER_SEC  1000000ULL
#define USEC_PER_MSEC 1000ULL
#define NSEC_PER_USEC 1000ULL


uint64_t StopWatch::getTotalElapsedMilliseconds() const {
	usec_t elapsed = totalElapsed_;

	if (started_)
		elapsed += getElapsed();

	return elapsed / USEC_PER_MSEC;
}

StopWatch::usec_t StopWatch::now() {
	struct timespec ts;

	assert(!clock_gettime(CLOCK_MONOTONIC, &ts));

	return (usec_t) ts.tv_sec * USEC_PER_SEC + (usec_t) ts.tv_nsec / NSEC_PER_USEC;
}

