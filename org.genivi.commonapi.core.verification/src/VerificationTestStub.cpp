/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VerificationTestStub.h"

namespace commonapi {
namespace verification {

void VerificationTestStub::testDerivedTypeMethod(
                                 commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testEnumExtended2InValue,
                                 commonapi::tests::DerivedTypeCollection::TestMap testMapInValue,
                                 commonapi::tests::DerivedTypeCollection::TestEnumExtended2& testEnumExtended2OutValue,
                                 commonapi::tests::DerivedTypeCollection::TestMap& testMapOutValue) {
    testEnumExtended2OutValue = testEnumExtended2InValue;
    testMapOutValue = testMapInValue;
    called++;
}

VerificationTestStub::VerificationTestStub() :
                TestInterfaceStubDefault(), called(0) {
}

} /* namespace verification */
} /* namespace commonapi */
