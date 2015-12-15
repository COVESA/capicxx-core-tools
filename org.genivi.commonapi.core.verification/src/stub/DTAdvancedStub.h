/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DTADVANCEDSTUB_H_
#define DTADVANCEDSTUB_H_

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

    virtual void fTest(const std::shared_ptr<CommonAPI::ClientId> clientId,
            TestInterface::tArray tArrayIn,
            TestInterface::tEnumeration tEnumerationIn,
            TestInterface::tStruct tStructIn,
            TestInterface::tUnion tUnionIn, TestInterface::tMap tMapIn,
            TestInterface::tTypedef tTypedefIn,
            std::vector<TestInterface::tEnumeration> _tEnumerationArrayIn,
            fTestReply_t _reply);
};

} /* namespace advanced */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1 */
#endif /* DTADVANCEDSTUB_H_ */
