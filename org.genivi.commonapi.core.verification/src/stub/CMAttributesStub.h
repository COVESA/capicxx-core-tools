/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CMATTRIBUTESSTUB_H_
#define CMATTRIBUTESSTUB_H_

#include "v1/commonapi/communication/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace communication {

class CMAttributesStub : public v1_0::commonapi::communication::TestInterfaceStubDefault {
public:
    CMAttributesStub();
    virtual ~CMAttributesStub();
    virtual void setTestValues(uint8_t);
};

} /* namespace v1 */
} /* namespace communication */
} /* namespace commonapi */

#endif /* CMATTRIBUTESSTUB_H_ */
