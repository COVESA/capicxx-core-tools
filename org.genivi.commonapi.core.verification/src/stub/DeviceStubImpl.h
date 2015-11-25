// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef DEVICESTUBIMPL_H_
#define DEVICESTUBIMPL_H_

#include <v1/commonapi/advanced/managed/DeviceStubDefault.hpp>

using namespace v1_0::commonapi::advanced::managed;

class DeviceStubImpl: public DeviceStubDefault {
public:
    DeviceStubImpl();
    virtual ~DeviceStubImpl();
    void doSomething(const std::shared_ptr<CommonAPI::ClientId> _client,
                     doSomethingReply_t _reply);
};

#endif /* DEVICESTUBIMPL_H_ */
