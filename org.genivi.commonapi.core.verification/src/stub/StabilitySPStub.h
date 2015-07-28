/* Copyright (C) 2015 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef STABILITYSPSTUB_H_
#define STABILITYSPSTUB_H_

#include "v1_0/commonapi/stability/sp/TestInterfaceStubDefault.hpp"
#include "v1_0/commonapi/stability/sp/TestInterface.hpp"

namespace v1_0 {
namespace commonapi {
namespace stability {
namespace sp {

class StabilitySPStub : public TestInterfaceStubDefault {

public:
    StabilitySPStub();
    virtual ~StabilitySPStub();

    virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId,
            TestInterface::tArray _x,
            testMethodReply_t _reply);
    virtual void setTestValues(TestInterface::tArray x);
};

} /* namespace v1_0 */
} /* namespace sp */
} /* namespace stability */
} /* namespace commonapi */
#endif /* STABILITYSPSTUB_H_ */

