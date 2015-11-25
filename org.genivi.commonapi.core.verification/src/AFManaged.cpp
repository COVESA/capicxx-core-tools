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

#include <v1/commonapi/advanced/managed/ManagerProxy.hpp>
#include <v1/commonapi/advanced/managed/DeviceProxy.hpp>
#include <v1/commonapi/advanced/managed/SpecialDeviceProxy.hpp>

#include <gtest/gtest.h>

using namespace v1_0::commonapi::advanced::managed;

const std::string &domain = "local";
const static std::string managerInstanceName = "commonapi.advanced.managed.Manager";
const static std::string connectionIdService = "service-sample";
const static std::string connectionIdClient = "client-sample";

const static std::string interfaceDevice = "commonapi.advanced.managed.Device";
const static std::string addressDevice1 = "local:" + interfaceDevice + ":commonapi.advanced.managed.Manager.device01";
const static std::string addressDevice2 = "local:" + interfaceDevice + ":commonapi.advanced.managed.Manager.device02";
const static std::string interfaceSpecialDevice = "commonapi.advanced.managed.SpecialDevice";
const static std::string addressSpecialDevice1 = "local:" + interfaceSpecialDevice + ":commonapi.advanced.managed.Manager.specialDevice00";

const static int sleepMilli = 1000;

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
    instanceAvailabilityStatusCallbackCalled_(false) {}

protected:
    void SetUp() {
        // CommonAPI::Runtime::setProperty("LibraryBase", "AFManaged");
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);
        std::mutex availabilityMutex;
        std::unique_lock<std::mutex> lock(availabilityMutex);
        std::condition_variable cv;
        bool proxyAvailable = false;

        std::thread t1([this, &proxyAvailable, &cv, &availabilityMutex]() {
            std::lock_guard<std::mutex> lock(availabilityMutex);

            testProxy_ = runtime_->buildProxy<ManagerProxy>(domain, managerInstanceName, connectionIdClient);
            ASSERT_TRUE((bool)testProxy_);
            testProxy_->isAvailableBlocking();
            proxyAvailable = true;
            cv.notify_one();
        });

        testStub_ = std::make_shared<AFManagedStub>(managerInstanceName);
        serviceRegistered_ = runtime_->registerService(domain, managerInstanceName, testStub_, connectionIdService);
        ASSERT_TRUE(serviceRegistered_);

        while(!proxyAvailable) {
            cv.wait(lock);
        }
        t1.join();
        ASSERT_TRUE(testProxy_->isAvailable());

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

        // register callbacks
        subscriptionIdSpecialDevice_ = specialDeviceEvent.subscribe(
                newSpecialDeviceAvailableCallbackFunc_);
        ASSERT_EQ(subscriptionIdSpecialDevice_,
                static_cast<CommonAPI::Event<>::Subscription>(0));

        subscriptionIdDevice_ = deviceEvent.subscribe(
                newDeviceAvailableCallbackFunc_);
        ASSERT_EQ(subscriptionIdDevice_,
                static_cast<CommonAPI::Event<>::Subscription>(0));

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
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
    }

    void newDeviceAvailableCallback(const std::string _address,
                            const CommonAPI::AvailabilityStatus _status) {
        ASSERT_TRUE(_address == addressDevice1 || _address == addressDevice2);
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
            std::cout << "New device available: " << _address << std::endl;
            deviceAvailableCount_++;
            devicesAvailable_.insert(CommonAPI::Address(_address));
        }

        if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
            std::cout << "Device removed: " << _address << std::endl;
            deviceAvailableCount_--;
            devicesAvailable_.erase(CommonAPI::Address(_address));
        }
    }

    void newSpecialDeviceAvailableCallback(
            const std::string _address,
            const CommonAPI::AvailabilityStatus _status) {
        ASSERT_TRUE(_address == addressSpecialDevice1);
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        if(_status == CommonAPI::AvailabilityStatus::AVAILABLE) {
            std::cout << "New device available: " << _address << std::endl;
            specialDeviceAvailableCount_++;
            specialDevicesAvailable_.insert(CommonAPI::Address(_address));
        }

        if(_status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
            std::cout << "Device removed: " << _address << std::endl;
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
        ASSERT_EQ(_availableSpecialDevices.size(), specialDeviceAvailableDesiredValue_);
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
        ASSERT_EQ(_availableDevices.size(), deviceAvailableDesiredValue_);
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

    bool received_;
    bool serviceRegistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<ManagerProxy<>> testProxy_;
    std::shared_ptr<AFManagedStub> testStub_;

    std::function<void(const std::string, const CommonAPI::AvailabilityStatus)> newDeviceAvailableCallbackFunc_;
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
    // Add
    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }

    // Remove
    testStub_->deviceRemoved(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
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
    // Add
    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }

    testStub_->deviceDetected(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 2);
    }

    // Remove
    testStub_->deviceRemoved(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 1);
    }

    testStub_->deviceRemoved(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
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
    // Add
    testStub_->specialDeviceDetected(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        ASSERT_EQ(specialDeviceAvailableCount_, 1);
    }

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }

    // Remove
    testStub_->specialDeviceRemoved(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(specialDeviceAvailableCount_, 0);
    }

    testStub_->deviceRemoved(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 1);
    }
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
    // Add
    testStub_->specialDeviceDetected(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        ASSERT_EQ(specialDeviceAvailableCount_, 1);
    }

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }

    testStub_->deviceDetected(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 2);
    }

    // Remove
    testStub_->specialDeviceRemoved(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(specialDeviceAvailableCount_, 0);
    }

    testStub_->deviceRemoved(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 1);
    }

    testStub_->deviceRemoved(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
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
    // Add
    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
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
    // Add
    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
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
    // Add
    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }
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
    // Add
    testStub_->specialDeviceDetected(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        ASSERT_EQ(specialDeviceAvailableCount_, 1);
    }

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    testStub_->deviceDetected(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 2);
    }

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
            usleep(100);
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        dp->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    for(const auto &sdp : specialDeviceProxies) {
        while (!sdp->isAvailable()) {
            usleep(100);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(specialDeviceAvailableCount_, 0);
    }
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
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
    // Add
    testStub_->specialDeviceDetected(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        ASSERT_EQ(specialDeviceAvailableCount_, 1);
    }

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    testStub_->deviceDetected(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 2);
    }

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
            usleep(100);
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        dp->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    for(const auto &sdp : specialDeviceProxies) {
        while (!sdp->isAvailable()) {
            usleep(100);
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        sdp->doSomethingSpecial(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    // Remove all
    testStub_->getStubAdapter()->deactivateManagedInstances();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(specialDeviceAvailableCount_, 0);
    }
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
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
    // Add
    testStub_->specialDeviceDetected(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        ASSERT_EQ(specialDeviceAvailableCount_, 1);
    }

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    testStub_->deviceDetected(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 2);
    }

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
            usleep(100);
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        dp->doSomething(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }

    for(const auto &sdp : specialDeviceProxies) {
        while (!sdp->isAvailable()) {
            usleep(100);
        }
        CommonAPI::CallStatus call(CommonAPI::CallStatus::UNKNOWN);
        sdp->doSomethingSpecial(call);
        ASSERT_EQ(call, CommonAPI::CallStatus::SUCCESS);
    }
}

