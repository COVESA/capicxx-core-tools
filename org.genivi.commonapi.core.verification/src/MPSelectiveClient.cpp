/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <gtest/gtest.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <CommonAPI/CommonAPI.hpp>
#include <v1/commonapi/multiprocess/bselective/MPSelectiveProxy.hpp>

#define MAX_LOOPS 100

using namespace v1::commonapi::multiprocess::bselective;

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class MPSelectiveTest: public ::testing::Test {
protected:
    void SetUp() {
    }

    void TearDown() {
    }
};

bool waitUntilAvailable(std::shared_ptr<MPSelectiveProxy<>> myProxy) {
    int counter = 0;
    while (!myProxy->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        counter++;
        if (counter > 100)
            return false;
    }
    return true;
}

bool waitUntilNotAvailable(std::shared_ptr<MPSelectiveProxy<>> myProxy) {
    int counter = 0;
    while (myProxy->isAvailable()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        counter++;
        if (counter > 100)
            return false;
    }
    return true;
}

TEST_F(MPSelectiveTest, Subscriptions) {

    CommonAPI::CallStatus subStatus;
    uint8_t result = 0;
    int message_count = 0;
    CommonAPI::Event<uint8_t>::Subscription subscription;
    bool serviceAvailable;
    bool serviceNotAvailable;

    CommonAPI::Runtime::setProperty("LogContext", "E01C");
    CommonAPI::Runtime::setProperty("LibraryBase", "MPSelective");

    std::shared_ptr < CommonAPI::Runtime > runtime = CommonAPI::Runtime::get();

    std::string domain = "local";
    std::string instance = "commonapi.multiprocess.bselective.MPSelective";
    std::string connection = "client-sample";

    std::shared_ptr<MPSelectiveProxy<>> myProxy = runtime->buildProxy<MPSelectiveProxy>(domain,
            instance, connection);

    int loopcounter = 0;
    while (loopcounter++ < MAX_LOOPS) {

      // clear message count
      message_count = 0;
      // wait until available
      serviceAvailable = waitUntilAvailable(myProxy);
      ASSERT_TRUE(serviceAvailable);

      // subscribe to the broadcast
      // std::cout << "Subscribing " << std::endl;
      subStatus = CommonAPI::CallStatus::UNKNOWN;
      subscription = myProxy->getBTestSelectiveSelectiveEvent().subscribe([&](
          const uint8_t &y
      ) {
          result = y;
          message_count++;
          // std::cout << "Broadcast message received - " << (int)y << std::endl;
      },
      [&](
          const CommonAPI::CallStatus &status
      ) {
          subStatus = status;
      });

      // wait until not available
      serviceNotAvailable = waitUntilNotAvailable(myProxy);
      ASSERT_TRUE(serviceNotAvailable);
      // expected message count = 1
      ASSERT_EQ(message_count, 1);
      // clear message count
      message_count = 0;

      // wait until available
      serviceAvailable = waitUntilAvailable(myProxy);
      ASSERT_TRUE(serviceAvailable);

      // wait until not available
      serviceNotAvailable = waitUntilNotAvailable(myProxy);
      ASSERT_TRUE(serviceNotAvailable);

      // expected message count = 1
      ASSERT_EQ(message_count, 1);

      // clear message count
      message_count = 0;

      // wait until available
      serviceAvailable = waitUntilAvailable(myProxy);
      ASSERT_TRUE(serviceAvailable);

      // unsubscribe
      // std::cout << "Unsubscribing " << std::endl;
      myProxy->getBTestSelectiveSelectiveEvent().unsubscribe(subscription);
      // wait until not available
      serviceNotAvailable = waitUntilNotAvailable(myProxy);
      ASSERT_TRUE(serviceNotAvailable);

      // expected message count = 0
      ASSERT_EQ(message_count, 0);

      // clear message count
      message_count = 0;
      // wait until available
      serviceAvailable = waitUntilAvailable(myProxy);
      ASSERT_TRUE(serviceAvailable);

      // wait until not available
      serviceNotAvailable = waitUntilNotAvailable(myProxy);
      ASSERT_TRUE(serviceNotAvailable);

      // expected message count = 0
      ASSERT_EQ(message_count, 0);
      message_count = 0;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
