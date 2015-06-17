/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WIN32
#include <unistd.h>
#endif

#include <iostream>

#include <CommonAPI/CommonAPI.hpp>
#include <v1_0/commonapi/examples/E05ManagerProxy.hpp>

using namespace v1_0::commonapi::examples;

void newDeviceAvailable(const std::string address, const CommonAPI::AvailabilityStatus status) {
    if (status == CommonAPI::AvailabilityStatus::AVAILABLE) {
        std::cout << "New device available: " << address << std::endl;
    }

    if (status == CommonAPI::AvailabilityStatus::NOT_AVAILABLE) {
        std::cout << "Device removed: " << address << std::endl;
    }
}

int main() {
	CommonAPI::Runtime::setProperty("LogContext", "E05C");
	CommonAPI::Runtime::setProperty("LibraryBase", "E05Manager");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    const std::string &domain = "local";
    const std::string &instance = "commonapi.examples.Manager";
    std::shared_ptr<E05ManagerProxy<>> myProxy = runtime->buildProxy<E05ManagerProxy>(domain, instance);
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
