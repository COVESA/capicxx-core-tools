// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/**
* @file advanced/selective
*/

#include <functional>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <fstream>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/advanced/bselective/TestInterfaceProxy.hpp"
#include "stub/AFSelectiveStub.h"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";
const std::string otherclientId = "other-client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.advanced.bselective.TestInterface";
const int tasync = 100000;

using namespace v1_0::commonapi::advanced::bselective;

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class AFSelective: public ::testing::Test {

public:

    void recvSubscribedValue(uint8_t y) {
        value_ = y;
    }

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);
        std::mutex availabilityMutex;
        std::mutex availabilityMutex2;
        std::unique_lock<std::mutex> lock(availabilityMutex);
        std::unique_lock<std::mutex> lock2(availabilityMutex2);
        std::condition_variable cv;
        std::condition_variable cv2;        
        bool proxyAvailable = false;
        bool proxy2Available = false;

        std::thread t1([this, &proxyAvailable, &cv, &availabilityMutex]() {
            std::lock_guard<std::mutex> lock(availabilityMutex);
            testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
            testProxy_->isAvailableBlocking();
            ASSERT_TRUE((bool)testProxy_);
            proxyAvailable = true;
            cv.notify_one();
        });
        
        std::thread t2([this, &proxy2Available, &cv2, &availabilityMutex2]() {
            std::lock_guard<std::mutex> lock2(availabilityMutex2);
            testProxy2_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, otherclientId);
            testProxy2_->isAvailableBlocking();
            ASSERT_TRUE((bool)testProxy2_);
            proxy2Available = true;
            cv2.notify_one();
        });        
        
        testStub_ = std::make_shared<AFSelectiveStub>();
        bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        while(!proxyAvailable) {
            cv.wait(lock);
        }
        t1.join();
        ASSERT_TRUE(testProxy_->isAvailable());
        
        while(!proxy2Available) {
            cv2.wait(lock2);
        }
        t2.join();
        ASSERT_TRUE(testProxy2_->isAvailable());        
    }

    void TearDown() {
        bool serviceUnregistered =
                runtime_->unregisterService(domain, AFSelectiveStub::StubInterface::getInterface(),
                        testAddress);

         ASSERT_TRUE(serviceUnregistered);

         // wait that proxies are not available
         int counter = 0;  // counter for avoiding endless loop
         while ( testProxy_->isAvailable() && counter < 10 ) {
             std::this_thread::sleep_for(std::chrono::milliseconds(100));
             counter++;
         }
         ASSERT_FALSE(testProxy_->isAvailable());

         counter = 0;  // counter for avoiding endless loop
         while ( testProxy2_->isAvailable() && counter < 10 ) {
             std::this_thread::sleep_for(std::chrono::milliseconds(100));
             counter++;
         }
         ASSERT_FALSE(testProxy2_->isAvailable());
    }

    uint8_t value_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<AFSelectiveStub> testStub_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<TestInterfaceProxy<>> testProxy2_;        
};


/**
* @test Test selective broadcasts.
*  - inform stub to stop accepting subscriptions
*  - try to subscribe to the selective broadcast
*  - check that an error was received
*  - inform stub to send a broadcast
*  - check that nothing was received in a reasonable time
*/
TEST_F(AFSelective, SelectiveBroadcastRejected) {

    CommonAPI::CallStatus callStatus;
    CommonAPI::CallStatus subStatus;
    uint8_t result = 0;
    uint8_t in_ = 0;
    uint8_t out_ = 0;
       
    // send value '2' via a method call - this tells stub to stop accepting subs
    in_ = 2;
    testProxy_->testMethod(in_, callStatus, out_);    
    
    // subscribe
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });    
    
    // check that subscription failed correctly
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    // EXPECT_EQ(subStatus, CommonAPI::CallStatus::SUBSCRIPTION_REFUSED); // Not supported by SOME/IP yet.    

    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;   
    testProxy_->testMethod(in_, callStatus, out_);  
    
    // check that no value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        usleep(10000);
    }
    uint8_t expected = 0;
    EXPECT_EQ(expected, result);

}

