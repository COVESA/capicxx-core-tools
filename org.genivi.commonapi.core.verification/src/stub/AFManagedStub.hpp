// Copyright (C) 2015-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef AFMANAGEDSTUB_HPP_
#define AFMANAGEDSTUB_HPP_

#include <map>

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/advanced/managed/ManagerStubDefault.hpp>

#include "DeviceStubImpl.hpp"
#include "SpecialDeviceStubImpl.hpp"

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

    void addDevice(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x, addDeviceReply_t _reply);
    void removeDevice(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x, removeDeviceReply_t _reply);

private:
    std::string managerInstanceName;

    typedef std::shared_ptr<DeviceStubImpl> DevicePtr;
    typedef std::shared_ptr<SpecialDeviceStubImpl> SpecialDevicePtr;

    std::map<std::string, DevicePtr> myDevices;
    std::map<std::string, SpecialDevicePtr> mySpecialDevices;

    std::string getDeviceName(unsigned int);
    std::string getSpecialDeviceName(unsigned int);
};

#endif /* AFMANAGEDSTUB_HPP_ */
