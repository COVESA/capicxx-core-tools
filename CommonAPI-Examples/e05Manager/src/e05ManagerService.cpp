/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.h>
#include "E05ManagerStubImpl.h"

using namespace commonapi::examples;

static unsigned int cnt = 0; // counter for simulating external events
const static unsigned int maxDeviceNumber = 3;
const static std::string managerInstanceName = "commonapi.examples.Manager";

int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    std::shared_ptr<CommonAPI::Factory> factory = runtime->createFactory();
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime->getServicePublisher();

    const std::string& serviceAddress = "local:commonapi.examples.Manager:" + managerInstanceName;
    std::shared_ptr<E05ManagerStubImpl> myService = std::make_shared < E05ManagerStubImpl > (managerInstanceName);

    const bool serviceRegistered = servicePublisher->registerService(myService, serviceAddress, factory);

    if (!serviceRegistered) {
        std::cout << "Error: Unable to register service." << std::endl;
    }

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
