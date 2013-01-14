/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "test-interface-proxy.h"

#include <gtest/gtest.h>

#include <limits>

using namespace ::testing;

ACTION_P(SyncMethodCopyInValues2, cs) { arg3 = arg0; arg4 = arg1; arg2 = cs; }
ACTION_P(ASyncMethodReturnInValues2, cs) { arg2(cs, arg0, arg1); return cs; }

TEST(TestInterfaceProxy, VoidPredefinedTypeMethodWorks) {
	MockTestInterfaceProxyBase mockTestInterfaceProxyBase;

	EXPECT_CALL(mockTestInterfaceProxyBase, testVoidPredefinedTypeMethod(1234567, "Hello World!", _))
		.WillOnce(SetArgReferee<2>(CommonAPI::CallStatus::SUCCESS));

	CommonAPI::CallStatus callStatusOutValue;
	mockTestInterfaceProxyBase.testVoidPredefinedTypeMethod(1234567, "Hello World!", callStatusOutValue);

	ASSERT_EQ(callStatusOutValue, CommonAPI::CallStatus::SUCCESS);
}

TEST(TestInterfaceProxy, VoidPredefinedTypeMethodAsyncWorks) {
	MockTestInterfaceProxyBase mockTestInterfaceProxyBase;

	EXPECT_CALL(mockTestInterfaceProxyBase, testVoidPredefinedTypeMethodAsyncWithoutStdFuture(1234567, "Hello World!", _))
		.WillOnce(DoAll(
				InvokeArgument<2>(CommonAPI::CallStatus::SUCCESS),
				Return(CommonAPI::CallStatus::SUCCESS)));

	CommonAPI::CallStatus callStatusOutValue;

	std::future<CommonAPI::CallStatus> asyncFuture = mockTestInterfaceProxyBase.testVoidPredefinedTypeMethodAsync(
			1234567,
			"Hello World!",
			[&] (const CommonAPI::CallStatus& callStatusCallback) { callStatusOutValue = callStatusCallback; });

	CommonAPI::CallStatus callStatusFuture = asyncFuture.get();

	ASSERT_EQ(callStatusFuture, CommonAPI::CallStatus::SUCCESS);
	ASSERT_EQ(callStatusOutValue, callStatusFuture);
}

TEST(TestInterfaceProxy, PredefinedTypeMethodWorks) {
	MockTestInterfaceProxyBase mockTestInterfaceProxyBase;

	EXPECT_CALL(mockTestInterfaceProxyBase, testPredefinedTypeMethod(_, _, _, _, _))
		.WillOnce(SyncMethodCopyInValues2(CommonAPI::CallStatus::SUCCESS));

	const uint32_t uint32InValue = std::numeric_limits<uint32_t>::max();
	const std::string& stringInValue = "Hello World!";

	CommonAPI::CallStatus callStatusOutValue;
	uint32_t uint32OutValue;
	std::string stringOutValue;

	mockTestInterfaceProxyBase.testPredefinedTypeMethod(
			uint32InValue,
			stringInValue,
			callStatusOutValue,
			uint32OutValue,
			stringOutValue);

	ASSERT_EQ(callStatusOutValue, CommonAPI::CallStatus::SUCCESS);
	ASSERT_EQ(uint32OutValue, uint32InValue);
	ASSERT_EQ(stringOutValue, stringInValue);
}

TEST(TestInterfaceProxy, PredefinedTypeMethodAsyncWorks) {
	MockTestInterfaceProxyBase mockTestInterfaceProxyBase;

	EXPECT_CALL(mockTestInterfaceProxyBase, testPredefinedTypeMethodAsyncWithoutStdFuture(_, _, _))
		.WillOnce(ASyncMethodReturnInValues2(CommonAPI::CallStatus::SUCCESS));

	const uint32_t uint32InValue = std::numeric_limits<uint32_t>::max();
	const std::string& stringInValue = "Hello World!";

	CommonAPI::CallStatus callStatusOutValue;
	uint32_t uint32OutValue;
	std::string stringOutValue;

	std::future<CommonAPI::CallStatus> asyncFuture = mockTestInterfaceProxyBase.testPredefinedTypeMethodAsync(
			uint32InValue,
			stringInValue,
			[&] (const CommonAPI::CallStatus& callStatusCallback, const uint32_t& uint32InValueCallback, const std::string& stringInValueCallback) {
		callStatusOutValue = callStatusCallback;
		uint32OutValue = uint32InValueCallback;
		stringOutValue = stringInValueCallback;
	});

	CommonAPI::CallStatus callStatusFuture = asyncFuture.get();

	ASSERT_EQ(callStatusFuture, CommonAPI::CallStatus::SUCCESS);
	ASSERT_EQ(callStatusOutValue, callStatusFuture);
	ASSERT_EQ(uint32OutValue, uint32InValue);
	ASSERT_EQ(stringOutValue, stringInValue);
}

