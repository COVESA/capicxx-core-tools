// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>

#include "E04PhoneBookStubImpl.hpp"

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E04S");
    CommonAPI::Runtime::setProperty("LogApplication", "E04S");
    CommonAPI::Runtime::setProperty("LibraryBase", "E04PhoneBook");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    const std::string &domain = "local";
    const std::string &instance = "commonapi.examples.PhoneBook";
    const std::string &connection = "service-sample";

    std::shared_ptr<E04PhoneBookStubImpl> myService = std::make_shared<E04PhoneBookStubImpl>();
    myService->setPhoneBookAttribute(myService->createTestPhoneBook());

    bool successfullyRegistered = runtime->registerService(domain, instance, myService, connection);

    while (!successfullyRegistered) {
        std::cout << "Register Service failed, trying again in 100 milliseconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        successfullyRegistered = runtime->registerService(domain, instance, myService);
    }

    std::cout << "Successfully Registered Service!" << std::endl;

    while (true) {
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
