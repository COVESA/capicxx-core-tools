/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.Collection
import java.util.HashSet
import java.util.LinkedList
import java.util.List
import java.util.Set
import javax.inject.Inject
import org.eclipse.emf.common.util.EList
import org.franca.core.franca.FAnnotationBlock
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FField
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMapType
import org.franca.core.franca.FMethod
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FStructType
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FUnionType
import org.genivi.commonapi.core.deployment.PropertyAccessor

import static com.google.common.base.Preconditions.*

import static extension org.genivi.commonapi.core.generator.FrancaGeneratorExtensions.*

class FTypeGenerator {
    @Inject private extension FrancaGeneratorExtensions francaGeneratorExtensions

    def static isdeprecated(FAnnotationBlock annotations) {
        if(annotations == null)
            return false
        for(annotation : annotations.elements) {
            if(annotation.type.value.equals(FAnnotationType::DEPRECATED_VALUE))
                return true;
        }
        return false
    }

    def static breaktext(String text, FAnnotationType annotation) {
        var commentBody = ""
        var startIndex = 0
        var endIndex = 0
        var commentText = text.replace("\r\n", "\n")
        commentBody += " * " + annotation.getName() + ": "
        // do for all lines in the comment
        while (startIndex < commentText.length) {
            endIndex = commentText.indexOf('\n', startIndex);
            if (endIndex == -1) {
                endIndex = commentText.length
            }
            commentBody += "\n * " + commentText.substring(startIndex, endIndex).trim()
            startIndex = endIndex + 1
        }

        return commentBody;
    }

    def static generateComments(FModelElement model, boolean inline) {
        var intro = ""
        var tail = ""
        var annoCommentText = ""
        if( model != null && model.comment != null){
            if(!inline) {
                intro = "/*\n"
                tail = "\n */"
            }
            for(annoComment : model.comment.elements) {
                if(annoComment != null){
                    annoCommentText += breaktext(annoComment.comment, annoComment.type)
                }
            }
            return intro + annoCommentText + tail
        }
        return ""
    }


    def static getTyp(FModelElement element) {
        if(element instanceof FInterface || element instanceof FTypeCollection)
            return ModelTyp::INTERFACE
        if(element instanceof FMethod)
            return ModelTyp::METHOD
        if(element instanceof FEnumerationType)
            return ModelTyp::ENUM
        return ModelTyp::UNKNOWN
    }

    def generateFTypeDeclarations(FTypeCollection fTypeCollection, PropertyAccessor deploymentAccessor) '''
        «FOR type: fTypeCollection.types.sortTypes(fTypeCollection)»
            «type.generateFTypeDeclaration(deploymentAccessor)»
        «ENDFOR»
        «IF fTypeCollection instanceof FInterface»
            «FOR method : fTypeCollection.methodsWithError»
                «generateComments(method, false)»
                «method.errors.generateDeclaration(method.errors, deploymentAccessor)»
            «ENDFOR»
        «ENDIF»
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

    def dispatch generateFTypeDeclaration(FTypeDef fTypeDef, PropertyAccessor deploymentAccessor) '''
        «generateComments(fTypeDef, false)»
        typedef «fTypeDef.actualType.getElementType(fTypeDef, false)» «fTypeDef.elementName»;
    '''

    def dispatch generateFTypeDeclaration(FArrayType fArrayType, PropertyAccessor deploymentAccessor) '''
        «generateComments(fArrayType, false)»
        «IF fArrayType.elementType.derived != null && fArrayType.elementType.derived instanceof FStructType && (fArrayType.elementType.derived as FStructType).polymorphic»
            typedef std::vector<std::shared_ptr< «fArrayType.elementType.getElementType(fArrayType, true)»>> «fArrayType.elementName»;
        «ELSE»
            typedef std::vector< «fArrayType.elementType.getElementType(fArrayType, true)»> «fArrayType.elementName»;
        «ENDIF»
    '''

    def dispatch generateFTypeDeclaration(FMapType fMap, PropertyAccessor deploymentAccessor) '''
        «generateComments(fMap, false)»
        typedef std::unordered_map< «fMap.generateKeyType», «fMap.generateValueType»«fMap.generateHasher»> «fMap.elementName»;
    '''

    def dispatch generateFTypeDeclaration(FStructType fStructType, PropertyAccessor deploymentAccessor) '''
        «generateComments(fStructType, false)»
        «IF fStructType.polymorphic»
        «fStructType.createSerials()»

