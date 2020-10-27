// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "E04PhoneBookStubImpl.hpp"

#include <algorithm>


using namespace commonapi::examples;

E04PhoneBookStubImpl::E04PhoneBookStubImpl() {
}

E04PhoneBookStubImpl::~E04PhoneBookStubImpl() {
}

bool E04PhoneBookStubImpl::contains(std::vector<E04PhoneBook::phoneBookDataElementEnum>& vec,
                                    const E04PhoneBook::phoneBookDataElementEnum& el) {

    if (std::find(vec.begin(), vec.end(), el) != vec.end()) {
        return true;
    }
    return false;
}

void E04PhoneBookStubImpl::onPhoneBookDataSetSelectiveSubscriptionChanged(const std::shared_ptr<CommonAPI::ClientId> clientId,
                                                                          const CommonAPI::SelectiveBroadcastSubscriptionEvent event) {

    if (event == CommonAPI::SelectiveBroadcastSubscriptionEvent::SUBSCRIBED) {
        std::cout << "onPhoneBookDataSetSelectiveSubscriptionChanged(SUBSCRIBED) called ("
                  << clientId->hashCode() << ")." << std::endl;

    } else if (event == CommonAPI::SelectiveBroadcastSubscriptionEvent::UNSUBSCRIBED) {
        std::cout << "onPhoneBookDataSetSelectiveSubscriptionChanged(UNSUBSCRIBED) called ("
                  << clientId->hashCode() << "." << std::endl;
    }
}

void E04PhoneBookStubImpl::setPhoneBookDataFilter(const std::shared_ptr<CommonAPI::ClientId> &_client,
                                                  const E04PhoneBook::elementFilterStruct &_elementFilter,
                                                  const std::vector<E04PhoneBook::contentFilterStruct> &_contentFilter,
                                                  const setPhoneBookDataFilterReply_t &_reply) {
    std::shared_ptr < CommonAPI::ClientIdList > clientList = getSubscribersForPhoneBookDataSetSelective();
    std::cout << "setPhoneBookDataFilter called from client " << _client->hashCode() << " of ("
              << clientList->size() << ")" << std::endl;

    std::vector < E04PhoneBook::phoneBookDataElementMap > lPhoneBookDataSet;

    phoneBookClientData.erase(_client);

    std::vector<E04PhoneBook::phoneBookStruct>::const_iterator it0;
    for (it0 = getPhoneBookAttribute().begin(); it0 != getPhoneBookAttribute().end(); it0++) {

        E04PhoneBook::phoneBookDataElementMap lPhoneBookDataElement;

        if (_elementFilter.getAddName()) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> name = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            name->setContent(it0->getName());
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::NAME] = name;
        }

        if (_elementFilter.getAddForename()) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> forename = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            forename->setContent(it0->getForename());
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::FORENAME] = forename;
        }

        if (_elementFilter.getAddOrganisation()) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> organisation = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            organisation->setContent(it0->getOrganisation());
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::ORGANISATION] = organisation;
        }

        if (_elementFilter.getAddAddress()) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> address = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            address->setContent(it0->getAddress());
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::ADDRESS] = address;
        }

        if (_elementFilter.getAddEmail()) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> email = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            email->setContent(it0->getEmail());
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::EMAIL] = email;
        }

        if (_elementFilter.getAddPhoneNumber()) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementPhoneNumber> phoneNumber = std::make_shared<E04PhoneBook::phoneBookDataElementPhoneNumber>();
            phoneNumber->setContent(it0->getPhoneNumber());
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::PHONENUMBER] = phoneNumber;
        }

        lPhoneBookDataSet.push_back(lPhoneBookDataElement);
    }

    phoneBookClientData[_client] = lPhoneBookDataSet;

    // Send client data
    const std::shared_ptr<CommonAPI::ClientIdList> receivers(new CommonAPI::ClientIdList);
    receivers->insert(_client);
    std::cout << "firePhoneBookDataSetSelective: " << receivers->size() << " / " << phoneBookClientData[_client].size() << std::endl;

    firePhoneBookDataSetSelective(lPhoneBookDataSet, receivers);

    receivers->erase(_client);

    std::cout << "setPhoneBookDataFilter end." << std::endl;
    _reply();
}

std::vector<E04PhoneBook::phoneBookStruct> E04PhoneBookStubImpl::createTestPhoneBook() {

    std::vector<E04PhoneBook::phoneBookStruct> lPhoneBook;
    E04PhoneBook::phoneBookStruct lPhoneBookEntry;
    E04PhoneBook::phoneNumberMap lPhoneBookEntryMap;

    // 1. entry
    lPhoneBookEntryMap[E04PhoneBook::phoneNumberEnum::WORK] = "0111/12345-0";

    lPhoneBookEntry.setName("Gehring");
    lPhoneBookEntry.setForename("Jürgen");
    lPhoneBookEntry.setOrganisation("BMW");
    lPhoneBookEntry.setAddress("Max-Diamand-Straße 13, 80788 München");
    lPhoneBookEntry.setEmail("juergen.gehring@bmw.de");
    lPhoneBookEntry.setPhoneNumber(lPhoneBookEntryMap);
    lPhoneBook.push_back(lPhoneBookEntry);

    lPhoneBookEntryMap.clear();

    // 2. entry
    lPhoneBookEntryMap[E04PhoneBook::phoneNumberEnum::MOBILE1] = "0222/23456-0";

    lPhoneBookEntry.setName("Müller");
    lPhoneBookEntry.setForename("Alfred");
    lPhoneBookEntry.setOrganisation("Audi");
    lPhoneBookEntry.setAddress("August-Horch-Straße 27, 85055 Ingolstadt");
    lPhoneBookEntry.setEmail("alfred.mueller@audi.de");
    lPhoneBookEntry.setPhoneNumber(lPhoneBookEntryMap);
    lPhoneBook.push_back(lPhoneBookEntry);

    lPhoneBookEntryMap.clear();

    // 3. entry
    lPhoneBookEntryMap[E04PhoneBook::phoneNumberEnum::HOME] = "0333/34567-0";
    lPhoneBookEntryMap[E04PhoneBook::phoneNumberEnum::WORK] = "0444/34567-1";

    lPhoneBookEntry.setName("Maier");
    lPhoneBookEntry.setForename("Hansi");
    lPhoneBookEntry.setOrganisation("Daimler");
    lPhoneBookEntry.setAddress("Mercedesstraße 137, 70546 Stuttgart");
    lPhoneBookEntry.setEmail("hansi.maier@daimler.de");
    lPhoneBookEntry.setPhoneNumber(lPhoneBookEntryMap);
    lPhoneBook.push_back(lPhoneBookEntry);

    lPhoneBookEntryMap.clear();

    return lPhoneBook;
}
