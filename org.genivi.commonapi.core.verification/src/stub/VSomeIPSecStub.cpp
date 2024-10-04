#include <chrono>
#include <iostream>
#include <thread>

#include "VSomeIPSecStub.hpp"

namespace v1{
namespace commonapi {
namespace vsomeipsec {

ClientIdServiceImpl::ClientIdServiceImpl(const clientHandler_t &_client_validation_handler):
        client_validation_handler(_client_validation_handler) {
}

const uint32_t &ClientIdServiceImpl::getAAttribute(
        const std::shared_ptr<CommonAPI::ClientId> _client) {

    if (_client->getHostAddress() != "") {
        if(client_validation_handler) {
            client_validation_handler(_client);
        }
    }
    return ClientIdServiceStubDefault::getAAttribute();
}

void ClientIdServiceImpl::onB2SelectiveSubscriptionChanged(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) {

    (void)_event;
    if(client_validation_handler) {
        client_validation_handler(_client);
    }
}

bool ClientIdServiceImpl::onB2SelectiveSubscriptionRequested(
        const std::shared_ptr<CommonAPI::ClientId> _client) {

    if(client_validation_handler) {
        client_validation_handler(_client);
    }
    return true;
}

void ClientIdServiceImpl::m(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _i, mReply_t _reply) {

    uint32_t o = _i;
    _reply(o);

    if(client_validation_handler) {
        client_validation_handler(_client);
    }
}

} /* namespace vsomeipsec */
} /* namespace commonapi */
} /* namespace v1 */
