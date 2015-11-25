/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file Runtime
*/

#include <functional>
#include <fstream>
#include <gtest/gtest.h>
#include "CommonAPI/CommonAPI.hpp"

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class RTLoadingRuntime: public ::testing::Test {

protected:
    void SetUp() {
    }

    void TearDown() {
    }
};

/**
* @test Loads Default Runtime.
*     - Calls CommonAPI::Runtime::get().
*     - Success if return value is true.
*/
TEST_F(RTLoadingRuntime, LoadsDefaultRuntime) {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    ASSERT_TRUE((bool)runtime);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
