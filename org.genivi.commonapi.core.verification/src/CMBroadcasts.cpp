/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Communication
*/

#include <functional>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <fstream>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/communication/TestInterfaceProxy.hpp"
#include "stub/CMBroadcastsStub.h"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.communication.TestInterface";
const int tasync = 100000;

using namespace v1_0::commonapi::communication;

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class CMBroadcasts: public ::testing::Test {

public:

    void recvSubscribedValue(uint8_t y) {
        value_ = y;
    }

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);
        std::mutex availabilityMutex;
        std::unique_lock<std::mutex> lock(availabilityMutex);
        std::condition_variable cv;
        bool proxyAvailable = false;

        std::thread t1([this, &proxyAvailable, &cv, &availabilityMutex]() {
            std::lock_guard<std::mutex> lock(availabilityMutex);
            testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
            testProxy_->isAvailableBlocking();
            ASSERT_TRUE((bool)testProxy_);
            proxyAvailable = true;
            cv.notify_one();
        });
        testStub_ = std::make_shared<CMBroadcastsStub>();
        bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        while(!proxyAvailable) {
            cv.wait(lock);
        }
        t1.join();
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        bool serviceUnregistered =
                runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                        testAddress);

        ASSERT_TRUE(serviceUnregistered);

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 10 ) {
            usleep(100000);
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    uint8_t value_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CMBroadcastsStub> testStub_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
};

/**
* @test Test broadcasts. Subscribe to a broadcast, and see that the value
*  is correctly received.
*/
TEST_F(CMBroadcasts, NormalBroadcast) {

    CommonAPI::CallStatus callStatus;
    uint8_t result = 0;

    // subscribe to broadcast
    testProxy_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    });

    // send value '1' via a method call - this tells stub to broadcast
    uint8_t in_ = 1;
    uint8_t out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        usleep(10000);
    }
    EXPECT_EQ(result, 1);
}

/**
* @test Test selective broadcasts.
*  - inform stub to stop accepting subscriptions
*  - try to subscribe to the selective broadcast
*  - check that an error was received
*  - inform stub to send a broadcast
*  - check that nothing was received in a reasonable time
*/
TEST_F(CMBroadcasts, SelectiveBroadcastRejected) {

    CommonAPI::CallStatus callStatus;
    CommonAPI::CallStatus subStatus;
    uint8_t result = 0;
    uint8_t in_ = 0;
    uint8_t out_ = 0;

    // send value '2' via a method call - this tells stub to stop accepting subs
    in_ = 2;
    testProxy_->testMethod(in_, callStatus, out_);

    // subscribe
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });

    // check that subscription failed correctly
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    // The following does not happen in SOME/IP, so it's commented out.
    //EXPECT_EQ(subStatus, CommonAPI::CallStatus::SUBSCRIPTION_REFUSED);

    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);

    // check that no value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        usleep(10000);
    }
    EXPECT_EQ(result, 0);

}

/**
* @test Test selective broadcasts.
*  - inform stub to start accepting subscriptions
*  - subscribe to the selective broadcast
*  - check that no error was received (in a reasonable time)
*  - inform stub to send a broadcast
*  - check that a correct value is received
*/
TEST_F(CMBroadcasts, SelectiveBroadcast) {

    CommonAPI::CallStatus callStatus;
    CommonAPI::CallStatus subStatus;
    uint8_t result = 0;
    uint8_t in_ = 0;
    uint8_t out_ = 0;

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;
    testProxy_->testMethod(in_, callStatus, out_);

    // subscribe
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });

    // check that no error was received
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    EXPECT_EQ(subStatus, CommonAPI::CallStatus::SUCCESS);

    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        usleep(10000);
    }
    EXPECT_EQ(result, 1);
}

/**
* @test Test BroadcastStubGoesOfflineOnlineAgain.
*  - service offline
*  - subscribe to broadcast
*  - service online
*  - fire broadcast -> proxy should receive
*  - service offline
*  - service online
*  - fire again -> proxy should receive again
*/
TEST_F(CMBroadcasts, BroadcastStubGoesOfflineOnlineAgain) {
    CommonAPI::CallStatus callStatus;
    uint8_t result = 0;

    runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                            testAddress);

    // wait that proxy is not available
    int counter = 0;  // counter for avoiding endless loop
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    // subscribe to broadcast
    testProxy_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    });

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    // wait that proxy is  available
    counter = 0;  // counter for avoiding endless loop
    while ( !testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    // send value '1' via a method call - this tells stub to broadcast
    uint8_t in_ = 1;
    uint8_t out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        usleep(10000);
    }
    EXPECT_EQ(result, 1);

    runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                            testAddress);

    // wait that proxy is not available
    counter = 0;  // counter for avoiding endless loop
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }

    ASSERT_FALSE(testProxy_->isAvailable());

    serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    // wait that proxy is  available
    counter = 0;  // counter for avoiding endless loop
    while ( !testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    result = 0;
    // send value '1' via a method call - this tells stub to broadcast
    in_ = 1;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        usleep(10000);
    }
    EXPECT_EQ(result, 1);
}

/**
* @test Test SelectiveBroadcastStubGoesOfflineOnlineAgain.
*  - service offline
*  - subscribe to selective broadcast
*  - service online
*  - fire selective broadcast -> proxy should receive
*  - service offline
*  - service online
*  - fire again -> proxy should receive again
*/
TEST_F(CMBroadcasts, SelectiveBroadcastStubGoesOfflineOnlineAgain) {
    CommonAPI::CallStatus callStatus;
    uint8_t result = 0;
    uint8_t in_ = 1;
    uint8_t out_ = 0;

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;
    testProxy_->testMethod(in_, callStatus, out_);

    runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                            testAddress);

    // wait that proxy is not available
    int counter = 0;  // counter for avoiding endless loop
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint32_t errorHandlerCount = 0;
    // subscribe to broadcast
    testProxy_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        (void) status;
        ++errorHandlerCount;
    });

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    // wait that proxy is  available
    counter = 0;  // counter for avoiding endless loop
    while ( !testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    // send value '3' via a method call - this tells stub to broadcast selective
    in_ = 3;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        usleep(10000);
    }
    EXPECT_EQ(result, 1);

    runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                            testAddress);

    // wait that proxy is not available
    counter = 0;  // counter for avoiding endless loop
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    // wait that proxy is  available
    counter = 0;  // counter for avoiding endless loop
    while ( !testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;
    testProxy_->testMethod(in_, callStatus, out_);

    result = 0;
    // send value '3' via a method call - this tells stub to broadcast selective
    in_ = 3;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        usleep(10000);
    }
    EXPECT_EQ(result, 1);
    EXPECT_EQ(errorHandlerCount, 2u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
