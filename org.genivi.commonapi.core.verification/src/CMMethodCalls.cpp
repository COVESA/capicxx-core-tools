/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Communication
*/

#include <functional>
#include <fstream>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/communication/TestInterfaceProxy.hpp"
#include "stub/CMMethodCallsStub.h"

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

class CMMethodCalls: public ::testing::Test {

public:
    void recvValue(const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        value_ = y;
    }

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStub_ = std::make_shared<CMMethodCallsStub>();
        bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
        ASSERT_TRUE((bool)testProxy_);

        testProxy_->isAvailableBlocking();
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress);

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
    std::shared_ptr<TestInterfaceStub> testStub_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
};

/**
* @test Call test method synchronous and check call status.
*     - Test stub sets in-value of test method equal out-value of test method.
*     - Make synchronous call of test method.
*     - Check if returned call status is CommonAPI::CallStatus::SUCCESS.
*     - Check if out value of test method is equal to in value.
*/
TEST_F(CMMethodCalls, SynchronousMethodCall) {

    uint8_t x = 5;
    uint8_t y = 0;
    CommonAPI::CallStatus callStatus;
    testProxy_->testMethod(x, callStatus, y);

    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

}

/**
* @test Call test method asynchronous and check call status.
*   - Test stub sets in-value of test method equal out-value of test method.
*   - Make asynchronous call of test method.
*   - Do checks of call status (CommonAPI::CallStatus::SUCCESS) and returned value in callback function.
*/
TEST_F(CMMethodCalls, AsynchronousMethodCall) {

    uint8_t x = 5;
    //uint8_t y = 0;

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMMethodCalls::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    testProxy_->testMethodAsync(x, myCallback);
    usleep(tasync);
    EXPECT_EQ(x, value_);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
