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
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.core.resources.ResourcesPlugin
import org.eclipse.core.runtime.Path
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
import org.franca.deploymodel.core.FDeployedInterface
import org.franca.deploymodel.core.FDeployedTypeCollection
import org.franca.deploymodel.dsl.fDeploy.FDInterface
import org.franca.deploymodel.dsl.fDeploy.FDModel
import org.franca.deploymodel.dsl.fDeploy.FDProvider
import org.franca.deploymodel.dsl.fDeploy.FDTypes
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.FPreferences
import org.genivi.commonapi.core.preferences.PreferenceConstants

import static com.google.common.base.Preconditions.*

class FrancaGenerator implements IGenerator {
    @Inject private extension FTypeCollectionGenerator
    @Inject private extension FInterfaceGenerator
    @Inject private extension FInterfaceProxyGenerator
    @Inject private extension FInterfaceStubGenerator
    @Inject private extension FrancaGeneratorExtensions

    @Inject private FrancaPersistenceManager francaPersistenceManager
    @Inject private FDeployManager fDeployManager

    override doGenerate(Resource input, IFileSystemAccess fileSystemAccess) {
        var List<FDInterface> deployedInterfaces = new LinkedList<FDInterface>()
        var List<FDTypes> deployedTypeCollections = new LinkedList<FDTypes>()
        var List<FDProvider> deployedProviders = new LinkedList<FDProvider>()
        var IResource res = null
        val String CORE_SPECIFICATION_TYPE = "core.deployment"

        // generate code from the fidl or fdepl file 
        if (input.URI.fileExtension.equals(francaPersistenceManager.fileExtension) ||
            input.URI.fileExtension.equals(FDeployManager.fileExtension)) {

            var model = fDeployManager.loadModel(input.URI, input.URI);
            
            // fModels is the map of all models from imported fidl files
            var fModels = fDeployManager.fidlModels
            
            if (model instanceof FDModel) {
                // read deployment information              
                deployedInterfaces = getFDInterfaces(model, CORE_SPECIFICATION_TYPE)
                deployedTypeCollections = getFDTypesList(model, CORE_SPECIFICATION_TYPE)
                deployedProviders = getFDProviders(model, CORE_SPECIFICATION_TYPE)
            }
            // add the model from the given fidl file
            else if(model instanceof FModel) {
                fModels.put(input.URI.lastSegment, model);
            }

            for (fModelEntry : fModels.entrySet) {
                //System.out.println("Generation code for: " + fModelEntry.key)
                var fModel = fModelEntry.value

                if (fModel != null) {

                    // actually generate code
                    doGenerateComponents(fModel, deployedInterfaces, deployedTypeCollections, deployedProviders,
                        fileSystemAccess, res)
                }
            }
            fDeployManager.clearFidlModels
        } else {
            // input has not  *.fidl or *.fdepl
            checkArgument(false, "Unknown input: " + input)
        }
    }

    def private doGenerateComponents(FModel fModel, List<FDInterface> deployedInterfaces,
        List<FDTypes> deployedTypeCollections, List<FDProvider> deployedProviders,
        IFileSystemAccess fileSystemAccess, IResource res) {

        val allReferencedFTypes = fModel.allReferencedFTypes
        val allFTypeTypeCollections = allReferencedFTypes.filter[eContainer instanceof FTypeCollection].map[
            eContainer as FTypeCollection]
        val allFTypeFInterfaces = allReferencedFTypes.filter[eContainer instanceof FInterface].map[
			eContainer as FInterface]

        val generateTypeCollections = fModel.typeCollections.toSet
        generateTypeCollections.addAll(allFTypeTypeCollections)

        val interfacesToGenerate = fModel.allReferencedFInterfaces.toSet
        interfacesToGenerate.addAll(allFTypeFInterfaces)

        val defaultDeploymentAccessor = new PropertyAccessor() 
        interfacesToGenerate.forEach [
            val currentInterface = it
            var PropertyAccessor deploymentAccessor
            if (deployedInterfaces != null && deployedInterfaces.exists[it.target == currentInterface]) {
                deploymentAccessor = new PropertyAccessor(
                    new FDeployedInterface(deployedInterfaces.filter[it.target == currentInterface].last))
            } else {
                deploymentAccessor = defaultDeploymentAccessor
            }
            generateInterface(it, fileSystemAccess, deploymentAccessor, res)
        ]
        
        generateTypeCollections.forEach [
            val currentTypeCollection = it
            if (!(currentTypeCollection instanceof FInterface)) {
                var PropertyAccessor deploymentAccessor
                if (deployedTypeCollections != null &&
                    deployedTypeCollections.exists[it.target == currentTypeCollection]) {
                    deploymentAccessor = new PropertyAccessor(
                        new FDeployedTypeCollection(
                            deployedTypeCollections.filter[it.target == currentTypeCollection].last))
                } else {
                    deploymentAccessor = defaultDeploymentAccessor
                }
                generate(it, fileSystemAccess, deploymentAccessor, res)
            }
        ]

        fModel.interfaces.forEach [
            val currentInterface = it
            var PropertyAccessor deploymentAccessor
            if (deployedInterfaces.exists[it.target == currentInterface]) {
                deploymentAccessor = new PropertyAccessor(
                    new FDeployedInterface(deployedInterfaces.filter[it.target == currentInterface].last))
            } else {
                deploymentAccessor = defaultDeploymentAccessor
            }
            if (FPreferences::instance.getPreference(PreferenceConstants::P_GENERATEPROXY, "true").equals("true")) {
                it.generateProxy(fileSystemAccess, deploymentAccessor, res)
            }
            if (FPreferences::instance.getPreference(PreferenceConstants::P_GENERATESTUB, "true").equals("true")) {
                it.generateStub(fileSystemAccess, res)
            }
        ]

        return;
    }

