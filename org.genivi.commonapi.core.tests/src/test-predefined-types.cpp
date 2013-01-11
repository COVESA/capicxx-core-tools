/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <commonapi/tests/PredefinedTypeCollection.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <limits>

TEST(PredefinedDataTypes, HaveCorrectSize) {
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestUInt8), 1);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestUInt16), 2);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestUInt32), 4);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestUInt64), 8);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestInt8), 1);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestInt16), 2);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestInt32), 4);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestInt64), 8);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestDouble), 8);
	ASSERT_EQ(sizeof(::commonapi::tests::PredefinedTypeCollection::TestFloat), 4);
}

TEST(PredefinedDataTypes, HaveCorrectTypes) {
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestUInt8), typeid(uint8_t));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestUInt16), typeid(uint16_t));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestUInt32), typeid(uint32_t));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestUInt64), typeid(uint64_t));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestInt8), typeid(int8_t));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestInt16), typeid(int16_t));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestInt32), typeid(int32_t));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestInt64), typeid(int64_t));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestDouble), typeid(double));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestFloat), typeid(float));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestString), typeid(std::string));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestBoolean), typeid(bool));
	ASSERT_EQ(typeid(::commonapi::tests::PredefinedTypeCollection::TestByteBuffer), typeid(CommonAPI::ByteBuffer));
}

TEST(PredefinedDataTypes, StoreCorrectValues) {
	::commonapi::tests::PredefinedTypeCollection::TestUInt8 testUInt8 = std::numeric_limits<uint8_t>::max();
	ASSERT_EQ(testUInt8, std::numeric_limits<uint8_t>::max());

	::commonapi::tests::PredefinedTypeCollection::TestUInt16 testUInt16 = std::numeric_limits<uint16_t>::max();
	ASSERT_EQ(testUInt16, std::numeric_limits<uint16_t>::max());

	::commonapi::tests::PredefinedTypeCollection::TestUInt32 testUInt32 = std::numeric_limits<uint32_t>::max();
	ASSERT_EQ(testUInt32, std::numeric_limits<uint32_t>::max());

	::commonapi::tests::PredefinedTypeCollection::TestUInt64 testUInt64 = std::numeric_limits<uint64_t>::max();
	ASSERT_EQ(testUInt64, std::numeric_limits<uint64_t>::max());

	::commonapi::tests::PredefinedTypeCollection::TestInt8 testInt8 = std::numeric_limits<int8_t>::max();
	ASSERT_EQ(testInt8, std::numeric_limits<int8_t>::max());

	::commonapi::tests::PredefinedTypeCollection::TestInt16 testInt16 = std::numeric_limits<int16_t>::max();
	ASSERT_EQ(testInt16, std::numeric_limits<int16_t>::max());

	::commonapi::tests::PredefinedTypeCollection::TestInt32 testInt32 = std::numeric_limits<int32_t>::max();
	ASSERT_EQ(testInt32, std::numeric_limits<int32_t>::max());

	::commonapi::tests::PredefinedTypeCollection::TestInt64 testInt64 = std::numeric_limits<int64_t>::max();
	ASSERT_EQ(testInt64, std::numeric_limits<int64_t>::max());

	::commonapi::tests::PredefinedTypeCollection::TestDouble testDouble = std::numeric_limits<double>::max();
	ASSERT_EQ(testDouble, std::numeric_limits<double>::max());

	::commonapi::tests::PredefinedTypeCollection::TestFloat testFloat = std::numeric_limits<float>::max();
	ASSERT_EQ(testFloat, std::numeric_limits<float>::max());

	::commonapi::tests::PredefinedTypeCollection::TestString testString = std::string("Hello World!");
	ASSERT_EQ(testString, std::string("Hello World!"));

	::commonapi::tests::PredefinedTypeCollection::TestBoolean testBoolean = true;
	ASSERT_EQ(testBoolean, true);

	::commonapi::tests::PredefinedTypeCollection::TestByteBuffer testByteBuffer;
	ASSERT_EQ(testByteBuffer.empty(), true);
	ASSERT_EQ(testByteBuffer.size(), 0);
	testByteBuffer.emplace_back(std::numeric_limits<uint8_t>::max());
	testByteBuffer.emplace_back(0x05);
	testByteBuffer.emplace_back(0xA0);
	testByteBuffer.emplace_back(0xCB);
	testByteBuffer.emplace_back(0xFF);
	ASSERT_EQ(testByteBuffer.empty(), false);
	ASSERT_EQ(testByteBuffer.size(), 5);
	ASSERT_EQ(testByteBuffer[0], std::numeric_limits<uint8_t>::max());
	ASSERT_EQ(testByteBuffer[1], 0x05);
	ASSERT_EQ(testByteBuffer[2], 0xA0);
	ASSERT_EQ(testByteBuffer[3], 0xCB);
	ASSERT_EQ(testByteBuffer[4], 0xFF);
}

int main(int argc, char** argv) {
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}
