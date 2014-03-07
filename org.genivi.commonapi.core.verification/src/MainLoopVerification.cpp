/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Main Loop Integration
*/

#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.h"
#include "utils/VerificationMainLoop.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "utils/VerificationTestStub.h"
#include <functional>

const std::string testAddress6 = "local:my.eigth.test:commonapi.address.six";
const std::string testAddress7 = "local:my.eigth.test:commonapi.address.seven";
const std::string testAddress8 = "local:my.eigth.test:commonapi.address.eight";

class PingPongTestStub : public commonapi::tests::TestInterfaceStubDefault {
    virtual void testPredefinedTypeMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
                                          uint32_t uint32InValue,
                                          std::string stringInValue,
                                          uint32_t& uint32OutValue,
                                          std::string& stringOutValue) {
        stringOutValue = stringInValue;
        uint32OutValue = uint32InValue;
    }
};

class MainLoopTest: public ::testing::Test {

protected:
    void SetUp() {

        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool) runtime_);

        contextForProxy_ = runtime_->getNewMainLoopContext();
        contextForStub_ = runtime_->getNewMainLoopContext();
        ASSERT_TRUE((bool) contextForProxy_);
        ASSERT_TRUE((bool) contextForStub_);
        ASSERT_FALSE(contextForProxy_ == contextForStub_);

        mainLoopForProxy_ = new CommonAPI::VerificationMainLoop(contextForProxy_);
        mainLoopForStub_ = new CommonAPI::VerificationMainLoop(contextForStub_);

        mainloopFactoryProxy_ = runtime_->createFactory(contextForProxy_);
        mainloopFactoryStub_ = runtime_->createFactory(contextForStub_);
        ASSERT_TRUE((bool) mainloopFactoryProxy_);
        ASSERT_TRUE((bool) mainloopFactoryStub_);
        ASSERT_FALSE(mainloopFactoryProxy_ == mainloopFactoryStub_);

        servicePublisher_ = runtime_->getServicePublisher();
        ASSERT_TRUE((bool) servicePublisher_);

        stub_ = std::make_shared<commonapi::verification::VerificationTestStub>();
        ASSERT_TRUE(servicePublisher_->registerService(stub_, testAddress8, mainloopFactoryStub_));

        callbackCalled = 0;
        lastBroadcastNumber = 0;
        outInt = 0;
    }

    void TearDown() {
        servicePublisher_->unregisterService(testAddress8);
        mainLoopForProxy_->stop();
        mainLoopForStub_->stop();
        usleep(200);
        delete mainLoopForProxy_;
        delete mainLoopForStub_;
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<CommonAPI::MainLoopContext> contextForProxy_;
    std::shared_ptr<CommonAPI::MainLoopContext> contextForStub_;
    std::shared_ptr<CommonAPI::Factory> mainloopFactoryProxy_;
    std::shared_ptr<CommonAPI::Factory> mainloopFactoryStub_;
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;
    std::shared_ptr<commonapi::verification::VerificationTestStub> stub_;

    CommonAPI::VerificationMainLoop* mainLoopForProxy_;
    CommonAPI::VerificationMainLoop* mainLoopForStub_;

    int callbackCalled;
    int lastBroadcastNumber;
    uint32_t outInt;
    CommonAPI::CallStatus callStatus;

public:
    void broadcastCallback(uint32_t intValue, std::string stringValue) {
        // check correct order
        lastBroadcastNumber++;
        ASSERT_EQ(lastBroadcastNumber, intValue);
        // check, if broadcast is handled after method call
        ASSERT_EQ(outInt, 1);
    }
};

