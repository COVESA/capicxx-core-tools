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
#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <cassert>
#include <chrono>
#include <sys/time.h>
#include <future>

const long maxTimeout = 10;

namespace CommonAPI {

class VerificationMainLoop {
 public:
    VerificationMainLoop() = delete;
    VerificationMainLoop(const VerificationMainLoop&) = delete;
    VerificationMainLoop& operator=(const VerificationMainLoop&) = delete;
    VerificationMainLoop(VerificationMainLoop&&) = delete;
    VerificationMainLoop& operator=(VerificationMainLoop&&) = delete;

    explicit VerificationMainLoop(std::shared_ptr<MainLoopContext> context) :
            context_(context), currentMinimalTimeoutInterval_(TIMEOUT_INFINITE), running_(false), breakLoop_(false), dispatchWatchesTooLong(false) {
        wakeFd_.fd = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK);
        wakeFd_.events = POLLIN;

        assert(wakeFd_.fd != -1);
        registerFileDescriptor(wakeFd_);

        dispatchSourceListenerSubscription_ = context_->subscribeForDispatchSources(
                std::bind(&CommonAPI::VerificationMainLoop::registerDispatchSource, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&CommonAPI::VerificationMainLoop::deregisterDispatchSource, this, std::placeholders::_1));
        watchListenerSubscription_ = context_->subscribeForWatches(
                std::bind(&CommonAPI::VerificationMainLoop::registerWatch, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&CommonAPI::VerificationMainLoop::deregisterWatch, this, std::placeholders::_1));
        timeoutSourceListenerSubscription_ = context_->subscribeForTimeouts(
                std::bind(&CommonAPI::VerificationMainLoop::registerTimeout, this, std::placeholders::_1, std::placeholders::_2),
                std::bind(&CommonAPI::VerificationMainLoop::deregisterTimeout, this, std::placeholders::_1));
        wakeupListenerSubscription_ = context_->subscribeForWakeupEvents(
                std::bind(&CommonAPI::VerificationMainLoop::wakeup, this));

        stopPromise = new std::promise<bool>;
    }

    ~VerificationMainLoop() {
        deregisterFileDescriptor(wakeFd_);

        context_->unsubscribeForDispatchSources(dispatchSourceListenerSubscription_);
        context_->unsubscribeForWatches(watchListenerSubscription_);
        context_->unsubscribeForTimeouts(timeoutSourceListenerSubscription_);
        context_->unsubscribeForWakeupEvents(wakeupListenerSubscription_);

        close(wakeFd_.fd);

        delete stopPromise;
    }

    /**
     * \brief Runs the mainloop indefinitely until stop() is called.
     *
     * Runs the mainloop infinitely until stop() is called. The given timeout (milliseconds)
     * will be overridden if a timeout-event is present that defines an earlier ready time.
     */
    void run(const int64_t& timeoutInterval = TIMEOUT_INFINITE) {
        running_ = true;
        while(running_) {
            doSingleIteration(timeoutInterval);
        }

        if (stopPromise) {
        	stopPromise->set_value(true);
        }
    }

    void runVerification(const long& timeoutInterval, bool dispatchTimeoutAndWatches = false, bool dispatchDispatchSources = false) {
        running_ = true;

        prepare(maxTimeout);
        long ti = timeoutInterval*((long)(1000/currentMinimalTimeoutInterval_));

        while (ti>0) {
            ti--;
            doVerificationIteration(dispatchTimeoutAndWatches, dispatchDispatchSources);
        }
        running_ = false;
        wakeup();
    }

    std::future<bool> stop() {
    	// delete old promise to secure, that always a new future object is returned
    	delete stopPromise;
    	stopPromise = new std::promise<bool>;

        running_ = false;
        wakeup();

        return stopPromise->get_future();
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
        prepare(timeout);
        poll();
        if(check()) {
            dispatch();
        }
    }

