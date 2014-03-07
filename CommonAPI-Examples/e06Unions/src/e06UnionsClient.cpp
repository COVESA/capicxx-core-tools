/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include <CommonAPI/CommonAPI.h>
#include <commonapi/examples/E06UnionsProxy.h>
#include <commonapi/examples/CommonTypes.h>

using namespace commonapi::examples;

int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();

    std::shared_ptr<CommonAPI::Factory> factory = runtime->createFactory();
    const std::string& serviceAddress = "local:commonapi.examples.Unions:commonapi.examples.Unions";
    std::shared_ptr<E06UnionsProxyDefault> myProxy = factory->buildProxy<E06UnionsProxy> (serviceAddress);

    while (!myProxy->isAvailable()) {
        usleep(10);
    }

    myProxy->getUAttribute().getChangedEvent().subscribe([&](const CommonTypes::SettingsUnion& v) {

        std::cout << "Received change message." << std::endl;

        if ( v.isType<CommonTypes::MyTypedef>() ) {

            std::cout << "Received MyTypedef with value " << v.get<CommonTypes::MyTypedef>() << std::endl;
        } else if ( v.isType<CommonTypes::MyEnum>() ) {

            std::cout << "Received MyEnum with value " << (int) (v.get<CommonTypes::MyEnum>()) << std::endl;
        } else if ( v.isType<uint8_t>() ) {

            std::cout << "Received uint8_t with value " << (int) (v.get<uint8_t>()) << std::endl;
        } else if ( v.isType<std::string>() ) {

            std::cout << "Received string with value " << v.get<std::string>() << std::endl;
        }
    });

    while (true) {
        usleep(10);
    }

    return 0;
}
