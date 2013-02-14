/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <CommonAPI/Factory.h>
#include <CommonAPI/tests/EchoStubDefault.h>

#include <iostream>
#include <chrono>
#include <thread>


class EchoStubBenchmark: public CommonAPI::tests::EchoStubDefault {
 public:
	virtual void getTestDataCopy(CommonAPI::tests::Echo::TestData testData, CommonAPI::tests::Echo::TestData& testDataCopy) {
	 	testDataCopy = testData;
 	}

    virtual void getTestDataArrayCopy(CommonAPI::tests::Echo::TestDataArray testDataArray, CommonAPI::tests::Echo::TestDataArray& testDataArrayCopy) {
    	testDataArrayCopy = testDataArray;
    }
};


int main(void) {
    std::shared_ptr<CommonAPI::Factory> factory = CommonAPI::Runtime::load()->createFactory();
    std::string serviceAddress = "local:comommonapi.tests.EchoService:commonapi.tests.Echo";
    auto echoStub = std::make_shared<EchoStubBenchmark>();

    bool success = factory->registerService(echoStub, serviceAddress);
    if (!success) {
    	std::cerr << "Unable to register service!\n";
    	return -1;
    }

    std::chrono::seconds sleepDuration(5);
    while(true) {
    	std::this_thread::sleep_for(sleepDuration);
    }

    return 0;
}
