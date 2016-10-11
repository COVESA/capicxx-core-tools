/* Copyright (C) 2014-2015 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * @file Communication
 */

#include <functional>
#include <fstream>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/communication/TestInterfaceProxy.hpp"
#include "v1/commonapi/communication/TestInterfaceStubDefault.hpp"
#include "v1/commonapi/communication/DaemonStubDefault.hpp"
#include "stub/CMAttributesStub.h"

const std::string daemonId = "service-sample";
const std::string clientId = "client-sample";
const std::string serviceId = "test-service";

const std::string domain = "local";
const std::string testAddress = "commonapi.communication.TestInterface";
const std::string daemonAddress = "commonapi.communication.Daemon";

const unsigned int wt = 100000;

typedef std::shared_ptr<v1_0::commonapi::communication::TestInterfaceProxy<>> ProxyPtr;

std::mutex mut;
std::deque<uint32_t> data_queue;
std::condition_variable data_cond;

using namespace v1_0::commonapi::communication;

class SubscriptionHandler {

public:

    SubscriptionHandler(ProxyPtr pp) : myProxy_(pp) {
        availabilityStatus_ = CommonAPI::AvailabilityStatus::UNKNOWN;
        callbackTestAttribute_ = std::bind(&SubscriptionHandler::myCallback, this, std::placeholders::_1);
        subscriptionStarted_ = false;
        testAttribute_ = 0;
    }

    void receiveServiceAvailable(CommonAPI::AvailabilityStatus as) {

        availabilityStatus_ = as;

        if (as == CommonAPI::AvailabilityStatus::AVAILABLE) {

            startSubscribe();

        } else if (as == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {

            cancelSubscribe();
        }
    }

    void myCallback(const uint8_t& val) {

        testAttribute_ = val;
        myQueue_.push_back(val);

        /* for test purposes, magic value 99 unsubscribes the attribute. */
        if (val == 99) {

            // We can't call cancleSubscribe() within the async handler
            // because it lead to a dead lock with the SomeIP-binding.
            // Therefore we unsubscribe within the main thread of the test case!
            //
            // The reason for the dead lock is because within the async handler
            // the connection thread is holding the Connection::sendReceiveMutex_
            // which will locked again when calling Connection::removeEventHandler
            // which is implicitly called when unsubscribing!

            // cancelSubscribe();
        }
    }

    void startSubscribe() {
        if (!subscriptionStarted_) {
            subscribedListener_ =
                    myProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackTestAttribute_);
            subscriptionStarted_ = true;
        }
    }

    void cancelSubscribe() {
        if (subscriptionStarted_) {
            myProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListener_);
            subscriptionStarted_ = false;
        }
    }

    CommonAPI::AvailabilityStatus getAvailabilityStatus() {
        return availabilityStatus_;
    }


    uint8_t getSubscriptedTestAttribute() {
        return testAttribute_;
    }

    void resetSubcriptedTestAttribute() {
        testAttribute_ = 0;
    }

    bool wait_Attribute(uint8_t expected) {
        uint8_t count = 0;
        while (count++ < 50) {
            if (testAttribute_ == expected) {
                usleep(wt); // Wait a while to ensure its not (unwanted) changed
                return true;
            }
            usleep(100000);
        }
        return false;
    }

    std::deque<uint8_t> myQueue_;

private:

    bool subscriptionStarted_;
    uint8_t testAttribute_;
    ProxyPtr myProxy_;

    CommonAPI::AvailabilityStatus availabilityStatus_;
    CommonAPI::Event<uint8_t>::Subscription subscribedListener_;

    std::function<void(const uint8_t&)> callbackTestAttribute_;
};

class SubscribeUnsubscribeHandler {

public:

    SubscribeUnsubscribeHandler() {
        okAttribute_ = 0;
        notOkAttribute_ = 0;
    }

    void okCallback(const uint8_t& val) {
        okAttribute_ = val;
    }

    void notOkCallback(const uint8_t& val) {
        notOkAttribute_ = val;
    }

    uint8_t getSubscribedOkAttribute() {
        return okAttribute_;
    }

    uint8_t getSubscribedNotOkAttribute() {
        return notOkAttribute_;
    }

    bool wait_OkAttribute(uint8_t expected) {
        uint8_t count = 0;
        while (count++ < 50) {
            if (okAttribute_ == expected) {
                usleep(wt); // Wait a while to ensure its not (unwanted) changed
                return true;
            }
            usleep(100000);
        }
        return false;
    }

    bool wait_NotOkAttribute(uint8_t expected) {
        uint8_t count = 0;
        while (count++ < 50) {
            if (notOkAttribute_ == expected) {
                usleep(wt); // Wait a while to ensure its not (unwanted) changed
                return true;
            }
            usleep(100000);
        }
        return false;
    }
private:

