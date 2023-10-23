/* Copyright (C) 2015-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file StabilitySP
*/

#include <functional>
#include <fstream>
#include <gtest/gtest.h>
#include <thread>
#include "CommonAPI/CommonAPI.hpp"
#include "v1/commonapi/stability/sp/TestInterfaceProxy.hpp"
#include "stub/StabilitySPStub.hpp"

const std::string serviceId = "service-sample";
const std::string clientId = "client-sample";

const std::string domain = "local";
const std::string testAddress = "commonapi.stability.sp.TestInterface";
const std::string COMMONAPI_CONFIG_SUFFIX = ".conf";
const int MAXSERVERCOUNT = 40;
const int MAXTHREADCOUNT = 8;
const int MAXMETHODCALLS = 80;
const int MAXREGLOOPS = 16;
const int MAXREGCOUNT = 16;
const int MESSAGESIZE = 80;
const int MAXSUBSCRIPTIONSETS = 10;

#ifdef _WIN32
std::mutex gtestMutex;
#endif

using namespace v1_0::commonapi::stability::sp;

class Environment: public ::testing::Environment {
public:
    virtual ~Environment() {
    }
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

class StabilitySP: public ::testing::Test {

protected:
    void SetUp() {
        runtime_ = CommonAPI::Runtime::get();

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE((bool)runtime_);
        }

        testStub_ = std::make_shared<StabilitySPStub>();
        serviceRegistered_ = runtime_->registerService(domain, testAddress, testStub_, serviceId);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceRegistered_);
        }

        testProxy_ = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress, clientId);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE((bool)testProxy_);
        }

        testProxy_->isAvailableBlocking();

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(testProxy_->isAvailable());
        }
    }

    void TearDown() {
        bool unregistered = runtime_->unregisterService(domain, StabilitySPStub::StubInterface::getInterface(), testAddress);

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

    uint8_t value_;
    bool serviceRegistered_;
    bool serviceUnregistered_;
    std::shared_ptr<CommonAPI::Runtime> runtime_;

    std::shared_ptr<TestInterfaceProxy<>> testProxy_;
    std::shared_ptr<TestInterfaceStub> testStub_;
};
/**
* @test Register and unregister services in a loop.
*    - do MAXREGLOOPS times:
*    -   register MAXREGCOUNT addresses as services
*    -   unregister the addresses that were just registered
*    - check the return code of each register/unregister call
*    - test fails if any of the return codes are false
**/

TEST_F(StabilitySP, RepeatedRegistrations) {

    std::shared_ptr<TestInterfaceStubDefault> testMultiRegisterStub_;

    testMultiRegisterStub_ = std::make_shared<StabilitySPStub>();
    for (unsigned int loopcount = 0; loopcount < MAXREGLOOPS; loopcount++) {
        for (unsigned int regcount = 0; regcount < MAXREGCOUNT; regcount++) {
            serviceRegistered_ = runtime_->registerService(domain, testAddress + std::to_string(regcount), testMultiRegisterStub_, serviceId);

            {
#ifdef _WIN32
                std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
                ASSERT_TRUE(serviceRegistered_);
            }
        }
        for (unsigned int regcount = 0; regcount < MAXREGCOUNT; regcount++) {
            serviceUnregistered_ = runtime_->unregisterService(domain, StabilitySPStub::StubInterface::getInterface(), testAddress + std::to_string(regcount));

            {
#ifdef _WIN32
                std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
                ASSERT_TRUE(serviceUnregistered_);
            }
        }
    }
}

/* Helper class. Creates proxies for each server and calls a method for each */
class ProxyThread {
public:
    ProxyThread() :
        asyncCounter(0)
    {
    }
    std::atomic<int> asyncCounter;
    std::shared_ptr<TestInterfaceProxy<>> proxy_[MAXSERVERCOUNT];
    // callback for asynchronous attribute functions.
    void recvValue(const CommonAPI::CallStatus& callStatus, TestInterface::tArray arrayResultValue) {
        std::lock_guard<std::mutex> lock(recvValue_mutex_);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            EXPECT_EQ(callStatus, CommonAPI::CallStatus::SUCCESS);
        }
        asyncCounter++;

