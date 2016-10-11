/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MPSelectiveSTUBIMPL_H_
#define MPSelectiveSTUBIMPL_H_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/multiprocess/bselective/MPSelectiveStubDefault.hpp>

class MPSelectiveStubImpl: public v1_0::commonapi::multiprocess::bselective::MPSelectiveStubDefault {

public:
    MPSelectiveStubImpl();
    virtual ~MPSelectiveStubImpl();

};

#endif