    uint8_t okAttribute_;
    uint8_t notOkAttribute_;
};

class ThreeCallbackHandler {
public:
    ThreeCallbackHandler() :
        callbackCounter_1_(0),
        callbackCounter_2_(0),
        callbackCounter_3_(0),
        callbackValue_1_(0),
        callbackValue_2_(0),
        callbackValue_3_(0) {

    }

    void callback_1(const uint8_t& val) {
        callbackCounter_1_++;
        callbackValue_1_ = val;
    }

    void callback_2(const uint8_t& val) {
        callbackCounter_2_++;
        callbackValue_2_ = val;
    }

    void callback_3(const uint8_t& val) {
        callbackCounter_3_++;
        callbackValue_3_ = val;
    }

    uint8_t getCallbackCounter_1() {
        return callbackCounter_1_;
    }

    uint8_t getCallbackCounter_2() {
        return callbackCounter_2_;
    }

    uint8_t getCallbackCounter_3() {
        return callbackCounter_3_;
    }

    uint8_t getCallbackValue_1() {
        return callbackValue_1_;
    }

    uint8_t getCallbackValue_2() {
        return callbackValue_2_;
    }

    uint8_t getCallbackValue_3() {
        return callbackValue_3_;
    }

    bool waitCallbacks() {
        uint8_t count = 0;
        while (count++ < 50) {
            if (callbackCounter_1_ > 0 && callbackCounter_2_ > 0 && callbackCounter_3_ > 0) {
                usleep(wt); // Sleep to ensure that it not called again in the meanwhile
                return true;
            }
            usleep(100000);
        }
        return false;
    }

private:
    uint8_t callbackCounter_1_;
    uint8_t callbackCounter_2_;
    uint8_t callbackCounter_3_;

    uint8_t callbackValue_1_;
    uint8_t callbackValue_2_;
    uint8_t callbackValue_3_;
};

void testSubscription(ProxyPtr pp) {

    SubscriptionHandler subscriptionHandler(pp);

    std::function<void(const CommonAPI::AvailabilityStatus&)> callbackAvailabilityStatus =
            std::bind(&SubscriptionHandler::receiveServiceAvailable, &subscriptionHandler, std::placeholders::_1);

    pp->getProxyStatusEvent().subscribe(callbackAvailabilityStatus);

    int cnt = 0;
    while (!(subscriptionHandler.getSubscriptedTestAttribute() > 0
            && subscriptionHandler.getAvailabilityStatus()
                    == CommonAPI::AvailabilityStatus::NOT_AVAILABLE)
            && cnt < 1000) {
        usleep(10000);
        cnt++;
    }

    std::lock_guard < std::mutex > lk(mut);
    data_queue.push_back(subscriptionHandler.getSubscriptedTestAttribute());
    data_cond.notify_one();
}

class CMAttributeSubscription: public ::testing::Test {

protected:
    void SetUp() {

        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
        ASSERT_TRUE((bool)testProxy_);

        testStub_ = std::make_shared<TestInterfaceStubDefault>();

        deregisterService_ = false;
    }

    void TearDown() {
        // secure, that service is deregistered by finishing the test
        if (deregisterService_) {
            bool isUnregistered = runtime_->unregisterService(domain,
                    TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
            ASSERT_TRUE(isUnregistered);
        }

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 10 ) {
            usleep(100000);
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<TestInterfaceStubDefault> testStub_;

    bool deregisterService_;
};

class Environment: public ::testing::Environment {

public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
        stubDaemon_ = std::make_shared<DaemonStubDefault>();
        CommonAPI::Runtime::get()->registerService(domain, daemonAddress, stubDaemon_, daemonId);
    }

    virtual void TearDown() {
        CommonAPI::Runtime::get()->unregisterService(domain, DaemonStubDefault::StubInterface::getInterface(),
                daemonAddress);
    }

private:
    std::shared_ptr<DaemonStubDefault> stubDaemon_;
};

/**
 * @test Subscription standard test.
 *    - Register service and check if proxy is available.
 *    - Proxy subscribes for TestAttribute (uint8_t).
 *    - Change attribute in service several times by set method.
 *    - Callback function in proxy writes the received values in a queue.
 *    - Check if values in the queue are the same as the values that were set in the service.
 *    - Unregister test service.
 */
TEST_F(CMAttributeSubscription, SubscriptionStandard) {

    deregisterService_ = true;

    uint8_t defaultValue = 33;

    // initialize test stub with default value
    testStub_->setTestAttributeAttribute(defaultValue);

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    SubscriptionHandler subscriptionHandler(testProxy_);
    std::function<void (const uint8_t&)> myCallback =
            std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription subscribedListener =
            testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);

    const uint8_t testNumber = 10;
    for (uint8_t i=1; i<testNumber+1; i++) {
        usleep(100000);
        testStub_->setTestAttributeAttribute(i);
    }

