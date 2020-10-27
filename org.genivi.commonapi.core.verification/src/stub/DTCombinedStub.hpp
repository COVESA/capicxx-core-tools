/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DTCOMBINEDSTUB_HPP_
#define DTCOMBINEDSTUB_HPP_

#include "v1/commonapi/datatypes/combined/TestInterfaceStubDefault.hpp"
#include "v1/commonapi/datatypes/combined/TestInterface.hpp"

namespace v1 {
namespace commonapi {
namespace datatypes {
namespace combined {

using namespace v1_0::commonapi::datatypes::combined;

class DTCombinedStub : public TestInterfaceStubDefault {

public:
    DTCombinedStub();
    virtual ~DTCombinedStub();

    virtual void fTest(const std::shared_ptr<CommonAPI::ClientId> _client,
            TestInterface::tStructL3 _tStructL3In,
            fTestReply_t _reply);
};

} /* namespace combined */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* DTCOMBINEDSTUB_HPP_ */
