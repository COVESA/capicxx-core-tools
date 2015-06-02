/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMMETHODCALLSSTUB_H_
#define CMMETHODCALLSSTUB_H_

#include "v1_0/commonapi/communication/TestInterfaceStubDefault.hpp"

namespace v1_0 {
namespace commonapi {
namespace communication {

class CMMethodCallsStub : public v1_0::commonapi::communication::TestInterfaceStubDefault {
public:
    CMMethodCallsStub();
    virtual ~CMMethodCallsStub();

    void testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x, 
        testMethodReply_t _reply);
};

} /* namespace v1_0 */
} /* namespace communication */
} /* namespace commonapi */

#endif /* CMMETHODCALLSSTUB_H_ */
