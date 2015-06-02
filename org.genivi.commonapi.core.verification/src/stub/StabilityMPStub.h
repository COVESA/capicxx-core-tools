/* Copyright (C) 2015 BMW Group
 * Author: JP Ikaheimonen (jp_ikaheimonen@mentor.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef STABILITYMPSTUB_H_
#define STABILITYMPSTUB_H_

#include "v1_0/commonapi/stability/mp/TestInterfaceStubDefault.hpp"
#include "v1_0/commonapi/stability/mp/TestInterface.hpp"

namespace v1_0 {
namespace commonapi {
namespace stability {
namespace mp {

class StabilityMPStub : public TestInterfaceStubDefault {

public:
    StabilityMPStub();
    virtual ~StabilityMPStub();

    virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
            TestInterface::tArray tArrayIn,
            testMethodReply_t _reply);
    virtual void setTestValues(TestInterface::tArray x);
};

} /* namespace v1_0 */
} /* namespace mp */
} /* namespace stability */
} /* namespace commonapi */
#endif /* STABILITYMPSTUB_H_ */

