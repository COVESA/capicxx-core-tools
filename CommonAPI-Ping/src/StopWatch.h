/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef STOP_WATCH_H_
#define STOP_WATCH_H_

#include <cstdint>


class StopWatch {
public:
	typedef uint64_t usec_t;

	StopWatch():
		started_(false),
		totalElapsed_(0),
		startTimePoint_(0) {
	}

	inline void reset() {
		started_ = false;
		totalElapsed_ = 0;
	}

	inline void start() {
		startTimePoint_ = now();
		started_ = true;
	}

	inline void stop() {
		totalElapsed_ += getElapsed();
		started_ = false;
	}

	uint64_t getTotalElapsedMilliseconds() const;

	inline uint64_t getTotalElapsedSeconds() const {
		return getTotalElapsedMilliseconds() / 1000;
	}

private:
	inline usec_t getElapsed() const {
		return now() - startTimePoint_;
	}

	static usec_t now();

	bool started_;
	usec_t startTimePoint_;
	usec_t totalElapsed_;
};

#endif // STOP_WATCH_H_
