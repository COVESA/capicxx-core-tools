/* Copyright (C) 2016 BMW Group
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DTDEPLOYMENTSTUB_H_
#define DTDEPLOYMENTSTUB_H_

#include "v1/commonapi/datatypes/deployment/TestInterfaceStubDefault.hpp"

namespace v1 {
namespace commonapi {
namespace datatypes {
namespace deployment {

class DTDeploymentStub : public v1_0::commonapi::datatypes::deployment::TestInterfaceStubDefault {
public:
    DTDeploymentStub();
    virtual ~DTDeploymentStub();

};

} /* namespace deployment */
} /* namespace datatypes */
} /* namespace commonapi */
} /* namespace v1 */
#endif /* DTDEPLOYMENTSTUB_H_ */
