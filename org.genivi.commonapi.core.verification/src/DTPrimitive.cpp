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
#include <atomic>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"

#include "v1/commonapi/datatypes/primitive/TestInterfaceProxy.hpp"
#include "stub/DTPrimitiveStub.h"

const std::string domain = "local";
const std::string testAddress = "commonapi.datatypes.primitive.TestInterface";
const std::string connectionIdService = "service-sample";
const std::string connectionIdClient = "client-sample";

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

class DTPrimitive: public ::testing::Test {

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStub_ = std::make_shared<v1_0::commonapi::datatypes::primitive::DTPrimitiveStub>();
        serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, connectionIdService);
        ASSERT_TRUE(serviceRegistered_);

        testProxy_ = runtime_->buildProxy<v1_0::commonapi::datatypes::primitive::TestInterfaceProxy>(domain, testAddress, connectionIdClient);
        int i = 0;
        while(!testProxy_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        ASSERT_TRUE(runtime_->unregisterService(domain, v1_0::commonapi::datatypes::primitive::DTPrimitiveStub::StubInterface::getInterface(), testAddress));

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 100 ) {
            std::this_thread::sleep_for(std::chrono::microseconds(tasync));
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    std::atomic<bool> received_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<v1_0::commonapi::datatypes::primitive::TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<v1_0::commonapi::datatypes::primitive::DTPrimitiveStub> testStub_;
};

/**
* @test Test function call with primitive types
*   - Primitive types are:  uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, bool, float, double, std::string, ByteBuffer
*   - Function call of a function that has for each primitive type one argument (test values) and one return value
*   - The stub copies the test values to the return values
*   - On client side the test values are compared with the return values
*/
TEST_F(DTPrimitive, SendAndReceive) {

    CommonAPI::CallStatus callStatus;

    uint8_t uint8TestValue = +101;
    int8_t int8TestValue = -101;
    uint16_t uint16TestValue = +40004;
    int16_t int16TestValue = -5005;
    uint32_t uint32TestValue = +1000000001;
    int32_t int32TestValue = -20002;
    uint64_t uint64TestValue = +4000000000000000004;
    int64_t int64TestValue = -5000000005;
    bool booleanTestValue = true;
    float floatTestValue = 1.01f;
    double doubleTestValue = 12345.12345;
    std::string stringTestValue = "∃y ∀x ¬(x ≺ y)";
    CommonAPI::ByteBuffer byteBufferTestValue(5, 0);

    uint8_t uint8ResultValue = 0;
    int8_t int8ResultValue = 0;
    uint16_t uint16ResultValue = 0;
    int16_t int16ResultValue = 0;
    uint32_t uint32ResultValue = 0;
    int32_t int32ResultValue = 0;
    uint64_t uint64ResultValue = 0;
    int64_t int64ResultValue = 0;
    bool booleanResultValue = false;
    float floatResultValue = 0.0;
    double doubleResultValue = 0;
    std::string stringResultValue = "";
    CommonAPI::ByteBuffer byteBufferResultValue;

    testProxy_->fTest(
            uint8TestValue,
            int8TestValue,
            uint16TestValue,
            int16TestValue,
            uint32TestValue,
            int32TestValue,
            uint64TestValue,
            int64TestValue,
            booleanTestValue,
            floatTestValue,
            doubleTestValue,
            stringTestValue,
            byteBufferTestValue,
            callStatus,
            uint8ResultValue,
            int8ResultValue,
            uint16ResultValue,
            int16ResultValue,
            uint32ResultValue,
            int32ResultValue,
            uint64ResultValue,
            int64ResultValue,
            booleanResultValue,
            floatResultValue,
            doubleResultValue,
            stringResultValue,
            byteBufferResultValue
    );

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    EXPECT_EQ(uint8TestValue, uint8ResultValue);
    EXPECT_EQ(int8TestValue, int8ResultValue);
    EXPECT_EQ(uint16TestValue, uint16ResultValue);
    EXPECT_EQ(int16TestValue, int16ResultValue);
    EXPECT_EQ(uint32TestValue, uint32ResultValue);
    EXPECT_EQ(int32TestValue, int32ResultValue);
    EXPECT_EQ(uint64TestValue, uint64ResultValue);
    EXPECT_EQ(int64TestValue, int64ResultValue);
    EXPECT_EQ(booleanTestValue, booleanResultValue);
    EXPECT_EQ(floatTestValue, floatResultValue);
    EXPECT_EQ(doubleTestValue, doubleResultValue);
    EXPECT_EQ(stringTestValue, stringResultValue);
    EXPECT_EQ(byteBufferTestValue, byteBufferResultValue);
}

