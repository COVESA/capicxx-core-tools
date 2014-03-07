/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E01HELLOWORLDSTUBIMPL_H_
#define E01HELLOWORLDSTUBIMPL_H_

#include <CommonAPI/CommonAPI.h>
#include <commonapi/examples/E01HelloWorldStubDefault.h>

class E01HelloWorldStubImpl: public commonapi::examples::E01HelloWorldStubDefault {

public:
    E01HelloWorldStubImpl();
    virtual ~E01HelloWorldStubImpl();
    virtual void sayHello(const std::shared_ptr<CommonAPI::ClientId> clientId, std::string name, std::string& message);

};

#endif /* E01HELLOWORLDSTUBIMPL_H_ */
