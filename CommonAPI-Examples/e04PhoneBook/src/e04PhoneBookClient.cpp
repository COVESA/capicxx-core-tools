/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <iostream>

#include <CommonAPI/CommonAPI.h>
#include <commonapi/examples/E04PhoneBook.h>
#include <commonapi/examples/E04PhoneBookProxy.h>

using namespace commonapi::examples;

// Utility functions only for printing results to the console

std::string phoneNumberType2String(E04PhoneBook::phoneNumberEnum phoneNumberType) {

    switch (static_cast<int32_t>(phoneNumberType)) {
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

        std::cout << "Name: " << myIterator->name << std::endl;
        std::cout << "Forename: " << myIterator->forename << std::endl;
        std::cout << "Organisation: " << myIterator->organisation << std::endl;
        std::cout << "Address: " << myIterator->address << std::endl;
        std::cout << "EMail: " << myIterator->email << std::endl;

        for (E04PhoneBook::phoneNumberMap::const_iterator myPhoneNumberIterator = myIterator->phoneNumber.begin();
                        myPhoneNumberIterator != myIterator->phoneNumber.end();
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
                                    > (it1->second))->content;
                    std::cout << "Name = " << name << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::FORENAME: {
                    std::string forename = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > (it1->second))->content;
                    std::cout << "Forename = " << forename << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::ORGANISATION: {
                    std::string organisation = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > ((*it0).at(E04PhoneBook::phoneBookDataElementEnum::ORGANISATION)))->content;
                    std::cout << "Organisation = " << organisation << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::ADDRESS: {
                    std::string address = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > ((*it0).at(E04PhoneBook::phoneBookDataElementEnum::ADDRESS)))->content;
                    std::cout << "Address = " << address << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::EMAIL: {
                    std::string email = (std::static_pointer_cast < E04PhoneBook::phoneBookDataElementString
                                    > ((*it0).at(E04PhoneBook::phoneBookDataElementEnum::EMAIL)))->content;
                    std::cout << "EMail = " << email << std::endl;
                }
                break;

                case E04PhoneBook::phoneBookDataElementEnum::PHONENUMBER: {
                    E04PhoneBook::phoneNumberMap phoneNumber = (std::static_pointer_cast
                                    < E04PhoneBook::phoneBookDataElementPhoneNumber
                                    > ((*it0).at(E04PhoneBook::phoneBookDataElementEnum::PHONENUMBER)))->content;
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
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::load();

    std::shared_ptr<CommonAPI::Factory> factoryA = runtime->createFactory();
    std::shared_ptr<CommonAPI::Factory> factoryB = runtime->createFactory();

    const std::string& serviceAddress = "local:commonapi.examples.PhoneBook:commonapi.examples.PhoneBook";
    std::shared_ptr < E04PhoneBookProxyDefault > myProxyA = factoryA->buildProxy < E04PhoneBookProxy > (serviceAddress);
    while (!myProxyA->isAvailable()) {
        usleep(10);
    }
    std::shared_ptr < E04PhoneBookProxyDefault > myProxyB = factoryB->buildProxy < E04PhoneBookProxy > (serviceAddress);
    while (!myProxyB->isAvailable()) {
        usleep(10);
    }

    // Subscribe A to broadcast
    myProxyA->getPhoneBookDataSetSelectiveEvent().subscribe(
                    [&](const std::vector<E04PhoneBook::phoneBookDataElementMap>& phoneBookDataSet) {
                        printFilterResult(phoneBookDataSet, "A");
                    });

    // Subscribe B to broadcast
    myProxyB->getPhoneBookDataSetSelectiveEvent().subscribe(
                    [&](const std::vector<E04PhoneBook::phoneBookDataElementMap>& phoneBookDataSet) {
                        printFilterResult(phoneBookDataSet, "B");
                    });

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

    std::cout << "Call setPhoneBookDataFilter B ..." << std::endl;
    E04PhoneBook::elementFilterStruct lElementFilterB = {true, false, false, false, false, true};
    std::vector<E04PhoneBook::contentFilterStruct> lContentFilterB = { {E04PhoneBook::phoneBookDataElementEnum::NAME, "*"}};

    myProxyB->setPhoneBookDataFilter(lElementFilterB, lContentFilterB, myCallStatus);
    if (myCallStatus != CommonAPI::CallStatus::SUCCESS)
        std::cerr << "Remote call setPhoneBookDataFilter B failed: " << (int) myCallStatus << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}
