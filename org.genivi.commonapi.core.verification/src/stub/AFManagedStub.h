// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef AFMANAGEDSTUB_H_
#define AFMANAGEDSTUB_H_

#include <map>

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/advanced/managed/ManagerStubDefault.hpp>

#include "DeviceStubImpl.h"
#include "SpecialDeviceStubImpl.h"

using namespace v1_0::commonapi::advanced::managed;

class AFManagedStub: public ManagerStubDefault {

public:
    AFManagedStub();
    AFManagedStub(const std::string);
    virtual ~AFManagedStub();

    void deviceDetected(unsigned int);
    void specialDeviceDetected(unsigned int);

    void deviceRemoved(unsigned int);
    void specialDeviceRemoved(unsigned int);

    void addDevice(const std::shared_ptr<CommonAPI::ClientId>, uint8_t, addDeviceReply_t);
    void removeDevice(const std::shared_ptr<CommonAPI::ClientId>, uint8_t, removeDeviceReply_t);

private:
    std::string managerInstanceName;

    typedef std::shared_ptr<DeviceStubImpl> DevicePtr;
    typedef std::shared_ptr<SpecialDeviceStubImpl> SpecialDevicePtr;

    std::map<std::string, DevicePtr> myDevices;
    std::map<std::string, SpecialDevicePtr> mySpecialDevices;

    std::string getDeviceName(unsigned int);
    std::string getSpecialDeviceName(unsigned int);
};

#endif /* AFMANAGEDSTUB_H_ */