/**
* @test Verifies Transport Reading When Dispatching Watches.
* 	- get proxy with available flag = true
* 	- generate big test data
* 	- send asynchronous test message
* 	- dispatch dispatchSource: the message must not be arrived
* 	- dispatch watches (reads transport).
* 	- dispatch dispatchSources again: now the message must be arrived.
*/
TEST_F(MainLoopTest, VerifyTransportReadingWhenDispatchingWatches) {
    auto proxy = mainloopFactoryProxy_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress8);
    ASSERT_TRUE((bool) proxy);

    std::thread stubThread = std::thread([&](){ mainLoopForStub_->run(); });

    while (!proxy->isAvailable()) {
        mainLoopForProxy_->doSingleIteration();
        usleep(500);
    }

    ASSERT_TRUE(proxy->isAvailable());

    mainLoopForStub_->stop();
    stubThread.join();

    uint32_t uint32Value = 42;
    std::string stringValue = "Hai :)";
    bool running = true;

    commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testEnumExtended2InValue =
                    commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK;
    commonapi::tests::DerivedTypeCollection::TestMap testMapInValue;

    // Estimated amount of data (differring padding at beginning/end of Map/Array etc. not taken into account):
    // 4 + 4 + 500 * (4 + (4 + 4 + 100 * (11 + 1 + 4)) + 4 ) = 811008
    for (uint32_t i = 0; i < 500; ++i) {
        commonapi::tests::DerivedTypeCollection::TestArrayTestStruct testArrayTestStruct;
        for (uint32_t j = 0; j < 100; ++j) {
            commonapi::tests::DerivedTypeCollection::TestStruct testStruct("Hai all (:", j);
            testArrayTestStruct.push_back(testStruct);
        }
        testMapInValue.insert( {i, testArrayTestStruct});
    }

    std::future<CommonAPI::CallStatus> futureStatus =
                    proxy->testDerivedTypeMethodAsync(
                                    testEnumExtended2InValue,
                                    testMapInValue,
                                    [&] (const CommonAPI::CallStatus& status,
                                                    commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testEnumExtended2OutValue,
                                                    commonapi::tests::DerivedTypeCollection::TestMap testMapOutValue) {
                                        mainLoopForProxy_->stop();
                                        callbackCalled++;
                                    }
                                    );

    mainLoopForProxy_->runVerification(15, true, true);

    // 1. just dispatch dispatchSources
    mainLoopForStub_->runVerification(15, false, true);
    EXPECT_EQ(stub_->getCalledTestDerivedTypeMethod(), 0);

    // 2. just dispatch watches (reads transport)
    mainLoopForStub_->runVerification(20, true, false);
    EXPECT_EQ(stub_->getCalledTestDerivedTypeMethod(), 0);

    // 3. just dispatch dispatchSources again. This should dispatch the messages already read from transport in 2.
    mainLoopForStub_->doVerificationIteration(false, true);
    EXPECT_EQ(stub_->getCalledTestDerivedTypeMethod(), 1);
}

/**
* @test Verifies Synchronous Call Message Handling Order.
* 	- get proxy with available flag = true
* 	- subscribe for broadcast event
* 	- generate 5 test broadcasts
* 	- 5 broadcasts should arrive in the right order
*/
TEST_F(MainLoopTest, VerifySyncCallMessageHandlingOrder) {
    auto proxy = mainloopFactoryProxy_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress8);
    ASSERT_TRUE((bool) proxy);

    std::thread stubThread = std::thread([&](){ mainLoopForStub_->run(); });

    while (!proxy->isAvailable()) {
        mainLoopForProxy_->doSingleIteration();
        usleep(500);
    }

    ASSERT_TRUE(proxy->isAvailable());

    auto& broadcastEvent = proxy->getTestPredefinedTypeBroadcastEvent();
    broadcastEvent.subscribe(std::bind(&MainLoopTest::broadcastCallback, this, std::placeholders::_1, std::placeholders::_2));

    CommonAPI::CallStatus callStatus;
    std::string outString;

    proxy->testPredefinedTypeMethod(0, "", callStatus, outInt, outString);
    ASSERT_EQ(outInt, 1);

    for (int i = 0; i < 10000; i++) {
        mainLoopForProxy_->doSingleIteration(100);
    }

    sleep(10);

    mainLoopForProxy_->stop();
    mainLoopForStub_->stop();

    stubThread.join();

    // in total 5 broadcasts should have been arrived
    ASSERT_EQ(lastBroadcastNumber, 5);
}

