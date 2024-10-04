// Copyright (C) 2022 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "E08CrcProtectionP04StubImpl.hpp"

using namespace v1::commonapi::examples;

E08CrcProtectionP04StubImpl::E08CrcProtectionP04StubImpl() {
    cnt = 0;
}

E08CrcProtectionP04StubImpl::~E08CrcProtectionP04StubImpl() {
}

void E08CrcProtectionP04StubImpl::incCounter() {
    cnt++;

    CommonTypes::aStruct aOK, aERROR;
    aOK.setValue1((uint16_t)cnt);
    aOK.setValue2((uint16_t)cnt+1);
    setAOKAttribute(aOK);

    aERROR.setValue1((uint16_t)cnt);
    aERROR.setValue2((uint16_t)cnt+2);
    setAERRORAttribute(aERROR);
    std::cout <<  "New counter value = " << cnt << "!" << std::endl;
}