        «ENDIF»
        «IF fStructType.hasPolymorphicBase()»
            «IF fStructType.base == null»
            struct «fStructType.elementName» : CommonAPI::PolymorphicStruct {
            «ELSE»
            struct «fStructType.elementName» : «fStructType.base.getElementName(fStructType, false)» {
            «ENDIF»
        «ELSE»
            struct «fStructType.elementName» : CommonAPI::Struct< «fStructType.allElements.map[getTypeName(fStructType, false)].join(", ")»> {
        «ENDIF»
            «IF fStructType.hasPolymorphicBase()»
                «IF fStructType.polymorphic || (fStructType.hasPolymorphicBase() && fStructType.hasDerivedTypes())»
                static COMMONAPI_EXPORT std::shared_ptr< «fStructType.elementName»> create(CommonAPI::Serial _serial);
                «ENDIF»
                CommonAPI::Serial getSerial() const { return «fStructType.elementName.toUpperCase»_SERIAL; }
            «ENDIF»

            «fStructType.elementName»()
            «IF fStructType.hasPolymorphicBase() && fStructType.base != null»
                : «fStructType.base.elementName»()
            «ENDIF»
            {
            «IF fStructType.allElements.size > 0»
            «var n = 0»
                «FOR element : fStructType.allElements»
                    «var nindex = n»
                    «IF (fStructType.hasPolymorphicBase)»
                        «{ nindex = n - (fStructType.allElements.size - fStructType.elements.size); "" }»
                    «ENDIF»
                    «IF nindex >= 0 »
                        «IF (element.type.derived instanceof FStructType && (element.type.derived as FStructType).hasPolymorphicBase) && !element.array»
                                std::get< «nindex»>(values_) = std::make_shared< «element.type.getElementType(fStructType, false)»>();
                        «ELSEIF element.type.derived != null && !element.array»
                                std::get< «nindex»>(values_) = «element.getTypeName(fStructType, false)»();
                        «ELSEIF element.type.predefined != null && !element.array»
                                std::get< «nindex»>(values_) = «element.type.generateDummyValue()»;
                        «ELSE»
                                std::get< «nindex»>(values_) = «element.getTypeName(fStructType, false)»();
                        «ENDIF»
                    «ENDIF»
                    «{ n = n + 1; "" }»
                «ENDFOR»
            «ENDIF»
            }
            «IF fStructType.allElements.size > 0»
                «fStructType.elementName»(«fStructType.allElements.map[getConstReferenceVariable(fStructType)].join(", ")»)
                «IF fStructType.hasPolymorphicBase() && fStructType.base != null»
                    : «fStructType.base.elementName»(«fStructType.base.allElements.map["_" + elementName].join(", ")»)
                «ENDIF»
                {
                    «IF fStructType.hasPolymorphicBase»
                        «var i = -1»
                        «FOR element : fStructType.elements»
                            std::get< «i = i+1»>(values_) = _«element.elementName»;
                        «ENDFOR»
                    «ELSE»
                        «var i = -1»
                        «FOR element : fStructType.allElements»
                            std::get< «i = i+1»>(values_) = _«element.elementName»;
                        «ENDFOR»
                    «ENDIF»
                }
            «ENDIF»
            «IF fStructType.hasPolymorphicBase()»
            template<class _Input>
            void readValue(CommonAPI::InputStream<_Input> &_input, const CommonAPI::EmptyDeployment *_depl) {
                «IF fStructType.derivedFStructTypes.empty»
                (void) _depl;
                «ENDIF»
                «var i = -1»
                «FOR element : fStructType.elements»
                _input.template readValue<CommonAPI::EmptyDeployment>(std::get< «i = i+1»>(values_));
                «ENDFOR»
                «IF fStructType.hasDerivedTypes()»
                switch (getSerial()) {
                «FOR derived : fStructType.derivedFStructTypes»
                «derived.generateCases(null, false)»
                    static_cast< «derived.elementName» *>(this)->template readValue<_Input>(_input, _depl);
                    break;
                «ENDFOR»
                default:
                    break;
                }
                «ENDIF»
            }

