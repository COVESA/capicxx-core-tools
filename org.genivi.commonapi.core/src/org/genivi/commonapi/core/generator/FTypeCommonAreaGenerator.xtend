/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.ArrayList
import java.util.LinkedList
import java.util.List
import javax.inject.Inject
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FModel
import org.franca.core.franca.FModelElement
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FUnionType
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor
import org.franca.core.franca.FTypedElement

class FTypeCommonAreaGenerator {
    @Inject private extension FrancaGeneratorExtensions

    def private getClassNamespaceWithName(FModelElement child, String name, FModelElement parent, String parentName) {
        var reference = name
        if (parent != null && parent != child)
            reference = parentName + '::' + reference
        return reference
    }

    def private isFEnumerationType(FType fType) {
        return fType instanceof FEnumerationType
    }

    def private getFEnumerationType(FType fType) {
        return fType as FEnumerationType
    }

    def private isFUnionType(FType fType) {
        return fType instanceof FUnionType
    }

    def private getFUnionType(FType fType) {
        return fType as FUnionType
    }

    def generateTypeWriters(FTypeCollection fTypes, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «FOR type: fTypes.types»
            «IF type.isFEnumerationType»
                «generateStreamImplementation(type.getFEnumerationType, type.name, fTypes, fTypes.name, deploymentAccessor)»
            «ENDIF»
        «ENDFOR»
    '''

    def private List<String> getNamespaceAsList(FModel fModel) {
        newArrayList(fModel.name.split("\\."))
    }

    def private FModel getModel(FModelElement fModelElement) {
        if (fModelElement.eContainer instanceof FModel)
            return (fModelElement.eContainer as FModel)
        return (fModelElement.eContainer as FModelElement).model
    }

    def private getWithNamespace(FEnumerationType fEnumerationType, String enumerationName, FModelElement parent, String parentName) {
        parent.model.namespaceAsList.join("::") + "::" + fEnumerationType.getClassNamespaceWithName(enumerationName, parent, parentName)
    }

    def private generateStreamImplementation(FEnumerationType fEnumerationType, String enumerationName, FModelElement parent, String parentName, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        template<>
        struct BasicTypeWriter<«fEnumerationType.getWithNamespace(enumerationName, parent, parentName)»> {
            inline static void writeType (CommonAPI::TypeOutputStream& typeStream) {
                typeStream.write«fEnumerationType.generateTypeOutput(deploymentAccessor)»Type();
            }
        };

        template<>
        struct InputStreamVectorHelper<«fEnumerationType.getWithNamespace(enumerationName, parent, parentName)»> {
            static void beginReadVector(InputStream& inputStream, const std::vector<«fEnumerationType.getWithNamespace(enumerationName, parent, parentName)»>& vectorValue) {
                inputStream.beginRead«fEnumerationType.generateTypeOutput(deploymentAccessor)»Vector();
            }
        };

        template <>
        struct OutputStreamVectorHelper<«fEnumerationType.getWithNamespace(enumerationName, parent, parentName)»> {
            static void beginWriteVector(OutputStream& outputStream, const std::vector<«fEnumerationType.getWithNamespace(enumerationName, parent, parentName)»>& vectorValue) {
                outputStream.beginWrite«fEnumerationType.generateTypeOutput(deploymentAccessor)»Vector(vectorValue.size());
            }
        };
    '''

    def private generateTypeOutput(FEnumerationType fEnumerationType, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        return fEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName.split("_").get(0).toUpperCase.replace("NT", "nt") + "Enum"
    }

    def generateVariantComparators(FTypeCollection fTypes) '''
        «FOR type: fTypes.types»
            «IF type.isFUnionType»
              «FOR base : type.getFUnionType.baseList»
                    «generateComparatorImplementation(type.getFUnionType, type.name, base, fTypes, fTypes.name)»
                «ENDFOR»
            «ENDIF»
        «ENDFOR»
    '''

    def private getClassNamespaceWithName(FTypedElement type, String name, FModelElement parent, String parentName) {
        var reference = type.getTypeName(parent)
        if (parent != null && parent != type) {
            reference = parentName + '::' + reference
        }
        return reference
    }



    def private List<String> getElementTypeNames(FUnionType fUnion, String unionName, FModelElement parent, String parentName) {
        var names = new ArrayList<String>
        var rev = fUnion.elements
        var iter = rev.iterator
        while (iter.hasNext) {
            var item = iter.next
            var lName = "";
            if (item.type.derived != null) {
                lName = parent.model.namespaceAsList.join("::") + "::" + item.getClassNamespaceWithName(item.name, parent, parentName)
            } else {
               lName = item.getTypeName(fUnion)
            }            
            names.add(lName)
        }

        if (fUnion.base != null) {
            for (base : fUnion.base.getElementTypeNames(fUnion.base.name, parent, parentName)) {
                names.add(base);
            }
        }

        return names.reverse
    }
    
    def private generateVariantComnparatorIf(List<String> list) {
        var counter = 1;
        var ret = "";
        for (item : list) {
            if (counter > 1) {
                ret = ret + " else ";
            }
            ret = ret + "if (rhs.getValueType() == " + counter + ") { \n" +
            "    " + item + " a = lhs.get<" + item + ">(); \n" +
            "    " + item + " b = rhs.get<" + item + ">(); \n" +
            "    " + "return (a == b); \n" +
            "}"
            counter = counter + 1;
        }
        return ret
    }

    def private generateComparatorImplementation(FUnionType fUnionType, String unionName, FUnionType base, FModelElement parent, String parentName) '''
        inline bool operator==(
                const «parent.model.namespaceAsList.join("::") + "::" + fUnionType.getClassNamespaceWithName(unionName, parent, parentName)»& lhs,
                const «parent.model.namespaceAsList.join("::") + "::" + base.getClassNamespaceWithName(base.name, parent, parentName)»& rhs) {
            if (lhs.getValueType() == rhs.getValueType()) {
                «var list = base.getElementTypeNames(unionName, parent, parentName)»
                «list.generateVariantComnparatorIf»
            }
            return false;
        }

        inline bool operator==(
                const «parent.model.namespaceAsList.join("::") + "::" + base.getClassNamespaceWithName(base.name, parent, parentName)»& lhs,
                const «parent.model.namespaceAsList.join("::") + "::" + fUnionType.getClassNamespaceWithName(unionName, parent, parentName)»& rhs) {
            return rhs == lhs;
        }

        inline bool operator!=(
                const «parent.model.namespaceAsList.join("::") + "::" + fUnionType.getClassNamespaceWithName(unionName, parent, parentName)»& lhs,
                const «parent.model.namespaceAsList.join("::") + "::" + base.getClassNamespaceWithName(base.name, parent, parentName)»& rhs) {
            return lhs != rhs;
        }

        inline bool operator!=(
                const «parent.model.namespaceAsList.join("::") + "::" + base.getClassNamespaceWithName(base.name, parent, parentName)»& lhs,
                const «parent.model.namespaceAsList.join("::") + "::" + fUnionType.getClassNamespaceWithName(unionName, parent, parentName)»& rhs) {
            return lhs != rhs;
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

    def getFQN(FType type, FTypeCollection fTypes) '''«fTypes.model.namespaceAsList.join("::")»::«type.getClassNamespaceWithName(type.name, fTypes, fTypes.name)»'''

    def getFQN(FType type, String name, FTypeCollection fTypes) '''«fTypes.model.namespaceAsList.join("::")»::«type.getClassNamespaceWithName(name, fTypes, fTypes.name)»'''

    def generateHash (FType type, String name, FTypeCollection fTypes, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
    //Hash for «name»
    template<>
    struct hash<«type.getFQN(name, fTypes)»> {
        inline size_t operator()(const «type.getFQN(name, fTypes)»& «name.toFirstLower») const {
            return static_cast<«type.getFEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(«name.toFirstLower»);
        }
    };
    '''

    def generateHash (FType type, FTypeCollection fTypes, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
    //Hash for «type.name»
    template<>
    struct hash<«type.getFQN(fTypes)»> {
        inline size_t operator()(const «type.getFQN(fTypes)»& «type.name.toFirstLower») const {
            return static_cast<«type.getFEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(«type.name.toFirstLower»);
        }
    };
    '''

    def generateHashers(FTypeCollection fTypes, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «FOR type: fTypes.types»
            «IF type.isFEnumerationType»
                «type.generateHash(fTypes, deploymentAccessor)»
            «ENDIF»
        «ENDFOR»
    '''
}