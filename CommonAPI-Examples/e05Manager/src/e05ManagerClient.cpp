/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include <CommonAPI/CommonAPI.h>
#include <commonapi/examples/E05ManagerProxy.h>

using namespace commonapi::examples;

void newDeviceAvailable(const std::string address, const CommonAPI::AvailabilityStatus status) {
    if (status == CommonAPI::AvailabilityStatus::AVAILABLE) {
        std::cout << "New device available: " << address << std::endl;
    }

    if (status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
        std::cout << "Device removed: " << address << std::endl;
    }
}

int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();

    std::shared_ptr<CommonAPI::Factory> factory = runtime->createFactory();

    const std::string& serviceAddress = "local:commonapi.examples.Manager:commonapi.examples.Manager";
    std::shared_ptr<E05ManagerProxyDefault> myProxy = factory->buildProxy<E05ManagerProxy>(serviceAddress);
    while (!myProxy->isAvailable()) {
        usleep(10);
    }

    std::cout << "Proxy available." << std::endl;

    CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& deviceEvent =
                    myProxy->getProxyManagerE05Device().getInstanceAvailabilityStatusChangedEvent();
    CommonAPI::ProxyManager::InstanceAvailabilityStatusChangedEvent& specialDeviceEvent =
                    myProxy->getProxyManagerE05SpecialDevice().getInstanceAvailabilityStatusChangedEvent();

    std::function<void(const std::string, const CommonAPI::AvailabilityStatus)> newDeviceAvailableFunc = newDeviceAvailable;

    deviceEvent.subscribe(newDeviceAvailableFunc);
    specialDeviceEvent.subscribe(newDeviceAvailableFunc);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}