    usleep(wt);
    ASSERT_EQ(subscriptionHandler.myQueue_.size(), std::deque<uint8_t>::size_type(testNumber + 1)); // + 1 due to the reason, that by subscribtion the current value is returned

    uint8_t t = 0;
    for(std::deque<uint8_t>::iterator it = subscriptionHandler.myQueue_.begin(); it != subscriptionHandler.myQueue_.end(); ++it) {
        if (t == 0) {
            EXPECT_EQ(defaultValue, *it);
            t++;
        } else {
            EXPECT_EQ(t, *it);
            t++;
        }
    }

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListener);

    bool isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);

    usleep(wt);

    deregisterService_ = !isUnregistered;
}


/**
 * @test Subscription test with subscription on available-event.
 *    - Subscribe for available-event.
 *    - Available-callback subscribes for TestPredefinedTypeAttribute if service is available for proxy and
 *      unsubscribes if service is not available for proxy.
 *    - Change attribute in service by set method; the new attribute value should be received by the proxy because the service is not registered.
 *    - Register service and change value again; the value should now be received.
 *    - Unregister and change value again.
 */
TEST_F(CMAttributeSubscription, SubscriptionOnAvailable) {

    SubscriptionHandler subscriptionHandler(testProxy_);

    std::function<void (const CommonAPI::AvailabilityStatus&)> callbackAvailabilityStatus =
            std::bind(&SubscriptionHandler::receiveServiceAvailable, &subscriptionHandler, std::placeholders::_1);
    testProxy_->getProxyStatusEvent().subscribe(callbackAvailabilityStatus);

    testStub_->setTestAttributeAttribute(1);
    usleep(wt);
    EXPECT_EQ(0, subscriptionHandler.getSubscriptedTestAttribute());

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    testStub_->setTestAttributeAttribute(2);
    subscriptionHandler.wait_Attribute(2);
    ASSERT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 2);

    bool serviceUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(serviceUnregistered);
    usleep(wt);
    ASSERT_EQ(subscriptionHandler.getAvailabilityStatus(), CommonAPI::AvailabilityStatus::NOT_AVAILABLE);

    testStub_->setTestAttributeAttribute(3);
    usleep(wt);
    ASSERT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 2);
}

/**
 * @test Subscription test with several threads.
 *    - Start several threads.
 *    - The threads subscribe for the availability status.
 *    - The available-callback subscribes for TestAttribute if service is available for proxy and
 *    - unsubscribes if service is not available for proxy.
 *    - Change attribute in service by set method; the new attribute value should be received by all the threads.
 *    - The new value is written into a queue.
 *    - Check if the values of each thread are written into the queue.
 */
TEST_F(CMAttributeSubscription, SubscriptionMultithreading) {
    std::thread t0(testSubscription, testProxy_);
    std::thread t1(testSubscription, testProxy_);
    std::thread t2(testSubscription, testProxy_);
    std::thread t3(testSubscription, testProxy_);
    std::thread t4(testSubscription, testProxy_);
    std::thread t5(testSubscription, testProxy_);
    std::thread t6(testSubscription, testProxy_);
    std::thread t7(testSubscription, testProxy_);
    std::thread t8(testSubscription, testProxy_);
    std::thread t9(testSubscription, testProxy_);

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    usleep(wt);
    testStub_->setTestAttributeAttribute(1);

    usleep(wt);

    bool serviceUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(serviceUnregistered);

    uint32_t data = 0;
    unsigned int cnt = 0;
    while (cnt<10) {

        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait_for(lk, std::chrono::milliseconds(2000), [] {return !data_queue.empty();});

        if ( !data_queue.empty() ) {
            data = data_queue.front();
            data_queue.pop_front();
        }
        lk.unlock();
        EXPECT_EQ(1, static_cast<int32_t>(data));
        cnt++;
    }

    t0.join();
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    t9.join();
}

/**
 * @test Subscription test : unsibscribe from the subscription callback.
 *    - Register service and check if proxy is available.
 *    - Proxy subscribes for TestAttribute (uint8_t).
 *    - Change attribute in service by set method.
 *    - Check if callback function in proxy received the right value.
 *    - Change value to the magic value 99: this triggers the callback to unsubscribe.
 *    - Change value again; the callback should now be called anymore.
 *    - Unregister the test service.
 */

