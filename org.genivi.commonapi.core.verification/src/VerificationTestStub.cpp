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
    calledTestDerivedTypeMethod++;
}

VerificationTestStub::VerificationTestStub() :
                TestInterfaceStubDefault(), calledTestDerivedTypeMethod(0) {
}

void VerificationTestStub::testPredefinedTypeMethod(const CommonAPI::ClientId& clientId,
                                                    uint32_t uint32InValue,
                                                    std::string stringInValue,
                                                    uint32_t& uint32OutValue,
                                                    std::string& stringOutValue) {
    uint32OutValue = 1;
    int broadcastNumber = 1;

    fireTestPredefinedTypeBroadcastEvent(broadcastNumber++, "");
    fireTestPredefinedTypeBroadcastEvent(broadcastNumber++, "");
    fireTestPredefinedTypeBroadcastEvent(broadcastNumber++, "");
    fireTestPredefinedTypeBroadcastEvent(broadcastNumber++, "");
    fireTestPredefinedTypeBroadcastEvent(broadcastNumber++, "");
    sleep(10);
}

} /* namespace verification */
} /* namespace commonapi */
