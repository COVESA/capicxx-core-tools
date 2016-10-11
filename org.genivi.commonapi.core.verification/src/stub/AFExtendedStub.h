// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef AFEXTENDEDSTUB_H_
#define AFEXTENDEDSTUB_H_

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

    void doSomething(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x,
        doSomethingReply_t _reply);

private:

};

class AFExtendedOnceImpl: public AFExtendedOnceStubDefault {

public:
    AFExtendedOnceImpl();
    virtual ~AFExtendedOnceImpl();
    void doSomething(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x,
        doSomethingReply_t _reply);
    void doSomethingSpecial(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x,
        doSomethingSpecialReply_t reply);

private:

};

class AFExtendedTwiceImpl: public AFExtendedTwiceStubDefault {

public:
    AFExtendedTwiceImpl();
    virtual ~AFExtendedTwiceImpl();
    void doSomething(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x,
        doSomethingReply_t _reply);
    void doSomethingSpecial(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x,
        doSomethingSpecialReply_t reply);
    void doSomethingExtraSpecial(const std::shared_ptr<CommonAPI::ClientId> clientId, uint8_t x,
        doSomethingExtraSpecialReply_t reply);

private:

};

} /* namespace extended */
} /* namespace advanced */
} /* namespace commonapi */
} /* namespace v1 */

#endif /* AFEXTENDEDSTUB_H_ */
