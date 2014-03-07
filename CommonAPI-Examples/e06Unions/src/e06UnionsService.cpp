/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <thread>
#include <iostream>

#include <CommonAPI/CommonAPI.h>
#include "E06UnionsStubImpl.h"

int main() {
    int n = 0;

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();
    std::shared_ptr<CommonAPI::Factory> factory = runtime->createFactory();
    std::shared_ptr<CommonAPI::ServicePublisher> servicePublisher = runtime->getServicePublisher();

    const std::string& serviceAddress = "local:commonapi.examples.Unions:commonapi.examples.Unions";
    std::shared_ptr<E06UnionsStubImpl> myService = std::make_shared<E06UnionsStubImpl>();
    servicePublisher->registerService(myService, serviceAddress, factory);

    while (true) {
        std::cout << "Set value " << n << " for union u." << std::endl;
        myService->setMyValue(n);
        n++;
        if (n == 4) {
            n = 0;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
