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
const std::string testAddress2 = "commonapi.communication.TestInterface2";
const int tasync = 20000;
const int timeout = 300;
const int maxTimeoutCalls = 10;

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

    CMMethodCalls() :
        timeoutCalls_(0)
    {

    }

    void recvValue(const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        values_.push_back(y);
    }

    void recvTimeout(const CommonAPI::CallStatus& callStatus, uint8_t y) {
        std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
        timeoutsOccured_.push_back(true);
        (void)y;
    }

    void timeoutCallback(const CommonAPI::CallStatus& callStatus, uint8_t result) {
        (void)result;
        std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::REMOTE_ERROR);
        timeoutsOccured_.push_back(true);
        CommonAPI::CallInfo callInfo(timeout);
        if(timeoutCalls_ < maxTimeoutCalls) {
            testProxy_->testMethodTimeoutAsync(
                    std::bind(
                            &CMMethodCalls::timeoutCallback,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                    &callInfo);
            timeoutCalls_++;
        }
    }

    void timeoutCallback2(const CommonAPI::CallStatus& callStatus, uint8_t result) {
        (void)result;
        std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::REMOTE_ERROR);
        timeoutsOccured_.push_back(true);
        CommonAPI::CallInfo callInfo(timeout);
        if(timeoutCalls_ < maxTimeoutCalls) {
            testProxy2_->testMethodTimeoutAsync(
                    std::bind(
                            &CMMethodCalls::timeoutCallback2,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                    &callInfo);
            timeoutCalls_++;
        }
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

        if(testStub2_)
            runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress2);

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        if(testProxy_) {
            while ( testProxy_->isAvailable() && counter < 100 ) {
                std::this_thread::sleep_for(std::chrono::microseconds(10000));
                counter++;
            }

            ASSERT_FALSE(testProxy_->isAvailable());
        }

        counter = 0;
        if(testProxy2_) {
            while ( testProxy2_->isAvailable() && counter < 100 ) {
                std::this_thread::sleep_for(std::chrono::microseconds(10000));
                counter++;
            }

            ASSERT_FALSE(testProxy2_->isAvailable());
        }
        values_.clear();

        std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
        timeoutsOccured_.clear();
    }

    std::vector<uint8_t> values_;

    std::mutex timeoutsMutex_;
    std::vector<bool> timeoutsOccured_;
    uint8_t timeoutCalls_;

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<TestInterfaceStub> testStub_;
    std::shared_ptr<TestInterfaceStub> testStub2_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy2_;
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
    std::this_thread::sleep_for(std::chrono::microseconds(tasync));
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

    testProxy_->testMethodAsync(x, [&](const CommonAPI::CallStatus& _callStatus, uint8_t _y) {
        EXPECT_EQ(_callStatus, CommonAPI::CallStatus::SUCCESS);
        values_.push_back(_y);

        uint8_t y = 0;
        uint8_t x2 = 7;
        CommonAPI::CallStatus status;
        testProxy_->testMethod(x2, status, y);
        EXPECT_EQ(status, CommonAPI::CallStatus::SUCCESS);
        EXPECT_EQ(x2, y);
    });

    for (int i = 0; i < 100; ++i) {
        if (values_.size()) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(1u, values_.size());
    if (values_.size()) {
        EXPECT_EQ(x, values_[0]);
    }
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

    for (int i = 0; i < 100; ++i) {
        if (values_.size() == 2) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(2u, values_.size());
    if (values_.size() == 2) {
        EXPECT_EQ(x, values_[0]);
        EXPECT_EQ(x2, values_[1]);
    }
}

/**
 * @test Call test method timeout asynchronous and call test method timeout asynchronous in callback (nested).
 *   - Register second service with other instance
 *   - Create second proxy to second service
 *   - Make asynchronous call of test method timeout (first proxy)
 *   - Make asynchronous call of test method timeout (second proxy)
 *   - Check in callbacks if timeout occured (CommonAPI::CallStatus::REMOTE_ERROR)
 *   - Make asynchronous calls of test method timeout in callbacks as long as timeoutCalls_ < maxTimeoutCalls_ (nested).
 *   - Check if the same amount of timeouts occured as async calls were done
 */
TEST_F(CMMethodCalls, NestedAsynchronousMethodCallsTimedOut) {

    testStub2_ = std::make_shared<CMMethodCallsStub>();
    bool serviceRegistered = runtime_->registerService(domain, testAddress2, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy2_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress2, clientId);
    ASSERT_TRUE((bool)testProxy2_);

    testProxy2_->isAvailableBlocking();
    ASSERT_TRUE(testProxy2_->isAvailable());

    timeoutsMutex_.lock();

    CommonAPI::CallInfo callInfo(timeout);
    testProxy_->testMethodTimeoutAsync(
            std::bind(
                    &CMMethodCalls::timeoutCallback,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2),
            &callInfo);
    timeoutCalls_++;

    testProxy2_->testMethodTimeoutAsync(
            std::bind(
                    &CMMethodCalls::timeoutCallback2,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2),
            &callInfo);
    timeoutCalls_++;

    uint8_t t=0;
    while(timeoutsOccured_.size() < timeoutCalls_ && t <= timeoutCalls_) {
        timeoutsMutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(timeout * 1000 + 500 * 1000));
        timeoutsMutex_.lock();
        t++;
    }

    EXPECT_EQ(timeoutCalls_, timeoutsOccured_.size());
    timeoutsMutex_.unlock();
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
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    CommonAPI::CallInfo info(100);

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMMethodCalls::recvTimeout, this, std::placeholders::_1, std::placeholders::_2);

    testProxy_->testMethodAsync(x, myCallback, &info);

    int t=0;
    timeoutsMutex_.lock();
    while(timeoutsOccured_.size() == 0 && t <= 8) {
        timeoutsMutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(tasync * 4));
        timeoutsMutex_.lock();
        t++;
    }

    EXPECT_EQ(1u, timeoutsOccured_.size());
    ASSERT_TRUE(timeoutsOccured_[0]);
    timeoutsMutex_.unlock();
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
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    CommonAPI::CallInfo info(100);

    testProxy_->testMethodAsync(x, [this, &info, &x](const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
        std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
        timeoutsOccured_.push_back(true);
        (void)y;
        testProxy_->testMethodAsync(x, [this](const CommonAPI::CallStatus& callStatus, uint8_t y) {
            std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
            timeoutsOccured_.push_back(true);
            (void)y;
        }, &info);
    }, &info);

    int t=0;
    timeoutsMutex_.lock();
    while(timeoutsOccured_.size() < 2 && t <= 8) {
        timeoutsMutex_.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(tasync * 4));
        timeoutsMutex_.lock();
        t++;
    }

    EXPECT_EQ(2u, timeoutsOccured_.size());
    ASSERT_TRUE(timeoutsOccured_[0]);
    ASSERT_TRUE(timeoutsOccured_[1]);
    timeoutsMutex_.unlock();
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
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    CommonAPI::CallInfo info(1000);

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMMethodCalls::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    testProxy_->testMethodAsync(x, myCallback, &info);

    std::this_thread::sleep_for(std::chrono::microseconds(tasync * 5));
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    for (int i = 0; i < 100; ++i) {
        if (values_.size()) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(1u, values_.size());
    if (values_.size())
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
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    uint8_t x2 = 7;
    CommonAPI::CallInfo info(1000);

    testProxy_->testMethodAsync(x, [this,&x2,&info](const CommonAPI::CallStatus& callStatus, uint8_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        values_.push_back(y);
        testProxy_->testMethodAsync(x2, [this](const CommonAPI::CallStatus& callStatus, uint8_t y) {
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
            values_.push_back(y);
        }, &info);
    }, &info);

    std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    for (int i = 0; i < 100; ++i) {
        if (values_.size() == 2) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(2u, values_.size());
    if (values_.size() == 2) {
        EXPECT_EQ(x, values_[0]);
        EXPECT_EQ(x2, values_[1]);
    }
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
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    CommonAPI::CallInfo info(100);
    CommonAPI::CallInfo info2(100);
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

    std::this_thread::sleep_for(std::chrono::microseconds(tasync * 10));
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    for (int i = 0; i < 100; ++i) {
        if (values_.size() == 3) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }

    EXPECT_EQ(3u, values_.size());
    auto it_values = values_.begin();
    while(it_values != values_.end()) {
        EXPECT_EQ(x, *it_values);
        ++it_values;
    }

    for (int i = 0; i < 100; ++i) {
        if (timeoutsOccured_.size() == 2) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
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