TEST_F(CMAttributeSubscription, SubscriptionUnsubscribeFromCallback) {

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    SubscriptionHandler subscriptionHandler(testProxy_);

    std::function<void (const uint8_t&)> myCallback = std::bind(&SubscriptionHandler::myCallback,
            &subscriptionHandler, std::placeholders::_1);
    subscriptionHandler.startSubscribe();

    testStub_->setTestAttributeAttribute(42);
    subscriptionHandler.wait_Attribute(42);
    EXPECT_EQ(42, subscriptionHandler.getSubscriptedTestAttribute());

    testStub_->setTestAttributeAttribute(99);
    subscriptionHandler.wait_Attribute(99);
    EXPECT_EQ(99, subscriptionHandler.getSubscriptedTestAttribute());

    subscriptionHandler.cancelSubscribe();
    usleep(wt);

    testStub_->setTestAttributeAttribute(250);
    usleep(wt);
    EXPECT_EQ(99, subscriptionHandler.getSubscriptedTestAttribute());

    bool isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);
}

/**
 * @test Test of subscribe and unsubscribe with two coexistent callbacks
 *  - subscribe both callbacks
 *  - change value
 *  - check that both callbacks were executed by changing the value
 *  - unsubscribe both callbacks
 *  - change value
 *  - check that both callbacks were not executed by changing the value
 */
