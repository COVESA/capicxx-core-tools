/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.io.File
import java.util.ArrayList
import java.util.HashSet
import java.util.List
import java.util.Map
import java.util.Set
import java.util.HashMap
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.emf.ecore.resource.Resource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.eclipse.xtext.generator.IGenerator
import org.franca.core.dsl.FrancaPersistenceManager
import org.franca.core.franca.FInterface
import org.franca.core.franca.FModel
import org.franca.deploymodel.core.FDeployedInterface
import org.franca.deploymodel.core.FDeployedTypeCollection
import org.franca.deploymodel.dsl.fDeploy.FDInterface
import org.franca.deploymodel.dsl.fDeploy.FDModel
import org.franca.deploymodel.dsl.fDeploy.FDProvider
import org.franca.deploymodel.dsl.fDeploy.FDTypes
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.FPreferences
import org.genivi.commonapi.core.preferences.PreferenceConstants
import org.franca.deploymodel.core.FDeployedProvider
import org.franca.core.franca.FTypeCollection

class FrancaGenerator implements IGenerator {
    @Inject private extension FTypeCollectionGenerator
    @Inject private extension FInterfaceGenerator
    @Inject private extension FInterfaceProxyGenerator
    @Inject private extension FInterfaceStubGenerator
    @Inject private extension FInterfaceDumpGeneratorExtension
    @Inject private extension FInterfacePlaybackGeneratorExtension
    @Inject private extension FrancaGeneratorExtensions

    @Inject private FrancaPersistenceManager francaPersistenceManager
    @Inject private FDeployManager fDeployManager

    val String CORE_SPECIFICATION_TYPE = "core.deployment"

    override doGenerate(Resource input, IFileSystemAccess fileSystemAccess) {
        if (!input.URI.fileExtension.equals(francaPersistenceManager.fileExtension) &&
            !input.URI.fileExtension.equals(FDeployManager.fileExtension)) {
                return
        }

        var List<FDInterface> deployedInterfaces = new ArrayList<FDInterface>()
        var List<FDTypes> deployedTypeCollections = new ArrayList<FDTypes>()
        var List<FDProvider> deployedProviders = new ArrayList<FDProvider>()
        var IResource res = null

        var rootModel = fDeployManager.loadModel(input.URI, input.URI);

        generatedFiles_ = new HashSet<String>()

        withDependencies_ = FPreferences::instance.getPreference(
            PreferenceConstants::P_GENERATE_DEPENDENCIES, "true"
        ).equals("true")

        // models holds the map of all models from imported .fidl files
        var models = fDeployManager.fidlModels
        // deployments holds the map of all models from imported .fdepl files
        var deployments = fDeployManager.deploymentModels

        if (rootModel instanceof FDModel) {
            deployments.put(input.URI.toString, rootModel)
        } else if (rootModel instanceof FModel) {
            models.put(input.URI.toString, rootModel)
        }

        // Categorize deployment information. Store for each deployment whether (or not)
        // it contains an interface or type collection deployment
        for (itsEntry : deployments.entrySet) {
            var FDModel itsDeployment = itsEntry.value

            val List<FDInterface> itsInterfaces = getFDInterfaces(itsDeployment, CORE_SPECIFICATION_TYPE)
            val List<FDTypes> itsTypeCollections = getFDTypesList(itsDeployment, CORE_SPECIFICATION_TYPE)
            val List<FDProvider> itsProviders = getAllFDProviders(itsDeployment)

            deployedInterfaces.addAll(itsInterfaces)
            deployedTypeCollections.addAll(itsTypeCollections)
            deployedProviders.addAll(itsProviders)
        }

        if (rootModel instanceof FDModel) {
            doGenerateDeployment(rootModel, deployments, models,
                deployedInterfaces, deployedTypeCollections, deployedProviders,
                fileSystemAccess, res, true)
        } else if (rootModel instanceof FModel) {
            doGenerateModel(rootModel, models,
                deployedInterfaces, deployedTypeCollections, deployedProviders,
                fileSystemAccess, res)
        }

        fDeployManager.clearFidlModels
        fDeployManager.clearDeploymentModels
    }

