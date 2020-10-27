// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef E05SPECIALDEVICESTUBIMPL_HPP_
#define E05SPECIALDEVICESTUBIMPL_HPP_

#include <v1/commonapi/examples/E05SpecialDeviceStubDefault.hpp>

using namespace v1_0::commonapi::examples;

class E05SpecialDeviceStubImpl: public E05SpecialDeviceStubDefault {
public:
    E05SpecialDeviceStubImpl();
    virtual ~E05SpecialDeviceStubImpl();

    void doSomethingSpecial(const std::shared_ptr<CommonAPI::ClientId> _client,
                            doSomethingSpecialReply_t _reply);
};

#endif // E05SPECIALDEVICESTUBIMPL_HPP_
