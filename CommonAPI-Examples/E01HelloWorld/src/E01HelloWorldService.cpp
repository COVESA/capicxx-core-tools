/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <thread>

#include <CommonAPI/CommonAPI.hpp>
#include "E01HelloWorldStubImpl.hpp"

using namespace std;

int main() {
	CommonAPI::Runtime::setProperty("LogContext", "E01S");
	CommonAPI::Runtime::setProperty("LibraryBase", "E01HelloWorld");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
	std::string instance = "commonapi.examples.HelloWorld";
	std::string connection = "service-sample";

	std::shared_ptr<E01HelloWorldStubImpl> myService = std::make_shared<E01HelloWorldStubImpl>();
    runtime->registerService(domain, instance, myService, connection);

    while (true) {
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    return 0;
}
