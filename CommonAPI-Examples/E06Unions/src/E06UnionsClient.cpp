/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef WIN32
#include <unistd.h>
#endif 
#include <iostream>

#include <CommonAPI/CommonAPI.hpp>

#include "commonapi/examples/CommonTypes.hpp"
#include "commonapi/examples/E06UnionsProxy.hpp"
#include "typeUtils.hpp"

using namespace commonapi::examples;

struct MyVisitor {

	explicit inline MyVisitor() {
	}

	template<typename... T>
	inline void eval(const CommonAPI::Variant<T...>& v) {
		CommonAPI::ApplyVoidVisitor<MyVisitor, CommonAPI::Variant<T...>, T...>::visit(*this, v);
	}

	void operator()(CommonTypes::MyTypedef val) {

		std::cout << "Received (C) MyTypedef with value " << (int)val << std::endl;
	}

	void operator()(CommonTypes::MyEnum val) {

		std::cout << "Received (C) MyEnum with value " << (int)val << std::endl;
	}

	void operator()(uint8_t val) {

		std::cout << "Received (C) uint8_t with value " << (int)val << std::endl;
	}

	void operator()(std::string val) {

		std::cout << "Received (C) string " << val << std::endl;
	}

	template<typename T>
	void operator()(const T&) {

		std::cout << "Received (C) change message with unknown type." << std::endl;
	}

	void operator()() {

		std::cout << "NOOP." << std::endl;
	}

};

void evalA (const CommonTypes::SettingsUnion& v) {

	if ( v.isType<CommonTypes::MyTypedef>() ) {

		std::cout << "Received (A) MyTypedef with value " << v.get<CommonTypes::MyTypedef>() << " at index " << (int)v.getValueType() << std::endl;

	} else if ( v.isType<CommonTypes::MyEnum>() ) {

		std::cout << "Received (A) MyEnum with value " << (int) (v.get<CommonTypes::MyEnum>()) << " at index " << (int)v.getValueType() << std::endl;

	} else if ( v.isType<uint8_t>() ) {

		std::cout << "Received (A) uint8_t with value " << (int) (v.get<uint8_t>()) << " at index " << (int)v.getValueType() << std::endl;

	} else if ( v.isType<std::string>() ) {

		std::cout << "Received (A) string " << v.get<std::string>() << " at index " << (int)v.getValueType() << std::endl;

	} else {

		std::cout << "Received (A) change message with unknown type." << std::endl;
	}
}

template <typename T1, typename... T>
void evalB (const CommonAPI::Variant<T1, T...>& v) {

	switch (v.getValueType()) {

	case typeIdOf<CommonTypes::MyTypedef, T1, T...>::value:

		std::cout << "Received (B) MyTypedef with value " << (int)(v.template get<CommonTypes::MyTypedef>()) << " at index " << (int)v.getValueType() << std::endl;
		break;

	case typeIdOf<CommonTypes::MyEnum, T1, T...>::value:

		std::cout << "Received (B) MyEnum with value " << (int)(v.template get<CommonTypes::MyEnum>()) << " at index " << (int)v.getValueType() << std::endl;
		break;

	case typeIdOf<uint8_t, T1, T...>::value:

		std::cout << "Received (B) uint8_t with value " << (int)(v.template get<uint8_t>()) << " at index " << (int)v.getValueType() << std::endl;
		break;

	case typeIdOf<std::string, T1, T...>::value:

		std::cout << "Received (B) string " << v.template get<std::string>() << " at index " << (int)v.getValueType() << std::endl;
		break;

	default:

		std::cout << "Received (B) change message with unknown type." << std::endl;
		break;
	}
}

void evalC(const CommonTypes::SettingsUnion& v) {

	MyVisitor visitor;
	visitor.eval(v);
}

void recv_msg(const CommonTypes::SettingsUnion& v) {

	evalA(v);
	evalB(v);
	evalC(v);
}

void recv_msg1(std::shared_ptr<CommonTypes::SettingsStruct> x) {

	if ( std::shared_ptr<CommonTypes::SettingsStructMyTypedef> sp = std::dynamic_pointer_cast<CommonTypes::SettingsStructMyTypedef>(x) ) {

		std::cout << "Received (D) MyTypedef with value " << (int)sp->getId() << std::endl;

	} else if ( std::shared_ptr<CommonTypes::SettingsStructMyEnum> sp = std::dynamic_pointer_cast<CommonTypes::SettingsStructMyEnum>(x) ) {

		std::cout << "Received (D) MyEnum with value " << (int)sp->getStatus() << std::endl;

	} else if ( std::shared_ptr<CommonTypes::SettingsStructUInt8> sp = std::dynamic_pointer_cast<CommonTypes::SettingsStructUInt8>(x) ) {

		std::cout << "Received (D) uint8_t with value " << (int)sp->getChannel() << std::endl;

	} else if ( std::shared_ptr<CommonTypes::SettingsStructString> sp = std::dynamic_pointer_cast<CommonTypes::SettingsStructString>(x) ) {

		std::cout << "Received (D) string " << sp->getName() << std::endl;

	} else {

		std::cout << "Received (D) change message with unknown type." << std::endl;
	}
}


int main() {
	CommonAPI::Runtime::setProperty("LogContext", "E06C");
	CommonAPI::Runtime::setProperty("LibraryBase", "E06Unions");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

    const std::string &domain = "local";
	const std::string &instance = "commonapi.examples.Unions";
	std::string connection = "client-sample";

	std::shared_ptr<E06UnionsProxy<>> myProxy = runtime->buildProxy<E06UnionsProxy>(domain, instance, connection);

    while (!myProxy->isAvailable()) {
        usleep(10);
    }

	std::function<void (CommonTypes::SettingsUnion)> f = recv_msg;
	myProxy->getUAttribute().getChangedEvent().subscribe(f);

	std::function<void (std::shared_ptr<CommonTypes::SettingsStruct>)> f1 = recv_msg1;
	myProxy->getXAttribute().getChangedEvent().subscribe(f1);

    while (true) {
        usleep(10);
    }

    return 0;
}
