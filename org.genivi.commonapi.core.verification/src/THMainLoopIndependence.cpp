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
#include "v1_0/commonapi/threading/TestInterfaceProxy.hpp"
#include "v1_0/commonapi/threading/TestInterfaceStubDefault.hpp"
#include "utils/VerificationMainLoop.h"

const std::string domain = "local";
const std::string instance6 = "my.test.commonapi.address.six";
const std::string instance7 = "my.test.commonapi.address.seven";
const std::string instance8 = "my.test.commonapi.address.eight";
const std::string mainloopName1 = "client-sample";
const std::string mainloopName2 = "service-sample";
const std::string thirdPartyServiceId = "mainloop-thirdParty";

class PingPongTestStub : public v1_0::commonapi::threading::TestInterfaceStubDefault {
	virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client,
			uint8_t _x,
			testMethodReply_t _reply) {

		_reply(_x);
    }
};

class MainLoopThreadContext {
public:
    void setupRuntime(std::promise<bool>& p) {
        runtime_ = CommonAPI::Runtime::get();
        p.set_value(true);
    }

    void setupMainLoopContext(std::promise<bool>& p, std::string mainloopName) {
    	mainLoopContext_ = std::make_shared<CommonAPI::MainLoopContext>(mainloopName);
        mainLoop_ = new CommonAPI::VerificationMainLoop(mainLoopContext_);
        p.set_value(true);
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

        std::promise<bool> promiseRuntime1, promiseRuntime2;
        std::future<bool> futureRuntime1 = promiseRuntime1.get_future();
        std::future<bool> futureRuntime2 = promiseRuntime2.get_future();

        mainLoopThread1_ = std::thread(
                        std::bind(&MainLoopThreadContext::setupRuntime, &threadCtx1_, std::move(promiseRuntime1)));
        mainLoopThread2_ = std::thread(
                        std::bind(&MainLoopThreadContext::setupRuntime, &threadCtx2_, std::move(promiseRuntime2)));

        mainLoopThread1_.detach();
        mainLoopThread2_.detach();

        futureRuntime1.wait_for(std::chrono::milliseconds(200));
        futureRuntime2.wait_for(std::chrono::milliseconds(200));

        // check that both threads have a runtime and it is the same
        ASSERT_TRUE((bool)threadCtx1_.runtime_);
        ASSERT_TRUE((bool)threadCtx2_.runtime_);
        ASSERT_EQ(threadCtx1_.runtime_, threadCtx2_.runtime_);

        std::promise<bool> promiseContext1, promiseContext2;
        std::future<bool> futureContext1 = promiseContext1.get_future();
        std::future<bool> futureContext2 = promiseContext2.get_future();

        mainLoopThread1_ = std::thread(
                        std::bind(
                                        &MainLoopThreadContext::setupMainLoopContext,
                                        &threadCtx1_,
                                        std::move(promiseContext1),
                                        mainloopName1));
        mainLoopThread2_ = std::thread(
                        std::bind(
                                        &MainLoopThreadContext::setupMainLoopContext,
                                        &threadCtx2_,
                                        std::move(promiseContext2),
                                        mainloopName2));
        mainLoopThread1_.detach();
        mainLoopThread2_.detach();

        futureContext1.wait_for(std::chrono::milliseconds(200));
        futureContext2.wait_for(std::chrono::milliseconds(200));

        // check that both threads have an own mainloop context
        ASSERT_TRUE((bool)threadCtx1_.mainLoopContext_);
        ASSERT_TRUE((bool)threadCtx2_.mainLoopContext_);
        ASSERT_NE(threadCtx1_.mainLoopContext_, threadCtx2_.mainLoopContext_);

        std::promise<bool> promiseFactory1, promiseFactory2;
        std::future<bool> futureFactory1 = promiseFactory1.get_future();
        std::future<bool> futureFactory2 = promiseFactory2.get_future();

        futureFactory1.wait_for(std::chrono::milliseconds(200));
        futureFactory2.wait_for(std::chrono::milliseconds(200));

        // set addresses
        threadCtx1_.setAddresses(instance7, instance8, instance6);
        threadCtx2_.setAddresses(instance8, instance7, instance6);

        threadCtx1_.createProxyAndStub();
        threadCtx2_.createProxyAndStub();

        mainLoopThread1_ = std::thread([&]() { threadCtx1_.mainLoop_->run(); });
        mainLoopThread2_ = std::thread([&]() { threadCtx2_.mainLoop_->run(); });

        usleep(200000);

        ASSERT_TRUE(threadCtx1_.proxy_->isAvailable());
        ASSERT_TRUE(threadCtx2_.proxy_->isAvailable());

        if (threadCtx1_.mainLoop_->isRunning()) {
        	std::future<bool> threadCtx1MainStopped = threadCtx1_.mainLoop_->stop();
        	threadCtx1MainStopped.get();
        }
        if (threadCtx2_.mainLoop_->isRunning()) {
        	std::future<bool> threadCtx2MainStopped = threadCtx2_.mainLoop_->stop();
        	threadCtx2MainStopped.get();
        }

        mainLoopThread1_.join();
        mainLoopThread2_.join();
    }

