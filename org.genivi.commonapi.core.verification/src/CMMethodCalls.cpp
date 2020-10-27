/* Copyright (C) 2014-2019 BMW Group
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
#include "stub/CMMethodCallsStub.hpp"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.communication.TestInterface";
const std::string testAddress2 = "commonapi.communication.TestInterface2";
const int tasync = 20000;
const int timeout = 300;
const int maxTimeoutCalls = 10;

#ifdef TESTS_BAT
const unsigned int wf = 10; /* "wait-factor" when run in BAT environment */
#else
const unsigned int wf = 1;
#endif

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
        EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
        std::lock_guard<std::mutex> itsLock(values_mutex_);
        values_.push_back(y);
    }

    void recvTimeout(const CommonAPI::CallStatus& callStatus, uint8_t y) {
        std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
        EXPECT_EQ(CommonAPI::CallStatus::NOT_AVAILABLE, callStatus);
        timeoutsOccured_.push_back(true);
        (void)y;
    }

    void timeoutCallback(const CommonAPI::CallStatus& callStatus, uint8_t result) {
        (void)result;
        {
            std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::REMOTE_ERROR);
            timeoutsOccured_.push_back(true);
        }
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
        {
            std::lock_guard<std::mutex> timeoutsLock(timeoutsMutex_);
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::REMOTE_ERROR);
            timeoutsOccured_.push_back(true);
        }
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

        int counter = 0;
        while (!testProxy_->isAvailable() && 100 > counter++) {
            std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
        }
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

    std::mutex values_mutex_;
    std::vector<uint8_t> values_;

    std::mutex timeoutsMutex_;
    std::vector<bool> timeoutsOccured_;
    std::atomic<uint8_t> timeoutCalls_;

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
* @test Call fire and forget method and check via broadcast that value was received.
*     - Subscribe to broadcast
*     - Check that broadcast subscription succeeded
*     - Make fire and forget method call
*     - Check via broadcast that value was correctly reveived (Stub fires broadcast when
*       value was received.
*/
TEST_F(CMMethodCalls, FireAndForget) {
    
    std::atomic<uint8_t> result;
    std::atomic<CommonAPI::CallStatus> subStatus(CommonAPI::CallStatus::UNKNOWN);
    testProxy_->getBTestEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });

    // check that subscription has succeeded
    for (int i = 0; i < 100; i++) {
        if (subStatus == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);


    uint8_t x = 5;
    CommonAPI::CallStatus callStatus;
    result = 0;
    callStatus = CommonAPI::CallStatus::REMOTE_ERROR;
    testProxy_->testDontCare(x, callStatus);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check via broadcast that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);
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
    std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
    {
        std::lock_guard<std::mutex> itsLock(values_mutex_);
        EXPECT_EQ(1u, values_.size());
        if (values_.size()) {
            EXPECT_EQ(x, values_[0]);
        } else {
            ADD_FAILURE() << "Async callback was not called";
        }
    }
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

        uint8_t y = 0;
        uint8_t x2 = 7;
        CommonAPI::CallStatus status;
        testProxy_->testMethod(x2, status, y);
        EXPECT_EQ(status, CommonAPI::CallStatus::SUCCESS);
        EXPECT_EQ(x2, y);
        {
            std::lock_guard<std::mutex> itsLock(values_mutex_);
            values_.push_back(_y);
        }
    });

    for (int i = 0; i < 100; ++i) {
        {
            std::lock_guard<std::mutex> itsLock(values_mutex_);
            if (values_.size()) break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    {
        std::lock_guard<std::mutex> itsLock(values_mutex_);
        EXPECT_EQ(1u, values_.size());
        if (values_.size()) {
            EXPECT_EQ(x, values_[0]);
        }
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
        {
            std::lock_guard<std::mutex> itsLock(values_mutex_);
            values_.push_back(y);
        }
        testProxy_->testMethodAsync(x2, [this](const CommonAPI::CallStatus& callStatus, uint8_t y) {
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
            {
                std::lock_guard<std::mutex> itsLock(values_mutex_);
                values_.push_back(y);
            }
        });
    });

    for (int i = 0; i < 100; ++i) {
        {
            std::lock_guard<std::mutex> itsLock(values_mutex_);
            if (values_.size() == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    {
        std::lock_guard<std::mutex> itsLock(values_mutex_);
        EXPECT_EQ(2u, values_.size());
        if (values_.size() == 2) {
            EXPECT_EQ(x, values_[0]);
            EXPECT_EQ(x2, values_[1]);
        }
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

    int counter = 0;
    while (!testProxy2_->isAvailable() && 100 > counter++) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
    }
    ASSERT_TRUE(testProxy2_->isAvailable());


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

    timeoutsMutex_.lock();
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
    CommonAPI::CallInfo info(1000 * wf);

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMMethodCalls::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    testProxy_->testMethodAsync(x, myCallback, &info);

    std::this_thread::sleep_for(std::chrono::microseconds(tasync * 5 * wf));
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    counter = 0;
    while (!testProxy_->isAvailable() && 100 > counter++) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    for (int i = 0; i < 100; ++i) {
        {
            std::lock_guard<std::mutex> itsLock(values_mutex_);
            if (values_.size()) break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    {
        std::lock_guard<std::mutex> itsLock(values_mutex_);
        EXPECT_EQ(1u, values_.size());
        if (values_.size()) {
            EXPECT_EQ(x, values_[0]);
        }
    }
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
        {
            std::lock_guard<std::mutex> itsLock(values_mutex_);
            values_.push_back(y);
        }
        testProxy_->testMethodAsync(x2, [this](const CommonAPI::CallStatus& callStatus, uint8_t y) {
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
            {
                std::lock_guard<std::mutex> itsLock(values_mutex_);
                values_.push_back(y);
            }
        }, &info);
    }, &info);

    std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    counter = 0;
    while (!testProxy_->isAvailable() && 100 > counter++) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    for (int i = 0; i < 100; ++i) {
        {
            std::lock_guard<std::mutex> itsLock(values_mutex_);
            if (values_.size() == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    {
        std::lock_guard<std::mutex> itsLock(values_mutex_);
        EXPECT_EQ(2u, values_.size());
        if (values_.size() == 2) {
            EXPECT_EQ(x, values_[0]);
            EXPECT_EQ(x2, values_[1]);
        }
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
    CommonAPI::CallInfo info3(tasync/10*wf);
    CommonAPI::CallInfo info4(tasync/10*wf);
    CommonAPI::CallInfo info5(tasync/10*wf);

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

    std::this_thread::sleep_for(std::chrono::microseconds(tasync * 10 * wf));
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    counter = 0;
    while (!testProxy_->isAvailable() && 100 > counter++) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    for (int i = 0; i < 100; ++i) {
        {
            std::lock_guard<std::mutex> itsLock(values_mutex_);
            if (values_.size() == 3) break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    {
        std::lock_guard<std::mutex> itsLock(values_mutex_);
        EXPECT_EQ(3u, values_.size());
        auto it_values = values_.begin();
        while(it_values != values_.end()) {
            EXPECT_EQ(x, *it_values);
            ++it_values;
        }
    }

    for (int i = 0; i < 100; ++i) {
        if (timeoutsOccured_.size() == 2) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }

    EXPECT_EQ(2u, timeoutsOccured_.size());
    {
        std::lock_guard<std::mutex> itsLock(values_mutex_);
        auto it_timeouts = values_.begin();
        while(it_timeouts != values_.end()) {
            EXPECT_EQ(x, *it_timeouts);
            ++it_timeouts;
        }
    }
}

#ifndef TESTS_BAT

/**
 * @test Call test method asynchronous when proxy is not available and delete proxy.
 *   - Unregister service.
 *   - Wait that proxy is not available.
 *   - Test stub sets in-value of test method.
 *   - Set timeout of asynchronous call.
 *   - Make asynchronous call of test method.
 *   - Start thread which deletes the proxy.
 *   - Check if proxy could be deleted.
 *   - Join created thread.
 */
TEST_F(CMMethodCalls, AsynchronousMethodCallProxyNotAvailableDeleteProxy) {
    runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress);

    int counter = 0;
    while ( testProxy_->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        counter++;
    }
    ASSERT_FALSE(testProxy_->isAvailable());

    uint8_t x = 5;
    int timeout = 1000;
    CommonAPI::CallInfo info(timeout);

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            std::bind(&CMMethodCalls::recvTimeout, this, std::placeholders::_1, std::placeholders::_2);

    testProxy_->testMethodAsync(x, myCallback, &info);
    std::future<void> proxyCompletionFuture = testProxy_->getCompletionFuture();
    std::mutex m;
    std::promise<bool> p;
    auto future = p.get_future();

    std::thread deleteThread([&] {
        testProxy_.reset();
        std::lock_guard<std::mutex> itsLock(m);
        p.set_value(true);
    });

    auto status = future.wait_for(std::chrono::milliseconds(timeout / 2));
    EXPECT_EQ(std::future_status::ready, status);
    if (status == std::future_status::ready) {
        std::lock_guard<std::mutex> itsLock(m);
        auto result = future.get();
        ASSERT_TRUE(result);
    } else {
        ADD_FAILURE() << "Future wasn't ready";
    }

    if (std::future_status::timeout == proxyCompletionFuture.wait_for(std::chrono::seconds(5))) {
        ADD_FAILURE() << "Proxy wasn't destroyed within time";
    }

    if(deleteThread.joinable()) {
        deleteThread.join();
    }
}

/**
* @test Call test method via two proxies multiple times asynchronously while the
* service is unavailable and check if the provided callback is called with an
* error for every method call done.
*/

TEST_F(CMMethodCalls, AsynchronousMethodCallsReceiveNotAvailable) {
    // build second proxy to same service
    testProxy2_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
    ASSERT_TRUE((bool)testProxy2_);

    int counter = 0;
    while (!testProxy2_->isAvailable() && 100 > counter++) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync*wf));
    }
    ASSERT_TRUE(testProxy2_->isAvailable());

    runtime_->unregisterService(domain,
            CMMethodCallsStub::StubInterface::getInterface(), testAddress);

    for (int counter = 0; testProxy_->isAvailable() && counter < 100; counter++) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    ASSERT_FALSE(testProxy_->isAvailable());
    for (int counter = 0; testProxy2_->isAvailable() && counter < 100; counter++) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    ASSERT_FALSE(testProxy2_->isAvailable());

    std::uint32_t numberCalls(250);
    std::atomic<std::uint32_t> numberCallbackCalled(0);
    std::atomic<std::uint32_t> numberCallbackCalled2(0);
    std::promise<bool> p1, p2;


    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
            [&](const CommonAPI::CallStatus& callStatus, uint8_t y) {
        (void)y;
        numberCallbackCalled++;
        EXPECT_EQ(CommonAPI::CallStatus::NOT_AVAILABLE, callStatus);
        if (numberCallbackCalled == numberCalls) {
            try {
                p1.set_value(true);
            } catch (std::future_error &e) {
                ADD_FAILURE() << "myCallBack catched exception" << e.what()
                        << " " << std::dec << numberCallbackCalled;
            }
        }
    };

    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback2 =
            [&](const CommonAPI::CallStatus& callStatus, uint8_t y) {
        (void)y;
        numberCallbackCalled2++;
        EXPECT_EQ(CommonAPI::CallStatus::NOT_AVAILABLE, callStatus);
        if (numberCallbackCalled2 == numberCalls) {
            try {
                p2.set_value(true);
            } catch (std::future_error &e) {
                ADD_FAILURE() << "myCallBack2 catched exception" << e.what()
                        << " " << std::dec << numberCallbackCalled2;
            }
        }
    };

    CommonAPI::CallInfo its_info(100);

    uint8_t x = 5;
    for (std::uint32_t i = 0; i < numberCalls; i++ ) {
        testProxy_->testMethodAsync(x, myCallback, &its_info);
        testProxy2_->testMethodAsync(x, myCallback2, &its_info);
    }

    EXPECT_EQ(std::future_status::ready, p1.get_future().wait_for(std::chrono::seconds(10)));
    EXPECT_EQ(std::future_status::ready, p2.get_future().wait_for(std::chrono::seconds(10)));
    EXPECT_EQ(numberCalls, numberCallbackCalled);
    EXPECT_EQ(numberCalls, numberCallbackCalled2);

}

#endif /* #ifndef TESTS_BAT */

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
