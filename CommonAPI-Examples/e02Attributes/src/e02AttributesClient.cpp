/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include <CommonAPI/CommonAPI.h>
#include <commonapi/examples/E02AttributesProxy.h>

#include "AttributeCacheExtension.hpp"

using namespace commonapi::examples;

void recv_cb(const CommonAPI::CallStatus& callStatus, const int32_t& val) {
    std::cout << "Receive callback: " << val << std::endl;
}

void recv_cb_s(const CommonAPI::CallStatus& callStatus, const CommonTypes::a1Struct& valStruct) {
    std::cout << "Receive callback for structure: a1.s = " << valStruct.s << ", valStruct.a2.d = " << valStruct.a2.d
                    << std::endl;
}

int main() {
    std::shared_ptr < CommonAPI::Runtime > runtime = CommonAPI::Runtime::load();

    std::shared_ptr < CommonAPI::Factory > factory = runtime->createFactory();
    const std::string& serviceAddress = "local:commonapi.examples.Attributes:commonapi.examples.Attributes";
    //std::shared_ptr < E02AttributesProxyDefault > myProxy = factory->buildProxy < E02AttributesProxy > (serviceAddress);
    std::shared_ptr<CommonAPI::DefaultAttributeProxyFactoryHelper<E02AttributesProxy, AttributeCacheExtension>::class_t> myProxy =
    		factory->buildProxyWithDefaultAttributeExtension<E02AttributesProxy, AttributeCacheExtension>(serviceAddress);


    while (!myProxy->isAvailable()) {
        usleep(10);
    }

    CommonAPI::CallStatus callStatus;

    int32_t value = 0;

    // Get actual attribute value from service
    myProxy->getXAttribute().getValue(callStatus, value);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call A failed!\n";
        return -1;
    }
    std::cout << "Got attribute value: " << value << std::endl;

    // Subscribe for receiving values
    myProxy->getXAttribute().getChangedEvent().subscribe([&](const int32_t& val) {
        std::cout << "Received change message: " << val << std::endl;
    });

    value = 100;

    // Asynchronous call to set attribute of service
    std::function<void(const CommonAPI::CallStatus&, int32_t)> fcb = recv_cb;
    myProxy->getXAttribute().setValueAsync(value, fcb);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call failed!\n";
        return -1;
    }

    // Asynchronous call to set attribute of type structure in service
    CommonTypes::a1Struct valueStruct;

    valueStruct.s = "abc";
    valueStruct.a2.b = true;
    valueStruct.a2.d = 1234;

    std::function<void(const CommonAPI::CallStatus&, CommonTypes::a1Struct)> fcb_s = recv_cb_s;
    myProxy->getA1Attribute().setValueAsync(valueStruct, fcb_s);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote set of structure failed!\n";
        return -1;
    }

    while (true) {

    	int32_t valueCached = 0;
    	bool r;
    	r = myProxy->getXAttributeExtension().getCachedValue(valueCached);
    	std::cout << "Got cached attribute value[" << (int)r << "]: " << valueCached << std::endl;
		usleep(1000000);
    }
}
