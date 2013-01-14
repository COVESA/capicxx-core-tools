/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef TEST_INTERFACE_PROXY_H_
#define TEST_INTERFACE_PROXY_H_

#include "commonapi-mock.h"

#include <commonapi/tests/TestInterfaceProxy.h>


class MockTestPredefinedTypeAttributeAttribute:
		public commonapi::tests::TestInterfaceProxyBase::TestPredefinedTypeAttributeAttribute {
// TODO
};

class MockTestDerivedStructAttributeAttribute:
		public commonapi::tests::TestInterfaceProxyBase::TestDerivedStructAttributeAttribute {
// TODO
};

class MockTestDerivedArrayAttributeAttribute:
		public commonapi::tests::TestInterfaceProxyBase::TestDerivedArrayAttributeAttribute {
// TODO
};

class MockTestPredefinedTypeBroadcastEvent:
		public commonapi::tests::TestInterfaceProxyBase::TestPredefinedTypeBroadcastEvent {
// TODO
};



class MockTestInterfaceProxyBase: public MockProxy, public commonapi::tests::TestInterfaceProxyBase {
 public:
	MOCK_METHOD0(getTestPredefinedTypeAttributeAttribute, TestPredefinedTypeAttributeAttribute&());
	MOCK_METHOD0(getTestDerivedStructAttributeAttribute, TestDerivedStructAttributeAttribute&());
	MOCK_METHOD0(getTestDerivedArrayAttributeAttribute, TestDerivedArrayAttributeAttribute&());
	MOCK_METHOD0(getTestPredefinedTypeBroadcastEvent, TestPredefinedTypeBroadcastEvent&());

	MOCK_METHOD3(testVoidPredefinedTypeMethod,
		         void(const uint32_t& uint32Value,
                      const std::string& stringValue,
                      CommonAPI::CallStatus& callStatus));

	MOCK_METHOD3_WITH_FUTURE_RETURN(testVoidPredefinedTypeMethodAsync,
			CommonAPI::CallStatus (const uint32_t& uint32Value,
								   const std::string& stringValue,
								   TestVoidPredefinedTypeMethodAsyncCallback callback));

	MOCK_METHOD5(testPredefinedTypeMethod,
			void(const uint32_t& uint32InValue,
				 const std::string& stringInValue,
				 CommonAPI::CallStatus& callStatus,
				 uint32_t& uint32OutValue,
				 std::string& stringOutValue));

	MOCK_METHOD3_WITH_FUTURE_RETURN(testPredefinedTypeMethodAsync,
			CommonAPI::CallStatus (const uint32_t& uint32InValue,
								   const std::string& stringInValue,
								   TestPredefinedTypeMethodAsyncCallback callback));

	MOCK_METHOD3(testVoidDerivedTypeMethod,
			void(const commonapi::tests::DerivedTypeCollection::TestEnumExtended2& testEnumExtended2Value,
				 const commonapi::tests::DerivedTypeCollection::TestMap& testMapValue,
				 CommonAPI::CallStatus& callStatus));


	MOCK_METHOD3_WITH_FUTURE_RETURN(testVoidDerivedTypeMethodAsync,
			CommonAPI::CallStatus (const commonapi::tests::DerivedTypeCollection::TestEnumExtended2& testEnumExtended2Value,
								   const commonapi::tests::DerivedTypeCollection::TestMap& testMapValue,
		    		               TestVoidDerivedTypeMethodAsyncCallback callback));

	MOCK_METHOD5(testDerivedTypeMethod,
			void(const commonapi::tests::DerivedTypeCollection::TestEnumExtended2& testEnumExtended2InValue,
				 const commonapi::tests::DerivedTypeCollection::TestMap& testMapInValue,
				 CommonAPI::CallStatus& callStatus,
				 commonapi::tests::DerivedTypeCollection::TestEnumExtended2& testEnumExtended2OutValue,
				 commonapi::tests::DerivedTypeCollection::TestMap& testMapOutValue));

	MOCK_METHOD3_WITH_FUTURE_RETURN(testDerivedTypeMethodAsync,
			CommonAPI::CallStatus (const commonapi::tests::DerivedTypeCollection::TestEnumExtended2& testEnumExtended2InValue,
								   const commonapi::tests::DerivedTypeCollection::TestMap& testMapInValue,
								   TestDerivedTypeMethodAsyncCallback callback));
};

#endif /* TEST_INTERFACE_PROXY_H_ */
