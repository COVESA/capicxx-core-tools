/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DTDERIVEDSTUB_HPP_
#define DTDERIVEDSTUB_HPP_

#include "v1/commonapi/datatypes/derived/TestInterfaceStubDefault.hpp"
#include "v1/commonapi/datatypes/derived/TestInterface.hpp"

namespace v1 {
namespace commonapi {
namespace datatypes {
namespace derived {

using namespace v1_0::commonapi::datatypes::derived;

class DTDerivedStub : public TestInterfaceStubDefault {

public:
    DTDerivedStub();
    virtual ~DTDerivedStub();

    virtual void fTest(const std::shared_ptr<CommonAPI::ClientId> _client,
            TestInterface::tStructExt _tStructExtIn,
            TestInterface::tEnumExt _tEnumExtIn,
            TestInterface::tUnionExt _tUnionExtIn,
            std::shared_ptr<TestInterface::tBaseStruct> _tBaseStructIn,
            fTestReply_t _reply);
};

} /* namespace derived */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* DTDERIVEDSTUB_HPP_ */
