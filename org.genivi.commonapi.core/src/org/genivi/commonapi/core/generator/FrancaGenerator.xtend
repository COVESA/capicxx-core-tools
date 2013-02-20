/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
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
import org.franca.core.dsl.FrancaPersistenceManager
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
import org.franca.deploymodel.dsl.FDeployPersistenceManager
import org.franca.deploymodel.core.FDModelExtender
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessorWrapper
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor

import static com.google.common.base.Preconditions.*
import org.franca.deploymodel.core.FDeployedInterface
import org.franca.deploymodel.dsl.fDeploy.FDInterface
import java.util.List
import java.util.LinkedList

class FrancaGenerator implements IGenerator {
    @Inject private extension FTypeCollectionGenerator
    @Inject private extension FInterfaceGenerator
    @Inject private extension FInterfaceProxyGenerator
    @Inject private extension FInterfaceStubGenerator
    @Inject private extension FrancaGeneratorExtensions

    @Inject private FrancaPersistenceManager francaPersistenceManager
    @Inject private FDeployPersistenceManager fDeployPersistenceManager


    override doGenerate(Resource input, IFileSystemAccess fileSystemAccess) {
        var FModel fModel
        var List<FDInterface> deployedInterfaces

        if(input.URI.fileExtension.equals(francaPersistenceManager.fileExtension)) {
            fModel = francaPersistenceManager.loadModel(input.filePath)
            deployedInterfaces = new LinkedList<FDInterface>()

        } else if (input.URI.fileExtension.equals("fdepl" /* fDeployPersistenceManager.fileExtension */)) {
            var fDeployedModel = fDeployPersistenceManager.loadModel(input.filePathUrl);
            val fModelExtender = new FDModelExtender(fDeployedModel);

            checkArgument(fModelExtender.getFDInterfaces().size > 0, "No Interfaces were deployed, nothing to generate.")
            fModel = fModelExtender.getFDInterfaces().get(0).target.model
            deployedInterfaces = fModelExtender.getFDInterfaces()

        } else {
            checkArgument(false, "Unknown input: " + input)
        }

        doGenerateComponents(fModel, deployedInterfaces, fileSystemAccess)

//        mainStubGenerator.generate(basicModel.interfaces, null, fileSystemAccess, outputLocation, skeletonFolderPath)
//        automakeGenerator.generate(basicModel.interfaces, null, fileSystemAccess, outputLocation, skeletonFolderPath)
//        initTypeGeneration
//        for (fInterface: basicModel.interfaces) {
//            generateInterface(fInterface, deploymentAccessor, fileSystemAccess)
//        }
//        finalizeTypeGeneration(fileSystemAccess, outputLocation)
    }
    

    def private doGenerateComponents(FModel fModel, List<FDInterface> deployedInterfaces, IFileSystemAccess access) {
        val allReferencedFTypes = fModel.allReferencedFTypes
        val allFTypeTypeCollections = allReferencedFTypes.filter[eContainer instanceof FTypeCollection].map[eContainer as FTypeCollection]
        val allFTypeFInterfaces = allReferencedFTypes.filter[eContainer instanceof FInterface].map[eContainer as FInterface]

        val generateTypeCollections = fModel.typeCollections.toSet
        generateTypeCollections.addAll(allFTypeTypeCollections)

        val generateInterfaces = fModel.allReferencedFInterfaces.toSet
        generateInterfaces.addAll(allFTypeFInterfaces)

        val defaultDeploymentAccessor = new DeploymentInterfacePropertyAccessorWrapper(null) as DeploymentInterfacePropertyAccessor

        generateTypeCollections.forEach[generate(it, access, defaultDeploymentAccessor)]

        generateInterfaces.forEach[
            val currentInterface = it
            var DeploymentInterfacePropertyAccessor deploymentAccessor
            if(deployedInterfaces.exists[it.target == currentInterface]) {
                deploymentAccessor = new DeploymentInterfacePropertyAccessor(new FDeployedInterface(deployedInterfaces.filter[it.target == currentInterface].last))
            } else {
                deploymentAccessor = defaultDeploymentAccessor
            }
            generate(it, access, deploymentAccessor)
        ]

        fModel.interfaces.forEach[
            val currentInterface = it
            var DeploymentInterfacePropertyAccessor deploymentAccessor
            if(deployedInterfaces.exists[it.target == currentInterface]) {
                deploymentAccessor = new DeploymentInterfacePropertyAccessor(new FDeployedInterface(deployedInterfaces.filter[it.target == currentInterface].last))
            } else {
                deploymentAccessor = defaultDeploymentAccessor
            }
            it.generateProxy(access, deploymentAccessor)
            it.generateStub(access)
        ]

        return;
    }


    private var String filePrefix = "file://"
    def getFilePathUrl(Resource resource) {
        val filePath = resource.filePath
        return filePrefix + filePath
    }


	def private getFilePath(Resource resource) {
        if (resource.URI.file) {
            return resource.URI.toFileString
        }
        
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