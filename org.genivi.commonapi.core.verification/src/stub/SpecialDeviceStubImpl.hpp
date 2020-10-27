// Copyright (C) 2015-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef SPECIALDEVICESTUBIMPL_HPP_
#define SPECIALDEVICESTUBIMPL_HPP_

#include <v1/commonapi/advanced/managed/SpecialDeviceStubDefault.hpp>

using namespace v1_0::commonapi::advanced::managed;

class SpecialDeviceStubImpl: public SpecialDeviceStubDefault {
public:
    SpecialDeviceStubImpl();
    virtual ~SpecialDeviceStubImpl();

    void doSomethingSpecial(const std::shared_ptr<CommonAPI::ClientId> _client,
                            doSomethingReply_t _reply);
};

#endif /* SPECIALDEVICESTUBIMPL_HPP_ */
