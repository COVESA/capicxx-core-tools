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
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.FUnionType
import org.genivi.commonapi.core.deployment.PropertyAccessor

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

    def private List<String> getNamespaceAsList(FModel fModel) {
        newArrayList(fModel.name.split("\\."))
    }

    def private FModel getModel(FModelElement fModelElement) {
        if (fModelElement.eContainer instanceof FModel)
            return (fModelElement.eContainer as FModel)
        return (fModelElement.eContainer as FModelElement).model
    }

    def generateVariantComparators(FTypeCollection fTypes) '''
        «FOR type: fTypes.types»
            «IF type.isFUnionType»
              «FOR base : type.getFUnionType.baseList»
                    «generateComparatorImplementation(type.getFUnionType, base)»
                «ENDFOR»
            «ENDIF»
        «ENDFOR»
    '''

    def private getClassNamespaceWithName(FTypedElement type, String name, FModelElement parent, String parentName) {
        var reference = type.getTypeName(parent, false)
        if (parent != null && parent != type) {
            reference = parentName + '::' + reference
        }
        return reference
    }



    def private List<String> getElementTypeNames(FUnionType fUnion) {
        var names = new ArrayList<String>
        var rev = fUnion.elements
        var iter = rev.iterator
        val parent = (fUnion.eContainer as FTypeCollection)

        while (iter.hasNext) {
            var item = iter.next
            var lName = "";
            if (item.type.derived != null) {
                lName = parent.versionPrefix + parent.model.namespaceAsList.join("::") + "::" + item.getTypeName(fUnion, true)
            } else {
               lName = item.getTypeName(fUnion, false)
            }
            names.add(lName)
        }

        if (fUnion.base != null) {
            for (base : fUnion.base.elementTypeNames) {
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
            ret = ret + "if (_rhs.getValueType() == " + counter + ") { \n" +
            "    " + item + " a = _lhs.get<" + item + ">(); \n" +
            "    " + item + " b = _rhs.get<" + item + ">(); \n" +
            "    " + "return (a == b); \n" +
            "}"
            counter = counter + 1;
        }
        return ret
    }

    def private generateComparatorImplementation(FUnionType _derived, FUnionType _base) '''
        «val unionTypeName = _derived.getElementName(null, true)»
        «val unionBaseTypeName = _base.getElementName(null, true)»
        «val unionTypeContainerName = (_derived.eContainer as FTypeCollection).getTypeCollectionName(null)»
        «val unionBaseTypeContainerName = (_base.eContainer as FTypeCollection).getTypeCollectionName(null)»

        inline bool operator==(
                const «unionTypeContainerName»::«unionTypeName» &_lhs,
                const «unionBaseTypeContainerName»::«unionBaseTypeName» &_rhs) {
            if (_lhs.getValueType() == _rhs.getValueType()) {
                «var list = _base.elementTypeNames»
                «list.generateVariantComnparatorIf»
            }
            return false;
        }

        inline bool operator==(
                const «unionBaseTypeContainerName»::«unionBaseTypeName» &_lhs,
                const «unionTypeContainerName»::«unionTypeName» &_rhs) {
            return _rhs == _lhs;
        }

        inline bool operator!=(
                const «unionTypeContainerName»::«unionTypeName» &_lhs,
                const «unionBaseTypeContainerName»::«unionBaseTypeName» &_rhs) {
            return !(_lhs == _rhs);
        }

        inline bool operator!=(
                const «unionBaseTypeContainerName»::«unionBaseTypeName» &_lhs,
                const «unionTypeContainerName»::«unionTypeName» &_rhs) {
            return !(_rhs == _lhs);
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

    def getFQN(FType type, FTypeCollection fTypes) '''«fTypes.versionPrefix»«fTypes.model.namespaceAsList.join("::")»::«type.getClassNamespaceWithName(type.elementName, fTypes, fTypes.elementName)»'''

    def getFQN(FType type, String name, FTypeCollection fTypes) '''«fTypes.versionPrefix»«fTypes.model.namespaceAsList.join("::")»::«type.getClassNamespaceWithName(name, fTypes, fTypes.elementName)»'''

    def generateHash (FType type, String name, FTypeCollection fTypes, PropertyAccessor deploymentAccessor) '''
    //Hash for «name»
    template<>
    struct hash< «type.getFQN(name, fTypes)»> {
        inline size_t operator()(const «type.getFQN(name, fTypes)»& «name.toFirstLower») const {
            return static_cast< «type.getFEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(«name.toFirstLower»);
        }
    };
    '''

    def generateHash (FType type, FTypeCollection fTypes, PropertyAccessor deploymentAccessor) '''
    //Hash for «type.elementName»
    template<>
    struct hash< «type.getFQN(fTypes)»> {
        inline size_t operator()(const «type.getFQN(fTypes)»& «type.elementName.toFirstLower») const {
            return static_cast< «type.getFEnumerationType.getBackingType(deploymentAccessor).primitiveTypeName»>(«type.elementName.toFirstLower»);
        }
    };
    '''

    def generateHashers(FTypeCollection fTypes, PropertyAccessor deploymentAccessor) '''
        «FOR type: fTypes.types»
            «IF type.isFEnumerationType»
                «type.generateHash(fTypes, deploymentAccessor)»
            «ENDIF»
        «ENDFOR»
    '''
}