        TestInterface::tArray arrayTestValue;

        // check the contents of the attribute.
        for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
            arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
        }
        arrayTestValue.shrink_to_fit();


        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            EXPECT_EQ(arrayTestValue, arrayResultValue);
        }

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_EQ(arrayTestValue.size(), arrayResultValue.size()) << "Vectors arrayTestValue and arrayResultValue are of unequal length";
        }

        if(arrayTestValue.size() == arrayResultValue.size()) {
            for (std::uint32_t i = 0; i < arrayTestValue.size(); ++i) {
                {
#ifdef _WIN32
                    std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
                    EXPECT_EQ(arrayTestValue[i], arrayResultValue[i]) << "Vectors arrayTestValue and arrayResultValue differ at index " << i;
                }
            }
        }
    }
    // callback for attribute subscriptions.
    void recvSubscribedValue(TestInterface::tArray arrayResultValue) {
        std::lock_guard<std::mutex> lock(recvSubscribedValue_mutex_);

        // The following may happen because of the initial response
        if (arrayResultValue.size() == 0)
            return;

        asyncCounter++;
        //std::cout << "asyncCounter<" << this << ">: " << asyncCounter << std::endl;

        TestInterface::tArray arrayTestValue;

        // check the contents of the attribute.
        // The first byte may change, so ignore that one.
        arrayTestValue.push_back(arrayResultValue[0]);
        for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
            arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
        }

        arrayTestValue.shrink_to_fit();

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            EXPECT_EQ(arrayTestValue, arrayResultValue);
        }

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_EQ(arrayTestValue.size(), arrayResultValue.size()) << "Vectors arrayTestValue and arrayResultValue are of unequal length";
        }

        if(arrayTestValue.size() == arrayResultValue.size()) {
            for (std::uint32_t i = 0; i < arrayTestValue.size(); ++i) {
                {
#ifdef _WIN32
                    std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
                    EXPECT_EQ(arrayTestValue[i], arrayResultValue[i]) << "Vectors arrayTestValue and arrayResultValue differ at index " << i;
                }
            }
        }
    }

    // helper function for creating proxies.
    void createProxies(void) {
       std::shared_ptr<CommonAPI::Runtime> runtime_;
       runtime_ = CommonAPI::Runtime::get();
 
       // create a proxy for each of the servers
        for (unsigned int proxycount = 0; proxycount < MAXSERVERCOUNT; proxycount++) {
            proxy_[proxycount] = runtime_->buildProxy<TestInterfaceProxy>(domain, testAddress + std::to_string(proxycount), clientId);
            success_ = success_ && (bool)proxy_[proxycount];

            {
#ifdef _WIN32
                std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
                EXPECT_TRUE(success_);
            }

            for (unsigned int wait = 0; !proxy_[proxycount]->isAvailable() && wait < 100; ++wait) {
                std::this_thread::sleep_for(std::chrono::microseconds(10000));
            }

            if (!proxy_[proxycount]->isAvailable())
                std::cout << testAddress + std::to_string(proxycount) << std::endl;

            {
#ifdef _WIN32
                std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
                EXPECT_TRUE(proxy_[proxycount]->isAvailable());
            }
        }
    }

    void runMethodCalls(void) {
        createProxies();
        for (unsigned int loopcount = 0; loopcount < MAXMETHODCALLS; loopcount++) {
            for (unsigned int proxycount = 0; proxycount < MAXSERVERCOUNT; proxycount++) {
                exerciseMethod(proxy_[proxycount]);
            }
        }
    }
    void runSetAttributes() {
        createProxies();
        for (unsigned int loopcount = 0; loopcount < MAXMETHODCALLS; loopcount++) {
            for (unsigned int proxycount = 0; proxycount < MAXSERVERCOUNT; proxycount++) {
                exerciseSetAttribute(proxy_[proxycount]);
            }
        }

    }
    void runCreateProxiesAndSubscribe() {
         std::function<void (TestInterface::tArray)> myCallback =
            std::bind(&ProxyThread::recvSubscribedValue, this, std::placeholders::_1);
        createProxies();
        for (unsigned int proxycount = 0; proxycount < MAXSERVERCOUNT; proxycount++) {
            // subscribe for attributes in that proxy
            proxy_[proxycount]->getTestAttributeAttribute().getChangedEvent().subscribe(myCallback);
        }
    }

    void waitUntilAsyncCountIsFull(int expected) {
        int previousCount = asyncCounter;
        while (true) {
            if (asyncCounter == expected) break;
            for (unsigned int wait = 0; wait < 1000; wait++) {
                if (previousCount != asyncCounter) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(20000));
            }
            if (previousCount == asyncCounter) {
                break;
            }
            previousCount = asyncCounter;
        }

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            EXPECT_EQ(expected, asyncCounter);
        }
    }

    void runSetSubscribedAttributes(unsigned int id) {
        unsigned char message1 = (unsigned char)(id);
        unsigned char message2 = static_cast<unsigned char>(message1 + MAXTHREADCOUNT);
        unsigned char message = message1;

        for (unsigned int loopcount = 0; loopcount < MAXSUBSCRIPTIONSETS; loopcount++) {
            for (unsigned int proxycount = 0; proxycount < MAXSERVERCOUNT; proxycount++) {
                exerciseSetSubscribedAttribute(proxy_[proxycount], message);
                // toggle between two different messages
                message = static_cast<unsigned char>(message1 + message2 - message);
            }
        }

        // now wait until all the callbacks have been called
        int EXPECTED_COUNT = MAXSUBSCRIPTIONSETS * MAXSERVERCOUNT * MAXTHREADCOUNT;
        waitUntilAsyncCountIsFull(EXPECTED_COUNT);
    }

    void runGetAttributes() {
        createProxies();
        for (unsigned int loopcount = 0; loopcount < MAXMETHODCALLS; loopcount++) {
            for (unsigned int proxycount = 0; proxycount < MAXSERVERCOUNT; proxycount++) {
                exerciseGetAttribute(proxy_[proxycount]);
            }
        }

    }
    void runGetAttributesAsync() {
        createProxies();
        for (unsigned int loopcount = 0; loopcount < MAXMETHODCALLS; loopcount++) {
            for (unsigned int proxycount = 0; proxycount < MAXSERVERCOUNT; proxycount++) {
                exerciseGetAttributeAsync(proxy_[proxycount]);
            }
        }

        // now wait until all the callbacks have been called
        int EXPECTED_COUNT = MAXMETHODCALLS * MAXSERVERCOUNT;
        waitUntilAsyncCountIsFull(EXPECTED_COUNT);

    }
    void runSetAttributesAsync() {
        createProxies();
        for (unsigned int loopcount = 0; loopcount < MAXMETHODCALLS; loopcount++) {
            for (unsigned int proxycount = 0; proxycount < MAXSERVERCOUNT; proxycount++) {
                exerciseSetAttributeAsync(proxy_[proxycount]);
            }
        }

        // now wait until all the callbacks have been called
        int EXPECTED_COUNT = MAXMETHODCALLS * MAXSERVERCOUNT;
        waitUntilAsyncCountIsFull(EXPECTED_COUNT);

    }


    bool getSuccess(void) {
        return success_;
    }
    void setThread(std::thread *thread) {
        thread_ = thread;
    }
    std::thread * getThread(void) {
        return thread_;
    }
    bool exerciseMethod(std::shared_ptr<TestInterfaceProxy<>> proxy) {
        TestInterface::tArray arrayTestValue;
        TestInterface::tArray arrayResultValue;

        for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
            arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
        }

        CommonAPI::CallStatus callStatus;
        proxy->testMethod(arrayTestValue, callStatus, arrayResultValue);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            bool status = callStatus == CommonAPI::CallStatus::SUCCESS;
            EXPECT_TRUE(status);
            if (!status) {
                return false;
            }
        }

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            EXPECT_EQ(arrayTestValue, arrayResultValue);
        }

        return true;
    }
    bool exerciseSetAttribute(std::shared_ptr<TestInterfaceProxy<>> proxy) {
        TestInterface::tArray arrayTestValue;
        TestInterface::tArray arrayResultValue;

        for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
            arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
        }

        CommonAPI::CallStatus callStatus;
        proxy->getTestAttributeAttribute().setValue(arrayTestValue, callStatus, arrayResultValue);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            bool status = callStatus == CommonAPI::CallStatus::SUCCESS;
            EXPECT_TRUE(status);
            if (!status) {
                return false;
            }
        }

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            EXPECT_EQ(arrayTestValue, arrayResultValue);
        }

        return true;
    }

    bool exerciseSetSubscribedAttribute(std::shared_ptr<TestInterfaceProxy<>> proxy, unsigned char message_number) {
        TestInterface::tArray arrayTestValue;
        TestInterface::tArray arrayResultValue;

        arrayTestValue.push_back(message_number);
        for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
            arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
        }

        CommonAPI::CallStatus callStatus;
        proxy->getTestAttributeAttribute().setValue(arrayTestValue, callStatus, arrayResultValue);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            bool status = callStatus == CommonAPI::CallStatus::SUCCESS;
            EXPECT_TRUE(status);
            if (!status) {
                return false;
            }
        }

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            EXPECT_EQ(arrayTestValue, arrayResultValue);
        }

        return true;
    }

    bool exerciseGetAttribute(std::shared_ptr<TestInterfaceProxy<>> proxy) {
        TestInterface::tArray arrayTestValue;
        TestInterface::tArray arrayResultValue;

        for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
            arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
        }

        CommonAPI::CallStatus callStatus;
        proxy->getTestAttributeAttribute().getValue(callStatus, arrayResultValue);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            bool status = callStatus == CommonAPI::CallStatus::SUCCESS;
            EXPECT_TRUE(status);
            if (!status) {
                return false;
            }
        }

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            EXPECT_EQ(arrayTestValue, arrayResultValue);
        }

        return true;
    }
    bool exerciseGetAttributeAsync(std::shared_ptr<TestInterfaceProxy<>> proxy) {
        std::function<void (const CommonAPI::CallStatus&, TestInterface::tArray)> myCallback =
            std::bind(&ProxyThread::recvValue, this, std::placeholders::_1, std::placeholders::_2);

        CommonAPI::CallInfo callInfo(100*1000);

        proxy->getTestAttributeAttribute().getValueAsync(myCallback, &callInfo);
        return true;
    }
    bool exerciseSetAttributeAsync(std::shared_ptr<TestInterfaceProxy<>> proxy) {
        TestInterface::tArray arrayTestValue;
        std::function<void (const CommonAPI::CallStatus&, TestInterface::tArray)> myCallback =
            std::bind(&ProxyThread::recvValue, this, std::placeholders::_1, std::placeholders::_2);

        for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
            arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
        }

        CommonAPI::CallInfo callInfo(100 * 1000);

        proxy->getTestAttributeAttribute().setValueAsync(arrayTestValue, myCallback, &callInfo);
        return true;
    }

    std::thread *thread_ = 0;
    bool success_ = true;
    std::mutex recvValue_mutex_;
    std::mutex recvSubscribedValue_mutex_;
};
/**
* @test Create a number of services and proxies and send messages through them.
*    - Register MAXSERVERCOUNT addresses as services
*    - Create MAXTHREADCOUNT threads, each of which
*        creates a proxy for each service address and
*        then sends MAXMETHODCALLS messages to each.
*    - Each message is MESSAGESIZE bytes long.
*    - Test fails if any of the services fail to get registered
*      or if any of the proxies won't get available
*      or if the return message from the server is not correct
**/
TEST_F(StabilitySP, MultipleMethodCalls) {
    std::shared_ptr<TestInterfaceStubDefault> testMultiRegisterStub_;

    testMultiRegisterStub_ = std::make_shared<StabilitySPStub>();
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceRegistered_ = runtime_->registerService(domain, testAddress + std::to_string(regcount), testMultiRegisterStub_, serviceId);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceRegistered_);
        }
    }

    ProxyThread * proxyrunners[MAXTHREADCOUNT];
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount] = new ProxyThread();
        std::thread * thread = new std::thread(std::bind(&ProxyThread::runMethodCalls, proxyrunners[threadcount]));
        proxyrunners[threadcount]->setThread(thread);
    }

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount]->getThread()->join();
        delete proxyrunners[threadcount]->getThread();
        proxyrunners[threadcount]->setThread(0);
    }

    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceUnregistered_ = runtime_->unregisterService(domain, StabilitySPStub::StubInterface::getInterface(), testAddress + std::to_string(regcount));

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceUnregistered_);
        }
    }

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
            while (proxyrunners[threadcount]->proxy_[regcount]->isAvailable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        delete proxyrunners[threadcount];
        proxyrunners[threadcount] = nullptr;
    }
}
/**
* @test Create a number of services and proxies and set attributes through them.
*    - Register MAXSERVERCOUNT addresses as services
*    - Create MAXTHREADCOUNT threads, each of which
*        creates a proxy for each service address and
*        then sets attributes MAXMETHODCALLS times to each.
*    - Each attribute is MESSAGESIZE bytes long.
*    - Test fails if any of the services fail to get registered
*      or if any of the proxies won't get available
*      or if the return attribute from the server is not correct
**/
TEST_F(StabilitySP, MultipleAttributeSets) {
    std::shared_ptr<TestInterfaceStubDefault> testMultiRegisterStub_;

    testMultiRegisterStub_ = std::make_shared<StabilitySPStub>();
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceRegistered_ = runtime_->registerService(domain, testAddress + std::to_string(regcount), testMultiRegisterStub_, serviceId);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceRegistered_);
        }
    }
    ProxyThread * proxyrunners[MAXTHREADCOUNT];
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount] = new ProxyThread();
        std::thread * thread = new std::thread(std::bind(&ProxyThread::runSetAttributes, proxyrunners[threadcount]));
        proxyrunners[threadcount]->setThread(thread);
    }
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount]->getThread()->join();
        delete proxyrunners[threadcount]->getThread();
        proxyrunners[threadcount]->setThread(0);
    }
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceUnregistered_ = runtime_->unregisterService(domain, StabilitySPStub::StubInterface::getInterface(), testAddress + std::to_string(regcount));

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceUnregistered_);
        }
    }

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
            while (proxyrunners[threadcount]->proxy_[regcount]->isAvailable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        delete proxyrunners[threadcount];
        proxyrunners[threadcount] = nullptr;
    }
}

