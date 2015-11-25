/* Copyright (C) 2014, 2015 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E03METHODSSTUBIMPL_H_
#define E03METHODSSTUBIMPL_H_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E03MethodsStubDefault.hpp>

class E03MethodsStubImpl: public v1_2::commonapi::examples::E03MethodsStubDefault {

public:
    E03MethodsStubImpl();
    virtual ~E03MethodsStubImpl();
    virtual void incCounter();
    virtual void foo(const std::shared_ptr<CommonAPI::ClientId> _client,
            int32_t _x1,
            std::string _x2,
            fooReply_t _reply);

private:
    int cnt;
};

#endif /* E03METHODSSTUBIMPL_H_ */