    /*
     * The given timeout is a maximum timeout in ms, measured from the current time in the future
     * (a value of 0 means "no timeout"). It will be overridden if a timeout-event is present
     * that defines an earlier ready time.
     */
    void prepare(const int64_t& timeout = TIMEOUT_INFINITE) {
        currentMinimalTimeoutInterval_ = timeout;

        for (auto dispatchSourceIterator = registeredDispatchSources_.begin();
                        dispatchSourceIterator != registeredDispatchSources_.end();
                        dispatchSourceIterator++) {

            int64_t dispatchTimeout = TIMEOUT_INFINITE;
            if(dispatchSourceIterator->second->prepare(dispatchTimeout)) {
                sourcesToDispatch_.insert(*dispatchSourceIterator);
            } else if (dispatchTimeout < currentMinimalTimeoutInterval_) {
                currentMinimalTimeoutInterval_ = dispatchTimeout;
            }
        }

        int64_t currentContextTime = getCurrentTimeInMs();

        for (auto timeoutPriorityRange = registeredTimeouts_.begin();
                        timeoutPriorityRange != registeredTimeouts_.end();
                        timeoutPriorityRange++) {

            int64_t intervalToReady = timeoutPriorityRange->second->getReadyTime() - currentContextTime;

            if (intervalToReady <= 0) {
                timeoutsToDispatch_.insert(*timeoutPriorityRange);
                currentMinimalTimeoutInterval_ = TIMEOUT_NONE;
            } else if (intervalToReady < currentMinimalTimeoutInterval_) {
                currentMinimalTimeoutInterval_ = intervalToReady;
            }
        }
    }

    void poll() {
        for (auto fileDescriptor = managedFileDescriptors_.begin() + 1; fileDescriptor != managedFileDescriptors_.end(); ++fileDescriptor) {
            (*fileDescriptor).revents = 0;
        }

        size_t numReadyFileDescriptors = ::poll(&(managedFileDescriptors_[0]), managedFileDescriptors_.size(), currentMinimalTimeoutInterval_);

        // If no FileDescriptors are ready, poll returned because of a timeout that has expired.
        // The only case in which this is not the reason is when the timeout handed in "prepare"
        // expired before any other timeouts.
        if(!numReadyFileDescriptors) {
            int64_t currentContextTime = getCurrentTimeInMs();

            for (auto timeoutPriorityRange = registeredTimeouts_.begin();
                            timeoutPriorityRange != registeredTimeouts_.end();
                            timeoutPriorityRange++) {

                int64_t intervalToReady = timeoutPriorityRange->second->getReadyTime() - currentContextTime;

                if (intervalToReady <= 0) {
                    timeoutsToDispatch_.insert(*timeoutPriorityRange);
                }
            }
        }

        if (wakeFd_.revents) {
            acknowledgeWakeup();
        }
    }

    bool check() {
        //The first file descriptor always is the loop's wakeup-descriptor. All others need to be linked to a watch.
        for (auto fileDescriptor = managedFileDescriptors_.begin() + 1; fileDescriptor != managedFileDescriptors_.end(); ++fileDescriptor) {
            for (auto registeredWatchIterator = registeredWatches_.begin();
                        registeredWatchIterator != registeredWatches_.end();
                        registeredWatchIterator++) {
                const auto& correspondingWatchPriority = registeredWatchIterator->first;
                const auto& correspondingWatchPair = registeredWatchIterator->second;

                if (std::get<0>(correspondingWatchPair) == fileDescriptor->fd && fileDescriptor->revents) {
                    watchesToDispatch_.insert( { correspondingWatchPriority, {std::get<1>(correspondingWatchPair)} } );
                }
            }
        }

        for(auto dispatchSourceIterator = registeredDispatchSources_.begin(); dispatchSourceIterator != registeredDispatchSources_.end(); ++dispatchSourceIterator) {
            if((std::get<1>(*dispatchSourceIterator))->check()) {
                sourcesToDispatch_.insert( {std::get<0>(*dispatchSourceIterator), std::get<1>(*dispatchSourceIterator)});
            }
        }

        return !timeoutsToDispatch_.empty() || !watchesToDispatch_.empty() || !sourcesToDispatch_.empty();
    }

    void dispatch() {
        dispatchTimeouts();
        dispatchWatches();
        dispatchSources();

        timeoutsToDispatch_.clear();
        sourcesToDispatch_.clear();
        watchesToDispatch_.clear();
    }

    bool dispatchWatchesTooLong;

    void wakeup() {
        int64_t wake = 1;
        ::write(wakeFd_.fd, &wake, sizeof(int64_t));
    }

    void dispatchTimeouts() {

        for (auto timeoutIterator = timeoutsToDispatch_.begin(); timeoutIterator != timeoutsToDispatch_.end();
                        timeoutIterator++) {
            std::get<1>(*timeoutIterator)->dispatch();
        }
    }

