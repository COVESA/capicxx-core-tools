/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "AFExtendedStub.h"

namespace v1 {
namespace commonapi {
namespace advanced {
namespace extended {

AFExtendedBaseImpl::AFExtendedBaseImpl() {
}

AFExtendedBaseImpl::~AFExtendedBaseImpl() {
}

void AFExtendedBaseImpl::doSomething(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x1, doSomethingReply_t _reply) {
    (void) _client;

    // 100 is a special value for triggering broadcasts
    if (_x1 == 100) {
        fireBBaseEvent(1);
    }
    _reply(_x1);
}

AFExtendedOnceImpl::AFExtendedOnceImpl() {
}

AFExtendedOnceImpl::~AFExtendedOnceImpl() {
}

void AFExtendedOnceImpl::doSomething(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x1, doSomethingReply_t _reply) {
    (void) _client;
    // 100 is a special value for triggering broadcasts
    if (_x1 == 100) {
        fireBBaseEvent(2);
    }
    _reply((uint8_t)(_x1 + 1));
}

void AFExtendedOnceImpl::doSomethingSpecial(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x1, doSomethingSpecialReply_t _reply) {
     (void)_client;
     // 100 is a special value for triggering broadcasts
     if (_x1 == 100) {
         fireBSpecialEvent(3);
     }
     _reply((uint8_t)(_x1 + 2));
}

AFExtendedTwiceImpl::AFExtendedTwiceImpl() {
}

AFExtendedTwiceImpl::~AFExtendedTwiceImpl() {
}

void AFExtendedTwiceImpl::doSomething(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x1, doSomethingReply_t _reply) {
    (void) _client;
    // 100 is a special value for triggering broadcasts
    if (_x1 == 100) {
        fireBBaseEvent(4);
    }
    _reply((uint8_t)(_x1 + 3));
}

void AFExtendedTwiceImpl::doSomethingSpecial(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x1, doSomethingSpecialReply_t _reply) {
     (void)_client;
     // 100 is a special value for triggering broadcasts
     if (_x1 == 100) {
         fireBSpecialEvent(5);
     }
     _reply((uint8_t)(_x1 + 4));
}

void AFExtendedTwiceImpl::doSomethingExtraSpecial(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x1, doSomethingExtraSpecialReply_t _reply) {
     (void)_client;
     // 100 is a special value for triggering broadcasts
     if (_x1 == 100) {
         fireBExtraSpecialEvent(6);
     }
     _reply((uint8_t)(_x1 + 5));
}

} /* namespace extended */
} /* namespace advanced */
} /* namespace commonapi */
} /* namespace v1 */
