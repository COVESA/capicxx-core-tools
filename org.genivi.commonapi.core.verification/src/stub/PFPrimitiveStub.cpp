/* Copyright (C) 2015 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include "PFPrimitiveStub.h"

namespace v1 {
namespace commonapi {
namespace performance {
namespace primitive {


PFPrimitiveStub::PFPrimitiveStub() {

}

PFPrimitiveStub::~PFPrimitiveStub() {

}

void PFPrimitiveStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> _client,
        TestInterface::TestArray _x, testMethodReply_t _reply) {
    (void)_client;
    TestInterface::TestArray y;

    // Copy array!
    y = _x;
    _reply(y);
}

} /* namespace primitive */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace  v1     */

