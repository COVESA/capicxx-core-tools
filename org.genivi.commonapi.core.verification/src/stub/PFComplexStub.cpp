/* Copyright (C) 2015 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include "PFComplexStub.h"

namespace v1_0 {
namespace commonapi {
namespace performance {
namespace complex {


PFComplexStub::PFComplexStub() {

}

PFComplexStub::~PFComplexStub() {

}

void PFComplexStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,	TestInterface::tArray x, testMethodReply_t _reply) {

	TestInterface::tArray y;

	// Copy array!
	y = x;
	_reply(y);
}

} /* namespace complex */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace  v1_0     */
