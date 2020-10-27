// Copyright (C) 2015-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef AFEXTENDEDSTUB_HPP_
#define AFEXTENDEDSTUB_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/advanced/extended/AFExtendedBaseStubDefault.hpp>
#include <v1/commonapi/advanced/extended/AFExtendedOnceStubDefault.hpp>
#include <v1/commonapi/advanced/extended/AFExtendedTwiceStubDefault.hpp>

namespace v1 {
namespace commonapi {
namespace advanced {
namespace extended {

class AFExtendedBaseImpl: public AFExtendedBaseStubDefault {

public:
    AFExtendedBaseImpl();
    virtual ~AFExtendedBaseImpl();

    void doSomething(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x,
        doSomethingReply_t _reply);

private:

};

class AFExtendedOnceImpl: public AFExtendedOnceStubDefault {

public:
    AFExtendedOnceImpl();
    virtual ~AFExtendedOnceImpl();
    void doSomething(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x,
        doSomethingReply_t _reply);
    void doSomethingSpecial(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x,
        doSomethingSpecialReply_t _reply);

private:

};

class AFExtendedTwiceImpl: public AFExtendedTwiceStubDefault {

public:
    AFExtendedTwiceImpl();
    virtual ~AFExtendedTwiceImpl();
    void doSomething(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x,
        doSomethingReply_t _reply);
    void doSomethingSpecial(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _x,
        doSomethingSpecialReply_t _reply);
    void doSomethingExtraSpecial(const std::shared_ptr<CommonAPI::ClientId> _clientId, uint8_t _x,
        doSomethingExtraSpecialReply_t _reply);

private:

};

} /* namespace extended */
} /* namespace advanced */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* AFEXTENDEDSTUB_HPP_ */
