/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef THMAINLOOPINTEGRATION_H_
#define THMAINLOOPINTEGRATION_H_

#include "v1_0/commonapi/threading/TestInterfaceStubDefault.hpp"

namespace v1_0 {
namespace commonapi {
namespace threading {

class THMainLoopIntegrationStub : public TestInterfaceStubDefault {
public:
    THMainLoopIntegrationStub();
    virtual ~THMainLoopIntegrationStub();

    void testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x, testMethodReply_t _reply);

    uint8_t x_;
};

} /* namespace threading */
} /* namespace commonapi */
} /* namespace v1_0 */

#endif /* THMAINLOOPINTEGRATION_H_ */