    void TearDown() {
    	threadCtx1_.runtime_->unregisterService(domain, PingPongTestStub::StubInterface::getInterface(), instance6);
    	threadCtx1_.runtime_->unregisterService(domain, PingPongTestStub::StubInterface::getInterface(), instance7);
    	threadCtx2_.runtime_->unregisterService(domain, PingPongTestStub::StubInterface::getInterface(), instance8);
        usleep(2000);
        threadCtx1_.mainLoop_->stop();
        threadCtx2_.mainLoop_->stop();

        if (threadCtx1_.mainLoop_->isRunning()) {
        	std::future<bool> threadCtx1MainStopped = threadCtx1_.mainLoop_->stop();
        	threadCtx1MainStopped.get();
        }
        if (threadCtx2_.mainLoop_->isRunning()) {
        	std::future<bool> threadCtx2MainStopped = threadCtx2_.mainLoop_->stop();
        	threadCtx2MainStopped.get();
        }

        if(mainLoopThread1_.joinable()) {
            mainLoopThread1_.join();
        }
        if(mainLoopThread2_.joinable()) {
            mainLoopThread2_.join();
        }
    }

    MainLoopThreadContext threadCtx1_, threadCtx2_;
    std::thread mainLoopThread1_, mainLoopThread2_;
};

/**
* @test Proxy Receives Answer Only If Stub MainLoop Runs.
* 	- start proxy in thread 1 and call testPredefinedTypeMethod
* 	- proxy should not receive answer, if the stub mainloop does not run
* 	- run mainloop of stub
* 	- now the stub mainloop also runs, so the proxy should receive the answer
*/
TEST_F(THMainLoopIndependence, ProxyReceivesAnswerOnlyIfStubMainLoopRuns) {

    CommonAPI::CallStatus callStatus;

    uint8_t x, y;
    x = 1;
    y = 0;

    std::thread mainLoopRunnerProxy([&]() { threadCtx1_.mainLoop_->runVerification(5000, true, true); });
    mainLoopRunnerProxy.detach();

    mainLoopThread1_ = std::thread([&]() {  threadCtx1_.proxy_->testMethod(x, callStatus, y); });
    mainLoopThread1_.detach();

    usleep(100000);
    // proxy should not receive answer, if the stub mainloop does not run
    ASSERT_EQ(0, y);

    mainLoopThread2_ = std::thread([&]() { threadCtx2_.mainLoop_->run(); });
    mainLoopThread2_.detach();

    usleep(1000000);

    // now the stub mainloop also runs, so the proxy should receive the answer
    ASSERT_EQ(1, y);
}

/**
* @test Proxy Receives Just His Own Answers.
* 	- start 2 proxies in own threads
* 	- call test method in each proxy
* 	- now each proxy should have received the answer to his own request
*/
TEST_F(THMainLoopIndependence, ProxyReceivesJustHisOwnAnswers) {

    usleep(1000000);

    std::shared_ptr<PingPongTestStub> stubThirdParty = std::make_shared<PingPongTestStub>();
    auto runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE(runtime->registerService(domain, instance6, stubThirdParty, thirdPartyServiceId));

    CommonAPI::CallStatus callStatusProxy1, callStatusProxy2;

    uint8_t x1, y1, x2, y2;
    x1 = 1;
    x2 = 2;
    y1 = y2 = 0;

    std::thread mainLoopRunnerProxy1([&]() { threadCtx1_.mainLoop_->run(); });
    std::thread mainLoopRunnerProxy2([&]() { threadCtx2_.mainLoop_->run(); });
    mainLoopRunnerProxy1.detach();
    mainLoopRunnerProxy2.detach();

    // wait until threads are running
    while (!threadCtx1_.mainLoop_->isRunning() || !threadCtx2_.mainLoop_->isRunning()) {
    	usleep(100);
    }

    while(!(threadCtx1_.proxyThirdParty_->isAvailable() && threadCtx2_.proxyThirdParty_->isAvailable())) {
        usleep(10000);
    }

    mainLoopThread1_ = std::thread([&]() {  threadCtx1_.proxyThirdParty_->testMethod(x1, callStatusProxy1, y1); });
    mainLoopThread2_ = std::thread([&]() {  threadCtx2_.proxyThirdParty_->testMethod(x2, callStatusProxy2, y2); });
    mainLoopThread1_.detach();
    mainLoopThread2_.detach();

    usleep(1000000);
    // now each proxy should have received the answer to his own request
    ASSERT_EQ(1, y1);
    ASSERT_EQ(2, y2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
