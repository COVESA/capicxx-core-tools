/* Copyright (C) 2014, 2015 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>
#include "E04PhoneBookStubImpl.h"

int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    const std::string &domain = "local";
    const std::string &instance = "commonapi.examples.PhoneBook";
    std::shared_ptr<E04PhoneBookStubImpl> myService = std::make_shared<E04PhoneBookStubImpl>();
    myService->setPhoneBookAttribute(myService->createTestPhoneBook());

    runtime->registerService(domain, instance, myService);
    while (true) {
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
