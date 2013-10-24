/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.Collection
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
import org.franca.core.franca.FAnnotationType
import org.franca.core.franca.FAnnotationBlock
import org.franca.core.franca.FMethod
import java.util.ArrayList
import org.franca.core.franca.FAnnotation

class FTypeGenerator {
    @Inject private extension FrancaGeneratorExtensions francaGeneratorExtensions
    static int begrenzung = 80

    def static isdeprecated(FAnnotationBlock annotations) {
        if(annotations == null)
            return false
        for(annotation : annotations.elements) {
            if(annotation.type.value.equals(FAnnotationType::DEPRECATED_VALUE))
                return true;
        }
        return false
    }

    def static sortAnnotations(FAnnotationBlock annots) {
        var ArrayList<ArrayList<FAnnotation>> ret = new ArrayList<ArrayList<FAnnotation>>(4)
        ret.add(new ArrayList<FAnnotation>())
        ret.add(new ArrayList<FAnnotation>())
        ret.add(new ArrayList<FAnnotation>())
        ret.add(new ArrayList<FAnnotation>())
        for(anno : annots.elements) {
            if(anno == null){
            }else {
                if(anno.type.value.equals(FAnnotationType::DESCRIPTION_VALUE))
                    ret.get(0).add(anno)
                if(anno.type.value.equals(FAnnotationType::PARAM_VALUE))
                    ret.get(1).add(anno)
                if(anno.type.value.equals(FAnnotationType::DEPRECATED_VALUE))
                    ret.get(2).add(anno)
                if(anno.type.value.equals(FAnnotationType::AUTHOR_VALUE))
                    ret.get(3).add(anno)
            }
        }
        return ret
    }
    
    def private static findNextBreak(String text) {
        var breakIndex = text.substring(0, begrenzung).lastIndexOf(" ");
        if (breakIndex > 0) {
            return breakIndex;
        } else {
            breakIndex = text.substring(0, begrenzung).lastIndexOf("\n");
            if (breakIndex > -1) {
                return breakIndex;
            } else {
                return java.lang.Math.min(begrenzung, text.length);
            }
        }
    }

    def static breaktext(String text, int annotation) {
        var ret = ""
        var temptext = ""
        if(annotation == FAnnotationType::DESCRIPTION_VALUE && text.length > begrenzung) {
            ret = " * " + text.substring(0, findNextBreak(text)) + "\n";
            temptext = text.substring(findNextBreak(text));
        }else if(annotation != FAnnotationType::DESCRIPTION_VALUE && text.length > begrenzung) {
            if(annotation == FAnnotationType::AUTHOR_VALUE) {
                ret = " * @author "
            }if(annotation == FAnnotationType::DEPRECATED_VALUE){
                ret = " * @deprecated "
            }if(annotation == FAnnotationType::PARAM_VALUE){
                ret = " * @param "
            }
            ret = ret + text.substring(0, findNextBreak(text)) + "\n";
            temptext = text.substring(findNextBreak(text));
        }else {
            if(annotation == FAnnotationType::AUTHOR_VALUE)
                ret = " * @author "
            if(annotation == FAnnotationType::DEPRECATED_VALUE)
                ret = " * @deprecated "
            if(annotation == FAnnotationType::PARAM_VALUE)
                ret = " * @param "
            if(annotation == FAnnotationType::DESCRIPTION_VALUE)
                ret = " * "
            ret = ret + text + "\n";
        }
        while(temptext.length > begrenzung) {
            try {
                ret = ret + " * " + temptext.substring(0, findNextBreak(temptext)) + "\n";
                temptext = temptext.substring(findNextBreak(temptext));
            }
            catch (StringIndexOutOfBoundsException sie) {
                System.out.println("Comment problem in text " + text);
                sie.printStackTrace();
            }
        }
        if(temptext.length > 0)
            ret = ret + " * " + temptext + "\n"
        return ret;
    }

