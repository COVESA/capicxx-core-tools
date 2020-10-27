/* Copyright (C) 2015-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include "PFComplexStub.hpp"

namespace v1 {
namespace commonapi {
namespace performance {
namespace complex {


PFComplexStub::PFComplexStub() {

}

PFComplexStub::~PFComplexStub() {

}

void PFComplexStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, TestInterface::tArray _x, testMethodReply_t _reply) {
    (void)_client;
    TestInterface::tArray y;

    // Copy array!
    y = _x;
    _reply(y);
}

} /* namespace complex */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace  v1     */
