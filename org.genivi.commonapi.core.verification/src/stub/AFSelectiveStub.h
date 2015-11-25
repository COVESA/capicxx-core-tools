/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef AFSELECTIVESTUB_H_
#define AFSELECTIVESTUB_H_

#include "v1/commonapi/advanced/bselective/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace advanced {
namespace bselective {

class AFSelectiveStub : public v1_0::commonapi::advanced::bselective::TestInterfaceStubDefault {
public:
    AFSelectiveStub();
    virtual ~AFSelectiveStub();
    void testMethod(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x, 
        testMethodReply_t _reply);
    virtual bool onBTestSelectiveSelectiveSubscriptionRequested(const std::shared_ptr<CommonAPI::ClientId> _client);
        
private:
    bool acceptSubscriptions;
};


} /* namespace bselective */
} /* namespace advanced */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* AFSELECTIVESTUB_H_ */
