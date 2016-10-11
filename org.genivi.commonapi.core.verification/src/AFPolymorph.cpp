// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/**
* @file AFPolymorph.cpp
*/

#include <functional>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <fstream>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"

#include "v1/commonapi/advanced/polymorph/TestInterfaceProxy.hpp"
#include "stub/AFPolymorphStub.h"

const std::string domain = "local";
const std::string testAddress = "commonapi.advanced.polymorph.TestInterface";
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

using namespace v1_0::commonapi::advanced::polymorph;

class AFPolymorph: public ::testing::Test {

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStub_ = std::make_shared<AFPolymorphStub>();
        serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, connectionId_service);
        ASSERT_TRUE(serviceRegistered_);

        testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, connectionId_client);
        int i = 0;
        while(!testProxy_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        runtime_->unregisterService(domain, AFPolymorphStub::StubInterface::getInterface(), testAddress);

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 10 ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    bool received_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<AFPolymorphStub> testStub_;

};
/**
 * @test
 *  - Set and get a typedef-type attribute through a polymorphic structure
 *  - verify that the received data matches the transmitted data
 */
TEST_F(AFPolymorph, SetAndGetAttributeTypedef) {

    CommonAPI::CallStatus callStatus;

    auto a1 = std::make_shared<TestInterface::PStructMyTypedef>(-5);
    std::shared_ptr<TestInterface::PStructBase> a2 = std::make_shared<TestInterface::PStructBase>();
    std::shared_ptr<TestInterface::PStructBase> a3 = std::make_shared<TestInterface::PStructBase>();  
    
    a2 = static_cast<std::shared_ptr<TestInterface::PStructBase>>(a1);
    
    testProxy_->getA1Attribute().setValue(a2, callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    testProxy_->getA1Attribute().getValue(callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    
    std::shared_ptr<TestInterface::PStructMyTypedef> sp = 
        std::dynamic_pointer_cast<TestInterface::PStructMyTypedef>(a3);
    
    ASSERT_TRUE(sp != nullptr);
    EXPECT_EQ((int)sp->getId(), -5);

}

/**
 * @test
 *  - Set and get a enum-type attribute through a polymorphic structure
 *  - verify that the received data matches the transmitted data
 */
TEST_F(AFPolymorph, SetAndGetAttributeEnum) {

    CommonAPI::CallStatus callStatus;

    auto a1 = std::make_shared<TestInterface::PStructMyEnum>(TestInterface::MyEnum::Literal::OFF);
    std::shared_ptr<TestInterface::PStructBase> a2 = std::make_shared<TestInterface::PStructBase>();
    std::shared_ptr<TestInterface::PStructBase> a3 = std::make_shared<TestInterface::PStructBase>();  
    
    a2 = static_cast<std::shared_ptr<TestInterface::PStructBase>>(a1);
    
    testProxy_->getA1Attribute().setValue(a2, callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    testProxy_->getA1Attribute().getValue(callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    
    std::shared_ptr<TestInterface::PStructMyEnum> sp = 
        std::dynamic_pointer_cast<TestInterface::PStructMyEnum>(a3);
    
    ASSERT_TRUE(sp != nullptr);
    EXPECT_EQ((int)sp->getStatus(), TestInterface::MyEnum::Literal::OFF);

}

/**
 * @test
 *  - Set and get a uint-type attribute through a polymorphic structure
 *  - verify that the received data matches the transmitted data
 */
TEST_F(AFPolymorph, SetAndGetAttributeUInt) {

    CommonAPI::CallStatus callStatus;

    auto a1 = std::make_shared<TestInterface::PStructUInt8>(123);
    std::shared_ptr<TestInterface::PStructBase> a2 = std::make_shared<TestInterface::PStructBase>();
    std::shared_ptr<TestInterface::PStructBase> a3 = std::make_shared<TestInterface::PStructBase>();  
    
    a2 = static_cast<std::shared_ptr<TestInterface::PStructBase>>(a1);
    
    testProxy_->getA1Attribute().setValue(a2, callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    testProxy_->getA1Attribute().getValue(callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    
    std::shared_ptr<TestInterface::PStructUInt8> sp = 
        std::dynamic_pointer_cast<TestInterface::PStructUInt8>(a3);
    
    ASSERT_TRUE(sp != nullptr);
    EXPECT_EQ((int)sp->getChannel(), 123);

}

/**
 * @test
 *  - Set and get a string-type attribute through a polymorphic structure
 *  - verify that the received data matches the transmitted data
 */
TEST_F(AFPolymorph, SetAndGetAttributeString) {

    CommonAPI::CallStatus callStatus;

    auto a1 = std::make_shared<TestInterface::PStructString>("123");
    std::shared_ptr<TestInterface::PStructBase> a2 = std::make_shared<TestInterface::PStructBase>();
    std::shared_ptr<TestInterface::PStructBase> a3 = std::make_shared<TestInterface::PStructBase>();  
    
    a2 = static_cast<std::shared_ptr<TestInterface::PStructBase>>(a1);
    
    testProxy_->getA1Attribute().setValue(a2, callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    testProxy_->getA1Attribute().getValue(callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    
    std::shared_ptr<TestInterface::PStructString> sp = 
        std::dynamic_pointer_cast<TestInterface::PStructString>(a3);
    
    ASSERT_TRUE(sp != nullptr);
    EXPECT_EQ(sp->getName(), "123");

}

/**
 * @test
 *  - Set and get a struct-type attribute through a polymorphic structure
 *  - verify that the received data matches the transmitted data
 */
TEST_F(AFPolymorph, SetAndGetAttributeStruct) {

    CommonAPI::CallStatus callStatus;
    TestInterface::BStruct b;
    b.setA1(17);
    b.setA2(19);
    b.setS1("ABQZ");
    
    
    auto a1 = std::make_shared<TestInterface::PStructStruct>(b);
    std::shared_ptr<TestInterface::PStructBase> a2 = std::make_shared<TestInterface::PStructBase>();
    std::shared_ptr<TestInterface::PStructBase> a3 = std::make_shared<TestInterface::PStructBase>();  
    
    a2 = static_cast<std::shared_ptr<TestInterface::PStructBase>>(a1);
    
    testProxy_->getA1Attribute().setValue(a2, callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    testProxy_->getA1Attribute().getValue(callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    
    std::shared_ptr<TestInterface::PStructStruct> sp = 
        std::dynamic_pointer_cast<TestInterface::PStructStruct>(a3);
    
    ASSERT_TRUE(sp != nullptr);
    EXPECT_EQ(sp->getS().getA1(), 17);
    EXPECT_EQ(sp->getS().getA2(), 19);
    EXPECT_EQ(sp->getS().getS1(), "ABQZ");
}

/**
 * @test
 *  - Call a method whose input and output parameters are polymorphic structures
 *  - verify that the received data matches the transmitted data
 */
TEST_F(AFPolymorph, MethodCall) {

    CommonAPI::CallStatus callStatus;

    auto a1 = std::make_shared<TestInterface::PStructMyTypedef>(-5);
    std::shared_ptr<TestInterface::PStructBase> a2 = std::make_shared<TestInterface::PStructBase>();
    std::shared_ptr<TestInterface::PStructBase> a3 = std::make_shared<TestInterface::PStructBase>();  
    
    a2 = static_cast<std::shared_ptr<TestInterface::PStructBase>>(a1);
    
    testProxy_->testMethod(a2, callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    std::shared_ptr<TestInterface::PStructMyTypedef> sp = 
        std::dynamic_pointer_cast<TestInterface::PStructMyTypedef>(a3);
    
    ASSERT_TRUE(sp != nullptr);
    EXPECT_EQ((int)sp->getId(), -5);

}

/**
 * @test
 *  - Call a method with a special value that tells the stub to send a broadcast signal
 *  - verify that the received data matches the transmitted data
 */
TEST_F(AFPolymorph, Broadcast) {

    CommonAPI::CallStatus callStatus;
    int result;
    // subscribe to broadcast
    testProxy_->getBTestEvent().subscribe([&](
        const std::shared_ptr<TestInterface::PStructBase> &y
    ) {
        std::shared_ptr<TestInterface::PStructMyTypedef> sp = 
            std::dynamic_pointer_cast<TestInterface::PStructMyTypedef>(y);
    
        ASSERT_TRUE(sp != nullptr);
        result = (int)sp->getId();
    });

    // send '1' through method - this tells the stub to send the broadcast
    auto a1 = std::make_shared<TestInterface::PStructMyTypedef>(1);
    std::shared_ptr<TestInterface::PStructBase> a2 = std::make_shared<TestInterface::PStructBase>();
    std::shared_ptr<TestInterface::PStructBase> a3 = std::make_shared<TestInterface::PStructBase>();  
    
    a2 = static_cast<std::shared_ptr<TestInterface::PStructBase>>(a1);
    
    testProxy_->testMethod(a2, callStatus, a3);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    std::shared_ptr<TestInterface::PStructMyTypedef> sp = 
        std::dynamic_pointer_cast<TestInterface::PStructMyTypedef>(a3);
    
    ASSERT_TRUE(sp != nullptr);
    EXPECT_EQ((int)sp->getId(), 1);
    
    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);

}
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
