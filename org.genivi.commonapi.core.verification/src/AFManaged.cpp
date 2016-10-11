// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <thread>
#include <iostream>
#include <mutex>
#include <set>

#include <CommonAPI/CommonAPI.hpp>
#include "stub/AFManagedStub.h"
#include "stub/DeviceStubImpl.h"
#include "stub/SpecialDeviceStubImpl.h"
#include "stub/HLevelTopStubImpl.h"
#include "stub/HLevelMiddleStubImpl.h"
#include "stub/HLevelBottomStubImpl.h"

#include <v1/commonapi/advanced/managed/ManagerProxy.hpp>
#include <v1/commonapi/advanced/managed/DeviceProxy.hpp>
#include <v1/commonapi/advanced/managed/SpecialDeviceProxy.hpp>
#include <v1/commonapi/advanced/managed/HLevelTopProxy.hpp>
#include <v1/commonapi/advanced/managed/HLevelMiddleProxy.hpp>
#include <v1/commonapi/advanced/managed/HLevelBottomProxy.hpp>
#include <gtest/gtest.h>

using namespace v1_0::commonapi::advanced::managed;

const std::string &domain = "local";
const static std::string managerInstanceName = "commonapi.advanced.managed.Manager";
const static std::string connectionIdService = "service-sample";
const static std::string connectionIdClient = "client-sample";
const static std::string connectionIdClient2 = "other-client-sample";

const static std::string interfaceDevice = "commonapi.advanced.managed.Device:v1_0";
const static std::string addressDevice1 = "local:" + interfaceDevice + ":commonapi.advanced.managed.Manager.device01";
const static std::string addressDevice2 = "local:" + interfaceDevice + ":commonapi.advanced.managed.Manager.device02";
const static std::string interfaceSpecialDevice = "commonapi.advanced.managed.SpecialDevice:v1_0";
const static std::string addressSpecialDevice1 = "local:" + interfaceSpecialDevice + ":commonapi.advanced.managed.Manager.specialDevice00";

const static std::string hLevelTopStubInstanceName = "commonapi.advanced.managed.HLevelTop";
const static std::string hLevelMiddleInterface = "commonapi.advanced.managed.HLevelMiddle:v1_0";
const static std::string hLevelMiddleAddress = "local:" + hLevelMiddleInterface + ":commonapi.advanced.managed.HLevelTop.middle01";
const static std::string hLevelMiddleStubInstanceName = "commonapi.advanced.managed.HLevelTop.middle01";
const static std::string hLevelBottomInterface = "commonapi.advanced.managed.HLevelBottom:v1_0";
const static std::string hLevelBottomAddress = "local:" + hLevelBottomInterface + ":commonapi.advanced.managed.HLevelTop.middle01.bottom01";
const static std::string hLevelBottomStubInstanceName = "commonapi.advanced.managed.HLevelTop.middle01.bottom01";

const static int sleepMilli = 10;

class AFManaged: public ::testing::Test {

public:
AFManaged() :
    received_(false),
    serviceRegistered_(false),
    subscriptionIdSpecialDevice_(0),
    subscriptionIdDevice_(0),
    deviceAvailableCount_(0),
    specialDeviceAvailableCount_(0),
    specialDeviceAvailableDesiredValue_(0),
    deviceAvailableDesiredValue_(0),
    instanceAvailabilityStatusCallbackCalled_(false),
    proxyAvailableAndMethodCallSucceeded_(false) {}

public:

    void getInstanceAvailabilityStatusAsyncCallbackAvailable(
            const CommonAPI::CallStatus &_callStatus,
            const CommonAPI::AvailabilityStatus &_availabilityStatus) {
        ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, _callStatus);
        ASSERT_EQ(CommonAPI::AvailabilityStatus::AVAILABLE, _availabilityStatus);
        instanceAvailabilityStatusCallbackCalled_ = true;
    }

    void getInstanceAvailabilityStatusAsyncCallbackNotAvailable(
            const CommonAPI::CallStatus &_callStatus,
            const CommonAPI::AvailabilityStatus &_availabilityStatus) {
        ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, _callStatus);
        ASSERT_EQ(CommonAPI::AvailabilityStatus::NOT_AVAILABLE, _availabilityStatus);
        instanceAvailabilityStatusCallbackCalled_ = true;
    }

