// Copyright (C) 2015-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef HLEVELTOPSTUBIMPL_HPP_
#define HLEVELTOPSTUBIMPL_HPP_

#include <map>

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/advanced/managed/HLevelTopStubDefault.hpp>

#include "HLevelMiddleStubImpl.hpp"

using namespace v1::commonapi::advanced::managed;

class HLevelTopStubImpl: public HLevelTopStubDefault {

public:
    HLevelTopStubImpl();
    HLevelTopStubImpl(const std::string);
    virtual ~HLevelTopStubImpl();

    std::shared_ptr<HLevelMiddleStubImpl> deviceDetected(unsigned int);

    void deviceRemoved(unsigned int);
private:
    std::string managerInstanceName;

    typedef std::shared_ptr<HLevelMiddleStubImpl> DevicePtr;

    std::map<std::string, DevicePtr> myDevices;

    std::string getDeviceName(unsigned int);
};

#endif /* HLEVELTOPSTUBIMPL_HPP_ */
