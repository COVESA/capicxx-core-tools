/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_TESTS_BENCHMARKING_STATS_H_
#define COMMONAPI_TESTS_BENCHMARKING_STATS_H_

#include <CommonAPI/tests/PingProxy.h>

#include "BenchmarkStats.h"

#include <future>


namespace CommonAPI {
namespace tests {

class Benchmark {
 public:
	enum SendType:int {
		SEND_TYPE_EMPTY = 0,
		SEND_TYPE_COPY,
		SEND_TYPE_COPIES,
	};

	Benchmark(SendType sendType, unsigned long sendCount, unsigned long arraySize, bool verbose, bool async);

	bool run();

 private:
	Ping::TestData createTestData();
	Ping::TestDataArray createTestDataArray();

	CommonAPI::CallStatus doEmptySendBenchmark();
	CommonAPI::CallStatus doCopySendBenchmark();
	CommonAPI::CallStatus doCopiesSendBenchmark();

	CommonAPI::CallStatus doEmptyAsyncSendBenchmark();
	void emptyAsyncSendBenchmarkCallback(const CommonAPI::CallStatus&);

	CommonAPI::CallStatus doCopyAsyncSendBenchmark();
	void copyAsyncSendBenchmarkCallback(const CommonAPI::CallStatus&, const Ping::TestData&);

	CommonAPI::CallStatus doCopiesAsyncSendBenchmark();
	void copiesAsyncSendBenchmarkCallback(const CommonAPI::CallStatus&, const Ping::TestDataArray&);


	SendType sendType_;
	unsigned long sendCount_;
	unsigned long arraySize_;
	bool verbose_;
	bool async_;

	BenchmarkStats benchmarkStats_;
	std::shared_ptr<CommonAPI::tests::PingProxy<> > pingProxy_;

	PingProxyBase::GetEmptyResponseAsyncCallback getEmptyResponseAsyncCallback_;
	PingProxyBase::GetTestDataCopyAsyncCallback getTestDataCopyAsyncCallback_;
	PingProxyBase::GetTestDataArrayCopyAsyncCallback getTestDataArrayCopyAsyncCallback_;

	std::promise<CommonAPI::CallStatus> asyncPromise_;
};

} // namespace tests
} // namespace CommonAPI

#endif // COMMONAPI_TESTS_BENCHMARKING_STATS_H_