protected:
    void SetUp() {
        // CommonAPI::Runtime::setProperty("LibraryBase", "AFManaged");
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStub_ = std::make_shared<AFManagedStub>(managerInstanceName);
        serviceRegistered_ = runtime_->registerService(domain, managerInstanceName, testStub_, connectionIdService);
        ASSERT_TRUE(serviceRegistered_);

        testProxy_ = runtime_->buildProxy<ManagerProxy>(domain, managerInstanceName, connectionIdClient);
        ASSERT_TRUE((bool)testProxy_);

        int i = 0;
        while(!testProxy_->isAvailable() && i++ < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_TRUE(testProxy_->isAvailable());
    }

    void TearDown() {
        // Unregister callbacks
        CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& deviceEvent =
                testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusChangedEvent();
        CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& specialDeviceEvent =
                testProxy_->getProxyManagerSpecialDevice().getInstanceAvailabilityStatusChangedEvent();
        specialDeviceEvent.unsubscribe(subscriptionIdSpecialDevice_);
        deviceEvent.unsubscribe(subscriptionIdDevice_);
        runtime_->unregisterService(domain, AFManagedStub::StubInterface::getInterface(), managerInstanceName);

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 10 ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            counter++;
        }
        ASSERT_FALSE(testProxy_->isAvailable());

        if(deviceProxy_) {
            counter = 0;
            while ( deviceProxy_->isAvailable() && counter < 10 ) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                counter++;
            }
            ASSERT_FALSE(deviceProxy_->isAvailable());
        }

    }

    void subscribe() {
        // Get the events
        CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& deviceEvent =
                testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusChangedEvent();
        CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& specialDeviceEvent =
                testProxy_->getProxyManagerSpecialDevice().getInstanceAvailabilityStatusChangedEvent();

        // bind callbacks to member functions
        newDeviceAvailableCallbackFunc_ = std::bind(
                &AFManaged::newDeviceAvailableCallback, this,
                std::placeholders::_1, std::placeholders::_2);
        newSpecialDeviceAvailableCallbackFunc_ = std::bind(
                &AFManaged::newSpecialDeviceAvailableCallback, this,
                std::placeholders::_1, std::placeholders::_2);

        newDeviceAvailableBuildProxyCallbackFunc_ = std::bind(
                &AFManaged::newDeviceAvailableBuildProxyCallback, this,
                std::placeholders::_1, std::placeholders::_2);
        newDeviceAvailableBuildProxyAndSubscribeToProxyStatusEventCallbackFunc_ = std::bind(
                &AFManaged::newDeviceAvailableBuildProxyAndSubscribeToProxyStatusEventCallback, this,
                std::placeholders::_1, std::placeholders::_2);

        // register callbacks
        subscriptionIdSpecialDevice_ = specialDeviceEvent.subscribe(
                newSpecialDeviceAvailableCallbackFunc_);
        ASSERT_EQ(subscriptionIdSpecialDevice_,
                static_cast<CommonAPI::Event<>::Subscription>(0));

        subscriptionIdDevice_ = deviceEvent.subscribe(
                newDeviceAvailableCallbackFunc_);
        ASSERT_EQ(subscriptionIdDevice_,
                static_cast<CommonAPI::Event<>::Subscription>(0));
    }



    void newDeviceAvailableCallback(const std::string _address,
                            const CommonAPI::AvailabilityStatus _status) {
        ASSERT_TRUE(_address == addressDevice1 || _address == addressDevice2);
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
            //std::cout << "New device available: " << _address << std::endl;
            deviceAvailableCount_++;
            devicesAvailable_.insert(CommonAPI::Address(_address));
        }

        if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
            //std::cout << "Device removed: " << _address << std::endl;
            deviceAvailableCount_--;
            devicesAvailable_.erase(CommonAPI::Address(_address));
        }
    }

    void newDeviceAvailableBuildProxyCallback(const std::string _address,
                                              const CommonAPI::AvailabilityStatus _status) {
        ASSERT_TRUE(_address == addressDevice1 || _address == addressDevice2);
        std::shared_ptr<DeviceProxy<>> deviceProxy;
        if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
            const std::string deviceInstance(managerInstanceName + ".device01");
            if(testProxy_->isAvailable()) {
                deviceProxy = testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                        deviceInstance, connectionIdClient2);   /* when using dbus a further connection must
                                                                 * be used. Otherwise the proxy doesn't become available.
                                                                 * when using someip the number of dispatchers must be set to > 1
                                                                 * if no further connection should be used.
                                                                 */

                while (!deviceProxy->isAvailable()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
                deviceProxy->doSomething(call);
                ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
                proxyAvailableAndMethodCallSucceeded_ = true;
            }
        }

        if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
            proxyAvailableAndMethodCallSucceeded_ = false;
        }
    }

    void newDeviceAvailableBuildProxyAndSubscribeToProxyStatusEventCallback(const std::string _address,
                                                                            const CommonAPI::AvailabilityStatus _status) {
        ASSERT_TRUE(_address == addressDevice1 || _address == addressDevice2);
        if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
            const std::string deviceInstance(managerInstanceName + ".device01");
            if(testProxy_->isAvailable()) {
                deviceProxy_ = testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                        deviceInstance);    /*
                                             * when using someip the number of dispatchers must be set to > 1. Otherwise
                                             * a further connection must be used.
                                             */
                deviceProxy_->getProxyStatusEvent().subscribe([&]
                                                              (const CommonAPI::AvailabilityStatus _status) {
                    if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
                        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
                        deviceProxy_->doSomething(call);
                        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
                        proxyAvailableAndMethodCallSucceeded_ = true;
                    }
                });
            }
        }
        if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
            deviceProxy_.reset();
            proxyAvailableAndMethodCallSucceeded_ = false;
        }
    }

    void newSpecialDeviceAvailableCallback(
            const std::string _address,
            const CommonAPI::AvailabilityStatus _status) {
        ASSERT_TRUE(_address == addressSpecialDevice1);
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
            //std::cout << "New device available: " << _address << std::endl;
            specialDeviceAvailableCount_++;
            specialDevicesAvailable_.insert(CommonAPI::Address(_address));
        }

        if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
            //std::cout << "Device removed: " << _address << std::endl;
            specialDeviceAvailableCount_--;
            specialDevicesAvailable_.erase(CommonAPI::Address(_address));
        }
    }

    bool checkInstanceAvailabilityStatus(
            CommonAPI::ProxyManager* _proxyMananger,
            const CommonAPI::Address& _instanceAddress) {
        CommonAPI::CallStatus callStatus(CommonAPI::CallStatus::UNKNOWN);
        CommonAPI::AvailabilityStatus availabilityStatus(
                CommonAPI::AvailabilityStatus::UNKNOWN);
        _proxyMananger->getInstanceAvailabilityStatus(_instanceAddress.getInstance(),
                callStatus, availabilityStatus);
        if (callStatus == CommonAPI::CallStatus::SUCCESS &&
            availabilityStatus == CommonAPI::AvailabilityStatus::AVAILABLE) {
            return true;
        } else {
            return false;
        }
    }

    void getAvailableInstancesAsyncSpecialDeviceCallback(
            const CommonAPI::CallStatus &_callStatus,
            const std::vector<std::string> &_availableSpecialDevices) {
        ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, _callStatus);
        ASSERT_EQ(_availableSpecialDevices.size(), std::vector<std::string>::size_type(specialDeviceAvailableDesiredValue_));
        bool allDetectedAreAvailable = false;
        for(auto &i : _availableSpecialDevices) {
            for(auto &j : specialDevicesAvailable_) {
                if(i == j.getInstance()) {
                    allDetectedAreAvailable = true;
                    break;
                }
            }
            ASSERT_TRUE(allDetectedAreAvailable);
        }
    }

    void getAvailableInstancesAsyncDeviceCallback(
            const CommonAPI::CallStatus &_callStatus,
            const std::vector<std::string> &_availableDevices) {
        ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, _callStatus);
        ASSERT_EQ(_availableDevices.size(), std::vector<std::string>::size_type(deviceAvailableDesiredValue_));
        bool allDetectedAreAvailable = false;
        for(auto &i : _availableDevices) {
            for(auto &j : devicesAvailable_) {
                if(i == j.getInstance()) {
                    allDetectedAreAvailable = true;
                    break;
                }
            }
            ASSERT_TRUE(allDetectedAreAvailable);
        }
    }

    bool received_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<ManagerProxy<>> testProxy_;
    std::shared_ptr<AFManagedStub> testStub_;
    std::shared_ptr<DeviceProxy<>> deviceProxy_;

    std::function<void(const std::string, const CommonAPI::AvailabilityStatus)> newDeviceAvailableCallbackFunc_;
    std::function<void(const std::string, const CommonAPI::AvailabilityStatus)> newDeviceAvailableBuildProxyCallbackFunc_;
    std::function<void(const std::string, const CommonAPI::AvailabilityStatus)> newDeviceAvailableBuildProxyAndSubscribeToProxyStatusEventCallbackFunc_;
    std::function<void(const std::string, const CommonAPI::AvailabilityStatus)> newSpecialDeviceAvailableCallbackFunc_;
    CommonAPI::Event<>::Subscription subscriptionIdSpecialDevice_;
    CommonAPI::Event<>::Subscription subscriptionIdDevice_;

    int deviceAvailableCount_;
    int specialDeviceAvailableCount_;

    std::mutex deviceAvailableCountMutex_;
    std::mutex specialDeviceAvailableCountMutex_;

    std::set<CommonAPI::Address> devicesAvailable_;
    std::set<CommonAPI::Address> specialDevicesAvailable_;

    int specialDeviceAvailableDesiredValue_;
    int deviceAvailableDesiredValue_;

    bool instanceAvailabilityStatusCallbackCalled_;
    bool proxyAvailableAndMethodCallSucceeded_;
};

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Remove the managed interface from the manager
 *  - Check that the client is notified about the removed interface
 */
