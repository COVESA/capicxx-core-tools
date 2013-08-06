/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.h"
#include "VerificationMainLoop.h"
#include "commonapi/tests/TestInterfaceProxy.h"
#include "VerificationTestStub.h"
#include <functional>

const std::string testAddress8 = "local:my.eigth.test:commonapi.address.eight";

class MainLoopTest: public ::testing::Test {

protected:
    void SetUp() {

        runtime_ = CommonAPI::Runtime::load();
        ASSERT_TRUE((bool) runtime_);

        contextProxy_ = runtime_->getNewMainLoopContext();
        contextStub_ = runtime_->getNewMainLoopContext();
        ASSERT_TRUE((bool) contextProxy_);
        ASSERT_TRUE((bool) contextStub_);
        ASSERT_FALSE(contextProxy_ == contextStub_);

        mainLoopProxy_ = new CommonAPI::VerificationMainLoop(contextProxy_);
        mainLoopStub_ = new CommonAPI::VerificationMainLoop(contextStub_);

        mainloopFactoryProxy_ = runtime_->createFactory(contextProxy_);
        mainloopFactoryStub_ = runtime_->createFactory(contextStub_);
        ASSERT_TRUE((bool) mainloopFactoryProxy_);
        ASSERT_TRUE((bool) mainloopFactoryStub_);
        ASSERT_FALSE(mainloopFactoryProxy_ == mainloopFactoryStub_);

        servicePublisher_ = runtime_->getServicePublisher();
        ASSERT_TRUE((bool) servicePublisher_);
        callbackCalled = 0;
        lastBroadcastNumber = 0;
        outInt = 0;
    }

    void TearDown() {
        delete mainLoopProxy_;
        delete mainLoopStub_;
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<CommonAPI::MainLoopContext> contextProxy_;
    std::shared_ptr<CommonAPI::MainLoopContext> contextStub_;
    std::shared_ptr<CommonAPI::Factory> mainloopFactoryProxy_;
    std::shared_ptr<CommonAPI::Factory> mainloopFactoryStub_;
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher_;

    CommonAPI::VerificationMainLoop* mainLoopProxy_;
    CommonAPI::VerificationMainLoop* mainLoopStub_;

    int callbackCalled;
    int lastBroadcastNumber;
    uint32_t outInt;

public:
    void broadcastCallback(uint32_t intValue, std::string stringValue) {
        // check correct order
        lastBroadcastNumber++;
        ASSERT_EQ(lastBroadcastNumber, intValue);
        // check, if broadcast is handled after method call
        ASSERT_EQ(outInt, 1);
    }
};

TEST_F(MainLoopTest, VerifyTransportReadingWhenDispatchingWatches) {
    std::shared_ptr<commonapi::verification::VerificationTestStub> stub = std::make_shared<commonapi::verification::VerificationTestStub>();
    ASSERT_TRUE(servicePublisher_->registerService(stub, testAddress8, mainloopFactoryStub_));

    auto proxy = mainloopFactoryProxy_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress8);
    ASSERT_TRUE((bool) proxy);

    std::thread stubThread = std::thread([&](){ mainLoopStub_->run(); });
    stubThread.detach();

    while (!proxy->isAvailable()) {
        mainLoopProxy_->doSingleIteration();
        usleep(50000);
    }

    ASSERT_TRUE(proxy->isAvailable());

    mainLoopStub_->stop();


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
                                        mainLoopProxy_->stop();
                                        callbackCalled++;
                                    }
                                    );

    mainLoopProxy_->runVerification(15, true, true);

    // 1. just dispatch dispatchSources
    mainLoopStub_->runVerification(15, false, true);
    ASSERT_EQ(stub->getCalledTestDerivedTypeMethod(), 0);

    // 2. just dispatch watches (reads transport)
    mainLoopStub_->runVerification(20, true, false);
    ASSERT_EQ(stub->getCalledTestDerivedTypeMethod(), 0);

    // 3. just dispatch dispatchSources again. This should dispatch the messages already read from transport in 2.
    mainLoopStub_->doVerificationIteration(false, true);
    ASSERT_EQ(stub->getCalledTestDerivedTypeMethod(), 1);

    servicePublisher_->unregisterService(testAddress8);
}

TEST_F(MainLoopTest, VerifySyncCallMessageHandlingOrder) {
    std::shared_ptr<commonapi::verification::VerificationTestStub> stub = std::make_shared<commonapi::verification::VerificationTestStub>();
    ASSERT_TRUE(servicePublisher_->registerService(stub, testAddress8, mainloopFactoryStub_));

    auto proxy = mainloopFactoryProxy_->buildProxy<commonapi::tests::TestInterfaceProxy>(testAddress8);
    ASSERT_TRUE((bool) proxy);

    std::thread stubThread = std::thread([&](){ mainLoopStub_->run(); });
    stubThread.detach();

    std::thread proxyThread = std::thread([&](){ mainLoopProxy_->run(); });
    proxyThread.detach();

    while (!proxy->isAvailable()) {
        usleep(50000);
    }

    ASSERT_TRUE(proxy->isAvailable());

    auto& broadcastEvent = proxy->getTestPredefinedTypeBroadcastEvent();
    broadcastEvent.subscribe(std::bind(&MainLoopTest::broadcastCallback, this, std::placeholders::_1, std::placeholders::_2));

    CommonAPI::CallStatus callStatus;
    std::string outString;

    proxy->testPredefinedTypeMethod(0, "", callStatus, outInt, outString);
    ASSERT_EQ(outInt, 1);

    sleep(10);

    mainLoopProxy_->stop();
    mainLoopStub_->stop();

    // in total 5 broadcasts should have been arrived
    ASSERT_EQ(lastBroadcastNumber, 5);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
