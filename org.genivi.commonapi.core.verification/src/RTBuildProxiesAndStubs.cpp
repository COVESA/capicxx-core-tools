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
#include "v1_0/commonapi/runtime/TestInterfaceProxy.hpp"
#include "v1_0/commonapi/runtime/TestInterfaceStubDefault.hpp"

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

    std::thread t1([&runtime](){
        auto testProxy = runtime->buildProxy<v1_0::commonapi::runtime::TestInterfaceProxy>(domain,testAddress, applicationNameClient);
        testProxy->isAvailableBlocking();
        ASSERT_TRUE((bool)testProxy);
    });


    auto testStub = std::make_shared<v1_0::commonapi::runtime::TestInterfaceStubDefault>();
    ASSERT_TRUE((bool)testStub);

    ASSERT_TRUE(runtime->registerService(domain,testAddress,testStub, applicationNameService));
    t1.join();
    ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));
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
		testProxy->isAvailableBlocking();

		std::cout << "Executing synchronous method calls (for approximately 30 seconds)" << std::endl;
		for (int i = 0; i < 30; i++) {
		    CommonAPI::CallStatus callStatus;
		    testProxy->testMethod(callStatus);
		    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

		    usleep(1 * 1000 * 1000);
		}
		std::cout << std::endl;

		ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));
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
			if (0 == i) {
				std::cout << "Wait for proxy available" << std::flush;
			}
			std::cout << "." << std::flush;
			usleep(100 * 1000);
			i++;
		}
		std::cout << std::endl;

		ASSERT_TRUE(testProxy->isAvailable());

		usleep(5 * 1000 * 1000);

		ASSERT_TRUE(runtime->unregisterService(domain,v1_0::commonapi::runtime::TestInterfaceStub::StubInterface::getInterface(), testAddress));
    }
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
