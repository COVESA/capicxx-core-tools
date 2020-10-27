// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>
#include "E03MethodsStubImpl.hpp"

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E03S");
    CommonAPI::Runtime::setProperty("LogApplication", "E03S");
    CommonAPI::Runtime::setProperty("LibraryBase", "E03Methods");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Methods";

    std::shared_ptr<E03MethodsStubImpl> myService = std::make_shared<E03MethodsStubImpl>();
    while (!runtime->registerService(domain, instance, myService, "service-sample")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Successfully Registered Service!" << std::endl;

    while (true) {
        myService->incCounter(); // Change value of attribute, see stub implementation
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
