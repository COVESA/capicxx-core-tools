// Copyright (C) 2014-2019 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef _WIN32
#include <unistd.h>
#endif

#include <map>
#include <iostream>
#include <thread>

#include <CommonAPI/CommonAPI.hpp>
#include <commonapi/examples/E04PhoneBook.hpp>
#include <commonapi/examples/E04PhoneBookProxy.hpp>

using namespace commonapi::examples;

// Utility functions only for printing results to the console

std::string phoneNumberType2String(E04PhoneBook::phoneNumberEnum phoneNumberType) {

    switch (phoneNumberType) {
        case 0:
            return "WORK";
            break;
        case 1:
            return "HOME";
            break;
        case 2:
            return "MOBILE1";
            break;
        case 3:
            return "MOBILE2";
            break;
        default:
            return "";
            break;
    }
}

void printPhoneBook(const std::vector<E04PhoneBook::phoneBookStruct>& myPhoneBook) {
    std::vector<E04PhoneBook::phoneBookStruct>::const_iterator myIterator;

    std::cout << "Actual phoneBook content: " << std::endl;
    for (myIterator = myPhoneBook.begin(); myIterator != myPhoneBook.end(); myIterator++) {

        std::cout << "Name: " << myIterator->getName() << std::endl;
        std::cout << "Forename: " << myIterator->getForename() << std::endl;
        std::cout << "Organisation: " << myIterator->getOrganisation() << std::endl;
        std::cout << "Address: " << myIterator->getAddress() << std::endl;
        std::cout << "EMail: " << myIterator->getEmail() << std::endl;

        for (E04PhoneBook::phoneNumberMap::const_iterator myPhoneNumberIterator = myIterator->getPhoneNumber().begin();
                        myPhoneNumberIterator != myIterator->getPhoneNumber().end();
                        myPhoneNumberIterator++) {
            std::cout << "phoneNumber[" << phoneNumberType2String(myPhoneNumberIterator->first) << "]: ";
            std::cout << myPhoneNumberIterator->second << std::endl;
        }
        std::cout << std::endl;
    }
}

void printFilterResult(const std::vector<E04PhoneBook::phoneBookDataElementMap>& phoneBookDataSet, std::string proxy) {
    std::vector<E04PhoneBook::phoneBookDataElementMap>::const_iterator it0;

    std::cout << "Actual phoneBookDataSet for proxy " << proxy << ": " << std::endl;
    for (it0 = phoneBookDataSet.begin(); it0 != phoneBookDataSet.end(); it0++) {
        E04PhoneBook::phoneBookDataElementMap::const_iterator it1;

        for (it1 = (*it0).begin(); it1 != (*it0).end(); it1++) {
            switch (it1->first) {
                case E04PhoneBook::phoneBookDataElementEnum::NAME: {
                    std::string name = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > (it1->second))->getContent();
                    std::cout << "Name = " << name << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::FORENAME: {
                    std::string forename = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > (it1->second))->getContent();
                    std::cout << "Forename = " << forename << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::ORGANISATION: {
                    std::string organisation = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > ((*it0).at(E04PhoneBook::phoneBookDataElementEnum::ORGANISATION)))->getContent();
                    std::cout << "Organisation = " << organisation << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::ADDRESS: {
                    std::string address = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > ((*it0).at(E04PhoneBook::phoneBookDataElementEnum::ADDRESS)))->getContent();
                    std::cout << "Address = " << address << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::EMAIL: {
                    std::string email = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > ((*it0).at(E04PhoneBook::phoneBookDataElementEnum::EMAIL)))->getContent();
                    std::cout << "EMail = " << email << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::PHONENUMBER: {
                    E04PhoneBook::phoneNumberMap phoneNumber = (std::static_pointer_cast
                                    < E04PhoneBook::phoneBookDataElementPhoneNumber
                                    > ((*it0).at(E04PhoneBook::phoneBookDataElementEnum::PHONENUMBER)))->getContent();
                    for (E04PhoneBook::phoneNumberMap::iterator myPhoneNumberIterator = phoneNumber.begin();
                                    myPhoneNumberIterator != phoneNumber.end();
                                    myPhoneNumberIterator++) {
                        std::cout << "PhoneNumber[" << phoneNumberType2String(myPhoneNumberIterator->first) << "] = ";
                        std::cout << myPhoneNumberIterator->second << std::endl;
                    }
                }
                break;

                default: {
                    std::cout << "No result." << std::endl;
                }
                break;
            }
        }
    }
}

