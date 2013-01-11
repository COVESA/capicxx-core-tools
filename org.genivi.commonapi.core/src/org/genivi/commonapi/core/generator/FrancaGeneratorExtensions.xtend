/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import com.google.common.primitives.Ints
import java.util.List
import org.eclipse.core.runtime.Path
import org.eclipse.emf.ecore.EObject
import org.eclipse.emf.ecore.plugin.EcorePlugin
import org.eclipse.emf.ecore.resource.Resource
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModel
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FEnumerationType

import static com.google.common.base.Preconditions.*

class FrancaGeneratorExtensions {
    def getFullyQualifiedName(FModelElement fModelElement) {
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

    def getDirectoryPath(FModel fModel) {
        fModel.name.replace('.', '/')
    }
    
    def getDefineName(FModelElement fModelElement) {
        val defineSuffix = '_' + fModelElement.name.splitCamelCase.join('_').toUpperCase

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
        var signature = fMethod.inArgs.map['const ' + type.getNameReference(fMethod.model) + '& ' + name].join(', ')

        if (!fMethod.inArgs.empty)
            signature = signature + ', '

        signature = signature + 'CommonAPI::CallStatus& callStatus'

        if (fMethod.hasError)
            signature = signature + ', ' + fMethod.getErrorNameReference(fMethod.eContainer) + '& methodError'

        if (!fMethod.outArgs.empty)
            signature = signature + ', ' + fMethod.outArgs.map[type.getNameReference(fMethod.model) + '& ' + name].join(', ')

        return signature
    }
    
    def generateStubSignature(FMethod fMethod) {
        var signature = fMethod.inArgs.map[type.getNameReference(fMethod.model) + ' ' + name].join(', ')

        if (fMethod.hasError)
            signature = signature + ', ' + fMethod.getErrorNameReference(fMethod.eContainer) + '& methodError'

        if (!fMethod.outArgs.empty)
            signature = signature + ', ' + fMethod.outArgs.map[type.getNameReference(fMethod.model) + '& ' + name].join(', ')

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
        var signature = fMethod.inArgs.map['const ' + type.getNameReference(fMethod.model) + '& ' + name].join(', ')
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
        if (fMethod.errorEnum != null)
            return fMethod.errorEnum.getRelativeNameReference(source)

        var errorNameReference = fMethod.errors.errorName
        if (!fMethod.eContainer.equals(source)) 
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
        fAttribute.readonly != null
    }

    def isObservable(FAttribute fAttribute) {
        fAttribute.noSubscriptions == null
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

    def getNameReference(FTypeRef destination, EObject source) {
        if (destination.derived != null)
            return destination.derived.getRelativeNameReference(source)
        return destination.predefined.primitiveTypeName
    }

    def getErrorName(FEnumerationType fMethodErrors) {
        checkArgument(fMethodErrors.eContainer instanceof FMethod, 'Not FMethod errors')
        (fMethodErrors.eContainer as FMethod).name + 'Error'
    }

    def getBackingType(FEnumerationType fEnumerationType) {
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
        val file = EcorePlugin::workspaceRoot.getFile(platformPath)

        return file.location.toString
    }

    def generateCommonApiLicenseHeader() '''
        /* This Source Code Form is subject to the terms of the Mozilla Public
         * License, v. 2.0. If a copy of the MPL was not distributed with this
         * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
    '''
}