TEST(TestInterfaceProxy, VoidDerivedTypeMethodWorks) {
	MockTestInterfaceProxyBase mockTestInterfaceProxyBase;

	EXPECT_CALL(mockTestInterfaceProxyBase, testVoidDerivedTypeMethod(commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK, _, _))
		.WillOnce(SetArgReferee<2>(CommonAPI::CallStatus::SUCCESS));

	commonapi::tests::DerivedTypeCollection::TestMap testMap;
	CommonAPI::CallStatus callStatusReturn;
	mockTestInterfaceProxyBase.testVoidDerivedTypeMethod(commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK, testMap, callStatusReturn);

	ASSERT_EQ(callStatusReturn, CommonAPI::CallStatus::SUCCESS);
}

TEST(TestInterfaceProxy, VoidDerivedTypeMethodAsyncWorks) {
	MockTestInterfaceProxyBase mockTestInterfaceProxyBase;

	EXPECT_CALL(mockTestInterfaceProxyBase, testVoidDerivedTypeMethodAsyncWithoutStdFuture(
			commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK, _, _))
		.WillOnce(DoAll(
				InvokeArgument<2>(CommonAPI::CallStatus::SUCCESS),
				Return(CommonAPI::CallStatus::SUCCESS)));

	commonapi::tests::DerivedTypeCollection::TestMap testMap;
	CommonAPI::CallStatus callStatusOutValue;

	std::future<CommonAPI::CallStatus> asyncFuture = mockTestInterfaceProxyBase.testVoidDerivedTypeMethodAsync(
			commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK,
			testMap,
			[&] (const CommonAPI::CallStatus& callStatusCallback) { callStatusOutValue = callStatusCallback; });

	CommonAPI::CallStatus callStatusFuture = asyncFuture.get();

	ASSERT_EQ(callStatusFuture, CommonAPI::CallStatus::SUCCESS);
	ASSERT_EQ(callStatusOutValue, callStatusFuture);
}

TEST(TestInterfaceProxy, DerivedTypeMethodWorks) {
	MockTestInterfaceProxyBase mockTestInterfaceProxyBase;

	EXPECT_CALL(mockTestInterfaceProxyBase, testDerivedTypeMethod(_, _, _, _, _))
		.WillOnce(SyncMethodCopyInValues2(CommonAPI::CallStatus::SUCCESS));

	CommonAPI::CallStatus callStatusOutValue;
	commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testEnumExtended2OutValue;
	commonapi::tests::DerivedTypeCollection::TestMap testMapOutValue;

	mockTestInterfaceProxyBase.testDerivedTypeMethod(
			commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK,
			{ { 123, { { "First Element", 12345 }, { "Second Element", 6789 } } },
			  { 456, { { "Third Element", 333 }, { "Fourth Element", 4444 }, { "Fifth Element", 55555 } } },
			  { 7,   { } } },
			callStatusOutValue,
			testEnumExtended2OutValue,
			testMapOutValue);

	ASSERT_EQ(callStatusOutValue, CommonAPI::CallStatus::SUCCESS);
	ASSERT_EQ(testEnumExtended2OutValue, commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK);
	ASSERT_FALSE(testMapOutValue.empty());
	ASSERT_EQ(testMapOutValue.size(), 3);
	ASSERT_NE(testMapOutValue.find(123), testMapOutValue.end());
	ASSERT_NE(testMapOutValue.find(456), testMapOutValue.end());
	ASSERT_NE(testMapOutValue.find(7), testMapOutValue.end());
	ASSERT_FALSE(testMapOutValue[123].empty());
	ASSERT_EQ(testMapOutValue[123].size(), 2);
	ASSERT_EQ(testMapOutValue[123][0].testString, "First Element");
	ASSERT_EQ(testMapOutValue[123][0].uintValue, 12345);
	ASSERT_EQ(testMapOutValue[123][1].testString, "Second Element");
	ASSERT_EQ(testMapOutValue[123][1].uintValue, 6789);
	ASSERT_FALSE(testMapOutValue[456].empty());
	ASSERT_EQ(testMapOutValue[456].size(), 3);
	ASSERT_EQ(testMapOutValue[456][0].testString, "Third Element");
	ASSERT_EQ(testMapOutValue[456][0].uintValue, 333);
	ASSERT_EQ(testMapOutValue[456][1].testString, "Fourth Element");
	ASSERT_EQ(testMapOutValue[456][1].uintValue, 4444);
	ASSERT_EQ(testMapOutValue[456][2].testString, "Fifth Element");
	ASSERT_EQ(testMapOutValue[456][2].uintValue, 55555);
	ASSERT_TRUE(testMapOutValue[7].empty());
	ASSERT_EQ(testMapOutValue[7].size(), 0);
}

