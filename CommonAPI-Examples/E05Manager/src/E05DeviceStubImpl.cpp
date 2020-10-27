// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "E05DeviceStubImpl.hpp"

#include <iostream>


E05DeviceStubImpl::E05DeviceStubImpl() {
}

E05DeviceStubImpl::~E05DeviceStubImpl() {
}

void E05DeviceStubImpl::doSomething(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        doSomethingReply_t _reply) {
    std::cout << "E05DeviceStubImpl::doSomething() called." << std::endl;
    _reply();
}
