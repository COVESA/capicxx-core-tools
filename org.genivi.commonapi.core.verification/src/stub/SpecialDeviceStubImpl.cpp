// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include "SpecialDeviceStubImpl.h"

SpecialDeviceStubImpl::SpecialDeviceStubImpl() {
}

SpecialDeviceStubImpl::~SpecialDeviceStubImpl() {
}

void SpecialDeviceStubImpl::doSomethingSpecial(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        doSomethingSpecialReply_t _reply) {
    (void)_client;
    std::cout << "SpecialDeviceStubImpl::doSomethingSpecial() called."
            << std::endl;
    _reply();
}
