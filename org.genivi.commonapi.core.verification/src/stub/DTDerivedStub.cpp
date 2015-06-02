/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DTDerivedStub.h"
#include <unistd.h>

namespace v1_0 {
namespace commonapi {
namespace datatypes {
namespace derived {

using namespace v1_0::commonapi::datatypes::derived;

DTDerivedStub::DTDerivedStub() {
}

DTDerivedStub::~DTDerivedStub() {
}

void DTDerivedStub::fTest(const std::shared_ptr<CommonAPI::ClientId> clientId,
        TestInterface::tStructExt tStructExtIn,
        TestInterface::tEnumExt tEnumExtIn,
        TestInterface::tUnionExt tUnionExtIn,
        std::shared_ptr<TestInterface::tBaseStruct> tBaseStructIn,
        fTestReply_t _reply) {

    TestInterface::tStructExt tStructExtOut = tStructExtIn;
    TestInterface::tEnumExt tEnumExtOut = tEnumExtIn;
    TestInterface::tUnionExt tUnionExtOut = tUnionExtIn;
    std::shared_ptr<TestInterface::tBaseStruct> tBaseStructOut = tBaseStructIn;

	_reply(tStructExtOut, tEnumExtOut, tUnionExtOut, tBaseStructOut);

    fireBTestEvent(
            tStructExtOut,
            tEnumExtOut,
            tUnionExtOut,
            tBaseStructOut
    );
}

} /* namespace derived */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1_0 */
