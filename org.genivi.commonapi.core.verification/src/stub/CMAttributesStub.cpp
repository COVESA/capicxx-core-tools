/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "CMAttributesStub.hpp"

namespace v1 {
namespace commonapi {
namespace communication {

CMAttributesStub::CMAttributesStub() {
}

CMAttributesStub::~CMAttributesStub() {
}

void CMAttributesStub::setTestValues(uint8_t x) {

    setTestAttributeAttribute(x);
    setTestAAttribute(x);
    setTestBAttribute(x);
    setTestCAttribute(x);
}

} /* namespace v1 */
} /* namespace communication */
} /* namespace commonapi */

