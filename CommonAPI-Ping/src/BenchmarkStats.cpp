/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <iostream>
#include <iomanip>

#include "BenchmarkStats.h"


BenchmarkStats::BenchmarkStats(const char* envPrefix, unsigned long sendCountMax, bool verbose) :
	envPrefix_(envPrefix),
	sendCountMax_(sendCountMax),
	verbose_(verbose),
	started_(false),
	sendCount_(0),
	replyCount_(0) {
}

BenchmarkStats::~BenchmarkStats() {
	stopStatsThread();
}

void BenchmarkStats::reset() {
	stopStatsThread();

	sendCount_ = 0;
	replyCount_ = 0;

	totalStopWatch_.reset();
	creationStopWatch_.reset();
	transportStopWatch_.reset();

	if (verbose_)
		statsThread_ = std::thread(&BenchmarkStats::showProgress, this);

	totalStopWatch_.start();
	started_ = true;
}

void BenchmarkStats::stop() {
  const unsigned long totalCount = sendCount_ + replyCount_;

  stopStatsThread();

  totalStopWatch_.stop();

  std::cout << envPrefix_ << "_SENT=" << sendCount_ << ";\n"
            << envPrefix_ << "_RECEIVED=" << replyCount_ << ";\n"
            << envPrefix_ << "_TOTAL=" << totalCount << ";\n"
            << envPrefix_ << "_TIME=" << totalStopWatch_.getTotalElapsedMilliseconds() << ";\n"
            << envPrefix_ << "_TIME_SEC=" << totalStopWatch_.getTotalElapsedSeconds() << ";\n"
            << envPrefix_ << "_MSGS_PER_SEC=" << getMessagesPerSecondCount() << ";\n"
            << envPrefix_ << "_CREATION_TIME=" << creationStopWatch_.getTotalElapsedMilliseconds() << ";\n"
            << envPrefix_ << "_TRANSPORT_TIME=" << transportStopWatch_.getTotalElapsedMilliseconds() << ";\n";
  std::cout.flush();

	if (verbose_)
		std::cerr << std::left << std::setw(10) << "sent" << " "
				  << std::setw(10) << "received" << " " << std::setw(10)
				  << "totalCount" << " " << std::setw(12) << "time (sec)" << " "
				  << std::setw(13) << "time (usec)" << " " << std::setw(16)
				  << "msgs/sec (totalCount)" << " " << std::setw(15) << "creation time"
				  << " transport time" << std::endl << std::setw(10)
				  << sendCount_ << " " << std::setw(10) << replyCount_ << " "
				  << std::setw(10) << totalCount << " " << std::setw(12)
				  << totalStopWatch_.getTotalElapsedSeconds() << " " << std::setw(13)
				  << totalStopWatch_.getTotalElapsedMilliseconds() << " " << std::setw(16)
				  << getMessagesPerSecondCount() << " " << std::setw(15)
				  << creationStopWatch_.getTotalElapsedMilliseconds() << " "
				  << transportStopWatch_.getTotalElapsedMilliseconds() << std::endl;

}

unsigned long BenchmarkStats::getMessagesPerSecondCount() const {
  const int64_t totalElapsedSeconds = totalStopWatch_.getTotalElapsedSeconds();
  const long totalMessageCount = sendCount_ + replyCount_;

  if (!totalElapsedSeconds)
    return totalMessageCount;

  return totalMessageCount / totalElapsedSeconds;
}

void BenchmarkStats::stopStatsThread() {
	if (started_ && verbose_) {
		started_ = false;
		statsThread_.join();
	}
}

void BenchmarkStats::showProgress() const {
	const std::chrono::milliseconds updateRate(500);
	int count = 0;

	while (started_) {
		if (count++ >= 10) {
			std::cerr << "Sent " << sendCount_ << " message"
					<< (sendCount_ != 1 ? "s" : "") << " in "
					<< totalStopWatch_.getTotalElapsedSeconds() << " seconds" << " ("
					<< getMessagesPerSecondCount() << " msgs/sec" << ", "
					<< ((100 * sendCount_) / sendCountMax_) << "% done)\n";
			count = 0;
		}

		std::this_thread::sleep_for(updateRate);
	}
}
