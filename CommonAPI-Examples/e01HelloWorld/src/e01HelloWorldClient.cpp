/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include <CommonAPI/CommonAPI.h> //Defined in the Common API Runtime library
#include <commonapi/examples/E01HelloWorldProxy.h> //Part of the code we just generated

using namespace commonapi::examples;

int main() {
    std::shared_ptr < CommonAPI::Runtime > runtime = CommonAPI::Runtime::load();

    std::shared_ptr < CommonAPI::Factory > factory = runtime->createFactory();
    const std::string& serviceAddress = "local:commonapi.examples.HelloWorld:commonapi.examples.HelloWorld";
    std::shared_ptr<E01HelloWorldProxyDefault> myProxy = factory->buildProxy<E01HelloWorldProxy>(serviceAddress);

    while (!myProxy->isAvailable()) {
        usleep(10);
    }

    const std::string name = "World";
    CommonAPI::CallStatus callStatus;
    std::string returnMessage;

    myProxy->sayHello(name, callStatus, returnMessage);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call failed!\n";
        return -1;
    }

    std::cout << "Got message: '" << returnMessage << "'\n";

    return 0;
}
