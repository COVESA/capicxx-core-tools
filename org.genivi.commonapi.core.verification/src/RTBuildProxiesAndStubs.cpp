/* Copyright (C) 2014-2015 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Runtime
*/

#include <functional>
#include <fstream>
#include <thread>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/runtime/TestInterfaceProxy.hpp"
#include "v1/commonapi/runtime/TestInterfaceStubDefault.hpp"

const std::string domain = "local";
const std::string testAddress = "commonapi.runtime.TestInterface";
const std::string applicationNameService = "service-sample";
const std::string applicationNameClient = "client-sample";

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class RTBuildProxiesAndStubs: public ::testing::Test {

protected:
    void SetUp() {
    }

    void TearDown() {
    }
};

/**
* @test Loads Runtime, creates proxy and stub/service.
*   - Calls CommonAPI::Runtime::get() and checks if return value is true.
*   - Checks if test proxy with domain and test instance can be created.
*   - Checks if test stub can be created.
*   - Register the test service.
*   - Unregister the test service.
*/
TEST_F(RTBuildProxiesAndStubs, LoadedRuntimeCanBuildProxiesAndStubs) {

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<v1_0::commonapi::runtime::TestInterfaceProxy<>> testProxy;
    std::thread t1([&runtime, &testProxy](){
        testProxy = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
        testProxy->isAvailableBlocking();
        ASSERT_TRUE((bool)testProxy);
    });


    auto testStub = std::make_shared<v1_0::commonapi::runtime::TestInterfaceStubDefault>();
    ASSERT_TRUE((bool)testStub);

    ASSERT_TRUE(runtime->registerService(domain,testAddress,testStub, applicationNameService));
    if (t1.joinable()) {
        t1.join();
    }
    ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));

    int counter = 0;
    while (testProxy->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        counter++;
    }
    ASSERT_FALSE(testProxy->isAvailable());
}

/**
* @test Loads Runtime, creates proxy and stub/service two times.
*   - Calls CommonAPI::Runtime::get() and checks if return value is true
*   - Create stub and register service
*   - Create proxy
*   - Do some synchronous calls
*   - Unregister the service.
*   - Create stub and register service
*   - Create proxy
*   - Checks whether proxy is available
*   - Unregister the service
*/
TEST_F(RTBuildProxiesAndStubs, BuildProxiesAndStubsTwoTimes) {

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

    // first build sequence for proxy and stub
    {
        auto testStub = std::make_shared<v1_0::commonapi::runtime::TestInterfaceStubDefault>();
        ASSERT_TRUE((bool)testStub);
        ASSERT_TRUE(runtime->registerService(domain,testAddress,testStub, applicationNameService));

        auto testProxy = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
        ASSERT_TRUE((bool)testProxy);
        int i = 0;
        while(!testProxy->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy->isAvailable());

        for (int i = 0; i < 100; i++) {
            CommonAPI::CallStatus callStatus;
            testProxy->testMethod(callStatus);
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

            std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }

        ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));

        int counter = 0;
        while (testProxy->isAvailable() && counter < 100 ) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
            counter++;
        }
        ASSERT_FALSE(testProxy->isAvailable());
    }

    // second build sequence for proxy and stub
    {
        auto testStub = std::make_shared<v1_0::commonapi::runtime::TestInterfaceStubDefault>();
        ASSERT_TRUE((bool)testStub);
        ASSERT_TRUE(runtime->registerService(domain,testAddress,testStub, applicationNameService));

        auto testProxy = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
        ASSERT_TRUE((bool)testProxy);

        int i = 0;
        while( (i < 100)  && (!testProxy->isAvailable()) ) {
            std::this_thread::sleep_for(std::chrono::microseconds(100 * 1000));
            i++;
        }

        ASSERT_TRUE(testProxy->isAvailable());
        ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));

        int counter = 0;
        while (testProxy->isAvailable() && counter < 100 ) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
            counter++;
        }
        ASSERT_FALSE(testProxy->isAvailable());
    }
}

/**
* @test Loads Runtime, creates proxy two times with reassigning and create stub/service.
*   - Calls CommonAPI::Runtime::get() and checks if return value is true
*   - Create proxy
*   - Create proxy again and reassign
*   - Create stub and register service
*   - Checks whether proxy is available
*   - Do synchronous calls
*   - Unregister the service.
*/
TEST_F(RTBuildProxiesAndStubs, BuildProxyTwoTimesWithReassigningAndStub) {

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

    auto testProxy = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
    ASSERT_TRUE((bool)testProxy);

    testProxy = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
    ASSERT_TRUE((bool)testProxy);

    auto testStub = std::make_shared<v1_0::commonapi::runtime::TestInterfaceStubDefault>();
    ASSERT_TRUE((bool)testStub);
    ASSERT_TRUE(runtime->registerService(domain,testAddress,testStub, applicationNameService));

    int i = 0;
    while( (i < 100)  && (!testProxy->isAvailable()) ) {
        std::this_thread::sleep_for(std::chrono::microseconds(100 * 1000));
        i++;
    }

    ASSERT_TRUE(testProxy->isAvailable());

    CommonAPI::CallStatus callStatus;
    testProxy->testMethod(callStatus);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));

    int counter = 0;
    while (testProxy->isAvailable() && counter < 100 ) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        counter++;
    }
    ASSERT_FALSE(testProxy->isAvailable());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
