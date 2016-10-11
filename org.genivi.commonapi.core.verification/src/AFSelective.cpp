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
const int tasync = 10000;

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
        
        testStub_ = std::make_shared<AFSelectiveStub>();
        bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);
        int i = 0;
        while(!testProxy_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy_->isAvailable());
        
        testProxy2_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, otherclientId);
        i = 0;
        while(!testProxy2_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy2_->isAvailable());        
    }

    void TearDown() {
        bool serviceUnregistered =
                runtime_->unregisterService(domain, AFSelectiveStub::StubInterface::getInterface(),
                        testAddress);

         ASSERT_TRUE(serviceUnregistered);

         // wait that proxies are not available
         int counter = 0;  // counter for avoiding endless loop
         while ( testProxy_->isAvailable() && counter < 100 ) {
             std::this_thread::sleep_for(std::chrono::milliseconds(10));
             counter++;
         }
         ASSERT_FALSE(testProxy_->isAvailable());

         counter = 0;  // counter for avoiding endless loop
         while ( testProxy2_->isAvailable() && counter < 100 ) {
             std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(subStatus, CommonAPI::CallStatus::SUBSCRIPTION_REFUSED);

    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;   
    testProxy_->testMethod(in_, callStatus, out_);  

    std::this_thread::sleep_for(std::chrono::microseconds(tasync));
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
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);
    
    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;   
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
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
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);

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
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);
    
    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;   
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    uint8_t expected = 1;
    EXPECT_EQ(expected, result);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result2 != 0) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
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

/**
* @test Test Destruction of Proxies but service stay online
* There were an issue when a proxy which has nevery subscribed gets
* destructed with SomeIP binding (GLIPCI-1081). Therefore i added this
* test case.
*/
TEST_F(AFSelective, ProxyBuildAndDestroy) {

    {
        std::shared_ptr<TestInterfaceProxy<>> bad_proxy =
                runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, "blub");

        bad_proxy->isAvailableBlocking();

        std::shared_ptr<TestInterfaceProxy<>> proxy =
                runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, "blub");
        proxy->isAvailableBlocking();

        uint8_t result = 0;
        CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
        proxy->getBTestSelectiveSelectiveEvent().subscribe([&](
            const uint8_t &y
        ) {
            result = y;
        },
        [&](
            const CommonAPI::CallStatus &status
        ) {
            callStatus = status;
        });

        while (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        // send value '3' via a method call - this tells stub to broadcast through the selective bc

        uint8_t in = 3;
        uint8_t out = 0;
        proxy->testMethod(in, callStatus, out);

        // check that value was correctly received
        for (int i = 0; i < 100; i++) {
            if (result != 0) break;
            std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        }
        uint8_t expected = 1;
        EXPECT_EQ(expected, result);
    }

    {
        std::shared_ptr<TestInterfaceProxy<>> proxy =
                runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, "blub");
        proxy->isAvailableBlocking();

        uint8_t result;
        CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
        proxy->getBTestSelectiveSelectiveEvent().subscribe([&](
            const uint8_t &y
        ) {
            result = y;
        },
        [&](
            const CommonAPI::CallStatus &status
        ) {
            callStatus = status;
        });

        while (callStatus != CommonAPI::CallStatus::SUCCESS) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // send value '3' via a method call - this tells stub to broadcast through the selective bc
        result = 0;
        uint8_t in = 3;
        uint8_t out = 0;
        proxy->testMethod(in, callStatus, out);

        // check that value was correctly received
        for (int i = 0; i < 100; i++) {
            if (result != 0) break;
            std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        }
        uint8_t expected = 1;
        EXPECT_EQ(expected, result);
    }
}
 
TEST_F(AFSelective, SelectiveRejectedMultiBroadcast) {

    CommonAPI::CallStatus callStatus;
    CommonAPI::CallStatus subStatus;
    uint8_t result = 0;
    uint8_t result2 = 0;
    uint8_t result3 = 0;     
    uint8_t in_ = 0;
    uint8_t out_ = 0;

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
        if (subStatus == CommonAPI::CallStatus::SUBSCRIPTION_REFUSED) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(subStatus, CommonAPI::CallStatus::SUBSCRIPTION_REFUSED);

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
        if (subStatus == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);

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
        if (subStatus == CommonAPI::CallStatus::SUCCESS) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);

    // send value '3' via a method call - this tells stub to broadcast through the selective bc
    result = 0;
    in_ = 3;
    out_ = 0;   
    testProxy_->testMethod(in_, callStatus, out_);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0 && result3 != 0) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    uint8_t expected = 1;
    EXPECT_EQ(expected, result);
    uint8_t expected2 = 0;
    EXPECT_EQ(expected2, result2);
    uint8_t expected3 = 1;
    EXPECT_EQ(expected3, result3);
}