TEST_F(AFManaged, AddRemoveManagedInterfaceSingle) {
    subscribe();

    // Add
    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    // Remove
    testStub_->deviceRemoved(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 0);
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Add a second instance of the same managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Remove all the managed interfaces from the manager
 *  - Check that the client is notified about the removed interfaces
 */
TEST_F(AFManaged, AddRemoveManagedInterfaceMultiple) {
    subscribe();

    // Add
    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    testStub_->deviceDetected(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 2);

    // Remove
    testStub_->deviceRemoved(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    testStub_->deviceRemoved(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 0);
}

/**
 * @test
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Add a different managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Remove all the managed interfaces from the manager
 *  - Check that the client is notified about the removed interfaces
 */
TEST_F(AFManaged, AddRemoveMultipleManagedInterfacesSingle) {
    subscribe();
    // Add
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);

    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    // Remove
    testStub_->specialDeviceRemoved(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 0);

    testStub_->deviceRemoved(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 0);
}

/**
 * @test
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Add a different managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Add a second instance of the same managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Remove all the managed interfaces from the manager
 *  - Check that the client is notified about the removed interfaces
 */
TEST_F(AFManaged, AddRemoveMultipleManagedInterfacesMultiple) {
    subscribe();
    // Add
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);

    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    testStub_->deviceDetected(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 2);

    // Remove
    testStub_->specialDeviceRemoved(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 0);

    testStub_->deviceRemoved(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    testStub_->deviceRemoved(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 0);
}

/**
 * @test
 *  -
 */
TEST_F(AFManaged, AddRemoveMultipleManagedInterfacesMultipleProxyNotActive) {
    testProxy_.reset();

    testStub_->specialDeviceDetected(0);
    testStub_->deviceDetected(1);

    testProxy_ = runtime_->buildProxy<ManagerProxy>(domain, managerInstanceName, connectionIdClient);
    ASSERT_TRUE((bool)testProxy_);

    int i = 0;
    while(!testProxy_->isAvailable() && i++ < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(testProxy_->isAvailable());

    subscribe();

    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);

    for (int i = 0; i < 100; i++) {
         {
             std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
             if (deviceAvailableCount_ == 1) break;
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
     }
     ASSERT_EQ(deviceAvailableCount_, 1);

     testStub_->deviceDetected(2);
     for (int i = 0; i < 100; i++) {
         {
             std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
             if (deviceAvailableCount_ == 2) break;
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
     }
     ASSERT_EQ(deviceAvailableCount_, 2);

     // Remove
     testStub_->specialDeviceRemoved(0);
     for (int i = 0; i < 100; i++) {
         {
             std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
             if (specialDeviceAvailableCount_ == 0) break;
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
     }
     ASSERT_EQ(specialDeviceAvailableCount_, 0);

     testStub_->deviceRemoved(1);
     for (int i = 0; i < 100; i++) {
         {
             std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
             if (deviceAvailableCount_ == 1) break;
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
     }
     ASSERT_EQ(deviceAvailableCount_, 1);

     testStub_->deviceRemoved(2);
     for (int i = 0; i < 100; i++) {
         {
             std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
             if (deviceAvailableCount_ == 0) break;
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
     }
     ASSERT_EQ(deviceAvailableCount_, 0);
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Remove the managed interface from the manager
 *  - Check that the client is notified about the removed interface
 */
TEST_F(AFManaged, ProxyAddRemoveManagedInterfaceSingle) {
    CommonAPI::CallStatus callStatus;

    subscribe();

    // Add
    testProxy_->addDevice(1, callStatus);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    // Remove
    testProxy_->removeDevice(1, callStatus);
    EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(deviceAvailableCount_, 0);
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Build a proxy through the manager to the managed device
 *  - Call a method on the managed device and check call status
 *  - Explicitly deregister managed interface through its instance name
 */
TEST_F(AFManaged, BuildProxyThroughManagerAndMethodCallSingleDeregistrationExplicit) {
    subscribe();
    // Add
    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    const std::string deviceInstance(managerInstanceName + ".device01");
    if(testProxy_->isAvailable()) {
        std::shared_ptr<DeviceProxy<>> deviceProxy =
                testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                        deviceInstance);
        while (!deviceProxy->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        deviceProxy->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    // Remove
    testStub_->deviceRemoved(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(deviceAvailableCount_, 0);
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Build a proxy through the manager to the managed device
 *  - Call a method on the managed device and check call status
 *  - Deregister all managed interfaces through manager's stub adapter
 */
TEST_F(AFManaged, BuildProxyThroughManagerAndMethodCallSingleDeregistrationExplicitAll) {
    subscribe();
    // Add
    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    const std::string deviceInstance(managerInstanceName + ".device01");
    if(testProxy_->isAvailable()) {
        std::shared_ptr<DeviceProxy<>> deviceProxy =
                testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                        deviceInstance);
        while (!deviceProxy->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        deviceProxy->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }
    testStub_->getStubAdapter()->deactivateManagedInstances();
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(deviceAvailableCount_, 0);
}


/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Build a proxy through the manager to the managed device
 *  - Call a method on the managed device and check call status
 *  - Don't deregister managed interfaces. This is done in dtor of manager's
 *    StubAdapterInternal when manager service is unregistered in
 *    TearDown() method.
 */
TEST_F(AFManaged, BuildProxyThroughManagerAndMethodCallSingleDeregistrationImplicit) {
    subscribe();
    // Add
    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    const std::string deviceInstance(managerInstanceName + ".device01");
    if(testProxy_->isAvailable()) {
        std::shared_ptr<DeviceProxy<>> deviceProxy =
                testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                        deviceInstance);
        while (!deviceProxy->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        deviceProxy->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add managed interfaces to the manager
 *  - Check that the client is notified about the newly added interfaces
 *  - Build proxies through the manager to the managed interfaces
 *  - Call a method on the managed interfaces and check call status
 *  - Explicitly deregister managed interfaces through their instance name
 */
TEST_F(AFManaged, BuildProxyThroughManagerAndMethodCallMultipleDeregistrationExplicit) {
    subscribe();
    // Add
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    testStub_->deviceDetected(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 2);

    const std::string specialDeviceInstance(managerInstanceName + ".specialDevice00");
    const std::string deviceInstance01(managerInstanceName + ".device01");
    const std::string deviceInstance02(managerInstanceName + ".device02");
    std::vector<std::shared_ptr<DeviceProxy<>>> deviceProxies;
    std::vector<std::shared_ptr<SpecialDeviceProxy<>>> specialDeviceProxies;

    if(testProxy_->isAvailable()) {
        for (int var = 0; var < 20; ++var) {
            specialDeviceProxies.push_back(
                    testProxy_->getProxyManagerSpecialDevice().buildProxy<
                            SpecialDeviceProxy>(specialDeviceInstance));
            deviceProxies.push_back(
                    testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                            deviceInstance01));
            deviceProxies.push_back(
                    testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                            deviceInstance02));
        }
    }

    for(const auto &dp : deviceProxies) {
        while (!dp->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        dp->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    for(const auto &sdp : specialDeviceProxies) {
        while (!sdp->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        sdp->doSomethingSpecial(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    // Remove
    testStub_->specialDeviceRemoved(0);
    testStub_->deviceRemoved(1);
    testStub_->deviceRemoved(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0 && specialDeviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(deviceAvailableCount_, 0);
    ASSERT_LE(specialDeviceAvailableCount_, 0);
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add managed interfaces to the manager
 *  - Check that the client is notified about the newly added interfaces
 *  - Build proxies through the manager to the managed interfaces
 *  - Call a method on the managed interfaces and check call status
 *  - Deregister all managed interfaces through manager's stub adapter
 */
TEST_F(AFManaged, BuildProxyThroughManagerAndMethodCallMultipleDeregistrationExplicitAll) {
    subscribe();
    // Add
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    testStub_->deviceDetected(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 2);

    const std::string specialDeviceInstance(managerInstanceName + ".specialDevice00");
    const std::string deviceInstance01(managerInstanceName + ".device01");
    const std::string deviceInstance02(managerInstanceName + ".device02");
    std::vector<std::shared_ptr<DeviceProxy<>>> deviceProxies;
    std::vector<std::shared_ptr<SpecialDeviceProxy<>>> specialDeviceProxies;

    if(testProxy_->isAvailable()) {
        for (int var = 0; var < 20; ++var) {
            specialDeviceProxies.push_back(
                    testProxy_->getProxyManagerSpecialDevice().buildProxy<
                            SpecialDeviceProxy>(specialDeviceInstance));
            deviceProxies.push_back(
                    testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                            deviceInstance01));
            deviceProxies.push_back(
                    testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                            deviceInstance02));
        }
    }

    for(const auto &dp : deviceProxies) {
        while (!dp->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        dp->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    for(const auto &sdp : specialDeviceProxies) {
        while (!sdp->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        sdp->doSomethingSpecial(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    // Remove all
    testStub_->getStubAdapter()->deactivateManagedInstances();
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0 && specialDeviceAvailableCount_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(deviceAvailableCount_, 0);
    ASSERT_LE(specialDeviceAvailableCount_, 0);
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add managed interfaces to the manager
 *  - Check that the client is notified about the newly added interfaces
 *  - Build proxies through the manager to the managed interfaces
 *  - Call a method on the managed interfaces and check call status
 *  - Don't deregister managed interfaces. This is done in dtor of manager's
 *    StubAdapterInternal when manager service is unregistered in
 *    TearDown() method.
 */
TEST_F(AFManaged, BuildProxyThroughManagerAndMethodCallMultipleDeregistrationImplicit) {
    subscribe();
    // Add
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    testStub_->deviceDetected(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 2);

    const std::string specialDeviceInstance(managerInstanceName + ".specialDevice00");
    const std::string deviceInstance01(managerInstanceName + ".device01");
    const std::string deviceInstance02(managerInstanceName + ".device02");
    std::vector<std::shared_ptr<DeviceProxy<>>> deviceProxies;
    std::vector<std::shared_ptr<SpecialDeviceProxy<>>> specialDeviceProxies;

    if(testProxy_->isAvailable()) {
        for (int var = 0; var < 20; ++var) {
            specialDeviceProxies.push_back(
                    testProxy_->getProxyManagerSpecialDevice().buildProxy<
                            SpecialDeviceProxy>(specialDeviceInstance));
            deviceProxies.push_back(
                    testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                            deviceInstance01));
            deviceProxies.push_back(
                    testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
                            deviceInstance02));
        }
    }

    for(const auto &dp : deviceProxies) {
        while (!dp->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        dp->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    for(const auto &sdp : specialDeviceProxies) {
        while (!sdp->isAvailable()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        sdp->doSomethingSpecial(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Build a proxy through the manager to the managed device inside the
 *    availability event/callback
 *  - Call a method on the managed device and check call status inside the
 *    availability event/callback
 *  - Explicitly deregister managed interface through its instance name
 */
TEST_F(AFManaged, BuildProxyThroughManagerInAvailabilityEventAndMethodCallSingleDeregistrationExplicit) {
    subscribe();
    CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& deviceEvent =
             testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusChangedEvent();
    //Subscribe
    auto subscriptionIdDevice = deviceEvent.subscribe(
                    newDeviceAvailableBuildProxyCallbackFunc_);
    ASSERT_EQ(subscriptionIdDevice,
              static_cast<CommonAPI::Event<>::Subscription>(1));

    // Add
    testStub_->deviceDetected(1);
    {
        for (int i = 0; i < 100; ++i) {
            if (deviceAvailableCount_ == 1 && proxyAvailableAndMethodCallSucceeded_) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
        }
        ASSERT_EQ(deviceAvailableCount_, 1);
        ASSERT_TRUE(proxyAvailableAndMethodCallSucceeded_);
    }

    // Remove
    testStub_->deviceRemoved(1);
    {
        for (int i = 0; i < 100; ++i) {
            if (deviceAvailableCount_ == 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
        }
        ASSERT_LE(deviceAvailableCount_, 0);
    }
}

/**
 * @test
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Build a proxy through the manager to the managed device inside the
 *    availability event/callback
 *  - Subscribe to the proxy status event
 *  - Call a method on the managed device and check call status inside the
 *    proxy status event/callback (status == CommonAPI::AVAILABILITY_STATUS::AVAILABLE)
 *  - Remove and add the managed interface to the manager a few times
 *  - Explicitly deregister managed interface through its instance name
 */
TEST_F(AFManaged, BuildProxyThroughManagerInAvailabilityEventAndMethodCallInProxyStatusEventSingleDeregistrationExplicit) {
    subscribe();
    CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& deviceEvent =
             testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusChangedEvent();
    //Subscribe
    auto subscriptionIdDevice = deviceEvent.subscribe(
                    newDeviceAvailableBuildProxyAndSubscribeToProxyStatusEventCallbackFunc_);
    ASSERT_EQ(subscriptionIdDevice,
              static_cast<CommonAPI::Event<>::Subscription>(1));

    for(int i=0; i < 4; i++) {
        // Add
        testStub_->deviceDetected(1);
        {
            for (int i = 0; i < 100; ++i) {
                if (deviceAvailableCount_ == 1 && proxyAvailableAndMethodCallSucceeded_) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
            }
            ASSERT_EQ(deviceAvailableCount_, 1);
            ASSERT_TRUE(proxyAvailableAndMethodCallSucceeded_);
        }

        // Remove
        testStub_->deviceRemoved(1);
        {
            for (int i = 0; i < 100; ++i) {
                if (deviceAvailableCount_ == 0) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
            }
            ASSERT_LE(deviceAvailableCount_, 0);
        }
    }
}

/**
 * @test
 *  - Subscribe to the proxy status event of the manager
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add the managed interfaces to the manager
 *  - Check that the client is notified about the newly added interfaces
 *  - Unregister manager service
 *  - Explicitly delete the proxy of the manager inside the proxy status event
 *    callback
 *  - Register manager service and build new proxy (Setup())
 *  - Subscribe on the events about availability status changes at the manager
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Build a proxy through the manager to the managed device
 *  - Call a method on the managed device and check call status
 *  - TearDown()
 */
TEST_F(AFManaged, DeleteManagerProxyInsideProxyStatusEventCallbackAndMethodCall) {

    bool managerProxyDeleted = false;
    testProxy_->getProxyStatusEvent().subscribe([&] (const CommonAPI::AvailabilityStatus _status) {
        if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
            // delete proxy
            testProxy_.reset();
            managerProxyDeleted = true;
        }
    });

    subscribe();

    // Add devices
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);

    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    // unregister manager service
    runtime_->unregisterService(domain, AFManagedStub::StubInterface::getInterface(), managerInstanceName);
    for (int i = 0; i < 100; i++) {
        if (managerProxyDeleted) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_TRUE(managerProxyDeleted);

    // do a setup
    SetUp();

    subscribe();

    // Add device
    deviceAvailableCountMutex_.lock();
    deviceAvailableCount_ = 0;
    deviceAvailableCountMutex_.unlock();

    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);

    const std::string deviceInstance(managerInstanceName + ".device01");
    deviceProxy_ = testProxy_->getProxyManagerDevice().buildProxy<DeviceProxy>(
              deviceInstance, connectionIdClient2);
    int i = 0;
    while ( !deviceProxy_->isAvailable() && i < 10 ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        i++;
    }
    ASSERT_TRUE(deviceProxy_->isAvailable());

    // do method call
    CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
    deviceProxy_->doSomething(call);
    ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
}

/**
 * @test
 *  - Test the getConnectionId, getDomain and getInteface methods
 *    available via the ProxyManager of the respective managed interfaces
 *    of the manager
 */
TEST_F(AFManaged, ProxyManagerTestPrimitiveMethods) {

    subscribe();
    // Commented out because fails under D-Bus binding.
    // ASSERT_EQ(testProxy_->getProxyManagerDevice().getConnectionId(), connectionIdClient);
    // ASSERT_EQ(testProxy_->getProxyManagerSpecialDevice().getConnectionId(), connectionIdClient);

    ASSERT_EQ(testProxy_->getProxyManagerDevice().getDomain(), domain);
    ASSERT_EQ(testProxy_->getProxyManagerSpecialDevice().getDomain(), domain);

    ASSERT_EQ(testProxy_->getProxyManagerDevice().getInterface(), interfaceDevice);
    ASSERT_EQ(testProxy_->getProxyManagerSpecialDevice().getInterface(), interfaceSpecialDevice);
}

/**
 * @test
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstances method to check that all
 *    registered instances are returned
 *  - Use the ProxyManager's checkInstanceAvailabilityStatus method to check that all
 *    returned instances by getAvailableInstances are available
 *  - Add a different managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstances method to check that all
 *    registered instances are returned
 *  - Use the ProxyManager's checkInstanceAvailabilityStatus method to check that all
 *    returned instances by getAvailableInstances are available
 *  - Add a second instance of the same managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstances method to check that all
 *    registered instances are returned
 *  - Use the ProxyManager's checkInstanceAvailabilityStatus method to check that all
 *    returned instances by getAvailableInstances are available
 *  - Remove all the managed interfaces from the manager
 *  - Check that the client is notified about the removed interfaces
 */
TEST_F(AFManaged, ProxyManagerTestNonPrimitiveMethodsSync) {
    subscribe();
    // Add
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1 && specialDevicesAvailable_.size()) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);
    ASSERT_EQ(specialDevicesAvailable_.size(), 1u);
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::vector<std::string> availableSpecialDevices;
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(callStatus,
            availableSpecialDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableSpecialDevices.size(), 1u);
    bool allDetectedAreAvailable = false;
    for(auto &i : availableSpecialDevices) {
        for(auto &j : specialDevicesAvailable_) {
            if(i == j.getInstance()) {
                allDetectedAreAvailable = true;
                ASSERT_TRUE(checkInstanceAvailabilityStatus(&testProxy_->getProxyManagerSpecialDevice(), j));
                break;
            }
        }
        ASSERT_TRUE(allDetectedAreAvailable);
    }

    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1 && devicesAvailable_.size() == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    ASSERT_EQ(devicesAvailable_.size(), 1u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::vector<std::string> availableDevices;
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 1u);
    allDetectedAreAvailable = false;
    for(auto &i : availableDevices) {
        for(auto &j : devicesAvailable_) {
            if(i == j.getInstance()) {
                allDetectedAreAvailable = true;
                ASSERT_TRUE(checkInstanceAvailabilityStatus(&testProxy_->getProxyManagerDevice(), j));
                break;
            }
        }
        ASSERT_TRUE(allDetectedAreAvailable);
    }

    testStub_->deviceDetected(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 2 && devicesAvailable_.size() == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 2);
    ASSERT_EQ(devicesAvailable_.size(), 2u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 2u);
    allDetectedAreAvailable = false;
    for(auto &i : availableDevices) {
        for(auto &j : devicesAvailable_) {
            if(i == j.getInstance()) {
                allDetectedAreAvailable = true;
                ASSERT_TRUE(checkInstanceAvailabilityStatus(&testProxy_->getProxyManagerDevice(), j));
                break;
            }
        }
        ASSERT_TRUE(allDetectedAreAvailable);
    }

    // Remove
    testStub_->specialDeviceRemoved(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 0 && specialDevicesAvailable_.size() == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(specialDeviceAvailableCount_, 0);
    ASSERT_EQ(specialDevicesAvailable_.size(), 0u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableSpecialDevices.clear();
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(callStatus,
            availableSpecialDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableSpecialDevices.size(), 0u);

    testStub_->deviceRemoved(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    ASSERT_LE(devicesAvailable_.size(), 1u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 1u);
    allDetectedAreAvailable = false;
    for(auto &i : availableDevices) {
        for(auto &j : devicesAvailable_) {
            if(i == j.getInstance()) {
                allDetectedAreAvailable = true;
                ASSERT_TRUE(checkInstanceAvailabilityStatus(&testProxy_->getProxyManagerDevice(), j));
                break;
            }
        }
        ASSERT_TRUE(allDetectedAreAvailable);
    }

    testStub_->deviceRemoved(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0 && devicesAvailable_.size() == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(deviceAvailableCount_, 0);
    ASSERT_EQ(devicesAvailable_.size(), 0u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 0u);
}

/**
 * @test
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstancesAsync method to check that all
 *    registered instances are returned
 *  - Add a different managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstancesAsync method to check that all
 *    registered instances are returned
 *  - Add a second instance of the same managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstancesAsync method to check that all
 *    registered instances are returned
 *  - Remove all the managed interfaces from the manager
 *  - Check that the client is notified about the removed interfaces
 */
TEST_F(AFManaged, ProxyManagerTestNonPrimitiveMethodsAsync) {
    subscribe();
    std::function<
            void(const CommonAPI::CallStatus &,
                 const std::vector<std::string> &)> getAvailableInstancesAsyncSpecialDeviceCallbackFunc =
            std::bind(
                    &AFManaged_ProxyManagerTestNonPrimitiveMethodsAsync_Test::getAvailableInstancesAsyncSpecialDeviceCallback,
                    this, std::placeholders::_1, std::placeholders::_2);

    std::function<
            void(const CommonAPI::CallStatus &,
                 const std::vector<std::string> &)> getAvailableInstancesAsyncDeviceCallbackFunc =
            std::bind(
                    &AFManaged_ProxyManagerTestNonPrimitiveMethodsAsync_Test::getAvailableInstancesAsyncDeviceCallback,
                    this, std::placeholders::_1, std::placeholders::_2);

    // Add
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1 && specialDevicesAvailable_.size() == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);
    ASSERT_EQ(specialDevicesAvailable_.size(), 1u);
    specialDeviceAvailableDesiredValue_ = 1;
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstancesAsync(getAvailableInstancesAsyncSpecialDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1 && devicesAvailable_.size() == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    ASSERT_EQ(devicesAvailable_.size(), 1u);
    deviceAvailableDesiredValue_ = 1;
    testProxy_->getProxyManagerDevice().getAvailableInstancesAsync(
            getAvailableInstancesAsyncDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    testStub_->deviceDetected(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 2 && devicesAvailable_.size() == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 2);
    ASSERT_EQ(devicesAvailable_.size(), 2u);
    deviceAvailableDesiredValue_ = 2;
    testProxy_->getProxyManagerDevice().getAvailableInstancesAsync(
            getAvailableInstancesAsyncDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    // Remove
    testStub_->specialDeviceRemoved(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 0 && specialDevicesAvailable_.size() == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(specialDeviceAvailableCount_, 0);
    ASSERT_EQ(specialDevicesAvailable_.size(), 0u);
    specialDeviceAvailableDesiredValue_ = 0;
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstancesAsync(getAvailableInstancesAsyncSpecialDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    testStub_->deviceRemoved(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    ASSERT_LE(devicesAvailable_.size(), 1u);
    deviceAvailableDesiredValue_ = 1;
    testProxy_->getProxyManagerDevice().getAvailableInstancesAsync(getAvailableInstancesAsyncDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    testStub_->deviceRemoved(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0 && devicesAvailable_.size() == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(deviceAvailableCount_, 0);
    ASSERT_LE(devicesAvailable_.size(), 0u);
    deviceAvailableDesiredValue_ = 0;
    testProxy_->getProxyManagerDevice().getAvailableInstancesAsync(getAvailableInstancesAsyncDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
}

/**
 * @test
 *  - Add a managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstances method to check that all
 *    registered instances are returned
 *  - Use the ProxyManager's checkInstanceAvailabilityStatusAsync method to check that all
 *    returned instances by getAvailableInstances are available
 *  - Add a different managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstances method to check that all
 *    registered instances are returned
 *  - Use the ProxyManager's checkInstanceAvailabilityStatusAsync method to check that all
 *    returned instances by getAvailableInstances are available
 *  - Add a second instance of the same managed interface to the manager
 *  - Check that the client is notified about the newly added interface
 *  - Use the ProxyManager's getAvailableInstances method to check that all
 *    registered instances are returned
 *  - Use the ProxyManager's checkInstanceAvailabilityStatusAsync method to check that all
 *    returned instances by getAvailableInstances are available
 *  - Remove all the managed interfaces from the manager
 *  - Check that the client is notified about the removed interfaces
 */
TEST_F(AFManaged, DISABLED_ProxyManagerTestGetInstanceAvailabilityStatusAsync) {
    subscribe();
    std::function<
            void(const CommonAPI::CallStatus &,
                 const CommonAPI::AvailabilityStatus &)> getInstanceAvailabilityStatusAsyncCallbackAvailableFunc =
            std::bind(
                    &AFManaged::getInstanceAvailabilityStatusAsyncCallbackAvailable,
                    this, std::placeholders::_1, std::placeholders::_2);

    std::function<
            void(const CommonAPI::CallStatus &,
                 const CommonAPI::AvailabilityStatus &)> getInstanceAvailabilityStatusAsyncCallbackNotAvailableFunc =
            std::bind(
                    &AFManaged::getInstanceAvailabilityStatusAsyncCallbackNotAvailable,
                    this, std::placeholders::_1, std::placeholders::_2);

    // Add
    testStub_->specialDeviceDetected(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 1 && specialDevicesAvailable_.size() == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(specialDeviceAvailableCount_, 1);
    ASSERT_EQ(specialDevicesAvailable_.size(), 1u);
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::vector<std::string> availableSpecialDevices;
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(callStatus,
            availableSpecialDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableSpecialDevices.size(), 1u);
    bool allDetectedAreAvailable = false;
    instanceAvailabilityStatusCallbackCalled_ = false;
    for(auto &i : availableSpecialDevices) {
        for(auto &j : specialDevicesAvailable_) {
            if(i == j.getInstance()) {
                allDetectedAreAvailable = true;
                testProxy_->getProxyManagerSpecialDevice().getInstanceAvailabilityStatusAsync(
                        j.getInstance(),
                        getInstanceAvailabilityStatusAsyncCallbackAvailableFunc);
                std::this_thread::sleep_for(
                        std::chrono::milliseconds(sleepMilli));
                ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
                instanceAvailabilityStatusCallbackCalled_ = false;
                break;
            }
        }
        ASSERT_TRUE(allDetectedAreAvailable);
    }

    testStub_->deviceDetected(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1 && devicesAvailable_.size() == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    ASSERT_EQ(devicesAvailable_.size(), 1u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::vector<std::string> availableDevices;
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 1u);
    allDetectedAreAvailable = false;
    for(auto &i : availableDevices) {
        for(auto &j : devicesAvailable_) {
            if(i == j.getInstance()) {
                allDetectedAreAvailable = true;
                testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusAsync(
                        j.getInstance(),
                        getInstanceAvailabilityStatusAsyncCallbackAvailableFunc);
                std::this_thread::sleep_for(
                        std::chrono::milliseconds(sleepMilli));
                ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
                instanceAvailabilityStatusCallbackCalled_ = false;
                break;
            }
        }
        ASSERT_TRUE(allDetectedAreAvailable);
    }

    testStub_->deviceDetected(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 2 && devicesAvailable_.size() == 2) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 2);
    ASSERT_EQ(devicesAvailable_.size(), 2u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 2u);
    allDetectedAreAvailable = false;
    for(auto &i : availableDevices) {
        for(auto &j : devicesAvailable_) {
            if(i == j.getInstance()) {
                allDetectedAreAvailable = true;
                testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusAsync(
                        j.getInstance(),
                        getInstanceAvailabilityStatusAsyncCallbackAvailableFunc);
                std::this_thread::sleep_for(
                        std::chrono::milliseconds(sleepMilli));
                ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
                instanceAvailabilityStatusCallbackCalled_ = false;
                break;
            }
        }
        ASSERT_TRUE(allDetectedAreAvailable);
    }

    // Remove
    testStub_->specialDeviceRemoved(0);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (specialDeviceAvailableCount_ == 0 && specialDevicesAvailable_.size() == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(specialDeviceAvailableCount_, 0);
    ASSERT_EQ(specialDevicesAvailable_.size(),0u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableSpecialDevices.clear();
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(callStatus,
            availableSpecialDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableSpecialDevices.size(), 0u);
    testProxy_->getProxyManagerSpecialDevice().getInstanceAvailabilityStatusAsync(
            "managedTest.Manager.specialDevice00",
            getInstanceAvailabilityStatusAsyncCallbackNotAvailableFunc);
    std::this_thread::sleep_for(
            std::chrono::milliseconds(sleepMilli));
    ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
    instanceAvailabilityStatusCallbackCalled_ = false;

    testStub_->deviceRemoved(1);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 1 && devicesAvailable_.size() == 1) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_EQ(deviceAvailableCount_, 1);
    ASSERT_LE(devicesAvailable_.size(), 1u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 1u);
    testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusAsync(
            "commonapi.advanced.managed.Manager.device01",
            getInstanceAvailabilityStatusAsyncCallbackNotAvailableFunc);
    std::this_thread::sleep_for(
            std::chrono::milliseconds(sleepMilli));
    ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
    instanceAvailabilityStatusCallbackCalled_ = false;
    allDetectedAreAvailable = false;
    for(auto &i : availableDevices) {
        for(auto &j : devicesAvailable_) {
            if(i == j.getInstance()) {
                allDetectedAreAvailable = true;
                testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusAsync(
                        j.getInstance(),
                        getInstanceAvailabilityStatusAsyncCallbackAvailableFunc);
                std::this_thread::sleep_for(
                        std::chrono::milliseconds(sleepMilli));
                ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
                instanceAvailabilityStatusCallbackCalled_ = false;
                break;
            }
        }
        ASSERT_TRUE(allDetectedAreAvailable);
    }


    testStub_->deviceRemoved(2);
    for (int i = 0; i < 100; i++) {
        {
            std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
            if (deviceAvailableCount_ == 0 && devicesAvailable_.size() == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    ASSERT_LE(deviceAvailableCount_, 0);
    ASSERT_EQ(devicesAvailable_.size(), 0u);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 0u);
    testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusAsync(
            "commonapi.advanced.managed.Manager.device02",
            getInstanceAvailabilityStatusAsyncCallbackNotAvailableFunc);
    std::this_thread::sleep_for(
            std::chrono::milliseconds(sleepMilli));
    ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
}

TEST_F(AFManaged, AddRemoveHierarchicalManagedInterface) {

  bool hLevelTopServiceRegistered = false;
  int hLevelMiddleAvailableCount = 0;
  int hLevelBottomAvailableCount = 0;
  CommonAPI::Event<>::Subscription hLevelTopSubscriptionId;
  CommonAPI::Event<>::Subscription hLevelMiddleSubscriptionId;
  std::shared_ptr<HLevelTopStubImpl> hLevelTopStub;
  static std::shared_ptr<HLevelTopProxy<>> hLevelTopProxy;
  std::shared_ptr<HLevelMiddleStubImpl> hLevelMiddleStub;
  static std::shared_ptr<HLevelMiddleProxy<>> hLevelMiddleProxy;
  static std::shared_ptr<HLevelBottomProxy<>> hLevelBottomProxy;
  std::mutex hLevelMiddleAvailableCountMutex;
  std::mutex hLevelBottomAvailableCountMutex;

  // create top level stub
  hLevelTopStub = std::make_shared<HLevelTopStubImpl>(hLevelTopStubInstanceName);
  hLevelTopServiceRegistered = runtime_->registerService(domain, hLevelTopStubInstanceName, hLevelTopStub, connectionIdService);
  ASSERT_TRUE(hLevelTopServiceRegistered);
  std::mutex h_availabilityMutex;
  std::unique_lock<std::mutex> h_lock(h_availabilityMutex);
  std::condition_variable h_cv;
  bool h_proxyAvailable = false;

  std::thread h_t1([this, &h_proxyAvailable, &h_cv, &h_availabilityMutex]() {
      std::lock_guard<std::mutex> h_lock(h_availabilityMutex);

      hLevelTopProxy = runtime_->buildProxy<HLevelTopProxy>(domain, hLevelTopStubInstanceName, connectionIdClient);
      ASSERT_TRUE((bool)hLevelTopProxy);
      hLevelTopProxy->isAvailableBlocking();
      h_proxyAvailable = true;
      h_cv.notify_one();
  });

  while(!h_proxyAvailable) {
      h_cv.wait(h_lock);
  }
  h_t1.join();
  ASSERT_TRUE(hLevelTopProxy->isAvailable());
  CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& hierarchicalDeviceEvent =
          hLevelTopProxy->getProxyManagerHLevelMiddle().getInstanceAvailabilityStatusChangedEvent();

  hLevelTopSubscriptionId = hierarchicalDeviceEvent.subscribe([&]
                                            ( const std::string _address,
                                              const CommonAPI::AvailabilityStatus _status) {
                                                ASSERT_TRUE(_address == hLevelMiddleAddress);
                                                std::lock_guard<std::mutex> lock(hLevelMiddleAvailableCountMutex);
                                                if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
                                                    // std::cout << "New hierarchical device available: " << _address << std::endl;
                                                    hLevelMiddleAvailableCount++;
                                                }

                                                if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
                                                    // std::cout << "Hierarchical device removed: " << _address << std::endl;
                                                    hLevelMiddleAvailableCount--;
                                                }
                                              });
  ASSERT_EQ(hLevelTopSubscriptionId,
      static_cast<CommonAPI::Event<>::Subscription>(0));

  // Add a managed stub
  hLevelMiddleStub = hLevelTopStub->deviceDetected(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));
  {
      std::lock_guard<std::mutex> lock(hLevelMiddleAvailableCountMutex);
      ASSERT_EQ(hLevelMiddleAvailableCount, 1);
  }

  // check that the new stub is registered and reachable
  std::mutex h2_availabilityMutex;
  std::unique_lock<std::mutex> h2_lock(h2_availabilityMutex);
  std::condition_variable h2_cv;
  bool h2_proxyAvailable = false;

  std::thread h2_t1([this, &h2_proxyAvailable, &h2_cv, &h2_availabilityMutex]() {
      std::lock_guard<std::mutex> h2_lock(h2_availabilityMutex);

      hLevelMiddleProxy = runtime_->buildProxy<HLevelMiddleProxy>(domain, hLevelMiddleStubInstanceName, connectionIdClient);
      ASSERT_TRUE((bool)hLevelMiddleProxy);
      hLevelMiddleProxy->isAvailableBlocking();
      h2_proxyAvailable = true;
      h2_cv.notify_one();
  });

  while(!h2_proxyAvailable) {
      h2_cv.wait(h2_lock);
  }

  h2_t1.join();
  ASSERT_TRUE(hLevelMiddleProxy->isAvailable());

  // subscribe to the availability event of level 1 stub
  CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& hierarchical2DeviceEvent =
          hLevelMiddleProxy->getProxyManagerHLevelBottom().getInstanceAvailabilityStatusChangedEvent();

  hLevelMiddleSubscriptionId = hierarchical2DeviceEvent.subscribe([&]
                                            ( const std::string _address,
                                              const CommonAPI::AvailabilityStatus _status) {
                                                ASSERT_TRUE(_address == hLevelBottomAddress);
                                                std::lock_guard<std::mutex> lock(hLevelMiddleAvailableCountMutex);
                                                if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
                                                    // std::cout << "New hierarchical device available: " << _address << std::endl;
                                                    hLevelBottomAvailableCount++;
                                                }

                                                if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
                                                    // std::cout << "Hierarchical device removed: " << _address << std::endl;
                                                    hLevelBottomAvailableCount--;
                                                }
                                              });
  ASSERT_EQ(hLevelMiddleSubscriptionId,
      static_cast<CommonAPI::Event<>::Subscription>(0));

  // add a 'device' to the level 1 stub
  hLevelMiddleStub->deviceDetected(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));
  {
      std::lock_guard<std::mutex> lock(hLevelBottomAvailableCountMutex);
      ASSERT_EQ(hLevelBottomAvailableCount, 1);
  }

  // check that the new stub is registered and reachable
  std::mutex h3_availabilityMutex;
  std::unique_lock<std::mutex> h3_lock(h3_availabilityMutex);
  std::condition_variable h3_cv;
  bool h3_proxyAvailable = false;

  std::thread h3_t1([this, &h3_proxyAvailable, &h3_cv, &h3_availabilityMutex]() {
      std::lock_guard<std::mutex> h3_lock(h3_availabilityMutex);

      hLevelBottomProxy = runtime_->buildProxy<HLevelBottomProxy>(domain, hLevelBottomStubInstanceName, connectionIdClient);
      ASSERT_TRUE((bool)hLevelBottomProxy);
      hLevelBottomProxy->isAvailableBlocking();
      h3_proxyAvailable = true;
      h3_cv.notify_one();
  });

  while(!h3_proxyAvailable) {
      h3_cv.wait(h3_lock);
  }

  h3_t1.join();
  ASSERT_TRUE(hLevelBottomProxy->isAvailable());

  // call getAvailableInstances to see if a correct number of instances is seen
  {
    std::vector<std::string> instances;
    CommonAPI::CallStatus status = CommonAPI::CallStatus::UNKNOWN;
    hLevelTopProxy->getProxyManagerHLevelMiddle().getAvailableInstances(status, instances);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, status);
    ASSERT_EQ(1u, instances.size());
    ASSERT_EQ(instances.front(),  hLevelMiddleStubInstanceName);
  }
  {
    std::vector<std::string> instances;
    CommonAPI::CallStatus status = CommonAPI::CallStatus::UNKNOWN;
    hLevelMiddleProxy->getProxyManagerHLevelBottom().getAvailableInstances(status, instances);
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, status);
    ASSERT_EQ(1u, instances.size());
    ASSERT_EQ(instances.front(),  hLevelBottomStubInstanceName);
  }

  // Remove
  hLevelMiddleStub->deviceRemoved(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));
  {
      std::lock_guard<std::mutex> lock(hLevelBottomAvailableCountMutex);
      ASSERT_LE(hLevelBottomAvailableCount, 0);
  }

  hLevelTopStub->deviceRemoved(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(1 * 1000));
  {
      std::lock_guard<std::mutex> lock(hLevelMiddleAvailableCountMutex);
      ASSERT_LE(hLevelMiddleAvailableCount, 0);
  }

  // Unregister callbacks
  hierarchical2DeviceEvent.unsubscribe(hLevelMiddleSubscriptionId);
  hierarchicalDeviceEvent.unsubscribe(hLevelTopSubscriptionId);
  runtime_->unregisterService(domain, HLevelTopStubImpl::StubInterface::getInterface(), hLevelTopStubInstanceName);

  // wait that proxy is not available
  int counter = 0;  // counter for avoiding endless loop
  while ( hLevelBottomProxy->isAvailable() && counter < 10 ) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter++;
  }
  ASSERT_FALSE(hLevelBottomProxy->isAvailable());

  // wait that proxy is not available
  counter = 0;  // counter for avoiding endless loop
  while ( hLevelMiddleProxy->isAvailable() && counter < 10 ) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter++;
  }
  ASSERT_FALSE(hLevelMiddleProxy->isAvailable());

  // wait that proxy is not available
  counter = 0;
  while ( hLevelTopProxy->isAvailable() && counter < 10 ) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter++;
  }

  ASSERT_FALSE(hLevelTopProxy->isAvailable());

}

TEST_F(AFManaged, GetAvailableInstancesWithoutSubscribe) {
    // Add
    testStub_->deviceDetected(1);
    testStub_->deviceDetected(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    std::vector<std::string> instances;
    CommonAPI::CallStatus status = CommonAPI::CallStatus::UNKNOWN;
    for (int i = 0; i < 200; ++i) {
        testProxy_->getProxyManagerDevice().getAvailableInstances(status, instances);
        if(instances.size() == 2) {
            break;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
        }
    }
    ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, status);
    ASSERT_EQ(2u, instances.size());

    status = CommonAPI::CallStatus::UNKNOWN;
    testStub_->specialDeviceDetected(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    instances.clear();
    for (int i = 0; i < 200; ++i) {
        testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(status, instances);
        if(instances.size() == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    }
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstancesAsync([&status]
            (const CommonAPI::CallStatus & _status, const std::vector<std::string> & _instances) {
        if(_status == CommonAPI::CallStatus::SUCCESS) {
            status = CommonAPI::CallStatus::SUCCESS;
        }
        ASSERT_EQ(CommonAPI::CallStatus::SUCCESS, _status);
        ASSERT_EQ(1u, _instances.size());
    });

    for (int i = 0; i < 200; ++i) {
        if(status != CommonAPI::CallStatus::SUCCESS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
            break;
        }
    }
    EXPECT_EQ(CommonAPI::CallStatus::SUCCESS, status);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new ::testing::Environment());
    return RUN_ALL_TESTS();
}
