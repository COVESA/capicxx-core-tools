/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.Arrays
import java.util.Collection
import java.util.LinkedList
import java.util.List
import javax.inject.Inject
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FEnumerator
import org.franca.core.franca.FField
import org.franca.core.franca.FMapType
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FStructType
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FUnionType
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FInterface
import java.util.HashSet
import org.eclipse.emf.common.util.EList
import java.util.Comparator
import java.util.Set

import static org.genivi.commonapi.core.generator.FTypeGenerator.*

class TypeListSorter implements Comparator<FType> {
    @Inject private extension FTypeGenerator
    
    override compare(FType lhs, FType rhs) {
        if(lhs == rhs) return 0
        if(lhs.referencedTypes.contains(rhs)) {
            return 1
        }
        if(rhs.referencedTypes.contains(lhs)) {
            return -1
        }
        return 1
    }

    override equals(Object obj) {
        throw new UnsupportedOperationException()
    }
}

class FTypeGenerator {
	@Inject private extension FrancaGeneratorExtensions
	@Inject private extension TypeListSorter typeListSorter
	

    def generateFTypeDeclarations(FTypeCollection fTypeCollection) '''
        «FOR type: fTypeCollection.types.sortTypes(fTypeCollection)»
            «type.generateFTypeDeclaration»
        «ENDFOR»
    '''

    def private sortTypes(EList<FType> typeList, FTypeCollection containingTypeCollection) {
        var typeBlobs = new LinkedList<Set<FType>>()
        for(type: typeList) {
            var Set<FType> containingBlob

            for(blob: typeBlobs) {
                if(blob.contains(type) || !type.referencedTypes.filter[blob.contains(it)].empty) {
                    containingBlob = blob
                }
            }
            
            if(containingBlob == null) {
                var newBlob = new HashSet<FType>()
                newBlob.add(type)
                newBlob.addAll(type.referencedTypes.filter[containingTypeCollection.types.contains(it)])
                typeBlobs.add(newBlob)
            } else {
                containingBlob.add(type)
                containingBlob.addAll(type.referencedTypes.filter[containingTypeCollection.types.contains(it)])
            }
        }
        
        var sortedTypes = new LinkedList<FType>()
        for(blob: typeBlobs) {
            var sortedBlob = blob.toList
            sortedBlob = sortedBlob.sort(typeListSorter)
            sortedTypes.addAll(sortedBlob)
        }
        
        return sortedTypes
    }

    def dispatch List<FType> getReferencedTypes(FStructType fType) {
        var references = fType.elements.map[it.type.derived].filter[it != null].toList
        if(fType.base != null) {
            references.add(fType.base)
        }
        references.addAll(references.map[it.referencedTypes].flatten.toList)
        return references
    }

    def dispatch List<FType> getReferencedTypes(FEnumerationType fType) {
        var references = new LinkedList<FType>()
        if(fType.base != null) {
            references.add(fType.base)
        }
        references.addAll(references.map[it.referencedTypes].flatten.toList)
        return references
    }

    def dispatch List<FType> getReferencedTypes(FArrayType fType) {
        var references = new LinkedList<FType>()
        if(fType.elementType.derived != null) {
            references.add(fType.elementType.derived)
        }
        references.addAll(references.map[it.referencedTypes].flatten.toList)
        return references
    }
    
    def dispatch List<FType> getReferencedTypes(FUnionType fType) {
        var references = fType.elements.map[it.type.derived].filter[it != null].toList
        if(fType.base != null) {
            references.add(fType.base)
        }
        references.addAll(references.map[it.referencedTypes].flatten.toList)
        return references
    }
    
    def dispatch List<FType> getReferencedTypes(FMapType fType) {
        var references = new LinkedList<FType>()
        if(fType.keyType.derived != null) {
            references.add(fType.keyType.derived)
        }
        if(fType.valueType.derived != null) {
            references.add(fType.valueType.derived)
        }
        references.addAll(references.map[it.referencedTypes].flatten.toList)
        return references
    }
    
