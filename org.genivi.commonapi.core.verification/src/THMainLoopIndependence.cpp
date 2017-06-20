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
#include "v1/commonapi/threading/TestInterfaceStubDefault.hpp"
#include "utils/VerificationMainLoop.h"

const std::string domain = "local";
const std::string instance6 = "my.test.commonapi.address.six";
const std::string instance7 = "my.test.commonapi.address.seven";
const std::string instance8 = "my.test.commonapi.address.eight";
const std::string mainloopName1 = "client-sample";
const std::string mainloopName2 = "service-sample";
const std::string thirdPartyServiceId = "mainloop-thirdParty";

const int tasync = 10000;

class PingPongTestStub : public v1_0::commonapi::threading::TestInterfaceStubDefault {
    virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client,
            uint8_t _x,
            testMethodReply_t _reply) {
        (void)_client;
        _reply(_x);
    }
};

class MainLoopThreadContext {
public:
    void setRuntime(std::shared_ptr<CommonAPI::Runtime> _runtime) {
        runtime_ = _runtime;
    }

    void setupMainLoopContext(std::string mainloopName) {
        mainLoopContext_ = std::make_shared<CommonAPI::MainLoopContext>(mainloopName);
        mainLoop_ = new CommonAPI::VerificationMainLoop(mainLoopContext_);
    }

    void setAddresses(const std::string own, const std::string other, const std::string thirdParty) {
        ownAddress_ = own;

        otherAddress_  = other;
        thirdPartyAddress_ = thirdParty;
    }

