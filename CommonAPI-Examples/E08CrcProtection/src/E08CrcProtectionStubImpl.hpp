// Copyright (C) 2022 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef E08CRCPROTECTIONSTUBIMPL_HPP_
#define E08CRCPROTECTIONSTUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E08CrcProtectionStubDefault.hpp>

class E08CrcProtectionStubImpl: public v1_0::commonapi::examples::E08CrcProtectionStubDefault {

public:
    E08CrcProtectionStubImpl();
    virtual ~E08CrcProtectionStubImpl();
    virtual void incCounter();

private:
    int cnt;
};

#endif // E08CRCPROTECTIONSTUBIMPL_HPP_
