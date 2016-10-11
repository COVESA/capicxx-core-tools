/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "AFSelectiveStub.h"

namespace v1 {
namespace commonapi {
namespace advanced {
namespace bselective {
    
AFSelectiveStub::AFSelectiveStub() {
    acceptSubscriptions = true;
    fireInChangedHook_ = false;
}

AFSelectiveStub::~AFSelectiveStub() {
}

bool AFSelectiveStub::onBTestSelectiveSelectiveSubscriptionRequested(const std::shared_ptr<CommonAPI::ClientId> _client) {
    (void)_client;
    return acceptSubscriptions;
}

void AFSelectiveStub::onBTestSelectiveSelectiveSubscriptionChanged(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) {

    (void)_client;
    (void)_event;

    if (fireInChangedHook_ &&
            _event == CommonAPI::SelectiveBroadcastSubscriptionEvent::SUBSCRIBED) {
        fireBTestSelectiveSelective(1);
    }
}


void AFSelectiveStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x,
                                   testMethodReply_t _reply) {
    (void)_client;
    
                                       
    uint8_t y;
    
    switch (_x) {

    case 2: // stop accepting subscriptions
        acceptSubscriptions = false;
        break;
    case 3: // send data (= 1) through selective broadcast
        fireBTestSelectiveSelective(1);
        break;    
    case 4: // start accepting subscriptions
        acceptSubscriptions = true;
        break;
    case 5:
        fireInChangedHook_ = true;
        break;
    default:
        break;
    }
    
    y = _x;
    _reply(y);    
}

} /* namespace bselective */
} /* namespace advanced */
} /* namespace commonapi */
} /* namespace v1 */

