/* Copyright (C) 2015 Mentor Graphics
 * Author: Felix Scherzinger (felix_scherzinger@mentor.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

#include "PFPrimitiveStub.h"

namespace v1_0 {
namespace commonapi {
namespace performance {
namespace primitive {


PFPrimitiveStub::PFPrimitiveStub() {

}

PFPrimitiveStub::~PFPrimitiveStub() {

}

void PFPrimitiveStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
		TestInterface::TestArray x, testMethodReply_t _reply) {

	TestInterface::TestArray y;

	// Copy array!
	y = x;
	_reply(y);
}

} /* namespace primitive */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace  v1_0     */

