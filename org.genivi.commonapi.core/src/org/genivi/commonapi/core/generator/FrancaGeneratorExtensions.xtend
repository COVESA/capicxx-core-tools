/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import com.google.common.base.Charsets
import com.google.common.hash.Hasher
import com.google.common.hash.Hashing
import com.google.common.primitives.Ints
import java.util.List
import org.eclipse.core.resources.ResourcesPlugin
import org.eclipse.core.runtime.Path
import org.eclipse.core.runtime.preferences.DefaultScope
import org.eclipse.core.runtime.preferences.InstanceScope
import org.eclipse.emf.ecore.EObject
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.emf.ecore.util.EcoreUtil
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMapType
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModel
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FStructType
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FUnionType
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor$DefaultEnumBackingType
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor$EnumBackingType
import org.genivi.commonapi.core.preferences.PreferenceConstants

import static com.google.common.base.Preconditions.*
import org.franca.core.franca.FTypedElement

class FrancaGeneratorExtensions {
    def String getFullyQualifiedName(FModelElement fModelElement) {
        if (fModelElement.eContainer instanceof FModel)
            return (fModelElement.eContainer as FModel).name + '.' + fModelElement.name
        return (fModelElement.eContainer as FModelElement).fullyQualifiedName + '.' + fModelElement.name
    }

    def splitCamelCase(String string) {
        string.split("(?<!(^|[A-Z]))(?=[A-Z])|(?<!^)(?=[A-Z][a-z])")
    }

    def FModel getModel(FModelElement fModelElement) {
        if (fModelElement.eContainer instanceof FModel)
            return (fModelElement.eContainer as FModel)
        return (fModelElement.eContainer as FModelElement).model
    }
    
    def FInterface getContainingInterface(FModelElement fModelElement) {
        if (fModelElement.eContainer == null || fModelElement.eContainer instanceof FModel) {
            return null
        }
        if (fModelElement.eContainer instanceof FInterface) {
            return (fModelElement.eContainer as FInterface)
        }

        return (fModelElement.eContainer as FModelElement).containingInterface
    }
    
    def FTypeCollection getContainingTypeCollection(FModelElement fModelElement) {
        if (fModelElement.eContainer == null || fModelElement.eContainer instanceof FModel) {
            return null
        }
        if (fModelElement.eContainer instanceof FTypeCollection) {
            return (fModelElement.eContainer as FTypeCollection)
        }

        return (fModelElement.eContainer as FModelElement).containingTypeCollection
    }

    def getDirectoryPath(FModel fModel) {
        fModel.name.replace('.', '/')
    }
    
    def String getDefineName(FModelElement fModelElement) {
        val defineSuffix = '_' + fModelElement.name.splitCamelCase.join('_')

        if (fModelElement.eContainer instanceof FModelElement)
            return (fModelElement.eContainer as FModelElement).defineName + defineSuffix

        return (fModelElement.eContainer as FModel).defineName + defineSuffix
    }

    def getDefineName(FModel fModel) {
        fModel.name.toUpperCase.replace('.', '_')
    }

    def private dispatch List<String> getNamespaceAsList(FModel fModel) {
        newArrayList(fModel.name.split("\\."))
    }

    def private dispatch List<String> getNamespaceAsList(FModelElement fModelElement) {
        val namespaceList = fModelElement.eContainer.namespaceAsList
        val isRootElement = fModelElement.eContainer instanceof FModel

        if (!isRootElement)
            namespaceList.add((fModelElement.eContainer as FModelElement).name)

        return namespaceList
    }

    def private getSubnamespaceList(FModelElement destination, EObject source) {
        val sourceNamespaceList = source.namespaceAsList
        val destinationNamespaceList = destination.namespaceAsList
        val maxCount = Ints::min(sourceNamespaceList.size, destinationNamespaceList.size)
        var dropCount = 0

        while (dropCount < maxCount && sourceNamespaceList.get(dropCount).equals(destinationNamespaceList.get(dropCount)))
            dropCount = dropCount + 1

        return destinationNamespaceList.drop(dropCount)
    }

    def getRelativeNameReference(FModelElement destination, EObject source) {
        var nameReference = destination.name

        if (!destination.eContainer.equals(source)) {
            val subnamespaceList = destination.getSubnamespaceList(source)
            if (!subnamespaceList.empty)
                nameReference = subnamespaceList.join('::') + '::' + nameReference
        }

        return nameReference
    }

