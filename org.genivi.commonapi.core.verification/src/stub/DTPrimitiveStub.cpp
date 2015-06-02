/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DTPrimitiveStub.h"
#include <unistd.h>

namespace v1_0 {
namespace commonapi {
namespace datatypes {
namespace primitive {

DTPrimitiveStub::DTPrimitiveStub() {
}

DTPrimitiveStub::~DTPrimitiveStub() {
}

void DTPrimitiveStub::fTest(const std::shared_ptr<CommonAPI::ClientId> _client,
        uint8_t _uint8In,
        int8_t _int8In,
        uint16_t _uint16In,
        int16_t _int16In,
        uint32_t _uint32In,
        int32_t _int32In,
        uint64_t _uint64In,
        int64_t _int64In,
        bool _booleanIn,
        float _floatIn,
        double _doubleIn,
        std::string _stringIn,
        fTestReply_t _reply) {
    _reply(_uint8In,
           _int8In,
           _uint16In,
           _int16In,
           _uint32In,
           _int32In,
           _uint64In,
           _int64In,
           _booleanIn,
           _floatIn,
           _doubleIn,
           _stringIn);

    fireBTestEvent(
            _uint8In,
            _int8In,
            _uint16In,
            _int16In,
            _uint32In,
            _int32In,
            _uint64In,
            _int64In,
            _booleanIn,
            _floatIn,
            _doubleIn,
            _stringIn
    );
}

} /* namespace primitive */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1_0 */
