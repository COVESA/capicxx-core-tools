/* Copyright (C) 2014, 2015 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef E04PHONEBOOKSTUBIMPL_H_
#define E04PHONEBOOKSTUBIMPL_H_

#include <CommonAPI/CommonAPI.hpp>
#include <commonapi/examples/E04PhoneBookStubDefault.hpp>

using namespace commonapi::examples;

class E04PhoneBookStubImpl: public commonapi::examples::E04PhoneBookStubDefault {

public:
    E04PhoneBookStubImpl();
    virtual ~E04PhoneBookStubImpl();

    std::vector<E04PhoneBook::phoneBookStruct> createTestPhoneBook();
    bool contains(std::vector<E04PhoneBook::phoneBookDataElementEnum>&, const E04PhoneBook::phoneBookDataElementEnum&);
    void onPhoneBookDataSetSelectiveSubscriptionChanged(
                                                        const std::shared_ptr<CommonAPI::ClientId>,
                                                        const CommonAPI::SelectiveBroadcastSubscriptionEvent);

    void setPhoneBookDataFilter(const std::shared_ptr<CommonAPI::ClientId> _client,
    							E04PhoneBook::elementFilterStruct _elementFilter,
								std::vector<E04PhoneBook::contentFilterStruct> _contentFilter,
								setPhoneBookDataFilterReply_t _reply);

private:
    std::unordered_map<std::shared_ptr<CommonAPI::ClientId>, std::vector<E04PhoneBook::phoneBookDataElementMap>> phoneBookClientData;

};

#endif /* E04PHONEBOOKSTUBIMPL_H_ */
