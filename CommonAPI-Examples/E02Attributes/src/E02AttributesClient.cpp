/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#ifndef WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <CommonAPI/Extensions/AttributeCacheExtension.hpp>
#include <v1/commonapi/examples/E02AttributesProxy.hpp>

using namespace v1::commonapi::examples;

template<typename _AttributeType>
class AttrExt : public CommonAPI::Extensions::AttributeCacheExtension<_AttributeType>
{
public:
    AttrExt(_AttributeType& baseAttribute) :
        CommonAPI::Extensions::AttributeCacheExtension<_AttributeType>(baseAttribute) {
    };
};

void recv_cb(const CommonAPI::CallStatus& callStatus, const int32_t& val) {
    std::cout << "Receive callback: " << val << std::endl;
}

void recv_cb_s(const CommonAPI::CallStatus& callStatus, const CommonTypes::a1Struct& valStruct) {
    std::cout << "Receive callback for structure: a1.s = " << valStruct.getS()
              << ", valStruct.a2.b = " << (valStruct.getA2().getB() ? "TRUE" : "FALSE")
              << ", valStruct.a2.d = " << valStruct.getA2().getD()
              << std::endl;
}

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E02C");
    CommonAPI::Runtime::setProperty("LibraryBase", "E02Attributes");

    std::shared_ptr < CommonAPI::Runtime > runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Attributes"; 
    std::string connection = "client-sample";

    auto myProxy = runtime->buildProxyWithDefaultAttributeExtension<E02AttributesProxy, AttrExt>(domain, instance, connection);

    std::cout << "Waiting for service to become available." << std::endl;
    while (!myProxy->isAvailable()) {
        usleep(10);
    }

    CommonAPI::CallStatus callStatus;

    int32_t value = 0;

    CommonAPI::CallInfo info(1000);
    info.sender_ = 5678;

    // Get actual attribute value from service
    std::cout << "Getting attribute value: " << value << std::endl;
    myProxy->getXAttribute().getValue(callStatus, value, &info);
    if (callStatus != CommonAPI::CallStatus::SUCCESS) {
        std::cerr << "Remote call A failed!\n";
        return -1;
    }
    std::cout << "Got attribute value: " << value << std::endl;

    // Subscribe for receiving values
    myProxy->getXAttribute().getChangedEvent().subscribe([&](const int32_t& val) {
        std::cout << "Received change message: " << val << std::endl;
    });

    myProxy->getA1Attribute().getChangedEvent().subscribe([&](const CommonTypes::a1Struct& val) {
        std::cout << "Received change message for A1" << std::endl;
    });

    value = 100;

    // Asynchronous call to set attribute of service
    std::function<void(const CommonAPI::CallStatus&, int32_t)> fcb = recv_cb;
    myProxy->getXAttribute().setValueAsync(value, fcb, &info);

    // Asynchronous call to set attribute of type structure in service
    CommonTypes::a1Struct valueStruct;

    valueStruct.setS("abc");
    CommonTypes::a2Struct a2Struct = valueStruct.getA2();
    a2Struct.setA(123);
    a2Struct.setB(true);
    a2Struct.setD(1234);
    valueStruct.setA2(a2Struct);

    std::function<void(const CommonAPI::CallStatus&, CommonTypes::a1Struct)> fcb_s = recv_cb_s;
    myProxy->getA1Attribute().setValueAsync(valueStruct, fcb_s, &info);

    while (true) {
        int32_t errorValue = -1;
        int32_t valueCached = *myProxy->getXAttributeExtension().getCachedValue(errorValue);
        if (valueCached != errorValue) {
            std::cout << "Got cached attribute value[" << (int)valueCached << "]: " << valueCached << std::endl;
        } else {
            std::cout << "Got cached attribute error value[" << (int)valueCached << "]: " << valueCached << std::endl;
        }
        usleep(1000000);
    }
}
