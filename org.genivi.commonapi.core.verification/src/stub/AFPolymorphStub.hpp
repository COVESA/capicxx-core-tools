/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef AFPOLYMORPHSTUB_HPP_
#define AFPOLYMORPHSTUB_HPP_

#include "v1/commonapi/advanced/polymorph/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace advanced {
namespace polymorph {

class AFPolymorphStub : public v1_0::commonapi::advanced::polymorph::TestInterfaceStubDefault {
public:
    AFPolymorphStub();
    virtual ~AFPolymorphStub();
    virtual void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, 
                            std::shared_ptr<TestInterface::PStructBase> _x,
                            testMethodReply_t _reply);    
};


} /* namespace polymorph */
} /* namespace advanced */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* AFPOLYMORPHSTUB_HPP_ */