TEST_F(CMAttributeSubscription, SubscribeAndUnsubscribeTwoCallbacksCoexistent) {

    SubscribeUnsubscribeHandler subUnsubHandler;

    CommonAPI::Event<uint8_t>::Subscription subscribedListenerCallOk;
    CommonAPI::Event<uint8_t>::Subscription subscribedListenerCallNotOk;

    std::function<void (const uint8_t&)> callbackOk =
            std::bind(&SubscribeUnsubscribeHandler::okCallback, &subUnsubHandler, std::placeholders::_1);
    std::function<void (const uint8_t&)> callbackNotOk =
            std::bind(&SubscribeUnsubscribeHandler::notOkCallback, &subUnsubHandler, std::placeholders::_1);

    subscribedListenerCallOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackOk);
    subscribedListenerCallNotOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackNotOk);

    testStub_->setTestAttributeAttribute(1);
    usleep(wt);
    EXPECT_EQ(0, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(0, subUnsubHandler.getSubscribedNotOkAttribute());

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    testStub_->setTestAttributeAttribute(2);
    subUnsubHandler.wait_OkAttribute(2);
    subUnsubHandler.wait_NotOkAttribute(2);
    EXPECT_EQ(2, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(2, subUnsubHandler.getSubscribedNotOkAttribute());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListenerCallOk);
    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListenerCallNotOk);

    testStub_->setTestAttributeAttribute(3);
    usleep(wt);
    EXPECT_EQ(2, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(2, subUnsubHandler.getSubscribedNotOkAttribute());

    bool serviceUnregistered = runtime_->unregisterService(domain, TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(serviceUnregistered);
    usleep(wt);
}

/**
 * @test Test of subscribing and immediately unsubscribing a callback
 *  - subscribe first callback
 *  - subscribe second callback
 *  - unsubscribe second callback
 *  - change value
 *  - check that only first callback was executed
 */
/*
 * COMMENTED OUT because test case fails (rarely).
 * The reason for that is a different implementation as in test description:
 *  1. subscribe first callback
 *  2. change value
 *  3. subscribe second callback
 *  4. unsubscribe second callback
 *
 *  Between 3. and 4. (before unsubscription) the initial callback can be triggered.
 *  This leads to a failing 'EXPECT_EQ(0, subUnsubHandler.getSubscribedNotOkAttribute())'
 *  command (Expected is '0' Actual is '10').
 *
 */
//TEST_F(CMAttributeSubscription, SubscribeAndUnsubscribeImmediatelyUnsubscribing) {
//
//    SubscribeUnsubscribeHandler subUnsubHandler;
//
//    CommonAPI::Event<uint8_t>::Subscription subscribedListenerCallOk;
//    CommonAPI::Event<uint8_t>::Subscription subscribedListenerCallNotOk;
//
//    std::function<void (const uint8_t&)> callbackOk =
//            std::bind(&SubscribeUnsubscribeHandler::okCallback, &subUnsubHandler, std::placeholders::_1);
//    std::function<void (const uint8_t&)> callbackNotOk =
//            std::bind(&SubscribeUnsubscribeHandler::notOkCallback, &subUnsubHandler, std::placeholders::_1);
//
//    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
//    ASSERT_TRUE(serviceRegistered);
//
//    testProxy_->isAvailableBlocking();
//    ASSERT_TRUE(testProxy_->isAvailable());
//
//    // subscribe ok callback
//    subscribedListenerCallOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackOk);
//
//    testStub_->setTestAttributeAttribute(10);
//    subUnsubHandler.wait_OkAttribute(10);
//    EXPECT_EQ(10, subUnsubHandler.getSubscribedOkAttribute());
//
//    // subscribe notOk callback
//    subscribedListenerCallNotOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackNotOk);
//
//    // unsubscribe notOk callback
//    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListenerCallNotOk);
//
//    testStub_->setTestAttributeAttribute(12);
//    subUnsubHandler.wait_OkAttribute(12);
//    EXPECT_EQ(12, subUnsubHandler.getSubscribedOkAttribute());
//    EXPECT_EQ(0, subUnsubHandler.getSubscribedNotOkAttribute());
//
//    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListenerCallNotOk);
//    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListenerCallOk);
//
//    bool serviceUnregistered = runtime_->unregisterService(domain, TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
//    ASSERT_TRUE(serviceUnregistered);
//    usleep(wt);
//}

/**
 * @test Test of subscribing and unsubscribing sequentially
 *  - subscribe first callback
 *  - subscribe second callback
 *  - change value
 *  - check that both callbacks were executed by changing the value
 *  - unsubscribe first callback
 *  - change value
 *  - check that only second callback was executed
 *  - unsubscribe second callback
 *  - change value
 *  - check that both callbacks were not executed by changing the value
 */
TEST_F(CMAttributeSubscription, SubscribeAndUnsubscribeSequentially) {

    SubscribeUnsubscribeHandler subUnsubHandler;

    CommonAPI::Event<uint8_t>::Subscription subscribedListenerCallOk;
    CommonAPI::Event<uint8_t>::Subscription subscribedListenerCallNotOk;

    std::function<void (const uint8_t&)> callbackOk =
            std::bind(&SubscribeUnsubscribeHandler::okCallback, &subUnsubHandler, std::placeholders::_1);
    std::function<void (const uint8_t&)> callbackNotOk =
            std::bind(&SubscribeUnsubscribeHandler::notOkCallback, &subUnsubHandler, std::placeholders::_1);

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    // subscribe ok and notOk callback
    subscribedListenerCallOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackOk);
    subscribedListenerCallNotOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackNotOk);

    testStub_->setTestAttributeAttribute(12);
    subUnsubHandler.wait_OkAttribute(12);
    subUnsubHandler.wait_NotOkAttribute(12);
    EXPECT_EQ(12, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(12, subUnsubHandler.getSubscribedNotOkAttribute());

    // unsubscribe ok callback
    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListenerCallOk);

    testStub_->setTestAttributeAttribute(14);
    subUnsubHandler.wait_NotOkAttribute(14);
    EXPECT_EQ(12, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(14, subUnsubHandler.getSubscribedNotOkAttribute());

    // unsubscribe notOk callback
    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListenerCallNotOk);

    testStub_->setTestAttributeAttribute(16);
    usleep(wt);
    EXPECT_EQ(12, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(14, subUnsubHandler.getSubscribedNotOkAttribute());

    bool serviceUnregistered = runtime_->unregisterService(domain, TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(serviceUnregistered);
    usleep(wt);
}

/**
 * @test Test of subscribing and unsubscribing implicit with creating a new proxy with reassigning
 *  - subscribe first callback
 *  - subscribe second callback
 *  - change value
 *  - check that both callbacks were executed by changing the value
 *  - create new proxy with reassigning. So the connection won't be destroyed and the callbacks
 *    are unsubscribed implicitly.
 *  - subscribe second callback
 *  - change value
 *  - check that only second callback was executed
 *  - unsubscribe second callback
 *  - change value
 *  - check that both callbacks were not executed by changing the value
 */
TEST_F(CMAttributeSubscription, DISABLED_SubscribeAndUnsubscribeImplicitWithCreatingNewProxyWithReassigning) {

    SubscribeUnsubscribeHandler subUnsubHandler;

    CommonAPI::Event<uint8_t>::Subscription subscribedListenerCallOk;
    CommonAPI::Event<uint8_t>::Subscription subscribedListenerCallNotOk;

    std::function<void (const uint8_t&)> callbackOk =
            std::bind(&SubscribeUnsubscribeHandler::okCallback, &subUnsubHandler, std::placeholders::_1);
    std::function<void (const uint8_t&)> callbackNotOk =
            std::bind(&SubscribeUnsubscribeHandler::notOkCallback, &subUnsubHandler, std::placeholders::_1);

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    // subscribe ok and notOk callback
    subscribedListenerCallOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackOk);
    subscribedListenerCallNotOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackNotOk);

    testStub_->setTestAttributeAttribute(12);
    subUnsubHandler.wait_OkAttribute(12);
    subUnsubHandler.wait_NotOkAttribute(12);
    EXPECT_EQ(12, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(12, subUnsubHandler.getSubscribedNotOkAttribute());

    //build new proxy with reassigning --> Connection won't be destroyed + callbacks will be unsubscribed
    testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
    ASSERT_TRUE((bool)testProxy_);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    //subscribe notOk callback
    subscribedListenerCallNotOk = testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(callbackNotOk);

    testStub_->setTestAttributeAttribute(14);
    subUnsubHandler.wait_NotOkAttribute(14);
    EXPECT_EQ(12, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(14, subUnsubHandler.getSubscribedNotOkAttribute());

    // unsubscribe notOk callback
    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListenerCallNotOk);

    testStub_->setTestAttributeAttribute(16);
    usleep(wt);
    EXPECT_EQ(12, subUnsubHandler.getSubscribedOkAttribute());
    EXPECT_EQ(14, subUnsubHandler.getSubscribedNotOkAttribute());

    bool serviceUnregistered = runtime_->unregisterService(domain, TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(serviceUnregistered);
    usleep(wt);
}

/**
 * @test Test of behaviour in case unsubscribe is called two times
 *  - set default value
 *  - register service
 *  - subscribe for the attribute
 *  - current value must be communicated to the proxy
 *  - value of attribute is changed
 *  - changed value must be communicated to the proxy
 *  - proxy unsubscribes for the attribute
 *  - value of attribute is changed
 *  - changed value must not be communicated to the proxy
 *  - proxy unsubscribes again for the attribute
 *  - value of attribute is changed
 *  - changed value must not be communicated to the proxy
 *  - unregister service
 */
TEST_F(CMAttributeSubscription, SubscribeAndUnsubscribeUnsubscribe) {

    testStub_->setTestAttributeAttribute(42);

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    SubscriptionHandler subscriptionHandler(testProxy_);
    std::function<void (const uint8_t&)> myCallback =
            std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription subscribedListener =
            testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);

    subscriptionHandler.wait_Attribute(42);

    // check for initial value
    EXPECT_EQ(42, subscriptionHandler.getSubscriptedTestAttribute());

    testStub_->setTestAttributeAttribute(12);
    subscriptionHandler.wait_Attribute(12);

    // check for changed value
    EXPECT_EQ(12, subscriptionHandler.getSubscriptedTestAttribute());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListener);

    testStub_->setTestAttributeAttribute(24);
    usleep(wt);

    // value must not be changed
    EXPECT_EQ(12, subscriptionHandler.getSubscriptedTestAttribute());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListener);

    testStub_->setTestAttributeAttribute(26);
    usleep(wt);

    // value must not be changed
    EXPECT_EQ(12, subscriptionHandler.getSubscriptedTestAttribute());

    bool serviceUnregistered = runtime_->unregisterService(domain, TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(serviceUnregistered);
    usleep(wt);
}

