/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.cli;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.generator.IGenerator;
import org.eclipse.xtext.resource.XtextResourceSet;
import org.eclipse.xtext.validation.AbstractValidationMessageAcceptor;
import org.eclipse.xtext.validation.ValidationMessageAcceptor;
import org.franca.core.dsl.FrancaIDLRuntimeModule;
import org.franca.core.franca.FModel;
import org.franca.deploymodel.dsl.fDeploy.FDModel;
import org.genivi.commonapi.console.CommandlineTool;
import org.genivi.commonapi.console.ConsoleLogger;
import org.genivi.commonapi.core.generator.FrancaGenerator;
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions;
import org.genivi.commonapi.core.generator.GeneratorFileSystemAccess;
import org.genivi.commonapi.core.preferences.FPreferences;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.verification.CommandlineValidator;
import org.genivi.commonapi.core.verification.ValidatorCore;

import com.google.inject.Guice;
import com.google.inject.Injector;

/**
 * Receive command line arguments and set them as preference values for the code
 * generation.
 */
public class CommandlineToolMain extends CommandlineTool {

	protected GeneratorFileSystemAccess fsa;
	private FPreferences pref;
	protected Injector injector;
	protected IGenerator francaGenerator;
	protected String scope = "Core validation: ";

	private ValidationMessageAcceptor cliMessageAcceptor = new AbstractValidationMessageAcceptor() {

		@Override
		public void acceptInfo(String message, EObject object,
				EStructuralFeature feature, int index, String code,
				String... issueData) {
			ConsoleLogger.printLog(scope + message);
		}

		@Override
		public void acceptWarning(String message, EObject object,
				EStructuralFeature feature, int index, String code,
				String... issueData) {
			ConsoleLogger.printLog("Warning: " + scope + message);
		}

		@Override
		public void acceptError(String message, EObject object,
				EStructuralFeature feature, int index, String code,
				String... issueData) {
			hasValidationError = true;
			ConsoleLogger.printErrorLog("Error: " + scope + message);
		}
	};

	/**
	 * The constructor registers the needed bindings to use the generator
	 */
	public CommandlineToolMain() {

		injector = Guice.createInjector(new FrancaIDLRuntimeModule());

		fsa = injector.getInstance(GeneratorFileSystemAccess.class);

		pref = FPreferences.getInstance();

	}

	public int generateCore(List<String> fileList) {
		francaGenerator = injector.getInstance(FrancaGenerator.class);

		return doGenerate(fileList);
	}

	protected String normalize(String _path) {
		File itsFile = new File(_path);
		return itsFile.getAbsolutePath();
	}


	/**
	 * Call the franca generator for the specified list of files.
	 *
	 * @param fileList
	 *            the list of files to generate code from
	 */
	protected int doGenerate(List<String> _fileList) {
		fsa.setOutputConfigurations(FPreferences.getInstance()
				.getOutputpathConfiguration());

		XtextResourceSet rsset = injector.getProvider(XtextResourceSet.class)
				.get();

		ConsoleLogger.printLog("Using Franca Version " + getFrancaVersion());
		ConsoleLogger.printLog("and CommonAPI Version "
				+ FrancaGeneratorExtensions.getCoreVersion());
		int error_state = NO_ERROR_STATE;

		// Create absolute paths
		List<String> fileList = new ArrayList<String>();
		for (String path : _fileList) {
			String absolutePath = normalize(path);
			fileList.add(absolutePath);
		}

		for (String file : fileList) {
			URI uri = URI.createFileURI(file);
			Resource resource = null;
			try {
				resource = rsset.createResource(uri);
			} catch (IllegalStateException ise) {
				ConsoleLogger.printErrorLog("Failed to create a resource from "
						+ file + "\n" + ise.getMessage());
				error_state = ERROR_STATE;
				continue;
			}
			hasValidationError = false;
			if (isValidation) {
				validateCore(resource);
			}
			if (hasValidationError) {
				ConsoleLogger.printErrorLog(file
						+ " contains validation errors !");
				error_state = ERROR_STATE;
			} else if (isCodeGeneration) {
				ConsoleLogger.printLog("Generating code for " + file);
				try {
					if (FPreferences.getInstance().getPreference(
							PreferenceConstants.P_OUTPUT_SUBDIRS, "false").equals("true")) {
						String subdir = (new File(file)).getName();
						subdir = subdir.replace(".fidl", "");
						subdir = subdir.replace(".fdepl", "");
						fsa.setOutputConfigurations(FPreferences.getInstance()
								.getOutputpathConfiguration(subdir));
					}
					francaGenerator.doGenerate(resource, fsa);
				} catch (Exception e) {
					System.err.println("Failed to generate code for " + file
							+ " due to " + e.getMessage());
					error_state = ERROR_STATE;
				}
			}
			if (resource != null) {
				// Clear each resource from the resource set in order to let
				// other fidl files import it.
				// Otherwise an IllegalStateException will be thrown for a
				// resource that was already created.
				resource.unload();
				rsset.getResources().clear();
			}
		}
		if (dumpGeneratedFiles) {
			fsa.dumpGeneratedFiles();
		}
		fsa.clearFileList();
		dumpGeneratedFiles = false;
		return error_state;
	}

