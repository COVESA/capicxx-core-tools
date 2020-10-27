/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DTDerivedStub.hpp"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace v1 {
namespace commonapi {
namespace datatypes {
namespace derived {

using namespace v1_0::commonapi::datatypes::derived;

DTDerivedStub::DTDerivedStub() {
}

DTDerivedStub::~DTDerivedStub() {
}

void DTDerivedStub::fTest(const std::shared_ptr<CommonAPI::ClientId> _client,
        TestInterface::tStructExt _tStructExtIn,
        TestInterface::tEnumExt _tEnumExtIn,
        TestInterface::tUnionExt _tUnionExtIn,
        std::shared_ptr<TestInterface::tBaseStruct> _tBaseStructIn,
        fTestReply_t _reply) {
    (void)_client;

    TestInterface::tStructExt tStructExtOut = _tStructExtIn;
    TestInterface::tEnumExt tEnumExtOut = _tEnumExtIn;
    TestInterface::tUnionExt tUnionExtOut = _tUnionExtIn;
    std::shared_ptr<TestInterface::tBaseStruct> tBaseStructOut = _tBaseStructIn;

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
} /* namespace v1 */
