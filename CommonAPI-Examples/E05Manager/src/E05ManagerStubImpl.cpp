// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "E05ManagerStubImpl.hpp"

#include <sstream>
#include <iomanip>


using namespace v1_0::commonapi::examples;

E05ManagerStubImpl::E05ManagerStubImpl() {
}

E05ManagerStubImpl::E05ManagerStubImpl(const std::string instanceName) {
    managerInstanceName = instanceName;
}

E05ManagerStubImpl::~E05ManagerStubImpl() {
}

void E05ManagerStubImpl::deviceDetected(unsigned int n) {
    std::cout << "Device " << n << " detected!" << std::endl;

    std::string deviceInstanceName = getDeviceName(n);
    myDevices[deviceInstanceName] = DevicePtr(new E05DeviceStubImpl);
    const bool deviceRegistered = this->registerManagedStubE05Device(myDevices[deviceInstanceName], deviceInstanceName);

    if (!deviceRegistered) {
        std::cout << "Error: Unable to register device: " << deviceInstanceName << std::endl;
    }
}

void E05ManagerStubImpl::specialDeviceDetected(unsigned int n) {
    std::cout << "Special device " << n << " detected!" << std::endl;

    std::string specialDeviceInstanceName = getSpecialDeviceName(n);
    mySpecialDevices[specialDeviceInstanceName] = SpecialDevicePtr(new E05SpecialDeviceStubImpl);
    const bool specialDeviceRegistered = this->registerManagedStubE05SpecialDevice(mySpecialDevices[specialDeviceInstanceName],
                                                                                   specialDeviceInstanceName);

    if (!specialDeviceRegistered) {
        std::cout << "Error: Unable to register special device: " << specialDeviceInstanceName << std::endl;
    }
}

void E05ManagerStubImpl::deviceRemoved(unsigned int n) {
    std::cout << "Device " << n << " removed!" << std::endl;

    std::string deviceInstanceName = getDeviceName(n);
    const bool deviceDeregistered = this->deregisterManagedStubE05Device(deviceInstanceName);

    if (!deviceDeregistered) {
        std::cout << "Error: Unable to deregister device: " << deviceInstanceName << std::endl;
    } else {
        myDevices.erase(deviceInstanceName);
    }
}

void E05ManagerStubImpl::specialDeviceRemoved(unsigned int n) {
    std::cout << "Special device " << n << " removed!" << std::endl;

    std::string specialDeviceInstanceName = getSpecialDeviceName(n);
    const bool specialDeviceDeregistered = this->deregisterManagedStubE05SpecialDevice(specialDeviceInstanceName);

    if (!specialDeviceDeregistered) {
        std::cout << "Error: Unable to deregister special device: " << specialDeviceInstanceName << std::endl;
    } else {
        mySpecialDevices.erase(specialDeviceInstanceName);
    }
}

std::string E05ManagerStubImpl::getDeviceName(unsigned int n) {
    std::stringstream ss;
    ss << managerInstanceName << ".device" << std::setw(2) << std::hex << std::setfill('0') << n;
    return ss.str();
}

std::string E05ManagerStubImpl::getSpecialDeviceName(unsigned int n) {
    std::stringstream ss;
    ss << managerInstanceName << ".specialDevice" << std::setw(2) << std::hex << std::setfill('0') << n;
    return ss.str();
}
