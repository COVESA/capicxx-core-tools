/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.Collection
import java.util.HashSet
import javax.inject.Inject
import org.eclipse.core.runtime.Path
import org.eclipse.emf.ecore.plugin.EcorePlugin
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.eclipse.xtext.generator.IGenerator
import org.franca.core.dsl.FrancaIDLHelpers
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMapType
import org.franca.core.franca.FModel
import org.franca.core.franca.FStructType
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeCollection
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FUnionType

import static com.google.common.base.Preconditions.*

class FrancaGenerator implements IGenerator {
    @Inject private extension FTypeCollectionGenerator
    @Inject private extension FInterfaceGenerator
    @Inject private extension FInterfaceProxyGenerator
    @Inject private extension FInterfaceStubGenerator

	override doGenerate(Resource input, IFileSystemAccess fileSystemAccess) {
	    val isFrancaIDLResource = input.URI.fileExtension.equals(FrancaIDLHelpers::instance.fileExtension)
        checkArgument(isFrancaIDLResource, "Unknown input: " + input)

        val fModel = FrancaIDLHelpers::instance.loadModel(input.filePath)
        val allReferencedFTypes = fModel.allReferencedFTypes
        val allFTypeFInterfaces = allReferencedFTypes.filter[eContainer instanceof FInterface].map[eContainer as FInterface]
        
        val generateTypeCollections = new HashSet<FTypeCollection>
        generateTypeCollections.addAll(fModel.typeCollections)
        allReferencedFTypes.filter[eContainer instanceof FTypeCollection].forEach[generateTypeCollections.add(eContainer as FTypeCollection)]

        val generateInterfaces = fModel.allReferencedFInterfaces
        generateInterfaces.addAll(allFTypeFInterfaces)

        val generateInterfaceProxies = fModel.interfaces
        
        val generateInterfaceStubs = fModel.interfaces

        generateTypeCollections.forEach[generate(fileSystemAccess)]
        generateInterfaces.forEach[generate(fileSystemAccess)]
        generateInterfaceProxies.forEach[generateProxy(fileSystemAccess)]
        generateInterfaceStubs.forEach[generateStub(fileSystemAccess)]
	}

	def private getFilePath(Resource resource) {
        if (resource.URI.file)
            return resource.URI.toFileString
        
        val platformPath = new Path(resource.URI.toPlatformString(true))
        val file = EcorePlugin::workspaceRoot.getFile(platformPath)

        return file.location.toString
    }

	def private getAllReferencedFInterfaces(FModel fModel) {
	    val referencedFInterfaces = fModel.interfaces.toSet
	    fModel.interfaces.forEach[base?.addFInterfaceTree(referencedFInterfaces)]
	    return referencedFInterfaces
	}

	def private addFInterfaceTree(FInterface fInterface, Collection<FInterface> fInterfaceReferences) {
	    if (!fInterfaceReferences.contains(fInterface)) {
	        fInterfaceReferences.add(fInterface)
	        fInterface.base?.addFInterfaceTree(fInterfaceReferences)
	    }
	}
	
	def private getAllReferencedFTypes(FModel fModel) {
        val referencedFTypes = new HashSet<FType>

        fModel.typeCollections.forEach[referencedFTypes.addAll(types)]

        fModel.interfaces.forEach[
            attributes.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            types.forEach[addFTypeDerivedTree(referencedFTypes)]
            methods.forEach[
                inArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
                outArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            ]
            broadcasts.forEach[
                outArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            ]
        ]

        return referencedFTypes
    }

    def private void addDerivedFTypeTree(FTypeRef fTypeRef, Collection<FType> fTypeReferences) {
        fTypeRef.derived?.addFTypeDerivedTree(fTypeReferences)
    }

    def private dispatch void addFTypeDerivedTree(FTypeDef fTypeDef, Collection<FType> fTypeReferences) {
        if (!fTypeReferences.contains(fTypeDef)) {
            fTypeReferences.add(fTypeDef)
            fTypeDef.actualType.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def private dispatch void addFTypeDerivedTree(FArrayType fArrayType, Collection<FType> fTypeReferences) {
        if (!fTypeReferences.contains(fArrayType)) {
            fTypeReferences.add(fArrayType)
            fArrayType.elementType.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def private dispatch void addFTypeDerivedTree(FMapType fMapType, Collection<FType> fTypeReferences) {
        if (!fTypeReferences.contains(fMapType)) {
            fTypeReferences.add(fMapType)
            fMapType.keyType.addDerivedFTypeTree(fTypeReferences)
            fMapType.valueType.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def private dispatch void addFTypeDerivedTree(FStructType fStructType, Collection<FType> fTypeReferences) {
        if (!fTypeReferences.contains(fStructType)) {
            fTypeReferences.add(fStructType)
            fStructType.base?.addFTypeDerivedTree(fTypeReferences)
            fStructType.elements.forEach[type.addDerivedFTypeTree(fTypeReferences)]
        }
    }

    def private dispatch void addFTypeDerivedTree(FEnumerationType fEnumerationType, Collection<FType> fTypeReferences) {
        if (!fTypeReferences.contains(fEnumerationType)) {
            fTypeReferences.add(fEnumerationType)
            fEnumerationType.base?.addFTypeDerivedTree(fTypeReferences)
        }
    }

    def private dispatch void addFTypeDerivedTree(FUnionType fUnionType, Collection<FType> fTypeReferences) {
        if (!fTypeReferences.contains(fUnionType)) {
            fTypeReferences.add(fUnionType)
            fUnionType.base?.addFTypeDerivedTree(fTypeReferences)
            fUnionType.elements.forEach[type.addDerivedFTypeTree(fTypeReferences)]
        }
    }
}