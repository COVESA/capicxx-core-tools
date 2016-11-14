/* Copyright (C) 2015 BMW Group
 * Author: Lutz Bichler (lutz.bichler@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.deployment;

import java.util.List;

import org.franca.core.franca.FBroadcast;
import org.franca.core.franca.FEnumerationType;
import org.franca.core.franca.FInterface;
import org.franca.core.franca.FMethod;
import org.franca.deploymodel.core.FDeployedInterface;
import org.franca.deploymodel.core.FDeployedProvider;
import org.franca.deploymodel.core.FDeployedTypeCollection;
import org.franca.deploymodel.dsl.fDeploy.FDInterfaceInstance;
import org.franca.deploymodel.dsl.fDeploy.FDProperty;
import org.franca.deploymodel.dsl.fDeploy.FDProvider;
import org.franca.deploymodel.dsl.fDeploy.FDString;
import org.franca.deploymodel.dsl.fDeploy.FDValue;
import org.genivi.commonapi.core.DeploymentInterfacePropertyAccessor;
import org.genivi.commonapi.core.DeploymentProviderPropertyAccessor;
import org.genivi.commonapi.core.DeploymentTypeCollectionPropertyAccessor;

public class PropertyAccessor {
	
	protected static Integer defaultTimeout_ = new Integer(0);
	
	protected enum DeploymentType { NONE, INTERFACE, TYPE_COLLECTION, PROVIDER };
	protected DeploymentType type_;
	
	DeploymentInterfacePropertyAccessor interface_;
	DeploymentTypeCollectionPropertyAccessor typeCollection_;
	DeploymentProviderPropertyAccessor provider_;

	// The following definitions are contained in the specific accessors
	// for interfaces and type collections. We will simply cast them...
	public enum DefaultEnumBackingType {
		UInt8, UInt16, UInt32, UInt64, Int8, Int16, Int32, Int64
	}
	
	public enum EnumBackingType {
		UseDefault, UInt8, UInt16, UInt32, UInt64, Int8, Int16, Int32, Int64
	}
	
	public PropertyAccessor() {
		type_ = DeploymentType.NONE;
		interface_ = null;
		typeCollection_ = null;
		provider_ = null;		
	}
	
	public PropertyAccessor(FDeployedInterface _target) {
		type_ = (_target == null ? DeploymentType.NONE : DeploymentType.INTERFACE);
		interface_ = new DeploymentInterfacePropertyAccessor(_target);
		typeCollection_ = null;
		provider_ = null;
	}

	public PropertyAccessor(FDeployedTypeCollection _target) {
		type_ = (_target == null ? DeploymentType.NONE : DeploymentType.TYPE_COLLECTION);
		interface_ = null;
		typeCollection_ = new DeploymentTypeCollectionPropertyAccessor(_target);
		provider_ = null;
	}
	
	public PropertyAccessor(FDeployedProvider _target) {
		type_ = (_target == null ? DeploymentType.NONE : DeploymentType.PROVIDER);
		interface_ = null;
		typeCollection_ = null;
		provider_ = new DeploymentProviderPropertyAccessor(_target);
	}

	public DefaultEnumBackingType getDefaultEnumBackingType(FInterface obj) {
		try {
			switch (type_) {
			case INTERFACE:
				return from(interface_.getDefaultEnumBackingType(obj));
			case TYPE_COLLECTION:
				return from(typeCollection_.getDefaultEnumBackingType(obj));
			case PROVIDER:
			case NONE:
			default:
			}
		}
		catch (java.lang.NullPointerException e) {}
		return DefaultEnumBackingType.Int32;
	}

	public EnumBackingType getEnumBackingType (FEnumerationType obj) {
		try {
			switch (type_) {
			case INTERFACE:
				return from(interface_.getEnumBackingType(obj));
			case TYPE_COLLECTION:
				return from(typeCollection_.getEnumBackingType(obj));
			case PROVIDER:
			case NONE:
			default:
				return EnumBackingType.UseDefault;
			}
		}
		catch (java.lang.NullPointerException e) {}
		return EnumBackingType.Int32;
	}
	
	public enum BroadcastType {
		signal, error
	}

	public Integer getTimeout(FMethod obj) {
		try {
			if (type_ == DeploymentType.INTERFACE)
				return interface_.getTimeout(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return defaultTimeout_;
	}
	
	public List<String> getErrors(FMethod obj) {
		try {
			if (type_ == DeploymentType.INTERFACE)
				return interface_.getErrors(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}

	public List<FInterface> getClientInstanceReferences (FDProvider obj) {
		try {
			if (type_ == DeploymentType.PROVIDER)
				return provider_.getClientInstanceReferences(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}
	
	public String getDomain (FDInterfaceInstance obj) {
		try {
			if (type_ == DeploymentType.PROVIDER)
				return provider_.getDomain(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}
	
	public String getInstanceId (FDInterfaceInstance obj) {
		try {
			if (type_ == DeploymentType.PROVIDER)
				return provider_.getInstanceId(obj);
		}
		catch (java.lang.NullPointerException e) {}
		// Access the model directly, without accessor
		for(FDProperty property : obj.getProperties()) {
			if(property.eContainer() instanceof FDInterfaceInstance) {
				FDValue fdVal = property.getValue().getSingle();
				if(fdVal instanceof FDString) {
					String value = ((FDString) fdVal).getValue();
					return value;
				}
			}
		}
		return null;
	}
	
	public Integer getDefaultMethodTimeout (FDInterfaceInstance obj) {
		try {
			if (type_ == DeploymentType.PROVIDER)
				return provider_.getDefaultTimeout(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}
	
	public List<String> getPreregisteredProperties (FDInterfaceInstance obj) {
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
				return from(interface_.getBroadcastType(obj));
		}
		catch (java.lang.NullPointerException e) {}
		return BroadcastType.signal;
	}
	
	public String getErrorName (FBroadcast obj) {
		try {
			if (type_ == DeploymentType.INTERFACE)
				return interface_.getErrorName(obj);
		}
		catch (java.lang.NullPointerException e) {}
		return null;
	}

	private DefaultEnumBackingType from(DeploymentInterfacePropertyAccessor.DefaultEnumBackingType _source) {
		switch (_source) {
		case UInt8:
			return DefaultEnumBackingType.UInt8;
		case UInt16:
			return DefaultEnumBackingType.UInt16;
		case UInt32:
			return DefaultEnumBackingType.UInt32;
		case UInt64:
			return DefaultEnumBackingType.UInt64;
		case Int8:
			return DefaultEnumBackingType.Int8;
		case Int16:
			return DefaultEnumBackingType.Int16;
		case Int32:
			return DefaultEnumBackingType.Int32;
		case Int64:
			return DefaultEnumBackingType.Int64;
		default:
			return DefaultEnumBackingType.Int32;
		}
	}
	
	private EnumBackingType from(DeploymentInterfacePropertyAccessor.EnumBackingType _source) {
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
	
	private DefaultEnumBackingType from(DeploymentTypeCollectionPropertyAccessor.DefaultEnumBackingType _source) {
		if (_source != null) {
			switch (_source) {
			case UInt8:
				return DefaultEnumBackingType.UInt8;
			case UInt16:
				return DefaultEnumBackingType.UInt16;
			case UInt32:
				return DefaultEnumBackingType.UInt32;
			case UInt64:
				return DefaultEnumBackingType.UInt64;
			case Int8:
				return DefaultEnumBackingType.Int8;
			case Int16:
				return DefaultEnumBackingType.Int16;
			case Int32:
				return DefaultEnumBackingType.Int32;
			case Int64:
				return DefaultEnumBackingType.Int64;
			default:
				return DefaultEnumBackingType.Int32;
			}
		}
		return DefaultEnumBackingType.Int32;
	}
	
	private EnumBackingType from(DeploymentTypeCollectionPropertyAccessor.EnumBackingType _source) {
		if (_source != null) {
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
		return EnumBackingType.UseDefault;
	}
	
	private BroadcastType from(DeploymentInterfacePropertyAccessor.BroadcastType _source) {
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
