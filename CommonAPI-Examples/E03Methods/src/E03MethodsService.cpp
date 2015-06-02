/* Copyright (C) 2014, 2015 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>
#include "E03MethodsStubImpl.hpp"

int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
	std::string instance = "commonapi.examples.Methods";

    std::shared_ptr<E03MethodsStubImpl> myService = std::make_shared<E03MethodsStubImpl>();
    runtime->registerService(domain, instance, myService);

    while (true) {
        myService->incCounter(); // Change value of attribute, see stub implementation
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
