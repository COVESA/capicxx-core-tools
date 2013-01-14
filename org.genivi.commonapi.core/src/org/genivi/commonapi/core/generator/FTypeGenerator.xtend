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

import static org.genivi.commonapi.core.generator.FTypeGenerator.*
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FInterface
import java.util.HashSet

class FTypeGenerator {
	@Inject private extension FrancaGeneratorExtensions

    def dispatch generateFTypeDeclaration(FTypeDef fTypeDef) '''
        typedef «fTypeDef.actualType.getNameReference(fTypeDef.eContainer)» «fTypeDef.name»;
    '''

    def dispatch generateFTypeDeclaration(FArrayType fArrayType) '''
        typedef std::vector<«fArrayType.elementType.getNameReference(fArrayType.eContainer)»> «fArrayType.name»;
    '''

    def dispatch generateFTypeDeclaration(FMapType fMap) '''
        typedef std::unordered_map<«fMap.generateKeyType»«fMap.generateValueType»«fMap.generateComparatorType»> «fMap.name»;
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
    
    /*
     * TODO
     * 
     *   - Remove "Value" suffix for Variant members (Avoid fidl convention name clashing)
     * 
     *   - Full serialization over CommonAPI::*Stream!
     * 
     *   - Very inefficient setters, since contents of std::shared_ptr is always being copied!
     *     The FUnionType could be implemented as something like a "std::union_ptr<>", whose
     *     contents could be swapped.
     */
    def dispatch generateFTypeDeclaration(FUnionType fUnionType) '''
        class «fUnionType.name»: public «fUnionType.baseVariantName» {
         public:
            «fUnionType.name»() = default;
            «FOR parent : fUnionType.baseList»
                «FOR element : parent.elements»
                    «fUnionType.name»(const «element.type.getNameReference(fUnionType)»& «element.name»Value);
                «ENDFOR»
            «ENDFOR»
            «FOR element : fUnionType.elements»
                «fUnionType.name»(const «element.type.getNameReference(fUnionType)»& «element.name»Value);
            «ENDFOR»

            virtual ~«fUnionType.name»() { }

            «FOR element : fUnionType.elements»
                void set«element.name.toFirstUpper»Value(const std::shared_ptr<«element.type.getNameReference(fUnionType)»>& «element.name»Value);
            «ENDFOR»

            «FOR element : fUnionType.elements»
                const std::shared_ptr<«element.type.getNameReference(fUnionType)»>& get«element.name.toFirstUpper»Value() const;
            «ENDFOR»

            «FOR element : fUnionType.elements»
                inline bool is«element.name.toFirstUpper»Type() const;
            «ENDFOR»

            «FOR element : fUnionType.elements»
                static const uint32_t «element.name.toFirstUpper»Type;
            «ENDFOR»

         protected:
            enum class Type: uint32_t {
                _FIRST_TYPE«fUnionType.base?.generateLastTypeReference(fUnionType)»,
                «FOR element : fUnionType.elements»
                    «element.name.toUpperCase»,
                «ENDFOR»
                _LAST_TYPE
            };
        };
    '''


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

    def dispatch generateFTypeInlineImplementation(FUnionType fUnionType, FModelElement parent) '''
        «FOR element : fUnionType.elements»
            bool «fUnionType.getClassNamespace(parent)»::is«element.name.toFirstUpper»Type() const {
                return getType() == «element.name.toFirstUpper»Type;
            }
        «ENDFOR»
    '''


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

    def dispatch generateFTypeImplementation(FUnionType fUnionType, FModelElement parent) '''
    «FOR element : fUnionType.elements»
        const uint32_t «fUnionType.getClassNamespace(parent)»::«element.name.toFirstUpper»Type = static_cast<uint32_t>(Type::«element.name.toUpperCase»);
    «ENDFOR»

    «FOR base : fUnionType.baseList»
        «FOR element : base.elements»
            «fUnionType.getClassNamespace(parent)»::«fUnionType.name»(const «element.type.getNameReference(fUnionType)»& «element.name»Value):
                    «base.getRelativeNameReference(fUnionType)»(«element.name»Value) {
            }
        «ENDFOR»
    «ENDFOR»
    «FOR element : fUnionType.elements»
        «fUnionType.getClassNamespace(parent)»::«fUnionType.name»(const «element.type.getNameReference(fUnionType)»& «element.name»Value):
                type_(«element.name.toFirstUpper»Type),
                value_(std::make_shared<«element.type.getNameReference(fUnionType)»>(«element.name»Value)) {
        }
    «ENDFOR»

    «FOR element : fUnionType.elements»
        void «fUnionType.getClassNamespace(parent)»::set«element.name.toFirstUpper»Value(const std::shared_ptr<«element.type.getNameReference(fUnionType)»>& «element.name»Value) {
            type_ = «element.name.toFirstUpper»Type;
            value_ = std::make_shared<«element.type.getNameReference(fUnionType)»>(*«element.name»Value.get());
        }
    «ENDFOR»

    «FOR element : fUnionType.elements»
        const std::shared_ptr<«element.type.getNameReference(fUnionType)»>& «fUnionType.getClassNamespace(parent)»::get«element.name.toFirstUpper»Value() const {
            return (type_ == «element.name.toFirstUpper»Type) ?
                    std::static_pointer_cast<«element.type.getNameReference(fUnionType)»>(value_) :
                    std::shared_ptr<«element.type.getNameReference(fUnionType)»>();
        }
    «ENDFOR»
    '''

    def hasImplementation(FType fType) {
        return (fType instanceof FStructType) ||
                (fType instanceof FUnionType);
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

    def private generateLastTypeReference(FUnionType parent, FUnionType source) {
        " = " + parent.getRelativeNameReference(source) + "::Type::_LAST_TYPE"
    }

    def private getBaseVariantName(FUnionType fUnionType) {
        if (fUnionType.base != null)
            return fUnionType.base.getRelativeNameReference(fUnionType)
        return "CommonAPI::SerializableVariant"
    }

    def private isFEnumerationType(FMapType fMap) {
        fMap.keyType.derived instanceof FEnumerationType
    }
    def private generateKeyType(FMapType fMap) '''«fMap.keyType.getNameReference(fMap.eContainer)»'''
    def private generateValueType(FMapType fMap) ''', «fMap.valueType.getNameReference(fMap.eContainer)»'''
    def private generateComparatorType(FMapType fMap) '''«IF fMap.isFEnumerationType», «fMap.generateKeyType»Comparator«ENDIF»'''

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

    def private getBaseList(FUnionType fUnionType) {
        val baseList = new LinkedList<FUnionType>
        var currentBase = fUnionType.base
        
        while (currentBase != null) {
            baseList.add(0, currentBase)
            currentBase = currentBase.base
        }

        return baseList
    }

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