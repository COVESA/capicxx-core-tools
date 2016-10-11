// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef HLEVELMIDDLESTUBIMPL_H_
#define HLEVELMIDDLESTUBIMPL_H_

#include <map>

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/advanced/managed/HLevelMiddleStubDefault.hpp>

#include "HLevelBottomStubImpl.h"

using namespace v1::commonapi::advanced::managed;

class HLevelMiddleStubImpl: public HLevelMiddleStubDefault {

public:
    HLevelMiddleStubImpl();
    HLevelMiddleStubImpl(const std::string);
    virtual ~HLevelMiddleStubImpl();

    void deviceDetected(unsigned int);

    void deviceRemoved(unsigned int);
private:
    std::string managerInstanceName;

    typedef std::shared_ptr<HLevelBottomStubImpl> DevicePtr;

    std::map<std::string, DevicePtr> myDevices;

    std::string getDeviceName(unsigned int);
};

#endif /* HLEVELMIDDLESTUBIMPL_H_ */
