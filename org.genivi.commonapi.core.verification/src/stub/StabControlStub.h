/* Copyright (C) 2014 BMW Group
 * Author: JP Ikaheimonen (jp_ikaheimonen@mentor.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef STABCONTROLSTUB_H_
#define STABCONTROLSTUB_H_

#include "v1_0/commonapi/stability/mp/ControlInterfaceStubDefault.hpp"
#include "v1_0/commonapi/stability/mp/ControlInterface.hpp"

namespace v1_0 {
namespace commonapi {
namespace stability {
namespace mp {

using namespace v1_0::commonapi::stability::mp;

typedef std::function<void (uint8_t id, uint32_t data, uint8_t& command, uint32_t& data1, uint32_t &data2)> CommandListener;

class StabControlStub : public ControlInterfaceStubDefault {

public:
	StabControlStub();
    virtual ~StabControlStub();

    virtual void controlMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
    		uint8_t id,
    		uint32_t data,
                controlMethodReply_t _reply);    		

    static void registerListener(CommandListener listener);

    static CommandListener listener_;

};

} /* namespace v1_0 */
} /* namespace mp */
} /* namespace stability */
} /* namespace commonapi */
#endif /* STABCONTROLSTUB_H_ */


