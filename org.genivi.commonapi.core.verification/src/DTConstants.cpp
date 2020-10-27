/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file DataTypes
*/


#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/datatypes/constants/TestInterface.hpp"
#include "commonapi/datatypes/constants/TestTC.hpp"
class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class DTConstants: public ::testing::Test {

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

    }

    void TearDown() {
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
};

/**
* @test See that we can access constants in an interface and that they have correct values
*/
TEST_F(DTConstants, InterfaceConstants) {

    v1_0::commonapi::datatypes::constants::TestInterface tf;

    EXPECT_EQ(tf.i01, 14);
    EXPECT_EQ(tf.i02, -127);
    EXPECT_EQ(tf.i03, 1);
    EXPECT_EQ(tf.i04, 32);
    EXPECT_EQ(tf.i05, 0xa000);
    EXPECT_EQ(tf.i06, 0xfedcba98);
    EXPECT_EQ(tf.i07, 0x51);
    EXPECT_EQ(tf.i08, 0x51);
    EXPECT_EQ(tf.i09, 100000L);
    EXPECT_EQ(tf.i10, -100000L);
    EXPECT_EQ(tf.i11, 10000000LL);
    EXPECT_EQ(tf.i12, -10000000LL);

    CommonAPI::RangedInteger<1, 10> int1_40 = 5;
    EXPECT_EQ(tf.i40, int1_40);

    const std::string s1 = "abc";
    EXPECT_EQ(tf.s1, s1);

    EXPECT_TRUE(tf.b1);
    EXPECT_FALSE(tf.b2);
    EXPECT_FALSE(tf.b3);
    EXPECT_TRUE(tf.b4);
    EXPECT_FALSE(tf.b5);
    EXPECT_FALSE(tf.b6);
    EXPECT_TRUE(tf.b7);

    float delta = 0.01;
    EXPECT_TRUE(tf.f1 - 1.0 < delta);
    EXPECT_TRUE(1.0 - tf.f1 < delta);
    EXPECT_TRUE(tf.f2 + 1.0 < delta);
    EXPECT_TRUE(-1.0 - tf.f2 < delta);
    EXPECT_TRUE(tf.f3 - 1000 < delta);
    EXPECT_TRUE(1000 - tf.f3 < delta);
    delta = 0.00001;
    EXPECT_TRUE(tf.f4 - 0.001 < delta);
    EXPECT_TRUE(0.001 - tf.f4 < delta);
    EXPECT_TRUE(tf.f5 + 0.001 < delta);
    EXPECT_TRUE(-0.001 - tf.f5 < delta);

    double delta2 = 0.01;
    EXPECT_TRUE(tf.d1 - 1.0 < delta2);
    EXPECT_TRUE(1.0 - tf.d1 < delta2);
    EXPECT_TRUE(tf.d2 + 1.0 < delta2);
    EXPECT_TRUE(-1.0 - tf.d2 < delta2);
    EXPECT_TRUE(tf.d3 - 1000 < delta2);
    EXPECT_TRUE(1000 - tf.d3 < delta2);
    delta2 = 0.00001;
    EXPECT_TRUE(tf.d4 - 0.001 < delta2);
    EXPECT_TRUE(0.001 - tf.d4 < delta2);
    delta2 = 1E-18;
    EXPECT_TRUE(tf.d5 + 1E-12 < delta2);
    EXPECT_TRUE(-1E-12 - tf.d5 < delta2);

    EXPECT_EQ(v1_0::commonapi::datatypes::constants::TestInterface::E1::Enum1, tf.key1);
    EXPECT_EQ(v1_0::commonapi::datatypes::constants::TestInterface::E1::Enum2, tf.key2);
    EXPECT_EQ(v1_0::commonapi::datatypes::constants::TestInterface::E1::Enum3, tf.key3);

    EXPECT_EQ(tf.kv1.getKey(), "123");
    v1_0::commonapi::datatypes::constants::TestInterface::U unionValue;
    unionValue = false;
    EXPECT_EQ(tf.kv2.getKey(), "124");
    EXPECT_EQ(tf.kv2.getValue(), unionValue);
}
/**
* @test See that we can access constants in type collection and that they have correct values
*/
TEST_F(DTConstants, TypeCollectionConstants) {

    commonapi::datatypes::constants::TestTC tf;

    EXPECT_EQ(tf.i01, 14);
    EXPECT_EQ(tf.i02, -127);
    EXPECT_EQ(tf.i03, 1);
    EXPECT_EQ(tf.i04, 32);
    EXPECT_EQ(tf.i05, 0xa000);
    EXPECT_EQ(tf.i06, 0xfedcba98);
    EXPECT_EQ(tf.i07, 0x51);
    EXPECT_EQ(tf.i08, 0x51);
    EXPECT_EQ(tf.i09, 100000L);
    EXPECT_EQ(tf.i10, -100000L);
    EXPECT_EQ(tf.i11, 10000000LL);
    EXPECT_EQ(tf.i12, -10000000LL);

    CommonAPI::RangedInteger<1, 10> int1_40 = 5;
    EXPECT_EQ(tf.i40, int1_40);

    const std::string s1 = "abc";
    EXPECT_EQ(tf.s1, s1);

    EXPECT_TRUE(tf.b1);
    EXPECT_FALSE(tf.b2);
    EXPECT_FALSE(tf.b3);
    EXPECT_TRUE(tf.b4);
    EXPECT_FALSE(tf.b5);
    EXPECT_FALSE(tf.b6);
    EXPECT_TRUE(tf.b7);

    float delta = 0.01;
    EXPECT_TRUE(tf.f1 - 1.0 < delta);
    EXPECT_TRUE(1.0 - tf.f1 < delta);
    EXPECT_TRUE(tf.f2 + 1.0 < delta);
    EXPECT_TRUE(-1.0 - tf.f2 < delta);
    EXPECT_TRUE(tf.f3 - 1000 < delta);
    EXPECT_TRUE(1000 - tf.f3 < delta);
    delta = 0.00001;
    EXPECT_TRUE(tf.f4 - 0.001 < delta);
    EXPECT_TRUE(0.001 - tf.f4 < delta);
    EXPECT_TRUE(tf.f5 + 0.001 < delta);
    EXPECT_TRUE(-0.001 - tf.f5 < delta);

    double delta2 = 0.01;
    EXPECT_TRUE(tf.d1 - 1.0 < delta2);
    EXPECT_TRUE(1.0 - tf.d1 < delta2);
    EXPECT_TRUE(tf.d2 + 1.0 < delta2);
    EXPECT_TRUE(-1.0 - tf.d2 < delta2);
    EXPECT_TRUE(tf.d3 - 1000 < delta2);
    EXPECT_TRUE(1000 - tf.d3 < delta2);
    delta2 = 0.00001;
    EXPECT_TRUE(tf.d4 - 0.001 < delta2);
    EXPECT_TRUE(0.001 - tf.d4 < delta2);
    delta2 = 1E-18;
    EXPECT_TRUE(tf.d5 + 1E-12 < delta2);
    EXPECT_TRUE(-1E-12 - tf.d5 < delta2);

    EXPECT_EQ(commonapi::datatypes::constants::TestTC::E1::Enum1, tf.key1);
    EXPECT_EQ(commonapi::datatypes::constants::TestTC::E1::Enum2, tf.key2);
    EXPECT_EQ(commonapi::datatypes::constants::TestTC::E1::Enum3, tf.key3);

    EXPECT_EQ(tf.kv1.getKey(), "123");
    commonapi::datatypes::constants::TestTC::U unionValue;
    unionValue = false;
    EXPECT_EQ(tf.kv2.getKey(), "124");
    EXPECT_EQ(tf.kv2.getValue(), unionValue);
}
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