/**
* @test Create a number of services and proxies and get attributes through them.
*    - Register MAXSERVERCOUNT addresses as services
*       - Set the attribute for service, at the stub side.
*    - Create MAXTHREADCOUNT threads, each of which
*        creates a proxy for each service address and
*        then gets attributes MAXMETHODCALLS times for each.
*    - Each attribute is MESSAGESIZE bytes long.
*    - Test fails if any of the services fail to get registered
*      or if any of the proxies won't get available
*      or if the returned attribute from the server is not correct
**/
TEST_F(StabilitySP, MultipleAttributeGets) {
    std::shared_ptr<StabilitySPStub> testMultiRegisterStub_;

    testMultiRegisterStub_ = std::make_shared<StabilitySPStub>();
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceRegistered_ = runtime_->registerService(domain, testAddress + std::to_string(regcount), testMultiRegisterStub_, serviceId);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceRegistered_);
        }

    }
    TestInterface::tArray arrayTestValue;

    for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
    arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
    }
    testMultiRegisterStub_->setTestValues(arrayTestValue);

    ProxyThread * proxyrunners[MAXTHREADCOUNT];
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount] = new ProxyThread();
        std::thread * thread = new std::thread(std::bind(&ProxyThread::runGetAttributes, proxyrunners[threadcount]));
        proxyrunners[threadcount]->setThread(thread);
    }
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount]->getThread()->join();
        delete proxyrunners[threadcount]->getThread();
        proxyrunners[threadcount]->setThread(0);
    }
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceUnregistered_ = runtime_->unregisterService(domain, StabilitySPStub::StubInterface::getInterface(), testAddress + std::to_string(regcount));

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceUnregistered_);
        }
    }

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
            while (proxyrunners[threadcount]->proxy_[regcount]->isAvailable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        delete proxyrunners[threadcount];
        proxyrunners[threadcount] = nullptr;
    }
}

