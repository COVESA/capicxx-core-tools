/* Copyright (C) 2014 - 2015 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DEMO_MAIN_LOOP_H_
#define DEMO_MAIN_LOOP_H_


#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#define COMMONAPI_INTERNAL_COMPILATION
#endif
#include <CommonAPI/MainLoopContext.hpp>
#undef COMMONAPI_INTERNAL_COMPILATION

#include <vector>
#include <set>
#include <map>
#include <cassert>
#include <chrono>
#include <future>
#include <mutex>
#include <cstdio>

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#define DEFAULT_BUFLEN 512
#else
#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#endif

const long maxTimeout = 10;

namespace CommonAPI {

  typedef pollfd DemoMainLoopPollFd;

class VerificationMainLoop {
 public:
    VerificationMainLoop() = delete;
    VerificationMainLoop(const VerificationMainLoop&) = delete;
    VerificationMainLoop& operator=(const VerificationMainLoop&) = delete;
    VerificationMainLoop(VerificationMainLoop&&) = delete;
    VerificationMainLoop& operator=(VerificationMainLoop&&) = delete;

    explicit VerificationMainLoop(std::shared_ptr<MainLoopContext> context)
                : context_(context),
                  currentMinimalTimeoutInterval_(TIMEOUT_INFINITE),
                  hasToStop_(false),
                  running_(false),
                  isBroken_(false) {
    #ifdef WIN32
        wsaEvents_.push_back(WSACreateEvent());
        if (wsaEvents_[0] == WSA_INVALID_EVENT) {
            printf("Invalid Event Created!");
        }
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

        stopPromise = new std::promise<bool>;
    }

    ~VerificationMainLoop() {

    #ifndef WIN32
        unregisterFileDescriptor (wakeFd_);
    #endif  

        context_->unsubscribeForDispatchSources(
                dispatchSourceListenerSubscription_);
        context_->unsubscribeForWatches(watchListenerSubscription_);
        context_->unsubscribeForTimeouts(timeoutSourceListenerSubscription_);
        context_->unsubscribeForWakeupEvents(wakeupListenerSubscription_);

    #ifdef WIN32
        WSACloseEvent(wsaEvents_[0]);
    #else
        close(wakeFd_.fd);
    #endif

        delete stopPromise;
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
        if (stopPromise) {
            stopPromise->set_value(true);
        }
        running_ = false;
    }

    void runVerification(const long& timeoutInterval, bool dispatchTimeoutAndWatches = false, bool dispatchDispatchSources = false) {
        running_ = true;
        hasToStop_ = false;

        prepare(maxTimeout);
        long ti = timeoutInterval*((long)(1000/currentMinimalTimeoutInterval_));

        while (ti>0 && !hasToStop_) {
            ti--;
            doVerificationIteration(dispatchTimeoutAndWatches, dispatchDispatchSources);
        }
        if ( hasToStop_ && stopPromise) {
            stopPromise->set_value(true);
        }
        running_ = false;
        wakeup();
    }

    std::future<bool> stop() {
        // delete old promise to secure, that always a new future object is returned
        delete stopPromise;
        stopPromise = new std::promise<bool>;

        hasToStop_ = true;
        wakeup();

        return stopPromise->get_future();
    }

    bool isRunning() {
        return running_;
    }

    void doVerificationIteration(bool dispatchTimeoutAndWatches, bool dispatchDispatchSources) {
        prepare(maxTimeout);
        poll();
        if (dispatchTimeoutAndWatches || dispatchDispatchSources) {

            if (check()) {

                if (dispatchTimeoutAndWatches) {
                    dispatchWatches();
                    dispatchTimeouts();
                }

                if (dispatchDispatchSources) {
                    dispatchSources();
                }
            }
        }
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
        {
            std::lock_guard<std::mutex> itsLock(dispatchSourcesMutex_);
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

        {
            std::lock_guard<std::mutex> itsLock(watchesMutex_);
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

        if (prepare(timeout)) {
            dispatch();
        } else {
            poll();
            if (check()) {
                dispatch();
            }
        }
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

    #ifdef WIN32
        int managedFileDescriptorOffset = 0;
    #else
        int managedFileDescriptorOffset = 1;
    #endif

        {
            std::lock_guard<std::mutex> itsLock(fileDescriptorsMutex_);
            for (auto fileDescriptor = managedFileDescriptors_.begin() + managedFileDescriptorOffset; fileDescriptor != managedFileDescriptors_.end(); ++fileDescriptor) {
                (*fileDescriptor).revents = 0;
            }
        }

    #ifdef WIN32
        int numReadyFileDescriptors = 0;

        int errorCode = WSAWaitForMultipleEvents((DWORD)wsaEvents_.size(), wsaEvents_.data(), FALSE, (DWORD)currentMinimalTimeoutInterval_, FALSE);

        if (errorCode == WSA_WAIT_IO_COMPLETION) {
            printf("WSAWaitForMultipleEvents failed with error: WSA_WAIT_IO_COMPLETION");
        }
        else if (errorCode == WSA_WAIT_FAILED) {
            printf("WSAWaitForMultipleEvents failed with error: %ld\n", WSAGetLastError());
        }
        else {
            for (uint32_t i = 0; i < managedFileDescriptors_.size(); i++) {
                if (WaitForSingleObjectEx(wsaEvents_[i + 1], 0, true) != WAIT_TIMEOUT) {
                    numReadyFileDescriptors++;
                    managedFileDescriptors_[i].revents = POLLIN;
                }
            }
        }
    #else
        int numReadyFileDescriptors = ::poll(&(managedFileDescriptors_[0]),
                managedFileDescriptors_.size(), int(currentMinimalTimeoutInterval_));
    #endif
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

    #ifdef WIN32
        wakeupAck();
    #else
        // If the wakeup descriptor woke us up, we must acknowledge
        if (managedFileDescriptors_[0].revents) {
            wakeupAck();
        }
    #endif
    }

    bool check() {
        //The first file descriptor always is the loop's wakeup-descriptor (but not for windows anymore). All others need to be linked to a watch.
    #ifdef WIN32
        int managedFileDescriptorOffset = 0;
    #else
        int managedFileDescriptorOffset = 1;
    #endif
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
        dispatchTimeouts();
        dispatchWatches();
        dispatchSources();
    }

    bool dispatchWatchesTooLong;

    void wakeup() {
    #ifdef WIN32
        if (!WSASetEvent(wsaEvents_[0]))
        {
            printf("SetEvent failed (%d)\n", GetLastError());
            return;
        }
    #else
        int64_t wake = 1;
        if(::write(wakeFd_.fd, &wake, sizeof(int64_t)) == -1) {
            std::perror("VerificationMainLoop::wakeup");
        }
    #endif
    }

    void wakeupAck() {
    #ifdef WIN32
        for (unsigned int i = 0; i < wsaEvents_.size(); i++) {
            if (!WSAResetEvent(wsaEvents_[i]))
                {
                    printf("ResetEvent failed (%d)\n", GetLastError());
                    return;
                }
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

#ifdef WIN32
    void registerEvent(
        const HANDLE& wsaEvent) {
        wsaEvents_.push_back(wsaEvent);
    }

    void unregisterEvent(
        const HANDLE& wsaEvent) {
        for (auto it = wsaEvents_.begin();
            it != wsaEvents_.end(); it++) {
            if ((*it) == wsaEvent) {
                wsaEvents_.erase(it);
                break;
            }
        }
    }
#endif

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
        std::lock_guard<std::mutex> itsLock(watchesMutex_);
        pollfd fdToRegister = watch->getAssociatedFileDescriptor();
    
    #ifdef WIN32
        registerEvent(watch->getAssociatedEvent());
    #endif

        registerFileDescriptor(fdToRegister);
        std::mutex* mtx = new std::mutex;

        WatchToDispatchStruct* watchStruct = new WatchToDispatchStruct(fdToRegister.fd, watch, mtx, false, false);
        registeredWatches_.insert({ dispatchPriority, watchStruct});
    }

    void unregisterWatch(Watch* watch) {
        unregisterFileDescriptor(watch->getAssociatedFileDescriptor());

        std::lock_guard<std::mutex> itsLock(watchesMutex_);
    #ifdef WIN32
        unregisterEvent(watch->getAssociatedEvent());
    #endif

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

    int64_t currentMinimalTimeoutInterval_;
    bool breakLoop_;
    bool hasToStop_;
    bool running_;

#ifdef WIN32
    std::vector<HANDLE> wsaEvents_;
#else
    pollfd wakeFd_;
#endif

    bool isBroken_;

    std::promise<bool>* stopPromise;
};


} // namespace CommonAPI

#endif /* DEMO_MAIN_LOOP_H_ */
