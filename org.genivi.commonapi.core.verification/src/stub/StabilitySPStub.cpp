/* Copyright (C) 2015 BMW Group
 * Author: JP Ikaheimonen (jp_ikaheimonen@mentor.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StabilitySPStub.h"

#ifndef WIN32
#include <unistd.h>
#endif

namespace v1_0 {
namespace commonapi {
namespace stability {
namespace sp {

using namespace v1_0::commonapi::stability::sp;

StabilitySPStub::StabilitySPStub() {
}

StabilitySPStub::~StabilitySPStub() {
}

void StabilitySPStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
        TestInterface::tArray tArrayIn,
        testMethodReply_t _reply) {

        TestInterface::tArray tArrayOut;

        tArrayOut = tArrayIn;

        fireTestBroadcastEvent(
            tArrayIn
        );

        _reply(tArrayOut);


}

void StabilitySPStub::setTestValues(TestInterface::tArray x) {
    setTestAttributeAttribute(x);
}

} /* namespace v1_0 */
} /* namespace sp */
} /* namespace stability */
} /* namespace commonapi */


