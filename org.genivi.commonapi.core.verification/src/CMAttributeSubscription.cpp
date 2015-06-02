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
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "v1_0/commonapi/communication/TestInterfaceProxy.hpp"
#include "v1_0/commonapi/communication/TestInterfaceStubDefault.hpp"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.communication.TestInterface";

const unsigned int wt = 500000;

typedef std::shared_ptr<v1_0::commonapi::communication::TestInterfaceProxy<>> ProxyPtr;

std::mutex mut;
std::deque<uint32_t> data_queue;
std::condition_variable data_cond;

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
            cancelSubscribe();
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

    std::deque<uint8_t> myQueue_;

private:

    bool subscriptionStarted_;
    uint8_t testAttribute_;
    ProxyPtr myProxy_;

    CommonAPI::AvailabilityStatus availabilityStatus_;
    CommonAPI::Event<uint8_t>::Subscription subscribedListener_;

    std::function<void(const uint8_t&)> callbackTestAttribute_;
};

void testSubscription(ProxyPtr pp) {

    SubscriptionHandler subscriptionHandler(pp);

    std::function<void(CommonAPI::AvailabilityStatus)> callbackAvailabilityStatus =
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

        testProxy_ = runtime_->buildProxy<v1_0::commonapi::communication::TestInterfaceProxy>(domain, testAddress, clientId);
        ASSERT_TRUE((bool)testProxy_);

        testStub_ = std::make_shared<v1_0::commonapi::communication::TestInterfaceStubDefault>();
    }

    void TearDown() {
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<v1_0::commonapi::communication::TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<v1_0::commonapi::communication::TestInterfaceStubDefault> testStub_;
};

class Environment: public ::testing::Environment {

public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

private:
};

/**
 * @test Subscription standard test.
 *	- Register service and check if proxy is available.
 *	- Proxy subscribes for TestAttribute (uint8_t).
 * 	- Change attribute in service several times by set method.
 * 	- Callback function in proxy writes the received values in a queue.
 * 	- Check if values in the queue are the same as the values that were set in the service.
 * 	- Unregister test service.
 */
TEST_F(CMAttributeSubscription, SubscriptionStandard) {

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    for(unsigned int i = 0; !testProxy_->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    SubscriptionHandler subscriptionHandler(testProxy_);
    std::function<void (const uint8_t&)> myCallback = std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);
    //CommonAPI::Event<uint8_t>::Subscription subscribedListener =
    testProxy_->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);

    const uint8_t testNumber = 10;
    for (uint8_t i=1; i<testNumber+1; i++) {
        testStub_->setTestAttributeAttribute(i);
    }

    usleep(wt);
    ASSERT_EQ(subscriptionHandler.myQueue_.size(), testNumber);

    uint8_t t = 1;
    for(std::deque<uint8_t>::iterator it = subscriptionHandler.myQueue_.begin(); it != subscriptionHandler.myQueue_.end(); ++it) {
        EXPECT_EQ(*it, t);
        t++;
    }
    runtime_->unregisterService(domain, v1_0::commonapi::communication::TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
}

/**
 * @test Subscription test with subscription on available-event.
 *	- Subscribe for available-event.
 *	- Available-callback subscribes for TestPredefinedTypeAttribute if service is available for proxy and
 *	unsubscribes if service is not available for proxy.
 * 	- Change attribute in service by set method; the new attribute value should be received by the proxy because the service is not registered.
 * 	- Register service and change value again; the value should now be received.
 * 	- Unregister and change value again.
 */
TEST_F(CMAttributeSubscription, SubscriptionOnAvailable) {

    SubscriptionHandler subscriptionHandler(testProxy_);

    std::function<void (CommonAPI::AvailabilityStatus)> callbackAvailabilityStatus = std::bind(&SubscriptionHandler::receiveServiceAvailable, &subscriptionHandler, std::placeholders::_1);
    testProxy_->getProxyStatusEvent().subscribe(callbackAvailabilityStatus);

    testStub_->setTestAttributeAttribute(1);
    usleep(wt);
    EXPECT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 0);

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);
    usleep(wt);
    testStub_->setTestAttributeAttribute(2);
    usleep(wt);
    ASSERT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 2);

    bool serviceUnregistered = runtime_->unregisterService(domain, v1_0::commonapi::communication::TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
    ASSERT_TRUE(serviceUnregistered);
    usleep(wt);
    ASSERT_EQ(subscriptionHandler.getAvailabilityStatus(), CommonAPI::AvailabilityStatus::NOT_AVAILABLE);

    testStub_->setTestAttributeAttribute(3);
    usleep(wt);
    ASSERT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 2);
}

