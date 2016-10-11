/* Copyright (C) 2014, 2015 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#ifndef WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E03MethodsProxy.hpp>

using namespace v1_2::commonapi::examples;

void recv_cb(const CommonAPI::CallStatus& callStatus,
             const E03Methods::stdErrorTypeEnum& methodError,
             const int32_t& y1,
             const std::string& y2) {
    std::cout << "Result of asynchronous call of foo: " << std::endl;
    std::cout << "   callStatus: " << ((callStatus == CommonAPI::CallStatus::SUCCESS) ? "SUCCESS" : "NO_SUCCESS")
                    << std::endl;
    std::cout << "   error: "
                    << ((methodError == E03Methods::stdErrorTypeEnum::NO_FAULT) ? "NO_FAULT" :
                                    "MY_FAULT") << std::endl;
    std::cout << "   Output values: y1 = " << y1 << ", y2 = " << y2 << std::endl;
}

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E03C");
    CommonAPI::Runtime::setProperty("LogApplication", "E03C");
    CommonAPI::Runtime::setProperty("LibraryBase", "E03Methods");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.examples.Methods";

    std::shared_ptr<E03MethodsProxy<>> myProxy = runtime->buildProxy < E03MethodsProxy > (domain, instance, "client-sample");

    while (!myProxy->isAvailable()) {
        usleep(10);
    }

    // Subscribe to broadcast
    myProxy->getMyStatusEvent().subscribe([&](const int32_t& val) {
        std::cout << "Received status event: " << val << std::endl;
    });

    while (true) {
        int32_t inX1 = 5;
        std::string inX2 = "abc";
        CommonAPI::CallStatus callStatus;
        E03Methods::stdErrorTypeEnum methodError;
        int32_t outY1;
        std::string outY2;

        // Synchronous call
        std::cout << "Call foo with synchronous semantics ..." << std::endl;
        myProxy->foo(inX1, inX2, callStatus, methodError, outY1, outY2);

        std::cout << "Result of synchronous call of foo: " << std::endl;
        std::cout << "   callStatus: " << ((callStatus == CommonAPI::CallStatus::SUCCESS) ? "SUCCESS" : "NO_SUCCESS")
                  << std::endl;
        std::cout << "   error: "
                  << ((methodError == E03Methods::stdErrorTypeEnum::NO_FAULT) ? "NO_FAULT" : "MY_FAULT")
                  << std::endl;
        std::cout << "   Input values: x1 = " << inX1 << ", x2 = " << inX2 << std::endl;
        std::cout << "   Output values: y1 = " << outY1 << ", y2 = " << outY2 << std::endl;

        // Asynchronous call
        std::cout << "Call foo with asynchronous semantics ..." << std::endl;

        std::function<
                        void(const CommonAPI::CallStatus&,
                             const E03Methods::stdErrorTypeEnum&,
                             const int32_t&,
                             const std::string&)> fcb = recv_cb;
        myProxy->fooAsync(inX1, inX2, recv_cb);

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}
