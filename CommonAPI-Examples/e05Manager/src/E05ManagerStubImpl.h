/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E05MANAGERSTUBIMPL_H_
#define E05MANAGERSTUBIMPL_H_

#include <map>

#include <CommonAPI/CommonAPI.h>
#include <commonapi/examples/E05ManagerStubDefault.h>

#include "E05DeviceStubImpl.h"
#include "E05SpecialDeviceStubImpl.h"

using namespace commonapi::examples;

class E05ManagerStubImpl: public E05ManagerStubDefault {

public:
    E05ManagerStubImpl();
    E05ManagerStubImpl(const std::string);
    virtual ~E05ManagerStubImpl();

    void deviceDetected(unsigned int);
    void specialDeviceDetected(unsigned int);

    void deviceRemoved(unsigned int);
    void specialDeviceRemoved(unsigned int);
private:
    std::string managerInstanceName;

    typedef std::shared_ptr<E05DeviceStubImpl> DevicePtr;
    typedef std::shared_ptr<E05SpecialDeviceStubImpl> SpecialDevicePtr;

    std::map<std::string, DevicePtr> myDevices;
    std::map<std::string, SpecialDevicePtr> mySpecialDevices;

    std::string getDeviceName(unsigned int);
    std::string getSpecialDeviceName(unsigned int);
};

#endif /* E05MANAGERSTUBIMPL_H_ */