/**
* @test Test selective broadcasts.
*  - inform stub to start accepting subscriptions
*  - subscribe to the selective broadcast
*  - check that no error was received (in a reasonable time)
*  - inform stub to send a broadcast
*  - check that a correct value is received
*/
TEST_F(AFSelective, SelectiveBroadcast) {

    CommonAPI::CallStatus callStatus;
    CommonAPI::CallStatus subStatus;
    uint8_t result = 0;
    uint8_t in_ = 0;
    uint8_t out_ = 0;

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;   
    testProxy_->testMethod(in_, callStatus, out_);
    
    // subscribe
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });
    
    // check that no error was received
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    EXPECT_EQ(CommonAPI::CallStatus::UNKNOWN, subStatus);
    
    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;   
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        usleep(10000);
    }
    uint8_t expected = 1;
    EXPECT_EQ(expected, result);
}

/**
* @test Test multiple selective broadcasts.
*  - inform stub to start accepting subscriptions
*  - subscribe to the selective broadcast
*  - check that no error was received (in a reasonable time)
*  - inform stub to send a broadcast
*  - check that a correct value is received
*/
TEST_F(AFSelective, SelectiveMultiBroadcast) {

    CommonAPI::CallStatus callStatus;
    CommonAPI::CallStatus subStatus;
    uint8_t result = 0;
    uint8_t result2 = 0;    
    uint8_t in_ = 0;
    uint8_t out_ = 0;

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;   
    testProxy_->testMethod(in_, callStatus, out_);
    
    // subscribe
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });
    
    // check that no error was received
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    EXPECT_EQ(CommonAPI::CallStatus::UNKNOWN, subStatus);

    // subscribe from another proxy
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy2_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result2 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });
    
    // check that no error was received
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    EXPECT_EQ(CommonAPI::CallStatus::UNKNOWN, subStatus);
    
    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;   
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        usleep(10000);
    }
    uint8_t expected = 1;
    EXPECT_EQ(expected, result);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result2 != 0) break;
        usleep(10000);
    }
    uint8_t expected2 = 1;
    EXPECT_EQ(expected2, result2);
}

/**
* @test Test multiple selective broadcasts, with rejection.
*  - subscribe to stub three times:
*    once from proxy2,
*    once from proxy1 (accepted)
*    once from proxy2 (rejected)
*  - This should result with two subscription callbacks being called from broadcast.
*/

/* 
 * This test currently fails because a single proxy can only subscribe once.
 * When proxy subscribes the second time, the stub is not notified.
 * Therefore, whatever the stub did with the first subscription (rejected / accepted)
 * will also hold for the second subscription.
 */
 
TEST_F(AFSelective, DISABLED_SelectiveRejectedMultiBroadcast) {

    CommonAPI::CallStatus callStatus;
    CommonAPI::CallStatus subStatus;
    uint8_t result = 0;
    uint8_t result2 = 0;
    uint8_t result3 = 0;     
    uint8_t in_ = 0;
    uint8_t out_ = 0;

    // send value '4' via a method call - this tells stub to start accepting subs
    in_ = 4;   
    testProxy_->testMethod(in_, callStatus, out_);
    
    // subscribe
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });
    
    // check that no error was received
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    EXPECT_EQ(CommonAPI::CallStatus::UNKNOWN, subStatus);

    // subscribe from another proxy
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy2_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result3 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });
    
    // check that no error was received
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    EXPECT_EQ(CommonAPI::CallStatus::UNKNOWN, subStatus);
    
    // send value '2' via a method call - this tells stub to stop accepting subs
    in_ = 2;
    testProxy2_->testMethod(in_, callStatus, out_);    
    
    // subscribe
    subStatus = CommonAPI::CallStatus::UNKNOWN;
    testProxy2_->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result2 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        subStatus = status;
    });    
    
    // check that subscription failed correctly
    for (int i = 0; i < 100; i++) {
        if (subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        usleep(10000);
    }
    // EXPECT_EQ(subStatus, CommonAPI::CallStatus::SUBSCRIPTION_REFUSED); // Not supported by SOME/IP yet.    

    
    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;   
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        usleep(10000);
    }
    uint8_t expected = 1;
    EXPECT_EQ(expected, result);
    uint8_t expected2 = 0;
    EXPECT_EQ(expected2, result2);
    uint8_t expected3 = 1;
    EXPECT_EQ(expected3, result3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
