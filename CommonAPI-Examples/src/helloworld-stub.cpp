/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <commonapi/examples/HelloWorldInterfaceStubDefault.h>
#include <CommonAPI/CommonAPI.h>
#include <iostream>
#include <sstream>
#include <thread>


class MyHelloWorldStub: public commonapi::examples::HelloWorldInterfaceStubDefault {
 public:
	virtual void sayHello(std::string name, std::string& message) {
		std::stringstream messageStream;

		messageStream <<  "Hello " << name << "!";
		message = messageStream.str();

		std::cout << "sayHello('" << name << "'): '" << message << "'\n";
	}
};


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


	auto servicePublisher = runtime->getServicePublisher();
	if (!servicePublisher) {
		std::cerr << "Error: Unable to load service publisher!\n";
		return -1;
	}

	std::cout << "Service publisher loaded!\n";


	auto helloWorldStub = std::make_shared<MyHelloWorldStub>();
	const std::string& commonApiAddress = "local:commonapi.examples.HelloWorld:commonapi.examples.HelloWorld";
	const bool isStubRegistrationSuccessful = servicePublisher->registerService(helloWorldStub, commonApiAddress, factory);
	if (!isStubRegistrationSuccessful) {
		std::cerr << "Error: Unable to register service!\n";
		return -1;
	}

	std::cout << "Service registration successful!\n";


	while(true) {
		std::cout << "Waiting for calls... (Abort with CTRL+C)\n";
		std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    return 0;
}
