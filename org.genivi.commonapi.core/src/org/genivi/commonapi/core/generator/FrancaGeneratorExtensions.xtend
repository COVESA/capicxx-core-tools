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
import java.io.File
import java.io.IOException
import java.math.BigInteger
import java.util.ArrayList
import java.util.Collection
import java.util.HashMap
import java.util.HashSet
import java.util.LinkedHashMap
import java.util.LinkedList
import java.util.List
import java.util.Map
import java.util.jar.Manifest
import org.eclipse.core.resources.ResourcesPlugin
import org.eclipse.core.runtime.Path
import org.eclipse.emf.common.util.EList
import org.eclipse.emf.ecore.EObject
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.emf.ecore.util.EcoreUtil
import org.franca.core.franca.FArgument
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FConstantDef
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FEnumerator
import org.franca.core.franca.FExpression
import org.franca.core.franca.FField
import org.franca.core.franca.FInitializerExpression
import org.franca.core.franca.FIntegerConstant
import org.franca.core.franca.FIntegerInterval
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMapType
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModel
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FQualifiedElementRef
import org.franca.core.franca.FStringConstant
import org.franca.core.franca.FStructType
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.FUnionType
import org.franca.core.franca.FVersion
import org.franca.core.franca.FrancaFactory
import org.franca.deploymodel.dsl.fDeploy.FDArray
import org.franca.deploymodel.dsl.fDeploy.FDAttribute
import org.franca.deploymodel.dsl.fDeploy.FDBroadcast
import org.franca.deploymodel.dsl.fDeploy.FDEnumeration
import org.franca.deploymodel.dsl.fDeploy.FDInterface
import org.franca.deploymodel.dsl.fDeploy.FDMethod
import org.franca.deploymodel.dsl.fDeploy.FDModel
import org.franca.deploymodel.dsl.fDeploy.FDProvider
import org.franca.deploymodel.dsl.fDeploy.FDStruct
import org.franca.deploymodel.dsl.fDeploy.FDTypeDef
import org.franca.deploymodel.dsl.fDeploy.FDTypes
import org.franca.deploymodel.dsl.fDeploy.FDUnion
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.deployment.PropertyAccessor.DefaultEnumBackingType
import org.genivi.commonapi.core.deployment.PropertyAccessor.EnumBackingType
import org.genivi.commonapi.core.preferences.FPreferences
import org.genivi.commonapi.core.preferences.PreferenceConstants
import org.osgi.framework.FrameworkUtil

import static com.google.common.base.Preconditions.*

import static extension java.lang.Integer.*

class FrancaGeneratorExtensions {

    def boolean isComplex(String _typeName) {
        if (_typeName == "bool" ||
            _typeName == "int8" ||
            _typeName == "int16" ||
            _typeName == "int32" ||
            _typeName == "int64" ||
            _typeName == "uint8" ||
            _typeName == "uint16" ||
            _typeName == "uint32" ||
            _typeName == "uint64") {
            return false
        }
        return true
    }

    def String isSigned(String _typeName) {
        if (_typeName == "Int8" ||
            _typeName == "Int16" ||
            _typeName == "Int32" ||
            _typeName == "Int64") {
            return "true"
        }
        return "false"
    }

    def String getBitWidth(FBasicTypeId _typeId) {
        if (_typeId == FBasicTypeId.BOOLEAN ||
            _typeId == FBasicTypeId.INT8 ||
            _typeId == FBasicTypeId.UINT8)
            return "8"

        if (_typeId == FBasicTypeId.INT16 ||
            _typeId == FBasicTypeId.UINT16)
            return "16"

        if (_typeId == FBasicTypeId.INT32 ||
            _typeId == FBasicTypeId.UINT32)
            return "32"

        if (_typeId == FBasicTypeId.INT64 ||
            _typeId == FBasicTypeId.UINT64)
            return "64"

        return ""
    }
    
    def boolean isSignedBackingType(String _backingType) {
        return (_backingType == "int8_t" ||
                _backingType == "int16_t" ||
                _backingType == "int32_t" ||
                _backingType == "int64_t")
    }
    
    def String doCast(String _value, String _backingType) {
        if (_backingType.isSignedBackingType) {
            var BigInteger itsValue = new BigInteger(_value)
            var BigInteger itsSignedMax
            var BigInteger itsUnsignedMax
            
            if (_backingType == "int8_t") {
                itsSignedMax = new BigInteger("127");
                itsUnsignedMax = new BigInteger("255");
            } else if (_backingType == "int16_t") {
                itsSignedMax = new BigInteger("32767");
                itsUnsignedMax = new BigInteger("65535");
            } else if (_backingType == "int32_t") {
                itsSignedMax = new BigInteger("2147483647");
                itsUnsignedMax = new BigInteger("4294967295");
            } else {
                itsSignedMax = new BigInteger("9223372036854775807");
                itsUnsignedMax = new BigInteger("18446744073709551615");
            }                
            
            if (itsValue.compareTo(itsSignedMax) == 1 &&
                itsValue.compareTo(itsUnsignedMax) != 1) {
                itsValue = itsValue.subtract(itsUnsignedMax).subtract(BigInteger.ONE);
                return itsValue.toString
            }
        }
        
        return _value
    }

    def String generateIndent(int _indent) {
        var String  empty = ""
        var int i = 0
        while (i < _indent) {
            empty = empty + "    "
            i = i + 1
        }
        return empty
    }

    def String getRelativeName(FModelElement _element) {
        if (_element instanceof FTypeCollection)
            return ""
        val String next = (_element.eContainer as FModelElement).relativeName
        if (next != "")
            return next + "_" + _element.name
        return _element.name
    }

    def String getFullyQualifiedName(FModelElement fModelElement) {
        if (fModelElement.eContainer instanceof FModel)
            return (fModelElement.eContainer as FModel).name + '.' + fModelElement.elementName
        return (fModelElement.eContainer as FModelElement).fullyQualifiedName + '.' + fModelElement.elementName
    }

    def String getInterfaceVersion(FInterface _interface) {
        return "v" + _interface.version.major + "_" +_interface.version.minor
    }

    def String getFullyQualifiedNameWithVersion(FInterface _interface) {
        return _interface.getFullyQualifiedName + ":" + getInterfaceVersion(_interface)
    }   

