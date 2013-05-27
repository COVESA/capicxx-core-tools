/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.Arrays
import java.util.Collection
import java.util.HashSet
import java.util.LinkedList
import java.util.List
import javax.inject.Inject
import org.eclipse.emf.common.util.EList
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FEnumerator
import org.franca.core.franca.FField
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMapType
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FStructType
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FUnionType
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor

import static com.google.common.base.Preconditions.*

class FTypeGenerator {
    @Inject private extension FrancaGeneratorExtensions francaGeneratorExtensions


    def generateFTypeDeclarations(FTypeCollection fTypeCollection, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «FOR type: fTypeCollection.types.sortTypes(fTypeCollection)»
            «type.generateFTypeDeclaration(deploymentAccessor)»

        «ENDFOR»
    '''

    def private sortTypes(EList<FType> typeList, FTypeCollection containingTypeCollection) {
        checkArgument(typeList.hasNoCircularDependencies, 'FTypeCollection or FInterface has circular dependencies: ' + containingTypeCollection)
        var List<FType> sortedTypeList = new LinkedList<FType>()
        for(currentType: typeList) {
            var insertIndex = 0
            while(insertIndex < sortedTypeList.size && !currentType.isReferencedBy(sortedTypeList.get(insertIndex))) {
                insertIndex = insertIndex + 1
            }
            sortedTypeList.add(insertIndex, currentType)
        }
        return sortedTypeList
    }

    def private isReferencedBy(FType lhs, FType rhs) {
        return rhs.referencedTypes.contains(lhs)
    }

    def private hasNoCircularDependencies(EList<FType> types) {
    	val cycleDetector = new FTypeCycleDetector(francaGeneratorExtensions)
    	return !cycleDetector.hasCycle(types)
    }

    def private List<FType> getReferencedTypes(FType fType) {
        var references = fType.directlyReferencedTypes
        references.addAll(references.map[it.referencedTypes].flatten.toList)
        return references
    }

    def dispatch generateFTypeDeclaration(FTypeDef fTypeDef, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        typedef «fTypeDef.actualType.getNameReference(fTypeDef.eContainer)» «fTypeDef.name»;
    '''

    def dispatch generateFTypeDeclaration(FArrayType fArrayType, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        typedef std::vector<«fArrayType.elementType.getNameReference(fArrayType.eContainer)»> «fArrayType.name»;
    '''

    def dispatch generateFTypeDeclaration(FMapType fMap, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        typedef std::unordered_map<«fMap.generateKeyType»«fMap.generateValueType»> «fMap.name»;
    '''

    def dispatch generateFTypeDeclaration(FStructType fStructType, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        struct «fStructType.name»: «fStructType.baseStructName» {
            «FOR element : fStructType.elements»
                «element.getTypeName(fStructType)» «element.name»;
            «ENDFOR»

            «fStructType.name»() = default;
            «IF fStructType.allElements.size > 0»
                «fStructType.name»(«fStructType.allElements.map[getConstReferenceVariable(fStructType)].join(", ")»);
            «ENDIF»

            virtual void readFromInputStream(CommonAPI::InputStream& inputStream);
            virtual void writeToOutputStream(CommonAPI::OutputStream& outputStream) const;

            static inline void writeToTypeOutputStream(CommonAPI::TypeOutputStream& typeOutputStream) {
                «IF fStructType.base != null»
                    «fStructType.baseStructName»::writeToTypeOutputStream(typeOutputStream);
                «ENDIF»
                «FOR element : fStructType.elements»
                    «element.type.typeStreamSignature(deploymentAccessor)»
                «ENDFOR»
            }
        };
    '''
    
    def dispatch generateFTypeDeclaration(FEnumerationType fEnumerationType, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        generateDeclaration(fEnumerationType, fEnumerationType.name, deploymentAccessor)
    }

    def generateDeclaration(FEnumerationType fEnumerationType, String name, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        enum class «name»: «fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName» {
            «FOR parent : fEnumerationType.baseList SEPARATOR ',\n'»
                «FOR enumerator : parent.enumerators SEPARATOR ','»
                    «enumerator.name» = «parent.getRelativeNameReference(fEnumerationType)»::«enumerator.name»
                «ENDFOR»
            «ENDFOR»
            «IF fEnumerationType.base != null && !fEnumerationType.enumerators.empty»,«ENDIF»
            «FOR enumerator : fEnumerationType.enumerators SEPARATOR ','»
                «enumerator.name»«enumerator.generateValue»
            «ENDFOR»
        };

        // XXX Definition of a comparator still is necessary for GCC 4.4.1, topic is fixed since 4.5.1
        struct «name»Comparator;
    '''

    def dispatch generateFTypeDeclaration(FUnionType fUnionType, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        typedef CommonAPI::Variant<«fUnionType.getElementNames»>  «fUnionType.name»;
    '''

    def private String getElementNames(FUnionType fUnion) {
        var names = "";
        if (fUnion.base != null) {
            names = fUnion.base.getElementNames
        }
        if (names != "") {
            names = ", " + names
        }
        names = fUnion.elements.map[getTypeName(fUnion)].join(", ") + names
        return names
    }


    def dispatch generateFTypeInlineImplementation(FTypeDef fTypeDef, FModelElement parent, DeploymentInterfacePropertyAccessor deploymentAccessor) ''''''
    def dispatch generateFTypeInlineImplementation(FArrayType fArrayType, FModelElement parent, DeploymentInterfacePropertyAccessor deploymentAccessor) ''''''
    def dispatch generateFTypeInlineImplementation(FMapType fMap, FModelElement parent, DeploymentInterfacePropertyAccessor deploymentAccessor) ''''''

    def dispatch generateFTypeInlineImplementation(FStructType fStructType, FModelElement parent, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        bool operator==(const «fStructType.getClassNamespace(parent)»& lhs, const «fStructType.getClassNamespace(parent)»& rhs);
        inline bool operator!=(const «fStructType.getClassNamespace(parent)»& lhs, const «fStructType.getClassNamespace(parent)»& rhs) {
            return !(lhs == rhs);
        }
    '''

    def dispatch generateFTypeInlineImplementation(FEnumerationType fEnumerationType, FModelElement parent, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        fEnumerationType.generateInlineImplementation(fEnumerationType.name, parent, parent.name, deploymentAccessor)
    }

    def generateInlineImplementation(FEnumerationType fEnumerationType, String enumerationName, FModelElement parent, String parentName, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        inline CommonAPI::InputStream& operator>>(CommonAPI::InputStream& inputStream, «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& enumValue) {
            return inputStream.readEnumValue<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(enumValue);
        }

        inline CommonAPI::OutputStream& operator<<(CommonAPI::OutputStream& outputStream, const «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& enumValue) {
            return outputStream.writeEnumValue(static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(enumValue));
        }

        struct «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»Comparator {
            inline bool operator()(const «enumerationName»& lhs, const «enumerationName»& rhs) const {
                return static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(lhs) < static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(rhs);
            }
        };
        
        «FOR base : fEnumerationType.baseList BEFORE "\n" SEPARATOR "\n"»
            «fEnumerationType.generateInlineOperatorWithName(enumerationName, base, parent, parentName, "==", deploymentAccessor)»
            «fEnumerationType.generateInlineOperatorWithName(enumerationName, base, parent, parentName, "!=", deploymentAccessor)»
        «ENDFOR»
    '''

    def dispatch generateFTypeInlineImplementation(FUnionType fUnionType, FModelElement parent, DeploymentInterfacePropertyAccessor deploymentAccessor) ''''''

    def dispatch generateFTypeImplementation(FTypeDef fTypeDef, FModelElement parent) ''''''
    def dispatch generateFTypeImplementation(FArrayType fArrayType, FModelElement parent) ''''''
    def dispatch generateFTypeImplementation(FMapType fMap, FModelElement parent) ''''''
    def dispatch generateFTypeImplementation(FEnumerationType fEnumerationType, FModelElement parent) ''''''

    def dispatch generateFTypeImplementation(FStructType fStructType, FModelElement parent) '''
        «IF fStructType.allElements.size > 0»
            «fStructType.getClassNamespace(parent)»::«fStructType.name»(«fStructType.allElements.map[getConstReferenceVariable(fStructType) + "Value"].join(", ")»):
                    «fStructType.base?.generateBaseConstructorCall(fStructType)»
                    «FOR element : fStructType.elements SEPARATOR ','»
                        «element.name»(«element.name»Value)
                    «ENDFOR»
            {
            }
        «ENDIF»

        bool operator==(const «fStructType.getClassNamespace(parent)»& lhs, const «fStructType.getClassNamespace(parent)»& rhs) {
            if (&lhs == &rhs)
                return true;

            return
                «IF fStructType.allElements.size > 0»
                    «IF fStructType.base != null»
                        static_cast<«fStructType.base.getClassNamespace(parent)»>(lhs) == static_cast<«fStructType.base.getClassNamespace(parent)»>(rhs) &&
                    «ENDIF»
                    «FOR element : fStructType.elements SEPARATOR ' &&'»
                        lhs.«element.name» == rhs.«element.name»
                    «ENDFOR»
                «ELSE»
                    true
                «ENDIF»
            ;
        }

        void «fStructType.getClassNamespace(parent)»::readFromInputStream(CommonAPI::InputStream& inputStream) {
            «IF fStructType.base != null»
                «fStructType.base.getRelativeNameReference(fStructType)»::readFromInputStream(inputStream);
            «ENDIF»
            «FOR element : fStructType.elements»
                inputStream >> «element.name»;
            «ENDFOR»
        }

        void «fStructType.getClassNamespace(parent)»::writeToOutputStream(CommonAPI::OutputStream& outputStream) const {
            «IF fStructType.base != null»
                «fStructType.base.getRelativeNameReference(fStructType)»::writeToOutputStream(outputStream);
            «ENDIF»
            «FOR element : fStructType.elements»
                outputStream << «element.name»;
            «ENDFOR»
        }
    '''

    def dispatch generateFTypeImplementation(FUnionType fUnionType, FModelElement parent) ''' '''

    def hasImplementation(FType fType) {
        return (fType instanceof FStructType);
    }

    def generateRequiredTypeIncludes(FInterface fInterface) '''
        «FOR requiredTypeHeaderFile : fInterface.requiredTypeHeaderFiles»
            #include <«requiredTypeHeaderFile»>
        «ENDFOR»
    '''

    def private getRequiredTypeHeaderFiles(FInterface fInterface) {
        val headerSet = new HashSet<String>

        fInterface.attributes.forEach[type.derived?.addRequiredHeaders(headerSet)]
        fInterface.methods.forEach[
            inArgs.forEach[type.derived?.addRequiredHeaders(headerSet)]
            outArgs.forEach[type.derived?.addRequiredHeaders(headerSet)]
        ]
        fInterface.broadcasts.forEach[outArgs.forEach[type.derived?.addRequiredHeaders(headerSet)]]

        headerSet.remove(fInterface.headerPath)

        return headerSet
    }

    def addRequiredHeaders(FType fType, Collection<String> requiredHeaders) {
        requiredHeaders.add(fType.FTypeCollection.headerPath)
        fType.addFTypeRequiredHeaders(requiredHeaders)
    }

    def private getFTypeCollection(FType fType) {
        fType.eContainer as FTypeCollection
    }

    def private dispatch void addFTypeRequiredHeaders(FTypeDef fTypeDef, Collection<String> requiredHeaders) {
        requiredHeaders.add(fTypeDef.actualType.requiredHeaderPath)
    }
    def private dispatch void addFTypeRequiredHeaders(FArrayType fArrayType, Collection<String> requiredHeaders) {
        requiredHeaders.addAll('vector', fArrayType.elementType.requiredHeaderPath)
    }
    def private dispatch void addFTypeRequiredHeaders(FMapType fMapType, Collection<String> requiredHeaders) {
        requiredHeaders.addAll('unordered_map', fMapType.keyType.requiredHeaderPath, fMapType.valueType.requiredHeaderPath)
    }
    def private dispatch void addFTypeRequiredHeaders(FStructType fStructType, Collection<String> requiredHeaders) {
        if (fStructType.base != null)
            requiredHeaders.add(fStructType.base.FTypeCollection.headerPath)
        else
            requiredHeaders.addAll('CommonAPI/InputStream.h', 'CommonAPI/OutputStream.h', 'CommonAPI/SerializableStruct.h')
        fStructType.elements.forEach[requiredHeaders.add(type.requiredHeaderPath)]
    }
    def private dispatch void addFTypeRequiredHeaders(FEnumerationType fEnumerationType, Collection<String> requiredHeaders) {
        if (fEnumerationType.base != null)
            requiredHeaders.add(fEnumerationType.base.FTypeCollection.headerPath)
        requiredHeaders.addAll('cstdint', 'CommonAPI/InputStream.h', 'CommonAPI/OutputStream.h')
    }
    def private dispatch void addFTypeRequiredHeaders(FUnionType fUnionType, Collection<String> requiredHeaders) {
        if (fUnionType.base != null)
            requiredHeaders.add(fUnionType.base.FTypeCollection.headerPath)
        else
            requiredHeaders.add('CommonAPI/SerializableVariant.h')
        fUnionType.elements.forEach[requiredHeaders.add(type.requiredHeaderPath)]
        requiredHeaders.addAll('cstdint', 'memory')
    }
    
    def private getRequiredHeaderPath(FTypeRef fTypeRef) {
        if (fTypeRef.derived != null)
            return fTypeRef.derived.FTypeCollection.headerPath
        return fTypeRef.predefined.requiredHeaderPath
    }

    def private getRequiredHeaderPath(FBasicTypeId fBasicTypeId) {
        switch fBasicTypeId {
            case FBasicTypeId::STRING : 'string'
            case FBasicTypeId::BYTE_BUFFER : 'CommonAPI/ByteBuffer.h'
            default : 'cstdint'
        }
    }

    def private getClassNamespace(FModelElement child, FModelElement parent) {
        child.getClassNamespaceWithName(child.name, parent, parent.name)
    }
    
    def private getClassNamespaceWithName(FModelElement child, String name, FModelElement parent, String parentName) {
        var reference = name
        if (parent != null && parent != child)
            reference = parentName + '::' + reference
        return reference
    }

    def private generateBaseConstructorCall(FStructType parent, FStructType source) {
        var call = parent.getRelativeNameReference(source)
        call = call + "(" + parent.allElements.map[name + "Value"].join(", ") + ")"
        if (!source.elements.empty)
            call = call + ","
        return call
    }


    def private generateKeyType(FMapType fMap) '''«fMap.keyType.getNameReference(fMap.eContainer)»'''
    def private generateValueType(FMapType fMap) ''', «fMap.valueType.getNameReference(fMap.eContainer)»'''

    def private getBaseStructName(FStructType fStructType) {
        if (fStructType.base != null)
            return fStructType.base.getRelativeNameReference(fStructType)
        return "CommonAPI::SerializableStruct"
    }

    def private getConstReferenceVariable(FField destination, FModelElement source) {
        "const " + destination.getTypeName(source) + "& " + destination.name
    }

    def private List<FField> getAllElements(FStructType fStructType) {
        if (fStructType.base == null)
            return new LinkedList(fStructType.elements)

        val elements = fStructType.base.allElements
        elements.addAll(fStructType.elements)
        return elements
    }

    def private generateInlineOperatorWithName(FEnumerationType fEnumerationType, String enumerationName, FEnumerationType base, FModelElement parent, String parentName, String operator, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        inline bool operator«operator»(const «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& lhs, const «base.getRelativeNameReference(fEnumerationType)»& rhs) {
            return static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(lhs) «operator» static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(rhs);
        }
        inline bool operator«operator»(const «base.getRelativeNameReference(fEnumerationType)»& lhs, const «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& rhs) {
            return static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(lhs) «operator» static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(rhs);
        }
    '''

    def private getBaseList(FEnumerationType fEnumerationType) {
        val baseList = new LinkedList<FEnumerationType>
        var currentBase = fEnumerationType.base
        
        while (currentBase != null) {
            baseList.add(0, currentBase)
            currentBase = currentBase.base
        }

        return baseList
    }

    def private generateValue(FEnumerator fEnumerator) {
        val parsedValue = tryParseInteger(fEnumerator.value)
        if (parsedValue != null)
            return ' = ' + parsedValue
        return ''
    }

    def private tryParseInteger(String string) {
        if (!string.nullOrEmpty) {
            if (string.startsWith("0x") || string.startsWith("0X")) {
                try {
                    return "0x" + Integer::toHexString((Integer::parseInt(string.substring(2), 16)))
                } catch (NumberFormatException e) {
                    return null
                }
            } else if (string.startsWith("0b") || string.startsWith("0B")) {
                try {
                    return "0x" + Integer::toHexString((Integer::parseInt(string.substring(2), 2)))
                } catch (NumberFormatException e) {
                    return null
                }
            } else if (string.startsWith("0")) {
                try {
                    Integer::parseInt(string, 8)
                    return string
                } catch (NumberFormatException e) {
                    return null
                }
            } else {
                try {
                    return Integer::parseInt(string, 10)
                } catch (NumberFormatException e) {
                    try {
                        return "0x" + Integer::toHexString((Integer::parseInt(string, 16)))
                    } catch (NumberFormatException e2) {
                        return null
                    }
                }
            }
        }
        return null
    }
}
