/* Copyright (C) 2014 BMW Group
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include "AFPolymorphStub.h"

namespace v1 {
namespace commonapi {
namespace advanced {
namespace polymorph {
    
AFPolymorphStub::AFPolymorphStub() {
}

AFPolymorphStub::~AFPolymorphStub() {
}

void AFPolymorphStub::testMethod(const std::shared_ptr<CommonAPI::ClientId> _client, std::shared_ptr<TestInterface::PStructBase> _x1, testMethodReply_t _reply) {
    (void)_client;
    std::shared_ptr<v1::commonapi::advanced::polymorph::TestInterface::PStructMyTypedef> sp = 
        std::dynamic_pointer_cast<v1::commonapi::advanced::polymorph::TestInterface::PStructMyTypedef>(_x1);
    
    if (sp != nullptr) {
        if ((int)sp->getId() == 1) {
            // send the broadcast
            fireBTestEvent(_x1);            
        }
    }

    std::shared_ptr<TestInterface::PStructBase> y1 = _x1;
    _reply(y1);
}

} /* namespace polymorph */
} /* namespace advanced */
} /* namespace commonapi */
} /* namespace v1 */

