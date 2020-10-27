// Copyright (C) 2015-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <sstream>
#include <iomanip>

#include "HLevelMiddleStubImpl.hpp"

using namespace v1::commonapi::advanced::managed;

HLevelMiddleStubImpl::HLevelMiddleStubImpl() {
}

HLevelMiddleStubImpl::HLevelMiddleStubImpl(const std::string instanceName) {
    managerInstanceName = instanceName;
}

HLevelMiddleStubImpl::~HLevelMiddleStubImpl() {
}

void HLevelMiddleStubImpl::deviceDetected(unsigned int n) {
    std::cout << "Bottom level device " << n << " detected!" << std::endl;

    std::string deviceInstanceName = getDeviceName(n);
    myDevices[deviceInstanceName] = DevicePtr(new HLevelBottomStubImpl);
    const bool deviceRegistered = this->registerManagedStubHLevelBottom(myDevices[deviceInstanceName], deviceInstanceName);

    if (!deviceRegistered) {
        std::cout << "Error: Unable to register device: " << deviceInstanceName << std::endl;
    }
}

void HLevelMiddleStubImpl::deviceRemoved(unsigned int n) {
    std::cout << "Bottom level device " << n << " removed!" << std::endl;

    std::string deviceInstanceName = getDeviceName(n);
    const bool deviceDeregistered = this->deregisterManagedStubHLevelBottom(deviceInstanceName);

    if (!deviceDeregistered) {
        std::cout << "Error: Unable to deregister device: " << deviceInstanceName << std::endl;
    } else {
        myDevices.erase(deviceInstanceName);
    }
}

std::string HLevelMiddleStubImpl::getDeviceName(unsigned int n) {
    std::stringstream ss;
    ss << managerInstanceName << ".bottom" << std::setw(2) << std::hex << std::setfill('0') << n;
    return ss.str();
}
