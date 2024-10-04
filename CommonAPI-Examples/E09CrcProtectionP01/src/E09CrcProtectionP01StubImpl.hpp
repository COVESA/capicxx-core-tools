// Copyright (C) 2023 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef E09CRCPROTECTIONP01STUBIMPL_HPP_
#define E09CRCPROTECTIONP01STUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/examples/E09CrcProtectionP01StubDefault.hpp>

class E09CrcProtectionP01StubImpl: public v1_0::commonapi::examples::E09CrcProtectionP01StubDefault {

public:
    E09CrcProtectionP01StubImpl();
    virtual ~E09CrcProtectionP01StubImpl();
    virtual void incCounter();

private:
    int cnt;
};

#endif // E09CRCPROTECTIONP01STUBIMPL_HPP_