    void createProxyAndStub() {
        stub_ = std::make_shared<PingPongTestStub>();

        ASSERT_TRUE(runtime_->registerService(domain, ownAddress_, stub_, mainLoopContext_));

        proxy_ = runtime_->buildProxy<v1_0::commonapi::threading::TestInterfaceProxy>(domain, otherAddress_, mainLoopContext_);
        ASSERT_TRUE((bool)proxy_);

        proxyThirdParty_ = runtime_->buildProxy<v1_0::commonapi::threading::TestInterfaceProxy>(domain, thirdPartyAddress_, mainLoopContext_);
        ASSERT_TRUE((bool)proxyThirdParty_);
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::MainLoopContext> mainLoopContext_;

    std::string ownAddress_, otherAddress_, thirdPartyAddress_;
    std::shared_ptr<PingPongTestStub> stub_;
    std::shared_ptr<v1_0::commonapi::threading::TestInterfaceProxy<>> proxy_, proxyThirdParty_;

    CommonAPI::VerificationMainLoop* mainLoop_;
};

class THMainLoopIndependence: public ::testing::Test {
protected:
    void SetUp() {
        std::shared_ptr<CommonAPI::Runtime> runtimePtr1_, runtimePtr2_;

        std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
        threadCtx1_.setRuntime(runtime);
        threadCtx2_.setRuntime(runtime);

        // check that both threads have a runtime and it is the same
        ASSERT_TRUE((bool)threadCtx1_.runtime_);
        ASSERT_TRUE((bool)threadCtx2_.runtime_);
        ASSERT_EQ(threadCtx1_.runtime_, threadCtx2_.runtime_);

        threadCtx1_.setupMainLoopContext(mainloopName1);
        threadCtx2_.setupMainLoopContext(mainloopName2);

        // check that both threads have an own mainloop context
        ASSERT_TRUE((bool)threadCtx1_.mainLoopContext_);
        ASSERT_TRUE((bool)threadCtx2_.mainLoopContext_);
        ASSERT_NE(threadCtx1_.mainLoopContext_, threadCtx2_.mainLoopContext_);

        // set addresses
        threadCtx1_.setAddresses(instance7, instance8, instance6);
        threadCtx2_.setAddresses(instance8, instance7, instance6);

        threadCtx1_.createProxyAndStub();
        threadCtx2_.createProxyAndStub();

        mainLoopThread1_ = std::thread([&]() { threadCtx1_.mainLoop_->run(); });
        mainLoopThread2_ = std::thread([&]() { threadCtx2_.mainLoop_->run(); });

        ASSERT_TRUE((bool)threadCtx1_.proxy_);
        if (threadCtx1_.proxy_) {
            for (unsigned int i = 0; !threadCtx1_.proxy_->isAvailable() && i < 100; ++i) {
                std::this_thread::sleep_for(std::chrono::microseconds(tasync));
            }
            ASSERT_TRUE(threadCtx1_.proxy_->isAvailable());
        }

        ASSERT_TRUE((bool)threadCtx2_.proxy_);
        if (threadCtx2_.proxy_) {
            for (unsigned int i = 0; !threadCtx2_.proxy_->isAvailable() && i < 100; ++i) {
                std::this_thread::sleep_for(std::chrono::microseconds(tasync));
            }
            ASSERT_TRUE(threadCtx2_.proxy_->isAvailable());
        }


        // wait until threads are running
        while (!threadCtx1_.mainLoop_->isRunning() || !threadCtx2_.mainLoop_->isRunning()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        threadCtx1_.mainLoop_->stop();
        threadCtx2_.mainLoop_->stop();

        while(threadCtx1_.mainLoop_->isRunning() || threadCtx2_.mainLoop_->isRunning()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        if(mainLoopThread1_.joinable()) {
            mainLoopThread1_.join();
        }
        if(mainLoopThread2_.joinable()) {
            mainLoopThread2_.join();
        }
    }

    void TearDown() {
        threadCtx1_.runtime_->unregisterService(domain, PingPongTestStub::StubInterface::getInterface(), instance6);
        threadCtx1_.runtime_->unregisterService(domain, PingPongTestStub::StubInterface::getInterface(), instance7);
        threadCtx2_.runtime_->unregisterService(domain, PingPongTestStub::StubInterface::getInterface(), instance8);

        if (threadCtx1_.mainLoop_->isRunning()) {
            threadCtx1_.mainLoop_->stop();
        }

        if (threadCtx2_.mainLoop_->isRunning()) {
            threadCtx2_.mainLoop_->stop();
        }

        while(threadCtx1_.mainLoop_->isRunning() || threadCtx2_.mainLoop_->isRunning()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        if(mainLoopThread1_.joinable()) {
            mainLoopThread1_.join();
        }
        if(mainLoopThread2_.joinable()) {
            mainLoopThread2_.join();
        }
        if(mainLoopRunnerProxy1_.joinable()) {
            mainLoopRunnerProxy1_.join();
        }
        if(mainLoopRunnerProxy2_.joinable()) {
            mainLoopRunnerProxy2_.join();
        }

        threadCtx1_.proxy_.reset();
        threadCtx1_.proxyThirdParty_.reset();
        threadCtx2_.proxy_.reset();
        threadCtx2_.proxyThirdParty_.reset();

        std::this_thread::sleep_for(std::chrono::microseconds(20000));

        delete threadCtx1_.mainLoop_;
        delete threadCtx2_.mainLoop_;
    }

    MainLoopThreadContext threadCtx1_, threadCtx2_;
    std::thread mainLoopThread1_, mainLoopThread2_, mainLoopRunnerProxy1_, mainLoopRunnerProxy2_;
    std::condition_variable condVar1_, condVar2_;
    std::mutex m1_, m2_;
};

/**
* @test Proxy Receives Answer Only If Stub MainLoop Runs.
*     - start proxy in thread 1 and call testPredefinedTypeMethod
*     - proxy should not receive answer, if the stub mainloop does not run
*     - run mainloop of stub
*     - now the stub mainloop also runs, so the proxy should receive the answer
*/
TEST_F(THMainLoopIndependence, ProxyReceivesAnswerOnlyIfStubMainLoopRuns) {

    CommonAPI::CallStatus callStatus;

    bool finish = false;

    uint8_t x = 1;

    mainLoopRunnerProxy1_ = std::thread([&]() { threadCtx1_.mainLoop_->runVerification(5000, true, true); });

    mainLoopThread1_ = std::thread([&]() { 
        uint8_t y = 0;
        threadCtx1_.proxy_->testMethod(x, callStatus, y);
        for (int i = 0; i < 100; ++i) {
            if (y == 1) break;
            std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        }
        // now the stub mainloop also runs, so the proxy should receive the answer
        ASSERT_EQ(1, y);

        std::unique_lock<std::mutex> lock(m1_);
        finish = true;
        condVar1_.notify_one();
    });

    std::this_thread::sleep_for(std::chrono::microseconds(tasync));

    mainLoopThread2_ = std::thread([&]() { threadCtx2_.mainLoop_->run(); });

    std::unique_lock<std::mutex> lock(m1_);
    while(!finish) {
        condVar1_.wait(lock);
    }
}

/**
* @test Proxy Receives Just His Own Answers.
*     - start 2 proxies in own threads
*     - call test method in each proxy
*     - now each proxy should have received the answer to his own request
*/
TEST_F(THMainLoopIndependence, ProxyReceivesJustHisOwnAnswers) {
    std::shared_ptr<PingPongTestStub> stubThirdParty = std::make_shared<PingPongTestStub>();
    auto runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE(runtime->registerService(domain, instance6, stubThirdParty, thirdPartyServiceId));

    CommonAPI::CallStatus callStatusProxy1, callStatusProxy2;

    bool finish1, finish2 = false;

    mainLoopRunnerProxy1_ = std::thread([&]() { threadCtx1_.mainLoop_->run(); });
    mainLoopRunnerProxy2_ = std::thread([&]() { threadCtx2_.mainLoop_->run(); });

    while(!(threadCtx1_.proxyThirdParty_->isAvailable() && threadCtx2_.proxyThirdParty_->isAvailable())) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }

    mainLoopThread1_ = std::thread([&]() {
        uint8_t x1, y1;
        x1 = 1;
        y1 = 0;

        threadCtx1_.proxyThirdParty_->testMethod(x1, callStatusProxy1, y1);

        for (int i = 0; i < 100; ++i) {
            if (y1 == 1) break;
            std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        }
        ASSERT_EQ(1, y1);

        std::unique_lock<std::mutex> lock(m1_);
        finish1 = true;
        condVar1_.notify_one();
    });

    mainLoopThread2_ = std::thread([&]() { 
        uint8_t x2, y2;
        x2 = 2;
        y2 = 0;

        threadCtx2_.proxyThirdParty_->testMethod(x2, callStatusProxy2, y2);

        for (int i = 0; i < 100; ++i) {
            if (y2 == 2) break;
            std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        }
        ASSERT_EQ(2, y2);

        std::unique_lock<std::mutex> lock(m2_);
        finish2 = true;
        condVar2_.notify_one();
    });

    std::unique_lock<std::mutex> lock1(m1_);
    while(!finish1) {
        condVar1_.wait(lock1);
    }

    std::unique_lock<std::mutex> lock2(m2_);
    while(!finish2) {
        condVar2_.wait(lock2);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