TEST_F(AFSelective, Multiple_Subscriptions_SameConnection_CallErrorHandler) {
    std::shared_ptr<TestInterfaceProxy<>> my_proxy1 =
            runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, "blub");
    my_proxy1->isAvailableBlocking();

    std::shared_ptr<TestInterfaceProxy<>> my_proxy2 =
            runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, "blub");
    my_proxy2->isAvailableBlocking();

    std::shared_ptr<TestInterfaceProxy<>> my_proxy3 =
            runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, "blub");
    my_proxy3->isAvailableBlocking();

    uint32_t counter = 0;
    uint8_t result1 = 0;
    CommonAPI::CallStatus callStatus1 = CommonAPI::CallStatus::UNKNOWN;
    CommonAPI::CallStatus callStatus11 = CommonAPI::CallStatus::UNKNOWN;
    my_proxy1->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result1 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        callStatus1 = status;
        ++counter;
    });
    uint8_t result11 = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    my_proxy1->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result11 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        callStatus11 = status;
        ++counter;
    });

    uint8_t result2 = 0;
    CommonAPI::CallStatus callStatus2 = CommonAPI::CallStatus::UNKNOWN;
    CommonAPI::CallStatus callStatus22 = CommonAPI::CallStatus::UNKNOWN;
    my_proxy2->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result2 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        callStatus2 = status;
        ++counter;
    });
    uint8_t result22 = 0;
    my_proxy2->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result22 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        callStatus22 = status;
        ++counter;
    });
    uint8_t result3 = 0;
    CommonAPI::CallStatus callStatus3 = CommonAPI::CallStatus::UNKNOWN;
    my_proxy3->getBTestSelectiveSelectiveEvent().subscribe([&](
        const uint8_t &y
    ) {
        result3 = y;
    },
    [&](
        const CommonAPI::CallStatus &status
    ) {
        callStatus3 = status;
        ++counter;
    });

    uint32_t count = 0;
    while (callStatus1 != CommonAPI::CallStatus::SUCCESS ||
            callStatus11 != CommonAPI::CallStatus::SUCCESS ||
            callStatus2 != CommonAPI::CallStatus::SUCCESS ||
            callStatus22 != CommonAPI::CallStatus::SUCCESS ||
            callStatus3 != CommonAPI::CallStatus::SUCCESS) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        if (count++ == 10000) {
            break;
        }
    }
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus1);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus11);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus2);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus22);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, callStatus3);
    ASSERT_EQ(5u, counter);

    uint8_t in_ = 3;
    uint8_t out_ = 0;
    CommonAPI::CallStatus methodStatus;
    testProxy_->testMethod(in_, methodStatus, out_);

    count = 0;
    while (result1 != 1 ||
            result11 != 1 ||
            result2 != 1 ||
            result22 != 1 ||
            result3 != 1) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        if (count++ == 10000) {
            break;
        }
    }
    uint8_t expected = 1;
    EXPECT_EQ(expected, result1);
    EXPECT_EQ(expected, result11);
    EXPECT_EQ(expected, result2);
    EXPECT_EQ(expected, result22);
    EXPECT_EQ(expected, result3);
}

TEST_F(AFSelective, Fire_Selective_Within_Subscription_Changed_Hook) {
    CommonAPI::CallStatus callStatus, subStatus;
    uint8_t out_ = 0;
    uint8_t result = 0;

    // send value '5' via a method call - this tells stub to fire in changed hook
    uint8_t in_ = 5;
    testProxy2_->testMethod(in_, callStatus, out_);

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

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result != 0 && subStatus != CommonAPI::CallStatus::UNKNOWN) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }

    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);
    EXPECT_EQ(1, result);

    // No unregister the service and register again to ensure
    // fire an event from the changed hook still works

    subStatus = CommonAPI::CallStatus::UNKNOWN;
    result = 0;

    bool serviceUnregistered =
            runtime_->unregisterService(domain, AFSelectiveStub::StubInterface::getInterface(),
                    testAddress);
     ASSERT_TRUE(serviceUnregistered);

     std::this_thread::sleep_for(std::chrono::microseconds(tasync));

     bool serviceRegistered = runtime_->registerService(domain, testAddress, testStub_, serviceId);
     ASSERT_TRUE(serviceRegistered);

     // check that value was correctly received
     for (int i = 0; i < 100; i++) {
         if (result != 0 && subStatus != CommonAPI::CallStatus::UNKNOWN) break;
         std::this_thread::sleep_for(std::chrono::microseconds(tasync));
     }

     EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, subStatus);
     EXPECT_EQ(1, result);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
