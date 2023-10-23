/* Copyright (C) 2014 - 2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Threading
*/

#include <gtest/gtest.h>
#include <thread>
#include "CommonAPI/CommonAPI.hpp"
#include "utils/VerificationMainLoopWithQueue.hpp"
#include "v1/commonapi/threading/TestInterfaceProxy.hpp"
#include "v1/commonapi/threading/TestInterfaceStubDefault.hpp"

const std::string domain = "local";
const std::string instance = "my.test.commonapi.address";

class PingPongTestStub : public v1_0::commonapi::threading::TestInterfaceStubDefault {
    virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client,
            uint8_t _x,
            testMethodReply_t _reply) {
        (void)_client;
        _reply(_x);
    }
};

class THMainLoopTwoThreads: public ::testing::Test {
protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        context_ = std::make_shared<CommonAPI::MainLoopContext>("client-sample");
        ASSERT_TRUE((bool)context_);

        eventQueue_ = std::make_shared<CommonAPI::VerificationMainLoopEventQueue>();
        mainLoop_ = new CommonAPI::VerificationMainLoop(context_, eventQueue_);

        stub_ = std::make_shared<PingPongTestStub>();

        bool stubRegistered = runtime_->registerService(domain, instance, stub_, "service-sample");
        ASSERT_TRUE((bool)stubRegistered);

        proxy_ = runtime_->buildProxy<v1_0::commonapi::threading::TestInterfaceProxy>(domain, instance, context_);
        ASSERT_TRUE((bool)proxy_);

        eventQueueThread_ = std::thread([&]() { eventQueue_->run(); });
        mainLoopThread_ = std::thread([&]() { mainLoop_->run(); });
    }

    void TearDown() {
        runtime_->unregisterService(domain, PingPongTestStub::StubInterface::getInterface(), instance);

        if(mainLoop_->isRunning()) {
            mainLoop_->stop();
        }

        if(eventQueue_->isRunning()) {
            eventQueue_->stop();
        }

        while(mainLoop_->isRunning() || eventQueue_->isRunning()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        if(mainLoopThread_.joinable()) {
            mainLoopThread_.join();
        }

        if(eventQueueThread_.joinable()) {
            eventQueueThread_.join();
        }

        std::future<void> proxyCompletionFuture = proxy_->getCompletionFuture();

        proxy_.reset();

        if (std::future_status::timeout == proxyCompletionFuture.wait_for(std::chrono::seconds(5))) {
            ADD_FAILURE() << "Proxy wasn't destroyed within time";
        }

        delete mainLoop_;
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::MainLoopContext> context_;
    std::shared_ptr<CommonAPI::VerificationMainLoopEventQueue> eventQueue_;

    std::shared_ptr<PingPongTestStub> stub_;
    std::shared_ptr<v1_0::commonapi::threading::TestInterfaceProxy<>> proxy_;

    CommonAPI::VerificationMainLoop* mainLoop_;

    std::thread eventQueueThread_;
    std::thread mainLoopThread_;
};

/**
* @test Proxy Receives Available when MainLoop Dispatched sourced out to other thread.
*/
TEST_F(THMainLoopTwoThreads, ProxyGetsAvailableStatus) {
    std::atomic<bool> isAvailable(false);
    
    proxy_->getProxyStatusEvent().subscribe([&](const CommonAPI::AvailabilityStatus& val) {
        if (val == CommonAPI::AvailabilityStatus::AVAILABLE) {
            isAvailable = true;
        }
    });

    int counter = 0;
    while (!isAvailable && counter < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        counter++;
    }

    ASSERT_TRUE(proxy_->isAvailable());
}

/**
* @test Proxy gets function response when MainLoop Dispatched sourced out to other thread.
*/
TEST_F(THMainLoopTwoThreads, ProxyGetsFunctionResponse) {
    std::atomic<bool> isAvailable(false);

    proxy_->getProxyStatusEvent().subscribe([&](const CommonAPI::AvailabilityStatus& val) {
        if (val == CommonAPI::AvailabilityStatus::AVAILABLE) {
            isAvailable = true;
        }
    });

    
    int counter = 0;
    while (!isAvailable && counter < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        counter++;
    }

    ASSERT_TRUE(proxy_->isAvailable());

    CommonAPI::CallStatus callStatus;

    uint8_t x, y;
    x = 1;
    y = 0;

    proxy_->testMethod(x, callStatus, y);

    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
    ASSERT_EQ(1, y);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
