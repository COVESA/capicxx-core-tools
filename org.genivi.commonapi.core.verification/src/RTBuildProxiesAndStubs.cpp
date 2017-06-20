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
        ASSERT_TRUE((bool)testProxy);
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
    ASSERT_TRUE((bool)testProxy);
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

/**
* @test Loads Runtime, creates proxy and stub/service, await proxy destruction
*   - Calls CommonAPI::Runtime::get() and checks if return value is true.
*   - Checks if test proxy with domain and test instance can be created
*   - Checks if test stub can be created.
*   - Register the test service.
*   - Wait for service availability
*   - Unregister the test service.
*   - Wait for on future till proxy was destroyed after std::shared_ptr<> ref from thread was released
*/
TEST_F(RTBuildProxiesAndStubs, WaitForProxyDestruction) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<v1_0::commonapi::runtime::TestInterfaceProxy<>> testProxy = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
    ASSERT_TRUE((bool)testProxy);

    auto testStub = std::make_shared<v1_0::commonapi::runtime::TestInterfaceStubDefault>();
    ASSERT_TRUE((bool)testStub);

    ASSERT_TRUE(runtime->registerService(domain,testAddress,testStub, applicationNameService));

    for (int i = 0; i < 100; i++) {
        if (testProxy->isAvailable()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(testProxy->isAvailable());

    ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));

    std::shared_future<void> proxy_destroyed = testProxy->getCompletionFuture();
    // have to release own (one and only) ref
    testProxy.reset();
    ASSERT_TRUE(std::future_status::ready == proxy_destroyed.wait_for(std::chrono::seconds(5)));
}

/**
* @test Loads Runtime, creates proxy and stub/service, await proxy destruction
*   - Calls CommonAPI::Runtime::get() and checks if return value is true.
*   - Checks if test proxy with domain and test instance can be created (in an own thread).
*   - Checks if test stub can be created.
*   - Register the test service.
*   - Wait for service availability on the test proxy in it's thread.
*   - Unregister the test service.
*   - Wait till proxy was destroyed when std::shared_ptr<> in thread has been released.
*/
TEST_F(RTBuildProxiesAndStubs, WaitForProxyDestructionCreatedInThread) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

    std::future<void> proxy_destroyed;

    std::thread t1([&runtime, &proxy_destroyed](){
        std::shared_ptr<v1_0::commonapi::runtime::TestInterfaceProxy<>> testProxyInner = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
        ASSERT_TRUE((bool)testProxyInner);
        proxy_destroyed = testProxyInner->getCompletionFuture();
        for (int i = 0; i < 100; i++) {
            if (testProxyInner->isAvailable()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        EXPECT_TRUE(testProxyInner->isAvailable());
    });

    auto testStub = std::make_shared<v1_0::commonapi::runtime::TestInterfaceStubDefault>();
    ASSERT_TRUE((bool)testStub);

    ASSERT_TRUE(runtime->registerService(domain,testAddress,testStub, applicationNameService));
    if (t1.joinable()) {
       t1.join();
    }
    ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));

    ASSERT_TRUE(std::future_status::ready == proxy_destroyed.wait_for(std::chrono::seconds(5)));
}

/**
* @test Loads Runtime, creates proxy and stub/service, await proxy destruction in two threads
*   - Calls CommonAPI::Runtime::get() and checks if return value is true.
*   - Checks if test proxy with domain and test instance can be created (in an own thread).
*   - Wait till proxy was destroyed when std::shared_ptr<> in threads
*   - Join the threads that have been waiting for proxy destruction
*/
TEST_F(RTBuildProxiesAndStubs, WaitForProxyDestructionInTwoThreads) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);

    std::shared_ptr<v1_0::commonapi::runtime::TestInterfaceProxy<>> testProxy = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
    ASSERT_TRUE((bool)testProxy);

    auto testStub = std::make_shared<v1_0::commonapi::runtime::TestInterfaceStubDefault>();
    ASSERT_TRUE((bool)testStub);

    ASSERT_TRUE(runtime->registerService(domain,testAddress,testStub, applicationNameService));

    for (int i = 0; i < 100; i++) {
        if (testProxy->isAvailable()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(testProxy->isAvailable());

    ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));


    // convert "normal" std::future to std::shared_future to be able to share it between
    // threads to avoid exception "std::future_error: Future already retrieved" when
    // calling proxy's getCompletionFuture() two or more times
    std::shared_future<void> proxy_destroyed = testProxy->getCompletionFuture();

    std::thread t1([&runtime, &proxy_destroyed](){
        ASSERT_TRUE(std::future_status::ready == proxy_destroyed.wait_for(std::chrono::seconds(5)));
    });

    std::thread t2([&runtime, &proxy_destroyed](){
        ASSERT_TRUE(std::future_status::ready == proxy_destroyed.wait_for(std::chrono::seconds(5)));
    });

    testProxy.reset();

    if (t1.joinable()) {
       t1.join();
    }

    if (t2.joinable()) {
       t2.join();
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