    def String getFullyQualifiedCppName(FModelElement fModelElement) {
        if (fModelElement.eContainer instanceof FModel) {
            val containerName = (fModelElement.eContainer as FModel).name
            var prefix = "::"
            if (fModelElement instanceof FTypeCollection) {
                val FVersion itsVersion = fModelElement.version
                if (itsVersion != null) {
                    prefix = fModelElement.versionPrefix
                }
            }
            return (prefix + containerName + "::" + fModelElement.elementName).replace(".", "::")
        }
        return ((fModelElement.eContainer as FModelElement).fullyQualifiedCppName + "::" + fModelElement.elementName).replace(".", "::")
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
        if (fModelElement.eContainer != null && fModelElement.eContainer instanceof FModel) {
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
        var definePrefix = ""
        if (fModelElement instanceof FTypeCollection)
            if (fModelElement.version != null)
                definePrefix = "V" + fModelElement.version.major.toString() + "_"

        val defineSuffix = '_' + fModelElement.elementName.splitCamelCase.join('_')

        if (fModelElement.eContainer instanceof FModelElement)
            return definePrefix + (fModelElement.eContainer as FModelElement).defineName + defineSuffix

        return definePrefix + (fModelElement.eContainer as FModel).defineName + defineSuffix
    }

    def getDefineName(FModel fModel) {
        fModel.name.toUpperCase.replace('.', '_')
    }

    def getElementName(FModelElement fModelElement) {
        if ((fModelElement.name == null || fModelElement.name == "") && fModelElement instanceof FTypeCollection) {
            return "__Anonymous__"
        }
        else {
            return fModelElement.name
        }
    }

    def private dispatch List<String> getNamespaceAsList(FModel fModel) {
        newArrayList(fModel.name.split("\\."))
    }

    def private dispatch List<String> getNamespaceAsList(FModelElement fModelElement) {
        val namespaceList = fModelElement.eContainer.namespaceAsList
        val isRootElement = fModelElement.eContainer instanceof FModel

        if (!isRootElement)
            namespaceList.add((fModelElement.eContainer as FModelElement).elementName)

        return namespaceList
    }

    def private getSubnamespaceList(FModelElement destination, EObject source) {
        val sourceNamespaceList = source.namespaceAsList
        val destinationNamespaceList = destination.namespaceAsList
        val maxCount = Ints::min(sourceNamespaceList.size, destinationNamespaceList.size)
        var dropCount = 0

        while (dropCount < maxCount &&
            sourceNamespaceList.get(dropCount).equals(destinationNamespaceList.get(dropCount)))
            dropCount = dropCount + 1

        return destinationNamespaceList.drop(dropCount)
    }

    def getRelativeNameReference(FModelElement destination, EObject source) {
        var nameReference = destination.elementName

        if (destination.eContainer != null && !destination.eContainer.equals(source)) {
            val subnamespaceList = destination.getSubnamespaceList(source)
            if (!subnamespaceList.empty)
                nameReference = subnamespaceList.join('::') + '::' + nameReference
        }

        return nameReference
    }

    def getHeaderFile(FTypeCollection fTypeCollection) {
        fTypeCollection.elementName + ".hpp"
    }

    def getInstanceHeaderFile(FTypeCollection fTypeCollection) {
        fTypeCollection.elementName + "InstanceIds.hpp"
    }

    def getHeaderPath(FTypeCollection fTypeCollection) {
        fTypeCollection.versionPathPrefix + fTypeCollection.model.directoryPath + '/' + fTypeCollection.headerFile
    }

    def getInstanceHeaderPath(FTypeCollection fTypeCollection) {
        fTypeCollection.versionPathPrefix + fTypeCollection.model.directoryPath + '/' + fTypeCollection.instanceHeaderFile
    }

    def getSourceFile(FTypeCollection fTypeCollection) {
        fTypeCollection.elementName + ".cpp"
    }

    def getSourcePath(FTypeCollection fTypeCollection) {
        fTypeCollection.versionPathPrefix + fTypeCollection.model.directoryPath + '/' + fTypeCollection.sourceFile
    }

    def getProxyBaseHeaderFile(FInterface fInterface) {
        fInterface.elementName + "ProxyBase.hpp"
    }

    def getProxyBaseHeaderPath(FInterface fInterface) {
        fInterface.versionPathPrefix + fInterface.model.directoryPath + '/' + fInterface.proxyBaseHeaderFile
    }

    def getProxyBaseClassName(FInterface fInterface) {
        fInterface.elementName + 'ProxyBase'
    }

    def getProxyHeaderFile(FInterface fInterface) {
        fInterface.elementName + "Proxy.hpp"
    }

    def getProxyDumpWrapperHeaderFile(FInterface fInterface) {
        fInterface.elementName + "ProxyDumpWrapper.hpp"
    }

    def getProxyHeaderPath(FInterface fInterface) {
        fInterface.versionPathPrefix + fInterface.model.directoryPath + '/' + fInterface.proxyHeaderFile
    }

    def getProxyDumpWrapperHeaderPath(FInterface fInterface) {
        fInterface.versionPathPrefix + fInterface.model.directoryPath + '/' + fInterface.proxyDumpWrapperHeaderFile
    }

    def getStubDefaultHeaderFile(FInterface fInterface) {
        fInterface.elementName + "Stub" + skeletonNamePostfix + ".hpp"
    }

    def getSkeletonNamePostfix() {
        FPreferences::instance.getPreference(PreferenceConstants::P_SKELETONPOSTFIX, "Default")
    }

    def getHeaderDefineName(FInterface fInterface) {
        fInterface.defineName + "_STUB_" + skeletonNamePostfix.toUpperCase()
    }

    def getStubDefaultHeaderPath(FInterface fInterface) {
        fInterface.versionPathPrefix + fInterface.model.directoryPath + '/' + fInterface.stubDefaultHeaderFile
    }

    def getStubDefaultClassName(FInterface fInterface) {
        fInterface.elementName + "Stub" + skeletonNamePostfix
    }

    def getStubDefaultSourceFile(FInterface fInterface) {
        fInterface.elementName + "Stub" + skeletonNamePostfix + ".cpp"
    }

    def getStubDefaultSourcePath(FInterface fInterface) {
        fInterface.versionPathPrefix + fInterface.model.directoryPath + '/' + fInterface.getStubDefaultSourceFile
    }

    def getStubRemoteEventClassName(FInterface fInterface) {
        fInterface.elementName + 'StubRemoteEvent'
    }

    def getStubAdapterClassName(FInterface fInterface) {
        fInterface.elementName + 'StubAdapter'
    }

    def getStubCommonAPIClassName(FInterface fInterface) {
        'CommonAPI::Stub<' + fInterface.stubAdapterClassName + ', ' + fInterface.stubRemoteEventClassName + '>'
    }

    def getStubHeaderFile(FInterface fInterface) {
        fInterface.elementName + "Stub.hpp"
    }

    def getStubHeaderPath(FInterface fInterface) {
        fInterface.versionPathPrefix + fInterface.model.directoryPath + '/' + fInterface.stubHeaderFile
    }

    def getSerrializationFile(FInterface fInterface) {
        fInterface.elementName + "Serrialization.hpp"
    }

    def getSerrializationHeaderPath(FInterface fInterface) {
        fInterface.versionPathPrefix + fInterface.model.directoryPath + '/' + fInterface.serrializationFile
    }

    def generateSelectiveBroadcastStubIncludes(FInterface fInterface, Collection<String> generatedHeaders,
        Collection<String> libraryHeaders) {
        if (!fInterface.broadcasts.filter[!selective].empty) {
            libraryHeaders.add("unordered_set")
        }

        return null
    }

    def generateSelectiveBroadcastProxyIncludes(FInterface fInterface, Collection<String> generatedHeaders,
        Collection<String> libraryHeaders) {
        if (!fInterface.broadcasts.filter[!selective].empty) {
            libraryHeaders.add("CommonAPI/Types.hpp")
        }

        return null
    }

    def getStubClassName(FInterface fInterface) {
        fInterface.elementName + 'Stub'
    }

    def getStubFullClassName(FInterface fInterface) {
        fInterface.getFullName + 'Stub'
    }

    def hasAttributes(FInterface fInterface) {
        !fInterface.attributes.empty
    }

    def hasBroadcasts(FInterface fInterface) {
        !fInterface.broadcasts.empty
    }

    def hasSelectiveBroadcasts(FInterface fInterface) {
        !fInterface.broadcasts.filter[selective].empty
    }

    def Collection<FMethod> getMethodsWithError(FInterface _interface) {
        val Map<String, FMethod> itsMethods = new HashMap<String, FMethod>()
        for (method : _interface.methods.filter[errors != null]) {
            var existing = itsMethods.get(method.name)
            if (existing == null) {
                itsMethods.put(method.name, method)
            } else {
                val List<FEnumerator> itsAdditionals = new ArrayList<FEnumerator>()
                for (e : method.errors.enumerators) {
                    var found = false
                    for (f : existing.errors.enumerators) {
                        if (f.name == e.name) {
                            found = true
                        }
                    }
                    if (!found)
                        itsAdditionals.add(e)
                }
                existing.errors.enumerators.addAll(itsAdditionals)
            }
        }
        return itsMethods.values
    }

    def generateDefinition(FMethod fMethod, boolean _isDefault) {
        fMethod.generateDefinitionWithin(null, _isDefault)
    }

    def generateDefinitionWithin(FMethod fMethod, String parentClassName, boolean _isDefault) {
        var definition = 'void '
        if (FTypeGenerator::isdeprecated(fMethod.comment))
            definition = "COMMONAPI_DEPRECATED " + definition
        if (!parentClassName.nullOrEmpty)
            definition = definition + parentClassName + '::'

        definition = definition + fMethod.elementName + '(' + fMethod.generateDefinitionSignature(_isDefault) + ')'

        return definition
    }

    def generateDefinitionSignature(FMethod fMethod, boolean _isDefault) {
        var signature = fMethod.inArgs.map['const ' + getTypeName(fMethod, true) + ' &_' + elementName].join(', ')

        if (!fMethod.inArgs.empty)
            signature = signature + ', '

        signature = signature + 'CommonAPI::CallStatus &_internalCallStatus'

        if (fMethod.hasError)
            signature = signature + ', ' + fMethod.getErrorNameReference(fMethod.eContainer) + ' &_error'

        if (!fMethod.outArgs.empty)
            signature = signature + ', ' + fMethod.outArgs.map[getTypeName(fMethod, true) + ' &_' + elementName].join(', ')

        if (!fMethod.fireAndForget) {
            signature += ", const CommonAPI::CallInfo *_info"
            if (_isDefault)
                signature += " = nullptr"
        }
        return signature
    }

    def generateStubSignature(FMethod fMethod) {
        var signature = 'const std::shared_ptr<CommonAPI::ClientId> _client'

        if (!fMethod.inArgs.empty)
            signature = signature + ', '

        signature = signature + fMethod.inArgs.map[getTypeName(fMethod, true) + ' _' + elementName].join(', ')

        if (!fMethod.isFireAndForget) {
            if (signature != "")
                signature = signature + ", ";
            signature = signature + fMethod.elementName + "Reply_t _reply";
        }

        return signature
    }

    def generateOverloadedStubSignature(FMethod fMethod, LinkedHashMap<String, Boolean> replies) {
        var signature = 'const std::shared_ptr<CommonAPI::ClientId> _client'
        
        if(!fMethod.isFireAndForget && replies.containsValue(true)) {
            //replies containing error replies
            signature = signature + ', const CommonAPI::CallId_t _callId'
        }

        if (!fMethod.inArgs.empty)
            signature = signature + ', '

        signature = signature + fMethod.inArgs.map[getTypeName(fMethod, true) + ' _' + elementName].join(', ')

        if (!fMethod.isFireAndForget) {
            for (Map.Entry<String, Boolean> entry : replies.entrySet) {
                var methodName = entry.key
                var isErrorReply = entry.value
                signature = signature + ", " + methodName + 'Reply_t'
                if(isErrorReply) {
                    signature = signature + ' _' + methodName + 'Reply'
                } else {
                    signature = signature + ' _reply'
                }
            }
        }
        return signature
    }

    def generateStubReplySignature(FMethod fMethod) {
        var signature = ''

        if (fMethod.hasError)
            signature = signature + fMethod.getErrorNameReference(fMethod.eContainer) + ' _error'
        if (fMethod.hasError && !fMethod.outArgs.empty)
            signature = signature + ', '

        if (!fMethod.outArgs.empty)
            signature = signature + fMethod.outArgs.map[getTypeName(fMethod, true) + ' _' + elementName].join(', ')

        return signature
    }
    
    def generateStubErrorReplySignature(FBroadcast fBroadcast, PropertyAccessor deploymentAccessor) {
        checkArgument(fBroadcast.isErrorType(deploymentAccessor), 'FBroadcast is no error type: ' + fBroadcast)
        
        var signature = 'const CommonAPI::CallId_t _callId'

        if (!fBroadcast.errorArgs(deploymentAccessor).empty)
            signature = signature + ', ' + fBroadcast.errorArgs(deploymentAccessor).map[getTypeName(fBroadcast, true) + ' _' + elementName].join(', ')

        return signature
    }
    
    def errorReplyTypes(FBroadcast fBroadcast, FMethod fMethod, PropertyAccessor deploymentAccessor) {
        checkArgument(fBroadcast.isErrorType(deploymentAccessor), 'FBroadcast is no valid error type: ' + fBroadcast)

        var types = "const CommonAPI::CallId_t";
        var errorTypes = fBroadcast.errorArgs(deploymentAccessor).map[getTypeName(fBroadcast, true)].join(', ')
        if(errorTypes != "") {
            types = types + ", "
        }
        types = types + errorTypes
        return types
    }
    
    def errorReplyCallbackName(FBroadcast fBroadcast, PropertyAccessor deploymentAccessor) {
        checkArgument(fBroadcast.isErrorType(deploymentAccessor), 'FBroadcast is no valid error type: ' + fBroadcast)
        
        return fBroadcast.elementName + "Callback"
    }
    
    def errorReplyCallbackBindArgs(FBroadcast fBroadcast, PropertyAccessor deploymentAccessor) {
        checkArgument(fBroadcast.isErrorType(deploymentAccessor), 'FBroadcast is no valid error type: ' + fBroadcast)

        var bindArgs = "std::placeholders::_1, " + "\"" + deploymentAccessor.getErrorName(fBroadcast) + "\""
        if(fBroadcast.errorArgs(deploymentAccessor).size > 1) {
            for(var i=0; i < fBroadcast.errorArgs(deploymentAccessor).size; i++) {
                bindArgs = bindArgs + ', std::placeholders::_' + (i+2) 
            }
        }
        return bindArgs
    }
    
    def generateErrorReplyCallbackSignature(FBroadcast fBroadcast, FMethod fMethod, PropertyAccessor deploymentAccessor) {
        checkArgument(fBroadcast.isErrorType(deploymentAccessor), 'FBroadcast is no valid error type: ' + fBroadcast)
        
        var signature = 'const CommonAPI::CallId_t _callId'
        var signatureOut = fBroadcast.outArgs.map[getTypeName(fBroadcast, true) + ' _' + it.elementName].join(', ')
        if(signatureOut != "") {
            signature = signature + ", "
        }
        signature = signature + signatureOut
        return signature
    }
    
    def isErrorType(FBroadcast fBroadcast, PropertyAccessor deploymentAccessor) {        
        return (deploymentAccessor.getBroadcastType(fBroadcast) == PropertyAccessor.BroadcastType.error)
    }
    
    def isErrorType(FBroadcast fBroadcast, FMethod fMethod, PropertyAccessor deploymentAccessor) {
        return (fBroadcast.isErrorType(deploymentAccessor) &&
            deploymentAccessor.getErrors(fMethod) != null &&
            deploymentAccessor.getErrors(fMethod).contains(fBroadcast.elementName))
    }
    
    def errorName(FBroadcast fBroadcast, PropertyAccessor deploymentAccessor) {
        checkArgument(fBroadcast.isErrorType(deploymentAccessor), 'FBroadcast is no valid error type: ' + fBroadcast)
        checkArgument(!fBroadcast.outArgs.empty, 'FBroadcast of type error has no required error name: ' + fBroadcast)
        checkArgument(fBroadcast.outArgs.get(0).getTypeName(fBroadcast, true) == 'std::string',
        'FBroadcast of type error does not contain a string as first argument (error name): ' + fBroadcast)
        
        return fBroadcast.outArgs.get(0).elementName
    }
    
    def errorArgs(FBroadcast fBroadcast, PropertyAccessor deploymentAccessor) {
        checkArgument(fBroadcast.isErrorType(deploymentAccessor), 'FBroadcast is no error type: ' + fBroadcast)
        checkArgument(!fBroadcast.outArgs.empty, 'FBroadcast of type error has no required error name: ' + fBroadcast)
        
        return fBroadcast.outArgs.subList(1, fBroadcast.outArgs.size)
    }

    def String generateDummyValue(FTypeRef typeRef) {
        var String retval = ""
        if (typeRef.derived == null && typeRef.predefined != null) {
            retval +=
                switch typeRef.predefined {
                    case FBasicTypeId::BOOLEAN:     "false"
                    case FBasicTypeId::INT8:        "0"
                    case FBasicTypeId::UINT8:       "0u"
                    case FBasicTypeId::INT16:       "0"
                    case FBasicTypeId::UINT16:      "0u"
                    case FBasicTypeId::INT32:       "0"
                    case FBasicTypeId::UINT32:      "0ul"
                    case FBasicTypeId::INT64:       "0"
                    case FBasicTypeId::UINT64:      "0ull"
                    case FBasicTypeId::FLOAT:       "0.0f"
                    case FBasicTypeId::DOUBLE:      "0.0"
                    case FBasicTypeId::STRING:      "\"\""
                    case FBasicTypeId::BYTE_BUFFER: "CommonAPI::ByteBuffer()"
                    default: ""
                }
        }
        return retval
    }

    def String generateDummyArgumentInitializations(FMethod fMethod) {
       var String retval = ""
       for(list_element : fMethod.outArgs) {
          retval += getTypeName(list_element, fMethod, true)  + ' ' + list_element.elementName
          retval += list_element.type.generateDummyArgumentInitialization(list_element, fMethod)
          retval += ";\n"
       }
       return retval
    }

    private def String generateDummyArgumentInitialization(FTypeRef typeRef, FArgument list_element, FMethod fMethod) {
        if (list_element.array) {
            " = {}"
        } else if (typeRef.derived != null) {
            typeRef.derived.generateDummyArgumentInitialization(list_element, fMethod)
        } else if (typeRef.predefined != null) {
            typeRef.predefined.generateDummyArgumentInitialization
        } else {
            ""
        }
    }

    private def generateDummyArgumentInitialization(FType type, FArgument list_element, FMethod fMethod) {
        if ((type instanceof FArrayType) || (type instanceof FCompoundType)) {
            " = {}"
        } else if (type instanceof FEnumerationType) {
            if (type.enumerators.empty) {
                " = " + getTypeName(list_element, fMethod, true) + "(0u)"
            } else {
                " = " + getTypeName(list_element, fMethod, true) + "::" + enumPrefix + type.enumerators.get(0).elementName
            }
        } else if (type instanceof FIntegerInterval) {
            " = " + type.lowerBound
        } else if (type instanceof FTypeDef) {
            type.actualType.generateDummyArgumentInitialization(list_element, fMethod)
        } else {
            ""
        }
    }

    private def generateDummyArgumentInitialization(FBasicTypeId basicType) {
        switch basicType {
            case FBasicTypeId::BOOLEAN:     " = false"
            case FBasicTypeId::INT8:        " = 0"
            case FBasicTypeId::UINT8:       " = 0u"
            case FBasicTypeId::INT16:       " = 0"
            case FBasicTypeId::UINT16:      " = 0u"
            case FBasicTypeId::INT32:       " = 0"
            case FBasicTypeId::UINT32:      " = 0ul"
            case FBasicTypeId::INT64:       " = 0"
            case FBasicTypeId::UINT64:      " = 0ull"
            case FBasicTypeId::FLOAT:       " = 0.0f"
            case FBasicTypeId::DOUBLE:      " = 0.0"
            case FBasicTypeId::STRING:      " = \"\""
            case FBasicTypeId::BYTE_BUFFER: " = {}"
            default: ""
        }
    }

    def generateDummyArgumentDefinitions(FMethod fMethod) {
      var definition = ''
      if (fMethod.hasError)
         definition = fMethod.getErrorNameReference(fMethod.eContainer) + ' error;\n'
      if (!fMethod.outArgs.empty)
         definition = definition + generateDummyArgumentInitializations(fMethod)
      return definition
    }

    def generateDummyArgumentList(FMethod fMethod) {
        var arguments = ''
        if (fMethod.hasError)
            arguments = "error"
        if (!fMethod.outArgs.empty) {
            if (arguments != '')
                arguments = arguments + ', '
            arguments = arguments + fMethod.outArgs.map[elementName].join(', ')
        }
        return arguments
    }

    def generateFireSelectiveSignatur(FBroadcast fBroadcast, FInterface fInterface) {
        var signature = 'const std::shared_ptr<CommonAPI::ClientId> _client'

        if (!fBroadcast.outArgs.empty)
            signature = signature + ', '

        signature = signature +
            fBroadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')

        return signature
    }

    def generateSendSelectiveSignatur(FBroadcast fBroadcast, FInterface fInterface, Boolean withDefault) {
        var signature = fBroadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')

        if (!fBroadcast.outArgs.empty)
            signature = signature + ', '

        signature = signature + 'const std::shared_ptr<CommonAPI::ClientIdList> _receivers'

        if (withDefault)
            signature = signature + ' = nullptr'

        return signature
    }

    def generateStubSignatureErrorsAndOutArgs(FMethod fMethod) {
        var signature = ''

        if (fMethod.hasError)
            signature = signature + fMethod.getErrorNameReference(fMethod.eContainer) + ' &_error'
        if (fMethod.hasError && !fMethod.outArgs.empty)
            signature = signature + ', '

        if (!fMethod.outArgs.empty)
            signature = signature + fMethod.outArgs.map[getTypeName(fMethod, true) + ' &_' + elementName].join(', ')

        return signature
    }

    def generateArgumentsToStub(FMethod fMethod) {
        var arguments = ' _client'

        if (!fMethod.inArgs.empty)
            arguments = arguments + ', ' + fMethod.inArgs.map[elementName].join(', ')

        if ((fMethod.hasError || !fMethod.outArgs.empty))
            arguments = arguments + ', '

        if (fMethod.hasError)
            arguments = arguments + '_error'

        if (fMethod.hasError && !fMethod.outArgs.empty)
            arguments = arguments + ', '

        if (!fMethod.outArgs.empty)
            arguments = arguments + fMethod.outArgs.map[elementName].join(', ')

        return arguments
    }

    def generateArgumentsToStubOldStyle(FMethod fMethod) {
        var arguments = fMethod.inArgs.map[elementName].join(', ')

        if ((fMethod.hasError || !fMethod.outArgs.empty) && !fMethod.inArgs.empty)
            arguments = arguments + ', '

        if (fMethod.hasError)
            arguments = arguments + '_error'
        if (fMethod.hasError && !fMethod.outArgs.empty)
            arguments = arguments + ', '

        if (!fMethod.outArgs.empty)
            arguments = arguments + fMethod.outArgs.map[elementName].join(', ')

        return arguments
    }

    def generateAsyncDefinition(FMethod fMethod, boolean _isDefault) {
        fMethod.generateAsyncDefinitionWithin(null, _isDefault)
    }

    def generateAsyncDefinitionWithin(FMethod fMethod, String parentClassName, boolean _isDefault) {
        var definition = 'std::future<CommonAPI::CallStatus> '
        if (FTypeGenerator::isdeprecated(fMethod.comment))
            definition = "COMMONAPI_DEPRECATED " + definition

        if (!parentClassName.nullOrEmpty) {
            definition = definition + parentClassName + '::'
        }

        definition = definition + fMethod.elementName + 'Async(' + fMethod.generateAsyncDefinitionSignature(_isDefault) + ')'

        return definition
    }

    def generateAsyncDefinitionSignature(FMethod fMethod, boolean _isDefault) {
        var signature = fMethod.inArgs.map['const ' + getTypeName(fMethod, true) + ' &_' + elementName].join(', ')
        if (!fMethod.inArgs.empty) {
            signature = signature + ', '
        }

        signature += fMethod.asyncCallbackClassName + ' _callback'
        if (_isDefault)
            signature += " = nullptr"
        signature += ", const CommonAPI::CallInfo *_info"
        if (_isDefault)
            signature += " = nullptr"

        return signature
    }

    def getErrorType(FMethod _method) {
        var errorType = ""
        if (_method.hasError) {
            errorType = _method.getErrorNameReference(_method.eContainer)
        }
        return errorType
    }

    def getInTypeList(FMethod _method) {
        return _method.inArgs.map[getTypeName(_method, true)].join(', ')
    }

    def getOutTypeList(FMethod _method) {
        return _method.outArgs.map[getTypeName(_method, true)].join(', ')
    }

    def getErrorAndOutTypeList(FMethod _method) {
        val errorType = _method.errorType
        val outTypes = _method.outTypeList

        var errorAndOutTypes = errorType
        if (errorType != "" && outTypes != "")
            errorAndOutTypes = errorAndOutTypes + ", "
        errorAndOutTypes = errorAndOutTypes + outTypes

        return errorAndOutTypes
    }

    def private String getBasicMangledName(FBasicTypeId basicType) {
        switch (basicType) {
            case FBasicTypeId::BOOLEAN:
                return "b"
            case FBasicTypeId::INT8:
                return "i8"
            case FBasicTypeId::UINT8:
                return "u8"
            case FBasicTypeId::INT16:
                return "i16"
            case FBasicTypeId::UINT16:
                return "u16"
            case FBasicTypeId::INT32:
                return "i32"
            case FBasicTypeId::UINT32:
                return "u32"
            case FBasicTypeId::INT64:
                return "i64"
            case FBasicTypeId::UINT64:
                return "u64"
            case FBasicTypeId::FLOAT:
                return "f"
            case FBasicTypeId::DOUBLE:
                return "d"
            case FBasicTypeId::STRING:
                return "s"
            case FBasicTypeId::BYTE_BUFFER:
                return "au8"
            default: {
                return null
            }
        }
    }

    def private dispatch String getDerivedMangledName(FEnumerationType fType) {
        "Ce" + fType.fullyQualifiedName
    }

    def private dispatch String getDerivedMangledName(FMapType fType) {
        "Cm" + fType.fullyQualifiedName
    }

    def private dispatch String getDerivedMangledName(FStructType fType) {
        "Cs" + fType.fullyQualifiedName
    }

    def private dispatch String getDerivedMangledName(FUnionType fType) {
        "Cv" + fType.fullyQualifiedName
    }

    def private dispatch String getDerivedMangledName(FArrayType fType) {
        "Ca" + fType.elementType.mangledName
    }

    def private dispatch String getDerivedMangledName(FTypeDef fType) {
        fType.actualType.mangledName
    }

    def private String getMangledName(FTypeRef fTypeRef) {
        if (fTypeRef.derived != null) {
            return fTypeRef.derived.derivedMangledName
        } else {
            return fTypeRef.predefined.basicMangledName
        }
    }

    def private getBasicAsyncCallbackClassName(FMethod fMethod) {
        fMethod.elementName.toFirstUpper + 'AsyncCallback'
    }

    def private int16Hash(String originalString) {
        val hash32bit = originalString.hashCode

        var hash16bit = hash32bit.bitwiseAnd(0xFFFF)
        hash16bit = hash16bit.bitwiseXor((hash32bit >> 16).bitwiseAnd(0xFFFF))

        return hash16bit
    }

    def private getMangledAsyncCallbackClassName(FMethod fMethod) {
        val baseName = fMethod.basicAsyncCallbackClassName + '_'
        var mangledName = ''
        for (outArg : fMethod.outArgs) {
            mangledName = mangledName + outArg.type.mangledName
        }
        mangledName = mangledName + fMethod.errorEnum?.name

        return baseName + mangledName.int16Hash
    }

    def String getAsyncCallbackClassName(FMethod fMethod) {
        if (fMethod.needsMangling) {
            return fMethod.mangledAsyncCallbackClassName
        } else {
            return fMethod.basicAsyncCallbackClassName
        }
    }

    def hasError(FMethod fMethod) {
        fMethod.errorEnum != null || fMethod.errors != null
    }

    def generateASyncTypedefSignature(FMethod fMethod) {
        var signature = 'const CommonAPI::CallStatus&'

        if (fMethod.hasError)
            signature = signature + ', const ' + fMethod.getErrorNameReference(fMethod.eContainer) + '&'

        if (!fMethod.outArgs.empty)
            signature = signature + ', ' + fMethod.outArgs.map['const ' + getTypeName(fMethod, true) + '&'].join(', ')

        return signature
    }

    def needsMangling(FMethod fMethod) {
        for (otherMethod : fMethod.containingInterface.methods) {
            if (otherMethod != fMethod && otherMethod.basicAsyncCallbackClassName == fMethod.basicAsyncCallbackClassName &&
                otherMethod.generateASyncTypedefSignature != fMethod.generateASyncTypedefSignature) {
                return true
            }
        }
        return false
    }

    def getErrorNameReference(FMethod fMethod, EObject source) {
        checkArgument(fMethod.hasError, 'FMethod has no error: ' + fMethod)
        if (fMethod.errorEnum != null) {
            return fMethod.errorEnum.getElementName(fMethod, true)
        }

        var errorNameReference = fMethod.errors.errorName
        errorNameReference = (fMethod.eContainer as FInterface).getRelativeNameReference(source) + '::' +
            errorNameReference
        return errorNameReference
    }

    def getClassName(FAttribute fAttribute) {
        fAttribute.elementName.toFirstUpper + 'Attribute'
    }

    def generateGetMethodDefinition(FAttribute fAttribute) {
        fAttribute.generateGetMethodDefinitionWithin(null)
    }

    def generateGetMethodDefinitionWithin(FAttribute fAttribute, String parentClassName) {
        var definition = fAttribute.className + '& '

        if (!parentClassName.nullOrEmpty)
            definition = parentClassName + '::' + definition + parentClassName + '::'

        definition = definition + 'get' + fAttribute.className + '()'

        if (FTypeGenerator::isdeprecated(fAttribute.comment))
            definition = "COMMONAPI_DEPRECATED " + definition

        return definition
    }

    def isReadonly(FAttribute fAttribute) {
        fAttribute.readonly
    }

    def isObservable(FAttribute fAttribute) {
        fAttribute.noSubscriptions == false
    }

    def getStubAdapterClassFireChangedMethodName(FAttribute fAttribute) {
        'fire' + fAttribute.elementName.toFirstUpper + 'AttributeChanged'
    }

    def getStubRemoteEventClassSetMethodName(FAttribute fAttribute) {
        'onRemoteSet' + fAttribute.elementName.toFirstUpper + 'Attribute'
    }

    def getStubRemoteEventClassChangedMethodName(FAttribute fAttribute) {
        'onRemote' + fAttribute.elementName.toFirstUpper + 'AttributeChanged'
    }

    def getStubClassGetMethodName(FAttribute fAttribute) {
        'get' + fAttribute.elementName.toFirstUpper + 'Attribute'
    }

    def getClassName(FBroadcast fBroadcast) {
        var className = fBroadcast.elementName.toFirstUpper

        if (fBroadcast.selective)
            className = className + 'Selective'

        className = className + 'Event'

        return className
    }

    def generateGetMethodDefinition(FBroadcast fBroadcast) {
        fBroadcast.generateGetMethodDefinitionWithin(null)
    }

    def String getTypes(FBroadcast _broadcast) {
        return _broadcast.outArgs.map[getTypeName(_broadcast, true)].join(', ')
    }

    def generateGetMethodDefinitionWithin(FBroadcast fBroadcast, String parentClassName) {
        var definition = fBroadcast.className + '& '

        if (!parentClassName.nullOrEmpty)
            definition = parentClassName + '::' + definition + parentClassName + '::'

        definition = definition + 'get' + fBroadcast.className + '()'
    
        if (FTypeGenerator::isdeprecated(fBroadcast.comment))
            definition = "COMMONAPI_DEPRECATED " + definition

        return definition
    }

    def getStubAdapterClassFireEventMethodName(FBroadcast fBroadcast) {
        'fire' + fBroadcast.elementName.toFirstUpper + 'Event'
    }

    def getStubAdapterClassFireSelectiveMethodName(FBroadcast fBroadcast) {
        'fire' + fBroadcast.elementName.toFirstUpper + 'Selective';
    }

    def getStubAdapterClassSendSelectiveMethodName(FBroadcast fBroadcast) {
        'send' + fBroadcast.elementName.toFirstUpper + 'Selective';
    }

    def getSubscribeSelectiveMethodName(FBroadcast fBroadcast) {
        'subscribeFor' + fBroadcast.elementName + 'Selective';
    }

    def getUnsubscribeSelectiveMethodName(FBroadcast fBroadcast) {
        'unsubscribeFrom' + fBroadcast.elementName + 'Selective';
    }

    def getSubscriptionChangedMethodName(FBroadcast fBroadcast) {
        'on' + fBroadcast.elementName.toFirstUpper + 'SelectiveSubscriptionChanged';
    }

    def getSubscriptionRequestedMethodName(FBroadcast fBroadcast) {
        'on' + fBroadcast.elementName.toFirstUpper + 'SelectiveSubscriptionRequested';
    }

    def getStubAdapterClassSubscribersMethodName(FBroadcast fBroadcast) {
        'getSubscribersFor' + fBroadcast.elementName.toFirstUpper + 'Selective';
    }

    def getStubAdapterClassSubscriberListPropertyName(FBroadcast fBroadcast) {
        'subscribersFor' + fBroadcast.elementName.toFirstUpper + 'Selective_';
    }

    def getStubSubscribeSignature(FBroadcast fBroadcast) {
        'const std::shared_ptr<CommonAPI::ClientId> clientId, bool& success'
    }

    def String getTypeName(FTypedElement _element, FModelElement _source, boolean _isOther) {
        var String typeName = _element.type.getElementType(_source, _isOther)

        if (_element.type.derived instanceof FStructType && (_element.type.derived as FStructType).hasPolymorphicBase)
            typeName = 'std::shared_ptr< ' + typeName + '>'

        if (_element.array) {
            typeName = 'std::vector< ' + typeName + ' >'
        }

        return typeName
    }

    def String getElementType(FTypeRef _typeRef, FModelElement _container, boolean _isOther) {
        var String typeName
        if (_typeRef.derived != null) {
               typeName = _typeRef.derived.getElementName(_container, _isOther)
        } else if (_typeRef.predefined != null) {
            typeName = _typeRef.predefined.primitiveTypeName
        }
        return typeName
    }

    def boolean isPolymorphic(FTypeRef typeRef) {
        return (typeRef.derived != null && typeRef.derived instanceof FStructType && (typeRef.derived as FStructType).polymorphic)
    }

    def getNameReference(FTypeRef destination, EObject source) {
        if (destination.derived != null)
            return destination.derived.getRelativeNameReference(source)
        return destination.predefined.primitiveTypeName
    }

    def getErrorName(FEnumerationType fMethodErrors) {
        checkArgument(fMethodErrors.eContainer instanceof FMethod, 'Not FMethod errors')
        (fMethodErrors.eContainer as FMethod).elementName + 'Error'
    }

    def getBackingType(FEnumerationType fEnumerationType, PropertyAccessor deploymentAccessor) {
        if (deploymentAccessor.getEnumBackingType(fEnumerationType) == EnumBackingType::UseDefault) {
            if (fEnumerationType.containingInterface != null) {
                switch (deploymentAccessor.getDefaultEnumBackingType(fEnumerationType.containingInterface)) {
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
        switch (deploymentAccessor.getEnumBackingType(fEnumerationType)) {
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
            default: {
                return FBasicTypeId::INT32
            }
        }
    }

    def String getBaseType(FEnumerationType _enumeration, FEnumerationType _other, String _backingType) {
        var String baseType
        if (_enumeration.base != null) {
            baseType = _enumeration.base.getElementName(_other, false)
        } else {
            baseType = "CommonAPI::Enumeration<" + _backingType + ">"
        }
        return baseType
    }

    def private getMaximumEnumerationValue(FEnumerationType _enumeration) {
        var BigInteger maximum = BigInteger.ZERO;
        for (literal : _enumeration.enumerators) {
            if (literal.value != null && literal.value != "") {
                val BigInteger literalValue = new BigInteger(literal.value.enumeratorValue)
                if (maximum < literalValue)
                    maximum = literalValue
            }
        }
        return maximum
    }

    def void setEnumerationValues(FEnumerationType _enumeration) {
        var BigInteger currentValue = BigInteger.ZERO
        val predefineEnumValues = new ArrayList<String>()

        // collect all predefined enum values
        for (literal : _enumeration.enumerators) {
            if (literal.value != null) {
                predefineEnumValues.add(literal.value.enumeratorValue)
            }
        }
        if (_enumeration.base != null) {
            setEnumerationValues(_enumeration.base)
            currentValue = getMaximumEnumerationValue(_enumeration.base) + BigInteger.ONE
        }

        for (literal : _enumeration.enumerators) {
            if (literal.value == null || literal.value == "") {
                // not predefined
                while (predefineEnumValues.contains(String.valueOf(currentValue))) {
                    // increment it, if this was found in the list of predefined values
                    currentValue += BigInteger.ONE
                }
                literal.setValue(toExpression(currentValue.toString()))
            } else {
                var enumValue = literal.value.enumeratorValue
                if (enumValue != null) {
                    try {
                        val BigInteger literalValue = new BigInteger(enumValue)
                        literal.setValue(toExpression(literalValue.toString()))
                    } catch (NumberFormatException e) {
                        literal.setValue(toExpression(currentValue.toString()))
                    }
                } else {
                    literal.setValue(toExpression(currentValue.toString()))
                }
            }
            currentValue += BigInteger.ONE
        }
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
            default: throw new IllegalArgumentException("Unsupported basic type: " + fBasicTypeId.getName)
        }
    }

    def String typeStreamSignature(FTypeRef fTypeRef, PropertyAccessor deploymentAccessor, FField forThisElement) {
        if (forThisElement.array) {
            var String ret = ""
            ret = ret + "typeOutputStream.beginWriteVectorType();\n"
            ret = ret + fTypeRef.actualTypeStreamSignature(deploymentAccessor)
            ret = ret + "typeOutputStream.endWriteVectorType();\n"
            return ret
        }
        return fTypeRef.actualTypeStreamSignature(deploymentAccessor)
    }

    def String actualTypeStreamSignature(FTypeRef fTypeRef, PropertyAccessor deploymentAccessor) {
        if (fTypeRef.derived != null) {
            return fTypeRef.derived.typeStreamFTypeSignature(deploymentAccessor)
        }

        return fTypeRef.predefined.basicTypeStreamSignature
    }

    def String createSerials(FStructType fStructType) {
        var String serials;
        serials = "static const CommonAPI::Serial "
                  + fStructType.elementName.toUpperCase() + "_SERIAL = 0x"
                  + getSerialId(fStructType).toHexString.toUpperCase() + ";\n";
        for (derived : fStructType.derivedFStructTypes)
            serials = serials + createSerials(derived)
        return serials
    }

    def String generateCases(FStructType fStructType, FModelElement parent, boolean qualified) {
        var String itsCases = "case ";

        //if (parent != null)
        //    itsCases = itsCases + parent.elementName + "::"

        if (qualified) {
             itsCases = itsCases
             if (fStructType.containingInterface != null)
                 itsCases = itsCases + fStructType.containingInterface.elementName  + "::";
        }

        itsCases = itsCases + fStructType.elementName.toUpperCase() + "_SERIAL:\n";

        for (derived : fStructType.derivedFStructTypes)
            itsCases = itsCases + derived.generateCases(parent, qualified);

        return itsCases;
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
            default: return "UNDEFINED"
        }
    }

    def private dispatch String typeStreamFTypeSignature(FTypeDef fTypeDef,
        PropertyAccessor deploymentAccessor) {
        return fTypeDef.actualType.actualTypeStreamSignature(deploymentAccessor)
    }

    def private dispatch String typeStreamFTypeSignature(FArrayType fArrayType,
        PropertyAccessor deploymentAccessor) {
        return 'typeOutputStream.beginWriteVectorType();\n' +
            fArrayType.elementType.actualTypeStreamSignature(deploymentAccessor) + '\n' +
            'typeOutputStream.endWriteVectorType();'
    }

    def private dispatch String typeStreamFTypeSignature(FMapType fMap,
        PropertyAccessor deploymentAccessor) {
        return 'typeOutputStream.beginWriteMapType();\n' + fMap.keyType.actualTypeStreamSignature(deploymentAccessor) + '\n' +
            fMap.valueType.actualTypeStreamSignature(deploymentAccessor) + '\n' + 'typeOutputStream.endWriteMapType();'
    }

    def private dispatch String typeStreamFTypeSignature(FStructType fStructType,
        PropertyAccessor deploymentAccessor) {
        return 'typeOutputStream.beginWriteStructType();\n' +
            fStructType.getElementsTypeStreamSignature(deploymentAccessor) + '\n' +
            'typeOutputStream.endWriteStructType();'
    }

    def private dispatch String typeStreamFTypeSignature(FEnumerationType fEnumerationType,
        PropertyAccessor deploymentAccessor) {
        val backingType = fEnumerationType.getBackingType(deploymentAccessor)
        if (backingType != FBasicTypeId.UNDEFINED)
            return backingType.basicTypeStreamSignature

        return FBasicTypeId.INT32.basicTypeStreamSignature
    }

    def private dispatch String typeStreamFTypeSignature(FUnionType fUnionType,
        PropertyAccessor deploymentAccessor) {
        return 'typeOutputStream.writeVariantType();'
    }

    def private String getElementsTypeStreamSignature(FStructType fStructType,
        PropertyAccessor deploymentAccessor) {
        var signature = fStructType.elements.map[type.typeStreamSignature(deploymentAccessor, it)].join

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

    def boolean hasDerivedTypes(FStructType fStructType) {
        return !fStructType.derivedFStructTypes.empty
    }

    def getSerialId(FStructType fStructType) {
        val hasher = Hashing::murmur3_32.newHasher
        hasher.putFTypeObject(fStructType);
        return hasher.hash.asInt
    }

    def private dispatch void putFTypeObject(Hasher hasher, FStructType fStructType) {
        if (fStructType.base != null)
            hasher.putFTypeObject(fStructType.base)

        hasher.putString(fStructType.fullyQualifiedName, Charsets::UTF_8)
        hasher.putString('FStructType', Charsets::UTF_8)
        fStructType.elements.forEach [
            hasher.putFTypeRef(type)
            // avoid cases where the positions of 2 consecutive elements of the same type are switched
            hasher.putString(elementName, Charsets::UTF_8)
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
            hasher.putString(fTypeRef.predefined.getName, Charsets::UTF_8);
    }


    def getDerivedFStructTypes(FStructType fStructType) {
        return EcoreUtil.UsageCrossReferencer::find(fStructType, fStructType.model.eResource.resourceSet).map[EObject].
            filter[it instanceof FStructType].map[it as FStructType].filter[base == fStructType]
    }

    def boolean isStructEmpty(FStructType fStructType) {
        if(!fStructType.elements.empty || fStructType.polymorphic) {
            return false
        }
        if(fStructType.base != null) {
            return isStructEmpty(fStructType.base)
        }
        return true
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

    def generateMajorVersionNamespace(FTypeCollection _tc) '''

        «var FVersion itsVersion = _tc.version»
        «IF itsVersion != null && (itsVersion.major != 0 || itsVersion.minor != 0)»
        // Compatibility
        namespace v«itsVersion.major.toString»_«itsVersion.minor.toString» = v«itsVersion.major.toString»;
        «ENDIF»
    '''

    def generateVersionNamespaceBegin(FTypeCollection _tc) '''
        «var FVersion itsVersion = _tc.version»
        «IF itsVersion != null && (itsVersion.major != 0 || itsVersion.minor != 0)»
            namespace v«itsVersion.major.toString» {
        «ENDIF»
    '''

    def generateVersionNamespaceEnd(FTypeCollection _tc) '''
        «var FVersion itsVersion = _tc.version»
        «IF itsVersion != null && (itsVersion.major != 0 || itsVersion.minor != 0)»
            } // namespace v«itsVersion.major.toString»
        «ENDIF»
    '''

    def generateDeploymentNamespaceBegin(FTypeCollection _tc) '''
        namespace «_tc.elementName»_ {
    '''

    def generateDeploymentNamespaceEnd(FTypeCollection _tc) '''
        } // namespace «_tc.elementName»_
    '''

    def getVersionPrefix(FTypeCollection _tc) {
        var String prefix = "::"
        var FVersion itsVersion = _tc.version
        if (itsVersion != null && (itsVersion.major != 0 || itsVersion.minor != 0)) {
            prefix += "v" + itsVersion.major.toString + "::"
        }
        return prefix
    }

    def getVersionPathPrefix(FTypeCollection _tc) {
        var String prefix = ""
        var FVersion itsVersion = _tc.version
        if (itsVersion != null && (itsVersion.major != 0 || itsVersion.minor != 0)) {
            prefix = "v" + itsVersion.major.toString + "/"
        }
        return prefix
    }

    def getFilePath(Resource resource) {
        if (resource.URI.file)
            return resource.URI.toFileString

        val platformPath = new Path(resource.URI.toPlatformString(true))
        val file = ResourcesPlugin::getWorkspace().getRoot().getFile(platformPath);

        return file.location.toString
    }

    def getLicenseHeader() {
        return FPreferences::instance.getPreference(PreferenceConstants::P_LICENSE, PreferenceConstants.DEFAULT_LICENSE)
    }

    static def getFrancaVersion() {
        val bundleName = "org.franca.core"
        getBundleVersion(bundleName)
    }

    static def getCoreVersion() {
        val bundleName = "org.genivi.commonapi.core"
        getBundleVersion(bundleName)
    }

    static def getBundleVersion(String bundleName) {
        val bundle = FrameworkUtil::getBundle(FrancaGeneratorExtensions)
        if (bundle != null) {

            //OSGI framework running
            val bundleContext = bundle.getBundleContext();
            for (b : bundleContext.bundles) {
                if (b.symbolicName.equals(bundleName)) {
                    return b.version.toString
                }
            }
        } else {

            //pure java runtime
            try {
                val manifestsEnum = typeof(FrancaGeneratorExtensions).classLoader.getResources("META-INF/MANIFEST.MF")
                val bundleRegex = "^" + bundleName.replace(".", "\\.") + "(;singleton:=true)?$"
                while (manifestsEnum.hasMoreElements) {
                    val manifestURL = manifestsEnum.nextElement
                    val manifest = new Manifest(manifestURL.openStream)
                    val name = manifest.mainAttributes.getValue("Bundle-SymbolicName")
                    if (name != null && name.matches(bundleRegex)) {
                        val version = manifest.mainAttributes.getValue("Bundle-Version")
                        return version
                    }
                }
            } catch (IOException exc) {
                System.out.println("Failed to get the bundle version: " + exc.getMessage())
            }
            return ""
        }
    }

    def generateCommonApiLicenseHeader() '''
        /*
        * This file was generated by the CommonAPI Generators.
        * Used org.genivi.commonapi.core «getCoreVersion()».
        * Used org.franca.core «getFrancaVersion()».
        *
        «getCommentedString(getLicenseHeader())»
        */
    '''

    def getCommentedString(String string) {
        val lines = string.split("\n");
        var builder = new StringBuilder();
        for (String line : lines) {
            builder.append("* " + line + "\n");
        }
        return builder.toString()
    }

    def stubManagedSetName(FInterface fInterface) {
        'registered' + fInterface.elementName + 'Instances'
    }

    def stubManagedSetGetterName(FInterface fInterface) {
        'get' + fInterface.elementName + 'Instances'
    }

    def stubRegisterManagedName(FInterface fInterface) {
        'registerManagedStub' + fInterface.elementName
    }

    def stubRegisterManagedAutoName(FInterface fInterface) {
        'registerManagedStub' + fInterface.elementName + 'AutoInstance'
    }

    def stubRegisterManagedMethod(FInterface fInterface) {
        'bool ' + fInterface.stubRegisterManagedName + '(std::shared_ptr< ' + fInterface.getStubFullClassName + '>, const std::string&)'
    }

    def stubRegisterManagedMethodWithInstanceNumber(FInterface fInterface) {
        'bool ' + fInterface.stubRegisterManagedName + '(std::shared_ptr< ' + fInterface.getStubFullClassName + '>, const uint32_t)'
    }

    def stubRegisterManagedMethodImpl(FInterface fInterface) {
        fInterface.stubRegisterManagedName + '(std::shared_ptr< ' + fInterface.getStubFullClassName + '> _stub, const std::string &_instance)'
    }

    def stubRegisterManagedMethodWithInstanceNumberImpl(FInterface fInterface) {
        fInterface.stubRegisterManagedName + '(std::shared_ptr< ' + fInterface.getStubFullClassName + '> _stub, const uint32_t _instanceNumber)'
    }

    def stubDeregisterManagedName(FInterface fInterface) {
        'deregisterManagedStub' + fInterface.elementName
    }

    def proxyManagerGetterName(FInterface fInterface) {
        'getProxyManager' + fInterface.elementName
    }

    def proxyManagerMemberName(FInterface fInterface) {
        'proxyManager' + fInterface.elementName + '_'
    }

    def List<FInterface> getBaseInterfaces(FInterface _interface) {
        val List<FInterface> baseInterfaces = new ArrayList<FInterface>()
        if (_interface.base != null) {
            baseInterfaces.addAll(getBaseInterfaces(_interface.base))
            if (!baseInterfaces.contains(_interface.base))
                baseInterfaces.add(_interface.base)
        }
        return baseInterfaces
    }

    def generateBaseInstantiations(FInterface _interface) {
        val List<FInterface> baseInterfaces = getBaseInterfaces(_interface)
        return baseInterfaces.map[getTypeCollectionName(_interface) + "(_address, _connection)"].join(',\n')
    }

    def generateDBusBaseInstantiations(FInterface _interface) {
        val List<FInterface> baseInterfaces = getBaseInterfaces(_interface)
        return baseInterfaces.map[getTypeCollectionName(_interface) + "DBusProxy(_address, _connection)"].join(',\n')
    }

    def generateSomeIPBaseInstantiations(FInterface _interface) {
        val List<FInterface> baseInterfaces = getBaseInterfaces(_interface)
        return baseInterfaces.map[getTypeCollectionName(_interface) + "SomeIPProxy(_address, _connection)"].join(',\n')
    }

    def String generateBaseRemoteHandlerConstructorsCalls(FInterface _interface) {
        val List<FInterface> baseInterfaces = getBaseInterfaces(_interface)
        var String itsCalls = ""
        itsCalls = baseInterfaces.map[getTypeCollectionName(_interface) + "StubDefault::RemoteEventHandler(_defaultStub)"].join(', ')
        if (itsCalls != "")
            itsCalls = itsCalls + ','
        return itsCalls
    }

    def List<FMethod> getInheritedMethods(FInterface fInterface) {
        val List<FMethod> result = new ArrayList<FMethod>()

        if(fInterface.base == null) {
            return result
        }

        result.addAll(fInterface.base.methods)
        result.addAll(fInterface.base.inheritedMethods)

        return result
    }

    def List<FAttribute> getInheritedAttributes(FInterface fInterface) {
        val List<FAttribute> result = new ArrayList<FAttribute>()
        if(fInterface.base == null) {
            return result
        }

        result.addAll(fInterface.base.attributes)
        result.addAll(fInterface.base.inheritedAttributes)

        return result
    }

    def List<FBroadcast> getInheritedBroadcasts(FInterface fInterface) {
        val List<FBroadcast> result = new ArrayList<FBroadcast>()

        if(fInterface.base == null) {
            return result
        }

        result.addAll(fInterface.base.broadcasts)
        result.addAll(fInterface.base.inheritedBroadcasts)

        return result
    }

    def static FExpression toExpression(String value)
    {
        return switch trimmedValue : if(value != null) value.trim else ""
        {
            case trimmedValue.length > 0:
            {
                try
                {
                    FrancaFactory.eINSTANCE.createFIntegerConstant => [
                        ^val = new BigInteger(trimmedValue)
                    ]
                }
                catch(NumberFormatException e)
                {
                    FrancaFactory.eINSTANCE.createFStringConstant => [
                        ^val = trimmedValue
                    ]
                }
            }
            default:
            {
                null
            }
        }
    }

    def static String getEnumeratorValue(FExpression expression)
    {
        return switch (expression)
        {
            FIntegerConstant: expression.^val.toString
            FStringConstant: expression.^val
            FQualifiedElementRef: expression.element.constantValue
            default: null
        }
    }

    def static String getConstantValue(FModelElement  expression)
    {
        return switch (expression)
        {
            FConstantDef: expression.rhs.constantType
            default: null
        }
    }

    def static String getConstantType(FInitializerExpression  expression)
    {
        return switch (expression)
        {
            FIntegerConstant: expression.^val.toString
            default: null
        }
    }

    def Integer getTimeout(FMethod _method, PropertyAccessor _accessor) {
        var timeout = 0;
        try {
            timeout = _accessor.getTimeout(_method)
        }
        catch (NullPointerException e) {
            // intentionally empty
        }
        return timeout
    }

    def boolean isTheSameVersion(FVersion _mine, FVersion _other) {
        return ((_mine == null && _other == null) ||
                (_mine != null && _other != null &&
                 _mine.major == _other.major &&
                 _mine.minor == _other.minor));

    }

    def EObject getCommonContainer(EObject _me, EObject _other) {
        if (_other == null)
            return null

        if (_me == _other)
            return _me

        if (_me instanceof FTypeCollection && _other instanceof FTypeCollection) {
            val FTypeCollection me = _me as FTypeCollection
            val FTypeCollection other = _other as FTypeCollection

            if (me.eContainer == other.eContainer &&
                ((me.version == null && other.version == null) ||
                 (me.version != null && other.version != null &&
                  me.version.major == other.version.major &&
                  me.version.minor == other.version.minor))) {
                return _me;
               }
        }

          return getCommonContainer(_me, _other.eContainer)
    }

    def String getContainerName(EObject _container) {
        var String name = ""
        if (_container instanceof FTypeCollection) {
            name = _container.name
            if (name == null) name = "__Anonymous__"
        } else if (_container instanceof FModelElement) {
            name = _container.name
        } else if (_container instanceof FModel) {
            name = _container.name
        }
        return name
    }

    def String getPartialName(FModelElement _me, EObject _until) {
        var String name = _me.containerName
        var EObject container = _me.eContainer
        while (container != null && container != _until) {
            if (container instanceof FModel) {
                name = container.containerName + "::" + name
            }
            if (container instanceof FModelElement) {
                name = container.containerName + "::" + name
            }
            container = container.eContainer
        }
        return name.replace(".", "::")
    }

    def String getFullName(EObject _me) {
        var String name = ""
        if (_me instanceof FModelElement) {
            var String prefix = ""
            var EObject container = _me
            while (container != null) {
                if (container instanceof FTypeCollection) {
                    prefix = container.versionPrefix
                }
                val containerName = getContainerName(container)
                if (containerName != null && containerName != "") {
                    if (name != "") {
                        name = containerName + "::" + name
                    } else {
                        name = containerName
                    }
                }

                container = container.eContainer
            }
            name = prefix + name
        }
        return name.replace(".", "::")
    }

    def String getElementName(FModelElement _me, FModelElement _other, boolean _isOther) {
        if (_other == null)
            return _me.containerName

        var FVersion myVersion = null
        if (_me instanceof FTypeCollection)
            myVersion = _me.version

        var FVersion otherVersion = null
        if (_other instanceof FTypeCollection)
            otherVersion = _other.version

        if (isTheSameVersion(myVersion, otherVersion)) {
            val EObject myContainer = _me.eContainer
            val EObject otherContainer = _other.eContainer

            if (myContainer == otherContainer) {
                var String name = _me.containerName
                if (_isOther) {
                    name = getContainerName(myContainer) + "::" + _me.containerName
                }
                return name
            }

            if (myContainer.getCommonContainer(otherContainer) != null) {
                return _me.getPartialName(myContainer.eContainer)
            }

            if (otherContainer.getCommonContainer(myContainer) != null) {
                var String name = _me.containerName
                if (_isOther) {
                    name = getContainerName(myContainer) + "::" + name
                }
                return name
            }
        }

        return _me.getFullName()
    }

    def String getTypeCollectionName(FTypeCollection _me, FTypeCollection _other) {
        if (_other == null ||
            _me.eContainer() != _other.eContainer() ||
             !isTheSameVersion(_me.version, _other.version)) {
            return _me.getFullName()
        }
        return _me.getContainerName()
    }

    def String getEnumPrefix(){
              FPreferences::instance.getPreference(PreferenceConstants::P_ENUMPREFIX, "")
    }

    def List<FField> getAllElements(FStructType _struct) {
        if (_struct.base == null)
            return new LinkedList(_struct.elements)

        val elements = _struct.base.allElements
        elements.addAll(_struct.elements)
        return elements
    }

    def List<FField> getAllElements(FUnionType _union) {
        if (_union.base == null)
            return new LinkedList(_union.elements)

        val elements = _union.base.allElements
        elements.addAll(_union.elements)
        return elements
    }

    /**
     * Get all interfaces from the deployment model that have a specification that contains
     * the given selector string (e.g.: "dbus" for commonapi.dbus specification)
     * @return the list of FDInterfaces
     */
    def List<FDInterface> getFDInterfaces(FDModel fdmodel, String selector) {
        var List<FDInterface> fdinterfaces = new ArrayList<FDInterface>()

        for(depl : fdmodel.getDeployments()) {
            if (depl instanceof FDInterface) {
                var specname = depl.spec?.name
                if(specname != null && specname.contains(selector)) {
                    fdinterfaces.add(depl);
                }
            }
        }
        return fdinterfaces;
    }

      /**
     * Get all type collections from the deployment model that have a specification that contains
     * the given selector string (e.g.: "dbus" for commonapi.dbus specification)
     * @return the list of FDTypes
     */
    def List<FDTypes> getFDTypesList(FDModel fdmodel, String selector) {
        var List<FDTypes> fdTypes = new ArrayList<FDTypes>()

        for(depl : fdmodel.getDeployments()) {
            if (depl instanceof FDTypes) {
                var specname = depl.spec?.name
                if(specname != null && specname.contains(selector)) {
                    fdTypes.add(depl);
                }
            }
        }
        return fdTypes;
    }

      /**
     * Get all providers from the deployment model that have a specification that contains
     * the given selector string (e.g.: "dbus" for commonapi.dbus specification)
     * @return the list of FDTypes
     */
    def List<FDProvider> getFDProviders(FDModel fdmodel, String selector) {
        var List<FDProvider> fdProviders = new ArrayList<FDProvider>()

        for(depl : fdmodel.getDeployments()) {
            if (depl instanceof FDProvider) {
                var specname = depl.spec?.name
                if(specname != null && specname.contains(selector)) {
                    fdProviders.add(depl);
                }
            }
        }
        return fdProviders;
    }
    def List<FDProvider> getAllFDProviders(FDModel fdmodel) {
        var List<FDProvider> fdProviders = new ArrayList<FDProvider>()

        for(depl : fdmodel.getDeployments()) {
            if (depl instanceof FDProvider) {
                fdProviders.add(depl);
            }
        }
        return fdProviders;
    }
    
    /**
     * Copy deployment attributes into the destination deployment
     */
    def mergeDeployments(FDInterface _source, FDInterface _target) {
        if (_source != null && _source.target != null &&
            _target != null && _source.target.equals(_target.target)) {

            var List<FDAttribute> itsNewAttributes = new ArrayList<FDAttribute>()                
            var List<FDBroadcast> itsNewBroadcasts = new ArrayList<FDBroadcast>()
            var List<FDMethod> itsNewMethods = new ArrayList<FDMethod>()

            // Merge attributes
            for (FDAttribute s : _source.attributes) {
                var boolean hasBeenMerged = false
                for (FDAttribute t : _target.attributes) {
                    if (s.target.equals(t.target)) {
                        for (p : s.properties)
                            t.properties.add(EcoreUtil.copy(p))
                        hasBeenMerged = true
                    }
                }
                if (!hasBeenMerged)
                    itsNewAttributes.add(EcoreUtil.copy(s))
            }

            // Merge broadcasts
            for (FDBroadcast s : _source.broadcasts) {
                var boolean hasBeenMerged = false
                for (FDBroadcast t : _target.broadcasts) {
                    if (s.target.equals(t.target)) {
                        for (p : s.properties)
                            t.properties.add(EcoreUtil.copy(p))
                        hasBeenMerged = true
                    }
                }
                if (!hasBeenMerged) {
                    itsNewBroadcasts.add(EcoreUtil.copy(s))
                }
            }

            // Merge methods
            for (FDMethod s : _source.methods) {
                var boolean hasBeenMerged = false
                for (FDMethod t : _target.methods) {
                    if (s.target.equals(t.target)) {
                        for (p : s.properties)
                            t.properties.add(EcoreUtil.copy(p))
                        hasBeenMerged = true
                    }
                }
                if (!hasBeenMerged)
                    itsNewMethods.add(EcoreUtil.copy(s))
            }

            _target.attributes.addAll(itsNewAttributes)
            _target.broadcasts.addAll(itsNewBroadcasts)
            _target.methods.addAll(itsNewMethods)

            // Merge types
            mergeDeployments(_source.types, _target.types)
        }
    }
    
    def mergeDeployments(FDTypes _source, FDInterface _target) {
        mergeDeployments(_source.types, _target.types)
    }

    def mergeDeployments(FDTypes _source, FDTypes _target) {
        if (_source != null && _source.target != null &&
            _target != null && _source.target.equals(_target.target)) {
            // Merge types
            mergeDeployments(_source.types, _target.types)
        }
    }

    def mergeDeploymentsExt(FDTypes _source, FDTypes _target) {
        if (_source != null && _source.target != null &&
            _target != null && _target.target != null) {
            // Merge types
            mergeDeployments(_source.types, _target.types)
        }
    }

    def mergeDeployments(EList<FDTypeDef> _source, EList<FDTypeDef> _target) {
        var List<FDTypeDef> itsNewTypeDefs = new ArrayList<FDTypeDef>()

        for (FDTypeDef s : _source) {
            var boolean hasBeenMerged = false
            for (FDTypeDef t : _target) {
                if (s instanceof FDArray && t instanceof FDArray) {
                    val FDArray itsSourceArray = s as FDArray
                    val FDArray itsTargetArray = t as FDArray
                    if (itsSourceArray.target.equals(itsTargetArray.target)) {
                        for (p : itsSourceArray.properties)
                            itsTargetArray.properties.add(EcoreUtil.copy(p))
                        hasBeenMerged = true
                    }
                } else if (s instanceof FDEnumeration && t instanceof FDEnumeration) {
                    val FDEnumeration itsSourceEnumeration = s as FDEnumeration
                    val FDEnumeration itsTargetEnumeration = t as FDEnumeration
                    if (itsSourceEnumeration.target.equals(itsTargetEnumeration.target)) {
                        for (p : itsSourceEnumeration.properties)
                            itsTargetEnumeration.properties.add(EcoreUtil.copy(p))
                        hasBeenMerged = true
                    }
                } else if (s instanceof FDStruct && t instanceof FDStruct) {
                    val FDStruct itsSourceStruct = s as FDStruct
                    val FDStruct itsTargetStruct = t as FDStruct
                    if (itsSourceStruct.target.equals(itsTargetStruct.target)) {
                        for (p : itsSourceStruct.properties)
                            itsTargetStruct.properties.add(EcoreUtil.copy(p))
                        hasBeenMerged = true
                    }
                } else if (s instanceof FDUnion && t instanceof FDUnion) {
                    val FDUnion itsSourceUnion = s as FDUnion
                    val FDUnion itsTargetUnion = t as FDUnion
                    if (itsSourceUnion.target.equals(itsTargetUnion.target)) {
                        for (p : itsSourceUnion.properties)
                            itsTargetUnion.properties.add(EcoreUtil.copy(p))
                        hasBeenMerged = true
                    }
                } 
            } 
            if (!hasBeenMerged)
                itsNewTypeDefs.add(EcoreUtil.copy(s))
        }

        _target.addAll(itsNewTypeDefs)
    }

    def public getAllReferencedFInterfaces(FModel fModel)
    {
        val referencedFInterfaces = fModel.interfaces.toSet
        fModel.interfaces.forEach[base?.addFInterfaceTree(referencedFInterfaces)]
        fModel.interfaces.forEach[managedInterfaces.forEach[addFInterfaceTree(referencedFInterfaces)]]
        return referencedFInterfaces
    }

    def public void addFInterfaceTree(FInterface fInterface, Collection<FInterface> fInterfaceReferences)
    {
        if(!fInterfaceReferences.contains(fInterface))
        {
            fInterfaceReferences.add(fInterface)
            fInterface.base?.addFInterfaceTree(fInterfaceReferences)
        }
    }

    def public getAllReferencedFTypes(FModel fModel)
    {
        val referencedFTypes = new HashSet<FType>

        fModel.typeCollections.forEach[types.forEach[addFTypeDerivedTree(referencedFTypes)]]

        fModel.interfaces.forEach [
            attributes.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            types.forEach[addFTypeDerivedTree(referencedFTypes)]
            methods.forEach [
                inArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
                outArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            ]
            broadcasts.forEach [
                outArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            ]
        ]

        return referencedFTypes
    }

    def public void addDerivedFTypeTree(FTypeRef fTypeRef, Collection<FType> fTypeReferences)
    {
        fTypeRef.derived?.addFTypeDerivedTree(fTypeReferences)
    }

    def public dispatch void addFTypeDerivedTree(FTypeDef fTypeDef, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fTypeDef))
        {
            fTypeReferences.add(fTypeDef)
            fTypeDef.actualType.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def public dispatch void addFTypeDerivedTree(FArrayType fArrayType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fArrayType))
        {
            fTypeReferences.add(fArrayType)
            fArrayType.elementType?.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def public dispatch void addFTypeDerivedTree(FMapType fMapType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fMapType))
        {
            fTypeReferences.add(fMapType)
            fMapType.keyType.addDerivedFTypeTree(fTypeReferences)
            fMapType.valueType.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def public dispatch void addFTypeDerivedTree(FStructType fStructType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fStructType))
        {
            fTypeReferences.add(fStructType)
            fStructType.base?.addFTypeDerivedTree(fTypeReferences)
            fStructType.elements.forEach[type.addDerivedFTypeTree(fTypeReferences)]
        }
    }

    def public dispatch void addFTypeDerivedTree(FEnumerationType fEnumerationType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fEnumerationType))
        {
            fTypeReferences.add(fEnumerationType)
            fEnumerationType.base?.addFTypeDerivedTree(fTypeReferences)
        }
    }

    def public dispatch void addFTypeDerivedTree(FUnionType fUnionType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fUnionType))
        {
            fTypeReferences.add(fUnionType)
            fUnionType.base?.addFTypeDerivedTree(fTypeReferences)
            fUnionType.elements.forEach[type.addDerivedFTypeTree(fTypeReferences)]
        }
    }
    def public supportsTypeValidation(FAttribute fAttribute) {
        fAttribute.type.derived instanceof FEnumerationType
    }
    def public validateType(FAttribute fAttribute, FInterface fInterface) {
        if (fAttribute.type.derived instanceof FEnumerationType && !fAttribute.array) {
            "_value.validate()"
        }
        else "true"
    }
    def public supportsValidation(FTypeRef fTtypeRef) {
        fTtypeRef.derived instanceof FEnumerationType
    }

    def public String getCanonical(String _basePath, String _uri) {
        if (_uri.startsWith("platform:") || _uri.startsWith("file:"))
            return _uri

        if (_uri.startsWith(File.separatorChar.toString))
            return "file:" + _uri

        // handle Windows absolute paths...
        var baseSplitted = _basePath.split(":")
        var uriSplitted = _uri.split(":")

        if (uriSplitted.length == 2 && _uri.startsWith(uriSplitted.get(0))) {
            // c:/...   windows abslolute
            return "file:/" + _uri
        }

        if (_uri.startsWith("/")) {
            // get the drive letter under windows for absolute paths
            if(baseSplitted.length == 3) {
                return "file:" + baseSplitted.get(1) + ":" + _uri
            }
            else {
                // linux...
                return "file:" + _uri
            }
        }
        // handle relative paths (resolve  uri against basePath)
        var char separatorCharacter = File.separatorChar;
        if (_basePath.startsWith("platform:") || _basePath.startsWith("file:")) {
            separatorCharacter = '/'
        }
        val String itsCompleteName = _basePath + separatorCharacter + _uri
        val String[] itsSegments = itsCompleteName.split(separatorCharacter.toString)

        var int ignore = 0
        var int i = itsSegments.length - 1
        var List<String> itsCanonicalName = new ArrayList<String>()

        /*
         * Run from the last segment to the first. Identity segments (.) are ignored,
         * while backward segments (..) let us ignore the next segment that is neither
         * identify nor backward. Thus, if several backward segments follow on each other
         * we increase the number of non-identity/non-backward segments we need to ignore.
         * Whenever we find a "normal" segment and the "ignore counter" is zero, it is
         * added to the result, otherwise the "ignore counter" is decreased.
         */
        while (i >= 0) {
            var String itsCurrentSegment = itsSegments.get(i)
            if (itsCurrentSegment == "..") {
                ignore = ignore + 1
            } else if (itsCurrentSegment != ".") {
                if (ignore == 0) {
                    itsCanonicalName.add(itsCurrentSegment)
                } else {
                    ignore = ignore - 1
                }
            }
            i = i - 1
        }

        /*
         * As we ran from back to front and added segments, we need to reverse,
         * before joining the segments
         */
        return itsCanonicalName.reverse.join(separatorCharacter.toString)
    }

}
