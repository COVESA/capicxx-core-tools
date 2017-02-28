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
#include "stub/CMAttributesStub.h"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.communication.TestInterface";
const int tasync = 10000;

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

class CMAttributes: public ::testing::Test {

public:
    void recvValue(const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        value_ = y;
    }

    void recvSubscribedValue(uint8_t y) {
        value_ = y;
    }

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStub_ = std::make_shared<CMAttributesStub>();
        bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
        int i = 0;
        while(!testProxy_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        }
        ASSERT_TRUE(testProxy_->isAvailable());

        value_ = 0;
    }

    void TearDown() {
        bool serviceUnregistered =
                runtime_->unregisterService(domain, CMAttributesStub::StubInterface::getInterface(),
                        testAddress);

        ASSERT_TRUE(serviceUnregistered);

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 100 ) {
            std::this_thread::sleep_for(std::chrono::microseconds(tasync));
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    std::atomic<uint8_t> value_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<CMAttributesStub> testStub_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
};

/**
* @test Test synchronous getValue API function for attributes with combinations of
*  additional properties readonly and noSubscriptions (testAttribute,
*  testA readonly, testB noSubscriptions, testC readonly noSubscriptions).
*     - Set attribute to certain value on stub side.
*     - Call getValue.
*     - Check if returned call status is CommonAPI::CallStatus::SUCCESS.
*     - Check if value of is equal to expected value.
*/
TEST_F(CMAttributes, AttributeGetSynchronous) {

    CommonAPI::CallStatus callStatus;

    uint8_t x = 5;
    uint8_t y = 0;
    testStub_->setTestValues(x);
    testProxy_->getTestAttributeAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 6;
    y = 0;
    testStub_->setTestValues(x);
    testProxy_->getTestAAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 7;
    y = 0;
    testStub_->setTestValues(x);
    testProxy_->getTestBAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 8;
    y = 0;
    testStub_->setTestValues(x);
    testProxy_->getTestCAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

}

/**
* @test Test asynchronous getValue API function for attributes with combinations of
*  additional properties readonly and noSubscriptions (testAttribute,
*  testA readonly, testB noSubscriptions, testC readonly noSubscriptions).
*   - Set attribute to certain value on stub side.
*   - Call getValue.
*   - Check if returned call status is CommonAPI::CallStatus::SUCCESS.
*   - Check if value of is equal to expected value.
*/
TEST_F(CMAttributes, AttributeGetAsynchronous) {

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMAttributes::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    uint8_t x = 5;
    testStub_->setTestValues(x);
    testProxy_->getTestAttributeAttribute().getValueAsync(myCallback);
    for (int i = 0; i < 100; i++) {
        if (value_ == x) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(x, value_);

    x = 6;
    testStub_->setTestValues(x);
    testProxy_->getTestAttributeAttribute().getValueAsync(myCallback);
    for (int i = 0; i < 100; i++) {
        if (value_ == x) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(x, value_);

    x = 7;
    testStub_->setTestValues(x);
    testProxy_->getTestAttributeAttribute().getValueAsync(myCallback);
    for (int i = 0; i < 100; i++) {
        if (value_ == x) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(x, value_);

    x = 8;
    testStub_->setTestValues(x);
    testProxy_->getTestAttributeAttribute().getValueAsync(myCallback);
    for (int i = 0; i < 100; i++) {
        if (value_ == x) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(x, value_);
}

/**
* @test Test synchronous setValue API function for attributes with combinations of
*  additional properties readonly and noSubscriptions (testAttribute, testB noSubscriptions)
*   - Set attribute to certain value on proxy side.
*   - Check if returned call status is CommonAPI::CallStatus::SUCCESS.
*   - Check if returned value of setValue is equal to expected value.
*/
TEST_F(CMAttributes, AttributeSetSynchronous) {

    CommonAPI::CallStatus callStatus;

    uint8_t x = 5;
    uint8_t y = 0;
    testProxy_->getTestAttributeAttribute().setValue(x, callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 6;
    y = 0;
    testProxy_->getTestBAttribute().setValue(x, callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

}

/**
* @test Test asynchronous setValue API function for attributes with combinations of
*  additional properties readonly and noSubscriptions (testAttribute, testB noSubscriptions).
*   - Set attribute to certain value on proxy side.
*   - Check if returned call status is CommonAPI::CallStatus::SUCCESS.
*   - Check if returned value of setValue is equal to expected value.
*/
TEST_F(CMAttributes, AttributeSetAsynchronous) {

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMAttributes::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    testStub_->setTestValues(0);

    uint8_t x = 5;
    testProxy_->getTestAttributeAttribute().setValueAsync(x, myCallback);
    for (int i = 0; i < 100; i++) {
        if (value_ == x) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(x, value_);

    x = 6;
    testProxy_->getTestBAttribute().setValueAsync(x, myCallback);
    for (int i = 0; i < 100; i++) {
        if (value_ == x) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(x, value_);
}

/**
* @test Test subscription API function for attributes
*
*   - Subscribe on testAttribute.
*   - Set attribute to certain value on stub side.
*   - Do checks of call status (CommonAPI::CallStatus::SUCCESS) and returned value in callback function.
*   - Checks if returned value of setValue is equal to expected value.
*   - Set attribute to certain value with synchronous call from proxy.
*   - Check again.
*/
TEST_F(CMAttributes, AttributeSubscription) {

    CommonAPI::CallStatus callStatus;
    std::function<void (uint8_t)> myCallback =
            std::bind(&CMAttributes::recvSubscribedValue, this, std::placeholders::_1);

    testStub_->setTestValues(0);

    uint8_t y = 0;
    uint8_t x = 5;

    value_ = 0;

    testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);
    testStub_->setTestValues(x);
    for (int i = 0; i < 100; i++) {
        if (value_ == x) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(x, value_);

    x = 6;
    testProxy_->getTestAttributeAttribute().setValue(x, callStatus, y);
    for (int i = 0; i < 100; i++) {
        if (value_ == x) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(x, value_);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