    def private void doGenerateDeployment(FDModel _deployment,
                                          Map<String, FDModel> _deployments,
                                          Map<String, FModel> _models,
                                          List<FDInterface> _interfaces,
                                          List<FDTypes> _typeCollections,
                                          List<FDProvider> _providers,
                                          IFileSystemAccess _access,
                                          IResource _res,
                                          boolean _mustGenerate) {
        val String deploymentName
            = _deployments.entrySet.filter[it.value == _deployment].head.key

        var int lastIndex = deploymentName.lastIndexOf(File.separatorChar)
        if (lastIndex == -1) {
            lastIndex = deploymentName.lastIndexOf('/')
        }

        var String basePath = deploymentName.substring(
            0, lastIndex)

        var Set<String> itsImports = new HashSet<String>()
        for (anImport : _deployment.imports) {
            val String canonical = getCanonical(basePath, anImport.importURI)
            itsImports.add(canonical)
        }

        for (itsEntry : _models.entrySet) {
            if (itsImports.contains(itsEntry.key)) {
                doInsertAccessors(itsEntry.value, _interfaces, _typeCollections)
            }
        }

        for (itsEntry : _deployments.entrySet) {
            if (itsImports.contains(itsEntry.key)) {
                doGenerateDeployment(itsEntry.value, _deployments, _models,
                    _interfaces, _typeCollections, _providers,
                    _access, _res, withDependencies_)
            }
        }

        if (_mustGenerate) {
            for (itsEntry : _models.entrySet) {
                if (itsImports.contains(itsEntry.key)) {

                    doGenerateModel(itsEntry.value, _models,
                        _interfaces, _typeCollections, _providers,
                        _access, _res)
                }
            }
        }
    }

    def private void doGenerateModel(FModel _model,
                                     Map<String, FModel> _models,
                                     List<FDInterface> _interfaces,
                                     List<FDTypes> _typeCollections,
                                     List<FDProvider> _providers,
                                     IFileSystemAccess _access,
                                     IResource _res) {

        val String modelName
            = _models.entrySet.filter[it.value == _model].head.key

        if (generatedFiles_.contains(modelName)) {
            return
        }

        generatedFiles_.add(modelName);

        doGenerateComponents(_model,
            _interfaces, _typeCollections, _providers,
            _access, _res)

        if (withDependencies_) {
            for (itsEntry : _models.entrySet) {
                var FModel itsModel = itsEntry.value
                if (itsModel != null) {
                    doGenerateComponents(itsModel,
                        _interfaces, _typeCollections, _providers,
                        _access, _res)
                }
            }
        }
    }

    def private void doInsertAccessors(FModel _model,
                                       List<FDInterface> _deployedInterfaces,
                                       List<FDTypes> _deployedTypeCollections) {
        var typeCollectionsToGenerate = _model.typeCollections.toSet
        var interfacesToGenerate = _model.interfaces.toSet

        val defaultDeploymentAccessor = new PropertyAccessor()
        interfacesToGenerate.forEach [
            val currentInterface = it
            var PropertyAccessor interfaceDeploymentAccessor
            if (_deployedInterfaces != null && _deployedInterfaces.exists[it.target == currentInterface]) {
                interfaceDeploymentAccessor = new PropertyAccessor(
                    new FDeployedInterface(_deployedInterfaces.filter[it.target == currentInterface].last))
            } else {
                interfaceDeploymentAccessor = defaultDeploymentAccessor
            }
            insertAccessor(currentInterface, interfaceDeploymentAccessor)
        ]

        // for all type collections
        typeCollectionsToGenerate.forEach [
            val currentTypeCollection = it
            if (!(currentTypeCollection instanceof FInterface)) {
                var PropertyAccessor typeCollectionDeploymentAccessor
                if (_deployedTypeCollections != null &&
                    _deployedTypeCollections.exists[it.target == currentTypeCollection]) {
                    typeCollectionDeploymentAccessor = new PropertyAccessor(
                        new FDeployedTypeCollection(
                            _deployedTypeCollections.filter[it.target == currentTypeCollection].last))
                } else {
                    typeCollectionDeploymentAccessor = defaultDeploymentAccessor
                }
                insertAccessor(currentTypeCollection, typeCollectionDeploymentAccessor)
            }
        ]
    }

