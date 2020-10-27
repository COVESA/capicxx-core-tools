/* Copyright (C) 2014-2019 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DTCombinedStub.hpp"

namespace v1 {
namespace commonapi {
namespace datatypes {
namespace combined {

using namespace v1_0::commonapi::datatypes::combined;

DTCombinedStub::DTCombinedStub(){
}

DTCombinedStub::~DTCombinedStub(){
}

void DTCombinedStub::fTest(const std::shared_ptr<CommonAPI::ClientId> _client,
        TestInterface::tStructL3 _tStructL3In, fTestReply_t _reply)
{
    (void)_client;
    _reply(_tStructL3In);
}

} /* namespace combined */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1 */
