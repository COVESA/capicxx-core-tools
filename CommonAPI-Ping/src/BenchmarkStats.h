/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BENCHMARKING_STATS_H_
#define BENCHMARKING_STATS_H_

#include <thread>
#include <memory>

#include "StopWatch.h"


class BenchmarkStats {
 public:
	BenchmarkStats(const char* envPrefix, unsigned long sendCountMax, bool verbose);

	~BenchmarkStats();

	void reset();
	void stop();

	inline void startCreation() {
		creationStopWatch_.start();
	}

	void stopCreation() {
		creationStopWatch_.stop();
	}

	void startTransport() {
		transportStopWatch_.start();
	}
	void stopTransport() {
		transportStopWatch_.stop();
	}

	inline void addSendReplyDelta(const long& sendDelta = 1, const long& replyDelta = 1) {
		sendCount_ += sendDelta;
		replyCount_ += replyDelta;
	}

	inline unsigned long getSendCount() const {
		return sendCount_;
	}

	unsigned long getMessagesPerSecondCount() const;

private:
	void stopStatsThread();
	void showProgress() const;

	const char* envPrefix_;
	const long sendCountMax_;
	bool verbose_;

	volatile bool started_;
	std::thread statsThread_;

	unsigned long sendCount_;
	unsigned long replyCount_;

	StopWatch totalStopWatch_;
	StopWatch creationStopWatch_;
	StopWatch transportStopWatch_;
};

#endif // BENCHMARKING_STATS_H_
