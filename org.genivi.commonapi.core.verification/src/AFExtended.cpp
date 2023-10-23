/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Communication
*/

#include <functional>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>
#include "CommonAPI/CommonAPI.hpp"
#include "stub/AFExtendedStub.hpp"
#include "v1/commonapi/advanced/extended/AFExtendedBaseProxy.hpp"
#include "v1/commonapi/advanced/extended/AFExtendedOnceProxy.hpp"
#include "v1/commonapi/advanced/extended/AFExtendedTwiceProxy.hpp"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddressBase = "commonapi.advanced.extended.AFExtendedBase";
const std::string testAddressOnce = "commonapi.advanced.extended.AFExtendedOnce";
const std::string testAddressTwice = "commonapi.advanced.extended.AFExtendedTwice";

const int tasync = 10000;

using namespace ::v1::commonapi::advanced::extended;

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class AFExtended: public ::testing::Test {

public:
    void recvValue(const CommonAPI::CallStatus& callStatus, uint32_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        values_.push_back(y);
    }

    void recvTimeout(const CommonAPI::CallStatus& callStatus, uint32_t y) {
        EXPECT_EQ(callStatus, CommonAPI::CallStatus::NOT_AVAILABLE);
        timeoutsOccured_.push_back(true);
        (void)y;
    }

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStubBase_ = std::make_shared<AFExtendedBaseImpl>();
        bool serviceRegistered = runtime_->registerService(domain, testAddressBase, testStubBase_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        testProxyBase_ = runtime_->buildProxy<AFExtendedBaseProxy>(domain, testAddressBase, clientId);
        ASSERT_TRUE((bool)testProxyBase_);

        testProxyBase_->isAvailableBlocking();
        ASSERT_TRUE(testProxyBase_->isAvailable());

        testStubOnce_ = std::make_shared<AFExtendedOnceImpl>();
        serviceRegistered = runtime_->registerService(domain, testAddressOnce, testStubOnce_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        testProxyOnce_ = runtime_->buildProxy<AFExtendedOnceProxy>(domain, testAddressOnce, clientId);
        ASSERT_TRUE((bool)testProxyOnce_);

        testProxyOnce_->isAvailableBlocking();
        ASSERT_TRUE(testProxyOnce_->isAvailable());

        testStubTwice_ = std::make_shared<AFExtendedTwiceImpl>();
        serviceRegistered = runtime_->registerService(domain, testAddressTwice, testStubTwice_, serviceId);
        ASSERT_TRUE(serviceRegistered);

        testProxyTwice_ = runtime_->buildProxy<AFExtendedTwiceProxy>(domain, testAddressTwice, clientId);
        ASSERT_TRUE((bool)testProxyTwice_);

        testProxyTwice_->isAvailableBlocking();
        ASSERT_TRUE(testProxyTwice_->isAvailable());
    }

    void TearDown() {
        runtime_->unregisterService(domain, AFExtendedBaseImpl::StubInterface::getInterface(), testAddressBase);
        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        if(testProxyBase_) {
            while ( testProxyBase_->isAvailable() && counter < 100 ) {
                std::this_thread::sleep_for(std::chrono::microseconds(10000));
                counter++;
            }

            ASSERT_FALSE(testProxyBase_->isAvailable());
        }

        runtime_->unregisterService(domain, AFExtendedOnceImpl::StubInterface::getInterface(), testAddressOnce);
        // wait that proxy is not available
        counter = 0;  // counter for avoiding endless loop
        if(testProxyOnce_) {
            while ( testProxyOnce_->isAvailable() && counter < 100 ) {
                std::this_thread::sleep_for(std::chrono::microseconds(10000));
                counter++;
            }

            ASSERT_FALSE(testProxyOnce_->isAvailable());
        }

        runtime_->unregisterService(domain, AFExtendedTwiceImpl::StubInterface::getInterface(), testAddressTwice);
        // wait that proxy is not available
        counter = 0;  // counter for avoiding endless loop
        if(testProxyTwice_) {
            while ( testProxyTwice_->isAvailable() && counter < 100 ) {
                std::this_thread::sleep_for(std::chrono::microseconds(10000));
                counter++;
            }

            ASSERT_FALSE(testProxyTwice_->isAvailable());
        }
        values_.clear();
        timeoutsOccured_.clear();
    }

    std::vector<uint32_t> values_;
    std::vector<bool> timeoutsOccured_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<AFExtendedBaseImpl> testStubBase_;
    std::shared_ptr<AFExtendedBaseProxy<>> testProxyBase_;
    std::shared_ptr<AFExtendedOnceImpl> testStubOnce_;
    std::shared_ptr<AFExtendedOnceProxy<>> testProxyOnce_;
    std::shared_ptr<AFExtendedTwiceImpl> testStubTwice_;
    std::shared_ptr<AFExtendedTwiceProxy<>> testProxyTwice_;

};

/**
* @test Check that method calls work through extended interfaces
*/
TEST_F(AFExtended, MethodCall) {

    uint8_t x = 5;
    uint8_t y = 0;
    CommonAPI::CallStatus callStatus;
    testProxyBase_->doSomething(x, callStatus, y);

    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 6;
    y = 0;
    testProxyOnce_->doSomething(x, callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(7, y);

    x = 7;
    y = 0;
    testProxyTwice_->doSomething(x, callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(10, y);

    x = 8;
    y = 0;
    testProxyOnce_->doSomethingSpecial(x, callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(10, y);

    x = 9;
    y = 0;
    testProxyTwice_->doSomethingSpecial(x, callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(13, y);

    x = 10;
    y = 0;
    testProxyTwice_->doSomethingExtraSpecial(x, callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(15, y);
}
/**
* @test Check that attributes work through extended interfaces
*/
TEST_F(AFExtended, Attributes) {
    CommonAPI::CallStatus callStatus;

    // setup receiver callback
    std::function<void (const CommonAPI::CallStatus&, uint8_t)> myCallback =
          std::bind(&AFExtended::recvValue, this, std::placeholders::_1, std::placeholders::_2);

    uint32_t x = 5;
    uint32_t y = 0;
    testStubBase_->setBaseAttributeAttribute(x);
    testProxyBase_->getBaseAttributeAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 6;
    y = 0;
    testStubOnce_->setBaseAttributeAttribute(x);
    testProxyOnce_->getBaseAttributeAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 7;
    y = 0;
    testStubTwice_->setBaseAttributeAttribute(x);
    testProxyTwice_->getBaseAttributeAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 8;
    y = 0;
    testStubOnce_->setSpecialAttributeAttribute(x);
    testProxyOnce_->getSpecialAttributeAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 9;
    y = 0;
    testStubTwice_->setSpecialAttributeAttribute(x);
    testProxyTwice_->getSpecialAttributeAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);

    x = 10;
    y = 0;
    testStubTwice_->setExtraSpecialAttributeAttribute(x);
    testProxyTwice_->getExtraSpecialAttributeAttribute().getValue(callStatus, y);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(x, y);
}

/**
* @test Test broadcasts. Subscribe to a broadcast, and see that the value
*  is correctly received.
*/
TEST_F(AFExtended, Broadcast) {

    CommonAPI::CallStatus callStatus;
    std::atomic<uint8_t> result;
    result = 0;

    // subscribe to broadcast
    testProxyBase_->getBBaseEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    });

    // send value '100' via a method call - this tells stub to broadcast
    uint8_t in_ = 100;
    uint8_t out_ = 0;
    testProxyBase_->doSomething(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 1) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 1);

    result = 0;
    // subscribe to broadcast
    testProxyOnce_->getBBaseEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    });

    // send value '100' via a method call - this tells stub to broadcast
    in_ = 100;
    out_ = 0;
    testProxyOnce_->doSomething(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 2) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 2);

    result = 0;
    // subscribe to broadcast
    testProxyOnce_->getBSpecialEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    });

    // send value '100' via a method call - this tells stub to broadcast
    in_ = 100;
    out_ = 0;
    testProxyOnce_->doSomethingSpecial(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 3) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 3);

    result = 0;
    // subscribe to broadcast
    testProxyTwice_->getBBaseEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    });

    // send value '100' via a method call - this tells stub to broadcast
    in_ = 100;
    out_ = 0;
    testProxyTwice_->doSomething(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 4) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 4);

    result = 0;
    // subscribe to broadcast
    testProxyTwice_->getBSpecialEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    });

    // send value '100' via a method call - this tells stub to broadcast
    in_ = 100;
    out_ = 0;
    testProxyTwice_->doSomethingSpecial(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 5) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 5);

    result = 0;
    // subscribe to broadcast
    testProxyTwice_->getBExtraSpecialEvent().subscribe([&](
        const uint8_t &y
    ) {
        result = y;
    });

    // send value '100' via a method call - this tells stub to broadcast
    in_ = 100;
    out_ = 0;
    testProxyTwice_->doSomethingExtraSpecial(in_, callStatus, out_);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);

    // check that value was correctly received
    for (int i = 0; i < 100; i++) {
        if (result == 6) break;
        std::this_thread::sleep_for(std::chrono::microseconds(tasync));
    }
    EXPECT_EQ(result, 6);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
