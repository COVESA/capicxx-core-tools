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

#include "v1/commonapi/datatypes/advanced/TestInterfaceProxy.hpp"
#include "stub/DTAdvancedStub.h"

const std::string domain = "local";
const std::string testAddress = "commonapi.datatypes.advanced.TestInterface";
const std::string connectionIdService = "service-sample";
const std::string connectionIdClient = "client-sample";

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class DTAdvanced: public ::testing::Test {

public:
    void recvEnumValue(const CommonAPI::CallStatus& callStatus,
             ::v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration responseValue) {
        (void)responseValue;
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    }
    void recvInvalidEnumValue(const CommonAPI::CallStatus& callStatus,
             ::v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration responseValue) {
        (void)responseValue;
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::INVALID_VALUE);
    }


protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);
        std::mutex availabilityMutex;
        std::unique_lock<std::mutex> lock(availabilityMutex);
        std::condition_variable cv;
        bool proxyAvailable = false;

        std::thread t1([this, &proxyAvailable, &cv, &availabilityMutex]() {
            std::lock_guard<std::mutex> lock(availabilityMutex);
            testProxy_ = runtime_->buildProxy<v1_0::commonapi::datatypes::advanced::TestInterfaceProxy>(domain, testAddress, connectionIdClient);
            testProxy_->isAvailableBlocking();
            ASSERT_TRUE((bool)testProxy_);
            proxyAvailable = true;
            cv.notify_one();
        });

        testStub_ = std::make_shared<v1_0::commonapi::datatypes::advanced::DTAdvancedStub>();
        serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, connectionIdService);
        ASSERT_TRUE(serviceRegistered_);

        while(!proxyAvailable) {
            cv.wait(lock);
        }
        t1.join();
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        ASSERT_TRUE(runtime_->unregisterService(domain, v1_0::commonapi::datatypes::advanced::DTAdvancedStub::StubInterface::getInterface(), testAddress));
        
        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 10 ) {
            usleep(100000);
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    bool received_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<v1_0::commonapi::datatypes::advanced::TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<v1_0::commonapi::datatypes::advanced::DTAdvancedStub> testStub_;
};

/*
* @test Test function call with advanced types
*   - Advanced types are: arrays, enumerations, structs, unions, maps, typedefs
*   - Function call of a function that has for each advanced type one argument (test values) and one return value
*   - The stub copies the test values to the return values
*   - On client side the test values are compared with the return values
*/
TEST_F(DTAdvanced, SendAndReceive) {

    CommonAPI::CallStatus callStatus;

    v1_0::commonapi::datatypes::advanced::TestInterface::tArray arrayTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tStruct structTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tUnion unionTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tMap mapTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef typedefTestValue;

    arrayTestValue.push_back("Test1");
    arrayTestValue.push_back("Test2");
    arrayTestValue.push_back("Test3");

    enumerationTestValue = v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::VALUE2;

    structTestValue.setBooleanMember(true);
    structTestValue.setUint8Member(42);
    structTestValue.setStringMember("Hello World");

    uint8_t u = 53;
    unionTestValue = u;

    mapTestValue[1] = "Hello";
    mapTestValue[2] = "World";

    typedefTestValue = 64;

    std::vector<v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration> enumArrayIn;
    enumArrayIn.push_back(enumerationTestValue);

    v1_0::commonapi::datatypes::advanced::TestInterface::tArray arrayResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tStruct structResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tUnion unionResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tMap mapResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef typedefResultValue;
    std::vector<v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration> enumArrayOut;

    testProxy_->fTest(
            arrayTestValue,
            enumerationTestValue,
            structTestValue,
            unionTestValue,
            mapTestValue,
            typedefTestValue,
            enumArrayIn,
            callStatus,
            arrayResultValue,
            enumerationResultValue,
            structResultValue,
            unionResultValue,
            mapResultValue,
            typedefResultValue,
            enumArrayOut
    );

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(arrayTestValue, arrayResultValue);
    EXPECT_EQ(enumerationTestValue, enumerationResultValue);
    EXPECT_EQ(structTestValue, structResultValue);
    EXPECT_EQ(unionTestValue, unionResultValue);
    EXPECT_EQ(mapTestValue, mapResultValue);
    EXPECT_EQ(typedefTestValue, typedefResultValue);
    EXPECT_EQ(enumArrayIn, enumArrayOut);
}

