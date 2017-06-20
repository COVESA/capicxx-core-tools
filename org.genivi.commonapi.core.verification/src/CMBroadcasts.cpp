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
#include <atomic>

#include <gtest/gtest.h>

#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/communication/TestInterfaceProxy.hpp"
#include "stub/CMBroadcastsStub.h"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";
const std::string otherclientId = "other-client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.communication.TestInterface";

const int tasync = 10000;

#ifdef TESTS_BAT
const unsigned int wf = 10; /* "wait-factor" when run in BAT environment */
#else
const unsigned int wf = 1;
#endif

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

        testStub_ = std::make_shared<CMBroadcastsStub>();
        bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
        ASSERT_TRUE((bool)testProxy_);
        int i = 0;
        while(!testProxy_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        bool serviceUnregistered =
                runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                        testAddress);

        ASSERT_TRUE(serviceUnregistered);

        if (testProxy_) {
            // wait that proxy is not available
            int counter = 0;  // counter for avoiding endless loop
            do {
                std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
                counter++;
            } while ( testProxy_->isAvailable() && counter < 100 );

            ASSERT_FALSE(testProxy_->isAvailable());
        }
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
    std::atomic<uint8_t> result;
    std::atomic<CommonAPI::CallStatus> subStatus;
    result = 0;

    // subscribe to broadcast
    testProxy_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });

    // check that subscription has succeeded
    for (int i = 0; i < 100; i++) {
        if (subStatus == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);

    // send value '1' via a method call - this tells stub to broadcast
    uint8_t in_ = 1;
    uint8_t out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
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
    std::atomic<CommonAPI::CallStatus> subStatus;
    std::atomic<uint8_t> result;
    result = 0;
    uint8_t in_ = 0;
    uint8_t out_ = 0;

    // send value '2' via a method call - this tells stub to stop accepting subs
    in_ = 2;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

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
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    // The following does not happen in SOME/IP, so it's commented out.
    EXPECT_EQ(subStatus, CommonAPI::CallStatus::SUBSCRIPTION_REFUSED);

    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that no value was correctly received
    std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
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
    std::atomic<CommonAPI::CallStatus> subStatus;
    std::atomic<uint8_t> result;
    result = 0;
    uint8_t in_ = 0;
    uint8_t out_ = 0;

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

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
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(subStatus, CommonAPI::CallStatus::SUCCESS);

    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
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
    std::atomic<uint8_t> result;
    std::atomic<CommonAPI::CallStatus> subStatus;
    result = 0;

    runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                            testAddress);

    // wait that proxy is not available
    int counter = 0;  // counter for avoiding endless loop
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    // subscribe to broadcast
    testProxy_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    // wait that proxy is available
    counter = 0;  // counter for avoiding endless loop
    while ( !testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        counter++;
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    // check that subscription has succeeded
    for (int i = 0; i < 100; i++) {
        if (subStatus == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);

    // send value '1' via a method call - this tells stub to broadcast
    uint8_t in_ = 1;
    uint8_t out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);

    subStatus = CommonAPI::CallStatus::UNKNOWN;

    runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                            testAddress);

    // wait that proxy is not available
    counter = 0;  // counter for avoiding endless loop
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        counter++;
    }

    ASSERT_FALSE(testProxy_->isAvailable());

    serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    // wait that proxy is  available
    counter = 0;  // counter for avoiding endless loop
    while ( !testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        counter++;
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    // check that subscription has succeeded
    for (int i = 0; i < 100; i++) {
        if (subStatus == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);

    result = 0;
    // send value '1' via a method call - this tells stub to broadcast
    in_ = 1;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
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
    std::atomic<uint8_t> result;
    result = 0;
    uint8_t in_ = 1;
    uint8_t out_ = 0;

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                            testAddress);

    // wait that proxy is not available
    int counter = 0;  // counter for avoiding endless loop
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    std::atomic<uint32_t> errorHandlerCount;
    errorHandlerCount = 0;
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
    while ( !testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        counter++;
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    // Let the proxy re-subscribe before trigger a notify one
    std::this_thread::sleep_for(std::chrono::microseconds(tasync * 2));

    // send value '3' via a method call - this tells stub to broadcast selective
    in_ = 3;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);

    runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                            testAddress);

    // wait that proxy is not available
    counter = 0;  // counter for avoiding endless loop
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    // wait that proxy is  available
    counter = 0;  // counter for avoiding endless loop
    while ( !testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        counter++;
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // Let the proxy re-subscribe before trigger a notify
    std::this_thread::sleep_for(std::chrono::microseconds(tasync * 2));

    result = 0;
    // send value '3' via a method call - this tells stub to broadcast selective
    in_ = 3;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);
    EXPECT_EQ(errorHandlerCount, 2u);
}

