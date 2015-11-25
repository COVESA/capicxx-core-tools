// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include "DeviceStubImpl.h"

DeviceStubImpl::DeviceStubImpl() {
}

DeviceStubImpl::~DeviceStubImpl() {
}

void DeviceStubImpl::doSomething(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        doSomethingReply_t _reply) {
    (void)_client;
    std::cout << "DeviceStubImpl::doSomething() called." << std::endl;
    _reply();
}
