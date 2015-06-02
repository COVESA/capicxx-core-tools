/* Copyright (C) 2014 BMW Group
 * Copyright (C) 2015 Mentor Graphics
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * Author: Felix Scherzinger (felix_scherzinger@mentor.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Performance_Complex
*/

#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"

#include "v1_0/commonapi/performance/complex/TestInterfaceProxy.hpp"
#include "stub/PFComplexStub.h"

#include "utils/StopWatch.h"

const int usecPerSecond = 1000000;

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.performance.complex.TestInterface";
const int tasync = 100000;

// Define the max. array size to test
const int maxArraySize = 4096 / 16;
// Define the loop count how often the commonAPI test function is called for calculating the mean time
const int loopCountPerPaylod = 1000;

using namespace v1_0::commonapi::performance::complex;

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class PFComplex: public ::testing::Test {
public:
	 void recvArray(const CommonAPI::CallStatus& callStatus, TestInterface::tArray y) {
		 // Stop the time & verify call status
		 watch_.stop();
		 EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

		 std::unique_lock<std::mutex> uniqueLock(synchLock_);
		 condVar_.notify_one();
	 }

protected:
	void SetUp() {
		runtime_ = CommonAPI::Runtime::get();
		ASSERT_TRUE((bool)runtime_);

		testStub_ = std::make_shared<PFComplexStub>();
		serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, "service-sample");
		ASSERT_TRUE(serviceRegistered_);

		testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, "client-sample");
		ASSERT_TRUE((bool)testProxy_);

        testProxy_->isAvailableBlocking();
        ASSERT_TRUE(testProxy_->isAvailable());
	}

	void TearDown() {
        runtime_->unregisterService(domain, PFComplexStub::StubInterface::getInterface(), testAddress);
    }

    void printTestValues(size_t payloadSize, size_t objectSize) {
		// Get elapsed time, calculate mean time and print out!
		StopWatch::usec_t methodCallTime = watch_.getTotalElapsedMicroseconds();
		StopWatch::usec_t meanTime = (methodCallTime / loopCountPerPaylod);
		StopWatch::usec_t perByteTime = meanTime / objectSize;
		uint32_t callsPerSeconds = usecPerSecond / (methodCallTime / loopCountPerPaylod);

		std::cout << "[MEASURING ]  Payload-Size=" << std::setw(7) << std::setfill('.') << payloadSize
				  << ", Mean-Time=" << std::setw(7) << std::setfill('.') << meanTime
				  << "us, per-Byte(payload)=" << std::setw(7) << std::setfill('.')
				  << (perByteTime <= 0 ? ".....<1" : std::to_string(perByteTime)) << "us"
				  << ", calls/s=" << std::setw(7) << std::setfill('.') << callsPerSeconds
				  << std::endl;
    }

    std::string configFileName_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<TestInterfaceStubDefault> testStub_;

    StopWatch watch_;
    std::mutex synchLock_;
    std::condition_variable condVar_;
    uint32_t callCount_;
    std::function<void (const CommonAPI::CallStatus&, TestInterface::tArray)> myCallback_;
    uint32_t arraySize_ = 1;
};

/**
* @test Test synchronous ping pong function call
*   - complex array is array of a struct containing an union and another struc with primitive datatypes
* 	- The stub just set the in array to the out array
* 	- CallStatus and array content will be used to verify the sync call has succeeded
* 	- Using double payload every cycle, starting with 1 end with maxPrimitiveArraySize
* 	- Doing primitiveLoopSize loops to build the mean time
*/
TEST_F(PFComplex, Ping_Pong_Complex_Synchronous) {
	CommonAPI::CallStatus callStatus;

	watch_.reset();

	// Loop until maxPrimitiveArraySize
	while (arraySize_ <= maxArraySize) {

		// Create in-array with actual arraySize
		TestInterface::tArray in;
		TestInterface::innerStruct innerTestStruct(123, true, 4, "test", 35);
		std::string unionMember = std::string("Hello World");
		TestInterface::innerUnion innerTestUnion = unionMember;
		TestInterface::tStruct testStruct(innerTestStruct, innerTestUnion);
		for (uint32_t i = 0; i < arraySize_; ++i) {
			in.push_back(testStruct);
		}

		// Sum up payload size of primitive memebers
		size_t payloadSize = sizeof(innerTestStruct.getBooleanMember()) + sizeof(innerTestStruct.getUint8Member())
				+ sizeof(innerTestStruct.getUint16Member()) + sizeof(innerTestStruct.getUint32Member())
				+ sizeof(innerTestStruct.getStringMember()) + sizeof(unionMember);

		// Call commonAPI method loopCountPerPaylod times to calculate mean time
		for (uint32_t i = 0; i < loopCountPerPaylod; ++i) {

			// Create an empty out-array for every commonAPI function call
			TestInterface::tArray out;

			// Call commonAPI function and measure time
			watch_.start();
			testProxy_->testMethod(in, callStatus, out);
			watch_.stop();

			// Check the call was successful & out array has same elements than in array
			EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
			EXPECT_EQ(in, out);
		}

		// Printing results
		printTestValues(arraySize_ * payloadSize, arraySize_ * sizeof(testStruct));

		// Increase array size for next iteration
		arraySize_ *= 2;

		// Reset StopWatch for next iteration
		watch_.reset();
	}
}

/**
* @test Test asynchronous ping pong function call
*   - complex array is array of a struct containing an union and another struc with primitive datatypes
* 	- The stub just set (copies) the in array to the out array
* 	- Only the CallStatus will be used to verify the async call has succeeded
* 	- Using double payload every cycle, starting with 1 end with maxPrimitiveArraySize
* 	- Doing loopCountPerPaylod loops to calc the mean time
*/
TEST_F(PFComplex, Ping_Pong_Complex_Asynchronous) {
    myCallback_ = std::bind(&PFComplex::recvArray, this, std::placeholders::_1, std::placeholders::_2);

    std::unique_lock<std::mutex> uniqueLock(synchLock_);

    watch_.reset();

    // Loop until maxPrimitiveArraySize
	while (arraySize_ <= maxArraySize) {

		// Create in-array with actual arraySize
		TestInterface::tArray in;
		TestInterface::innerStruct innerTestStruct(123, true, 4, "test", 35);
		std::string unionMember = std::string("Hello World");
		TestInterface::innerUnion innerTestUnion = unionMember;
		TestInterface::tStruct testStruct(innerTestStruct, innerTestUnion);
		for (uint32_t i = 0; i < arraySize_; ++i) {
			in.push_back(testStruct);
		}

		// Sum up payload size of primitive memebers
		size_t payloadSize = sizeof(innerTestStruct.getBooleanMember()) + sizeof(innerTestStruct.getUint8Member())
				+ sizeof(innerTestStruct.getUint16Member()) + sizeof(innerTestStruct.getUint32Member())
				+ sizeof(innerTestStruct.getStringMember()) + sizeof(unionMember);

		// Call commonAPI method loopCountPerPaylod times to calculate mean time
		for (uint32_t i = 0; i < loopCountPerPaylod; ++i) {
			TestInterface::tArray out;
			watch_.start();
			testProxy_->testMethodAsync(in, myCallback_);
			condVar_.wait(uniqueLock);
		}

		// Printing results
		printTestValues(arraySize_ * payloadSize, arraySize_ * sizeof(testStruct));

		// Increase array size for next iteration
		arraySize_ *= 2;

		// Reset StopWatch for next iteration
		watch_.reset();
	}
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}