/**
* @test Create a number of services and proxies and get attributes through them.
*    - Register MAXSERVERCOUNT addresses as services
*       - Set the attribute for service, at the stub side.
*    - Create MAXTHREADCOUNT threads, each of which
*        creates a proxy for each service address and
*        then gets attributes MAXMETHODCALLS times for each asynchronously
*    - Each attribute is MESSAGESIZE bytes long.
*    - Test fails if any of the services fail to get registered
*      or if any of the proxies won't get available
*      or if the callbacks are not called correct number of times
**/
TEST_F(StabilitySP, MultipleAttributeGetAsyncs) {
    std::shared_ptr<StabilitySPStub> testMultiRegisterStub_;

    testMultiRegisterStub_ = std::make_shared<StabilitySPStub>();
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceRegistered_ = runtime_->registerService(domain, testAddress + std::to_string(regcount), testMultiRegisterStub_, serviceId);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceRegistered_);
        }
    }
    TestInterface::tArray arrayTestValue;

    for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
    arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
    }
    testMultiRegisterStub_->setTestValues(arrayTestValue);

    ProxyThread * proxyrunners[MAXTHREADCOUNT];
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount] = new ProxyThread();
        std::thread * thread = new std::thread(std::bind(&ProxyThread::runGetAttributesAsync, proxyrunners[threadcount]));
        proxyrunners[threadcount]->setThread(thread);
    }
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount]->getThread()->join();
        delete proxyrunners[threadcount]->getThread();
        proxyrunners[threadcount]->setThread(0);
    }
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceUnregistered_ = runtime_->unregisterService(domain, StabilitySPStub::StubInterface::getInterface(), testAddress + std::to_string(regcount));

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceUnregistered_);
        }
    }

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
            while (proxyrunners[threadcount]->proxy_[regcount]->isAvailable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        delete proxyrunners[threadcount];
        proxyrunners[threadcount] = nullptr;
    }
}

