/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "E04PhoneBookStubImpl.h"

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

void E04PhoneBookStubImpl::setPhoneBookDataFilter(const std::shared_ptr<CommonAPI::ClientId> clientId,
                                                  E04PhoneBook::elementFilterStruct elementFilter,
                                                  std::vector<E04PhoneBook::contentFilterStruct> contentFilter) {

    std::shared_ptr < CommonAPI::ClientIdList > clientIdList = getSubscribersForPhoneBookDataSetSelective();
    std::cout << "setPhoneBookDataFilter called from client " << clientId->hashCode() << " of ("
              << clientIdList->size() << ")" << std::endl;

    std::vector < E04PhoneBook::phoneBookDataElementMap > lPhoneBookDataSet;
    phoneBookClientData.erase(clientId);

    std::vector<E04PhoneBook::phoneBookStruct>::const_iterator it0;
    for (it0 = getPhoneBookAttribute().begin(); it0 != getPhoneBookAttribute().end(); it0++) {

        E04PhoneBook::phoneBookDataElementMap lPhoneBookDataElement;

        if (elementFilter.addName) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> name = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            name->content = it0->name;
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::NAME] = name;
        }

        if (elementFilter.addForename) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> forename = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            forename->content = it0->forename;
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::FORENAME] = forename;
        }

        if (elementFilter.addOrganisation) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> organisation = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            organisation->content = it0->organisation;
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::ORGANISATION] = organisation;
        }

        if (elementFilter.addAddress) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> address = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            address->content = it0->address;
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::ADDRESS] = address;
        }

        if (elementFilter.addEmail) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementString> email = std::make_shared<E04PhoneBook::phoneBookDataElementString>();
            email->content = it0->email;
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::EMAIL] = email;
        }

        if (elementFilter.addPhoneNumber) {
            std::shared_ptr<E04PhoneBook::phoneBookDataElementPhoneNumber> phoneNumber = std::make_shared<E04PhoneBook::phoneBookDataElementPhoneNumber>();
            phoneNumber->content = it0->phoneNumber;
            lPhoneBookDataElement[E04PhoneBook::phoneBookDataElementEnum::PHONENUMBER] = phoneNumber;
        }

        lPhoneBookDataSet.push_back(lPhoneBookDataElement);

    }

    phoneBookClientData[clientId] = lPhoneBookDataSet;

    // Send client data
    const std::shared_ptr<CommonAPI::ClientIdList> receivers(new CommonAPI::ClientIdList);
    receivers->insert(clientId);
    std::cout << "firePhoneBookDataSetSelective: " << receivers->size() << " / " << phoneBookClientData[clientId].size()
              << std::endl;
    firePhoneBookDataSetSelective(lPhoneBookDataSet, receivers);
    receivers->erase(clientId);

    std::cout << "setPhoneBookDataFilter end." << std::endl;
}

std::vector<E04PhoneBook::phoneBookStruct> E04PhoneBookStubImpl::createTestPhoneBook() {

    std::vector<E04PhoneBook::phoneBookStruct> lPhoneBook;
    E04PhoneBook::phoneBookStruct lPhoneBookEntry;

    // 1. entry
    lPhoneBookEntry.name = "Gehring";
    lPhoneBookEntry.forename = "Jürgen";
    lPhoneBookEntry.organisation = "BMW";
    lPhoneBookEntry.address = "Max-Diamand-Straße 13, 80788 München";
    lPhoneBookEntry.email = "juergen.gehring@bmw.de";
    lPhoneBookEntry.phoneNumber[E04PhoneBook::phoneNumberEnum::WORK] = "0111/12345-0";
    lPhoneBook.push_back(lPhoneBookEntry);
    lPhoneBookEntry.phoneNumber.clear();

    // 2. entry
    lPhoneBookEntry.name = "Müller";
    lPhoneBookEntry.forename = "Alfred";
    lPhoneBookEntry.organisation = "Audi";
    lPhoneBookEntry.address = "August-Horch-Straße 27, 85055 Ingolstadt";
    lPhoneBookEntry.email = "alfred.mueller@audi.de";
    lPhoneBookEntry.phoneNumber[E04PhoneBook::phoneNumberEnum::MOBILE1] = "0222/23456-0";
    lPhoneBook.push_back(lPhoneBookEntry);
    lPhoneBookEntry.phoneNumber.clear();

    // 3. entry
    lPhoneBookEntry.name = "Maier";
    lPhoneBookEntry.forename = "Hansi";
    lPhoneBookEntry.organisation = "Daimler";
    lPhoneBookEntry.address = "Mercedesstraße 137, 70546 Stuttgart";
    lPhoneBookEntry.email = "hansi.maier@daimler.de";
    lPhoneBookEntry.phoneNumber[E04PhoneBook::phoneNumberEnum::HOME] = "0333/34567-0";
    lPhoneBookEntry.phoneNumber[E04PhoneBook::phoneNumberEnum::WORK] = "0444/34567-1";
    lPhoneBook.push_back(lPhoneBookEntry);
    lPhoneBookEntry.phoneNumber.clear();

    return lPhoneBook;
}
