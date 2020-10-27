/* Copyright (C) 2016-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file DataTypes
*/

#include <functional>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <fstream>
#include <atomic>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"

#include "v1/commonapi/datatypes/deployment/TestInterfaceProxy.hpp"
#include "stub/DTDeploymentStub.hpp"

const std::string domain = "local";
const std::string testAddress = "commonapi.datatypes.deployment.TestInterface";
const std::string connectionIdService = "service-sample";
const std::string connectionIdClient = "client-sample";

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class DTDeployment: public ::testing::Test {

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStub_ = std::make_shared<v1_0::commonapi::datatypes::deployment::DTDeploymentStub>();
        serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, connectionIdService);
        ASSERT_TRUE(serviceRegistered_);

        testProxy_ = runtime_->buildProxy<v1_0::commonapi::datatypes::deployment::TestInterfaceProxy>(domain, testAddress, connectionIdClient);
        ASSERT_TRUE((bool)testProxy_);
        int i = 0;
        while(!testProxy_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        ASSERT_TRUE(
                runtime_->unregisterService(domain,
                        v1_0::commonapi::datatypes::deployment::DTDeploymentStub::StubInterface::getInterface(),
                        testAddress));

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 100 ) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    bool received_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<v1_0::commonapi::datatypes::deployment::TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<v1_0::commonapi::datatypes::deployment::DTDeploymentStub> testStub_;
};

/**
* @test Test Try to get noSubscription attribute deployed with GetterID=0 and NotifierID=0
* - Set value to attribute via stub
* - Set value to attribute via proxy
* - Check via stub that proxy set correct value
* - Try to get Attribute via proxy and make sure CallStatus::NOT_AVAILABLE is returned
*/
TEST_F(DTDeployment, TryGetNoSubsriptionAttributeWithGetterIDSetToZeroInDeployment) {

    CommonAPI::CallStatus callStatus;
    testStub_->setMyAttrGetterIsZeroSetterIsNotZeroNotifierIsZero_NoSubAttribute(4711);
    std::uint32_t testValue(9999);
    std::uint32_t resultValue(0);
    testProxy_->getMyAttrGetterIsZeroSetterIsNotZeroNotifierIsZero_NoSubAttribute().setValue(testValue, callStatus, resultValue);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
    EXPECT_EQ(testValue, resultValue);

    EXPECT_EQ(testValue, testStub_->getMyAttrGetterIsZeroSetterIsNotZeroNotifierIsZero_NoSubAttribute());

    // this isn't possible as method id is zero
    resultValue = 0;
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getMyAttrGetterIsZeroSetterIsNotZeroNotifierIsZero_NoSubAttribute().getValue(callStatus, resultValue);
    ASSERT_EQ(CommonAPI::CallStatus::NOT_AVAILABLE, callStatus);
    EXPECT_EQ(std::uint32_t(0), resultValue);
}

/**
* @test Test Try to get attribute deployed with GetterID=0
* - Subscribe to changed event of attribute
* - Set value to attribute via stub
* - Make sure subscription handler was called
* - Set value to attribute via proxy
* - Make sure subscription handler was called
* - Check via stub that proxy set correct value
* - Try to get Attribute via proxy and make sure CallStatus::NOT_AVAILABLE is returned
*/
TEST_F(DTDeployment, TryGetAttributeWithGetterIDSetToZeroInDeployment) {

    std::uint32_t testValue(4711);
    std::atomic<std::uint32_t> resultValueSubscription(0);
    CommonAPI::CallStatus callStatus;

    testProxy_->getMyAttrGetterIsZeroSetterIsNotZeroNotifierIsNotZeroAttribute().getChangedEvent().subscribe(
            [&](const std::uint32_t _value) {
                resultValueSubscription = _value;
            });

    testStub_->setMyAttrGetterIsZeroSetterIsNotZeroNotifierIsNotZeroAttribute(testValue);
    std::int32_t i(0);
    while(resultValueSubscription != testValue && i < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        i++;
    }
    EXPECT_EQ(testValue, resultValueSubscription);

    std::uint32_t testValueProxy(9999);
    std::uint32_t resultValue(0);
    resultValueSubscription = 0; // reset
    testProxy_->getMyAttrGetterIsZeroSetterIsNotZeroNotifierIsNotZeroAttribute().setValue(testValueProxy, callStatus, resultValue);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
    EXPECT_EQ(testValueProxy, resultValue);

    while(resultValueSubscription != testValueProxy && i < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        i++;
    }
    EXPECT_EQ(testValueProxy, resultValueSubscription);

    EXPECT_EQ(testValueProxy, testStub_->getMyAttrGetterIsZeroSetterIsNotZeroNotifierIsNotZeroAttribute());

    // this isn't possible as method id is zero
    resultValue = 0;
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getMyAttrGetterIsZeroSetterIsNotZeroNotifierIsNotZeroAttribute().getValue(callStatus, resultValue);
    ASSERT_EQ(CommonAPI::CallStatus::NOT_AVAILABLE, callStatus);
    EXPECT_EQ(std::uint32_t(0), resultValue);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
