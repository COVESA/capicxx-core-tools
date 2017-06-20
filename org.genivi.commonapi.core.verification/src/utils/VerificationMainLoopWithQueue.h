// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef DEMO_MAIN_LOOP_H_
#define DEMO_MAIN_LOOP_H_

#include <CommonAPI/CommonAPI.hpp>

#include <vector>
#include <set>
#include <map>
#include <memory>
#include <condition_variable>
#include <cstdio>
#include <atomic>
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#define DEFAULT_BUFLEN 512
#else
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif

#include <cassert>
#include <mutex>

namespace CommonAPI {

class VerificationMainLoopEventQueue {
public:

    VerificationMainLoopEventQueue() :
        running_(false) {
    }

    ~VerificationMainLoopEventQueue() {
    }

    void run(const int64_t& timeoutInterval = TIMEOUT_INFINITE) {
            (void)timeoutInterval;
        running_ = true;
        std::unique_lock<std::mutex> queueUniqueLock(queueLock_);
        while (running_) {
            queueCondition_.wait(queueUniqueLock);

            for (unsigned int i = 0; i < queue_.size(); i++) {
                queue_.at(i)();
            }
        }
    }

    void stop() {
        std::unique_lock<std::mutex> queueUniqueLock(queueLock_);
        running_ = false;
        queueCondition_.notify_one();
    }

    void pushToQueue(std::function<void()> func) {
        std::lock_guard<std::mutex> queueUniqueLock(queueLock_);
        queue_.push_back(func);
        queueCondition_.notify_one();
    }

    bool isRunning() {
        return running_;
    }

private:
    bool running_;
    std::vector<std::function<void()>> queue_;
    std::condition_variable queueCondition_;
    std::mutex queueLock_;
};

class VerificationMainLoop {
 public:
     VerificationMainLoop() = delete;
     VerificationMainLoop(const VerificationMainLoop&) = delete;
     VerificationMainLoop& operator=(const VerificationMainLoop&) = delete;
     VerificationMainLoop(VerificationMainLoop&&) = delete;
     VerificationMainLoop& operator=(VerificationMainLoop&&) = delete;

     explicit VerificationMainLoop(std::shared_ptr<MainLoopContext> context, std::shared_ptr<VerificationMainLoopEventQueue> eventQueue) :
        context_(context),
        currentMinimalTimeoutInterval_(TIMEOUT_INFINITE),
        running_(false),
        eventQueue_(eventQueue)
    {
    #ifdef _WIN32
         WSADATA wsaData;
         int iResult;

         SOCKET ListenSocket = INVALID_SOCKET;

         struct addrinfo *result = NULL;
         struct addrinfo hints;

         // Initialize Winsock
         iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
         if (iResult != 0) {
             printf("WSAStartup failed with error: %d\n", iResult);
         }

         ZeroMemory(&hints, sizeof(hints));
         hints.ai_family = AF_INET;
         hints.ai_socktype = SOCK_STREAM;
         hints.ai_protocol = IPPROTO_TCP;
         hints.ai_flags = AI_PASSIVE;

         // Resolve the server address and port
         iResult = getaddrinfo(NULL, "0", &hints, &result);
         if (iResult != 0) {
             printf("getaddrinfo failed with error: %d\n", iResult);
             WSACleanup();
         }

         // Create a SOCKET for connecting to server
         ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
         if (ListenSocket == INVALID_SOCKET) {
             printf("socket failed with error: %ld\n", WSAGetLastError());
             freeaddrinfo(result);
             WSACleanup();
         }

         // Setup the TCP listening socket
         iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
         if (iResult == SOCKET_ERROR) {
             printf("bind failed with error: %d\n", WSAGetLastError());
             freeaddrinfo(result);
             closesocket(ListenSocket);
             WSACleanup();
         }

         sockaddr* connected_addr = new sockaddr();
         USHORT port = 0;
         int namelength = sizeof(sockaddr);
         iResult = getsockname(ListenSocket, connected_addr, &namelength);
         if (iResult == SOCKET_ERROR) {
             printf("getsockname failed with error: %d\n", WSAGetLastError());
         } else if (connected_addr->sa_family == AF_INET) {
             port = ((struct sockaddr_in*)connected_addr)->sin_port;
         }
         delete connected_addr;

         freeaddrinfo(result);

         iResult = listen(ListenSocket, SOMAXCONN);
         if (iResult == SOCKET_ERROR) {
             printf("listen failed with error: %d\n", WSAGetLastError());
             closesocket(ListenSocket);
             WSACleanup();
         }

         wsaData;
         wakeFd_.fd = INVALID_SOCKET;
         struct addrinfo *ptr = NULL;

         // Initialize Winsock
         iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
         if (iResult != 0) {
             printf("WSAStartup failed with error: %d\n", iResult);
         }

         ZeroMemory(&hints, sizeof(hints));
         hints.ai_family = AF_UNSPEC;
         hints.ai_socktype = SOCK_STREAM;
         hints.ai_protocol = IPPROTO_TCP;

         // Resolve the server address and port
         iResult = getaddrinfo("127.0.0.1", std::to_string(ntohs(port)).c_str(), &hints, &result);
         if (iResult != 0) {
             printf("getaddrinfo failed with error: %d\n", iResult);
             WSACleanup();
         }

         // Attempt to connect to an address until one succeeds
         for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

             // Create a SOCKET for connecting to server
             wakeFd_.fd = socket(ptr->ai_family, ptr->ai_socktype,
                 ptr->ai_protocol);
             if (wakeFd_.fd == INVALID_SOCKET) {
                 printf("socket failed with error: %ld\n", WSAGetLastError());
                 WSACleanup();
             }

             // Connect to server.
             iResult = connect(wakeFd_.fd, ptr->ai_addr, (int)ptr->ai_addrlen);
             if (iResult == SOCKET_ERROR) {
                 printf("connect failed with error: %ld\n", WSAGetLastError());
                 closesocket(wakeFd_.fd);
                 wakeFd_.fd = INVALID_SOCKET;
                 continue;
             }
             break;
         }

