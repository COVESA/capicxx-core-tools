/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E06UNIONSSTUBIMPL_H_
#define E06UNIONSSTUBIMPL_H_

#include <CommonAPI/CommonAPI.hpp>

#include "../src-gen/commonapi/examples/E06UnionsStubDefault.hpp"

class E06UnionsStubImpl: public commonapi::examples::E06UnionsStubDefault {

public:
    E06UnionsStubImpl();
    virtual ~E06UnionsStubImpl();

    virtual void setMyValue(int);
};

#endif /* E06UNIONSSTUBIMPL_H_ */