            template<class _Input, class _Deployment>
            void readValue(CommonAPI::InputStream<_Input> &_input, const _Deployment *_depl) {
                «var j = -1»
                «var k = fStructType.allElements.size - fStructType.elements.size - 1»
                «FOR element : fStructType.elements»
                _input.template readValue<>(std::get< «j = j+1»>(values_), std::get< «k = k+1»>(_depl->values_));
                «ENDFOR»
                «IF fStructType.hasDerivedTypes()»
                switch (getSerial()) {
                «FOR derived : fStructType.derivedFStructTypes»
                «derived.generateCases(null, false)»
                    static_cast< «derived.elementName» *>(this)->template readValue<>(_input, _depl);
                    break;
                «ENDFOR»
                default:
                    break;
                }
                «ENDIF»
            }
            template<class _Output>
            void writeType(CommonAPI::TypeOutputStream<_Output> &_output, const CommonAPI::EmptyDeployment *_depl) {
                «var l = -1»
                «FOR element : fStructType.elements»
                _output.writeType(std::get< «l = l+1»>(values_), _depl);
                «ENDFOR»
                «IF fStructType.hasDerivedTypes()»
                switch (getSerial()) {
                «FOR derived : fStructType.derivedFStructTypes»
                «derived.generateCases(null, false)»
                    static_cast< «derived.elementName» *>(this)->template writeType<_Output>(_output, _depl);
                    break;
                «ENDFOR»
                default:
                    break;
                }
                «ENDIF»
            }
            template<class _Output, class _Deployment>
            void writeType(CommonAPI::TypeOutputStream<_Output> &_output, const _Deployment *_depl) {
                «var l1 = -1»
                «var l2 = fStructType.allElements.size - fStructType.elements.size - 1»
                «FOR element : fStructType.elements»
                _output.writeType(std::get< «l1 = l1+1»>(values_), std::get< «l2 = l2+1»>(_depl->values_));
                «ENDFOR»
                «IF fStructType.hasDerivedTypes()»
                switch (getSerial()) {
                «FOR derived : fStructType.derivedFStructTypes»
                «derived.generateCases(null, false)»
                    static_cast< «derived.elementName» *>(this)->template writeType<_Output, _Deployment>(_output, _depl);
                    break;
                «ENDFOR»
                default:
                    break;
                }
                «ENDIF»
            }

            template<class _Output>
            void writeValue(CommonAPI::OutputStream<_Output> &_output, const CommonAPI::EmptyDeployment *_depl) {
                «IF fStructType.derivedFStructTypes.empty»
                (void) _depl;
                «ENDIF»
                «var m = -1»
                «FOR element : fStructType.elements»
                _output.template writeValue<CommonAPI::EmptyDeployment>(std::get< «m = m+1»>(values_));
                «ENDFOR»
                «IF fStructType.hasDerivedTypes()»
                switch (getSerial()) {
                «FOR derived : fStructType.derivedFStructTypes»
                «derived.generateCases(null, false)»
                    static_cast< «derived.elementName» *>(this)->template writeValue<_Output>(_output, _depl);
                    break;
                «ENDFOR»
                default:
                    break;
                }
                «ENDIF»
            }

            template<class _Output, class _Deployment>
            void writeValue(CommonAPI::OutputStream<_Output> &_output, const _Deployment *_depl) {
                «var n = -1»
                «var o = fStructType.allElements.size - fStructType.elements.size - 1»
                «FOR element : fStructType.elements»
                _output.template writeValue<>(std::get< «n = n+1»>(values_), std::get< «o = o + 1»>(_depl->values_));
                «ENDFOR»
                «IF fStructType.hasDerivedTypes()»
                switch (getSerial()) {
                «FOR derived : fStructType.derivedFStructTypes»
                «derived.generateCases(null, false)»
                    static_cast< «derived.elementName» *>(this)->template writeValue<>(_output, _depl);
                    break;
                «ENDFOR»
                default:
                    break;
                }
                «ENDIF»
            }
            «var p = -1»
            «FOR element : fStructType.elements»
                «generateComments(element, false)»
                «val String typeName = element.getTypeName(fStructType, false)»
                inline const «typeName» &get«element.elementName.toFirstUpper»() const { return std::get< «p = p+1»>(values_); }
                inline void set«element.elementName.toFirstUpper»(const «typeName» «IF typeName.isComplex»&«ENDIF»_value) { std::get< «p»>(values_) = _value; }
            «ENDFOR»