TEST_F(CMBroadcasts, NormalBroadcast_Two_proxies_subscribe_and_one_reset) {
    CommonAPI::CallStatus callStatus;
    std::atomic<uint8_t> result, result2;
    std::atomic<CommonAPI::CallStatus> subStatus, subStatus2;
    result = 0;
    result2 = 0;

    auto anotherProxy = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
    int i = 0;
    while(!anotherProxy->isAvailable() && i++ < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(anotherProxy->isAvailable());

    // subscribe to broadcast
    uint32_t subscription = testProxy_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });
    // subscribe to broadcast
    anotherProxy->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result2 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus2 = status;
    });

    // check that all subscriptions have succeeded
    for (int i = 0; i < 100; i++) {
        if (subStatus == CommonAPI::CallStatus::SUCCESS &&
                subStatus2 == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus2);

    // send value '1' via a method call - this tells stub to broadcast
    uint8_t in_ = 1;
    uint8_t out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1 && result2 == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);
    EXPECT_EQ(result2, 1);

    result = 0;
    result2 = 0;

    anotherProxy.reset();

    // send value '1' via a method call - this tells stub to broadcast
    in_ = 1;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);

    testProxy_->getBTestEvent().unsubscribe(subscription);

    result = 0;
    result2 = 0;

    anotherProxy = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
    i = 0;
    while(!anotherProxy->isAvailable() && i++ < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(anotherProxy->isAvailable());

    subStatus = CommonAPI::CallStatus::UNKNOWN;
    subStatus2 = CommonAPI::CallStatus::UNKNOWN;

    // subscribe to broadcast
    testProxy_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });

    // subscribe to broadcast
    anotherProxy->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result2 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus2 = status;
    });

    // check that all subscriptions have succeeded
    for (int i = 0; i < 100; i++) {
        if (subStatus == CommonAPI::CallStatus::SUCCESS &&
                subStatus2 == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus2);

    // send value '1' via a method call - this tells stub to broadcast
    in_ = 1;
    out_ = 0;
    testProxy_->testMethod(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1 && result2 == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);
    EXPECT_EQ(result2, 1);
}

TEST_F(CMBroadcasts, Two_proxies_subscribe_delete_one_proxy_status_listener_test) {
    std::atomic<CommonAPI::CallStatus> subStatus1;
    std::atomic<CommonAPI::CallStatus> subStatus2;
    std::atomic<CommonAPI::CallStatus> subStatus3;
    std::atomic<CommonAPI::CallStatus> subStatus4;
    std::atomic<uint8_t> result, result2;
    result = 0;
    result2 = 0;

    // subscribe first Proxy
    subStatus1 = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus1 = status;
    });
    // check that status was correctly received
    for (int i = 0; i < 100; i++) {
        if (subStatus1 == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus1);

    // subscribe second Proxy (another connection)
    subStatus2 = CommonAPI::CallStatus::UNKNOWN;
    auto testProxy2_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
    testProxy2_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus2 = status;
    });
    // check that status was correctly received
    for (int i = 0; i < 100; i++) {
        if (subStatus2 == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus2);

    // Build second proxy on same connection than first proxy
    auto secondProxy = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
    int i = 0;
    while(!secondProxy->isAvailable() && i++ < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(secondProxy->isAvailable());

    // subscribe second Proxy same connection
    subStatus3 = CommonAPI::CallStatus::UNKNOWN;
    secondProxy->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus3 = status;
    });

    // Build second proxy on same connection than first proxy
    auto thirdProxy = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, otherclientId);
    i = 0;
    while(!thirdProxy->isAvailable() && i++ < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(thirdProxy->isAvailable());

    // subscribe second Proxy same connection
    subStatus4 = CommonAPI::CallStatus::UNKNOWN;
    thirdProxy->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus4 = status;
    });

    // Delete first proxy to ensure same and other connection receive their status anyways!
    testProxy_.reset();

    // check that status was correctly received anyways
    for (int i = 0; i < 100; i++) {
        if (subStatus3 == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus3);

    // check that status was correctly received anyways
    for (int i = 0; i < 100; i++) {
        if (subStatus4 == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus4);

    subStatus1 = CommonAPI::CallStatus::UNKNOWN;
    subStatus2 = CommonAPI::CallStatus::UNKNOWN;
    subStatus3 = CommonAPI::CallStatus::UNKNOWN;
    subStatus4 = CommonAPI::CallStatus::UNKNOWN;

    bool serviceUnregistered =
            runtime_->unregisterService(domain, CMBroadcastsStub::StubInterface::getInterface(),
                    testAddress);
     ASSERT_TRUE(serviceUnregistered);

     std::this_thread::sleep_for(std::chrono::microseconds(tasync));

     bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
     ASSERT_TRUE(serviceRegistered);

     // check that all states were correctly received after service comes up again
     for (int i = 0; i < 100; i++) {
         if (subStatus2 == CommonAPI::CallStatus::SUCCESS &&
                 subStatus4 == CommonAPI::CallStatus::SUCCESS &&
                 subStatus4 == CommonAPI::CallStatus::SUCCESS) break;
         std::this_thread::sleep_for(std::chrono::microseconds(tasync));
     }
     ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus2);
     ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus3);
     ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus4);
     ASSERT_EQ(CommonAPI::CallStatus::UNKNOWN, subStatus1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
