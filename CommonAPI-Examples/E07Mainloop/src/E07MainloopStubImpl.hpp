// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef E07MAINLOOPSTUBIMPL_HPP_
#define E07MAINLOOPSTUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E07MainloopStubDefault.hpp>

class E07MainloopStubImpl
    : public v1_0::commonapi::examples::E07MainloopStubDefault {

public:
    E07MainloopStubImpl();
    virtual ~E07MainloopStubImpl();

    virtual void sayHello(const std::shared_ptr<CommonAPI::ClientId> _client,
            std::string _name, sayHelloReply_t _return);
    virtual void incAttrX();

};

#endif // E07MAINLOOPSTUBIMPL_HPP_
