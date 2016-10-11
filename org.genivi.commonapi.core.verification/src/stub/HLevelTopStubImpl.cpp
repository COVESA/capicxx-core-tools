// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <sstream>
#include <iomanip>

#include "HLevelTopStubImpl.h"

using namespace v1::commonapi::advanced::managed;

HLevelTopStubImpl::HLevelTopStubImpl() {
}

HLevelTopStubImpl::HLevelTopStubImpl(const std::string instanceName) {
    managerInstanceName = instanceName;
}

HLevelTopStubImpl::~HLevelTopStubImpl() {
}

std::shared_ptr<HLevelMiddleStubImpl> HLevelTopStubImpl::deviceDetected(unsigned int n) {
    std::cout << "Middle level device " << n << " detected!" << std::endl;

    std::string deviceInstanceName = getDeviceName(n);
    myDevices[deviceInstanceName] = DevicePtr(new HLevelMiddleStubImpl(deviceInstanceName));
    const bool deviceRegistered = this->registerManagedStubHLevelMiddle(myDevices[deviceInstanceName], deviceInstanceName);

    if (!deviceRegistered) {
        std::cout << "Error: Unable to register device: " << deviceInstanceName << std::endl;
    }
    return myDevices[deviceInstanceName];
}

void HLevelTopStubImpl::deviceRemoved(unsigned int n) {
    std::cout << "Middle level device " << n << " removed!" << std::endl;

    std::string deviceInstanceName = getDeviceName(n);
    const bool deviceDeregistered = this->deregisterManagedStubHLevelMiddle(deviceInstanceName);

    if (!deviceDeregistered) {
        std::cout << "Error: Unable to deregister device: " << deviceInstanceName << std::endl;
    } else {
        myDevices.erase(deviceInstanceName);
    }
}

std::string HLevelTopStubImpl::getDeviceName(unsigned int n) {
    std::stringstream ss;
    ss << managerInstanceName << ".middle" << std::setw(2) << std::hex << std::setfill('0') << n;
    return ss.str();
}
