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
        values_.push_back(y);
    }

    void recvTimeout(const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
        timeoutsOccured_.push_back(true);
        (void)y;
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
        if(testProxy_) {
            while ( testProxy_->isAvailable() && counter < 10 ) {
                usleep(100000);
                counter++;
            }

            ASSERT_FALSE(testProxy_->isAvailable());
        }
        values_.clear();
        timeoutsOccured_.clear();
    }

    std::vector<uint8_t> values_;
    std::vector<bool> timeoutsOccured_;
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
*   - Test stub sets in-value of test method.
*   - Make asynchronous call of test method.
*   - Do checks of call status (CommonAPI::CallStatus::SUCCESS) and stored value in callback function.
*/
TEST_F(CMMethodCalls, AsynchronousMethodCall) {

    uint8_t x = 5;
    //uint8_t y = 0;

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMMethodCalls::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    testProxy_->testMethodAsync(x, myCallback);
    usleep(tasync);
    EXPECT_EQ(1u, values_.size());
    EXPECT_EQ(x, values_[0]);
}

/**
 * @test Call test method asynchronous and call test method synchronous in callback (nested).
 *   - Test stub sets in-values of test methods.
 *   - Make asynchronous call of test method.
 *   - Make asynchronous call of test method in callback (nested).
 *   - Do checks of call status (CommonAPI::CallStatus::SUCCESS) and stored values in callback functions.
 */
TEST_F(CMMethodCalls, NestedSynchronousMethodCall) {
    uint8_t x = 5;
    uint8_t x2 = 7;
    uint8_t y = 0;

    testProxy_->testMethodAsync(x, [&](const CommonAPI::CallStatus& _callStatus, uint8_t _y) {
        EXPECT_EQ(_callStatus, CommonAPI::CallStatus::SUCCESS);
        values_.push_back(_y);

        CommonAPI::CallStatus status;
        testProxy_->testMethod(x2, status, y);
        EXPECT_EQ(status, CommonAPI::CallStatus::SUCCESS);
        EXPECT_EQ(x2, y);
    });

    usleep(tasync * 2);
    EXPECT_EQ(1u, values_.size());
    EXPECT_EQ(x, values_[0]);
}

/**
 * @test Call test method asynchronous and call test method asynchronous in callback (nested).
 *   - Test stub sets in-values of test methods.
 *   - Make asynchronous call of test method.
 *   - Make asynchronous call of test method in callback (nested).
 *   - Do checks of call status (CommonAPI::CallStatus::SUCCESS) and stored values in callback functions.
 */
TEST_F(CMMethodCalls, NestedAsynchronousMethodCall) {

    uint8_t x = 5;
    uint8_t x2 = 7;

    testProxy_->testMethodAsync(x, [this, &x2](const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        values_.push_back(y);
        testProxy_->testMethodAsync(x2, [this](const CommonAPI::CallStatus& callStatus, uint8_t y) {
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
            values_.push_back(y);
        });
    });

    usleep(tasync * 2);
    EXPECT_EQ(2u, values_.size());
    EXPECT_EQ(x, values_[0]);
    EXPECT_EQ(x2, values_[1]);
}

/**
 * @test Call test method asynchronous when proxy is not available.
 *   - Unregister service.
 *   - Wait that proxy is not available.
 *   - Test stub sets in-value of test method.
 *   - Set timeout of asynchronous call.
 *   - Make asynchronous call of test method.
 *   - Do checks of call status (CommonAPI::CallStatus::NOT_AVAILABLE) and that timeout occurred.
 */
TEST_F(CMMethodCalls, AsynchronousMethodCallProxyNotAvailable) {
    runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress);

    int counter = 0;
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    CommonAPI::CallInfo info(2000);

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMMethodCalls::recvTimeout, this, std::placeholders::_1, std::placeholders::_2);

    testProxy_->testMethodAsync(x, myCallback, &info);

    int t=0;
    while(timeoutsOccured_.size() == 0 && t <= 8) {
        usleep(tasync * 10);
        t++;
    }
    EXPECT_EQ(1u, timeoutsOccured_.size());
    ASSERT_TRUE(timeoutsOccured_[0]);
}

/**
 *  @test Call test method asynchronous and call test method asynchronous in callback (nested)
 *        when proxy is not available.
 *    - Unregister service.
 *    - Wait that proxy is not available.
 *    - Test stub sets in-value of test methods.
 *    - Set timeout of asynchronous calls.
 *    - Make asynchronous call of test method.
 *    - Make asynchronous call of test method in callback (nested).
 *    - Do checks of call status (CommonAPI::CallStatus::NOT_AVAILABLE) and that timeouts occurred.
 */
