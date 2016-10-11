/* Copyright (C) 2014 - 2015 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Threading
*/

#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "utils/VerificationMainLoop.h"
#include "v1/commonapi/threading/TestInterfaceProxy.hpp"
#include "utils/VerificationMainLoop.h"
#include "stub/THMainLoopIntegrationStub.h"

const std::string domain = "local";
const std::string instance = "my.test.commonapi.address";
const std::string connection_client = "client-sample";
const std::string connection_service = "service-sample";

class THMainLoopIntegration: public ::testing::Test {

protected:
    void SetUp() {

        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool) runtime_);

        contextForProxy_ = std::make_shared<CommonAPI::MainLoopContext>(connection_client);
        contextForStub_ = std::make_shared<CommonAPI::MainLoopContext>(connection_service);

        ASSERT_TRUE((bool) contextForProxy_);
        ASSERT_TRUE((bool) contextForStub_);
        ASSERT_FALSE(contextForProxy_ == contextForStub_);

        mainLoopForProxy_ = new CommonAPI::VerificationMainLoop(contextForProxy_);
        mainLoopForStub_ = new CommonAPI::VerificationMainLoop(contextForStub_);

        testStub_ = std::make_shared<v1_0::commonapi::threading::THMainLoopIntegrationStub>();
        serviceRegistered_ = runtime_->registerService(domain, instance, testStub_, contextForStub_);
        ASSERT_TRUE(serviceRegistered_);

        testProxy_ = runtime_->buildProxy<v1_0::commonapi::threading::TestInterfaceProxy>(domain, instance, contextForProxy_);
        ASSERT_TRUE((bool) testProxy_);

        callbackCalled_ = 0;
        lastBroadcastNumber_ = 0;
        outInt_ = 0;
    }

    void TearDown() {
        runtime_->unregisterService(domain, v1_0::commonapi::threading::THMainLoopIntegrationStub::StubInterface::getInterface(), instance);

        if (mainLoopForProxy_->isRunning()) {
            std::future<bool> proxyStopped = mainLoopForProxy_->stop();
            // synchronisation with stopped mainloop
            proxyStopped.get();
        }
        if (mainLoopForStub_->isRunning()) {
            std::future<bool> stubStopped = mainLoopForStub_->stop();
            // synchronisation with stopped mainloop
            stubStopped.get();
        }

        testProxy_.reset();

        std::this_thread::sleep_for(std::chrono::microseconds(200));

        delete mainLoopForProxy_;
        delete mainLoopForStub_;
    }

    bool serviceRegistered_;

    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<CommonAPI::MainLoopContext> contextForProxy_;
    std::shared_ptr<CommonAPI::MainLoopContext> contextForStub_;
    std::shared_ptr<CommonAPI::Factory> mainloopFactoryProxy_;
    std::shared_ptr<CommonAPI::Factory> mainloopFactoryStub_;

    CommonAPI::VerificationMainLoop* mainLoopForProxy_;
    CommonAPI::VerificationMainLoop* mainLoopForStub_;

    std::shared_ptr<v1_0::commonapi::threading::TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<v1_0::commonapi::threading::THMainLoopIntegrationStub> testStub_;

    int callbackCalled_;
    uint8_t lastBroadcastNumber_;
    int outInt_;

public:
    void broadcastCallback(uint8_t value) {

        // check correct order
        lastBroadcastNumber_++;
        ASSERT_EQ(lastBroadcastNumber_, value);
    }
};