         freeaddrinfo(result);

         if (wakeFd_.fd == INVALID_SOCKET) {
             printf("Unable to connect to server!\n");
             WSACleanup();
         }

         // Accept a client socket
         sendFd_.fd = accept(ListenSocket, NULL, NULL);
         if (sendFd_.fd == INVALID_SOCKET) {
             printf("accept failed with error: %d\n", WSAGetLastError());
             closesocket(ListenSocket);
             WSACleanup();
         }

         wakeFd_.events = POLLIN;
         registerFileDescriptor(wakeFd_);
    #else
        wakeFd_.fd = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
        wakeFd_.events = POLLIN;
        registerFileDescriptor(wakeFd_);
    #endif

        dispatchSourceListenerSubscription_ = context_->subscribeForDispatchSources(
                std::bind(&VerificationMainLoop::registerDispatchSource, this,
                        std::placeholders::_1, std::placeholders::_2),
                std::bind(&VerificationMainLoop::unregisterDispatchSource,
                        this, std::placeholders::_1));
        watchListenerSubscription_ = context_->subscribeForWatches(
                std::bind(&VerificationMainLoop::registerWatch, this,
                        std::placeholders::_1, std::placeholders::_2),
                std::bind(&VerificationMainLoop::unregisterWatch, this,
                        std::placeholders::_1));
        timeoutSourceListenerSubscription_ = context_->subscribeForTimeouts(
                std::bind(&VerificationMainLoop::registerTimeout, this,
                        std::placeholders::_1, std::placeholders::_2),
                std::bind(&VerificationMainLoop::unregisterTimeout, this,
                        std::placeholders::_1));
        wakeupListenerSubscription_ = context_->subscribeForWakeupEvents(
                std::bind(&VerificationMainLoop::wakeup, this));
    }

     ~VerificationMainLoop() {
         unregisterFileDescriptor (wakeFd_);

         context_->unsubscribeForDispatchSources(
                 dispatchSourceListenerSubscription_);
         context_->unsubscribeForWatches(watchListenerSubscription_);
         context_->unsubscribeForTimeouts(timeoutSourceListenerSubscription_);
         context_->unsubscribeForWakeupEvents(wakeupListenerSubscription_);

     #ifdef _WIN32
         // shutdown the connection since no more data will be sent
         int iResult = shutdown(wakeFd_.fd, SD_SEND);
         if (iResult == SOCKET_ERROR) {
             printf("shutdown failed with error: %d\n", WSAGetLastError());
             closesocket(wakeFd_.fd);
             WSACleanup();
         }

         // cleanup
         closesocket(wakeFd_.fd);
         WSACleanup();
     #else
         close(wakeFd_.fd);
     #endif

         cleanup();
    }

     /**
      * \brief Runs the mainloop indefinitely until stop() is called.
      *
      * Runs the mainloop indefinitely until stop() is called. The given timeout (milliseconds)
      * will be overridden if a timeout-event is present that defines an earlier ready time.
      */
    void run(const int64_t& timeoutInterval = TIMEOUT_INFINITE) {
        running_ = true;
        hasToStop_ = false;

        while(!hasToStop_) {
            doSingleIteration(timeoutInterval);
        }
        running_ = false;
    }

