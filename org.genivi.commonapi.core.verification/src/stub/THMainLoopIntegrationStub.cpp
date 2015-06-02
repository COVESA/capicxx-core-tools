/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "THMainLoopIntegrationStub.h"

namespace v1_0 {
namespace commonapi {
namespace threading {

THMainLoopIntegrationStub::THMainLoopIntegrationStub() {
    x_ = 0;
}

THMainLoopIntegrationStub::~THMainLoopIntegrationStub() {
}

void THMainLoopIntegrationStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x, testMethodReply_t _reply) {

	uint8_t y = x;
    x_ = x;

    uint8_t broadcastNumber = 0;
    broadcastNumber++;
    fireTestBroadcastEvent(broadcastNumber);
    broadcastNumber++;
    fireTestBroadcastEvent(broadcastNumber);
    broadcastNumber++;
    fireTestBroadcastEvent(broadcastNumber);
    broadcastNumber++;
    fireTestBroadcastEvent(broadcastNumber);
    broadcastNumber++;
    fireTestBroadcastEvent(broadcastNumber);

    _reply(y);
}

} /* namespace threading */
} /* namespace commonapi */
} /* namespace v1_0 */
