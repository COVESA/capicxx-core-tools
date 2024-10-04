// Copyright (C) 2022 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef E08CRCPROTECTIONP04STUBIMPL_HPP_
#define E08CRCPROTECTIONP04STUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E08CrcProtectionP04StubDefault.hpp>

class E08CrcProtectionP04StubImpl: public v1_0::commonapi::examples::E08CrcProtectionP04StubDefault {

public:
    E08CrcProtectionP04StubImpl();
    virtual ~E08CrcProtectionP04StubImpl();
    virtual void incCounter();

private:
    int cnt;
};

#endif // E08CRCPROTECTIONP04STUBIMPL_HPP_
