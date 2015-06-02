/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E02ATTRIBUTESSTUBIMPL_H_
#define E02ATTRIBUTESSTUBIMPL_H_

#include <CommonAPI/CommonAPI.hpp>
#include <v1_0/commonapi/examples/E02AttributesStubDefault.hpp>

class E02AttributesStubImpl: public v1_0::commonapi::examples::E02AttributesStubDefault {

public:
    E02AttributesStubImpl();
    virtual ~E02AttributesStubImpl();
    virtual void incCounter();

private:
    int cnt;
};

#endif /* E02ATTRIBUTESSTUBIMPL_H_ */