/**
* @test Verifies communication with Main Loop.
*   - get proxy with available flag = true
*   - generate big test data
*   - send synchronous test message
*/
TEST_F(THMainLoopIntegration, VerifyCommunicationWithMainLoop) {

    std::thread proxyThread;
    std::thread stubThread;
    proxyThread = std::thread([&](){ mainLoopForProxy_->run(); });
    stubThread = std::thread([&](){ mainLoopForStub_->run(); });
    proxyThread.detach();
    stubThread.detach();

    // wait until threads are running
    while (!mainLoopForProxy_->isRunning() || !mainLoopForStub_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    for(unsigned int i = 0; !testProxy_->isAvailable() && i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    uint8_t x = 5;
    uint8_t y = 0;
    CommonAPI::CallStatus callStatus;

    testProxy_->testMethod(x, callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);
}

/**
* @test Verifies Transport Reading When Dispatching Watches.
*     - get proxy with available flag = true
*     - generate big test data
*     - send asynchronous test message
*     - dispatch dispatchSource: the message must not be arrived
*     - dispatch watches (reads transport).
*     - dispatch dispatchSources again: now the message must be arrived.
*/
TEST_F(THMainLoopIntegration, VerifyTransportReading) {

    std::thread proxyThread = std::thread([&](){ mainLoopForProxy_->run(); });
    std::thread stubThread = std::thread([&](){ mainLoopForStub_->run(); });
    proxyThread.detach();

    // wait until threads are running
    while (!mainLoopForProxy_->isRunning() || !mainLoopForStub_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    for(unsigned int i = 0; !testProxy_->isAvailable() && i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    if (mainLoopForStub_->isRunning()) {
        std::future<bool> stubStopped = mainLoopForStub_->stop();
        // synchronisation with stopped mainloop
        stubStopped.get();
    }
    if (stubThread.joinable()) {
        stubThread.join();
    }

    uint8_t x = 5;
    //uint8_t y = 0;

    std::future<CommonAPI::CallStatus> futureStatus = testProxy_->testMethodAsync(x,
                                    [&] (const CommonAPI::CallStatus& status, uint8_t y) {
                    (void)status;
                    (void)y;
                                        callbackCalled_++;
                                    }
                                    );

    // 1. just dispatch watches (reads transport)
    mainLoopForStub_->runVerification(1, true, false);
    std::this_thread::sleep_for(std::chrono::microseconds(10000));
    EXPECT_EQ(testStub_->x_, 0);

    // 2. just dispatch dispatchSources. This should dispatch the messages already read from transport in 1.
    mainLoopForStub_->doVerificationIteration(false, true);
    EXPECT_EQ(testStub_->x_, x);
}

/**
* @test Verifies Synchronous Call Message Handling Order.
*     - get proxy with available flag = true
*     - subscribe for broadcast event
*     - generate 5 test broadcasts
*     - 5 broadcasts should arrive in the right order
*/
TEST_F(THMainLoopIntegration, VerifySyncCallMessageHandlingOrder) {

    std::thread stubThread = std::thread([&](){ mainLoopForStub_->run(3000); });
    // wait until thread is running
    while (!mainLoopForStub_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    for(unsigned int i = 0; !testProxy_->isAvailable() && i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        mainLoopForProxy_->doSingleIteration(30);
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    auto& broadcastEvent = testProxy_->getTestBroadcastEvent();
    broadcastEvent.subscribe(std::bind(&THMainLoopIntegration::broadcastCallback, this, std::placeholders::_1));

    uint8_t x = 5;

    // TODO: Why sync call blocks here!?
    testProxy_->testMethodAsync(x, [&](const CommonAPI::CallStatus& status, uint8_t y) {
        (void) status;
        (void) y;
    });

    for(unsigned int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        mainLoopForProxy_->doSingleIteration(30);
    }

    if (mainLoopForStub_->isRunning()) {
        std::future<bool> stubStopped = mainLoopForStub_->stop();
        // synchronisation with stopped mainloop
        stubStopped.get();
    }
    if (stubThread.joinable()) {
        stubThread.join();
    }

    // in total 5 broadcasts should have been arrived
    ASSERT_EQ(lastBroadcastNumber_, 5);
}

/**
* @test Verifies SelectiveError Handler is called correctly when used with mainloop
*   - get proxy with available flag = true
*   - Subscribe for selective Event and register error handler
*   - Stub fires event upon subscription
*   - Check that subscription handler and error handler were both called once
*   - Unregister Service and register Service again
*   - Check that subscription error handler was called again after service went
*     offline and came online again (resubscription took place) and that the
*     event was received a second time
*/
TEST_F(THMainLoopIntegration, SelectiveErrorHandlerWithMainLoop) {

    std::thread proxyThread;
    std::thread stubThread;
    proxyThread = std::thread([&](){ mainLoopForProxy_->run(); });
    stubThread = std::thread([&](){ mainLoopForStub_->run(); });
    proxyThread.detach();
    stubThread.detach();

    // wait until threads are running
    while (!mainLoopForProxy_->isRunning() || !mainLoopForStub_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    for(unsigned int i = 0; !testProxy_->isAvailable() && i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    std::uint32_t broadcastTestValue(1234);
    std::uint32_t selectiveHandlerCalled(0);
    std::uint32_t selectiveErrorHandlerCalled(0);

    testStub_->setSecondTestBroadcastValueToFireOnSubscription_(broadcastTestValue);

    testProxy_->getSecondTestBroadcastSelectiveEvent().subscribe(
        [&](const uint32_t _value) {
            EXPECT_EQ(broadcastTestValue, _value);
            ++selectiveHandlerCalled;
        },
        [&](const CommonAPI::CallStatus _callStatus) {
            EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, _callStatus);
            ++selectiveErrorHandlerCalled;
        }
    );
    // Stub will fire SecondTestBroadcast once the subscription happened
    for (int i = 0; i < 200; ++i) {
        if (selectiveHandlerCalled == 1 && selectiveErrorHandlerCalled == 1) {
            break;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    EXPECT_EQ(1u, selectiveHandlerCalled);
    EXPECT_EQ(1u, selectiveErrorHandlerCalled);

    bool serviceDeregistered =
            runtime_->unregisterService(domain,
                    v1_0::commonapi::threading::THMainLoopIntegrationStub::StubInterface::getInterface(),
                    instance);
    ASSERT_TRUE(serviceDeregistered);
    serviceRegistered_ = runtime_->registerService(domain, instance, testStub_,
            contextForStub_);
    ASSERT_TRUE(serviceRegistered_);
    // Stub will fire SecondTestBroadcast once the resubscription happened
    for (int i = 0; i < 200; ++i) {
        if (selectiveHandlerCalled == 2 && selectiveErrorHandlerCalled == 2) {
            break;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    EXPECT_EQ(2u, selectiveHandlerCalled);
    EXPECT_EQ(2u, selectiveErrorHandlerCalled);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