    def dispatch List<FType> getReferencedTypes(FTypeDef fType) {
        var references = new LinkedList<FType>()
        if(fType.actualType.derived != null) {
            references.add(fType.actualType.derived)
        }
        references.addAll(references.map[it.referencedTypes].flatten.toList)
        return references
    }

    def dispatch generateFTypeDeclaration(FTypeDef fTypeDef) '''
        typedef «fTypeDef.actualType.getNameReference(fTypeDef.eContainer)» «fTypeDef.name»;
    '''

    def dispatch generateFTypeDeclaration(FArrayType fArrayType) '''
        typedef std::vector<«fArrayType.elementType.getNameReference(fArrayType.eContainer)»> «fArrayType.name»;
    '''

    def dispatch generateFTypeDeclaration(FMapType fMap) '''
        typedef std::unordered_map<«fMap.generateKeyType»«fMap.generateValueType»> «fMap.name»;
    '''

    def dispatch generateFTypeDeclaration(FStructType fStructType) '''
        struct «fStructType.name»: «fStructType.baseStructName» {
            «FOR element : fStructType.elements»
                «element.type.getNameReference(fStructType)» «element.name»;
            «ENDFOR»

            «fStructType.name»() = default;
            «fStructType.name»(«fStructType.allElements.map[getConstReferenceVariable(fStructType)].join(", ")»);

            virtual void readFromInputStream(CommonAPI::InputStream& inputStream);
            virtual void writeToOutputStream(CommonAPI::OutputStream& outputStream) const;

            static inline void writeToTypeOutputStream(CommonAPI::TypeOutputStream& typeOutputStream) {
                «IF fStructType.base != null»
                    «fStructType.baseStructName»::writeToTypeOutputStream(typeOutputStream);
                «ENDIF»
                «FOR element : fStructType.elements»
                    «element.type.typeStreamSignature»
                «ENDFOR»
            }
        };
    '''
    
    def dispatch generateFTypeDeclaration(FEnumerationType fEnumerationType) {
        fEnumerationType.generateDeclaration(fEnumerationType.name)
    }

    def generateDeclaration(FEnumerationType fEnumerationType, String name) '''
        enum class «name»: «fEnumerationType.backingType.primitiveTypeName» {
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

    def dispatch generateFTypeDeclaration(FUnionType fUnionType) '''
		typedef CommonAPI::Variant<«fUnionType.getElementNames»>  «fUnionType.name»;
    '''

    def private getElementNames(FUnionType fUnion) {
   		var names = "";
    	if (fUnion.base != null) {
    		names = fUnion.base.getElementNames
    	}
    	if (names != "") {
    		names = ", " + names
    	}
    	names = fUnion.elements.map[type.getNameReference(fUnion)].join(", ") + names
    	return names
    }


    def dispatch generateFTypeInlineImplementation(FTypeDef fTypeDef, FModelElement parent) ''''''
    def dispatch generateFTypeInlineImplementation(FArrayType fArrayType, FModelElement parent) ''''''
    def dispatch generateFTypeInlineImplementation(FMapType fMap, FModelElement parent) ''''''

    def dispatch generateFTypeInlineImplementation(FStructType fStructType, FModelElement parent) '''
        bool operator==(const «fStructType.getClassNamespace(parent)»& lhs, const «fStructType.getClassNamespace(parent)»& rhs);
        inline bool operator!=(const «fStructType.getClassNamespace(parent)»& lhs, const «fStructType.getClassNamespace(parent)»& rhs) {
            return !(lhs == rhs);
        }
    '''

    def dispatch generateFTypeInlineImplementation(FEnumerationType fEnumerationType, FModelElement parent) {
        fEnumerationType.generateInlineImplementation(fEnumerationType.name, parent, parent.name)
    }

    def generateInlineImplementation(FEnumerationType fEnumerationType, String enumerationName, FModelElement parent, String parentName) '''
        inline CommonAPI::InputStream& operator>>(CommonAPI::InputStream& inputStream, «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& enumValue) {
            return inputStream.readEnumValue<«fEnumerationType.backingType.primitiveTypeName»>(enumValue);
        }