            «IF fStructType.hasPolymorphicBase() && fStructType.elements.size > 0»
            std::tuple< «fStructType.elements.map[getTypeName(fStructType, false)].join(", ")»> values_;
            «ENDIF»
        «ELSE»
            «var k = -1»
            «FOR element : fStructType.allElements»
                «generateComments(element, false)»
                «val String typeName = element.getTypeName(fStructType, false)»
                inline const «typeName» &get«element.elementName.toFirstUpper»() const { return std::get< «k = k+1»>(values_); }
                inline void set«element.elementName.toFirstUpper»(const «typeName» «IF typeName.isComplex»&«ENDIF»_value) { std::get< «k»>(values_) = _value; }
            «ENDFOR»
        «ENDIF»
            inline bool operator==(const «fStructType.name»& _other) const {
            «IF fStructType.allElements.size > 0»
                «FOR element : fStructType.allElements BEFORE
                'return (' SEPARATOR ' && ' AFTER ');'»get«element.elementName.toFirstUpper»() == _other.get«element.elementName.toFirstUpper»()«ENDFOR»
            «ELSE»
                (void) _other;
                return true;
            «ENDIF»    }
            inline bool operator!=(const «fStructType.name» &_other) const {
                return !((*this) == _other);
            }

        };
    '''

    def dispatch generateFTypeDeclaration(FEnumerationType fEnumerationType, PropertyAccessor deploymentAccessor) {
        generateDeclaration(fEnumerationType, fEnumerationType, deploymentAccessor)
    }

    def String getInitialValue(FEnumerationType _enumeration) {
        if (_enumeration.base != null)
            return _enumeration.base.getInitialValue()
        return "Literal::" + enumPrefix + _enumeration.enumerators.head.elementName
    }

    def String generateLiterals(FEnumerationType _enumeration, String _backingType) {
        var String literals = new String()
        var String comment = ""
        if (_enumeration.base != null)
            literals += _enumeration.base.generateLiterals(_backingType) + ",\n"
        for (enumerator : _enumeration.enumerators) {
            comment = generateComments(enumerator, false)
            if (comment != "") {
                literals += comment + "\n"
            }
            literals += enumPrefix + enumerator.elementName + " = " + enumerator.value.enumeratorValue.doCast(_backingType)
            if (enumerator != _enumeration.enumerators.last) literals += ",\n"
        }
        return literals
    }

    def String generateLiteralValidation(FEnumerationType _enumeration, String backingType, Set<String> _values) '''
        «IF _enumeration.base!=null»«_enumeration.base.generateLiteralValidation(backingType, _values)»«ENDIF»
        «FOR enumerator : _enumeration.enumerators»
            «IF _values.contains(enumerator.value.enumeratorValue)»//«ENDIF»case static_cast< «backingType»>(Literal::«enumPrefix»«enumerator.elementName»):
            «{_values.add(enumerator.value.enumeratorValue) ""}»
        «ENDFOR»
    '''

    def generateDeclaration(FEnumerationType _enumeration, FModelElement _parent, PropertyAccessor _accessor) '''
        «_enumeration.setEnumerationValues»
        «IF _enumeration.name == null»
            «IF _parent.name != null»
                «_enumeration.name = _parent.name + "Enum"»
            «ELSE»
                «_enumeration.name = (_parent.eContainer as FModelElement).name + "Error"»
            «ENDIF»
        «ENDIF»
        «val backingType = _enumeration.getBackingType(_accessor).primitiveTypeName»

        struct «_enumeration.name» : CommonAPI::Enumeration< «backingType»> {
            enum Literal : «backingType» {
                «_enumeration.generateLiterals(backingType)»
            };

            «_enumeration.name»()
                : CommonAPI::Enumeration< «backingType»>(static_cast< «backingType»>(«_enumeration.initialValue»)) {}
            «_enumeration.name»(Literal _literal)
                : CommonAPI::Enumeration< «backingType»>(static_cast< «backingType»>(_literal)) {}
            «_enumeration.generateBaseTypeAssignmentOperator(_enumeration, _accessor)»

