/* Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <CommonAPI/CommonAPI.hpp>

template<typename _AttributeType>
class AttributeCacheExtension: public CommonAPI::AttributeExtension<_AttributeType> {
	typedef CommonAPI::AttributeExtension<_AttributeType> __baseClass_t;

 public:
	typedef typename _AttributeType::ValueType value_t;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

	AttributeCacheExtension(_AttributeType& baseAttribute) :
		CommonAPI::AttributeExtension<_AttributeType>(baseAttribute),
		isCacheValid_(false) {
	}

	~AttributeCacheExtension() {}

	bool getCachedValue(value_t& value) {

		if (isCacheValid_) {

			value = cachedValue_;

		} else {

			__baseClass_t::getBaseAttribute().getChangedEvent().subscribe(std::bind(
						&AttributeCacheExtension<_AttributeType>::onValueUpdate,
						this,
						std::placeholders::_1));

			CommonAPI::CallStatus callStatus;

			__baseClass_t::getBaseAttribute().getValue(callStatus, value);
		}

		return isCacheValid_;
	}

 private:

	void onValueUpdate(const value_t& t) {
		isCacheValid_ = true;
		cachedValue_ = t;
	}

	mutable bool isCacheValid_;
	mutable value_t cachedValue_;

};