/**
 * @test Test of subscribing in case that service is not available
 *  - set default value
 *  - subscribe for the attribute
 *  - no value is communicated to the proxy
 *  - register service
 *  - current value must be communicated to the proxy
 *  - value of attribute is changed
 *  - changed value must be communicated to the proxy
 *  - unregister service
 */
TEST_F(CMAttributeSubscription, SubscribeServiceNotAvailable) {

    deregisterService_ = true;

    uint8_t defaultValue = 33;

    // initialize test stub with default value
    testStub_->setTestAttributeAttribute(defaultValue);

    // subscribe for attribute change by the proxy
    SubscriptionHandler subscriptionHandler(testProxy_);
    std::function<void (const uint8_t&)> myCallback =
            std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription subscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);

    usleep(wt);
    EXPECT_EQ(0, subscriptionHandler.getSubscriptedTestAttribute());

    // register service
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    subscriptionHandler.wait_Attribute(defaultValue);

    // check received attribute value
    EXPECT_EQ(defaultValue, subscriptionHandler.getSubscriptedTestAttribute());

    int8_t newValue = 123;

    testStub_->setTestAttributeAttribute(newValue);
    subscriptionHandler.wait_Attribute(newValue);

    EXPECT_EQ(newValue, subscriptionHandler.getSubscriptedTestAttribute());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListener);

    bool isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);

    deregisterService_ = !isUnregistered;
}

/**
 * @test Test of unregister a service in case a proxy is subscribed for an attribute of this service.
 * During the unregistered time of the service the value of the attribute is changed.
 *  - register service
 *  - proxy subscribes for an attribute of the service
 *  - value of attribute is set
 *  - changed value must be communicated to the proxy
 *  - unregister service
 *  - value of attribute is changed
 *  - changed value must not be communicated to the proxy
 *  - register service
 *  - current attribute value must be communicated to the proxy
 *  - value of attribute is changed
 *  - changed value must be communicated to the proxy
 *  - unregister service
 */