            inline bool validate() const {
                switch (value_) {
                    «var Set<String> values = new HashSet<String>»
                    «_enumeration.generateLiteralValidation(backingType, values)»
                    return true;
                default:
                    return false;
                }
            }

            inline bool operator==(const «_enumeration.name» &_other) const { return (value_ == _other.value_); }
            inline bool operator!=(const «_enumeration.name» &_other) const { return (value_ != _other.value_); }
            inline bool operator<=(const «_enumeration.name» &_other) const { return (value_ <= _other.value_); }
            inline bool operator>=(const «_enumeration.name» &_other) const { return (value_ >= _other.value_); }
            inline bool operator<(const «_enumeration.name» &_other) const { return (value_ < _other.value_); }
            inline bool operator>(const «_enumeration.name» &_other) const { return (value_ > _other.value_); }

            inline bool operator==(const Literal &_value) const { return (value_ == static_cast< «backingType»>(_value)); }
            inline bool operator!=(const Literal &_value) const { return (value_ != static_cast< «backingType»>(_value)); }
            inline bool operator<=(const Literal &_value) const { return (value_ <= static_cast< «backingType»>(_value)); }
            inline bool operator>=(const Literal &_value) const { return (value_ >= static_cast< «backingType»>(_value)); }
            inline bool operator<(const Literal &_value) const { return (value_ < static_cast< «backingType»>(_value)); }
            inline bool operator>(const Literal &_value) const { return (value_ > static_cast< «backingType»>(_value)); }
        };
    '''

    def CharSequence generateEnumBaseTypeConstructor(FEnumerationType _enumeration, String _name, String _baseName, String _backingType) '''
       «IF _enumeration.base != null»
           «generateEnumBaseTypeConstructor(_enumeration.base, _name, _baseName, _backingType)»
           «_name»(const «_enumeration.getBaseType(_enumeration, _backingType)»::Literal &_value)
               : «_baseName»(_value) {}
       «ELSE»
            «_name»(const Literal &_value)
                : «_baseName»(static_cast< «_backingType»>(_value)) {}
            «_name»(const «_backingType» &_value)
                : «_baseName»(_value) {}
       «ENDIF»
    '''

    def CharSequence generateBaseTypeAssignmentOperator(FEnumerationType _enumeration, FEnumerationType _other, PropertyAccessor _accessor) '''
        «IF _other.base != null»
            «val backingType = _enumeration.getBackingType(_accessor).primitiveTypeName»
            «val baseTypeName = _other.getBaseType(_enumeration, backingType)»
            «_enumeration.name» &operator=(const «baseTypeName»::Literal &_value) {
                value_ = static_cast< «backingType»>(_value);
                return (*this);
            }
            «_enumeration.generateBaseTypeAssignmentOperator(_other.base, _accessor)»
        «ENDIF»
    '''

    def dispatch generateFTypeDeclaration(FUnionType fUnionType, PropertyAccessor deploymentAccessor) '''
        «generateComments(fUnionType, false)»
        typedef CommonAPI::Variant< «fUnionType.getElementNames»>  «fUnionType.elementName»;
    '''

    def private String getElementNames(FUnionType fUnion) {
        var names = ""
        if (fUnion.base != null) {
            names += fUnion.base.getElementNames
            if (names != "") {
                names += ", "
            }
        }

        names += fUnion.elements.map[getTypeName(fUnion, false)].join(", ")

        return names
    }

    def dispatch generateFTypeInlineImplementation(FTypeDef fTypeDef, FModelElement parent, PropertyAccessor deploymentAccessor) ''''''
    def dispatch generateFTypeInlineImplementation(FArrayType fArrayType, FModelElement parent, PropertyAccessor deploymentAccessor) ''''''
    def dispatch generateFTypeInlineImplementation(FMapType fMap, FModelElement parent, PropertyAccessor deploymentAccessor) ''''''

    def dispatch generateFTypeInlineImplementation(FStructType fStructType, FModelElement parent, PropertyAccessor deploymentAccessor) '''
    '''

    def dispatch generateFTypeInlineImplementation(FEnumerationType fEnumerationType, FModelElement parent, PropertyAccessor deploymentAccessor) '''
    '''

    def dispatch generateFTypeInlineImplementation(FUnionType fUnionType, FModelElement parent, PropertyAccessor deploymentAccessor) ''''''

    def dispatch generateFTypeImplementation(FTypeDef fTypeDef, FModelElement parent, PropertyAccessor _accessor) ''''''
    def dispatch generateFTypeImplementation(FArrayType fArrayType, FModelElement parent, PropertyAccessor _accessor) ''''''
    def dispatch generateFTypeImplementation(FMapType fMap, FModelElement parent, PropertyAccessor _accessor) ''''''
    def dispatch generateFTypeImplementation(FEnumerationType _enumeration, FModelElement _parent, PropertyAccessor _accessor) '''
    '''

    def dispatch generateFTypeImplementation(FStructType fStructType, FModelElement parent, PropertyAccessor _accessor) '''
        «IF fStructType.isStructEmpty»
        static_assert(false, "struct «fStructType.name» must not be empty !");
        «ENDIF»
        «IF fStructType.polymorphic || (fStructType.hasPolymorphicBase() && fStructType.hasDerivedTypes())»
        std::shared_ptr< «fStructType.getClassNamespace(parent)»> «fStructType.getClassNamespace(parent)»::create(CommonAPI::Serial _serial) {
            switch (_serial) {
            case «parent.elementName»::«fStructType.elementName.toUpperCase()»_SERIAL:
                return std::make_shared< «fStructType.getClassNamespace(parent)»>();
            «FOR derived : fStructType.derivedFStructTypes»
            «derived.generateCases(parent, true)»
                «IF derived.derivedFStructTypes.empty»
                return std::make_shared< «derived.getClassNamespace(parent)»>();
                «ELSE»
                return «derived.getClassNamespace(parent)»::create(_serial);
                «ENDIF»
            «ENDFOR»
            default:
                break;
            }
            return std::shared_ptr< «fStructType.getClassNamespace(parent)»>();
        }
        «ENDIF»