    void dispatchWatches() {

        for (auto watchIterator = watchesToDispatch_.begin();
                        watchIterator != watchesToDispatch_.end();
                        watchIterator++) {
            Watch* watch = watchIterator->second;
            const unsigned int flags = 7;
            watch->dispatch(flags);
        }
    }

    void dispatchSources() {

        breakLoop_ = false;
        for (auto dispatchSourceIterator = sourcesToDispatch_.begin();
                        dispatchSourceIterator != sourcesToDispatch_.end() && !breakLoop_; dispatchSourceIterator++) {
            while (std::get<1>(*dispatchSourceIterator)->dispatch())
                ;
        }
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
            timeoutsToDispatch_.clear();
            sourcesToDispatch_.clear();
            watchesToDispatch_.clear();
        }
    }

 private:
    void registerFileDescriptor(const pollfd& fileDescriptor) {
        managedFileDescriptors_.push_back(fileDescriptor);
    }

    void deregisterFileDescriptor(const pollfd& fileDescriptor) {
        for (auto it = managedFileDescriptors_.begin(); it != managedFileDescriptors_.end(); it++) {
            if ((*it).fd == fileDescriptor.fd) {
                managedFileDescriptors_.erase(it);
                break;
            }
        }
    }

    void registerDispatchSource(DispatchSource* dispatchSource, const DispatchPriority dispatchPriority) {
        registeredDispatchSources_.insert( {dispatchPriority, dispatchSource} );
    }

    void deregisterDispatchSource(DispatchSource* dispatchSource) {
        for(auto dispatchSourceIterator = registeredDispatchSources_.begin();
                dispatchSourceIterator != registeredDispatchSources_.end();
                dispatchSourceIterator++) {

            if(dispatchSourceIterator->second == dispatchSource) {
                registeredDispatchSources_.erase(dispatchSourceIterator);
                break;
            }
        }
        breakLoop_ = true;
    }

    void registerWatch(Watch* watch, const DispatchPriority dispatchPriority) {
        registerFileDescriptor(watch->getAssociatedFileDescriptor());
        registeredWatches_.insert( { dispatchPriority, {watch->getAssociatedFileDescriptor().fd, watch} } );
    }

    void deregisterWatch(Watch* watch) {
        for(auto watchIterator = registeredWatches_.begin();
                watchIterator != registeredWatches_.end();
                watchIterator++) {

            if(watchIterator->second.second == watch) {
                registeredWatches_.erase(watchIterator);
            }
        }
    }

    void registerTimeout(Timeout* timeout, const DispatchPriority dispatchPriority) {
        registeredTimeouts_.insert( {dispatchPriority, timeout} );
    }

    void deregisterTimeout(Timeout* timeout) {
        for(auto timeoutIterator = registeredTimeouts_.begin();
                timeoutIterator != registeredTimeouts_.end();
                timeoutIterator++) {

            if(timeoutIterator->second == timeout) {
                registeredTimeouts_.erase(timeoutIterator);
                break;
            }
        }
    }

    void acknowledgeWakeup() {
        int64_t buffer;
        while (::read(wakeFd_.fd, &buffer, sizeof(int64_t)) == sizeof(buffer));
    }

    std::shared_ptr<MainLoopContext> context_;

    std::vector<pollfd> managedFileDescriptors_;

    std::multimap<DispatchPriority, DispatchSource*> registeredDispatchSources_;
    std::multimap<DispatchPriority, std::pair<int, Watch*>> registeredWatches_;
    std::multimap<DispatchPriority, Timeout*> registeredTimeouts_;

    std::set<std::pair<DispatchPriority, DispatchSource*>> sourcesToDispatch_;
    std::set<std::pair<DispatchPriority, Watch*>> watchesToDispatch_;
    std::set<std::pair<DispatchPriority, Timeout*>> timeoutsToDispatch_;

    DispatchSourceListenerSubscription dispatchSourceListenerSubscription_;
    WatchListenerSubscription watchListenerSubscription_;
    TimeoutSourceListenerSubscription timeoutSourceListenerSubscription_;
    WakeupListenerSubscription wakeupListenerSubscription_;

    int64_t currentMinimalTimeoutInterval_;
    bool breakLoop_;
    bool running_;

    pollfd wakeFd_;

    std::promise<bool>* stopPromise;
};


} // namespace CommonAPI

#endif /* DEMO_MAIN_LOOP_H_ */
