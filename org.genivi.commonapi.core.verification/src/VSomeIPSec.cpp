#include <chrono>
#include <iostream>
#include <thread>
#include <gtest/gtest.h>
#include <CommonAPI/CommonAPI.hpp>
#include "stub/VSomeIPSecStub.hpp"
#include <v1/commonapi/vsomeipsec/ClientIdServiceProxy.hpp>

using namespace ::v1::commonapi::vsomeipsec;

const std::string domain = "local";
const std::string instance = "1";
const std::string applicationNameService = "service-sample";
const std::string applicationNameClient = "client-sample";
const u_int32_t newValue {5};
const uint8_t requestersNumber {1};
const std::string localIP = "127.0.0.1";

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class VSomeIPSec: public ::testing::Test {
protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();
        ASSERT_TRUE((bool)runtime_);

        testStub_ = std::make_shared<ClientIdServiceImpl>([](const std::shared_ptr<CommonAPI::ClientId> _client) {
            EXPECT_EQ(_client->getHostAddress(), localIP);
        });
        ASSERT_TRUE((bool)testStub_);

        ASSERT_TRUE(runtime_->registerService(domain, instance, testStub_, applicationNameService));

        testProxy_ = runtime_->buildProxy<ClientIdServiceProxy>(domain, instance, applicationNameClient);
        ASSERT_TRUE((bool)testProxy_);

        ASSERT_TRUE(testProxy_->isAvailableBlocking());
    }

    void TearDown() {
        bool unregistered = runtime_->unregisterService(domain,
                            ClientIdServiceStubDefault::StubInterface::getInterface(),
                            instance);
        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(unregistered);
        }

        // wait that proxy is not available
        int counter = 0;  // counter for avoiding endless loop
        while ( testProxy_->isAvailable() && counter < 100 ) {
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
            counter++;
        }
        ASSERT_FALSE(testProxy_->isAvailable());
    }

    std::shared_ptr<CommonAPI::Runtime> runtime_;
    std::shared_ptr<ClientIdServiceImpl> testStub_;
    std::shared_ptr<ClientIdServiceProxy<>> testProxy_;

    std::mutex mutex_;
    std::condition_variable cv_;
    bool recieved_msg_ = false;

    uint32_t received_value_ = 0;

};

/**
* @test Check the host_adress on methods for attributes interface
*/
TEST_F(VSomeIPSec, TestClientAttribute) {
    uint32_t itsValue;
    CommonAPI::CallStatus itsStatus;

    auto itsKeyA = testProxy_->getAAttribute().getChangedEvent().subscribe(
            [](const uint32_t &_value) {(void)_value;}
    );

    testProxy_->getAAttribute().setValue(newValue, itsStatus, itsValue);
    ASSERT_EQ(itsStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(itsValue, newValue);

    testProxy_->getAAttribute().getValue(itsStatus, itsValue);
    ASSERT_EQ(itsStatus, CommonAPI::CallStatus::SUCCESS);
    EXPECT_EQ(itsValue, newValue);

    testProxy_->getAAttribute().getChangedEvent().unsubscribe(itsKeyA);
}

/**
* @test Check the host_adress on methods for broadcast selective interface
*/
TEST_F(VSomeIPSec, TestClientBroadcastSelective) {
    auto itsKeyB2 = testProxy_->getB2SelectiveEvent().subscribe(
            [&](const uint32_t &_value) {
                received_value_ = _value;
                std::unique_lock<std::mutex> lk(mutex_);
                recieved_msg_ = true;
                cv_.notify_one();
            }
    );

    while(testStub_->getSubscribersForB2Selective()== nullptr || testStub_->getSubscribersForB2Selective()->size() == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
    ASSERT_EQ(testStub_->getSubscribersForB2Selective()->size(), requestersNumber);

    testStub_->fireB2Selective(newValue);

    // Block until callback happens
    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait(lk, [&]{ return recieved_msg_; });
    ASSERT_EQ(newValue, received_value_);

    testProxy_->getB2SelectiveEvent().unsubscribe(itsKeyB2);
}

/**
* @test Check the host_adress on methods for methods interface
*/
TEST_F(VSomeIPSec, TestClientMethod) {
    uint32_t itsValue;
    CommonAPI::CallStatus itsStatus;
    testProxy_->m(newValue, itsStatus, itsValue);
    ASSERT_EQ(itsStatus, CommonAPI::CallStatus::SUCCESS);
    ASSERT_EQ(newValue, itsValue);

}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