	/**
	 * Validate the fidl/fdepl file resource
	 *
	 * @param resource
	 */
	private void validateCore(Resource resource) {
		EObject model = null;
		CommandlineValidator cliValidator = new CommandlineValidator(
				cliMessageAcceptor);

		//ConsoleLogger.printLog("validating " + resource.getURI().lastSegment());

		model = cliValidator.loadResource(resource);

		if (model != null) {
			// check existence of imported fidl/fdepl files
			if (model instanceof FModel) {
				cliValidator.validateImports((FModel) model, resource.getURI());

				// validate against GENIVI rules
				ValidatorCore validator = new ValidatorCore();
				try {
					validator.validateModel((FModel) model, cliMessageAcceptor);
				} catch (Exception e) {
					ConsoleLogger.printErrorLog(e.getMessage());
					hasValidationError = true;
					return;
				}
			}
			if (model instanceof FDModel) {
				// don't validate fdepl files at the moment
				return;
				// cliValidator.validateImports((FDModel) model, resource.getURI());
			}
			// XText validation
			cliValidator.validateResourceWithImports(resource);
		} else {
			// model is null, no resource factory was registered !
			hasValidationError = true;
		}
	}

	public void setNoCommonCode() {
		pref.setPreference(PreferenceConstants.P_GENERATE_COMMON, "false");
		ConsoleLogger.printLog("No common code will be generated");
	}

	public void setNoProxyCode() {
		pref.setPreference(PreferenceConstants.P_GENERATE_PROXY, "false");
		ConsoleLogger.printLog("No proxy code will be generated");
	}

	public void setNoStubCode() {
		pref.setPreference(PreferenceConstants.P_GENERATE_STUB, "false");
		ConsoleLogger.printLog("No stub code will be generated");
	}

	public void setDefaultDirectory(String optionValue) {
		ConsoleLogger.printLog("Default output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_DEFAULT, optionValue);
		// In the case where no other output directories are set,
		// this default directory will be used for them
		pref.setPreference(PreferenceConstants.P_OUTPUT_COMMON, optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_PROXIES, optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_STUBS, optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_SKELETON, optionValue);
	}

	public void setDestinationSubdirs() {
		ConsoleLogger.printLog("Using destination subdirs");
		pref.setPreference(PreferenceConstants.P_OUTPUT_SUBDIRS, "true");
	}

	public void setCommonDirectory(String optionValue) {
		ConsoleLogger.printLog("Common output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_COMMON, optionValue);
	}

	public void setProxyDirectory(String optionValue) {
		ConsoleLogger.printLog("Proxy output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_PROXIES, optionValue);
	}

	public void setStubDirectory(String optionValue) {
		ConsoleLogger.printLog("Stub output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_STUBS, optionValue);
	}

	public void setSkeletonDirectory(String optionValue) {
		ConsoleLogger.printLog("Skeleton output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_SKELETON, optionValue);
	}

	public void setLogLevel(String optionValue) {
		if (PreferenceConstants.LOGLEVEL_QUIET.equals(optionValue)) {
			pref.setPreference(PreferenceConstants.P_LOGOUTPUT, "false");
			ConsoleLogger.enableLogging(false);
			ConsoleLogger.enableErrorLogging(false);
		}
		if (PreferenceConstants.LOGLEVEL_VERBOSE.equals(optionValue)) {
			pref.setPreference(PreferenceConstants.P_LOGOUTPUT, "true");
			ConsoleLogger.enableErrorLogging(true);
			ConsoleLogger.enableLogging(true);
		}
	}

	public void setEnumPrefix(String optionValue) {
		pref.setPreference(PreferenceConstants.P_ENUMPREFIX, optionValue);
		ConsoleLogger.printLog("Enum prefix: " + optionValue);
	}

	public void setCreateSkeletonCode() {
		pref.setPreference(PreferenceConstants.P_GENERATE_SKELETON, "true");
		ConsoleLogger.printLog("Skeleton code will be created");
	}

	public void setSkeletonPostfix(String optionValue) {
		String postfix = (optionValue == null ? "Default" : optionValue);
		pref.setPreference(PreferenceConstants.P_SKELETONPOSTFIX, postfix);
		ConsoleLogger.printLog("Skeleton postfix: " + postfix);
	}

	public void disableValidation() {
		ConsoleLogger.printLog("Validation is off");
		isValidation = false;
	}

    public void enableValidationWarningsAsErrors() {
        isValidationWarningsAsErrors = true;
    }

	/**
	 * Set the text from a file which will be inserted as a comment in each
	 * generated file (for example your license)
	 *
	 * @param fileWithText
	 * @return
	 */
	public void setLicenseText(String fileWithText) {

		String licenseText = getLicenseText(fileWithText);

		if (licenseText != null && !licenseText.isEmpty()) {
			pref.setPreference(PreferenceConstants.P_LICENSE, licenseText);
		}
	}

	/**
	 * set a preference value to disable code generation
	 */
	public void disableCodeGeneration() {
		ConsoleLogger.printLog("Code generation is off");
		pref.setPreference(PreferenceConstants.P_GENERATE_CODE, "false");
	}

	/**
	 * Set a preference value to disable code generation for included types and
	 * interfaces
	 */
	public void noCodeforDependencies() {
		ConsoleLogger.printLog("Code generation for dependencies is switched off");
		pref.setPreference(PreferenceConstants.P_GENERATE_DEPENDENCIES, "false");
	}

	public void listGeneratedFiles() {
		dumpGeneratedFiles = true;
	}

	public void disableSyncCalls() {
		ConsoleLogger.printLog("Code generation for synchronous calls is off");
		pref.setPreference(PreferenceConstants.P_GENERATE_SYNC_CALLS, "false");
	}

}