/**
* @test Create a number of services and proxies and set attributes through them.
*    - Register MAXSERVERCOUNT addresses as services
*       - Set the attribute for service, at the stub side.
*    - Create MAXTHREADCOUNT threads, each of which
*        creates a proxy for each service address and
*        then sets attributes MAXMETHODCALLS times for each asynchronously
*    - Each attribute is MESSAGESIZE bytes long.
*    - Test fails if any of the services fail to get registered
*      or if any of the proxies won't get available
*      or if the callbacks are not called correct number of times
**/
TEST_F(StabilitySP, MultipleAttributeSetAsyncs) {
    std::shared_ptr<StabilitySPStub> testMultiRegisterStub_;

    testMultiRegisterStub_ = std::make_shared<StabilitySPStub>();
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceRegistered_ = runtime_->registerService(domain, testAddress + std::to_string(regcount), testMultiRegisterStub_, serviceId);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceRegistered_);
        }
    }
    TestInterface::tArray arrayTestValue;

    for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
    arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
    }
    testMultiRegisterStub_->setTestValues(arrayTestValue);

    ProxyThread * proxyrunners[MAXTHREADCOUNT];
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount] = new ProxyThread();
        std::thread * thread = new std::thread(std::bind(&ProxyThread::runSetAttributesAsync, proxyrunners[threadcount]));
        proxyrunners[threadcount]->setThread(thread);
    }
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount]->getThread()->join();
        delete proxyrunners[threadcount]->getThread();
        proxyrunners[threadcount]->setThread(0);
    }
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceUnregistered_ = runtime_->unregisterService(domain, StabilitySPStub::StubInterface::getInterface(), testAddress + std::to_string(regcount));

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceUnregistered_);
        }
    }

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
            while (proxyrunners[threadcount]->proxy_[regcount]->isAvailable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        delete proxyrunners[threadcount];
        proxyrunners[threadcount] = nullptr;
    }
}