    void stop() {
        hasToStop_ = true;
        wakeup();

        eventQueue_->stop();

        while(eventQueue_->isRunning()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    bool isRunning() {
        return running_;
    }

    /**
     * \brief Executes a single cycle of the mainloop.
     *
     * Subsequently calls prepare(), poll(), check() and, if necessary, dispatch().
     * The given timeout (milliseconds) represents the maximum time
     * this iteration will remain in the poll state. All other steps
     * are handled in a non-blocking way. Note however that a source
     * might claim to have infinite amounts of data to dispatch.
     * This demo-implementation of a Mainloop will dispatch a source
     * until it no longer claims to have data to dispatch.
     * Dispatch will not be called if no sources, watches and timeouts
     * claim to be ready during the check()-phase.
     *
     * @param timeout The maximum poll-timeout for this iteration.
     */
    void doSingleIteration(const int64_t& timeout = TIMEOUT_INFINITE) {
        externalIterationMutex_.lock();
        {
            std::lock_guard<std::mutex> itsDispatchSourcesLock(dispatchSourcesMutex_);
            std::lock_guard<std::mutex> itsWatchesLock(watchesMutex_);

            for (auto dispatchSourceIterator = registeredDispatchSources_.begin();
                dispatchSourceIterator != registeredDispatchSources_.end();
                dispatchSourceIterator++) {

                (dispatchSourceIterator->second)->mutex_->lock();
                if ((dispatchSourceIterator->second)->deleteObject_) {
                    if (!(dispatchSourceIterator->second)->isExecuted_) {
                        (dispatchSourceIterator->second)->mutex_->unlock();
                        bool contained = false;
                        for (std::set<std::pair<DispatchPriority, DispatchSourceToDispatchStruct*>>::iterator dispatchSourceIteratorInner = sourcesToDispatch_.begin();
                            dispatchSourceIteratorInner != sourcesToDispatch_.end(); dispatchSourceIteratorInner++) {
                            if (std::get<1>(*dispatchSourceIteratorInner)->dispatchSource_ == (dispatchSourceIterator->second)->dispatchSource_) {
                                contained = true;
                                break;
                            }
                        }
                        if (!contained) {
                            delete (dispatchSourceIterator->second)->dispatchSource_;
                            (dispatchSourceIterator->second)->dispatchSource_ = NULL;
                            delete (dispatchSourceIterator->second)->mutex_;
                            (dispatchSourceIterator->second)->mutex_ = NULL;
                            delete dispatchSourceIterator->second;
                            dispatchSourceIterator = registeredDispatchSources_.erase(dispatchSourceIterator);
                        }
                        if (dispatchSourceIterator == registeredDispatchSources_.end()) {
                            break;
                        }
                    }
                    else {
                        (dispatchSourceIterator->second)->mutex_->unlock();
                    }
                }
                else {
                    (dispatchSourceIterator->second)->mutex_->unlock();
                }
            }

            for (auto watchesIterator = registeredWatches_.begin();
                watchesIterator != registeredWatches_.end();
                watchesIterator++) {

                (watchesIterator->second)->mutex_->lock();
                if ((watchesIterator->second)->deleteObject_) {
                    if (!(watchesIterator->second)->isExecuted_) {
                        (watchesIterator->second)->mutex_->unlock();
                        bool contained = false;
                        for (auto watchesIteratorInner = watchesToDispatch_.begin();
                            watchesIteratorInner != watchesToDispatch_.end(); watchesIteratorInner++) {
                            if (std::get<1>(*watchesIteratorInner)->watch_ == (watchesIterator->second)->watch_) {
                                contained = true;
                                break;
                            }
                        }
                        if (!contained) {
                            delete (watchesIterator->second)->watch_;
                            (watchesIterator->second)->watch_ = NULL;
                            delete (watchesIterator->second)->mutex_;
                            (watchesIterator->second)->mutex_ = NULL;
                            delete watchesIterator->second;
                            watchesIterator = registeredWatches_.erase(watchesIterator);
                        }
                        if (watchesIterator == registeredWatches_.end()) {
                            break;
                        }
                    }
                    else {
                        (watchesIterator->second)->mutex_->unlock();
                    }
                }
                else {
                    (watchesIterator->second)->mutex_->unlock();
                }
            }
        }

        {
            std::lock_guard<std::mutex> itsLock(timeoutsMutex_);
            for (auto timeoutIterator = registeredTimeouts_.begin();
                timeoutIterator != registeredTimeouts_.end();
                timeoutIterator++) {

                (timeoutIterator->second)->mutex_->lock();
                if ((timeoutIterator->second)->deleteObject_) {
                    if (!(timeoutIterator->second)->isExecuted_) {
                        (timeoutIterator->second)->mutex_->unlock();
                        bool contained = false;
                        for (std::set<std::pair<DispatchPriority, TimeoutToDispatchStruct*>>::iterator timeoutIteratorInner = timeoutsToDispatch_.begin();
                            timeoutIteratorInner != timeoutsToDispatch_.end(); timeoutIteratorInner++) {
                            if (std::get<1>(*timeoutIteratorInner)->timeout_ == (timeoutIterator->second)->timeout_) {
                                contained = true;
                                break;
                            }
                        }
                        if (!contained) {
                            delete (timeoutIterator->second)->timeout_;
                            (timeoutIterator->second)->timeout_ = NULL;
                            delete (timeoutIterator->second)->mutex_;
                            (timeoutIterator->second)->mutex_ = NULL;
                            delete timeoutIterator->second;
                            timeoutIterator = registeredTimeouts_.erase(timeoutIterator);
                        }
                        if (timeoutIterator == registeredTimeouts_.end()) {
                            break;
                        }
                    }
                    else {
                        (timeoutIterator->second)->mutex_->unlock();
                    }
                }
                else {
                    (timeoutIterator->second)->mutex_->unlock();
                }
            }
        }

        if (prepare(timeout)) {
            dispatch();
        } else {
            poll();
            if (check()) {
                dispatch();
            }
        }
        externalIterationMutex_.unlock();
    }

    /*
     * The given timeout is a maximum timeout in ms, measured from the current time in the future
     * (a value of 0 means "no timeout"). It will be overridden if a timeout-event is present
     * that defines an earlier ready time.
     */
    bool prepare(const int64_t& timeout = TIMEOUT_INFINITE) {
        currentMinimalTimeoutInterval_ = timeout;

        dispatchSourcesMutex_.lock();
        for (auto dispatchSourceIterator = registeredDispatchSources_.begin();
                dispatchSourceIterator != registeredDispatchSources_.end();
                dispatchSourceIterator++) {

            int64_t dispatchTimeout = TIMEOUT_INFINITE;
            dispatchSourcesMutex_.unlock();
            if (!(dispatchSourceIterator->second->deleteObject_) &&
                    (dispatchSourceIterator->second)->dispatchSource_->prepare(dispatchTimeout)) {
                sourcesToDispatch_.insert(*dispatchSourceIterator);
            } else if (dispatchTimeout > 0 && dispatchTimeout < currentMinimalTimeoutInterval_) {
                currentMinimalTimeoutInterval_ = dispatchTimeout;
            }
            dispatchSourcesMutex_.lock();
        }
        dispatchSourcesMutex_.unlock();

        int64_t currentContextTime = getCurrentTimeInMs();

        {
            std::lock_guard<std::mutex> itsLock(timeoutsMutex_);
            for (auto timeoutPriorityRange = registeredTimeouts_.begin();
                    timeoutPriorityRange != registeredTimeouts_.end();
                    timeoutPriorityRange++) {

                (timeoutPriorityRange->second)->mutex_->lock();
                bool deleteObject = (timeoutPriorityRange->second)->deleteObject_;
                (timeoutPriorityRange->second)->mutex_->unlock();

                if (!deleteObject) {
                    if (!(timeoutPriorityRange->second)->timeoutElapsed_) { // check that timeout is not elapsed
                        int64_t intervalToReady = (timeoutPriorityRange->second)->timeout_->getReadyTime()
                            - currentContextTime;

                        if (intervalToReady <= 0) {
                            // set information that timeout is elapsed
                            (timeoutPriorityRange->second)->timeoutElapsed_ = true;

                            timeoutsToDispatch_.insert(*timeoutPriorityRange);
                            currentMinimalTimeoutInterval_ = TIMEOUT_NONE;
                        } else if (intervalToReady < currentMinimalTimeoutInterval_) {
                            currentMinimalTimeoutInterval_ = intervalToReady;
                        }
                    }
                }
            }
        }

        return (!sourcesToDispatch_.empty() || !timeoutsToDispatch_.empty());
    }

    void poll() {

        // copy file descriptors
        std::vector<pollfd> fileDescriptors;
        {
            std::lock_guard<std::mutex> itsLock(fileDescriptorsMutex_);
            for (auto fileDescriptor = managedFileDescriptors_.begin();
                    fileDescriptor != managedFileDescriptors_.end();
                    ++fileDescriptor) {
                (*fileDescriptor).revents = 0;
                fileDescriptors.push_back(*fileDescriptor);
            }
        }

#ifdef _WIN32
        int numReadyFileDescriptors = WSAPoll(&fileDescriptors[0], fileDescriptors.size(), int(currentMinimalTimeoutInterval_));
#else
        int numReadyFileDescriptors = ::poll(&(fileDescriptors[0]),
                fileDescriptors.size(), int(currentMinimalTimeoutInterval_));
#endif

        // update file descriptors
        {
            std::lock_guard<std::mutex> itsLock(fileDescriptorsMutex_);
            for (auto itFds = fileDescriptors.begin();
                    itFds != fileDescriptors.end();
                    itFds++) {
                for(auto itManagedFds = managedFileDescriptors_.begin();
                        itManagedFds != managedFileDescriptors_.end();
                        ++itManagedFds) {
                    if((*itFds).fd == (*itManagedFds).fd &&
                            (*itFds).events == (*itManagedFds).events) {
                        (*itManagedFds).revents = (*itFds).revents;
                        continue;
                    }
                }
            }
        }

        if (!numReadyFileDescriptors) {
            int64_t currentContextTime = getCurrentTimeInMs();

            {
                std::lock_guard<std::mutex> itsLock(timeoutsMutex_);
                for (auto timeoutPriorityRange = registeredTimeouts_.begin();
                        timeoutPriorityRange != registeredTimeouts_.end();
                        timeoutPriorityRange++) {

                    (timeoutPriorityRange->second)->mutex_->lock();
                    bool deleteObject = (timeoutPriorityRange->second)->deleteObject_;
                    (timeoutPriorityRange->second)->mutex_->unlock();

                    if (!deleteObject) {
                        if (!(timeoutPriorityRange->second)->timeoutElapsed_) { // check that timeout is not elapsed
                            int64_t intervalToReady =
                                (timeoutPriorityRange->second)->timeout_->getReadyTime()
                                    - currentContextTime;

                            if (intervalToReady <= 0) {
                                // set information that timeout is elapsed
                                (timeoutPriorityRange->second)->timeoutElapsed_ = true;

                                timeoutsToDispatch_.insert(*timeoutPriorityRange);
                            }
                        }
                    }
                }
            }
        }

        // If the wakeup descriptor woke us up, we must acknowledge
        if (managedFileDescriptors_[0].revents) {
            wakeupAck();
        }
    }

    bool check() {
        int managedFileDescriptorOffset = 1;
         {
             std::lock_guard<std::mutex> itsLock(fileDescriptorsMutex_);
             for (auto fileDescriptor = managedFileDescriptors_.begin() + managedFileDescriptorOffset;
                     fileDescriptor != managedFileDescriptors_.end(); ++fileDescriptor) {
                 {
                     std::lock_guard<std::mutex> itsWatchesLock(watchesMutex_);
                     for (auto registeredWatchIterator = registeredWatches_.begin();
                             registeredWatchIterator != registeredWatches_.end();
                             registeredWatchIterator++) {

                         (registeredWatchIterator->second)->mutex_->lock();
                         bool deleteObject = (registeredWatchIterator->second)->deleteObject_;
                         (registeredWatchIterator->second)->mutex_->unlock();

                         if (!deleteObject) {
                             if ((registeredWatchIterator->second)->fd_ == fileDescriptor->fd
                                     && fileDescriptor->revents) {
                                 watchesToDispatch_.insert(*registeredWatchIterator);
                             }
                         }
                     }
                 }
             }
         }

         dispatchSourcesMutex_.lock();
         for (auto dispatchSourceIterator = registeredDispatchSources_.begin();
                 dispatchSourceIterator != registeredDispatchSources_.end();
                 ++dispatchSourceIterator) {
             dispatchSourcesMutex_.unlock();
             if (!dispatchSourceIterator->second->deleteObject_&&
                     dispatchSourceIterator->second->dispatchSource_->check()) {
                 sourcesToDispatch_.insert(*dispatchSourceIterator);
             }
             dispatchSourcesMutex_.lock();
         }
         dispatchSourcesMutex_.unlock();

         return (!timeoutsToDispatch_.empty() ||
                 !watchesToDispatch_.empty() ||
                 !sourcesToDispatch_.empty());
    }

    void dispatch() {
        externalIterationMutex_.unlock();
        eventQueue_->pushToQueue(std::bind(&VerificationMainLoop::doExternalIteration, this));
        externalIterationMutex_.lock();
    }

    void doExternalIteration()
    {
        std::lock_guard<std::mutex> itsExternalIterationLock(externalIterationMutex_);
        dispatchTimeouts();
        dispatchWatches();
        dispatchSources();
    }

    void wakeup() {
    #ifdef _WIN32
        // Send an initial buffer
        char *sendbuf = "1";

        int iResult = send(sendFd_.fd, sendbuf, (int)strlen(sendbuf), 0);
        if (iResult == SOCKET_ERROR) {
            int error = WSAGetLastError();

            if (error != WSANOTINITIALISED) {
                printf("send failed with error: %d\n", error);
            }
        }
    #else
        int64_t wake = 1;
        if(::write(wakeFd_.fd, &wake, sizeof(int64_t)) == -1) {
            std::perror("VerificationMainLoopWithQueue::wakeup");
        }
    #endif
    }

    void wakeupAck() {
    #ifdef _WIN32
        // Receive until the peer closes the connection
        int iResult;
        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen = DEFAULT_BUFLEN;

        iResult = recv(wakeFd_.fd, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            //printf("Bytes received from %d: %d\n", wakeFd_.fd, iResult);
        }
        else if (iResult == 0) {
            printf("Connection closed\n");
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
        }
    #else
        int64_t buffer;
        while(::read(wakeFd_.fd, &buffer, sizeof(int64_t)) == sizeof(buffer));
    #endif
    }

    void dispatchTimeouts() {
        if (timeoutsToDispatch_.size() > 0)
        {
            for (auto timeoutIterator = timeoutsToDispatch_.begin();
                    timeoutIterator != timeoutsToDispatch_.end(); timeoutIterator++) {
                auto timeoutToDispatchStruct = std::get<1>(*timeoutIterator);
                timeoutToDispatchStruct->mutex_->lock();
                if (!timeoutToDispatchStruct->deleteObject_) {
                    timeoutToDispatchStruct->isExecuted_ = true;
                    timeoutToDispatchStruct->mutex_->unlock();
                    timeoutToDispatchStruct->timeout_->dispatch();
                    timeoutToDispatchStruct->mutex_->lock();
                    timeoutToDispatchStruct->isExecuted_ = false;
                }
                timeoutToDispatchStruct->mutex_->unlock();
            }

            timeoutsToDispatch_.clear();
        }
    }

    void dispatchWatches() {
        if (watchesToDispatch_.size() > 0)
        {
            for (auto watchIterator = watchesToDispatch_.begin();
                    watchIterator != watchesToDispatch_.end(); watchIterator++) {
                auto watchToDispatchStruct = std::get<1>(*watchIterator);
                watchToDispatchStruct->mutex_->lock();
                if (!watchToDispatchStruct->deleteObject_) {
                    watchToDispatchStruct->isExecuted_ = true;
                    watchToDispatchStruct->mutex_->unlock();
                    Watch* watch = watchToDispatchStruct->watch_;
                    const unsigned int flags = (unsigned int)(watch->getAssociatedFileDescriptor().events);
                    watch->dispatch(flags);
                    watchToDispatchStruct->mutex_->lock();
                    watchToDispatchStruct->isExecuted_ = false;
                }
                watchToDispatchStruct->mutex_->unlock();
            }
            watchesToDispatch_.clear();
        }
    }

    void dispatchSources() {
        if (sourcesToDispatch_.size() > 0)
        {
            isBroken_ = false;
            for (auto dispatchSourceIterator = sourcesToDispatch_.begin();
                    dispatchSourceIterator != sourcesToDispatch_.end() && !isBroken_;
                    dispatchSourceIterator++) {
                auto dispatchSourceToDispatchStruct = std::get<1>(*dispatchSourceIterator);
                dispatchSourceToDispatchStruct->mutex_->lock();
                if (!dispatchSourceToDispatchStruct->deleteObject_) {
                    dispatchSourceToDispatchStruct->isExecuted_ = true;
                    dispatchSourceToDispatchStruct->mutex_->unlock();
                    while(!dispatchSourceToDispatchStruct->deleteObject_ &&
                            dispatchSourceToDispatchStruct->dispatchSource_->dispatch());
                    dispatchSourceToDispatchStruct->mutex_->lock();
                    dispatchSourceToDispatchStruct->isExecuted_ = false;
                }
                dispatchSourceToDispatchStruct->mutex_->unlock();
            }
            {
                sourcesToDispatch_.clear();
            }
        }
    }

 private:

    void cleanup() {
        {
            std::lock_guard<std::mutex> itsLock(dispatchSourcesMutex_);
            for (auto dispatchSourceIterator = registeredDispatchSources_.begin();
                dispatchSourceIterator != registeredDispatchSources_.end();) {

                delete (dispatchSourceIterator->second)->dispatchSource_;
                (dispatchSourceIterator->second)->dispatchSource_ = NULL;
                delete (dispatchSourceIterator->second)->mutex_;
                (dispatchSourceIterator->second)->mutex_ = NULL;
                delete dispatchSourceIterator->second;
                dispatchSourceIterator = registeredDispatchSources_.erase(dispatchSourceIterator);
            }
        }

        {
            std::lock_guard<std::mutex> itsLock(timeoutsMutex_);
            for (auto timeoutIterator = registeredTimeouts_.begin();
                timeoutIterator != registeredTimeouts_.end();) {

                delete (timeoutIterator->second)->timeout_;
                (timeoutIterator->second)->timeout_ = NULL;
                delete (timeoutIterator->second)->mutex_;
                (timeoutIterator->second)->mutex_ = NULL;
                delete timeoutIterator->second;
                timeoutIterator = registeredTimeouts_.erase(timeoutIterator);
            }
        }

        {
            std::lock_guard<std::mutex> itsLock(watchesMutex_);
            for (auto watchesIterator = registeredWatches_.begin();
                watchesIterator != registeredWatches_.end();) {

                delete (watchesIterator->second)->watch_;
                (watchesIterator->second)->watch_ = NULL;
                delete (watchesIterator->second)->mutex_;
                (watchesIterator->second)->mutex_ = NULL;
                delete watchesIterator->second;
                watchesIterator = registeredWatches_.erase(watchesIterator);
            }
        }
    }

    void registerFileDescriptor(const pollfd& fileDescriptor) {
        std::lock_guard<std::mutex> itsLock(fileDescriptorsMutex_);
        managedFileDescriptors_.push_back(fileDescriptor);
    }

    void unregisterFileDescriptor(const pollfd& fileDescriptor) {
        wakeup();
        std::lock_guard<std::mutex> itsLock(fileDescriptorsMutex_);
        for (auto it = managedFileDescriptors_.begin();
            it != managedFileDescriptors_.end(); it++) {
            if ((*it).fd == fileDescriptor.fd && (*it).events == fileDescriptor.events) {
                managedFileDescriptors_.erase(it);
                break;
            }
        }
    }

    void registerDispatchSource(DispatchSource* dispatchSource, const DispatchPriority dispatchPriority) {
        DispatchSourceToDispatchStruct* dispatchSourceStruct =
                new DispatchSourceToDispatchStruct(dispatchSource,
                        new std::mutex, false, false);
        std::lock_guard<std::mutex> itsLock(dispatchSourcesMutex_);
        registeredDispatchSources_.insert(
            { dispatchPriority, dispatchSourceStruct });
    }

    void unregisterDispatchSource(DispatchSource* dispatchSource) {
        {
            std::lock_guard<std::mutex> itsLock(dispatchSourcesMutex_);
            for (auto dispatchSourceIterator = registeredDispatchSources_.begin();
                dispatchSourceIterator != registeredDispatchSources_.end();
                dispatchSourceIterator++) {

                if ((dispatchSourceIterator->second)->dispatchSource_ == dispatchSource){
                    (dispatchSourceIterator->second)->mutex_->lock();
                    (dispatchSourceIterator->second)->deleteObject_ = true;
                    (dispatchSourceIterator->second)->mutex_->unlock();
                    break;
                }
            }
            isBroken_ = true;
        }
    }

    void registerWatch(Watch* watch, const DispatchPriority dispatchPriority) {
        pollfd fdToRegister = watch->getAssociatedFileDescriptor();

        registerFileDescriptor(fdToRegister);

        std::lock_guard<std::mutex> itsLock(watchesMutex_);
        std::mutex* mtx = new std::mutex;

        WatchToDispatchStruct* watchStruct = new WatchToDispatchStruct(fdToRegister.fd, watch, mtx, false, false);
        registeredWatches_.insert({ dispatchPriority, watchStruct});
    }

    void unregisterWatch(Watch* watch) {
        unregisterFileDescriptor(watch->getAssociatedFileDescriptor());

        std::lock_guard<std::mutex> itsLock(watchesMutex_);

        for (auto watchIterator = registeredWatches_.begin();
            watchIterator != registeredWatches_.end(); watchIterator++) {

            if ((watchIterator->second)->watch_ == watch) {
                (watchIterator->second)->mutex_->lock();
                (watchIterator->second)->deleteObject_ = true;
                (watchIterator->second)->mutex_->unlock();
                break;
            }
        }
    }

    void registerTimeout(Timeout* timeout, const DispatchPriority dispatchPriority) {
        TimeoutToDispatchStruct* timeoutStruct = new TimeoutToDispatchStruct(
                timeout, new std::mutex, false, false, false);
        std::lock_guard<std::mutex> itsLock(timeoutsMutex_);
        registeredTimeouts_.insert(
            { dispatchPriority, timeoutStruct });
    }

    void unregisterTimeout(Timeout* timeout) {
        std::lock_guard<std::mutex> itsLock(timeoutsMutex_);
        for (auto timeoutIterator = registeredTimeouts_.begin();
                timeoutIterator != registeredTimeouts_.end();
                timeoutIterator++) {

            if ((timeoutIterator->second)->timeout_ == timeout) {
                (timeoutIterator->second)->mutex_->lock();
                (timeoutIterator->second)->deleteObject_ = true;
                (timeoutIterator->second)->mutex_->unlock();
                break;
            }
        }
    }

    std::shared_ptr<MainLoopContext> context_;

    std::vector<pollfd> managedFileDescriptors_;
    std::mutex fileDescriptorsMutex_;

    struct DispatchSourceToDispatchStruct {
        DispatchSource* dispatchSource_;
        std::mutex* mutex_;
        bool isExecuted_; /* execution flag: indicates, whether the dispatchSource is dispatched currently */
        bool deleteObject_; /* delete flag: indicates, whether the dispatchSource can be deleted*/

        DispatchSourceToDispatchStruct(DispatchSource* _dispatchSource,
            std::mutex* _mutex,
            bool _isExecuted,
            bool _deleteObject) {
                dispatchSource_ = _dispatchSource;
                mutex_ = _mutex;
                isExecuted_ = _isExecuted;
                deleteObject_ = _deleteObject;
        }
    };

    struct TimeoutToDispatchStruct {
        Timeout* timeout_;
        std::mutex* mutex_;
        bool isExecuted_; /* execution flag: indicates, whether the timeout is dispatched currently */
        bool deleteObject_; /* delete flag: indicates, whether the timeout can be deleted*/
        bool timeoutElapsed_; /* timeout elapsed flag: indicates, whether the timeout is elapsed*/

        TimeoutToDispatchStruct(Timeout* _timeout,
            std::mutex* _mutex,
            bool _isExecuted,
            bool _deleteObject,
            bool _timeoutElapsed) {
                timeout_ = _timeout;
                mutex_ = _mutex;
                isExecuted_ = _isExecuted;
                deleteObject_ = _deleteObject;
                timeoutElapsed_ = _timeoutElapsed;
        }
    };

    struct WatchToDispatchStruct {
        int fd_;
        Watch* watch_;
        std::mutex* mutex_;
        bool isExecuted_; /* execution flag: indicates, whether the watch is dispatched currently */
        bool deleteObject_; /* delete flag: indicates, whether the watch can be deleted*/

        WatchToDispatchStruct(int _fd,
            Watch* _watch,
            std::mutex* _mutex,
            bool _isExecuted,
            bool _deleteObject) {
                fd_ = _fd;
                watch_ = _watch;
                mutex_ = _mutex;
                isExecuted_ = _isExecuted;
                deleteObject_ = _deleteObject;
        }
    };

    std::multimap<DispatchPriority, DispatchSourceToDispatchStruct*> registeredDispatchSources_;
    std::multimap<DispatchPriority, WatchToDispatchStruct*> registeredWatches_;
    std::multimap<DispatchPriority, TimeoutToDispatchStruct*> registeredTimeouts_;

    std::mutex dispatchSourcesMutex_;
    std::mutex watchesMutex_;
    std::mutex timeoutsMutex_;

    std::set<std::pair<DispatchPriority, DispatchSourceToDispatchStruct*>> sourcesToDispatch_;
    std::set<std::pair<DispatchPriority, WatchToDispatchStruct*>> watchesToDispatch_;
    std::set<std::pair<DispatchPriority, TimeoutToDispatchStruct*>> timeoutsToDispatch_;

    DispatchSourceListenerSubscription dispatchSourceListenerSubscription_;
    WatchListenerSubscription watchListenerSubscription_;
    TimeoutSourceListenerSubscription timeoutSourceListenerSubscription_;
    WakeupListenerSubscription wakeupListenerSubscription_;

    std::atomic<int64_t> currentMinimalTimeoutInterval_;
    std::atomic<bool> hasToStop_;
    std::atomic<bool> running_;

#ifdef _WIN32
    pollfd sendFd_;
#endif

    pollfd wakeFd_;

    bool isBroken_;

    std::shared_ptr<VerificationMainLoopEventQueue> eventQueue_;

    std::mutex externalIterationMutex_;
};


} // namespace CommonAPI

#endif /* DEMO_MAIN_LOOP_H_ */
