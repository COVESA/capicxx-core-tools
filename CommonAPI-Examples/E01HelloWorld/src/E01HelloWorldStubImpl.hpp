/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E01HELLOWORLDSTUBIMPL_H_
#define E01HELLOWORLDSTUBIMPL_H_

#include <CommonAPI/CommonAPI.hpp>
#include <v0_1/commonapi/examples/E01HelloWorldStubDefault.hpp>

class E01HelloWorldStubImpl: public v0_1::commonapi::examples::E01HelloWorldStubDefault {

public:
    E01HelloWorldStubImpl();
    virtual ~E01HelloWorldStubImpl();

    virtual void sayHello(const std::shared_ptr<CommonAPI::ClientId> _client, std::string _name, sayHelloReply_t _return);

};

#endif /* E01HELLOWORLDSTUBIMPL_H_ */