    def private void doGenerateComponents(FModel _model,
                                          List<FDInterface> _deployedInterfaces,
                                          List<FDTypes> _deployedTypeCollections,
                                          List<FDProvider> _deployedProviders,
                                          IFileSystemAccess _fileSystemAccess,
                                          IResource _res) {
        var typeCollectionsToGenerate = _model.typeCollections.toSet
        var interfacesToGenerate = _model.interfaces.toSet

        typeCollectionsToGenerate.forEach[
            val currentTypeCollection = it
            if (!(currentTypeCollection instanceof FInterface)) {
                if (FPreferences::instance.getPreference(PreferenceConstants::P_GENERATE_COMMON, "true").equals("true")) {
                    var currentPropertyAccessor = getAccessor(it)
                    if (currentPropertyAccessor == null) {
                        currentPropertyAccessor = new PropertyAccessor()
                    }
                    generate(it, _fileSystemAccess, currentPropertyAccessor, _res)
                }
            }
        ]

        // for all interfaces
        interfacesToGenerate.forEach [
            val currentInterface = it
            var PropertyAccessor deploymentAccessor
            if (_deployedInterfaces.exists[it.target == currentInterface]) {
                deploymentAccessor = new PropertyAccessor(
                    new FDeployedInterface(_deployedInterfaces.filter[it.target == currentInterface].last))
            } else {
                deploymentAccessor = new PropertyAccessor()
            }
            if (FPreferences::instance.getPreference(PreferenceConstants::P_GENERATE_PROXY, "true").equals("true")) {
                it.generateProxy(_fileSystemAccess, deploymentAccessor, _res)
                it.generateDumper(_fileSystemAccess, deploymentAccessor, _res)
                it.generatePlayback(_fileSystemAccess, deploymentAccessor, _res)
            }
            if (FPreferences::instance.getPreference(PreferenceConstants::P_GENERATE_STUB, "true").equals("true")) {
                it.generateStub(_fileSystemAccess, deploymentAccessor, _res)
            }
            if (FPreferences::instance.getPreference(PreferenceConstants::P_GENERATE_COMMON, "true").equals("true")) {
                generateInterface(it, _fileSystemAccess, deploymentAccessor, _res)
            }
        ]
        // generate interface instance header
        interfacesToGenerate.forEach [
            val currentInterface = it
            val List<String> deployedInstances = new ArrayList<String>()
            _deployedProviders.forEach [
                val deploymentAccessor = new PropertyAccessor(new FDeployedProvider(it))
                it.instances.filter[currentInterface == target].forEach [
                    var instanceId = deploymentAccessor.getInstanceId(it)
                    if (instanceId != null) {
                        deployedInstances.add(instanceId)
                    }
                ]
            ]
            if (!deployedInstances.isEmpty) {
                currentInterface.generateInstanceIds(_fileSystemAccess, deployedInstances)
            }
        ]
    }

    private static Map<FTypeCollection, PropertyAccessor> accessors__ = new HashMap<FTypeCollection, PropertyAccessor>()

    def insertAccessor(FTypeCollection _tc, PropertyAccessor _pa) {
        accessors__.put(_tc, _pa)
    }

    def PropertyAccessor getAccessor(FTypeCollection _tc) {
        return accessors__.get(_tc)
    }

    private boolean withDependencies_ = false
    private Set<String> generatedFiles_;
}
