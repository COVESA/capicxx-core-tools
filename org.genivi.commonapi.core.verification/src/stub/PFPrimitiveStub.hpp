/* Copyright (C) 2015-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PFPRIMITIVESTUB_HPP_
#define PFPRIMITIVESTUB_HPP_

#include "v1/commonapi/performance/primitive/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace performance {
namespace primitive {

class PFPrimitiveStub : public TestInterfaceStubDefault {
public:
    PFPrimitiveStub();
    virtual ~PFPrimitiveStub();

    virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, TestInterface::TestArray _x, testMethodReply_t _reply);
};

} /* namespace primitive */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace  v1     */

#endif /* PFPRIMITIVESTUB_HPP_ */
