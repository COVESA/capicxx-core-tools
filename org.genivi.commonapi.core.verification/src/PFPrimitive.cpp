/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Performance_Primitive
*/

#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"

#include "v1/commonapi/performance/primitive/TestInterfaceProxy.hpp"
#include "stub/PFPrimitiveStub.h"

#include "utils/StopWatch.h"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.performance.primitive.TestInterface";

const int usecPerSecond = 1000000;

// Define the max. array size to test
const int maxPrimitiveArraySize = 1024*16;
// Define the loop count how often the commonAPI test function is called for calculating the mean time
const int loopCountPerPaylod = 1000;

using namespace v1_0::commonapi::performance::primitive;

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class PFPrimitive: public ::testing::Test {
public:
     void recvArray(const CommonAPI::CallStatus& callStatus, TestInterface::TestArray y) {
         (void)y;
         std::unique_lock<std::mutex> uniqueLock(synchLock_);
         EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
         callCount_++;
         if (callCount_ == loopCountPerPaylod) {
             condVar_.notify_one();
         }
     }

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStub_ = std::make_shared<PFPrimitiveStub>();
        bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
        ASSERT_TRUE((bool)testProxy_);

        int i = 0;
        while(!testProxy_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy_->isAvailable());

        callCount_ = 0;
    }

    void TearDown() {
        bool unregistered = runtime_->unregisterService(domain, PFPrimitiveStub::StubInterface::getInterface(), testAddress);
        ASSERT_TRUE(unregistered);

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 100 ) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    void printTestValues() {
        // Get elapsed time, calculate mean time and print out!
        StopWatch::usec_t methodCallTime = watch_.getTotalElapsedMicroseconds();
        StopWatch::usec_t meanTime = (methodCallTime / loopCountPerPaylod);
        StopWatch::usec_t perByteTime = StopWatch::usec_t(meanTime / arraySize_);
        uint32_t callsPerSeconds = uint32_t(usecPerSecond / (methodCallTime / loopCountPerPaylod));

        std::cout << "[MEASURING ]  Size=" << std::setw(7) << std::setfill('.') << arraySize_
                  << ", Mean-Time=" << std::setw(7) << std::setfill('.') << meanTime << "us"
                  << ", per-Byte=" << std::setw(7) << std::setfill('.')
                  << (perByteTime <= 0 ? ".....<1" : std::to_string(perByteTime)) << "us"
                  << ", calls/s=" << std::setw(7) << std::setfill('.') << callsPerSeconds
                  << std::endl;
    }

    std::string configFileName_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<TestInterfaceStub> testStub_;

    StopWatch watch_;
    std::mutex synchLock_;
    std::condition_variable condVar_;
    uint32_t callCount_;
    std::function<void (const CommonAPI::CallStatus&, TestInterface::TestArray)> myCallback_;
    uint32_t arraySize_ = 1;
};

/**
* @test Test synchronous ping pong function call
*   - primitive array is array of UInt_8
*     - The stub just set the in array to the out array
*     - CallStatus and array content will be used to verify the sync call has succeeded
*     - Using double payload every cycle, starting with 1 end with maxPrimitiveArraySize
*     - Doing primitiveLoopSize loops to build the mean time
*/
TEST_F(PFPrimitive, Ping_Pong_Primitive_Synchronous) {
    CommonAPI::CallStatus callStatus;

    watch_.reset();

    // Loop until maxPrimitiveArraySize
    while (arraySize_ <= maxPrimitiveArraySize) {

        // Create in-array with actual arraySize
        TestInterface::TestArray in(arraySize_);

        // Call commonAPI method loopCountPerPaylod times to calculate mean time
        for (uint32_t i = 0; i < loopCountPerPaylod; ++i) {

            // Create an empty out-array for every commonAPI function call
            TestInterface::TestArray out;

            // Call commonAPI function and measure time
            watch_.start();
            testProxy_->testMethod(in, callStatus, out);
            watch_.stop();

            // Check the call was successful & out array has same elements than in array
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
            EXPECT_EQ(in, out);
        }

        // Printing results
        printTestValues();

        // Increase array size for next iteration
        arraySize_ *= 2;

        // Reset StopWatch for next iteration
        watch_.reset();
    }
}

/**
* @test Test asynchronous ping pong function call
*   - primitive array is array of UInt_8
*     - The stub just set (copies) the in array to the out array
*     - Only the CallStatus will be used to verify the async call has succeeded
*     - Using double payload every cycle, starting with 1 end with maxPrimitiveArraySize
*     - Doing primitiveLoopSize loops to build the mean time
*/
TEST_F(PFPrimitive, Ping_Pong_Primitive_Asynchronous) {
    myCallback_ = std::bind(&PFPrimitive::recvArray, this, std::placeholders::_1, std::placeholders::_2);

    // Loop until maxPrimitiveArraySize
    while (arraySize_ <= maxPrimitiveArraySize) {

        watch_.reset();

        // Initialize testData, call count stop watch for next iteration!
        TestInterface::TestArray in(arraySize_);

#ifdef _WIN32
        // DBus under Windows is way to slow at the moment (about 10 times slower than linux), so without an increase in timeout, this test never succeeds.
        // Only raising for WIN32, since linux should run with the default timeout without problems.
        CommonAPI::CallInfo callInfo(60000);
#endif

        watch_.start();
        for (uint32_t i = 0; i < loopCountPerPaylod; ++i) {
#ifdef _WIN32
            testProxy_->testMethodAsync(in, myCallback_, &callInfo);
#else
            testProxy_->testMethodAsync(in, myCallback_);
#endif
        }
        {
            std::unique_lock<std::mutex> uniqueLock(synchLock_);
            while (callCount_ != loopCountPerPaylod) {
                condVar_.wait(uniqueLock);
            }
            callCount_ = 0;
        }
        watch_.stop();

        // Printing results
        printTestValues();

        // Increase array size for next iteration
        arraySize_ *= 2;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}

