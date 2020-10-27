/* Copyright (C) 2015-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.deployment;

import java.util.List;

import org.franca.core.franca.FArgument;
import org.franca.core.franca.FArrayType;
import org.franca.core.franca.FAttribute;
import org.franca.core.franca.FBroadcast;
import org.franca.core.franca.FEnumerationType;
import org.franca.core.franca.FField;
import org.franca.core.franca.FMethod;
import org.franca.deploymodel.core.FDeployedInterface;
import org.franca.deploymodel.core.FDeployedTypeCollection;
import org.franca.deploymodel.dsl.fDeploy.FDExtensionElement;
import org.franca.deploymodel.dsl.fDeploy.FDProperty;
import org.franca.deploymodel.dsl.fDeploy.FDString;
import org.franca.deploymodel.dsl.fDeploy.FDValue;
import org.franca.deploymodel.ext.providers.FDeployedProvider;
import org.genivi.commonapi.core.Deployment;

public class PropertyAccessor {

	protected static Integer defaultTimeout_ = new Integer(0);

	protected enum DeploymentType { NONE, INTERFACE, TYPE_COLLECTION, PROVIDER, OVERWRITE };
	protected DeploymentType type_;


	Deployment.ProviderPropertyAccessor provider_;
	Deployment.IDataPropertyAccessor dataAccessor_;

	// The following definitions are contained in the specific accessors
	// for interfaces and type collections. We will simply cast them...

	public enum EnumBackingType {
		UseDefault, UInt8, UInt16, UInt32, UInt64, Int8, Int16, Int32, Int64
	}

	public PropertyAccessor() {
		type_ = DeploymentType.NONE;
		dataAccessor_ = null;
		provider_ = null;
	}

	public PropertyAccessor(FDeployedInterface _target) {
		type_ = (_target == null ? DeploymentType.NONE : DeploymentType.INTERFACE);
		dataAccessor_ = new Deployment.InterfacePropertyAccessor(_target);
		provider_ = null;
	}

	public PropertyAccessor(FDeployedTypeCollection _target) {
		type_ = (_target == null ? DeploymentType.NONE : DeploymentType.TYPE_COLLECTION);
		dataAccessor_ = new Deployment.TypeCollectionPropertyAccessor(_target);
		provider_ = null;
	}

	public PropertyAccessor(FDeployedProvider _target) {
		type_ = (_target == null ? DeploymentType.NONE : DeploymentType.PROVIDER);
		dataAccessor_ = null;
		provider_ = new Deployment.ProviderPropertyAccessor(_target);
	}

	public PropertyAccessor(PropertyAccessor _parent, FField _element) {
		type_ = DeploymentType.OVERWRITE;
		provider_ = null;
		if (_parent.type_ != DeploymentType.PROVIDER )
			dataAccessor_ = _parent.dataAccessor_.getOverwriteAccessor(_element);
		else
			dataAccessor_ = null;
	}
	public PropertyAccessor(PropertyAccessor _parent, FArrayType _element) {
		type_ = DeploymentType.OVERWRITE;
		provider_ = null;
		if (_parent.type_ != DeploymentType.PROVIDER )
			dataAccessor_ = _parent.dataAccessor_.getOverwriteAccessor(_element);
		else
			dataAccessor_ = null;
	}
	public PropertyAccessor(PropertyAccessor _parent, FArgument _element) {
		type_ = DeploymentType.OVERWRITE;
		provider_ = null;
		if (_parent.type_ == DeploymentType.INTERFACE ) {
			Deployment.InterfacePropertyAccessor ipa = (Deployment.InterfacePropertyAccessor) _parent.dataAccessor_;
			dataAccessor_ = ipa.getOverwriteAccessor(_element);
		}
		else
			dataAccessor_ = null;
	}
	public PropertyAccessor(PropertyAccessor _parent, FAttribute _element) {
		type_ = DeploymentType.OVERWRITE;
		provider_ = null;
		if (_parent.type_ == DeploymentType.INTERFACE ) {
			Deployment.InterfacePropertyAccessor ipa = (Deployment.InterfacePropertyAccessor) _parent.dataAccessor_;
			dataAccessor_ = ipa.getOverwriteAccessor(_element);
		}
		else
			dataAccessor_ = null;
	}

	public EnumBackingType getEnumBackingType (FEnumerationType obj) {
		try {
			switch (type_) {
			case INTERFACE:
				return from(((Deployment.InterfacePropertyAccessor)dataAccessor_).getEnumBackingType(obj));
			case TYPE_COLLECTION:
				return from(((Deployment.TypeCollectionPropertyAccessor)dataAccessor_).getEnumBackingType(obj));
			case PROVIDER:
			case NONE:
			default:
				return EnumBackingType.UseDefault;
			}
		}
		catch (java.lang.NullPointerException e) {}
		return EnumBackingType.UInt8;
	}

	public enum BroadcastType {
		signal, error
	}

	public Integer getTimeout(FMethod obj) {
		try {
			if (type_ == DeploymentType.INTERFACE)
				return ((Deployment.InterfacePropertyAccessor)dataAccessor_).getMethodTimeout(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return defaultTimeout_;
	}

	public List<String> getErrors(FMethod obj) {
		try {
			if (type_ == DeploymentType.INTERFACE)
				return ((Deployment.InterfacePropertyAccessor)dataAccessor_).getErrors(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}

	public String getDomain (FDExtensionElement obj) {
		try {
			if (type_ == DeploymentType.PROVIDER)
				return provider_.getDomain(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}

	public String getInstanceId (FDExtensionElement obj) {
		try {
			if (type_ == DeploymentType.PROVIDER)
				return provider_.getInstanceId(obj);
		}
		catch (java.lang.NullPointerException e) {}
		// Access the model directly, without accessor
		for(FDProperty property : obj.getProperties().getItems()) {
			if(property.eContainer().eContainer() instanceof FDExtensionElement) {
				FDValue fdVal = property.getValue().getSingle();
				if(fdVal instanceof FDString) {
					String value = ((FDString) fdVal).getValue();
					return value;
				}
			}
		}
		return null;
	}

	public List<String> getPreregisteredProperties (FDExtensionElement obj) {
		try {
			if (type_ == DeploymentType.PROVIDER)
				return provider_.getPreregisteredProperties(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}

	public BroadcastType getBroadcastType (FBroadcast obj) {
		try {
			if (type_ == DeploymentType.INTERFACE)
				return from(((Deployment.InterfacePropertyAccessor)dataAccessor_).getBroadcastType(obj));
		}
		catch (java.lang.NullPointerException e) {}
		return BroadcastType.signal;
	}

	public String getErrorName (FBroadcast obj) {
		try {
			if (type_ == DeploymentType.INTERFACE)
				return ((Deployment.InterfacePropertyAccessor)dataAccessor_).getErrorName(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}

	private EnumBackingType from(Deployment.Enums.EnumBackingType _source) {
		switch (_source) {
		case UInt8:
			return EnumBackingType.UInt8;
		case UInt16:
			return EnumBackingType.UInt16;
		case UInt32:
			return EnumBackingType.UInt32;
		case UInt64:
			return EnumBackingType.UInt64;
		case Int8:
			return EnumBackingType.Int8;
		case Int16:
			return EnumBackingType.Int16;
		case Int32:
			return EnumBackingType.Int32;
		case Int64:
			return EnumBackingType.Int64;
		default:
			return EnumBackingType.UseDefault;
		}
	}


	private BroadcastType from(Deployment.InterfacePropertyAccessor.BroadcastType _source) {
		if (_source != null) {
			switch (_source) {
			case signal:
				return BroadcastType.signal;
			case error:
				return BroadcastType.error;
			default:
				return BroadcastType.signal;
			}
		}
		return BroadcastType.signal;
	}
}
