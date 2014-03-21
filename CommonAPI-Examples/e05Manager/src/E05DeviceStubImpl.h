/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E05DEVICESTUBIMPL_H_
#define E05DEVICESTUBIMPL_H_

#include <commonapi/examples/E05DeviceStubDefault.h>

using namespace commonapi::examples;

class E05DeviceStubImpl: public E05DeviceStubDefault {
public:
    E05DeviceStubImpl();
    virtual ~E05DeviceStubImpl();
    void doSomething();
};

#endif /* E05DEVICESTUBIMPL_H_ */
