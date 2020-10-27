/* Copyright (C) 2015-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PFCOMPLEXSTUB_HPP_
#define PFCOMPLEXSTUB_HPP_

#include "v1/commonapi/performance/complex/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace performance {
namespace complex {

class PFComplexStub : public TestInterfaceStubDefault {
public:
    PFComplexStub();
    virtual ~PFComplexStub();

    virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, TestInterface::tArray _x, testMethodReply_t _reply);
};

} /* namespace complex */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace  v1     */

#endif /* PFCOMPLEXSTUB_HPP_ */