/**
* @test Test attribute functions with primitive types
*   - Call set function of attributes with primitive types
*   - Call get function and check if the return value is the same
*/
TEST_F(DTPrimitive, AttributeSet) {

    CommonAPI::CallStatus callStatus;

    uint8_t uint8TestValue = +101;
    int8_t int8TestValue = -101;
    uint16_t uint16TestValue = +40004;
    int16_t int16TestValue = -5005;
    uint32_t uint32TestValue = +1000000001;
    int32_t int32TestValue = -20002;
    uint64_t uint64TestValue = +4000000000000000004;
    int64_t int64TestValue = -5000000005;
    bool booleanTestValue = true;
    float floatTestValue = 1.01f;
    double doubleTestValue = 12345.12345;
    std::string stringTestValue = "∃y ∀x ¬(x ≺ y)";
    CommonAPI::ByteBuffer byteBufferTestValue(5, 0);

    uint8_t uint8ResultValue = 0;
    int8_t int8ResultValue = 0;
    uint16_t uint16ResultValue = 0;
    int16_t int16ResultValue = 0;
    uint32_t uint32ResultValue = 0;
    int32_t int32ResultValue = 0;
    uint64_t uint64ResultValue = 0;
    int64_t int64ResultValue = 0;
    bool booleanResultValue = false;
    float floatResultValue = 0.0;
    double doubleResultValue = 0;
    std::string stringResultValue = "";
    CommonAPI::ByteBuffer byteBufferResultValue;

    testProxy_->getAUint8Attribute().setValue(uint8TestValue, callStatus, uint8ResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(uint8TestValue, uint8ResultValue);

    testProxy_->getAInt8Attribute().setValue(int8TestValue, callStatus, int8ResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(int8TestValue, int8ResultValue);

    testProxy_->getAUint16Attribute().setValue(uint16TestValue, callStatus, uint16ResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(uint16TestValue, uint16ResultValue);

    testProxy_->getAInt16Attribute().setValue(int16TestValue, callStatus, int16ResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(int16TestValue, int16ResultValue);

    testProxy_->getAUint32Attribute().setValue(uint32TestValue, callStatus, uint32ResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(uint32TestValue, uint32ResultValue);

    testProxy_->getAInt32Attribute().setValue(int32TestValue, callStatus, int32ResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(int32TestValue, int32ResultValue);

    testProxy_->getAUint64Attribute().setValue(uint64TestValue, callStatus, uint64ResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(uint64TestValue, uint64ResultValue);

    testProxy_->getAInt64Attribute().setValue(int64TestValue, callStatus, int64ResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(int64TestValue, int64ResultValue);

    testProxy_->getABooleanAttribute().setValue(booleanTestValue, callStatus, booleanResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(booleanTestValue, booleanResultValue);

    testProxy_->getAFloatAttribute().setValue(floatTestValue, callStatus, floatResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(floatTestValue, floatResultValue);

    testProxy_->getADoubleAttribute().setValue(doubleTestValue, callStatus, doubleResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(doubleTestValue, doubleResultValue);

    testProxy_->getAStringAttribute().setValue(stringTestValue, callStatus, stringResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(stringTestValue, stringResultValue);

    testProxy_->getAByteBufferAttribute().setValue(byteBufferTestValue, callStatus, byteBufferResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(byteBufferTestValue, byteBufferResultValue);
}

/**
* @test Test broadcast with primitive types
*   - Subscribe to broadcast which contains primitive types
*   - Call function to cause the stub to fire broadcast event with the same content
*   - Check if the values in the callback function are as expected
*/
TEST_F(DTPrimitive, BroadcastReceive) {

    CommonAPI::CallStatus callStatus;

    uint8_t uint8TestValue = +101;
    int8_t int8TestValue = -101;
    uint16_t uint16TestValue = +40004;
    int16_t int16TestValue = -5005;
    uint32_t uint32TestValue = +1000000001;
    int32_t int32TestValue = -20002;
    uint64_t uint64TestValue = +4000000000000000004;
    int64_t int64TestValue = -5000000005;
    bool booleanTestValue = true;
    float floatTestValue = 1.01f;
    double doubleTestValue = 12345.12345;
    const std::string stringTestValue = "∃y ∀x ¬(x ≺ y)";
    CommonAPI::ByteBuffer byteBufferTestValue(5, 0);

    uint8_t uint8ResultValue = 0;
    int8_t int8ResultValue = 0;
    uint16_t uint16ResultValue = 0;
    int16_t int16ResultValue = 0;
    uint32_t uint32ResultValue = 0;
    int32_t int32ResultValue = 0;
    uint64_t uint64ResultValue = 0;
    int64_t int64ResultValue = 0;
    bool booleanResultValue = false;
    float floatResultValue = 0.0;
    double doubleResultValue = 0;
    std::string stringResultValue = "";
    CommonAPI::ByteBuffer byteBufferResultValue;

    received_ = false;
    testProxy_->getBTestEvent().subscribe([&](
            const uint8_t& uint8ResultValue,
            const int8_t& int8ResultValue,
            const uint16_t& uint16ResultValue,
            const int16_t& int16ResultValue,
            const uint32_t& uint32ResultValue,
            const int32_t& int32ResultValue,
            const uint64_t& uint64ResultValue,
            const int64_t& int64ResultValue,
            const bool& booleanResultValue,
            const float& floatResultValue,
            const double& doubleResultValue,
            const std::string& stringResultValue,
            const CommonAPI::ByteBuffer byteBufferResultValue
            ) {
        EXPECT_EQ(uint8TestValue, uint8ResultValue);
        EXPECT_EQ(int8TestValue, int8ResultValue);
        EXPECT_EQ(uint16TestValue, uint16ResultValue);
        EXPECT_EQ(int16TestValue, int16ResultValue);
        EXPECT_EQ(uint32TestValue, uint32ResultValue);
        EXPECT_EQ(int32TestValue, int32ResultValue);
        EXPECT_EQ(uint64TestValue, uint64ResultValue);
        EXPECT_EQ(int64TestValue, int64ResultValue);
        EXPECT_EQ(booleanTestValue, booleanResultValue);
        EXPECT_EQ(floatTestValue, floatResultValue);
        EXPECT_EQ(doubleTestValue, doubleResultValue);
        EXPECT_EQ(stringTestValue, stringResultValue);
        EXPECT_EQ(stringTestValue, stringResultValue);
        EXPECT_EQ(byteBufferTestValue, byteBufferResultValue);
        received_ = true;
    });

    testProxy_->fTest(
            uint8TestValue,
            int8TestValue,
            uint16TestValue,
            int16TestValue,
            uint32TestValue,
            int32TestValue,
            uint64TestValue,
            int64TestValue,
            booleanTestValue,
            floatTestValue,
            doubleTestValue,
            stringTestValue,
            byteBufferTestValue,
            callStatus,
            uint8ResultValue,
            int8ResultValue,
            uint16ResultValue,
            int16ResultValue,
            uint32ResultValue,
            int32ResultValue,
            uint64ResultValue,
            int64ResultValue,
            booleanResultValue,
            floatResultValue,
            doubleResultValue,
            stringResultValue,
            byteBufferResultValue
    );

    for (int i = 0; i < 100; ++i) {
        if (received_) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    ASSERT_TRUE(received_);
}

/**
* @test Test broadcast with empty broadcast
*   - Subscribe to broadcast which does not contain any datatypes
*   - Call function twice to cause the stub to fire a broadcast event
*   - Check if the callback function was called twice
*/
TEST_F(DTPrimitive, EmptyBroadcastReceive) {

    CommonAPI::CallStatus callStatus;
    std::atomic<std::int32_t> callbackCalled(0);
    int numberFunctionCalls = 2;

    received_ = false;
    testProxy_->getBTestEmptyEvent().subscribe([&]() {
        received_ = true;
        callbackCalled++;
    });

    for (int var = 0; var < numberFunctionCalls; ++var) {
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
        testProxy_->fTestEmptyBroadcast(callStatus);
    }

    for (int i = 0; i < 100; ++i) {
        if (received_ && numberFunctionCalls == callbackCalled) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    ASSERT_TRUE(received_);
    ASSERT_EQ(numberFunctionCalls, callbackCalled);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