/**
 * @test
 *  - Test the getConnectionId, getDomain and getInteface methods
 *    available via the ProxyManager of the respective managed interfaces
 *    of the manager
 */
TEST_F(AFManaged, ProxyManagerTestPrimitiveMethods) {
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
    // Add
    testStub_->specialDeviceDetected(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        ASSERT_EQ(specialDeviceAvailableCount_, 1);
    }
    ASSERT_EQ(specialDevicesAvailable_.size(), 1);
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::vector<std::string> availableSpecialDevices;
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(callStatus,
            availableSpecialDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableSpecialDevices.size(), 1);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }
    ASSERT_EQ(devicesAvailable_.size(), 1);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::vector<std::string> availableDevices;
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 1);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 2);
    }
    ASSERT_EQ(devicesAvailable_.size(), 2);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 2);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(specialDeviceAvailableCount_, 0);
    }
    ASSERT_EQ(specialDevicesAvailable_.size(), 0);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableSpecialDevices.clear();
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(callStatus,
            availableSpecialDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableSpecialDevices.size(), 0);

    testStub_->deviceRemoved(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 1);
    }
    ASSERT_LE(devicesAvailable_.size(), 1);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 1);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
    ASSERT_EQ(devicesAvailable_.size(), 0);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 0);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        ASSERT_EQ(specialDeviceAvailableCount_, 1);
    }
    ASSERT_EQ(specialDevicesAvailable_.size(), 1);
    specialDeviceAvailableDesiredValue_ = 1;
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstancesAsync(getAvailableInstancesAsyncSpecialDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    testStub_->deviceDetected(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }
    ASSERT_EQ(devicesAvailable_.size(), 1);
    deviceAvailableDesiredValue_ = 1;
    testProxy_->getProxyManagerDevice().getAvailableInstancesAsync(
            getAvailableInstancesAsyncDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    testStub_->deviceDetected(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 2);
    }
    ASSERT_EQ(devicesAvailable_.size(), 2);
    deviceAvailableDesiredValue_ = 2;
    testProxy_->getProxyManagerDevice().getAvailableInstancesAsync(
            getAvailableInstancesAsyncDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    // Remove
    testStub_->specialDeviceRemoved(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(specialDeviceAvailableCount_, 0);
    }
    ASSERT_EQ(specialDevicesAvailable_.size(), 0);
    specialDeviceAvailableDesiredValue_ = 0;
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstancesAsync(getAvailableInstancesAsyncSpecialDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    testStub_->deviceRemoved(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 1);
    }
    ASSERT_LE(devicesAvailable_.size(), 1);
    deviceAvailableDesiredValue_ = 1;
    testProxy_->getProxyManagerDevice().getAvailableInstancesAsync(getAvailableInstancesAsyncDeviceCallbackFunc);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));

    testStub_->deviceRemoved(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 1);
    }
    ASSERT_LE(devicesAvailable_.size(), 0);
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
    std::function<
            void(const CommonAPI::CallStatus &,
                 const CommonAPI::AvailabilityStatus &)> getInstanceAvailabilityStatusAsyncCallbackAvailableFunc =
            std::bind(
                    &AFManaged_DISABLED_ProxyManagerTestGetInstanceAvailabilityStatusAsync_Test::getInstanceAvailabilityStatusAsyncCallbackAvailable,
                    this, std::placeholders::_1, std::placeholders::_2);

    std::function<
            void(const CommonAPI::CallStatus &,
                 const CommonAPI::AvailabilityStatus &)> getInstanceAvailabilityStatusAsyncCallbackNotAvailableFunc =
            std::bind(
                    &AFManaged_DISABLED_ProxyManagerTestGetInstanceAvailabilityStatusAsync_Test::getInstanceAvailabilityStatusAsyncCallbackNotAvailable,
                    this, std::placeholders::_1, std::placeholders::_2);

    // Add
    testStub_->specialDeviceDetected(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(specialDeviceAvailableCountMutex_);
        ASSERT_EQ(specialDeviceAvailableCount_, 1);
    }
    ASSERT_EQ(specialDevicesAvailable_.size(), 1);
    CommonAPI::CallStatus callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::vector<std::string> availableSpecialDevices;
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(callStatus,
            availableSpecialDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableSpecialDevices.size(), 1);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 1);
    }
    ASSERT_EQ(devicesAvailable_.size(), 1);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    std::vector<std::string> availableDevices;
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 1);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_EQ(deviceAvailableCount_, 2);
    }
    ASSERT_EQ(devicesAvailable_.size(), 2);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 2);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(specialDeviceAvailableCount_, 0);
    }
    ASSERT_EQ(specialDevicesAvailable_.size(), 0);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableSpecialDevices.clear();
    testProxy_->getProxyManagerSpecialDevice().getAvailableInstances(callStatus,
            availableSpecialDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableSpecialDevices.size(), 0);
    testProxy_->getProxyManagerSpecialDevice().getInstanceAvailabilityStatusAsync(
            "local:managed.SpecialDevice:managed-test.Manager.specialDevice00",
            getInstanceAvailabilityStatusAsyncCallbackNotAvailableFunc);
    std::this_thread::sleep_for(
            std::chrono::milliseconds(sleepMilli));
    ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
    instanceAvailabilityStatusCallbackCalled_ = false;

    testStub_->deviceRemoved(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 1);
    }
    ASSERT_LE(devicesAvailable_.size(), 1);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 1);
    testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusAsync(
            "local:managed.Device:commonapi.advanced.managed.Manager.device01",
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
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilli));
    {
        std::lock_guard<std::mutex> lock(deviceAvailableCountMutex_);
        ASSERT_LE(deviceAvailableCount_, 0);
    }
    ASSERT_EQ(devicesAvailable_.size(), 0);
    callStatus = CommonAPI::CallStatus::UNKNOWN;
    availableDevices.clear();
    testProxy_->getProxyManagerDevice().getAvailableInstances(callStatus,
            availableDevices);
    ASSERT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(availableDevices.size(), 0);
    testProxy_->getProxyManagerDevice().getInstanceAvailabilityStatusAsync(
            "local:managed.Device:commonapi.advanced.managed.Manager.device02",
            getInstanceAvailabilityStatusAsyncCallbackNotAvailableFunc);
    std::this_thread::sleep_for(
            std::chrono::milliseconds(sleepMilli));
    ASSERT_TRUE(instanceAvailabilityStatusCallbackCalled_);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new ::testing::Environment());
    return RUN_ALL_TESTS();
}
