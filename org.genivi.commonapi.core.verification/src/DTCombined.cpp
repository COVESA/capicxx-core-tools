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

#include "v1/commonapi/datatypes/combined/TestInterfaceProxy.hpp"
#include "stub/DTCombinedStub.h"

const std::string domain = "local";
const std::string testAddress = "commonapi.datatypes.combined.TestInterface";
const std::string connectionIdService = "service-sample";
const std::string connectionIdClient = "client-sample";

using namespace v1_0::commonapi::datatypes::combined;

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {

    }

    virtual void TearDown() {
    }
};

class DTCombined: public ::testing::Test {

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
            testProxy_ = runtime_->buildProxy<v1_0::commonapi::datatypes::combined::TestInterfaceProxy>(domain, testAddress, connectionIdClient);
            testProxy_->isAvailableBlocking();
            ASSERT_TRUE((bool)testProxy_);
            proxyAvailable = true;
            cv.notify_one();
        });

        testStub_ = std::make_shared<v1_0::commonapi::datatypes::combined::DTCombinedStub>();
        serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, connectionIdService);
        ASSERT_TRUE(serviceRegistered_);

        while(!proxyAvailable) {
            cv.wait(lock);
        }
        t1.join();
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        ASSERT_TRUE(runtime_->unregisterService(domain, v1_0::commonapi::datatypes::combined::TestInterfaceStub::StubInterface::getInterface(), testAddress));

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 10 ) {
            usleep(100000);
            counter++;
        }

        ASSERT_FALSE(testProxy_->isAvailable());
    }

    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<v1_0::commonapi::datatypes::combined::TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<v1_0::commonapi::datatypes::combined::DTCombinedStub> testStub_;
};