    private var String filePrefix = "file://"

    def getFilePathUrl(Resource resource)
    {
        val filePath = resource.filePath
        return filePrefix + filePath
    }

    def private getFilePath(Resource resource)
    {
        if(resource.URI.file)
        {
            return resource.URI.toFileString
        }

        val platformPath = new Path(resource.URI.toPlatformString(true))
        val file = ResourcesPlugin::getWorkspace().getRoot().getFile(platformPath);

        return file.location.toString
    }

    def private getAllReferencedFInterfaces(FModel fModel)
    {
        val referencedFInterfaces = fModel.interfaces.toSet
        fModel.interfaces.forEach[base?.addFInterfaceTree(referencedFInterfaces)]
        fModel.interfaces.forEach[managedInterfaces.forEach[addFInterfaceTree(referencedFInterfaces)]]
        return referencedFInterfaces
    }

    def private void addFInterfaceTree(FInterface fInterface, Collection<FInterface> fInterfaceReferences)
    {
        if(!fInterfaceReferences.contains(fInterface))
        {
            fInterfaceReferences.add(fInterface)
            fInterface.base?.addFInterfaceTree(fInterfaceReferences)
        }
    }

    def private getAllReferencedFTypes(FModel fModel)
    {
        val referencedFTypes = new HashSet<FType>

        fModel.typeCollections.forEach[types.forEach[addFTypeDerivedTree(referencedFTypes)]]

        fModel.interfaces.forEach [
            attributes.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            types.forEach[addFTypeDerivedTree(referencedFTypes)]
            methods.forEach [
                inArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
                outArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            ]
            broadcasts.forEach [
                outArgs.forEach[type.addDerivedFTypeTree(referencedFTypes)]
            ]
        ]

        return referencedFTypes
    }

    def private void addDerivedFTypeTree(FTypeRef fTypeRef, Collection<FType> fTypeReferences)
    {
        fTypeRef.derived?.addFTypeDerivedTree(fTypeReferences)
    }

    def private dispatch void addFTypeDerivedTree(FTypeDef fTypeDef, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fTypeDef))
        {
            fTypeReferences.add(fTypeDef)
            fTypeDef.actualType.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def private dispatch void addFTypeDerivedTree(FArrayType fArrayType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fArrayType))
        {
            fTypeReferences.add(fArrayType)
            fArrayType.elementType.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def private dispatch void addFTypeDerivedTree(FMapType fMapType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fMapType))
        {
            fTypeReferences.add(fMapType)
            fMapType.keyType.addDerivedFTypeTree(fTypeReferences)
            fMapType.valueType.addDerivedFTypeTree(fTypeReferences)
        }
    }

    def private dispatch void addFTypeDerivedTree(FStructType fStructType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fStructType))
        {
            fTypeReferences.add(fStructType)
            fStructType.base?.addFTypeDerivedTree(fTypeReferences)
            fStructType.elements.forEach[type.addDerivedFTypeTree(fTypeReferences)]
        }
    }

    def private dispatch void addFTypeDerivedTree(FEnumerationType fEnumerationType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fEnumerationType))
        {
            fTypeReferences.add(fEnumerationType)
            fEnumerationType.base?.addFTypeDerivedTree(fTypeReferences)
        }
    }

    def private dispatch void addFTypeDerivedTree(FUnionType fUnionType, Collection<FType> fTypeReferences)
    {
        if(!fTypeReferences.contains(fUnionType))
        {
            fTypeReferences.add(fUnionType)
            fUnionType.base?.addFTypeDerivedTree(fTypeReferences)
            fUnionType.elements.forEach[type.addDerivedFTypeTree(fTypeReferences)]
        }
    }

}
