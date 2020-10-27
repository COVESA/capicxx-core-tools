/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef AFSELECTIVESTUB_HPP_
#define AFSELECTIVESTUB_HPP_

#include "v1/commonapi/advanced/bselective/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace advanced {
namespace bselective {

class AFSelectiveStub : public v1_0::commonapi::advanced::bselective::TestInterfaceStubDefault {
public:
    AFSelectiveStub();
    virtual ~AFSelectiveStub();
    void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x, testMethodReply_t _reply);
    virtual bool onBTestSelectiveSelectiveSubscriptionRequested(const std::shared_ptr<CommonAPI::ClientId> _client);
    virtual void onBTestSelectiveSelectiveSubscriptionChanged(const std::shared_ptr<CommonAPI::ClientId> _client,
            const CommonAPI::SelectiveBroadcastSubscriptionEvent _event);
        
private:
    bool acceptSubscriptions;
    bool fireInChangedHook_;
};


} /* namespace bselective */
} /* namespace advanced */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* AFSELECTIVESTUB_HPP_ */