/**
* @test Create a number of services and proxies and set attributes through them.
*    - Register MAXSERVERCOUNT addresses as services
*       - Set the attribute for service, at the stub side.
*    - Create MAXTHREADCOUNT threads, each of which
*        creates a proxy for each service address and
*        then sets attributes MAXMETHODCALLS times for each asynchronously
*    - Each attribute is MESSAGESIZE bytes long.
*    - Test fails if any of the services fail to get registered
*      or if any of the proxies won't get available
*      or if the callbacks are not called correct number of times
**/
TEST_F(StabilitySP, MultipleAttributeSubscriptions) {
    std::shared_ptr<StabilitySPStub> testMultiRegisterStub_;

    testMultiRegisterStub_ = std::make_shared<StabilitySPStub>();
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceRegistered_ = runtime_->registerService(domain, testAddress + std::to_string(regcount), testMultiRegisterStub_, serviceId);

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceRegistered_);
        }
    }
    TestInterface::tArray arrayTestValue;
    arrayTestValue.push_back(0);
    for (unsigned int messageindex = 0; messageindex < MESSAGESIZE; messageindex++) {
        arrayTestValue.push_back((unsigned char)(messageindex & 0xFF));
    }
    testMultiRegisterStub_->setTestValues(arrayTestValue);

    ProxyThread * proxyrunners[MAXTHREADCOUNT];
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount] = new ProxyThread();
        std::thread * thread = new std::thread(std::bind(&ProxyThread::runCreateProxiesAndSubscribe, proxyrunners[threadcount]));
        proxyrunners[threadcount]->setThread(thread);
    }
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount]->getThread()->join();
        delete proxyrunners[threadcount]->getThread();
        proxyrunners[threadcount]->setThread(0);
    }

    arrayTestValue.clear();
    testMultiRegisterStub_->setTestValues(arrayTestValue);
    // sleep here a while to let the subscriptions sink in
    std::this_thread::sleep_for(std::chrono::microseconds(100000));

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount]->asyncCounter = 0;
    }

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        std::thread * thread = new std::thread(std::bind(&ProxyThread::runSetSubscribedAttributes, proxyrunners[threadcount], threadcount));
        proxyrunners[threadcount]->setThread(thread);
    }
    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        proxyrunners[threadcount]->getThread()->join();
        delete proxyrunners[threadcount]->getThread();
        proxyrunners[threadcount]->setThread(0);
    }
    for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
        serviceUnregistered_ = runtime_->unregisterService(domain, StabilitySPStub::StubInterface::getInterface(), testAddress + std::to_string(regcount));

        {
#ifdef _WIN32
            std::lock_guard<std::mutex> gtestLock(gtestMutex);
#endif
            ASSERT_TRUE(serviceUnregistered_);
        }
    }

    for (unsigned int threadcount = 0; threadcount < MAXTHREADCOUNT; threadcount++) {
        for (unsigned int regcount = 0; regcount < MAXSERVERCOUNT; regcount++) {
            while (proxyrunners[threadcount]->proxy_[regcount]->isAvailable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        delete proxyrunners[threadcount];
        proxyrunners[threadcount] = nullptr;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new Environment());
    return RUN_ALL_TESTS();
}