/**
* @test Synchronous Calls Do Not Deadlock.
* 	- get proxy with available flag = true
* 	- call synchronous test method in syncCallThread
* 	- 5 broadcasts should arrive in the right order
* 	- run the mainloop again in order to give the syncCallThread a chance to return
*/
TEST_F(MainLoopTest, SyncCallsDoNotDeadlock) {
    auto proxy = mainloopFactoryProxy_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress8);
    ASSERT_TRUE((bool) proxy);

    std::thread stubThread = std::thread([&]() {mainLoopForStub_->run();});

    // let the proxy become available
    while (!proxy->isAvailable()) {
        mainLoopForProxy_->doSingleIteration();
        usleep(500);
    }

    uint32_t inInt, outInt;
    std::string inStr, outStr;
    inInt = 1;
    outInt = 0;

    callStatus = CommonAPI::CallStatus::REMOTE_ERROR;

    std::thread syncCallThread = std::thread(
                    [&]() {proxy->testPredefinedTypeMethod(inInt, inStr, callStatus, outInt, outStr);}
                    );
    sleep(10);

    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);

    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        mainLoopForProxy_->runVerification(10, true, true); // run the mainloop again in order to give the syncCallThread a chance to return
    }
    mainLoopForStub_->stop();
    stubThread.join();
    syncCallThread.join();
}

class MainLoopThreadContext {
public:
    void setupRuntime(std::promise<bool>& p) {
        runtime_ = CommonAPI::Runtime::load();
        p.set_value(true);
    }

    void setupMainLoopContext(std::promise<bool>& p) {
        mainLoopContext_ = runtime_->getNewMainLoopContext();
        mainLoop_ = new CommonAPI::VerificationMainLoop(mainLoopContext_);
        p.set_value(true);
    }

    void setupFactory(std::promise<bool>& p) {
        factory_ = runtime_->createFactory(mainLoopContext_);
        servicePublisher_ = runtime_->getServicePublisher();
        p.set_value(true);
    }

    void setAddresses(const std::string own, const std::string other, const std::string thirdParty) {
        ownAddress_ = own;

        otherAddress_  = other;
        thirdPartyAddress_ = thirdParty;
    }

