/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "E03MethodsStubImpl.h"

using namespace commonapi::examples;

E03MethodsStubImpl::E03MethodsStubImpl() {
    cnt = 0;
}

E03MethodsStubImpl::~E03MethodsStubImpl() {
}

void E03MethodsStubImpl::incCounter() {
    cnt++;
    fireMyStatusEvent((int32_t) cnt);
    std::cout << "New counter value = " << cnt << "!" << std::endl;
}
;

void E03MethodsStubImpl::foo(int32_t x1,
                             std::string x2,
                             E03Methods::fooError& methodError,
                             int32_t& y1,
                             std::string& y2) {

    std::cout << "foo called, setting new values." << std::endl;

    methodError = (E03Methods::fooError) E03Methods::stdErrorTypeEnum::MY_FAULT;
    y1 = 42;
    y2 = "xyz";
}
