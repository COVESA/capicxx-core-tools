/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.ui.handler;

import java.util.List;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.emf.common.util.BasicDiagnostic;
import org.eclipse.emf.common.util.Diagnostic;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.xtext.builder.EclipseResourceFileSystemAccess2;
import org.franca.deploymodel.dsl.fDeploy.FDModel;
import org.genivi.commonapi.core.preferences.FPreferences;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;
import org.genivi.commonapi.core.verification.DeploymentValidator;

public class CoreGenerationCommand extends GenerationCommand {

	@Override
	protected void setupPreferences(IFile file) {
		initPreferences(file, CommonApiUiPlugin.getDefault().getPreferenceStore());
	}

	/**
	 * Set the properties for the code generation from the resource properties (set with the property page, via the context menu).
	 * Take default values from the eclipse preference page.
	 * @param file
	 * @param store - the eclipse preference store
	 */
	private void initPreferences(IFile file, IPreferenceStore store) {
		FPreferences instance = FPreferences.getInstance();

		String outputFolderCommon = null;
		String outputFolderProxies = null;
		String outputFolderStubs = null;
		String outputFolderSkeleton = null;
		String licenseHeader = null;
		String generateCommon = null;
		String generateProxy = null;
		String generateStub = null;
		String generateSkeleton = null;
		String generateDependencies = null;
		String skeletonPostfix = null;
		String enumPrefix = null;
		String generateSyncCalls = null;

		IProject project = file.getProject();
		IResource resource = file;

		try {
			// Should project or file specific properties be used ?
			String useProject1 = project.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_USEPROJECTSETTINGS));
			String useProject2 = file.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_USEPROJECTSETTINGS));
			if("true".equals(useProject1) || "true".equals(useProject2)) {
				resource = project;
			}
			outputFolderCommon = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT_COMMON));
			outputFolderSkeleton = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT_SKELETON));
			outputFolderProxies = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT_PROXIES));
			outputFolderStubs = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT_STUBS));
			licenseHeader = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_LICENSE));
			generateCommon = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_COMMON));
			generateProxy = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_PROXY));
			generateStub = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_STUB));
			generateSkeleton = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_SKELETON));
			generateDependencies = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_DEPENDENCIES));
			generateSyncCalls = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_SYNC_CALLS));
			skeletonPostfix = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_SKELETONPOSTFIX));
			enumPrefix = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_ENUMPREFIX));
		} catch (CoreException ce) {
			System.err.println("Failed to get property for " + resource.getName());
		}

		// Set defaults from the preference store in the case, where the value was not specified in the properties.
		if(outputFolderCommon == null) {
			outputFolderCommon = store.getString(PreferenceConstants.P_OUTPUT_COMMON);
		}
		if(outputFolderProxies == null) {
			outputFolderProxies = store.getString(PreferenceConstants.P_OUTPUT_PROXIES);
		}
		if(outputFolderStubs == null) {
			outputFolderStubs = store.getString(PreferenceConstants.P_OUTPUT_STUBS);
		}
		if(outputFolderSkeleton == null) {
			outputFolderSkeleton = store.getString(PreferenceConstants.P_OUTPUT_SKELETON);
		}
		if(skeletonPostfix == null) {
			skeletonPostfix = store.getString(PreferenceConstants.P_SKELETONPOSTFIX);
		}
		if(enumPrefix == null) {
			enumPrefix = store.getString(PreferenceConstants.P_ENUMPREFIX);
		}
		if(licenseHeader == null) {
			licenseHeader = store.getString(PreferenceConstants.P_LICENSE);
		}
		if (generateCommon == null) {
			generateCommon = store.getString(PreferenceConstants.P_GENERATE_COMMON);
		}
		if(generateProxy == null) {
			generateProxy = store.getString(PreferenceConstants.P_GENERATE_PROXY);
		}
		if(generateStub == null) {
			generateStub = store.getString(PreferenceConstants.P_GENERATE_STUB);
		}
		if(generateSkeleton == null) {
			generateSkeleton = store.getString(PreferenceConstants.P_GENERATE_SKELETON);
		}
		if(generateDependencies == null) {
			generateDependencies = store.getString(PreferenceConstants.P_GENERATE_DEPENDENCIES);
		}
		if(generateSyncCalls == null) {
			generateSyncCalls = store.getString(PreferenceConstants.P_GENERATE_SYNC_CALLS);
		}
		// finally, store the properties for the code generator
		instance.setPreference(PreferenceConstants.P_OUTPUT_COMMON, outputFolderCommon);
		instance.setPreference(PreferenceConstants.P_OUTPUT_PROXIES, outputFolderProxies);
		instance.setPreference(PreferenceConstants.P_OUTPUT_STUBS, outputFolderStubs);
		instance.setPreference(PreferenceConstants.P_OUTPUT_SKELETON, outputFolderSkeleton);
		instance.setPreference(PreferenceConstants.P_LICENSE, licenseHeader);
		instance.setPreference(PreferenceConstants.P_GENERATE_COMMON, generateCommon);
		instance.setPreference(PreferenceConstants.P_GENERATE_PROXY, generateProxy);
		instance.setPreference(PreferenceConstants.P_GENERATE_STUB, generateStub);
		instance.setPreference(PreferenceConstants.P_GENERATE_SKELETON, generateSkeleton);
		instance.setPreference(PreferenceConstants.P_SKELETONPOSTFIX, skeletonPostfix);
		instance.setPreference(PreferenceConstants.P_ENUMPREFIX, enumPrefix);
		instance.setPreference(PreferenceConstants.P_GENERATE_DEPENDENCIES, generateDependencies);
		instance.setPreference(PreferenceConstants.P_GENERATE_SYNC_CALLS, generateSyncCalls);
	}

	@Override
	protected void setupOutputDirectories(EclipseResourceFileSystemAccess2 fileSystemAccess) {
		fileSystemAccess.setOutputConfigurations(FPreferences.getInstance().getOutputpathConfiguration());
	}

    public boolean isDeploymentValidatorEnabled()
    {
        IPreferenceStore prefs = CommonApiUiPlugin.getValidatorPreferences();
        return prefs != null && prefs.getBoolean(PreferenceConstants.P_ENABLE_CORE_DEPLOYMENT_VALIDATOR);
    }
    @Override
    protected List<Diagnostic> validateDeployment(List<FDModel> fdepls)
    {
        if (!isDeploymentValidatorEnabled())
            return null;
        BasicDiagnostic diagnostics = new BasicDiagnostic();
        DeploymentValidator coreValidator = new DeploymentValidator();
        coreValidator.validate(fdepls, diagnostics);
        return diagnostics.getChildren();
    }

}