    '''

    def dispatch generateFTypeImplementation(FUnionType fUnionType, FModelElement parent, PropertyAccessor _accessor) '''
    '''

    def hasImplementation(FType fType) {
        if (fType instanceof FStructType) {
            return fType.isPolymorphic
        }
        return false
    }

    def void generateInheritanceIncludes(FInterface fInterface, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        if(fInterface.base != null) {
            generatedHeaders.add(fInterface.base.stubHeaderPath)
        }
    }

    def void generateRequiredTypeIncludes(FInterface fInterface, Collection<String> generatedHeaders, Collection<String> libraryHeaders, boolean isStub) {
        if (!fInterface.attributes.filter[array].nullOrEmpty) {
            libraryHeaders.add('vector')
        }
        if (!fInterface.methods.map[inArgs.filter[array]].nullOrEmpty) {
            libraryHeaders.add('vector')
        }
        if (!fInterface.methods.map[outArgs.filter[array]].nullOrEmpty) {
            libraryHeaders.add('vector')
        }
        if (!fInterface.broadcasts.map[outArgs.filter[array]].nullOrEmpty) {
            libraryHeaders.add('vector')
        }

        fInterface.attributes.forEach[type.derived?.addRequiredHeaders(generatedHeaders, libraryHeaders)]
        fInterface.methods.forEach[
            inArgs.forEach[type.derived?.addRequiredHeaders(generatedHeaders, libraryHeaders)]
            outArgs.forEach[type.derived?.addRequiredHeaders(generatedHeaders, libraryHeaders)]
            errorEnum?.addRequiredHeaders(generatedHeaders, libraryHeaders)
        ]
        if (isStub) {
            fInterface.managedInterfaces.forEach[
                generatedHeaders.add(stubHeaderPath)
            ]
        }
        fInterface.broadcasts.forEach[outArgs.forEach[type.derived?.addRequiredHeaders(generatedHeaders, libraryHeaders)]]

        generatedHeaders.remove(fInterface.headerPath)
    }

    def addRequiredHeaders(FType fType, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        generatedHeaders.add(fType.FTypeCollection.headerPath)
        fType.addFTypeRequiredHeaders(generatedHeaders, libraryHeaders)
    }

    def private getFTypeCollection(FType fType) {
        fType.eContainer as FTypeCollection
    }

    def private dispatch void addFTypeRequiredHeaders(FTypeDef fTypeDef, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        fTypeDef.actualType.getRequiredHeaderPath(generatedHeaders, libraryHeaders)
    }
    def private dispatch void addFTypeRequiredHeaders(FArrayType fArrayType, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        libraryHeaders.add('vector')
        fArrayType.elementType.getRequiredHeaderPath(generatedHeaders, libraryHeaders)
    }
    def private dispatch void addFTypeRequiredHeaders(FMapType fMapType, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        libraryHeaders.add('unordered_map')
        fMapType.keyType.getRequiredHeaderPath(generatedHeaders, libraryHeaders)
        fMapType.valueType.getRequiredHeaderPath(generatedHeaders, libraryHeaders)
    }
    def private dispatch void addFTypeRequiredHeaders(FStructType fStructType, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        if (fStructType.base != null)
            generatedHeaders.add(fStructType.base.FTypeCollection.headerPath)
        else
            libraryHeaders.addAll('CommonAPI/Deployment.hpp', 'CommonAPI/InputStream.hpp', 'CommonAPI/OutputStream.hpp', 'CommonAPI/Struct.hpp')
        if (fStructType.polymorphic || (fStructType.hasPolymorphicBase() && fStructType.hasDerivedTypes()))
            libraryHeaders.add('CommonAPI/Export.hpp')
        fStructType.elements.forEach[type.getRequiredHeaderPath(generatedHeaders, libraryHeaders)]
    }
    def private dispatch void addFTypeRequiredHeaders(FEnumerationType fEnumerationType, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        if (fEnumerationType.base != null)
            generatedHeaders.add(fEnumerationType.base.FTypeCollection.headerPath)
        libraryHeaders.addAll('cstdint', 'CommonAPI/InputStream.hpp', 'CommonAPI/OutputStream.hpp')
    }
    def private dispatch void addFTypeRequiredHeaders(FUnionType fUnionType, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        if (fUnionType.base != null)
            generatedHeaders.add(fUnionType.base.FTypeCollection.headerPath)
        else
            libraryHeaders.add('CommonAPI/Variant.hpp')
        fUnionType.elements.forEach[type.getRequiredHeaderPath(generatedHeaders, libraryHeaders)]
        libraryHeaders.addAll('cstdint', 'memory')
    }

    def private void getRequiredHeaderPath(FTypeRef fTypeRef, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        if (fTypeRef.derived != null) {
            generatedHeaders.add(fTypeRef.derived.FTypeCollection.headerPath)
            }
        fTypeRef.predefined.getRequiredHeaderPath(generatedHeaders, libraryHeaders)
    }

    def private void getRequiredHeaderPath(FBasicTypeId fBasicTypeId, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        switch fBasicTypeId {
            case FBasicTypeId::STRING : libraryHeaders.add('string')
            case FBasicTypeId::BYTE_BUFFER : libraryHeaders.add('CommonAPI/ByteBuffer.hpp')
            default : libraryHeaders.add('cstdint')
        }
    }

    def private getClassNamespace(FModelElement child, FModelElement parent) {
        child.getClassNamespaceWithName(child.elementName, parent, parent.elementName)
    }

    def private getClassNamespaceWithName(FModelElement child, String name, FModelElement parent, String parentName) {
        var reference = name
        if (parent != null && parent != child)
            reference = parentName + '::' + reference
        return reference
    }

    def private generateHasher(FMapType fMap) {
        if (fMap.keyType.derived instanceof FEnumerationType)  {
            return ''', CommonAPI::EnumHasher< «fMap.keyType.derived.getFullName»>'''
        }
        return ""
    }

    def private generateKeyType(FMapType fMap) {
        if (fMap.keyType.polymorphic) {
            return "std::shared_ptr<" + fMap.keyType.getElementType(null, true) + ">"
        }
        if(fMap.keyType.derived != null) {
            return fMap.keyType.derived.getFullName
        } else {
            return fMap.keyType.getElementType(null, true)
        }
    }

    def private generateValueType(FMapType fMap) {
        if (fMap.valueType.polymorphic) {
            return "std::shared_ptr<" + fMap.valueType.getElementType(null, true) + ">"
        }
        if(fMap.valueType.derived != null) {
            return fMap.valueType.derived.getFullName
        } else {
            return fMap.valueType.getElementType(null, true)
        }
    }

    def private getConstReferenceVariable(FField destination, FModelElement source) {
        "const " + destination.getTypeName(source, false) + " &_" + destination.elementName
    }
}