/**
 * @test Subscription test with several threads.
 *	- Start several threads.
 *	- The threads subscribe for the availability status.
 *	- The available-callback subscribes for TestAttribute if service is available for proxy and
 *	- unsubscribes if service is not available for proxy.
 * 	- Change attribute in service by set method; the new attribute value should be received by all the threads.
 * 	- The new value is written into a queue.
 * 	- Check if the values of each thread are written into the queue.
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

    bool serviceUnregistered = runtime_->unregisterService(domain, v1_0::commonapi::communication::TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
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
        EXPECT_EQ(static_cast<int32_t>(data), 1);
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
 * @test Subscription test for cancellable callbacks.
 *	- Register service and check if proxy is available.
 *	- Proxy subscribes for TestAttribute (uint8_t) with the subscribeCancellableListener method.
 * 	- Change attribute in service by set method.
 * 	- Check if callback function in proxy received the right value.
 * 	- Set desired subscription status to CANCEL.
 * 	- Change value again; the callback should be called again but with return value CommonAPI::SubscriptionStatus::CANCEL.
 * 	- Change value again; the callback should now be called anymore.
 * 	- Unregister the test service.
 */
/* this test is no longer valid for CommonAPI 3.0.

TEST_F(CMAttributeSubscription, SubscriptionCancellable) {

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_);
    ASSERT_TRUE(serviceRegistered);

    for(unsigned int i = 0; !testProxy_->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    SubscriptionHandler subscriptionHandler(testProxy_);

    std::function<CommonAPI::SubscriptionStatus (const uint8_t&)> myCallback = std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);
    testProxy_->getTestAttributeAttribute().getChangedEvent().subscribeCancellableListener(myCallback);
    subscriptionHandler.setSubscriptionStatus(CommonAPI::SubscriptionStatus::RETAIN);

    testStub_->setTestAttributeAttribute(42);
    usleep(wt);
    EXPECT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 42);

    subscriptionHandler.setSubscriptionStatus(CommonAPI::SubscriptionStatus::CANCEL);

    testStub_->setTestAttributeAttribute(99);
    usleep(wt);
    EXPECT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 99);

    testStub_->setTestAttributeAttribute(250);
    usleep(wt);
    EXPECT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 99);

    runtime_->unregisterService(domain, v1_0::commonapi::communication::TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
}
*/
/**
 * @test Subscription test : unsibscribe from the subscription callback.
 *	- Register service and check if proxy is available.
 *	- Proxy subscribes for TestAttribute (uint8_t).
 * 	- Change attribute in service by set method.
 * 	- Check if callback function in proxy received the right value.
 * 	- Change value to the magic value 99: this triggers the callback to unsubscribe.
 * 	- Change value again; the callback should now be called anymore.
 * 	- Unregister the test service.
 */

TEST_F(CMAttributeSubscription, SubscriptionUnsubscribeFromCallback) {

    bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
    ASSERT_TRUE(serviceRegistered);

    for(unsigned int i = 0; !testProxy_->isAvailable() && i < 100; ++i) {
        usleep(10000);
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    SubscriptionHandler subscriptionHandler(testProxy_);

    std::function<void (const uint8_t&)> myCallback = std::bind(&SubscriptionHandler::myCallback, &subscriptionHandler, std::placeholders::_1);
    subscriptionHandler.startSubscribe();    

    testStub_->setTestAttributeAttribute(42);
    usleep(wt);
    EXPECT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 42);

    testStub_->setTestAttributeAttribute(99);
    usleep(wt);
    EXPECT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 99);

    testStub_->setTestAttributeAttribute(250);
    usleep(wt);
    EXPECT_EQ(subscriptionHandler.getSubscriptedTestAttribute(), 99);

    runtime_->unregisterService(domain, v1_0::commonapi::communication::TestInterfaceStubDefault::StubInterface::getInterface(), testAddress);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
