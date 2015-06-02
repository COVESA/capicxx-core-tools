/* Copyright (C) 2015 BMW Group
 * Author: JP Ikaheimonen (jp_ikaheimonen@mentor.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "StabControlStub.h"
#include <unistd.h>

namespace v1_0 {
namespace commonapi {
namespace stability {
namespace mp {

using namespace v1_0::commonapi::stability::mp;

StabControlStub::StabControlStub() {
}

StabControlStub::~StabControlStub() {
}

CommandListener StabControlStub::listener_ = 0;


void StabControlStub::controlMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
		uint8_t id,
		uint32_t data,
		controlMethodReply_t _reply)
{
		// client sends commands and responses through this interface.
		// call the listener with the data.
		uint8_t command;
                uint32_t min;
                uint32_t max;

		if (StabControlStub::listener_) {
                    StabControlStub::listener_(id, data, command, min, max);
		}
                _reply(command, min, max);

}

void StabControlStub::registerListener(CommandListener listener ) {
	StabControlStub::listener_ = listener;
}

} /* namespace v1_0 */
} /* namespace mp */
} /* namespace stability */
} /* namespace commonapi */



