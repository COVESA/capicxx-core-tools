// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef E04PHONEBOOKSTUBIMPL_HPP_
#define E04PHONEBOOKSTUBIMPL_HPP_

#include <CommonAPI/CommonAPI.hpp>
#include <commonapi/examples/E04PhoneBookStubDefault.hpp>

using namespace commonapi::examples;

class E04PhoneBookStubImpl: public commonapi::examples::E04PhoneBookStubDefault {

public:
    E04PhoneBookStubImpl();
    virtual ~E04PhoneBookStubImpl();

    std::vector<E04PhoneBook::phoneBookStruct> createTestPhoneBook();
    bool contains(std::vector<E04PhoneBook::phoneBookDataElementEnum>&, const E04PhoneBook::phoneBookDataElementEnum&);
    void onPhoneBookDataSetSelectiveSubscriptionChanged(const std::shared_ptr<CommonAPI::ClientId> ,
                                                        const CommonAPI::SelectiveBroadcastSubscriptionEvent);

    void setPhoneBookDataFilter(const std::shared_ptr<CommonAPI::ClientId> &_client,
                                const E04PhoneBook::elementFilterStruct &_elementFilter,
                                const std::vector<E04PhoneBook::contentFilterStruct> &_contentFilter,
                                const setPhoneBookDataFilterReply_t &_reply);

private:
    std::unordered_map<std::shared_ptr<CommonAPI::ClientId>, std::vector<E04PhoneBook::phoneBookDataElementMap>> phoneBookClientData;

};

#endif // E04PHONEBOOKSTUBIMPL_HPP_