    def static generateComments(FModelElement model, boolean inline) {
        var typ = getTyp(model)
        var ret = ""
        var commexists = false
        if( model != null && model.comment != null){
            for (list : sortAnnotations(model.comment)){
                for (comment : list){
                    if(comment.type.value.equals(FAnnotationType::DESCRIPTION_VALUE) || 
                        (comment.type.value.equals(FAnnotationType::AUTHOR_VALUE) && typ == ModelTyp::INTERFACE) || 
                        (comment.type.value.equals(FAnnotationType::DEPRECATED_VALUE) && (typ == ModelTyp::METHOD || typ==ModelTyp::ENUM)) || 
                        (comment.type.value.equals(FAnnotationType::PARAM_VALUE) && typ == ModelTyp::METHOD)){
                        if(!inline && !commexists)
                            ret = "/**\n"
                        commexists = true
                        ret = ret + breaktext(comment.comment, comment.type.value)
                    }
                }
            }
            if(!inline && commexists)
                ret = ret + " */"
            if(inline && commexists)
                ret = ret + " * "
        }
        return ret
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

    def generateFTypeDeclarations(FTypeCollection fTypeCollection, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «FOR type: fTypeCollection.types.sortTypes(fTypeCollection)»
            «generateComments(type, false)»
            «type.generateFTypeDeclaration(deploymentAccessor)»
        «ENDFOR»
        «IF fTypeCollection instanceof FInterface»
            «FOR method : (fTypeCollection as FInterface).methods.filter[errors != null]»
                «generateComments(method, false)»
                «method.errors.generateDeclaration(method.errors.errorName, deploymentAccessor)»
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

    def dispatch generateFTypeDeclaration(FTypeDef fTypeDef, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «generateComments(fTypeDef, false)»
        typedef «fTypeDef.actualType.getNameReference(fTypeDef.eContainer)» «fTypeDef.elementName»;
    '''

    def dispatch generateFTypeDeclaration(FArrayType fArrayType, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «generateComments(fArrayType, false)»
        «IF fArrayType.elementType.derived != null && fArrayType.elementType.derived instanceof FStructType && (fArrayType.elementType.derived as FStructType).polymorphic»
            typedef std::vector<std::shared_ptr<«fArrayType.elementType.getNameReference(fArrayType.eContainer)»>> «fArrayType.name»;
        «ELSE»
            typedef std::vector<«fArrayType.elementType.getNameReference(fArrayType.eContainer)»> «fArrayType.elementName»;
        «ENDIF»
    '''

    def dispatch generateFTypeDeclaration(FMapType fMap, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «generateComments(fMap, false)»
        typedef std::unordered_map<«fMap.generateKeyType», «fMap.generateValueType»«fMap.generateHasher»> «fMap.elementName»;
    '''

    def dispatch generateFTypeDeclaration(FStructType fStructType, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «generateComments(fStructType, false)»
        struct «fStructType.elementName»: «fStructType.baseStructName» {
            «FOR element : fStructType.elements»
               «generateComments(element, false)»
                «element.getTypeName(fStructType)» «element.elementName»;
            «ENDFOR»

            «fStructType.elementName»() = default;
            «IF fStructType.allElements.size > 0»
                «fStructType.elementName»(«fStructType.allElements.map[getConstReferenceVariable(fStructType)].join(", ")»);
            «ENDIF»

            «IF fStructType.hasPolymorphicBase»
                enum: uint32_t { SERIAL_ID = 0x«Integer::toHexString(fStructType.serialId)» };

                static «fStructType.elementName»* createInstance(const uint32_t& serialId);

                virtual uint32_t getSerialId() const;
                virtual void createTypeSignature(CommonAPI::TypeOutputStream& typeOutputStream) const;
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
        generateDeclaration(fEnumerationType, fEnumerationType.elementName, deploymentAccessor)
    }

    def generateDeclaration(FEnumerationType fEnumerationType, String name, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        enum class «name»: «fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName» {
            «FOR parent : fEnumerationType.baseList SEPARATOR ',\n'»
                «FOR enumerator : parent.enumerators SEPARATOR ','»
                    «enumerator.elementName» = «parent.getRelativeNameReference(fEnumerationType)»::«enumerator.elementName»
                «ENDFOR»
            «ENDFOR»
            «IF fEnumerationType.base != null && !fEnumerationType.enumerators.empty»,«ENDIF»
            «FOR enumerator : fEnumerationType.enumerators SEPARATOR ','»
                «generateComments(enumerator, false)»
                «enumerator.elementName»«enumerator.generateValue»
            «ENDFOR»
        };

        // Definition of a comparator still is necessary for GCC 4.4.1, topic is fixed since 4.5.1
        struct «name»Comparator;
    '''

    def dispatch generateFTypeDeclaration(FUnionType fUnionType, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «generateComments(fUnionType, false)»
        typedef CommonAPI::Variant<«fUnionType.getElementNames»>  «fUnionType.elementName»;
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
        fEnumerationType.generateInlineImplementation(fEnumerationType.elementName, parent, parent.elementName, deploymentAccessor)
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
            «fStructType.getClassNamespace(parent)»::«fStructType.elementName»(«fStructType.allElements.map[getConstReferenceVariable(fStructType) + "Value"].join(", ")»):
                    «fStructType.base?.generateBaseConstructorCall(fStructType)»
                    «FOR element : fStructType.elements SEPARATOR ','»
                        «element.elementName»(«element.elementName»Value)
                    «ENDFOR»
            {
            }
        «ENDIF»

        «IF fStructType.hasPolymorphicBase»
            «fStructType.getClassNamespace(parent)»* «fStructType.getClassNamespace(parent)»::createInstance(const uint32_t& serialId) {
                if (serialId == SERIAL_ID)
                    return new «fStructType.elementName»;

                «IF fStructType.hasDerivedFStructTypes»
                    const std::function<«fStructType.elementName»*()> createDerivedInstanceFuncs[] = {
                        «FOR derivedFStructType : fStructType.getDerivedFStructTypes SEPARATOR ','»
                            [&]() { return «derivedFStructType.getRelativeNameReference(fStructType)»::createInstance(serialId); }
                        «ENDFOR» 
                    };

                    for (auto& createDerivedInstanceFunc : createDerivedInstanceFuncs) {
                        «fStructType.elementName»* derivedInstance = createDerivedInstanceFunc();
                        if (derivedInstance != NULL)
                            return derivedInstance;
                    }

                «ENDIF»
                return NULL;
            }

            uint32_t «fStructType.getClassNamespace(parent)»::getSerialId() const {
                return SERIAL_ID;
            }

            void «fStructType.getClassNamespace(parent)»::createTypeSignature(CommonAPI::TypeOutputStream& typeOutputStream) const {
                «fStructType.elementName»::writeToTypeOutputStream(typeOutputStream);
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
                        lhs.«element.elementName» == rhs.«element.elementName»
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
                inputStream >> «element.elementName»;
            «ENDFOR»
        }

        void «fStructType.getClassNamespace(parent)»::writeToOutputStream(CommonAPI::OutputStream& outputStream) const {
            «IF fStructType.base != null»
                «fStructType.base.getRelativeNameReference(fStructType)»::writeToOutputStream(outputStream);
            «ENDIF»
            «FOR element : fStructType.elements»
                outputStream << «element.elementName»;
            «ENDFOR»
        }
    '''

    def dispatch generateFTypeImplementation(FUnionType fUnionType, FModelElement parent) ''' '''

    def hasImplementation(FType fType) {
        return (fType instanceof FStructType);
    }

    def void generateRequiredTypeIncludes(FInterface fInterface, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        fInterface.attributes.forEach[type.derived?.addRequiredHeaders(generatedHeaders, libraryHeaders)]
        fInterface.methods.forEach[
            inArgs.forEach[type.derived?.addRequiredHeaders(generatedHeaders, libraryHeaders)]
            outArgs.forEach[type.derived?.addRequiredHeaders(generatedHeaders, libraryHeaders)]
        ]
        fInterface.managedInterfaces.forEach[
            generatedHeaders.add(stubHeaderPath)
        ]
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
            libraryHeaders.addAll('CommonAPI/InputStream.h', 'CommonAPI/OutputStream.h', 'CommonAPI/SerializableStruct.h')
        fStructType.elements.forEach[type.getRequiredHeaderPath(generatedHeaders, libraryHeaders)]
    }
    def private dispatch void addFTypeRequiredHeaders(FEnumerationType fEnumerationType, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        if (fEnumerationType.base != null)
            generatedHeaders.add(fEnumerationType.base.FTypeCollection.headerPath)
        libraryHeaders.addAll('cstdint', 'CommonAPI/InputStream.h', 'CommonAPI/OutputStream.h')
    }
    def private dispatch void addFTypeRequiredHeaders(FUnionType fUnionType, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        if (fUnionType.base != null)
            generatedHeaders.add(fUnionType.base.FTypeCollection.headerPath)
        else
            libraryHeaders.add('CommonAPI/SerializableVariant.h')
        fUnionType.elements.forEach[type.getRequiredHeaderPath(generatedHeaders, libraryHeaders)]
        libraryHeaders.addAll('cstdint', 'memory')
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
    
    def private void getRequiredHeaderPath(FTypeRef fTypeRef, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        if (fTypeRef.derived != null) {
            generatedHeaders.add(fTypeRef.derived.FTypeCollection.headerPath)
            }
        fTypeRef.predefined.getRequiredHeaderPath(generatedHeaders, libraryHeaders)
    }

    def private void getRequiredHeaderPath(FBasicTypeId fBasicTypeId, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        switch fBasicTypeId {
            case FBasicTypeId::STRING : libraryHeaders.add('string')
            case FBasicTypeId::BYTE_BUFFER : libraryHeaders.add('CommonAPI/ByteBuffer.h')
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

    def private generateBaseConstructorCall(FStructType parent, FStructType source) {
        var call = parent.getRelativeNameReference(source)
        call = call + "(" + parent.allElements.map[elementName + "Value"].join(", ") + ")"
        if (!source.elements.empty)
            call = call + ","
        return call
    }

    def private generateHasher(FMapType fMap) {
        if (fMap.keyType.derived instanceof FEnumerationType)  {
            return ''', CommonAPI::EnumHasher<«fMap.keyType.getNameReference(fMap.eContainer)»>'''
        }

        return ""
    }

    def private generateKeyType(FMapType fMap) {
        if (fMap.keyType.polymorphic) {
            return "std::shared_ptr<" + fMap.keyType.getNameReference(fMap.eContainer) + ">"
        }
        else {
            return fMap.keyType.getNameReference(fMap.eContainer)
        }
    }

    def private generateValueType(FMapType fMap) {
        if (fMap.valueType.polymorphic) {
            return "std::shared_ptr<" + fMap.valueType.getNameReference(fMap.eContainer) + ">"
        }
        else {
            return fMap.valueType.getNameReference(fMap.eContainer)
        }
    }

    def private getBaseStructName(FStructType fStructType) {
        if (fStructType.base != null)
            return fStructType.base.getRelativeNameReference(fStructType)

        if (fStructType.hasPolymorphicBase)
            return "CommonAPI::SerializablePolymorphicStruct"

        return "CommonAPI::SerializableStruct"
    }

    def private getConstReferenceVariable(FField destination, FModelElement source) {
        "const " + destination.getTypeName(source) + "& " + destination.elementName
    }

    def private List<FField> getAllElements(FStructType fStructType) {
        if (fStructType.base == null)
            return new LinkedList(fStructType.elements)

        val elements = fStructType.base.allElements
        elements.addAll(fStructType.elements)
        return elements
    }

    def private generateInlineOperatorWithName(FEnumerationType fEnumerationType, String enumerationName, FEnumerationType base, FModelElement parent, String parentName, String operator, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        inline bool operator«operator»(const «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& lhs, const «base.getClassNamespaceWithName(base.elementName, base.eContainer as FModelElement, (base.eContainer as FModelElement).elementName)»& rhs) {
            return static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(lhs) «operator» static_cast<«fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(rhs);
        }
        inline bool operator«operator»(const «base.getClassNamespaceWithName(base.elementName, base.eContainer as FModelElement, (base.eContainer as FModelElement).elementName)»& lhs, const «fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)»& rhs) {
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
