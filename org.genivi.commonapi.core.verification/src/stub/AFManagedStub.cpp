// Copyright (C) 2015-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <sstream>
#include <iomanip>

#include "AFManagedStub.hpp"

using namespace v1_0::commonapi::advanced::managed;

AFManagedStub::AFManagedStub() {
}

AFManagedStub::AFManagedStub(const std::string instanceName) {
    managerInstanceName = instanceName;
}

AFManagedStub::~AFManagedStub() {
}

void AFManagedStub::addDevice(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x, addDeviceReply_t _reply) {
    (void)_client;
    deviceDetected(_x);
    _reply();
}

void AFManagedStub::removeDevice(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x, removeDeviceReply_t _reply) {
    (void)_client;
    deviceRemoved(_x);
    _reply();
}

void AFManagedStub::deviceDetected(unsigned int n) {
    //std::cout << "Device " << n << " detected!" << std::endl;

    std::string deviceInstanceName = getDeviceName(n);
    myDevices[deviceInstanceName] = DevicePtr(new DeviceStubImpl);
    const bool deviceRegistered = this->registerManagedStubDevice(myDevices[deviceInstanceName], deviceInstanceName);

    if (!deviceRegistered) {
        std::cout << "Error: Unable to register device: " << deviceInstanceName << std::endl;
    }
}

void AFManagedStub::specialDeviceDetected(unsigned int n) {
    //std::cout << "Special device " << n << " detected!" << std::endl;

    std::string specialDeviceInstanceName = getSpecialDeviceName(n);
    mySpecialDevices[specialDeviceInstanceName] = SpecialDevicePtr(new SpecialDeviceStubImpl);
    const bool specialDeviceRegistered = this->registerManagedStubSpecialDevice(mySpecialDevices[specialDeviceInstanceName],
                                                                                   specialDeviceInstanceName);

    if (!specialDeviceRegistered) {
        std::cout << "Error: Unable to register special device: " << specialDeviceInstanceName << std::endl;
    }
}

void AFManagedStub::deviceRemoved(unsigned int n) {
    //std::cout << "Device " << n << " removed!" << std::endl;

    std::string deviceInstanceName = getDeviceName(n);
    const bool deviceDeregistered = this->deregisterManagedStubDevice(deviceInstanceName);

    if (!deviceDeregistered) {
        std::cout << "Error: Unable to deregister device: " << deviceInstanceName << std::endl;
    } else {
        myDevices.erase(deviceInstanceName);
    }
}

void AFManagedStub::specialDeviceRemoved(unsigned int n) {
    //std::cout << "Special device " << n << " removed!" << std::endl;

    std::string specialDeviceInstanceName = getSpecialDeviceName(n);
    const bool specialDeviceDeregistered = this->deregisterManagedStubSpecialDevice(specialDeviceInstanceName);

    if (!specialDeviceDeregistered) {
        std::cout << "Error: Unable to deregister special device: " << specialDeviceInstanceName << std::endl;
    } else {
        mySpecialDevices.erase(specialDeviceInstanceName);
    }
}

std::string AFManagedStub::getDeviceName(unsigned int n) {
    std::stringstream ss;
    ss << managerInstanceName << ".device" << std::setw(2) << std::hex << std::setfill('0') << n;
    return ss.str();
}

std::string AFManagedStub::getSpecialDeviceName(unsigned int n) {
    std::stringstream ss;
    ss << managerInstanceName << ".specialDevice" << std::setw(2) << std::hex << std::setfill('0') << n;
    return ss.str();
}
