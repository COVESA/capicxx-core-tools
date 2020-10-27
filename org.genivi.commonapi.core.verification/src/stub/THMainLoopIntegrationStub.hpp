/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef THMAINLOOPINTEGRATION_HPP_
#define THMAINLOOPINTEGRATION_HPP_

#include "v1/commonapi/threading/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace threading {

class THMainLoopIntegrationStub : public TestInterfaceStubDefault {
public:
    THMainLoopIntegrationStub();
    virtual ~THMainLoopIntegrationStub();

    void testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x, testMethodReply_t _reply);

    uint8_t x_;

    void onSecondTestBroadcastSelectiveSubscriptionChanged(
            const std::shared_ptr<CommonAPI::ClientId> _client,
            const CommonAPI::SelectiveBroadcastSubscriptionEvent _event);

    bool onSecondTestBroadcastSelectiveSubscriptionRequested(
            const std::shared_ptr<CommonAPI::ClientId> _client);


    void setSecondTestBroadcastValueToFireOnSubscription_(const uint32_t &_value);
private:
    uint32_t secondTestBroadcastValueToFireOnSubscription_;
};

} /* namespace threading */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* THMAINLOOPINTEGRATION_HPP_ */
