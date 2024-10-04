// Copyright (C) 2022 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <CommonAPI/AttributeCacheExtension.hpp>
#include <v1/commonapi/examples/E08CrcProtectionP04Proxy.hpp>

using namespace v1::commonapi::examples;

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E08C");
    CommonAPI::Runtime::setProperty("LogApplication", "E08C");
    CommonAPI::Runtime::setProperty("LibraryBase", "E08CrcProtectionP04");

    std::shared_ptr < CommonAPI::Runtime > runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Attributes"; 
    std::string connection = "client-sample";

    auto myProxy = runtime->buildProxyWithDefaultAttributeExtension<E08CrcProtectionP04Proxy, CommonAPI::Extensions::AttributeCacheExtension>(domain, instance, connection);

    std::cout << "Waiting for service to become available." << std::endl;
    while (!myProxy->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    CommonTypes::aStruct aOK, aERROR;
    CommonAPI::CallInfo info(1000);
    info.sender_ = 1;

    // Subscribe aOK for receiving values
    myProxy->getAOKAttribute().getChangedEvent().subscribe(
        [&](const CommonTypes::aStruct& val) {
            std::cout << "Received change message on aOK:" << std::endl
                      << "\tCommonCRCLength: " << val.getCommonCRCLength() << std::endl
                      << "\tCommonCRCCounter: " << val.getCommonCRCCounter() << std::endl
                      << "\tCommonCRCID: " << val.getCommonCRCID() << std::endl
                      << "\tCommonCRC: " << val.getCommonCRC() << std::endl
	                  << "\tvalue1: " << val.getValue1() << std::endl
  	                  << "\tvalue2: " << val.getValue2() << std::endl;
        },
        [&](const CommonAPI::CallStatus &status) {
             if (status == CommonAPI::CallStatus::INVALID_VALUE) {
                std::cout << "Subscription (Changed Event) of aOK attribute returned CallStatus==INVALID_VALUE" << std::endl;
                // do something... (no eventhandler gets called in case of INVALID_VALUE status) 
            } else  if (status == CommonAPI::CallStatus::SUCCESS) {
                std::cout << "Got valid response for aOK Async getter" << std::endl;
            }
        }
    );

    // Subscribe aERROR for receiving values
    myProxy->getAERRORAttribute().getChangedEvent().subscribe(
        [&](const CommonTypes::aStruct& val) {
            std::cout << "Received change message on aERROR:" << std::endl
                      << "\tCommonCRCLength: " << val.getCommonCRCLength() << std::endl
	                  << "\tCommonCRCCounter: " << val.getCommonCRCCounter() << std::endl
                      << "\tCommonCRCID: " << val.getCommonCRCID() << std::endl
                      << "\tCommonCRC: " << val.getCommonCRC() << std::endl
                      << "\tvalue1: " << val.getValue1() << std::endl
                      << "\tvalue2: " << val.getValue2() << std::endl;
        },
        [&](const CommonAPI::CallStatus &status) {
             if (status == CommonAPI::CallStatus::INVALID_VALUE) {
                std::cout << "Subscription (Changed Event) of aERROR attribute returned CallStatus==INVALID_VALUE" << std::endl;
                // do something... (no eventhandler gets called in case of INVALID_VALUE status) 
            } else  if (status == CommonAPI::CallStatus::SUCCESS) {
                std::cout << "Got valid response for aERROR Async getter" << std::endl;
            }
        }
    );

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
