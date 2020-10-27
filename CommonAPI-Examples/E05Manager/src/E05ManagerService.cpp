// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>

#include "E05ManagerStubImpl.hpp"

using namespace v1_0::commonapi::examples;

static unsigned int cnt = 0; // counter for simulating external events
const static unsigned int maxDeviceNumber = 3;
const static std::string managerInstanceName = "commonapi.examples.Manager";
const std::string connectionIdService = "service-sample";

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E05S");
    CommonAPI::Runtime::setProperty("LogApplication", "E05S");
    CommonAPI::Runtime::setProperty("LibraryBase", "E05Manager");
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    std::shared_ptr<E05ManagerStubImpl> myService = std::make_shared < E05ManagerStubImpl > (managerInstanceName);

    bool successfullyRegistered = runtime->registerService("local", managerInstanceName, myService, connectionIdService);

    while (!successfullyRegistered) {
        std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        successfullyRegistered = runtime->registerService("local", managerInstanceName, myService, connectionIdService);
    }

    std::cout << "Successfully Registered Service!" << std::endl;

    while (true) {
        // Simulate external events
        if (cnt == 0) {
            myService->specialDeviceDetected(cnt);
        } else if (cnt < maxDeviceNumber) {
            myService->deviceDetected(cnt);
        } else if (cnt == maxDeviceNumber) {
            myService->specialDeviceRemoved(cnt % maxDeviceNumber);
        } else {
            myService->deviceRemoved(cnt % maxDeviceNumber);
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        cnt = ++cnt < maxDeviceNumber << 1 ? cnt : 0;
    }
    return 0;
}
