/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DTADVANCEDSTUB_HPP_
#define DTADVANCEDSTUB_HPP_

#include "v1/commonapi/datatypes/advanced/TestInterfaceStubDefault.hpp"
#include "v1/commonapi/datatypes/advanced/TestInterface.hpp"

namespace v1 {
namespace commonapi {
namespace datatypes {
namespace advanced {

using namespace v1_0::commonapi::datatypes::advanced;

class DTAdvancedStub : public TestInterfaceStubDefault {

public:
    DTAdvancedStub();
    virtual ~DTAdvancedStub();

    virtual void fTest(const std::shared_ptr<CommonAPI::ClientId> _client,
            TestInterface::tArray _tArrayIn,
            TestInterface::tEnumeration _tEnumerationIn,
            TestInterface::tStruct _tStructIn,
            TestInterface::tUnion _tUnionIn,
            TestInterface::tMap _tMapIn,
            TestInterface::tTypedef _tTypedefIn,
            std::vector<TestInterface::tEnumeration> _tEnumerationArrayIn,
            fTestReply_t _reply);
};

} /* namespace advanced */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* DTADVANCEDSTUB_HPP_ */
