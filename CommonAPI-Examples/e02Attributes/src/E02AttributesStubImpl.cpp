/* Copyright (C) 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "E02AttributesStubImpl.h"

E02AttributesStubImpl::E02AttributesStubImpl() {
	cnt = 0;
}

E02AttributesStubImpl::~E02AttributesStubImpl() {
}

void E02AttributesStubImpl::incCounter() {
	cnt++;
	setXAttribute((int32_t)cnt);
	std::cout <<  "New counter value = " << cnt << "!" << std::endl;
}
