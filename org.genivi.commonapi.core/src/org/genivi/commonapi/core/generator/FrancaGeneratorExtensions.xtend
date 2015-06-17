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
import java.math.BigInteger
import java.util.ArrayList
import java.util.Collection
import java.util.HashMap
import java.util.HashSet
import java.util.LinkedList
import java.util.List
import java.util.Map
import java.util.Set
import org.eclipse.core.resources.IResource
import org.eclipse.core.resources.ResourcesPlugin
import org.eclipse.core.runtime.Path
import org.eclipse.core.runtime.preferences.DefaultScope
import org.eclipse.core.runtime.preferences.InstanceScope
import org.eclipse.emf.ecore.EObject
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.emf.ecore.util.EcoreUtil
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FConstantDef
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FEnumerator
import org.franca.core.franca.FExpression
import org.franca.core.franca.FField
import org.franca.core.franca.FInitializerExpression
import org.franca.core.franca.FIntegerConstant
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
    
     def String getFullyQualifiedCppName(FModelElement fModelElement) {
        if (fModelElement.eContainer instanceof FModel) {
        	var name = (fModelElement.eContainer as FModel).name
        	if (fModelElement instanceof FTypeCollection) {
        		val FVersion itsVersion = fModelElement.version
        		if (itsVersion != null) {
        			name = fModelElement.versionPrefix + name
        		}
        	}
            return "::" + (name + "::" + fModelElement.elementName).replace(".", "::")
        }
        return "::" + ((fModelElement.eContainer as FModelElement).fullyQualifiedName + "::" + fModelElement.elementName).replace(".", "::")
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
        val defineSuffix = '_' + fModelElement.elementName.splitCamelCase.join('_')

        if (fModelElement.eContainer instanceof FModelElement)
            return (fModelElement.eContainer as FModelElement).defineName + defineSuffix

        return (fModelElement.eContainer as FModel).defineName + defineSuffix
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

    def getHeaderPath(FTypeCollection fTypeCollection) {
        fTypeCollection.versionPathPrefix + fTypeCollection.model.directoryPath + '/' + fTypeCollection.headerFile
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

    def getProxyHeaderPath(FInterface fInterface) {
        fInterface.versionPathPrefix + fInterface.model.directoryPath + '/' + fInterface.proxyHeaderFile
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

        signature = signature + 'CommonAPI::CallStatus &_status'

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

    def generateOverloadedStubSignature(FMethod fMethod, String methodName) {
        var signature = 'const std::shared_ptr<CommonAPI::ClientId> _client'

        if (!fMethod.inArgs.empty)
            signature = signature + ', '

        signature = signature + fMethod.inArgs.map[getTypeName(fMethod, true) + ' _' + elementName].join(', ')
        
        if (!fMethod.isFireAndForget) {
        	if (signature != "")
        		signature = signature + ", ";
        	signature = signature + methodName + "Reply_t _reply";
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
    
    def String generateDummyArgumentInitializations(FMethod fMethod) {
       var String retval = ""
       for(list_element : fMethod.outArgs) {
          retval += getTypeName(list_element, fMethod, true)  + ' ' + list_element.elementName
          if (((list_element.type.derived instanceof FStructType
             && (list_element.type.derived as FStructType).hasPolymorphicBase))
             || list_element.array){
                retval += " = {}"
          } else if(list_element.type.predefined != null) {
             // primitive types
             switch list_element.type.predefined {
               case FBasicTypeId::BOOLEAN: retval+= " = false"
               case FBasicTypeId::INT8: retval += " = 0"
               case FBasicTypeId::UINT8: retval += " = 0"
               case FBasicTypeId::INT16: retval += " = 0"
               case FBasicTypeId::UINT16: retval += " = 0"
               case FBasicTypeId::INT32: retval += " = 0"
               case FBasicTypeId::UINT32: retval += " = 0"
               case FBasicTypeId::INT64: retval += " = 0"
               case FBasicTypeId::UINT64: retval += " = 0"
               case FBasicTypeId::FLOAT: retval += " = 0.0f"
               case FBasicTypeId::DOUBLE: retval += " = 0.0"
               case FBasicTypeId::STRING: retval += " = \"\""
               case FBasicTypeId::BYTE_BUFFER: retval += " = {}"
               default: {}//System.out.println("No initialization generated for" +
                   //" non-basic type: " + list_element.getName)
             }
          }
          retval += ";\n"
       }
       return retval
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
            typeName = 'std::shared_ptr<' + typeName + '>'

        if (_element.array) {
            typeName = 'std::vector<' + typeName + '>'
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

    def String getBaseType(FEnumerationType _enumeration, String _backingType) {
        var String baseType
        if (_enumeration.base != null) {
            baseType = _enumeration.base.getElementName(_enumeration, false)
        } else {
            baseType = "CommonAPI::Enumeration<" + _backingType + ">"
        }
        return baseType
    }

	def private getMaximumEnumerationValue(FEnumerationType _enumeration) {
		var int maximum = 0;
		for (literal : _enumeration.enumerators) {
			if (literal.value != null && literal.value != "") {
				val int literalValue = Integer.parseInt(literal.value.enumeratorValue) 
				if (maximum < literalValue)
					maximum = literalValue
			}
		}
		return maximum
	} 

	def void setEnumerationValues(FEnumerationType _enumeration) {
		var int currentValue = 0
		val predefineEnumValues = new ArrayList<String>()

		// collect all predefined enum values
		for (literal : _enumeration.enumerators) {
			if (literal.value != null) {
				predefineEnumValues.add(literal.value.enumeratorValue)
			}
		}
		if (_enumeration.base != null) {
			setEnumerationValues(_enumeration.base)
			currentValue = getMaximumEnumerationValue(_enumeration.base) + 1
		}

		for (literal : _enumeration.enumerators) {
			if (literal.value == null || literal.value == "") {
				// not predefined
				while (predefineEnumValues.contains(String.valueOf(currentValue))) {
					// increment it, if this was found in the list of predefined values
					currentValue += 1
				}
				literal.setValue(toExpression(Integer.toString(currentValue)))
			} else {
				var enumValue = literal.value.enumeratorValue
				if (enumValue != null) {
					try {
						val int literalValue = Integer.parseInt(enumValue)
						literal.setValue(toExpression(Integer.toString(literalValue)))
					} catch (NumberFormatException e) {
						literal.setValue(toExpression(Integer.toString(currentValue)))
					}
				} else {
					literal.setValue(toExpression(Integer.toString(currentValue)))
				}
			}
			currentValue += 1
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
		//	itsCases = itsCases + parent.elementName + "::"
					
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

    def boolean hasDerivedFStructTypes(FStructType fStructType) {
        return EcoreUtil.UsageCrossReferencer::find(fStructType, fStructType.model.eResource.resourceSet).exists [
            EObject instanceof FStructType && (EObject as FStructType).base == fStructType
        ]
    }

    def getDerivedFStructTypes(FStructType fStructType) {
        return EcoreUtil.UsageCrossReferencer::find(fStructType, fStructType.model.eResource.resourceSet).map[EObject].
            filter[it instanceof FStructType].map[it as FStructType].filter[base == fStructType]
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

    def generateVersionNamespaceBegin(FTypeCollection _tc) '''
        «var FVersion itsVersion = _tc.version»
        «IF itsVersion != null && (itsVersion.major != 0 || itsVersion.minor != 0)»
            namespace v«itsVersion.major.toString»_«itsVersion.minor.toString» {
        «ENDIF»  
    '''

    def generateVersionNamespaceEnd(FTypeCollection _tc) '''
        «var FVersion itsVersion = _tc.version»
        «IF itsVersion != null && (itsVersion.major != 0 || itsVersion.minor != 0)»
            } // namespace v«itsVersion.major.toString»_«itsVersion.minor.toString»
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
            prefix += "v" + itsVersion.major.toString + "_" + itsVersion.minor.toString + "::"
        }
        return prefix
    }

    def getVersionPathPrefix(FTypeCollection _tc) {
    	var String prefix = ""
        var FVersion itsVersion = _tc.version
        if (itsVersion != null && (itsVersion.major != 0 || itsVersion.minor != 0)) {
            prefix = "v" + itsVersion.major.toString + "_" + itsVersion.minor.toString + "/"
        }
        return prefix
    }
    
	// TODO: Remove the following method
    def isFireAndForget(FMethod fMethod) {
        return fMethod.fireAndForget
    }

    def getFilePath(Resource resource) {
        if (resource.URI.file)
            return resource.URI.toFileString

        val platformPath = new Path(resource.URI.toPlatformString(true))
        val file = ResourcesPlugin::getWorkspace().getRoot().getFile(platformPath);

        return file.location.toString
    }

    def getHeader(FModel model, IResource res) {
        if (FrameworkUtil::getBundle(this.getClass()) != null) {
            var returnValue = DefaultScope::INSTANCE.getNode(PreferenceConstants::SCOPE).get(PreferenceConstants::P_LICENSE, "")
            returnValue = InstanceScope::INSTANCE.getNode(PreferenceConstants::SCOPE).get(PreferenceConstants::P_LICENSE, returnValue)
            returnValue = FPreferences::instance.getPreference(PreferenceConstants::P_LICENSE, returnValue)
            return returnValue
        }
        return ""
    }

    def getFrancaVersion() {
        val bundle = FrameworkUtil::getBundle(FrancaGeneratorExtensions)
        val bundleContext = bundle.getBundleContext();
        for (b : bundleContext.bundles) {
            if (b.symbolicName.equals("org.franca.core")) {
                return b.version.toString
            }
        }
    }

    def static getCoreVersion() {
        val bundle = FrameworkUtil::getBundle(FrancaGeneratorExtensions)
        val bundleContext = bundle.getBundleContext();
        for (b : bundleContext.bundles) {
            if (b.symbolicName.equals("org.genivi.commonapi.core")) {
                return b.version.toString
            }
        }
    }

    def generateCommonApiLicenseHeader(FModelElement model, IResource modelid) '''
        /*
        * This file was generated by the CommonAPI Generators.
        * Used org.genivi.commonapi.core «FrancaGeneratorExtensions::getCoreVersion()».
        * Used org.franca.core «getFrancaVersion()».
        *
        «getCommentedString(getHeader(model.model, modelid))»
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
        'bool ' + fInterface.stubRegisterManagedName + '(std::shared_ptr<' + fInterface.stubClassName + '>, const std::string&)'
    }

    def stubRegisterManagedMethodImpl(FInterface fInterface) {
        fInterface.stubRegisterManagedName + '(std::shared_ptr<' + fInterface.stubClassName + '> _stub, const std::string &_instance)'
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
    
    def Set<FInterface> getBaseInterfaces(FInterface _interface) {
    	val Set<FInterface> baseInterfaces = new HashSet<FInterface>()
    	if (_interface.base != null) {
    		baseInterfaces.add(_interface.base)
    		baseInterfaces.addAll(getBaseInterfaces(_interface.base))
    	}
    	return baseInterfaces
    }
    
    def generateBaseInstantiations(FInterface _interface) {
		val Set<FInterface> baseInterfaces = getBaseInterfaces(_interface)
		return baseInterfaces.map[getTypeCollectionName(_interface) + "(_address, _connection)"].join(',\n')
	}
	
    def generateDBusBaseInstantiations(FInterface _interface) {
		val Set<FInterface> baseInterfaces = getBaseInterfaces(_interface)
		return baseInterfaces.map[
					getTypeCollectionName(_interface) +
					"DBusProxy(_address, _connection)"		
		].join(',\n')
	}
	
	def String generateBaseRemoteHandlerConstructorsCalls(FInterface _interface) {
		val Set<FInterface> baseInterfaces = getBaseInterfaces(_interface)
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
                    prefix = (container as FTypeCollection).versionPrefix
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
    
}
