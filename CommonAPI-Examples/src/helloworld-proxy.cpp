/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <commonapi/examples/HelloWorldInterfaceProxy.h>
#include <commonapi/examples/HelloWorldLeafProxy.h>
#include <CommonAPI/CommonAPI.h>
#include <iostream>
#include <future>


int main(int argc, char** argv) {
	CommonAPI::Runtime::LoadState loadState;
	auto runtime = CommonAPI::Runtime::load(loadState);
	if (loadState != CommonAPI::Runtime::LoadState::SUCCESS) {
		std::cerr << "Error: Unable to load runtime!\n";
		return -1;
	}

	std::cout << "Runtime loaded!\n";


	auto factory = runtime->createFactory();
	if (!factory) {
		std::cerr << "Error: Unable to create factory!\n";
		return -1;
	}

	std::cout << "Factory created!\n";


	const std::string commonApiAddress = "local:commonapi.examples.HelloWorld:commonapi.examples.HelloWorld";
	auto helloWorldProxy = factory->buildProxy<commonapi::examples::HelloWorldInterfaceProxy>(commonApiAddress);
	if (!helloWorldProxy) {
		std::cerr << "Error: Unable to build proxy!\n";
		return -1;
	}

	std::cout << "Proxy created!\n";

	std::promise<CommonAPI::AvailabilityStatus> availabilityStatusPromise;
	helloWorldProxy->getProxyStatusEvent().subscribe([&](const CommonAPI::AvailabilityStatus& availabilityStatus) {
		availabilityStatusPromise.set_value(availabilityStatus);
		return CommonAPI::SubscriptionStatus::CANCEL;
	});

	auto availabilityStatusFuture = availabilityStatusPromise.get_future();
	std::cout << "Waiting for proxy availability...\n";
	availabilityStatusFuture.wait();

	if (availabilityStatusFuture.get() != CommonAPI::AvailabilityStatus::AVAILABLE) {
		std::cerr << "Proxy not available!\n";
		return -1;
	}

	std::cout << "Proxy available!\n";


	helloWorldProxy->getSaySomethingSelectiveEvent().subscribe([&](const std::string& message) {
	    std::cout << "Received broadcast message: " << message << "\n";
    });

	const std::string name = "World";
	CommonAPI::CallStatus callStatus;
	std::string helloWorldMessage;

	std::cout << "Sending name: '" << name << "'\n";
	helloWorldProxy->sayHello("World", callStatus, helloWorldMessage);
	if (callStatus != CommonAPI::CallStatus::SUCCESS) {
		std::cerr << "Remote call failed!\n";
		return -1;
	}

	std::cout << "Got message: '" << helloWorldMessage << "'\n";

	const std::string leafInstance = "commonapi.examples.HelloWorld.Leaf";
    CommonAPI::CallStatus callStatusAv;
    CommonAPI::AvailabilityStatus availabilityStatus;
	helloWorldProxy->getProxyManagerHelloWorldLeaf().getInstanceAvailabilityStatus(leafInstance, callStatusAv, availabilityStatus);

	if (callStatusAv == CommonAPI::CallStatus::SUCCESS && availabilityStatus == CommonAPI::AvailabilityStatus::AVAILABLE) {
	    auto helloWorldLeafProxy = helloWorldProxy->getProxyManagerHelloWorldLeaf().buildProxy<commonapi::examples::HelloWorldLeafProxy>(leafInstance);

	    const std::string nameLeaf = "WorldLeaf";
        CommonAPI::CallStatus callStatusLeaf;
        std::string helloWorldLeafMessage;

        std::cout << "Sending name: '" << nameLeaf << "'\n";
        helloWorldLeafProxy->sayHelloLeaf("World", callStatusLeaf, helloWorldLeafMessage);
        if (callStatusLeaf != CommonAPI::CallStatus::SUCCESS) {
            std::cerr << "Remote call failed!\n";
            return -1;
        }

        std::cout << "Got message: '" << helloWorldLeafMessage << "'\n";

	} else {
	    std::cout << "Leaf Proxy not available\n";
	    sleep(5);
	    return -1;
	}

	return 0;
}
