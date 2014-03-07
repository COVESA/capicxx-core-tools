/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E03METHODSSTUBIMPL_H_
#define E03METHODSSTUBIMPL_H_

#include <CommonAPI/CommonAPI.h>
#include <commonapi/examples/E03MethodsStubDefault.h>

class E03MethodsStubImpl: public commonapi::examples::E03MethodsStubDefault {

public:
    E03MethodsStubImpl();
    virtual ~E03MethodsStubImpl();
    virtual void incCounter();
    void foo(int32_t x1,
             std::string x2,
             commonapi::examples::E03Methods::fooError& methodError,
             int32_t& y1,
             std::string& y2);

private:
    int cnt;
};

#endif /* E03METHODSSTUBIMPL_H_ */