TEST(TestInterfaceProxy, DerivedTypeMethodAsyncWorks) {
	MockTestInterfaceProxyBase mockTestInterfaceProxyBase;

	EXPECT_CALL(mockTestInterfaceProxyBase, testDerivedTypeMethodAsyncWithoutStdFuture(_, _, _))
		.WillOnce(ASyncMethodReturnInValues2(CommonAPI::CallStatus::SUCCESS));

	CommonAPI::CallStatus callStatusOutValue;
	commonapi::tests::DerivedTypeCollection::TestEnumExtended2 testEnumExtended2OutValue;
	commonapi::tests::DerivedTypeCollection::TestMap testMapOutValue;

	std::future<CommonAPI::CallStatus> asyncFuture = mockTestInterfaceProxyBase.testDerivedTypeMethodAsync(
			commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK,
			{ { 123, { { "First Element", 12345 }, { "Second Element", 6789 } } },
			  { 456, { { "Third Element", 333 }, { "Fourth Element", 4444 }, { "Fifth Element", 55555 } } },
			  { 7,   { } } },
			[&] (const CommonAPI::CallStatus& callStatusCallback,
				 const commonapi::tests::DerivedTypeCollection::TestEnumExtended2& testEnumExtended2Callback,
				 const commonapi::tests::DerivedTypeCollection::TestMap& testMapCallback) {
		callStatusOutValue = callStatusCallback;
		testEnumExtended2OutValue = testEnumExtended2Callback;
		testMapOutValue = testMapCallback;
	});

	CommonAPI::CallStatus callStatusFuture = asyncFuture.get();

	ASSERT_EQ(callStatusFuture, CommonAPI::CallStatus::SUCCESS);
	ASSERT_EQ(callStatusOutValue, callStatusFuture);
	ASSERT_EQ(testEnumExtended2OutValue, commonapi::tests::DerivedTypeCollection::TestEnumExtended2::E_OK);
	ASSERT_FALSE(testMapOutValue.empty());
	ASSERT_EQ(testMapOutValue.size(), 3);
	ASSERT_NE(testMapOutValue.find(123), testMapOutValue.end());
	ASSERT_NE(testMapOutValue.find(456), testMapOutValue.end());
	ASSERT_NE(testMapOutValue.find(7), testMapOutValue.end());
	ASSERT_FALSE(testMapOutValue[123].empty());
	ASSERT_EQ(testMapOutValue[123].size(), 2);
	ASSERT_EQ(testMapOutValue[123][0].testString, "First Element");
	ASSERT_EQ(testMapOutValue[123][0].uintValue, 12345);
	ASSERT_EQ(testMapOutValue[123][1].testString, "Second Element");
	ASSERT_EQ(testMapOutValue[123][1].uintValue, 6789);
	ASSERT_FALSE(testMapOutValue[456].empty());
	ASSERT_EQ(testMapOutValue[456].size(), 3);
	ASSERT_EQ(testMapOutValue[456][0].testString, "Third Element");
	ASSERT_EQ(testMapOutValue[456][0].uintValue, 333);
	ASSERT_EQ(testMapOutValue[456][1].testString, "Fourth Element");
	ASSERT_EQ(testMapOutValue[456][1].uintValue, 4444);
	ASSERT_EQ(testMapOutValue[456][2].testString, "Fifth Element");
	ASSERT_EQ(testMapOutValue[456][2].uintValue, 55555);
	ASSERT_TRUE(testMapOutValue[7].empty());
	ASSERT_EQ(testMapOutValue[7].size(), 0);
}

int main(int argc, char** argv) {
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}