int main() {
    CommonAPI::Runtime::setProperty("LogContext", "E04C");
    CommonAPI::Runtime::setProperty("LogApplication", "E04C");
    CommonAPI::Runtime::setProperty("LibraryBase", "E04PhoneBook");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    const std::string &domain = "local";
    const std::string &instance = "commonapi.examples.PhoneBook";
    const std::string &connection = "client-sample";

    std::shared_ptr < E04PhoneBookProxy<> > myProxyA = runtime->buildProxy < E04PhoneBookProxy > (domain, instance, connection);
    while (!myProxyA->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    std::cout << "Service for Proxy A is available!" << std::endl;

    const CommonAPI::ConnectionId_t otherConnection = "other-client-sample";
    std::shared_ptr < E04PhoneBookProxy<> > myProxyB = runtime->buildProxy < E04PhoneBookProxy > (domain, instance, otherConnection);
    while (!myProxyB->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    std::cout << "Service for Proxy B is available!" << std::endl;

    // Subscribe A to broadcast
    myProxyA->getPhoneBookDataSetSelectiveEvent().subscribe(
                    [&](const std::vector<E04PhoneBook::phoneBookDataElementMap>& phoneBookDataSet) {
                        std::cout << "-- A --" << std::endl;
                        printFilterResult(phoneBookDataSet, "A");
                        std::cout << "-------" << std::endl;
                    });

    std::cout << "Subscribed A" << std::endl;

    // Subscribe B to broadcast
    myProxyB->getPhoneBookDataSetSelectiveEvent().subscribe(
                    [&](const std::vector<E04PhoneBook::phoneBookDataElementMap>& phoneBookDataSet) {
                        std::cout << "-- B --" << std::endl;
                        printFilterResult(phoneBookDataSet, "B");
                        std::cout << "-------" << std::endl;
                    });

    std::cout << "Subscribed B" << std::endl;

    // Get actual phoneBook from service
    CommonAPI::CallStatus myCallStatus;
    std::vector<E04PhoneBook::phoneBookStruct> myValue;

    myProxyA->getPhoneBookAttribute().getValue(myCallStatus, myValue);
    if (myCallStatus != CommonAPI::CallStatus::SUCCESS)
        std::cerr << "Remote call getPhoneBookAttribute failed!\n";
    else
        printPhoneBook (myValue);

    // Synchronous call setPhoneBookDataFilter
    std::cout << "Call setPhoneBookDataFilter A ..." << std::endl;
    E04PhoneBook::elementFilterStruct lElementFilterA = {true, true, false, false, false, false};
    std::vector<E04PhoneBook::contentFilterStruct> lContentFilterA = { {E04PhoneBook::phoneBookDataElementEnum::NAME, "*"}};

    myProxyA->setPhoneBookDataFilter(lElementFilterA, lContentFilterA, myCallStatus);
    if (myCallStatus != CommonAPI::CallStatus::SUCCESS)
        std::cerr << "Remote call setPhoneBookDataFilter A failed: " << (int) myCallStatus << std::endl;
    else
        std::cout << "Remote call setPhoneBookDataFilter A succeeded." << std::endl;

    std::cout << "Call setPhoneBookDataFilter B ..." << std::endl;
    E04PhoneBook::elementFilterStruct lElementFilterB = {true, false, false, false, false, true};
    std::vector<E04PhoneBook::contentFilterStruct> lContentFilterB = { {E04PhoneBook::phoneBookDataElementEnum::NAME, "*"}};

    myProxyB->setPhoneBookDataFilter(lElementFilterB, lContentFilterB, myCallStatus);
    if (myCallStatus != CommonAPI::CallStatus::SUCCESS)
        std::cerr << "Remote call setPhoneBookDataFilter B failed: " << (int) myCallStatus << std::endl;
    else
        std::cout << "Remote call setPhoneBookDataFilter B succeeded." << std::endl;

    while (true) {
        std::cout << "Now I am going to sleep for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}