    void createProxyAndStub() {
        stub_ = std::make_shared<PingPongTestStub>();
        ASSERT_TRUE(servicePublisher_->registerService(stub_, ownAddress_, factory_));
        proxy_ = factory_->buildProxy<commonapi::tests::TestInterfaceProxy>(otherAddress_);
        ASSERT_TRUE((bool)proxy_);
        proxyThirdParty_ = factory_->buildProxy<commonapi::tests::TestInterfaceProxy>(thirdPartyAddress_);
        ASSERT_TRUE((bool)proxyThirdParty_);
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CommonAPI::MainLoopContext> mainLoopContext_;
    std::shared_ptr<CommonAPI::Factory> factory_;
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;
    std::string ownAddress_, otherAddress_, thirdPartyAddress_;
    std::shared_ptr<PingPongTestStub> stub_;
    std::shared_ptr<commonapi::tests::TestInterfaceProxy<>> proxy_, proxyThirdParty_;

    CommonAPI::VerificationMainLoop* mainLoop_;
};

class MainLoopIndependenceTest: public ::testing::Test {
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
                                        std::move(promiseContext1)));
        mainLoopThread2_ = std::thread(
                        std::bind(
                                        &MainLoopThreadContext::setupMainLoopContext,
                                        &threadCtx2_,
                                        std::move(promiseContext2)));
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

        mainLoopThread1_ = std::thread(std::bind(&MainLoopThreadContext::setupFactory, &threadCtx1_, std::move(promiseFactory1)));
        mainLoopThread2_ = std::thread(std::bind(&MainLoopThreadContext::setupFactory, &threadCtx2_, std::move(promiseFactory2)));

        mainLoopThread1_.detach();
        mainLoopThread2_.detach();

        futureFactory1.wait_for(std::chrono::milliseconds(200));
        futureFactory2.wait_for(std::chrono::milliseconds(200));

        // check that both threads have a factory and a service publisher
        ASSERT_TRUE((bool)threadCtx1_.factory_);
        ASSERT_TRUE((bool)threadCtx2_.factory_);
        ASSERT_TRUE((bool)threadCtx1_.servicePublisher_);
        ASSERT_TRUE((bool)threadCtx2_.servicePublisher_);

        // set addresses
        threadCtx1_.setAddresses(testAddress7, testAddress8, testAddress6);
        threadCtx2_.setAddresses(testAddress8, testAddress7, testAddress6);

        threadCtx1_.createProxyAndStub();
        threadCtx2_.createProxyAndStub();

        mainLoopThread1_ = std::thread([&]() { threadCtx1_.mainLoop_->run(); });
        mainLoopThread2_ = std::thread([&]() { threadCtx2_.mainLoop_->run(); });

        usleep(200000);

        ASSERT_TRUE(threadCtx1_.proxy_->isAvailable());
        ASSERT_TRUE(threadCtx2_.proxy_->isAvailable());

        threadCtx1_.mainLoop_->stop();
        threadCtx2_.mainLoop_->stop();

        mainLoopThread1_.join();
        mainLoopThread2_.join();
    }

    void TearDown() {
        threadCtx1_.servicePublisher_->unregisterService(testAddress6);
        threadCtx1_.servicePublisher_->unregisterService(testAddress7);
        threadCtx2_.servicePublisher_->unregisterService(testAddress8);
        usleep(2000);
        threadCtx1_.mainLoop_->stop();
        threadCtx2_.mainLoop_->stop();

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
TEST_F(MainLoopIndependenceTest, ProxyReceivesAnswerOnlyIfStubMainLoopRuns) {
    CommonAPI::CallStatus callStatus;

    uint32_t inInt, outInt;
    std::string inStr, outStr;
    inInt = 1;
    outInt = 0;

    std::thread mainLoopRunnerProxy([&]() { threadCtx1_.mainLoop_->runVerification(5, true, true); });
    mainLoopRunnerProxy.detach();

    mainLoopThread1_ = std::thread([&]() {  threadCtx1_.proxy_->testPredefinedTypeMethod(inInt, inStr, callStatus, outInt, outStr); });
    mainLoopThread1_.detach();

    sleep(1);
    // proxy should not receive answer, if the stub mainloop does not run
    ASSERT_EQ(0, outInt);

    mainLoopThread2_ = std::thread([&]() { threadCtx2_.mainLoop_->run(); });
    mainLoopThread2_.detach();

    sleep(1);

    // now the stub mainloop also runs, so the proxy should receive the answer
    ASSERT_EQ(1, outInt);
}

/**
* @test Proxy Receives Just His Own Answers.
* 	- start 2 proxies in own threads
* 	- call test method in each proxy
* 	- now each proxy should have received the answer to his own request
*/
TEST_F(MainLoopIndependenceTest, ProxyReceivesJustHisOwnAnswers) {
    std::shared_ptr<PingPongTestStub> stubThirdParty = std::make_shared<PingPongTestStub>();
    auto runtime = CommonAPI::Runtime::load();
    ASSERT_TRUE(runtime->getServicePublisher()->registerService(stubThirdParty, testAddress6, runtime->createFactory()));

    CommonAPI::CallStatus callStatusProxy1, callStatusProxy2;

    uint32_t inIntProxy1, outIntProxy1, inIntProxy2, outIntProxy2;
    std::string inStrProxy1, outStrProxy1, inStrProxy2, outStrProxy2;
    inIntProxy1 = 1;
    inIntProxy2 = 2;
    outIntProxy1 = outIntProxy2 = 0;

    std::thread mainLoopRunnerProxy1([&]() { threadCtx1_.mainLoop_->run(); });
    std::thread mainLoopRunnerProxy2([&]() { threadCtx2_.mainLoop_->run(); });
    mainLoopRunnerProxy1.detach();
    mainLoopRunnerProxy2.detach();

    while(!(threadCtx1_.proxyThirdParty_->isAvailable() && threadCtx2_.proxyThirdParty_->isAvailable())) {
        usleep(5000);
    }


    mainLoopThread1_ = std::thread([&]() {  threadCtx1_.proxyThirdParty_->testPredefinedTypeMethod(inIntProxy1, inStrProxy1, callStatusProxy1, outIntProxy1, outStrProxy1); });
    mainLoopThread2_ = std::thread([&]() {  threadCtx2_.proxyThirdParty_->testPredefinedTypeMethod(inIntProxy2, inStrProxy2, callStatusProxy2, outIntProxy2, outStrProxy2); });
    mainLoopThread1_.detach();
    mainLoopThread2_.detach();

    sleep(5);
    // now each proxy should have received the answer to his own request
    ASSERT_EQ(1, outIntProxy1);
    ASSERT_EQ(2, outIntProxy2);

    sleep(1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