TEST_F(CMMethodCalls, NestedAsynchronousMethodCallProxyNotAvailable) {
    runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress);

    int counter = 0;
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    CommonAPI::CallInfo info(2000);

    testProxy_->testMethodAsync(x, [this, &info, &x](const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
        timeoutsOccured_.push_back(true);
        (void)y;
        testProxy_->testMethodAsync(x, [this](const CommonAPI::CallStatus& callStatus, uint8_t y) {
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
            timeoutsOccured_.push_back(true);
            (void)y;
        }, &info);
    }, &info);

    int t=0;
    while(timeoutsOccured_.size() < 2 && t <= 8) {
        usleep(tasync * 10);
        t++;
    }
    EXPECT_EQ(2u, timeoutsOccured_.size());
    ASSERT_TRUE(timeoutsOccured_[0]);
    ASSERT_TRUE(timeoutsOccured_[1]);
}

/**
 * @test Call test method asynchronous when proxy is not available. Proxy becomes available
 *       during call.
 *   - Unregiser service
 *   - Wait that proxy is not available.
 *   - Test stub sets in-value of test method.
 *   - Set timeout of asynchronous call.
 *   - Make asynchronous call of test method.
 *   - Proxy becomes available during call.
 *   - Do checks of call status (CommonAPI::CallStatus::SUCCESS) and stored value in callback function.
 */
TEST_F(CMMethodCalls, AsynchronousMethodCallProxyBecomesAvailable) {
    runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress);

    int counter = 0;
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    CommonAPI::CallInfo info(2000);

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMMethodCalls::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    testProxy_->testMethodAsync(x, myCallback, &info);

    usleep(tasync * 5);
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    usleep(tasync);
    EXPECT_EQ(1u, values_.size());
    EXPECT_EQ(x, values_[0]);
}

/**
 * @test Call test method asynchronous and call test method asynchronous in callback (nested)
 *       when proxy is not available. Proxy becomes available during call.
 *   - Unregiser service
 *   - Wait that proxy is not available.
 *   - Test stub sets in-values of test methods.
 *   - Set timeout of asynchronous calls.
 *   - Make asynchronous call of test method.
 *   - Make asynchronous call of test method in callback (nested).
 *   - Proxy becomes available during first async call.
 *   - Do checks of call status (CommonAPI::CallStatus::SUCCESS) and stored value in callback functions.
 */
TEST_F(CMMethodCalls, NestedAsynchronousMethodCallProxyBecomesAvailable) {
    runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress);

    int counter = 0;
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    uint8_t x2 = 7;
    CommonAPI::CallInfo info(2000);

    testProxy_->testMethodAsync(x, [this,&x2,&info](const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        values_.push_back(y);
        testProxy_->testMethodAsync(x2, [this](const CommonAPI::CallStatus& callStatus, uint8_t y) {
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
            values_.push_back(y);
        }, &info);
    }, &info);

    usleep(tasync * 5);
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    usleep(tasync);
    EXPECT_EQ(2u, values_.size());
    EXPECT_EQ(x, values_[0]);
    EXPECT_EQ(x2, values_[1]);
}

/**
 * @test Call test method asynchronous multiple times when proxy is not available. Proxy becomes available
 *       during call
 *   - Unregiser service
 *   - Wait that proxy is not available
 *   - Test stub set in-value of test methods.
 *   - Set timeouts of asynchronous calls (timeouts that are reached and timeouts that are not reached).
 *   - Make asynchronous calls of test method (2 expected timeouts, 3 successful calls).
 *   - Proxy becomes available during call
 *   - Do checks of call status (CommonAPI::CallStatus::SUCCESS and CommonAPI::CallStatus::NOT_AVAILABLE for expected timeouts),
 *     stored values and timeouts that occurred in callback functions.
 */
TEST_F(CMMethodCalls, AsynchronousMethodCallsProxyBecomesAvailable) {
    runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress);

    int counter = 0;
    while ( testProxy_->isAvailable() && counter < 10 ) {
        usleep(100000);
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    CommonAPI::CallInfo info(100);
    CommonAPI::CallInfo info2(500);
    CommonAPI::CallInfo info3(2000);
    CommonAPI::CallInfo info4(2000);
    CommonAPI::CallInfo info5(2000);

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> recvValueCallback =
            std::bind(&CMMethodCalls::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> recvTimeoutCallback =
            std::bind(&CMMethodCalls::recvTimeout, this, std::placeholders::_1, std::placeholders::_2);

    //expected timeouts during call
    testProxy_->testMethodAsync(x, recvTimeoutCallback, &info);
    testProxy_->testMethodAsync(x, recvTimeoutCallback, &info2);

    //expected successful calls
    testProxy_->testMethodAsync(x, recvValueCallback, &info3);
    testProxy_->testMethodAsync(x, recvValueCallback, &info4);
    testProxy_->testMethodAsync(x, recvValueCallback, &info5);

    usleep(tasync * 6);
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    usleep(tasync);

    EXPECT_EQ(3u, values_.size());
    auto it_values = values_.begin();
    while(it_values != values_.end()) {
        EXPECT_EQ(x, *it_values);
        ++it_values;
    }

    EXPECT_EQ(2u, timeoutsOccured_.size());
    auto it_timeouts = values_.begin();
    while(it_timeouts != values_.end()) {
        EXPECT_EQ(x, *it_timeouts);
        ++it_timeouts;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