/*
* @test Test function call with an invalid type
* Try to pass an invalid value. Check that it failed.
*/
TEST_F(DTAdvanced, SendAndReceiveInvalid) {

    CommonAPI::CallStatus callStatus;

    v1_0::commonapi::datatypes::advanced::TestInterface::tArray arrayTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tStruct structTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tUnion unionTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tMap mapTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef typedefTestValue;

    arrayTestValue.push_back("Test1");
    arrayTestValue.push_back("Test2");
    arrayTestValue.push_back("Test3");

    // put a deliberately invalid value to the enum data value
    enumerationTestValue = static_cast<const v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::Literal>(1234);

    structTestValue.setBooleanMember(true);
    structTestValue.setUint8Member(42);
    structTestValue.setStringMember("Hello World");

    uint8_t u = 53;
    unionTestValue = u;

    mapTestValue[1] = "Hello";
    mapTestValue[2] = "World";

    typedefTestValue = 64;

    std::vector<v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration> enumArrayIn;
    enumArrayIn.push_back(v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::VALUE2);

    v1_0::commonapi::datatypes::advanced::TestInterface::tArray arrayResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tStruct structResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tUnion unionResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tMap mapResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef typedefResultValue;
    std::vector<v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration> enumArrayOut;

    testProxy_->fTest(
            arrayTestValue,
            enumerationTestValue,
            structTestValue,
            unionTestValue,
            mapTestValue,
            typedefTestValue,
            enumArrayIn,
            callStatus,
            arrayResultValue,
            enumerationResultValue,
            structResultValue,
            unionResultValue,
            mapResultValue,
            typedefResultValue,
            enumArrayOut
    );

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::INVALID_VALUE);
}


/**
* @test Test attribute functions with invalid values
*   - Call set function of attributes with invalid types
*   - Check that the attribute's value has not changed
*/
TEST_F(DTAdvanced, AttributeSetInvalid) {

    CommonAPI::CallStatus callStatus;

    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationTestValue;

    // put a deliberately invalid value to the enum data value
    enumerationTestValue = static_cast<const v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::Literal>(1234);

    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationResultValue;

    testProxy_->getAEnumerationAttribute().setValue(enumerationTestValue, callStatus, enumerationResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::INVALID_VALUE);
}

/**
* @test Test attribute asynchronous functions with invalid values
*   - Call set asynch function of attributes with invalid types
*   - Callback should be called with error status
*   - Check that attribute value has not changed
*/
TEST_F(DTAdvanced, AttributeSetAsyncInvalid) {

    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationTestValue;

    std::function<void (const CommonAPI::CallStatus&,
        v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration)> myCallback =
            std::bind(&DTAdvanced::recvInvalidEnumValue, this, std::placeholders::_1, std::placeholders::_2);

    // put a deliberately invalid value to the enum data value
    enumerationTestValue = static_cast<const v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::Literal>(1234);

    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationResultValue;

    testProxy_->getAEnumerationAttribute().setValueAsync(enumerationTestValue, myCallback);
    usleep(100000);
}


