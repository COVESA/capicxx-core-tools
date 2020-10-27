/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "CMBroadcastsStub.hpp"

namespace v1 {
namespace commonapi {
namespace communication {

CMBroadcastsStub::CMBroadcastsStub() {
    acceptSubscriptions = true;
}

CMBroadcastsStub::~CMBroadcastsStub() {
}

bool CMBroadcastsStub::onBTestSelectiveSelectiveSubscriptionRequested(const std::shared_ptr<CommonAPI::ClientId> _client) {
    (void)_client;
    return acceptSubscriptions;
}


void CMBroadcastsStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x,
                                  testMethodReply_t _reply) {
    (void)_client;
                                       
    uint8_t y;
    
    switch (_x) {
    case 1: // send data (= 1) through non-selective broadcast
        fireBTestEvent(1);
        break;
    case 2: // stop accepting subscriptions
        acceptSubscriptions = false;
        break;
    case 3: // send data (= 1) through selective broadcast
        fireBTestSelectiveSelective(1);
        break;    
    case 4: // start accepting subscriptions
        acceptSubscriptions = true;
        break;
    default:
        break;
    }
    
    y = _x;
    _reply(y);    
}


} /* namespace v1 */
} /* namespace communication */
} /* namespace commonapi */

