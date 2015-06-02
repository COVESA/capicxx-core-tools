/* Copyright (C) 2015 BMW Group
 * Author: Lutz Bichler (lutz.bichler@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "E07MainloopStubImpl.hpp"

E07MainloopStubImpl::E07MainloopStubImpl() {
	setXAttribute(0);
}

E07MainloopStubImpl::~E07MainloopStubImpl() {
}

void E07MainloopStubImpl::sayHello(const std::shared_ptr<CommonAPI::ClientId> _client,
		std::string _name,
		sayHelloReply_t _reply) {

	std::stringstream messageStream;

    messageStream << "Hello " << _name << "!";
    std::cout << "sayHello('" << _name << "'): '" << messageStream.str() << "'\n";

    _reply(messageStream.str());
}

void E07MainloopStubImpl::incAttrX() {
	int32_t xValue = getXAttribute();
	xValue++;
	setXAttribute((int32_t)xValue);
	std::cout <<  "New counter value = " << xValue << "!" << std::endl;
}
