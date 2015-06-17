/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "E06UnionsStubImpl.h"

#include "commonapi/examples/CommonTypes.hpp"

using namespace commonapi::examples;

E06UnionsStubImpl::E06UnionsStubImpl() {
}

E06UnionsStubImpl::~E06UnionsStubImpl() {
}

void E06UnionsStubImpl::setMyValue(int n) {
    if (n >= 0 && n < 4) {
        std::cout << "Set type " << n << " for attribute u." << std::endl;

        CommonTypes::MyTypedef t0 = -5;
        CommonTypes::MyEnum t1 = CommonTypes::MyEnum::OFF;
        uint8_t t2 = 42;
        std::string t3 = "∃y ∀x ¬(x ≺ y)";

        if (n == 0) {
            CommonTypes::SettingsUnion v(t0);
            setUAttribute(v);
		    setXAttribute(std::make_shared<CommonTypes::SettingsStructMyTypedef>(t0));
        } else if (n == 1) {
            CommonTypes::SettingsUnion v(t1);
            setUAttribute(v);
		    setXAttribute(std::make_shared<CommonTypes::SettingsStructMyEnum>(t1));
        } else if (n == 2) {
            CommonTypes::SettingsUnion v(t2);
            setUAttribute(v);
		    setXAttribute(std::make_shared<CommonTypes::SettingsStructUInt8>(t2));
        } else if (n == 3) {
            CommonTypes::SettingsUnion v(t3);
            setUAttribute(v);
		    setXAttribute(std::make_shared<CommonTypes::SettingsStructString>(t3));
        }

    } else {
        std::cout << "Type number " << n << " not possible." << std::endl;
    }
}
