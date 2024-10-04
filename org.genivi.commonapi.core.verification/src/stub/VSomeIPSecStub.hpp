/* Copyright (C) 2015-2024 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef VSOMEIPSECSTUB_HPP_
#define VSOMEIPSECSTUB_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/vsomeipsec/ClientIdServiceStubDefault.hpp>

namespace v1 {
namespace commonapi {
namespace vsomeipsec {

using clientHandler_t = std::function<void(const std::shared_ptr<CommonAPI::ClientId> )>;

class ClientIdServiceImpl : public ClientIdServiceStubDefault {
private:
    clientHandler_t client_validation_handler;

public:
    ClientIdServiceImpl(const clientHandler_t &_client_validation_handler);

    const uint32_t &getAAttribute(const std::shared_ptr<CommonAPI::ClientId> _client);
    void onB2SelectiveSubscriptionChanged(
            const std::shared_ptr<CommonAPI::ClientId> _client,
            const CommonAPI::SelectiveBroadcastSubscriptionEvent _event);

    bool onB2SelectiveSubscriptionRequested(
            const std::shared_ptr<CommonAPI::ClientId> _client);

    void m(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _i,
            mReply_t _reply);
};

} /* namespace vsomeipsec */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* VSOMEIPSECSTUB_HPP_ */
