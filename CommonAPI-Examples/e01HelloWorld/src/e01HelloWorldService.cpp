/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <thread>

#include <CommonAPI/CommonAPI.h>
#include "E01HelloWorldStubImpl.h"

using namespace std;

int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    std::shared_ptr<CommonAPI::Factory> factory = runtime->createFactory();
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime->getServicePublisher();

    const std::string& serviceAddress = "local:commonapi.examples.HelloWorld:commonapi.examples.HelloWorld";
    std::shared_ptr<E01HelloWorldStubImpl> myService = std::make_shared<E01HelloWorldStubImpl>();
    servicePublisher->registerService(myService, serviceAddress, factory);

    while (true) {
        std::cout << "Waiting for calls... (Abort with CTRL+C)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    return 0;
}
