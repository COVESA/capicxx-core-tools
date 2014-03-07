/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include "E05DeviceStubImpl.h"

using namespace commonapi::examples;

E05DeviceStubImpl::E05DeviceStubImpl() {
}

E05DeviceStubImpl::~E05DeviceStubImpl() {
}

void E05DeviceStubImpl::doSomething() {
    std::cout << "E05DeviceStubImpl::doSomething() called." << std::endl;
}
