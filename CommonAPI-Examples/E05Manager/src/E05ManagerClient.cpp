// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef _WIN32
#include <unistd.h>
#endif

#include <iostream>
#include <thread>

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E05ManagerProxy.hpp>

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
    CommonAPI::Runtime::setProperty("LogApplication", "E05C");
    CommonAPI::Runtime::setProperty("LibraryBase", "E05Manager");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    const std::string &domain = "local";
    const std::string &instance = "commonapi.examples.Manager";
    const std::string connectionIdClient = "client-sample";

    std::shared_ptr<E05ManagerProxy<>> myProxy = runtime->buildProxy<E05ManagerProxy>(domain, instance, connectionIdClient);
    while (!myProxy->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
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
