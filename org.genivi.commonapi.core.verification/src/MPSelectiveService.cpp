/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <thread>

#include <CommonAPI/CommonAPI.hpp>
#include "stub/MPSelectiveStubImpl.hpp"

using namespace std;

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E01S");
    CommonAPI::Runtime::setProperty("LibraryBase", "MPSelective");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.multiprocess.bselective.MPSelective";
    std::string connection = "service-sample";

    std::shared_ptr<MPSelectiveStubImpl> myService = std::make_shared<MPSelectiveStubImpl>();
    bool successfullyRegistered = runtime->registerService(domain, instance, myService, connection);

    while (!successfullyRegistered) {
        // std::cout << "Register Service failed, trying again in 10 milliseconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        successfullyRegistered = runtime->registerService(domain, instance, myService, connection);
    }

    // std::cout << "Successfully Registered Service!" << std::endl;

    // wait a second
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // send a broadcast
    // std::cout << "Broadcast sent!" << std::endl;
    myService->fireBTestSelectiveSelective(1);
    // loop until script kills you
    int counter = 0;
    while (counter++ < 20) {
        // std::cout << "Waiting to be killed" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