        inline CommonAPI::OutputStream& operator<<(CommonAPI::OutputStream& outputStream, const «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& enumValue) {
            return outputStream.writeEnumValue(static_cast<«fEnumerationType.backingType.primitiveTypeName»>(enumValue));
        }

        struct «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»Comparator {
            inline bool operator()(const «enumerationName»& lhs, const «enumerationName»& rhs) const {
                return static_cast<«fEnumerationType.backingType.primitiveTypeName»>(lhs) < static_cast<«fEnumerationType.backingType.primitiveTypeName»>(rhs);
            }
        };
        
        «FOR base : fEnumerationType.baseList BEFORE "\n" SEPARATOR "\n"»
            «fEnumerationType.generateInlineOperatorWithName(enumerationName, base, parent, parentName, "==")»
            «fEnumerationType.generateInlineOperatorWithName(enumerationName, base, parent, parentName, "!=")»
        «ENDFOR»
    '''

    def dispatch generateFTypeInlineImplementation(FUnionType fUnionType, FModelElement parent) ''' '''
    def dispatch generateFTypeImplementation(FTypeDef fTypeDef, FModelElement parent) ''''''
    def dispatch generateFTypeImplementation(FArrayType fArrayType, FModelElement parent) ''''''
    def dispatch generateFTypeImplementation(FMapType fMap, FModelElement parent) ''''''
    def dispatch generateFTypeImplementation(FEnumerationType fEnumerationType, FModelElement parent) ''''''

    def dispatch generateFTypeImplementation(FStructType fStructType, FModelElement parent) '''
        «fStructType.getClassNamespace(parent)»::«fStructType.name»(«fStructType.allElements.map[getConstReferenceVariable(fStructType) + "Value"].join(", ")»):
                «fStructType.base?.generateBaseConstructorCall(fStructType)»
                «FOR element : fStructType.elements SEPARATOR ','»
                    «element.name»(«element.name»Value)
                «ENDFOR»
        {
        }

        bool operator==(const «fStructType.getClassNamespace(parent)»& lhs, const «fStructType.getClassNamespace(parent)»& rhs) {
            if (&lhs == &rhs)
                return true;

            return
                «IF fStructType.base != null»
                    static_cast<«fStructType.base.getClassNamespace(parent)»>(lhs) == static_cast<«fStructType.base.getClassNamespace(parent)»>(rhs) &&
                «ENDIF»
                «FOR element : fStructType.elements SEPARATOR ' &&'»
                    lhs.«element.name» == rhs.«element.name»
                «ENDFOR»
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
        "const " + destination.type.getNameReference(source) + "& " + destination.name
    }

    def private List<FField> getAllElements(FStructType fStructType) {
        if (fStructType.base == null)
            return new LinkedList(fStructType.elements)

        val elements = fStructType.base.allElements
        elements.addAll(fStructType.elements)
        return elements
    }

    def private generateInlineOperatorWithName(FEnumerationType fEnumerationType, String enumerationName, FEnumerationType base, FModelElement parent, String parentName, String operator) '''
        inline bool operator«operator»(const «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& lhs, const «base.getRelativeNameReference(fEnumerationType)»& rhs) {
            return static_cast<«fEnumerationType.backingType.primitiveTypeName»>(lhs) «operator» static_cast<«fEnumerationType.backingType.primitiveTypeName»>(rhs);
        }
        inline bool operator«operator»(const «base.getRelativeNameReference(fEnumerationType)»& lhs, const «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& rhs) {
            return static_cast<«fEnumerationType.backingType.primitiveTypeName»>(lhs) «operator» static_cast<«fEnumerationType.backingType.primitiveTypeName»>(rhs);
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

    private static List<Integer> integerRadixList = Arrays::asList(10, 16, 2, 8)

    def private tryParseInteger(String string) {
        if (!string.nullOrEmpty) {
            try {
                return Integer::decode(string)
            } catch (NumberFormatException e1) {
                for (radix : integerRadixList)
                    try {
                        return new Integer(Integer::parseInt(string, radix.intValue))
                    } catch (NumberFormatException e2) { }
            }
        }
        return null
    }
}