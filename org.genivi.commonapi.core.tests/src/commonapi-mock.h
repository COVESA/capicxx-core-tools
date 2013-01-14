/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef MOCK_PROXY_H_
#define MOCK_PROXY_H_

#include <CommonAPI/Proxy.h>

#include "gmock.h"


class MockProxy: virtual public CommonAPI::Proxy {
 public:
	MOCK_CONST_METHOD0(getAddress, std::string());
	MOCK_CONST_METHOD0(getDomain, const std::string&());
	MOCK_CONST_METHOD0(getServiceId, const std::string&());
	MOCK_CONST_METHOD0(getInstanceId, const std::string&());
	MOCK_CONST_METHOD0(isAvailable, bool());
	MOCK_METHOD0(getProxyStatusEvent, CommonAPI::ProxyStatusEvent&());
	MOCK_METHOD0(getInterfaceVersionAttribute, CommonAPI::InterfaceVersionAttribute&());
};


#endif // MOCK_PROXY_H_
