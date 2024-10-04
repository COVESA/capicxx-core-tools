// Copyright (C) 2023 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "E09CrcProtectionP01StubImpl.hpp"

using namespace v1::commonapi::examples;

E09CrcProtectionP01StubImpl::E09CrcProtectionP01StubImpl() {
    cnt = 0;
}

E09CrcProtectionP01StubImpl::~E09CrcProtectionP01StubImpl() {
}

void E09CrcProtectionP01StubImpl::incCounter() {
    cnt++;

    CommonTypes::aStruct aOK, aERROR;
    aOK.setValue1((uint8_t)cnt);
    aOK.setValue2((uint8_t)cnt+1);
    setAOKAttribute(aOK);

    aERROR.setValue1((uint8_t)cnt);
    aERROR.setValue2((uint8_t)cnt+2);
    setAERRORAttribute(aERROR);
    std::cout <<  "New counter value = " << cnt << "!" << std::endl;
}
