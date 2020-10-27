// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef E06UNIONSSTUBIMPL_HPP_
#define E06UNIONSSTUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>

#include "commonapi/examples/E06UnionsStubDefault.hpp"

class E06UnionsStubImpl: public commonapi::examples::E06UnionsStubDefault {

public:
    E06UnionsStubImpl();
    virtual ~E06UnionsStubImpl();

    virtual void setMyValue(int);
};

#endif // E06UNIONSSTUBIMPL_HPP_
