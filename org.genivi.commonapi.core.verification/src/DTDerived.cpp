/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file DataTypes
*/

#include <functional>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <fstream>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"

#include "v1_0/commonapi/datatypes/derived/TestInterfaceProxy.hpp"
#include "stub/DTDerivedStub.h"

const std::string domain = "local";
const std::string testAddress = "commonapi.datatypes.derived.TestInterface";
const std::string connectionId_client = "client-sample";
const std::string connectionId_service = "service-sample";

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class DTDerived: public ::testing::Test {

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);
        std::mutex availabilityMutex;
        std::unique_lock<std::mutex> lock(availabilityMutex);
        std::condition_variable cv;
        bool proxyAvailable = false;

        std::thread t1([this, &proxyAvailable, &cv, &availabilityMutex]() {
            testProxy_ = runtime_->buildProxy<v1_0::commonapi::datatypes::derived::TestInterfaceProxy>(domain, testAddress, connectionId_client);
            testProxy_->isAvailableBlocking();
            std::lock_guard<std::mutex> lock(availabilityMutex);
            ASSERT_TRUE((bool)testProxy_);
            proxyAvailable = true;
            cv.notify_one();
        });

        testStub_ = std::make_shared<v1_0::commonapi::datatypes::derived::DTDerivedStub>();
        serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, connectionId_service);
        ASSERT_TRUE(serviceRegistered_);

        while(!proxyAvailable) {
            cv.wait(lock);
        }
        t1.join();
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
		runtime_->unregisterService(domain, v1_0::commonapi::datatypes::derived::DTDerivedStub::StubInterface::getInterface(), testAddress);
    }

    bool received_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::DTDerivedStub> testStub_;

};

/*
* @test Test function call with derived types
*   - Derived types are: extended struct, extended enumeration, extended union, polymorphic struct
*   - Function call of a function that has for each derived type one argument (test values) and one return value
*   - The stub copies the test values to the return values
*   - On client side the test values are compared with the return values
*/
TEST_F(DTDerived, SendAndReceive) {

    CommonAPI::CallStatus callStatus;

    v1_0::commonapi::datatypes::derived::TestInterface::tStructExt structExtTestValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt enumExtTestValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt unionExtTestValue;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct> baseStructTestValue;

    structExtTestValue.setBaseMember(42);
    structExtTestValue.setExtendedMember("Hello World");

    enumExtTestValue = v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt::VALUE2;

    std::string u = "Hello World";
    unionExtTestValue = u;

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_two = std::make_shared<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>();
    l_two->setName("ABC");
    baseStructTestValue = l_two;

    v1_0::commonapi::datatypes::derived::TestInterface::tStructExt structExtResultValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt enumExtResultValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt unionExtResultValue;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct> baseStructResultValue;

    testProxy_->fTest(
            structExtTestValue,
            enumExtTestValue,
            unionExtTestValue,
            baseStructTestValue,
            callStatus,
            structExtResultValue,
            enumExtResultValue,
            unionExtResultValue,
            baseStructResultValue
    );

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(structExtTestValue, structExtResultValue);
    EXPECT_EQ(enumExtTestValue, enumExtResultValue);
    EXPECT_EQ(unionExtTestValue, unionExtResultValue);

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_twoResult =
            std::dynamic_pointer_cast<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>(baseStructResultValue);
    EXPECT_EQ(l_twoResult->getName(), "ABC");
}

/**
* @test Test attribute functions with derived types
*   - Call set function of attributes with derived types
*   - Call get function and check if the return value is the same
*/
TEST_F(DTDerived, AttributeSet) {

    CommonAPI::CallStatus callStatus;

    v1_0::commonapi::datatypes::derived::TestInterface::tStructExt structExtTestValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt enumExtTestValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt unionExtTestValue;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct> baseStructTestValue;

    structExtTestValue.setBaseMember(42);
    structExtTestValue.setExtendedMember("Hello World");

    enumExtTestValue = v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt::VALUE2;

    std::string u = "Hello World";
    unionExtTestValue = u;

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_two = std::make_shared<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>();
    l_two->setName("ABC");
    baseStructTestValue = l_two;

    v1_0::commonapi::datatypes::derived::TestInterface::tStructExt structExtResultValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt enumExtResultValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt unionExtResultValue;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct> baseStructResultValue;

    testProxy_->getAStructExtAttribute().setValue(structExtTestValue, callStatus, structExtResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(structExtTestValue, structExtResultValue);

    testProxy_->getAEnumExtAttribute().setValue(enumExtTestValue, callStatus, enumExtResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(enumExtTestValue, enumExtResultValue);

    testProxy_->getAUnionExtAttribute().setValue(unionExtTestValue, callStatus, unionExtResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(unionExtTestValue, unionExtResultValue);

    testProxy_->getABaseStructAttribute().setValue(baseStructTestValue, callStatus, baseStructResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_twoResult =
            std::dynamic_pointer_cast<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>(baseStructResultValue);
    EXPECT_EQ(l_twoResult->getName(), "ABC");

}

/**
* @test Test broadcast with derived types
*   - Subscribe to broadcast which contains derived types
*   - Call function to cause the stub to fire broadcast event with the same content
*   - Check if the values in the callback function are as expected
*/
TEST_F(DTDerived, BroadcastReceive) {

    CommonAPI::CallStatus callStatus;

    v1_0::commonapi::datatypes::derived::TestInterface::tStructExt structExtTestValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt enumExtTestValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt unionExtTestValue;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct> baseStructTestValue;

    structExtTestValue.setBaseMember(42);
    structExtTestValue.setExtendedMember("Hello World");

    enumExtTestValue = v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt::VALUE2;

    std::string u = "Hello World";
    unionExtTestValue = u;

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_two = std::make_shared<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>();
    l_two->setName("ABC");
    baseStructTestValue = l_two;

    v1_0::commonapi::datatypes::derived::TestInterface::tStructExt structExtResultValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt enumExtResultValue;
    v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt unionExtResultValue;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct> baseStructResultValue;

    received_ = false;
    testProxy_->getBTestEvent().subscribe([&](
            const v1_0::commonapi::datatypes::derived::TestInterface::tStructExt& structExtResultValue,
            const v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt& enumExtResultValue,
            const v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt& unionExtResultValue,
            const std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct>& baseStructResultValue
            ) {
        received_ = true;
        EXPECT_EQ(structExtTestValue, structExtResultValue);
        EXPECT_EQ(enumExtTestValue, enumExtResultValue);
        EXPECT_EQ(unionExtTestValue, unionExtResultValue);

        std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_twoResult =
                std::dynamic_pointer_cast<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>(baseStructResultValue);
        EXPECT_EQ(l_twoResult->getName(), "ABC");
    });

    testProxy_->fTest(
            structExtTestValue,
            enumExtTestValue,
            unionExtTestValue,
            baseStructTestValue,
            callStatus,
            structExtResultValue,
            enumExtResultValue,
            unionExtResultValue,
            baseStructResultValue
    );

    usleep(100000);
    ASSERT_TRUE(received_);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