TEST_F(CMAttributeSubscription, SubscribeUnregisterSetValueRegisterService) {
    deregisterService_ = true;

    uint8_t firstValue = 35;
    uint8_t secondValue = 43;
    uint8_t thirdValue = 198;

    // register service
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    // subscribe for attribute change by the proxy
    SubscriptionHandler subscriptionHandler(testProxy_);
    std::function<void (const uint8_t&)> myCallback =
            std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription subscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);

    // initialize test stub with default value
    testStub_->setTestAttributeAttribute(firstValue);
    subscriptionHandler.wait_Attribute(firstValue);

    // check received attribute value
    EXPECT_EQ(firstValue, subscriptionHandler.getSubscriptedTestAttribute());

    bool isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);
    deregisterService_ = !isUnregistered;

    testStub_->setTestAttributeAttribute(secondValue);
    usleep(wt);

    EXPECT_EQ(firstValue, subscriptionHandler.getSubscriptedTestAttribute());

    // register service
    serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    subscriptionHandler.wait_Attribute(secondValue);

    EXPECT_EQ(secondValue, subscriptionHandler.getSubscriptedTestAttribute());

    testStub_->setTestAttributeAttribute(thirdValue);
    subscriptionHandler.wait_Attribute(thirdValue);

    EXPECT_EQ(thirdValue, subscriptionHandler.getSubscriptedTestAttribute());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListener);

    isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);

    deregisterService_ = !isUnregistered;
}

/**
 * @test Test of unregister a service in case a proxy is subscribed for an attribute of this service.
 * During the unregistered time of the service the value of the attribute is not changed.
 *  - register service
 *  - proxy subscribes for an attribute of the service
 *  - value of attribute is set
 *  - changed value must be communicated to the proxy
 *  - unregister service
 *  - register service
 *  - current attribute value must be communicated to the proxy
 *  - value of attribute is changed
 *  - changed value must be communicated to the proxy
 *  - unregister service
 */
TEST_F(CMAttributeSubscription, SubscribeUnregisterNoValueSetRegisterService) {

    deregisterService_ = true;

    uint8_t firstValue = 35;
    uint8_t secondValue = 43;

    // register service
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    // subscribe for attribute change by the proxy
    SubscriptionHandler subscriptionHandler(testProxy_);
    std::function<void (const uint8_t&)> myCallback =
            std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription subscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);

    // initialize test stub with default value
    testStub_->setTestAttributeAttribute(firstValue);
    subscriptionHandler.wait_Attribute(firstValue);

    // check received attribute value
    EXPECT_EQ(firstValue, subscriptionHandler.getSubscriptedTestAttribute());

    bool isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);
    usleep(wt);

    subscriptionHandler.resetSubcriptedTestAttribute();
    EXPECT_EQ(0, subscriptionHandler.getSubscriptedTestAttribute());
    usleep(wt);

    // register service
    serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    subscriptionHandler.wait_Attribute(firstValue);

    EXPECT_EQ(firstValue, subscriptionHandler.getSubscriptedTestAttribute());

    testStub_->setTestAttributeAttribute(secondValue);
    subscriptionHandler.wait_Attribute(secondValue);

    EXPECT_EQ(secondValue, subscriptionHandler.getSubscriptedTestAttribute());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(subscribedListener);

    isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);

    deregisterService_ = !isUnregistered;
}

/**
 * @test Test of subscribing a second proxy a little bit later
 *  - proxy subscribes for an attribute of the service
 *  - register service
 *  - initial value must be communicated to the proxy
 *  - create a second proxy
 *  - second proxy subscribes for the same attribute of the service
 *  - current attribute value must be communicated to the proxy
 *  - value of attribute is changed
 *  - changed value must be communicated to both proxies
 *  - unregister service
 */
TEST_F(CMAttributeSubscription, SubscribeSecondProxyLater) {

    deregisterService_ = true;

    uint8_t defaultValue = 33;

    // initialize test stub with default value
    testStub_->setTestAttributeAttribute(defaultValue);

    // subscribe for attribute change by the proxy
    SubscriptionHandler subscriptionHandler(testProxy_);
    std::function<void (const uint8_t&)> myCallback =
            std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription firstSubscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);

    // register service
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    subscriptionHandler.wait_Attribute(defaultValue);

    // check received attribute value
    EXPECT_EQ(defaultValue, subscriptionHandler.getSubscriptedTestAttribute());

    // create second proxy
    std::shared_ptr<TestInterfaceProxy<>> secondTestProxy =
            runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
    ASSERT_TRUE((bool)secondTestProxy);

    SubscriptionHandler secondSubscriptionHandler(secondTestProxy);
    std::function<void (const uint8_t&)> secondMyCallback =
            std::bind(&SubscriptionHandler::myCallback, &secondSubscriptionHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription secondSubscribedListener =
        secondTestProxy->getTestAttributeAttribute().getChangedEvent().subscribe(secondMyCallback);

    secondSubscriptionHandler.wait_Attribute(defaultValue);

    EXPECT_EQ(defaultValue, secondSubscriptionHandler.getSubscriptedTestAttribute());

    int8_t newValue = 123;

    testStub_->setTestAttributeAttribute(newValue);

    subscriptionHandler.wait_Attribute(newValue);
    secondSubscriptionHandler.wait_Attribute(newValue);

    EXPECT_EQ(newValue, subscriptionHandler.getSubscriptedTestAttribute());
    EXPECT_EQ(newValue, secondSubscriptionHandler.getSubscriptedTestAttribute());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(firstSubscribedListener);
    secondTestProxy->getTestAttributeAttribute().getChangedEvent().unsubscribe(secondSubscribedListener);

    bool isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);

    deregisterService_ = !isUnregistered;
}