/**
* @test Test attribute functions with advanced types
*   - Call set function of attributes with advanced types
*   - Call get function and check if the return value is the same
*/
TEST_F(DTAdvanced, AttributeSet) {

    CommonAPI::CallStatus callStatus;

    v1_0::commonapi::datatypes::advanced::TestInterface::tArray arrayTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tStruct structTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tUnion unionTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tMap mapTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef typedefTestValue;

    arrayTestValue.push_back("Test1");
    arrayTestValue.push_back("Test2");
    arrayTestValue.push_back("Test3");

    enumerationTestValue = v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::VALUE2;

    structTestValue.setBooleanMember(true);
    structTestValue.setUint8Member(42);
    structTestValue.setStringMember("Hello World");

    uint8_t u = 53;
    unionTestValue = u;

    mapTestValue[1] = "Hello";
    mapTestValue[2] = "World";

    typedefTestValue = 64;

    v1_0::commonapi::datatypes::advanced::TestInterface::tArray arrayResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tStruct structResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tUnion unionResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tMap mapResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef typedefResultValue;

    testProxy_->getAArrayAttribute().setValue(arrayTestValue, callStatus, arrayResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(arrayTestValue, arrayResultValue);

    // check initial value of enumeration attribute
    enumerationTestValue = v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::VALUE1; // this is the expected default value
    EXPECT_EQ(enumerationTestValue, enumerationResultValue); // the uninitialized enumerationResultValue should have the default value
    enumerationResultValue = v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::VALUE2; // set to some other value
    testProxy_->getAEnumerationAttribute().getValue(callStatus, enumerationResultValue); // get value of attribute
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(enumerationTestValue, enumerationResultValue); // attribute value should default to the initial default value

    testProxy_->getAEnumerationAttribute().setValue(enumerationTestValue, callStatus, enumerationResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(enumerationTestValue, enumerationResultValue);

    testProxy_->getAStructAttribute().setValue(structTestValue, callStatus, structResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(structTestValue, structResultValue);

    testProxy_->getAUnionAttribute().setValue(unionTestValue, callStatus, unionResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(unionTestValue, unionResultValue);

    testProxy_->getAMapAttribute().setValue(mapTestValue, callStatus, mapResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(mapTestValue, mapResultValue);

    testProxy_->getATypedefAttribute().setValue(typedefTestValue, callStatus, typedefResultValue);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(typedefTestValue, typedefResultValue);
}

/**
* @test Test broadcast with advanced types
*   - Subscribe to broadcast which contains advanced types
*   - Call function to cause the stub to fire broadcast event with the same content
*   - Check if the values in the callback function are as expected
*/
TEST_F(DTAdvanced, BroadcastReceive) {

    CommonAPI::CallStatus callStatus;

    v1_0::commonapi::datatypes::advanced::TestInterface::tArray arrayTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tStruct structTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tUnion unionTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tMap mapTestValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef typedefTestValue;

    arrayTestValue.push_back("Test1");
    arrayTestValue.push_back("Test2");
    arrayTestValue.push_back("Test3");

    enumerationTestValue = v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration::VALUE2;

    structTestValue.setBooleanMember(true);
    structTestValue.setUint8Member(42);
    structTestValue.setStringMember("Hello World");

    uint8_t u = 53;
    unionTestValue = u;

    mapTestValue[1] = "Hello";
    mapTestValue[2] = "World";

    typedefTestValue = 64;

    std::vector<v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration> enumArrayIn;
    enumArrayIn.push_back(enumerationTestValue);

    v1_0::commonapi::datatypes::advanced::TestInterface::tArray arrayResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration enumerationResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tStruct structResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tUnion unionResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tMap mapResultValue;
    v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef typedefResultValue;
    std::vector<v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration> enumArrayOut;

    received_ = false;
    testProxy_->getBTestEvent().subscribe([&](
            const v1_0::commonapi::datatypes::advanced::TestInterface::tArray& arrayResultValue,
            const v1_0::commonapi::datatypes::advanced::TestInterface::tEnumeration& enumerationResultValue,
            const v1_0::commonapi::datatypes::advanced::TestInterface::tStruct& structResultValue,
            const v1_0::commonapi::datatypes::advanced::TestInterface::tUnion& unionResultValue,
            const v1_0::commonapi::datatypes::advanced::TestInterface::tMap& mapResultValue,
            const v1_0::commonapi::datatypes::advanced::TestInterface::tTypedef& typedefResultValue
            ) {
        received_ = true;
        EXPECT_EQ(arrayTestValue, arrayResultValue);
        EXPECT_EQ(enumerationTestValue, enumerationResultValue);
        EXPECT_EQ(structTestValue, structResultValue);
        EXPECT_EQ(unionTestValue, unionResultValue);
        EXPECT_EQ(mapTestValue, mapResultValue);
        EXPECT_EQ(typedefTestValue, typedefResultValue);
    });

    testProxy_->fTest(
            arrayTestValue,
            enumerationTestValue,
            structTestValue,
            unionTestValue,
            mapTestValue,
            typedefTestValue,
            enumArrayIn,
            callStatus,
            arrayResultValue,
            enumerationResultValue,
            structResultValue,
            unionResultValue,
            mapResultValue,
            typedefResultValue,
            enumArrayOut
    );

    usleep(100000);
    ASSERT_TRUE(received_);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
