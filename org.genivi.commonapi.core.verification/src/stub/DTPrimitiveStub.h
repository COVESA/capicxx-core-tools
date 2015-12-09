/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DTPRIMITIVESTUB_H_
#define DTPRIMITIVESTUB_H_

#include "v1/commonapi/datatypes/primitive/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace datatypes {
namespace primitive {

class DTPrimitiveStub : public v1_0::commonapi::datatypes::primitive::TestInterfaceStubDefault {
public:
    DTPrimitiveStub();
    virtual ~DTPrimitiveStub();

    virtual void fTest(const std::shared_ptr<CommonAPI::ClientId> _client, uint8_t _uint8In, int8_t _int8In, uint16_t _uint16In, int16_t _int16In, uint32_t _uint32In, int32_t _int32In, uint64_t _uint64In, int64_t _int64In, bool _booleanIn, float _floatIn, double _doubleIn, std::string _stringIn, CommonAPI::ByteBuffer _byteBufferIn, fTestReply_t _reply);
    virtual void fTestEmptyBroadcast(const std::shared_ptr<CommonAPI::ClientId> _client, fTestEmptyBroadcastReply_t _reply);
};

} /* namespace primitive */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1 */
#endif /* DTPRIMITIVESTUB_H_ */