/**
 * @test Test of subscribing three callbacks before registering the service
 *  - proxy subscribes three callbacks for an attribute of the service
 *  - register service
 *  - initial value must be communicated to every callback
 */
TEST_F(CMAttributeSubscription, SubscribeThreeCallbacksServiceNotAvailable) {

    deregisterService_ = true;

    ThreeCallbackHandler threeCallbackHandler;

    uint8_t defaultValue = 33;

    // initialize test stub with default value
    testStub_->setTestAttributeAttribute(defaultValue);

    std::function<void (const uint8_t&)> myCallback1 =
            std::bind(&ThreeCallbackHandler::callback_1, &threeCallbackHandler, std::placeholders::_1);
    std::function<void (const uint8_t&)> myCallback2 =
            std::bind(&ThreeCallbackHandler::callback_2, &threeCallbackHandler, std::placeholders::_1);
    std::function<void (const uint8_t&)> myCallback3 =
            std::bind(&ThreeCallbackHandler::callback_3, &threeCallbackHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription firstSubscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback1);
    usleep(wt);
    CommonAPI::Event<uint8_t>::Subscription secondSubscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback2);
    usleep(wt);
    CommonAPI::Event<uint8_t>::Subscription thirdSubscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback3);

    // register service
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    threeCallbackHandler.waitCallbacks();

    EXPECT_EQ(1, threeCallbackHandler.getCallbackCounter_1());
    EXPECT_EQ(1, threeCallbackHandler.getCallbackCounter_2());
    EXPECT_EQ(1, threeCallbackHandler.getCallbackCounter_3());

    EXPECT_EQ(defaultValue, threeCallbackHandler.getCallbackValue_1());
    EXPECT_EQ(defaultValue, threeCallbackHandler.getCallbackValue_2());
    EXPECT_EQ(defaultValue, threeCallbackHandler.getCallbackValue_3());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(firstSubscribedListener);
    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(secondSubscribedListener);
    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(thirdSubscribedListener);

    bool isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);

    deregisterService_ = !isUnregistered;
}

/**
 * @test Test of subscribing three callbacks after registering the service
 *  - register service
 *  - proxy subscribes three callbacks for an attribute of the service
 *  - initial value must be communicated to every callback
 */
TEST_F(CMAttributeSubscription, SubscribeThreeCallbacksServiceAvailable) {
    deregisterService_ = true;

    uint8_t defaultValue = 35;

    ThreeCallbackHandler threeCallbackHandler;

    testStub_->setTestAttributeAttribute(defaultValue);
    usleep(wt);

    // register service
    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    testProxy_->isAvailableBlocking();
    ASSERT_TRUE(testProxy_->isAvailable());

    std::function<void (const uint8_t&)> myCallback1 =
            std::bind(&ThreeCallbackHandler::callback_1, &threeCallbackHandler, std::placeholders::_1);
    std::function<void (const uint8_t&)> myCallback2 =
            std::bind(&ThreeCallbackHandler::callback_2, &threeCallbackHandler, std::placeholders::_1);
    std::function<void (const uint8_t&)> myCallback3 =
            std::bind(&ThreeCallbackHandler::callback_3, &threeCallbackHandler, std::placeholders::_1);

    CommonAPI::Event<uint8_t>::Subscription firstSubscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback1);
    usleep(wt);
    CommonAPI::Event<uint8_t>::Subscription secondSubscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback2);
    usleep(wt);
    CommonAPI::Event<uint8_t>::Subscription thirdSubscribedListener =
        testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback3);

    threeCallbackHandler.waitCallbacks();

    EXPECT_EQ(1, threeCallbackHandler.getCallbackCounter_1());
    EXPECT_EQ(1, threeCallbackHandler.getCallbackCounter_2());
    EXPECT_EQ(1, threeCallbackHandler.getCallbackCounter_3());

    EXPECT_EQ(defaultValue, threeCallbackHandler.getCallbackValue_1());
    EXPECT_EQ(defaultValue, threeCallbackHandler.getCallbackValue_2());
    EXPECT_EQ(defaultValue, threeCallbackHandler.getCallbackValue_3());

    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(firstSubscribedListener);
    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(secondSubscribedListener);
    testProxy_->getTestAttributeAttribute().getChangedEvent().unsubscribe(thirdSubscribedListener);

    bool isUnregistered = runtime_->unregisterService(domain,
            TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(isUnregistered);

    deregisterService_ = !isUnregistered;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
