/* Copyright (C) 2013-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef STOP_WATCH_HPP_
#define STOP_WATCH_HPP_

#include <cstdint>


class StopWatch {
public:
    typedef uint64_t usec_t;

    StopWatch():
        started_(false),
        totalElapsed_(0),
        startTimePoint_(0) {
    }

    inline void reset() {
        started_ = false;
        totalElapsed_ = 0;
    }

    inline void start() {
        startTimePoint_ = now();
        started_ = true;
    }

    inline void stop() {
        totalElapsed_ += getElapsed();
        started_ = false;
    }

    usec_t getTotalElapsedMicroseconds() const;
    usec_t getTotalElapsedSeconds() const;

private:
    inline usec_t getElapsed() const {
        return now() - startTimePoint_;
    }

    static usec_t now();

    bool started_;
    usec_t totalElapsed_;
    usec_t startTimePoint_;
};

#endif // STOP_WATCH_HPP_
