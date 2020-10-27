/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Communication
*/

#include <functional>
#include <fstream>
#include <algorithm>
#include <random>
#include <sstream>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/communication/TestInterfaceProxy.hpp"
#include "stub/CMMethodCallsStub.hpp"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";
const std::string clientId2 = "other-client-sample";

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

class CMBlockingCalls: public ::testing::Test {

public:

    CMBlockingCalls()
    {
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
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<TestInterfaceStub> testStub_;
    std::shared_ptr<TestInterfaceStub> testStub2_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy2_;
};

#ifndef TESTS_BAT

/**
* @test Call test method which generates blocking calls on stub side and check
* if answers are received.
*/

TEST_F(CMBlockingCalls, BlockInStubMethod) {
    std::mutex its_mutex;
    std::map<std::uint32_t, bool> calls;

    std::function<void (const CommonAPI::CallStatus&, uint32_t)> myCallback =
            [&](const CommonAPI::CallStatus& callStatus, std::uint32_t _callNumber) {
        EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
        std::lock_guard<std::mutex> its_lock(its_mutex);
        calls[_callNumber] = true;
    };

    const int number_calls = 20;
    const int number_iterations = 5;
    const int minimum_block_time = 100;
    const int maximum_block_time = 2000;

    std::random_device r;
    std::mt19937 e(r());
    std::uniform_int_distribution<std::uint32_t> distribution(
            minimum_block_time, maximum_block_time);
    std::vector<std::uint32_t> its_timeouts;
    for (int var = 0; var < number_calls; ++var) {
        its_timeouts.push_back(distribution(e));
        calls[var] = false;
    }
    std::stringstream buf;
    for (int var = 0; var < number_calls; ++var) {
        buf << std::to_string(its_timeouts[var]) << ", ";
        if (!(var % 10)) {
            buf << "\n";
        }
    }
    std::cout << buf.str() << std::endl;

    std::mutex timeout_mutex;
    std::condition_variable timeout_condition;
    std::unique_lock<std::mutex> timeout_lock(timeout_mutex);
    std::atomic<bool> timeout_occured(false);
    std::thread timeoutthread([&](){
        for (int iteration = 0; iteration < number_iterations; ++iteration) {
            CommonAPI::CallInfo its_info(600000);
            for (std::uint32_t its_timeout = 0; its_timeout < its_timeouts.size(); ++its_timeout) {
                testProxy_->testMethodBlockingAsync(its_timeouts[its_timeout],
                        std::bind(myCallback,std::placeholders::_1, its_timeout), &its_info);
            }

            for (;;) {
                bool finished(false);
                {
                    std::lock_guard<std::mutex> its_lock(its_mutex);
                    finished =
                            std::all_of(calls.cbegin(), calls.cend(),
                                    [](const std::map<std::uint32_t, bool>::value_type &v) {
                                        return v.second;
                                    });
                }
                if (finished) {
                    break;
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    if (timeout_occured) {
                        break;
                    }
                }
            }
            if (timeout_occured) {
                break;
            }
            std::cout << "Finished iteration " << std::dec << iteration << std::endl;
            for (std::uint32_t var = 0; var < its_timeouts.size(); ++var) {
                calls[var]= false;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        {
            std::lock_guard<std::mutex> timeout_lock2(timeout_mutex);
            timeout_condition.notify_one();
        }
    });

    if (std::cv_status::timeout == timeout_condition.wait_for(timeout_lock,
                    std::chrono::milliseconds(number_calls * number_iterations
                                    * maximum_block_time))) {
        ADD_FAILURE() << "Didn't receive all responses within time";
        timeout_occured = true;
    }
    timeout_lock.unlock();

    if (timeoutthread.joinable()) {
        timeoutthread.join();
    }
}

/**
* @test Call test method and block in registered callback when processing
* responses. Check that all responses are delivered.
*/

TEST_F(CMBlockingCalls, BlockInProxyCallback) {
    std::mutex its_mutex;
    std::map<std::uint32_t, bool> calls;

    std::function<void (const CommonAPI::CallStatus&,
            const std::uint8_t&, std::uint32_t)> myCallback =
            [&](const CommonAPI::CallStatus& callStatus,
                    const std::uint8_t& _y, std::uint32_t _callNumber) {
        EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
        std::this_thread::sleep_for(std::chrono::milliseconds(_y*10));
        std::lock_guard<std::mutex> its_lock(its_mutex);
        calls[_callNumber] = true;
    };

    const int number_calls = 20;
    const int number_iterations = 5;
    const int minimum_block_time = 10;
    const int maximum_block_time = 200;

    std::random_device r;
    std::mt19937 e(r());
    std::uniform_int_distribution<std::uint32_t> distribution(
            minimum_block_time, maximum_block_time);
    std::vector<std::uint8_t> its_timeouts;
    for (int var = 0; var < number_calls; ++var) {
        its_timeouts.push_back(static_cast<std::uint8_t>(distribution(e)));
        calls[var] = false;
    }
    std::stringstream buf;
    for (int var = 0; var < number_calls; ++var) {
        buf << std::to_string(its_timeouts[var]) << ", ";
        if (!(var % 10)) {
            buf << "\n";
        }
    }
    std::cout << buf.str() << std::endl;

    std::mutex timeout_mutex;
    std::condition_variable timeout_condition;
    std::unique_lock<std::mutex> timeout_lock(timeout_mutex);
    std::atomic<bool> timeout_occured(false);
    std::thread timeoutthread([&](){
        for (int iteration = 0; iteration < number_iterations; ++iteration) {
            CommonAPI::CallInfo its_info(600000);
            for (std::uint32_t var = 0; var < its_timeouts.size(); ++var) {
                    testProxy_->testMethodAsync(its_timeouts[var],
                            std::bind(myCallback,
                                      std::placeholders::_1,
                                      std::placeholders::_2, var), &its_info);
            }

            for (;;) {
                bool finished(false);
                {
                    std::lock_guard<std::mutex> its_lock(its_mutex);
                    finished =
                            std::all_of(calls.cbegin(), calls.cend(),
                                [](const std::map<std::uint32_t, bool>::value_type &v) {
                                    return v.second;
                                });
                }
                if (finished) {
                    break;
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    if (timeout_occured) {
                        break;
                    }
                }
            }
            if (timeout_occured) {
                break;
            }
            std::cout << "Finished iteration " << std::dec << iteration << std::endl;
            for (std::uint32_t var = 0; var < its_timeouts.size(); ++var) {
                calls[var]= false;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        {
            std::lock_guard<std::mutex> timeout_lock2(timeout_mutex);
            timeout_condition.notify_one();
        }
    });

    if (std::cv_status::timeout == timeout_condition.wait_for(timeout_lock,
                    std::chrono::milliseconds(number_calls * number_iterations
                                    * maximum_block_time * 10))) {
        ADD_FAILURE() << "Didn't receive all responses within time";
        timeout_occured = true;
    }
    timeout_lock.unlock();

    if (timeoutthread.joinable()) {
        timeoutthread.join();
    }
}

/**
* @test Register availability handler which blocks and (de)register the
* corresponding service multiple times. After the serice stays available do
* a method call and check that the answer is received
*/

TEST_F(CMBlockingCalls, BlockInAvailabilityHandler) {
    const int msToBlock = 150;

    testProxy2_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress2, clientId2);
    ASSERT_TRUE((bool)testProxy2_);

    std::atomic<CommonAPI::AvailabilityStatus> last_reported_availability_status(
            CommonAPI::AvailabilityStatus::UNKNOWN);
    std::atomic<std::uint32_t> handler_called(0);
    std::promise<bool> registrationTogglePromise;
    std::future<bool> registrationToggleFuture = registrationTogglePromise.get_future();

    CommonAPI::Event<CommonAPI::AvailabilityStatus>::Subscription itsSubscription =
            testProxy2_->getProxyStatusEvent().subscribe(
            [&](const CommonAPI::AvailabilityStatus& _status) {

        //wait until toggling between unregister/register is finished
        if (std::future_status::timeout ==
                registrationToggleFuture.wait_for(std::chrono::milliseconds(10000))) {
            ADD_FAILURE() << "Toggling between unregister/register wasn't done within time";
        } 

        #if 0
        std::cout << "Handler called with "
                << ((_status == CommonAPI::AvailabilityStatus::AVAILABLE) ?
                        " available " : " not available" ) << std::endl;
        #endif
        EXPECT_NE(last_reported_availability_status, _status);
        last_reported_availability_status = _status;
        std::this_thread::sleep_for(std::chrono::milliseconds(msToBlock));
        handler_called++;
    });

    std::uint32_t number_service_registration(10);
    for (std::uint32_t var = 0; var < number_service_registration; ++var) {
        runtime_->unregisterService(domain, CMMethodCallsStub::StubInterface::getInterface(), testAddress2);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // offer service again
        testStub2_ = std::make_shared<CMMethodCallsStub>();
        bool serviceRegistered = runtime_->registerService(domain, testAddress2, testStub2_, serviceId);
        ASSERT_TRUE(serviceRegistered);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    registrationTogglePromise.set_value(true);
    int counter = number_service_registration * 2 + 10;
    while (handler_called < number_service_registration*2 && counter < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(msToBlock));
        counter++;
    }
    EXPECT_GE(handler_called, number_service_registration*2);
    EXPECT_EQ(CommonAPI::AvailabilityStatus::AVAILABLE, last_reported_availability_status);
    CommonAPI::CallStatus cs(CommonAPI::CallStatus::UNKNOWN);
    std::uint8_t its_reply(0);
    testProxy2_->testMethod(23,cs,its_reply);
    EXPECT_EQ(23, its_reply);
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, cs);
    testProxy2_->getProxyStatusEvent().unsubscribe(itsSubscription);
}

/**
* @test Create proxy to service and wait until it is reported as available via
* a registered availability handler. As soon as it is available start sending
* requests to the service and wait for its replies. Check that the replies for
* this requests are dispatched even if the availablity handler for this service
* is blocked. This is tested through blocking in the availability handler
* after the main thread was notified about the the services' availability
*/

TEST_F(CMBlockingCalls, BlockInAvailabilityHandlerAndReceiveCallbacks) {
    std::mutex itsCallMutex;
    std::map<std::uint32_t, bool> calls;

    std::atomic<std::uint32_t> availabilityHandlerCalled(0);

    std::function<
            void(const CommonAPI::CallStatus&, const std::uint8_t&,
                 std::uint32_t)> myCallback =
            [&availabilityHandlerCalled, &itsCallMutex, &calls](
                    const CommonAPI::CallStatus& callStatus,
                    const std::uint8_t& _y, std::uint32_t _callNumber) {
        EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
        EXPECT_GT(availabilityHandlerCalled, 0u);
        std::this_thread::sleep_for(std::chrono::milliseconds(_y*10));
        std::lock_guard<std::mutex> its_lock(itsCallMutex);
        calls[_callNumber] = true;
    };

    const int number_calls = 20;
    const int number_iterations = 2;
    const int minimum_block_time = 10;
    const int maximum_block_time = 200;
    const int block_time_availability = 3000;

    std::random_device r;
    std::mt19937 e(r());
    std::uniform_int_distribution<std::uint32_t> distribution(
            minimum_block_time, maximum_block_time);
    std::vector<std::uint8_t> its_timeouts;
    for (int var = 0; var < number_calls; ++var) {
        its_timeouts.push_back(static_cast<std::uint8_t>(distribution(e)));
        calls[var] = false;
    }
    std::stringstream buf;
    for (int var = 0; var < number_calls; ++var) {
        buf << std::to_string(its_timeouts[var]) << ", ";
        if (!(var % 10)) {
            buf << "\n";
        }
    }
    std::cout << buf.str() << std::endl;

    testProxy2_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress2, clientId2);
    ASSERT_TRUE((bool)testProxy2_);

    std::promise<bool> availabilityPromise;
    std::atomic<CommonAPI::AvailabilityStatus> last_reported_availability_status(
            CommonAPI::AvailabilityStatus::UNKNOWN);

    CommonAPI::Event<CommonAPI::AvailabilityStatus>::Subscription itsSubscription =
             testProxy2_->getProxyStatusEvent().subscribe(
             [&](const CommonAPI::AvailabilityStatus& _status) {
         EXPECT_NE(last_reported_availability_status, _status);
         last_reported_availability_status = _status;
         availabilityHandlerCalled++;
         // notify about availability but don't return from handler to ensure
         // that replies arrive before availability handler returned from usercode
         if (_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
             availabilityPromise.set_value(true);
         }
         if (_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
             std::this_thread::sleep_for(std::chrono::milliseconds(block_time_availability));
         }
         #if 0
         std::cout << "\nHandler called with "
                 << ((_status == CommonAPI::AvailabilityStatus::AVAILABLE) ?
                         " available " : " not available" ) << std::endl;
         #endif
     });

    ASSERT_FALSE(testProxy2_->isAvailable());
    // offer second service
    testStub2_ = std::make_shared<CMMethodCallsStub>();
    bool serviceRegistered = runtime_->registerService(domain, testAddress2, testStub2_, serviceId);
    ASSERT_TRUE(serviceRegistered);


    std::mutex timeoutMutex;
    std::condition_variable timeoutCondition;
    std::unique_lock<std::mutex> timeoutLock(timeoutMutex);
    std::atomic<bool> timeoutOccured(false);
    std::thread timeoutthread([&](){
        // wait until service is marked as available
        if (std::future_status::timeout ==
                availabilityPromise.get_future().wait_for(std::chrono::milliseconds(10000))) {
            ADD_FAILURE() << "Service didn't become available within time";
        } else {
            std::cout << "Service is available start sending." << std::endl;
        }
        for (int iteration = 0; iteration < number_iterations; ++iteration) {
            CommonAPI::CallInfo its_info(600000);
            for (std::uint32_t its_timeout = 0; its_timeout < its_timeouts.size(); ++its_timeout) {
                if (testProxy2_->isAvailable()) {
                    testProxy2_->testMethodAsync(its_timeouts[its_timeout],
                            std::bind(myCallback,
                                      std::placeholders::_1,
                                      std::placeholders::_2, its_timeout), &its_info);
                } else {
                    ADD_FAILURE() << "Proxy isn't available";
                }
            }

            for (;;) {
                bool finished(false);
                {
                    std::lock_guard<std::mutex> its_lock(itsCallMutex);
                    finished =
                            std::all_of(calls.cbegin(), calls.cend(),
                                [](const std::map<std::uint32_t, bool>::value_type &v) {
                                    return v.second;
                                });
                }
                if (finished) {
                    break;
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    if (timeoutOccured) {
                        break;
                    }
                }
            }
            if (timeoutOccured) {
                break;
            }
            std::cout << "Finished iteration " << std::dec << iteration << std::endl;
            for (std::uint32_t var = 0; var < its_timeouts.size(); ++var) {
                calls[var]= false;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        {
            std::lock_guard<std::mutex> timeout_lock2(timeoutMutex);
            timeoutCondition.notify_one();
        }
    });

    std::uint32_t waitTime = (number_calls * number_iterations
            * maximum_block_time * 10) + block_time_availability;
    if (std::cv_status::timeout == timeoutCondition.wait_for(timeoutLock,
                    std::chrono::milliseconds(waitTime))) {
        ADD_FAILURE() << "Didn't receive all responses within time";
        timeoutOccured = true;
    }
    timeoutLock.unlock();
    testProxy2_->getProxyStatusEvent().unsubscribe(itsSubscription);

    if (timeoutthread.joinable()) {
        timeoutthread.join();
    }
}

/**
* @test Call test method which generates blocking calls on stub. Ensure working
* dispatching even if main dispatch thread still blocked after a dispatch thread
* was spawned and joined again because another dispatch thread returned from
* the usercode in the meanwhile.
*/
TEST_F(CMBlockingCalls, NestedBlockInStubMethods) {

    CommonAPI::CallInfo its_info(600000);

    // block "main_dispatch"
    testProxy_->testMethodBlockingAsync(1500, [] (const CommonAPI::CallStatus& callStatus) {
        (void)callStatus;
    }, &its_info);

    // block "2nd dispatch"
    testProxy_->testMethodBlockingAsync(250, [] (const CommonAPI::CallStatus& callStatus) {
        (void)callStatus;
    }, &its_info);

    uint8_t x = 5;
    uint8_t y = 0;
    std::mutex stMutex;
    CommonAPI::CallStatus st(CommonAPI::CallStatus::UNKNOWN);
    testProxy_->getTestAttributeAttribute().setValue(x, st, y);
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, st);

    // ensure that 2nd dispatch thread returned before  doing async call
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::promise<void> asyncAnswerReceived;
    CommonAPI::CallInfo its_info2(500);
    testProxy_->testMethodAsync(x, [&asyncAnswerReceived, &st, &stMutex] (const CommonAPI::CallStatus& callStatus, uint8_t y) {
        (void)y;
        {
            std::lock_guard<std::mutex> lock(stMutex);
            st = callStatus;
        }
         EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus);
         asyncAnswerReceived.set_value();
    }, &its_info2);

    if (std::future_status::timeout == asyncAnswerReceived.get_future().wait_for(std::chrono::milliseconds(5000))) {
        ADD_FAILURE() << "Didn't receive asnyAnswer within time";
    } else {
        std::lock_guard<std::mutex> lock(stMutex);
        EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, st);
    }
}


#endif /* #ifndef TESTS_BAT */

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
