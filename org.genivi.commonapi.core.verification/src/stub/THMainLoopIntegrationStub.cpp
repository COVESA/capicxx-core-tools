/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "THMainLoopIntegrationStub.hpp"

namespace v1 {
namespace commonapi {
namespace threading {

THMainLoopIntegrationStub::THMainLoopIntegrationStub() :
    x_(0),
    secondTestBroadcastValueToFireOnSubscription_(0) {
}

THMainLoopIntegrationStub::~THMainLoopIntegrationStub() {
}

void THMainLoopIntegrationStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, 
                                           uint8_t _x, testMethodReply_t _reply) {
    (void)_client;
    uint8_t y = _x;
    x_ = _x;

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

void THMainLoopIntegrationStub::onSecondTestBroadcastSelectiveSubscriptionChanged(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) {
    (void) _client;
    if(_event == CommonAPI::SelectiveBroadcastSubscriptionEvent::SUBSCRIBED) {
        fireSecondTestBroadcastSelective(secondTestBroadcastValueToFireOnSubscription_);
    }
}

bool THMainLoopIntegrationStub::onSecondTestBroadcastSelectiveSubscriptionRequested(
        const std::shared_ptr<CommonAPI::ClientId> _client) {
    (void) _client;
    return true;
}

void THMainLoopIntegrationStub::setSecondTestBroadcastValueToFireOnSubscription_(const uint32_t &_value) {
    secondTestBroadcastValueToFireOnSubscription_ = _value;
}

} /* namespace threading */
} /* namespace commonapi */
} /* namespace v1 */