/**
* @test Test function call with combined type
*   - The combined type is one structure with combinations of advanced and primitive types
*   - Function call of a function that has for each advanced type one argument (test values) and one return value
*   - The stub copies the test values to the return values
*   - On client side the test values are compared with the return values
*/
TEST_F(DTCombined, SendAndReceive) {

    CommonAPI::CallStatus callStatus;

    // TestValue is TV; start with Level0
    TestInterface::tEnum enumTV0 = TestInterface::tEnum::VALUE1;
    TestInterface::tEnum enumTV1 = TestInterface::tEnum::VALUE2;
    TestInterface::tArray arrayTV0 = {"Test1", "Test2", "Test3"};
    TestInterface::tArray arrayTV1 = {"Test4", "Test5", "Test6"};
    TestInterface::tArray arrayTV2 = {"Test7", "Test8", "Test9"};
    TestInterface::tStruct structTV0 = {true, 42, "Hello World", TestInterface::tEnum::VALUE1};
    TestInterface::tStruct structTV1 = {false, 53, "ABC", TestInterface::tEnum::VALUE2};
    TestInterface::tUnion unionTV0 = true;
    TestInterface::tUnion unionTV1 = (uint8_t)42;
    TestInterface::tUnion unionTV2 = std::string("Hello World");
    TestInterface::tUnion unionTV3 = static_cast<TestInterface::tEnum>(TestInterface::tEnum::VALUE1);
    TestInterface::tMap mapTV0 = {{1, "Hello"}, {2, "World"}};
    TestInterface::tMap mapTV1 = {{123, "ABC"}, {456, "DEF"}};

    // Level1
    TestInterface::tArrayEnum arrayEnumTV0 = {enumTV0, enumTV1, enumTV0};
    TestInterface::tArrayEnum arrayEnumTV1 = {enumTV1, enumTV0, enumTV1};
    TestInterface::tArrayEnum arrayEnumTV2 = {enumTV0, enumTV0, enumTV0};
    TestInterface::tArrayArray arrayArrayTV0 = {arrayTV0, arrayTV1, arrayTV2};
    TestInterface::tArrayArray arrayArrayTV1 = {arrayTV1, arrayTV2};
    TestInterface::tArrayStruct arrayStructTV0 = {structTV0, structTV0};
    TestInterface::tArrayStruct arrayStructTV1 = {structTV1, structTV1};
    TestInterface::tArrayStruct arrayStructTV2 = {structTV0, structTV1};
    TestInterface::tArrayStruct arrayStructTV3 = {structTV1, structTV0};
    TestInterface::tArrayUnion arrayUnionTV0 = {unionTV0, unionTV1, unionTV2, unionTV3};
    TestInterface::tArrayMap arrayMapTV0 = {mapTV0, mapTV1};
    TestInterface::tArrayMap arrayMapTV1 = {mapTV1, mapTV0};

    TestInterface::tStructL1 structL1TV0 = {enumTV0, arrayTV0, structTV0, unionTV0, mapTV0};
    TestInterface::tStructL1 structL1TV1 = {enumTV1, arrayTV1, structTV1, unionTV1, mapTV1};

    TestInterface::tUnionL1 unionL1TV0 = enumTV0;
    TestInterface::tUnionL1 unionL1TV1 = arrayTV0;
    TestInterface::tUnionL1 unionL1TV2 = structTV0;
    TestInterface::tUnionL1 unionL1TV3 = unionTV0;
    TestInterface::tUnionL1 unionL1TV4 = mapTV0;

    TestInterface::tMapEnum mapEnumTV0 = {{1, TestInterface::tEnum::VALUE1}, {2, TestInterface::tEnum::VALUE2}};
    TestInterface::tMapEnum mapEnumTV1 = {{123, TestInterface::tEnum::VALUE2}, {456, TestInterface::tEnum::VALUE1}};
    TestInterface::tMapArray mapArrayTV0 = {{123456789, arrayTV0}, {987654321, arrayTV1}};
    TestInterface::tMapStruct mapStructTV0 = {{"Hello", structTV0}};
    TestInterface::tMapStruct mapStructTV1 = {{"Hello", structTV0}, {"World", structTV1}};
    TestInterface::tMapStruct mapStructTV2 = {{"Hello", structTV0}, {"World", structTV1}, {"JG", structTV0}};
    TestInterface::tMapUnion mapUnionTV0 = {{1.23456789f, unionTV2}, {9.87654321f, unionTV3}};
    TestInterface::tMapUnion mapUnionTV1 = {{3.123f, unionTV0}, {0.111111f, unionTV1}};
    TestInterface::tMapMap mapMapTV0 = {{-1, mapTV0}, {-2, mapTV1}};


    // Level2
    TestInterface::tArrayArrayEnum arrayArrayEnumTV = {arrayEnumTV0, arrayEnumTV1, arrayEnumTV2};
    TestInterface::tArrayArrayArray arrayArrayArrayTV = {arrayArrayTV0, arrayArrayTV1};
    TestInterface::tArrayArrayStruct arrayArrayStructTV = {arrayStructTV0, arrayStructTV1, arrayStructTV2, arrayStructTV3};
    TestInterface::tArrayArrayUnion arrayArrayUnionTV = {arrayUnionTV0};
    TestInterface::tArrayArrayMap arrayArrayMapTV = {arrayMapTV0, arrayMapTV1};
    TestInterface::tArrayStructL1 arrayStructL1TV = {structL1TV0, structL1TV1};
    TestInterface::tArrayUnionL1 arrayUnionL1TV = {unionL1TV0, unionL1TV1, unionL1TV2, unionL1TV3, unionL1TV4};
    TestInterface::tArrayMapEnum arrayMapEnumTV = {mapEnumTV0, mapEnumTV1};
    TestInterface::tArrayMapArray arrayMapArrayTV = {mapArrayTV0};
    TestInterface::tArrayMapStruct arrayMapStructTV = {mapStructTV0, mapStructTV1, mapStructTV2};
    TestInterface::tArrayMapUnion arrayMapUnionTV = {mapUnionTV0, mapUnionTV1};
    TestInterface::tArrayMapMap arrayMapMapTV = {mapMapTV0};

    TestInterface::tStructL2 structL2TV = {arrayEnumTV0, arrayArrayTV0, arrayStructTV0, arrayUnionTV0, arrayMapTV0,
            structL1TV0, unionL1TV0, mapEnumTV0, mapArrayTV0, mapStructTV0, mapUnionTV0, mapMapTV0};

    //TestInterface::tUnionL2 unionL2TV = arrayEnumTV0;

    TestInterface::tMapArrayEnum mapArrayEnumTV = {{"A", arrayEnumTV0}, {"B", arrayEnumTV1}, {"C", arrayEnumTV1}};
    TestInterface::tMapArrayArray mapArrayArrayTV = {{"ABCDEFGHIJK", arrayArrayTV0}, {"LMNOPQRST", arrayArrayTV1}};
    TestInterface::tMapArrayStruct mapArrayStructTV = {{"Z", arrayStructTV3}, {"Y", arrayStructTV2}, {"X", arrayStructTV1}, {"W", arrayStructTV0}};
    TestInterface::tMapArrayUnion mapArrayUnionTV = {{"JG", arrayUnionTV0}};
    TestInterface::tMapArrayMap mapArrayMapTV = {{"0x1", arrayMapTV0}, {"0x2", arrayMapTV0}};
    TestInterface::tMapStructL1 mapStructL1TV = {{"Hello", structL1TV0}, {"World", structL1TV1}};
    TestInterface::tMapUnionL1 mapUnionL1TV = {{"A", unionL1TV0}, {"B", unionL1TV1}, {"C", unionL1TV2}, {"D", unionL1TV3}, {"E", unionL1TV4}};
    TestInterface::tMapMapEnum mapMapEnumTV = {{"Hello", mapEnumTV0}, {"World", mapEnumTV1}};
    TestInterface::tMapMapArray mapMapArrayTV = {{"JG", mapArrayTV0}};
    TestInterface::tMapMapStruct mapMapStructTV = {{"AA", mapStructTV0}, {"BB", mapStructTV1}, {"CC", mapStructTV2}};
    TestInterface::tMapMapUnion mapMapUnionTV = {{"Hello", mapUnionTV0}, {"World", mapUnionTV1}};
    TestInterface::tMapMapMap mapMapMapTV = {{"ABCDEFGHI", mapMapTV0}};

    TestInterface::tStructL3 tStructL3TV = {
        arrayArrayEnumTV,
        arrayArrayArrayTV,
        arrayArrayStructTV,
        arrayArrayUnionTV,
        arrayArrayMapTV,
        arrayStructL1TV,
        //arrayUnionL1TV,
        //arrayMapEnumTV,
        //arrayMapArrayTV,
        //arrayMapStructTV,
        //arrayMapUnionTV,
        //arrayMapMapTV,
        structL2TV,
        //tUnionL2TV,
        mapArrayEnumTV,
        mapArrayArrayTV,
        mapArrayStructTV,
        mapArrayUnionTV,
        mapArrayMapTV,
        //mapStructL1TV,
        mapUnionL1TV,
        mapMapEnumTV,
        mapMapArrayTV,
        mapMapStructTV,
        mapMapUnionTV,
        mapMapMapTV
    };

    TestInterface::tStructL3 tStructL3RV;

    testProxy_->fTest(tStructL3TV, callStatus, tStructL3RV);

    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(tStructL3TV, tStructL3RV);
}

