/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DTAdvancedStub.hpp"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace v1 {
namespace commonapi {
namespace datatypes {
namespace advanced {

using namespace commonapi::datatypes::advanced;

DTAdvancedStub::DTAdvancedStub() {
}

DTAdvancedStub::~DTAdvancedStub() {
}

void DTAdvancedStub::fTest(const std::shared_ptr<CommonAPI::ClientId> _client,
        TestInterface::tArray _tArrayIn,
        TestInterface::tEnumeration _tEnumerationIn,
        TestInterface::tStruct _tStructIn,
        TestInterface::tUnion _tUnionIn,
        TestInterface::tMap _tMapIn,
        TestInterface::tTypedef _tTypedefIn,
        std::vector<TestInterface::tEnumeration> _tEnumerationArrayIn,
        fTestReply_t _reply) {
    (void)_client;

    TestInterface::tArray tArrayOut = _tArrayIn;
    TestInterface::tEnumeration tEnumerationOut = _tEnumerationIn;
    TestInterface::tStruct tStructOut = _tStructIn;
    TestInterface::tUnion tUnionOut = _tUnionIn;
    TestInterface::tMap tMapOut = _tMapIn;
    TestInterface::tTypedef tTypedefOut = _tTypedefIn;
    std::vector<TestInterface::tEnumeration> _tEnumerationArrayOut = _tEnumerationArrayIn;

    _reply(tArrayOut, tEnumerationOut, tStructOut, tUnionOut, tMapOut, tTypedefOut, _tEnumerationArrayOut);

    fireBTestEvent(
            tArrayOut,
            tEnumerationOut,
            tStructOut,
            tUnionOut,
            tMapOut,
            tTypedefOut
    );
}

} /* namespace advanced */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1 */
