/* Copyright (C) 2015-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef STABILITYSPSTUB_HPP_
#define STABILITYSPSTUB_HPP_

#include "v1/commonapi/stability/sp/TestInterfaceStubDefault.hpp"
#include "v1/commonapi/stability/sp/TestInterface.hpp"

namespace v1 {
namespace commonapi {
namespace stability {
namespace sp {

class StabilitySPStub : public TestInterfaceStubDefault {

public:
    StabilitySPStub();
    virtual ~StabilitySPStub();

    virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client,
            TestInterface::tArray _x,
            testMethodReply_t _reply);
    virtual void setTestValues(const TestInterface::tArray &_x);
};

} /* namespace v1 */
} /* namespace sp */
} /* namespace stability */
} /* namespace commonapi */
#endif /* STABILITYSPSTUB_H_ */

