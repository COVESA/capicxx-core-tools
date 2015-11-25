/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DTAdvancedStub.h"

#ifndef WIN32
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

void DTAdvancedStub::fTest(const std::shared_ptr<CommonAPI::ClientId> clientId,
        TestInterface::tArray tArrayIn,
        TestInterface::tEnumeration tEnumerationIn,
        TestInterface::tStruct tStructIn,
        TestInterface::tUnion tUnionIn,
        TestInterface::tMap tMapIn,
        TestInterface::tTypedef tTypedefIn,
        fTestReply_t _reply) {
    (void)clientId;

    TestInterface::tArray tArrayOut = tArrayIn;
    TestInterface::tEnumeration tEnumerationOut = tEnumerationIn;
    TestInterface::tStruct tStructOut = tStructIn;
    TestInterface::tUnion tUnionOut = tUnionIn;
    TestInterface::tMap tMapOut = tMapIn;
    TestInterface::tTypedef tTypedefOut = tTypedefIn;

    _reply(tArrayOut, tEnumerationOut, tStructOut, tUnionOut, tMapOut, tTypedefOut);

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
