/* Copyright (C) 2015-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StabilitySPStub.hpp"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace v1 {
namespace commonapi {
namespace stability {
namespace sp {

using namespace v1_0::commonapi::stability::sp;

StabilitySPStub::StabilitySPStub() {
}

StabilitySPStub::~StabilitySPStub() {
}

void StabilitySPStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> _client,
        TestInterface::tArray _tArrayIn,
        testMethodReply_t _reply) {
    (void)_client;

        TestInterface::tArray tArrayOut;

        tArrayOut = _tArrayIn;

        fireTestBroadcastEvent(
            _tArrayIn
        );

        _reply(tArrayOut);


}

void StabilitySPStub::setTestValues(const TestInterface::tArray &_x) {
    setTestAttributeAttribute(_x);
}

} /* namespace v1 */
} /* namespace sp */
} /* namespace stability */
} /* namespace commonapi */
