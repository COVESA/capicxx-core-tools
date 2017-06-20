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

#include "v1/commonapi/datatypes/derived/TestInterfaceProxy.hpp"
#include "stub/DTDerivedStub.h"

const std::string domain = "local";
const std::string testAddress = "commonapi.datatypes.derived.TestInterface";
const std::string connectionId_client = "client-sample";
const std::string connectionId_service = "service-sample";

const int tasync = 10000;

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

        testStub_ = std::make_shared<v1_0::commonapi::datatypes::derived::DTDerivedStub>();
        serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, connectionId_service);
        ASSERT_TRUE(serviceRegistered_);

        testProxy_ = runtime_->buildProxy<v1_0::commonapi::datatypes::derived::TestInterfaceProxy>(domain, testAddress, connectionId_client);
        ASSERT_TRUE((bool)testProxy_);

        testProxy_->isAvailableBlocking();
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        runtime_->unregisterService(domain, v1_0::commonapi::datatypes::derived::DTDerivedStub::StubInterface::getInterface(), testAddress);

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        if (testProxy_) {
            while ( testProxy_->isAvailable() && counter < 100 ) {
                std::this_thread::sleep_for(std::chrono::microseconds(tasync));
                counter++;
            }

            ASSERT_FALSE(testProxy_->isAvailable());
        }
    }

    std::atomic<bool> received_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::DTDerivedStub> testStub_;

    v1_0::commonapi::datatypes::derived::TestInterface::tStructExt structExtTestValue_;
    v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt enumExtTestValue_;
    v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt unionExtTestValue_;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct> baseStructTestValue_;

    v1_0::commonapi::datatypes::derived::TestInterface::tStructExt structExtResultValue_;
    v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt enumExtResultValue_;
    v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt unionExtResultValue_;
    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct> baseStructResultValue_;

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

    structExtTestValue_.setBaseMember(42);
    structExtTestValue_.setExtendedMember("Hello World");

    enumExtTestValue_ = v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt::VALUE2;

    std::string u = "Hello World";
    unionExtTestValue_ = u;

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_two = std::make_shared<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>();
    l_two->setName("ABC");
    baseStructTestValue_ = l_two;

    testProxy_->fTest(
            structExtTestValue_,
            enumExtTestValue_,
            unionExtTestValue_,
            baseStructTestValue_,
            callStatus,
            structExtResultValue_,
            enumExtResultValue_,
            unionExtResultValue_,
            baseStructResultValue_
    );

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(structExtTestValue_, structExtResultValue_);
    EXPECT_EQ(enumExtTestValue_, enumExtResultValue_);
    EXPECT_EQ(unionExtTestValue_, unionExtResultValue_);

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_twoResult =
            std::dynamic_pointer_cast<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>(baseStructResultValue_);
    EXPECT_EQ(l_twoResult->getName(), "ABC");
}

/**
* @test Test attribute functions with derived types
*   - Call set function of attributes with derived types
*   - Call get function and check if the return value is the same
*/
TEST_F(DTDerived, AttributeSet) {

    CommonAPI::CallStatus callStatus;

    structExtTestValue_.setBaseMember(42);
    structExtTestValue_.setExtendedMember("Hello World");

    std::string u = "Hello World";
    unionExtTestValue_ = u;

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_two = std::make_shared<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>();
    l_two->setName("ABC");
    baseStructTestValue_ = l_two;

    testProxy_->getAStructExtAttribute().setValue(structExtTestValue_, callStatus, structExtResultValue_);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(structExtTestValue_, structExtResultValue_);

    // check initial value of enumeration attribute
    enumExtTestValue_ = v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt::VALUE1; // this is the expected default value
    EXPECT_EQ(enumExtTestValue_, enumExtResultValue_); // the uninitialized enumExtResultValue should have the default value
    enumExtResultValue_ = v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt::VALUE3; // set to some other value
    testProxy_->getAEnumExtAttribute().getValue(callStatus, enumExtResultValue_); // get value of attribute
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(enumExtTestValue_, enumExtResultValue_); // attribute value should default to the initial default value

    enumExtTestValue_ = v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt::VALUE3;
    testProxy_->getAEnumExtAttribute().setValue(enumExtTestValue_, callStatus, enumExtResultValue_);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(enumExtTestValue_, enumExtResultValue_);

    testProxy_->getAUnionExtAttribute().setValue(unionExtTestValue_, callStatus, unionExtResultValue_);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(unionExtTestValue_, unionExtResultValue_);

    testProxy_->getABaseStructAttribute().setValue(baseStructTestValue_, callStatus, baseStructResultValue_);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_twoResult =
            std::dynamic_pointer_cast<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>(baseStructResultValue_);
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
    std::atomic<CommonAPI::CallStatus> subStatus;

    structExtTestValue_.setBaseMember(42);
    structExtTestValue_.setExtendedMember("Hello World");

    enumExtTestValue_ = v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt::VALUE2;

    std::string u = "Hello World";
    unionExtTestValue_ = u;

    std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_two = std::make_shared<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>();
    l_two->setName("ABC");
    baseStructTestValue_ = l_two;

    received_ = false;
    testProxy_->getBTestEvent().subscribe([&](
            const v1_0::commonapi::datatypes::derived::TestInterface::tStructExt& structExtResultValue,
            const v1_0::commonapi::datatypes::derived::TestInterface::tEnumExt& enumExtResultValue,
            const v1_0::commonapi::datatypes::derived::TestInterface::tUnionExt& unionExtResultValue,
            const std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseStruct>& baseStructResultValue
            ) {
        EXPECT_EQ(structExtTestValue_, structExtResultValue);
        EXPECT_EQ(enumExtTestValue_, enumExtResultValue);
        EXPECT_EQ(unionExtTestValue_, unionExtResultValue);

        std::shared_ptr<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct> l_twoResult =
                std::dynamic_pointer_cast<v1_0::commonapi::datatypes::derived::TestInterface::tBaseTwoStruct>(baseStructResultValue);
        EXPECT_EQ(l_twoResult->getName(), "ABC");
        received_ = true;
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

    testProxy_->fTest(
            structExtTestValue_,
            enumExtTestValue_,
            unionExtTestValue_,
            baseStructTestValue_,
            callStatus,
            structExtResultValue_,
            enumExtResultValue_,
            unionExtResultValue_,
            baseStructResultValue_
    );

    for (int i = 0; i < 100; ++i) {
        if (received_) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    ASSERT_TRUE(received_);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
