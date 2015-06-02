/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>
#include "E02AttributesStubImpl.hpp"

int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
	std::string instance = "commonapi.examples.Attributes";

    std::shared_ptr<E02AttributesStubImpl> myService = std::make_shared<E02AttributesStubImpl>();
    runtime->registerService(domain, instance, myService);

    while (true) {
        myService->incCounter(); // Change value of attribute, see stub implementation
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    return 0;
}