    def getHeaderFile(FTypeCollection fTypeCollection) {
        fTypeCollection.name + ".h"
    }

    def getHeaderPath(FTypeCollection fTypeCollection) {
        fTypeCollection.model.directoryPath + '/' + fTypeCollection.headerFile
    }

    def getSourceFile(FTypeCollection fTypeCollection) {
        fTypeCollection.name + ".cpp"
    }

    def getSourcePath(FTypeCollection fTypeCollection) {
        fTypeCollection.model.directoryPath + '/' + fTypeCollection.sourceFile
    }

    def getProxyBaseHeaderFile(FInterface fInterface) {
        fInterface.name + "ProxyBase.h"
    }

    def getProxyBaseHeaderPath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.proxyBaseHeaderFile
    }

    def getProxyBaseClassName(FInterface fInterface) {
        fInterface.name + 'ProxyBase'
    }

    def getProxyHeaderFile(FInterface fInterface) {
        fInterface.name + "Proxy.h"
    }

    def getProxyHeaderPath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.proxyHeaderFile
    }

    def getStubDefaultHeaderFile(FInterface fInterface) {
        fInterface.name + "StubDefault.h"
    }

    def getStubDefaultHeaderPath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.stubDefaultHeaderFile
    }

    def getStubDefaultClassName(FInterface fInterface) {
        fInterface.name + 'StubDefault'
    }
    
    def getStubDefaultSourceFile(FInterface fInterface) {
        fInterface.name + "StubDefault.cpp"
    }

    def getStubDefaultSourcePath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.getStubDefaultSourceFile
    }
    
    def getStubRemoteEventClassName(FInterface fInterface) {
        fInterface.name + 'StubRemoteEvent'
    }

    def getStubAdapterClassName(FInterface fInterface) {
        fInterface.name + 'StubAdapter'
    }
    
    def getStubHeaderFile(FInterface fInterface) {
        fInterface.name + "Stub.h"
    }

    def getStubHeaderPath(FInterface fInterface) {
        fInterface.model.directoryPath + '/' + fInterface.stubHeaderFile
    }

    def getStubClassName(FInterface fInterface) {
        fInterface.name + 'Stub'
    }

    def hasAttributes(FInterface fInterface) {
        !fInterface.attributes.empty
    }

    def hasBroadcasts(FInterface fInterface) {
        !fInterface.broadcasts.empty
    }

    def generateDefinition(FMethod fMethod) {
        fMethod.generateDefinitionWithin(null)
    }

    def generateDefinitionWithin(FMethod fMethod, String parentClassName) {
        var definition = 'void '

        if (!parentClassName.nullOrEmpty)
            definition = definition + parentClassName + '::'

        definition = definition + fMethod.name + '(' + fMethod.generateDefinitionSignature + ')'

        return definition
    }

    def generateDefinitionSignature(FMethod fMethod) {
        var signature = fMethod.inArgs.map['const ' + getTypeName(fMethod.model) + '& ' + name].join(', ')

        if (!fMethod.inArgs.empty)
            signature = signature + ', '

        signature = signature + 'CommonAPI::CallStatus& callStatus'

        if (fMethod.hasError)
            signature = signature + ', ' + fMethod.getErrorNameReference(fMethod.eContainer) + '& methodError'

        if (!fMethod.outArgs.empty)
            signature = signature + ', ' + fMethod.outArgs.map[getTypeName(fMethod.model) + '& ' + name].join(', ')

        return signature
    }
    
    def generateStubSignature(FMethod fMethod) {
        var signature = fMethod.inArgs.map[getTypeName(fMethod.model) + ' ' + name].join(', ')
        if (!fMethod.inArgs.empty && (fMethod.hasError || !fMethod.outArgs.empty))
            signature = signature + ', '

        if (fMethod.hasError)
            signature = signature + fMethod.getErrorNameReference(fMethod.eContainer) + '& methodError'
        if (fMethod.hasError && !fMethod.outArgs.empty)
            signature = signature + ', '

        if (!fMethod.outArgs.empty)
            signature = signature + fMethod.outArgs.map[getTypeName(fMethod.model) + '& ' + name].join(', ')

        return signature
    }

    def generateAsyncDefinition(FMethod fMethod) {
        fMethod.generateAsyncDefinitionWithin(null)
    }

    def generateAsyncDefinitionWithin(FMethod fMethod, String parentClassName) {
        var definition = 'std::future<CommonAPI::CallStatus> '

        if (!parentClassName.nullOrEmpty)
            definition = definition + parentClassName + '::'

        definition = definition + fMethod.name + 'Async(' + fMethod.generateAsyncDefinitionSignature + ')'

        return definition
    }

    def generateAsyncDefinitionSignature(FMethod fMethod) {
        var signature = fMethod.inArgs.map['const ' + getTypeName(fMethod.model) + '& ' + name].join(', ')
        if (!fMethod.inArgs.empty)
            signature = signature + ', '
        return signature + fMethod.asyncCallbackClassName + ' callback'
    }

    def getAsyncCallbackClassName(FMethod fMethod) {
        fMethod.name.toFirstUpper + 'AsyncCallback'
    }

    def hasError(FMethod fMethod) {
        fMethod.errorEnum != null || fMethod.errors != null
    }

    def getErrorNameReference(FMethod fMethod, EObject source) {
        checkArgument(fMethod.hasError, 'FMethod has no error: ' + fMethod)
        if (fMethod.errorEnum != null) {
            return fMethod.errorEnum.getRelativeNameReference((source as FModelElement).model)
		}

        var errorNameReference = fMethod.errors.errorName
		errorNameReference = (fMethod.eContainer as FInterface).getRelativeNameReference(source) + '::' + errorNameReference
        return errorNameReference
    }

    def getClassName(FAttribute fAttribute) {
        fAttribute.name.toFirstUpper + 'Attribute'
    }

    def generateGetMethodDefinition(FAttribute fAttribute) {
        fAttribute.generateGetMethodDefinitionWithin(null)
    }

    def generateGetMethodDefinitionWithin(FAttribute fAttribute, String parentClassName) {
        var definition = fAttribute.className + '& '

        if (!parentClassName.nullOrEmpty)
            definition = parentClassName + '::' + definition + parentClassName + '::'

        definition = definition + 'get' + fAttribute.className + '()'

        return definition
    }
   
    def isReadonly(FAttribute fAttribute) {
        fAttribute.readonly
    }

    def isObservable(FAttribute fAttribute) {
        fAttribute.noSubscriptions == false
    }

    def getStubAdapterClassFireChangedMethodName(FAttribute fAttribute) {
        'fire' + fAttribute.name.toFirstUpper + 'AttributeChanged'
    }

    def getStubRemoteEventClassSetMethodName(FAttribute fAttribute) {
        'onRemoteSet' + fAttribute.name.toFirstUpper + 'Attribute'
    }

    def getStubRemoteEventClassChangedMethodName(FAttribute fAttribute) {
        'onRemote' + fAttribute.name.toFirstUpper + 'AttributeChanged'
    }

    def getStubClassGetMethodName(FAttribute fAttribute) {
        'get' + fAttribute.name.toFirstUpper + 'Attribute'
    }

    def getClassName(FBroadcast fBroadcast) {
        fBroadcast.name.toFirstUpper + 'Event'
    }

    def generateGetMethodDefinition(FBroadcast fBroadcast) {
        fBroadcast.generateGetMethodDefinitionWithin(null)
    }

    def generateGetMethodDefinitionWithin(FBroadcast fBroadcast, String parentClassName) {
        var definition = fBroadcast.className + '& '

        if (!parentClassName.nullOrEmpty)
            definition = parentClassName + '::' + definition + parentClassName + '::'

        definition = definition + 'get' + fBroadcast.className + '()'

        return definition
    }

    def getStubAdapterClassFireEventMethodName(FBroadcast fBroadcast) {
        'fire' + fBroadcast.name.toFirstUpper + 'Event'
    }

    def getTypeName(FTypedElement element, EObject source) {
        var typeName = element.type.getNameReference(source)

        if (element.type.derived instanceof FStructType && (element.type.derived as FStructType).hasPolymorphicBase)
            typeName = 'std::shared_ptr<' + typeName + '>'

        if ("[]".equals(element.array))
            typeName = 'std::vector<' + element.type.getNameReference(source) + '>'

        return typeName
    }
    
    def getNameReference(FTypeRef destination, EObject source) {
        if (destination.derived != null)
            return destination.derived.getRelativeNameReference(source)
        return destination.predefined.primitiveTypeName
    }

    def getErrorName(FEnumerationType fMethodErrors) {
        checkArgument(fMethodErrors.eContainer instanceof FMethod, 'Not FMethod errors')
        (fMethodErrors.eContainer as FMethod).name + 'Error'
    }

    def getBackingType(FEnumerationType fEnumerationType, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        if(deploymentAccessor.getEnumBackingType(fEnumerationType) == EnumBackingType::UseDefault) {
            if(fEnumerationType.containingInterface != null) {
                switch(deploymentAccessor.getDefaultEnumBackingType(fEnumerationType.containingInterface)) {
                    case DefaultEnumBackingType::UInt8:
                        return FBasicTypeId::UINT8
                    case DefaultEnumBackingType::UInt16:
                        return FBasicTypeId::UINT16
                    case DefaultEnumBackingType::UInt32:
                        return FBasicTypeId::UINT32
                    case DefaultEnumBackingType::UInt64:
                        return FBasicTypeId::UINT64
                    case DefaultEnumBackingType::Int8:
                        return FBasicTypeId::INT8
                    case DefaultEnumBackingType::Int16:
                        return FBasicTypeId::INT16
                    case DefaultEnumBackingType::Int32:
                        return FBasicTypeId::INT32
                    case DefaultEnumBackingType::Int64:
                        return FBasicTypeId::INT64
                }
            }
        }
        switch(deploymentAccessor.getEnumBackingType(fEnumerationType)) {
            case EnumBackingType::UInt8:
                return FBasicTypeId::UINT8
            case EnumBackingType::UInt16:
                return FBasicTypeId::UINT16
            case EnumBackingType::UInt32:
                return FBasicTypeId::UINT32
            case EnumBackingType::UInt64:
                return FBasicTypeId::UINT64
            case EnumBackingType::Int8:
                return FBasicTypeId::INT8
            case EnumBackingType::Int16:
                return FBasicTypeId::INT16
            case EnumBackingType::Int32:
                return FBasicTypeId::INT32
            case EnumBackingType::Int64:
                return FBasicTypeId::INT64
        }
        return FBasicTypeId::INT32
    }

    def getPrimitiveTypeName(FBasicTypeId fBasicTypeId) {
        switch fBasicTypeId {
            case FBasicTypeId::BOOLEAN: "bool"
            case FBasicTypeId::INT8: "int8_t"
            case FBasicTypeId::UINT8: "uint8_t"
            case FBasicTypeId::INT16: "int16_t"
            case FBasicTypeId::UINT16: "uint16_t"
            case FBasicTypeId::INT32: "int32_t"
            case FBasicTypeId::UINT32: "uint32_t"
            case FBasicTypeId::INT64: "int64_t"
            case FBasicTypeId::UINT64: "uint64_t"
            case FBasicTypeId::FLOAT: "float"
            case FBasicTypeId::DOUBLE: "double"
            case FBasicTypeId::STRING: "std::string"
            case FBasicTypeId::BYTE_BUFFER: "CommonAPI::ByteBuffer"
            default: throw new IllegalArgumentException("Unsupported basic type: " + fBasicTypeId.name)
        }
    }
    
    def String typeStreamSignature(FTypeRef fTypeRef, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        if (fTypeRef.derived != null)
            return fTypeRef.derived.typeStreamFTypeSignature(deploymentAccessor)
        return fTypeRef.predefined.basicTypeStreamSignature
    }
    
    def private String getBasicTypeStreamSignature(FBasicTypeId fBasicTypeId) {
        switch fBasicTypeId {
            case FBasicTypeId::BOOLEAN: return "typeOutputStream.writeBoolType();"
            case FBasicTypeId::INT8: return "typeOutputStream.writeInt8Type();"
            case FBasicTypeId::UINT8: return "typeOutputStream.writeUInt8Type();"
            case FBasicTypeId::INT16: return "typeOutputStream.writeInt16Type();"
            case FBasicTypeId::UINT16: return "typeOutputStream.writeUInt16Type();"
            case FBasicTypeId::INT32: return "typeOutputStream.writeInt32Type();"
            case FBasicTypeId::UINT32: return "typeOutputStream.writeUInt32Type();"
            case FBasicTypeId::INT64: return "typeOutputStream.writeInt64Type();"
            case FBasicTypeId::UINT64: return "typeOutputStream.writeUInt64Type();"
            case FBasicTypeId::FLOAT: return "typeOutputStream.writeFloatType();"
            case FBasicTypeId::DOUBLE: return "typeOutputStream.writeDoubleType();"
            case FBasicTypeId::STRING: return "typeOutputStream.writeStringType();"
            case FBasicTypeId::BYTE_BUFFER: return "typeOutputStream.writeByteBufferType();"
        }
    }
    
    def private dispatch String typeStreamFTypeSignature(FTypeDef fTypeDef, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        return fTypeDef.actualType.typeStreamSignature(deploymentAccessor)
    }

    def private dispatch String typeStreamFTypeSignature(FArrayType fArrayType, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        return 'typeOutputStream.beginWriteVectorType();\n' + 
        fArrayType.elementType.typeStreamSignature(deploymentAccessor) + '\n' + 
        'typeOutputStream.endWriteVectorType();'
    }

    def private dispatch String typeStreamFTypeSignature(FMapType fMap, DeploymentInterfacePropertyAccessor deploymentAccessor) {
    	return 'typeOutputStream.beginWriteMapType();\n' + 
    	fMap.keyType.typeStreamSignature(deploymentAccessor) + '\n' + fMap.valueType.typeStreamSignature(deploymentAccessor) + '\n' + 
    	'typeOutputStream.endWriteMapType();'
    }

    def private dispatch String typeStreamFTypeSignature(FStructType fStructType, DeploymentInterfacePropertyAccessor deploymentAccessor) {
    	return 'typeOutputStream.beginWriteStructType();\n' +
    	fStructType.getElementsTypeStreamSignature(deploymentAccessor) + '\n' +
    	'typeOutputStream.endWriteStructType();'
    }

    def private dispatch String typeStreamFTypeSignature(FEnumerationType fEnumerationType, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        return fEnumerationType.getBackingType(deploymentAccessor).basicTypeStreamSignature
    }

    def private dispatch String typeStreamFTypeSignature(FUnionType fUnionType, DeploymentInterfacePropertyAccessor deploymentAccessor) {
    	return 'typeOutputStream.writeVariantType();'
    }

    def private String getElementsTypeStreamSignature(FStructType fStructType, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        var signature = fStructType.elements.map[type.typeStreamSignature(deploymentAccessor)].join

        if (fStructType.base != null)
            signature = fStructType.base.getElementsTypeStreamSignature(deploymentAccessor) + signature

        return signature
    }

	def List<FType> getDirectlyReferencedTypes(FType type) {
		val directlyReferencedTypes = newLinkedList

		directlyReferencedTypes.addFTypeDirectlyReferencedTypes(type)

		return directlyReferencedTypes
	}

    def private dispatch addFTypeDirectlyReferencedTypes(List<FType> list, FStructType fType) {
    	list.addAll(fType.elements.filter[type.derived != null].map[type.derived])

        if (fType.base != null)
        	list.add(fType.base)
    }

    def private dispatch addFTypeDirectlyReferencedTypes(List<FType> list, FEnumerationType fType) {
        if (fType.base != null)
            list.add(fType.base)
    }

    def private dispatch addFTypeDirectlyReferencedTypes(List<FType> list, FArrayType fType) {
        if (fType.elementType.derived != null)
        	list.add(fType.elementType.derived)
    }

    def private dispatch addFTypeDirectlyReferencedTypes(List<FType> list, FUnionType fType) {
    	list.addAll(fType.elements.filter[type.derived != null].map[type.derived])

        if (fType.base != null)
            list.add(fType.base)
    }

    def private dispatch addFTypeDirectlyReferencedTypes(List<FType> list, FMapType fType) {
        if (fType.keyType.derived != null)
            list.add(fType.keyType.derived)

        if (fType.valueType.derived != null)
            list.add(fType.valueType.derived)
    }

    def private dispatch addFTypeDirectlyReferencedTypes(List<FType> list, FTypeDef fType) {
        if (fType.actualType.derived != null)
            list.add(fType.actualType.derived)
    }


    def boolean hasPolymorphicBase(FStructType fStructType) {
        if (fStructType.isPolymorphic)
            return true;

        return fStructType.base != null && fStructType.base.hasPolymorphicBase
    }

	def getSerialId(FStructType fStructType) {
		val hasher = Hashing::murmur3_32.newHasher
		hasher.putFTypeObject(fStructType);
		return hasher.hash.asInt
	}

	def private dispatch void putFTypeObject(Hasher hasher, FStructType fStructType) {
		if (fStructType.base != null)
			hasher.putFTypeObject(fStructType.base)

		hasher.putString('FStructType', Charsets::UTF_8)
		fStructType.elements.forEach[
			hasher.putFTypeRef(type)
			// avoid cases where the positions of 2 consecutive elements of the same type are switched
			hasher.putString(name, Charsets::UTF_8)
		]
	}

	def private dispatch void putFTypeObject(Hasher hasher, FEnumerationType fEnumerationType) {
		if (fEnumerationType.base != null)
		  hasher.putFTypeObject(fEnumerationType.base)

        hasher.putString('FEnumerationType', Charsets::UTF_8)
        hasher.putInt(fEnumerationType.enumerators.size)
	}

    def private dispatch void putFTypeObject(Hasher hasher, FArrayType fArrayType) {
    	hasher.putString('FArrayType', Charsets::UTF_8)
    	hasher.putFTypeRef(fArrayType.elementType)
    }

    def private dispatch void putFTypeObject(Hasher hasher, FUnionType fUnionType) {
    	if (fUnionType.base != null)
    	   hasher.putFTypeObject(fUnionType.base)

        hasher.putString('FUnionType', Charsets::UTF_8)
        fUnionType.elements.forEach[hasher.putFTypeRef(type)]
    }

    def private dispatch void putFTypeObject(Hasher hasher, FMapType fMapType) {
    	hasher.putString('FMapType', Charsets::UTF_8)
    	hasher.putFTypeRef(fMapType.keyType)
    	hasher.putFTypeRef(fMapType.valueType)
    }

    def private dispatch void putFTypeObject(Hasher hasher, FTypeDef fTypeDef) {
    	hasher.putFTypeRef(fTypeDef.actualType)
    }

    def private void putFTypeRef(Hasher hasher, FTypeRef fTypeRef) {
    	if (fTypeRef.derived != null)
    	   hasher.putFTypeObject(fTypeRef.derived)
    	else
    	   hasher.putString(fTypeRef.predefined.name, Charsets::UTF_8);
    }

    def boolean hasDerivedFStructTypes(FStructType fStructType) {
        return EcoreUtil$UsageCrossReferencer::find(fStructType, fStructType.model.eResource.resourceSet).exists[
            EObject instanceof FStructType && (EObject as FStructType).base == fStructType
        ]
    }

    def getDerivedFStructTypes(FStructType fStructType) {
        return EcoreUtil$UsageCrossReferencer::find(fStructType, fStructType.model.eResource.resourceSet)
                .map[EObject]
                .filter[it instanceof FStructType]
                .map[it as FStructType]
                .filter[base == fStructType]
    }

    def generateCppNamespace(FModel fModel) '''
        «fModel.namespaceAsList.map[toString].join("::")»::'''

    def generateNamespaceBeginDeclaration(FModel fModel) '''
        «FOR subnamespace : fModel.namespaceAsList»
            namespace «subnamespace» {
        «ENDFOR»
    '''

    def generateNamespaceEndDeclaration(FModel fModel) '''
        «FOR subnamespace : fModel.namespaceAsList.reverse»
            } // namespace «subnamespace»
        «ENDFOR»
    '''

    def isFireAndForget(FMethod fMethod) {
        return !fMethod.fireAndForget.nullOrEmpty
    }

    def getFilePath(Resource resource) {
        if (resource.URI.file)
            return resource.URI.toFileString

        val platformPath = new Path(resource.URI.toPlatformString(true))
        val file =  ResourcesPlugin::getWorkspace().getRoot().getFile(platformPath);

        return file.location.toString
    }
    
    def getHeader() {
        val deflt = DefaultScope::INSTANCE.getNode(PreferenceConstants::SCOPE).get(PreferenceConstants::P_LICENSE, "");
        return InstanceScope::INSTANCE.getNode(PreferenceConstants::SCOPE).get(PreferenceConstants::P_LICENSE, deflt);
    }

    def generateCommonApiLicenseHeader() '''
        /*
        * This file was generated by the CommonAPI Generators.
        *
        «getHeader()»
        */
    '''
}