/**
* @test Test that combined types are properly initialized
*/
TEST_F(DTCombined, CheckInitialValue) {

    TestInterface::tEnum e1;
    // check contents of e1
    EXPECT_EQ(e1, TestInterface::tEnum::VALUE1);

    TestInterface::tArray a1;
    // check that a1 is empty (no values)
    EXPECT_EQ(a1.size(), 0u);

    TestInterface::tStruct s1;
    // check that the elements of s1 are initialized
    EXPECT_EQ(s1.getBooleanMember(), false);
    EXPECT_EQ(s1.getUint8Member(), 0);
    EXPECT_EQ(s1.getStringMember().size(), 0u);
    EXPECT_EQ(s1.getEnumMember(), TestInterface::tEnum::VALUE1);

    TestInterface::tUnion u1;
    TestInterface::tUnion u2;
    u2 = false;
    // check that the union contains a good value
    // union is initialized with the first kind of value.
    // here, that is a boolean, and its value type is '4' - the number
    //  of types in the union.
    EXPECT_EQ(u1.getValueType(), 4);
    EXPECT_EQ(u1, u2);

    TestInterface::tMap m1;
    // check that the map is empty
    EXPECT_EQ(m1.size(), 0u);

    // Level1
    TestInterface::tStructL1 s_l1;
    // check that the elements of s_l1 are initialized
    EXPECT_EQ(s_l1.getEnumMember(), TestInterface::tEnum::VALUE1);
    EXPECT_EQ(s_l1.getArrayMemner().size(), 0u);
    EXPECT_EQ(s_l1.getStructMember().getBooleanMember(), false);
    EXPECT_EQ(s_l1.getStructMember().getUint8Member(), 0);
    EXPECT_EQ(s_l1.getStructMember().getStringMember().size(), 0u);
    EXPECT_EQ(s_l1.getStructMember().getEnumMember(), TestInterface::tEnum::VALUE1);
    EXPECT_EQ(s_l1.getUnionMember().getValueType(), 4);
    EXPECT_EQ(s_l1.getUnionMember(), u2);
    EXPECT_EQ(s_l1.getMapMember().size(), 0u);

    // Level2
    TestInterface::tStructL2 s_l2;
    // check that the elements of s_l2 are initialized
    EXPECT_EQ(s_l2.getArrayEnumMember().size(), 0u);
    EXPECT_EQ(s_l2.getArrayArrayMember().size(), 0u);
    EXPECT_EQ(s_l2.getArrayStructMember().size(), 0u);
    EXPECT_EQ(s_l2.getArrayUnionMember().size(), 0u);
    EXPECT_EQ(s_l2.getArrayMapMember().size(), 0u);
    EXPECT_EQ(s_l2.getStructL1Member().getEnumMember(), TestInterface::tEnum::VALUE1);
    EXPECT_EQ(s_l2.getStructL1Member().getArrayMemner().size(), 0u);
    EXPECT_EQ(s_l2.getStructL1Member().getStructMember().getBooleanMember(), false);
    EXPECT_EQ(s_l2.getStructL1Member().getStructMember().getUint8Member(), 0);
    EXPECT_EQ(s_l2.getStructL1Member().getStructMember().getStringMember().size(), 0u);
    EXPECT_EQ(s_l2.getStructL1Member().getStructMember().getEnumMember(), TestInterface::tEnum::VALUE1);
    EXPECT_EQ(s_l2.getStructL1Member().getUnionMember().getValueType(), 4);
    EXPECT_EQ(s_l2.getStructL1Member().getUnionMember(), u2);
    EXPECT_EQ(s_l2.getStructL1Member().getMapMember().size(), 0u);
    EXPECT_EQ(s_l2.getUnionL1Member().getValueType(), 5);
    TestInterface::tUnionL1 u_l1(TestInterface::tEnum::VALUE1);
    EXPECT_EQ(s_l2.getUnionL1Member(), u_l1);
    EXPECT_EQ(s_l2.getMapEnumMember().size(), 0u);
    EXPECT_EQ(s_l2.getMapArrayMember().size(), 0u);
    EXPECT_EQ(s_l2.getMapStructMember().size(), 0u);
    EXPECT_EQ(s_l2.getMapUnionMember().size(), 0u);
    EXPECT_EQ(s_l2.getMapMapMember().size(), 0u);

}
class DTCombined2: public ::testing::Test {
protected:
    void SetUp() {
    }

    void TearDown() {
    }
};
TEST_F(DTCombined2, VariantWithLiteralEnum) {
      TestInterface::tUnion u_l1(TestInterface::tEnum::VALUE2);
      TestInterface::tUnion u_l2(TestInterface::tEnum::VALUE2);
      TestInterface::tUnion u_l3(TestInterface::tEnum::VALUE2);
      TestInterface::tEnum e(TestInterface::tEnum::VALUE2);
      EXPECT_EQ(u_l1, u_l2);
      u_l2 = static_cast<TestInterface::tEnum>(TestInterface::tEnum::VALUE2);
      u_l2 = TestInterface::tEnum::VALUE2;
      u_l3 = e;
      EXPECT_EQ(u_l1, u_l2);
      EXPECT_EQ(u_l1, u_l3);
      EXPECT_EQ(u_l2, u_l3);
      u_l2 = (uint8_t)5;
}
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
