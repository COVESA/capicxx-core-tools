/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.cli;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.generator.IGenerator;
import org.eclipse.xtext.generator.JavaIoFileSystemAccess;
import org.eclipse.xtext.resource.XtextResourceSet;
import org.eclipse.xtext.validation.AbstractValidationMessageAcceptor;
import org.eclipse.xtext.validation.ValidationMessageAcceptor;
import org.franca.core.dsl.FrancaIDLRuntimeModule;
import org.franca.core.franca.FModel;
import org.genivi.commonapi.console.ConsoleLogger;
import org.genivi.commonapi.core.generator.FrancaGenerator;
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions;
import org.genivi.commonapi.core.preferences.FPreferences;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.verification.ValidatorCore;

import com.google.inject.Guice;
import com.google.inject.Injector;

/**
 * Receive command line arguments and set them as preference values for the code generation.
 */
public class CommandlineToolMain
{

	public static final String       FILESEPARATOR    = System.getProperty("file.separator");

	public static List<String>       files            = new ArrayList<String>();
	// All given files were saved in this list with an absolute path
	protected static Set<String>     filelist         = new LinkedHashSet<String>();
	// true if for all interfaces have to be generated the stubs
	protected static boolean         allstubs         = false;

	protected JavaIoFileSystemAccess fsa;
	private FPreferences             pref;
	protected Injector               injector;
	private List<String>             tempfilelist;

	protected IGenerator             francaGenerator;

	protected ValidatorCore		 	validator;
	private int validationErrorCount;
	protected String SCOPE = "Core validation: ";
	private boolean isValidation = true;
	public static final int ERROR_STATE = 1;
	
	private ValidationMessageAcceptor cliMessageAcceptor = new AbstractValidationMessageAcceptor() {

		@Override
		public void acceptInfo(String message, EObject object, EStructuralFeature feature, int index, String code, String... issueData) {
			ConsoleLogger.printLog(SCOPE + message);
		}

		@Override
		public void acceptWarning(String message, EObject object, EStructuralFeature feature, int index, String code, String... issueData) {
			ConsoleLogger.printLog("Warning: " + SCOPE + message);
		}

		@Override
		public void acceptError(String message, EObject object, EStructuralFeature feature, int index, String code, String... issueData) {
			validationErrorCount++;
			ConsoleLogger.printLog("Error: " + SCOPE + message);
		}
	};

	/**
	 * The constructor registers the needed bindings to use the generator
	 */
	public CommandlineToolMain()
	{
		injector = Guice.createInjector(new FrancaIDLRuntimeModule());

		fsa = injector.getInstance(JavaIoFileSystemAccess.class);

		pref = FPreferences.getInstance();

		validator = new ValidatorCore();

	}

	/**
	 * Set the text from a file which will be inserted as a comment in each generated file (for example your license)
	 * 
	 * @param fileWithText
	 * @return
	 */
	public void setLicenseText(String fileWithText) {
		if (fileWithText != null && !fileWithText.isEmpty())
		{
			File file = new File(createAbsolutPath(fileWithText));
			if (!file.exists() || file.isDirectory())
			{
				ConsoleLogger.printErrorLog("Please specify a path to an existing file after option -l");
			}
			BufferedReader inReader = null;
			String licenseText = "";
			try
			{
				inReader = new BufferedReader(new FileReader(file));
				String thisLine;
				while ((thisLine = inReader.readLine()) != null)
				{
					licenseText = licenseText + thisLine + "\n";
				}
				if (licenseText != null && !licenseText.isEmpty())
				{
					pref.setPreference(PreferenceConstants.P_LICENSE, licenseText);
				}
			}
			catch (IOException e)
			{
				ConsoleLogger.printLog("Failed to set the text from the given file: " + e.getLocalizedMessage());
			}
			finally
			{
				try
				{
					inReader.close();
				}
				catch (Exception e)
				{
					;
				}
			}
			ConsoleLogger.printLog("The following text was set as header: \n" + licenseText);
		}
		else
		{
			ConsoleLogger.printErrorLog("Please write a path to an existing file after -p");
		}
	}

	public void generateCore(List<String> fileList)
	{
		francaGenerator = injector.getInstance(FrancaGenerator.class);

		doGenerate(fileList);
	}

	/**
	 * Call the franca generator for the specified list of files.
	 * 
	 * @param fileList
	 *            the list of files to generate code from
	 */
	protected void doGenerate(List<String> fileList)
	{
		fsa.setOutputConfigurations(FPreferences.getInstance().getOutputpathConfiguration());

		XtextResourceSet rsset = injector.getProvider(XtextResourceSet.class).get();

		tempfilelist = new ArrayList<String>();

		/*
		 * Reading the options and the files given in the arguments and they
		 * will be saved in the predefined attributes
		 */
		String francaversion = getFrancaVersion();
		String coreversion = FrancaGeneratorExtensions.getCoreVersion();
		for (String element : fileList)
		{
			File file = new File(createAbsolutPath(element));
			if (!file.exists() || file.isDirectory())
			{
				ConsoleLogger.printErrorLog("The following path won't be generated because it doesn't exists:\n" + element + "\n");
			}
			else
				tempfilelist.add(createAbsolutPath(element));
		}
		if (tempfilelist.size() == 0)
		{
			ConsoleLogger.printErrorLog("There are no valid files to generate !");
			return;
		}
		ConsoleLogger.printLog("Using Franca Version " + francaversion);
		ConsoleLogger.printLog("and CommonAPI Version " + coreversion);

		for (String file : tempfilelist)
		{
			URI uri = URI.createFileURI(file);
			Resource resource = rsset.createResource(uri);
			validationErrorCount = 0;
			if(isValidation) {
				validate(resource);
			}	
			if(validationErrorCount == 0) {
				ConsoleLogger.printLog("Generating code for " + file);
				try {
					francaGenerator.doGenerate(resource, fsa);
				}
				catch (Exception e) {
					System.err.println("Failed to generate code for " + file );
					System.exit(ERROR_STATE);
				}	
			}
			else {
				ConsoleLogger.printErrorLog(file + " contains validation errors !");
				System.exit(ERROR_STATE);
			}
		}
	}

	private void validate(Resource resource) {
		EObject model = null;

		if(resource != null && resource.getURI().isFile())   {

			try {
				resource.load(Collections.EMPTY_MAP);
				model = resource.getContents().get(0);    
			} catch (IOException e) {
				e.printStackTrace();
			}
			ConsoleLogger.printLog("validating...");

			// check for (internal )resource validation errors
			for(org.eclipse.emf.ecore.resource.Resource.Diagnostic error : resource.getErrors()) {
				ConsoleLogger.printErrorLog("ERROR at line: " + error.getLine() + " : " + error.getMessage());
				validationErrorCount++;
			}

			// check for external validation errors if no errors have been detected so far
			if(validationErrorCount == 0 ) {
				if( model instanceof FModel) {
					FModel fmodel = (FModel)model;
					try {
						validator.validateModel(fmodel, cliMessageAcceptor);
					} catch (Exception e) {
						ConsoleLogger.printErrorLog(e.getMessage());
						validationErrorCount++;
					}
				}
			}
		}
	}

	/**
	 * creates a absolute path from a relative path which starts on the current
	 * user directory
	 * 
	 * @param path
	 *            the relative path which start on the current user-directory
	 * @return the created absolute path
	 */
	public String createAbsolutPath(String path)
	{
		return createAbsolutPath(path, System.getProperty("user.dir") + FILESEPARATOR);
	}

	/**
	 * Here we create an absolute path from a relativ path and a rootpath from
	 * which the relative path begins
	 * 
	 * @param path
	 *            the relative path which begins on rootpath
	 * @param rootpath
	 *            an absolute path to a folder
	 * @return the merded absolute path without points
	 */
	private String createAbsolutPath(String path, String rootpath)
	{
		if (System.getProperty("os.name").contains("Windows"))
		{
			if (path.startsWith(":", 1))
				return path;
		}
		else
		{
			if (path.startsWith(FILESEPARATOR))
				return path;
		}

		String ret = (rootpath.endsWith(FILESEPARATOR) ? rootpath : (rootpath + FILESEPARATOR)) + path;
		while (ret.contains(FILESEPARATOR + "." + FILESEPARATOR) || ret.contains(FILESEPARATOR + ".." + FILESEPARATOR))
		{
			if (ret.contains(FILESEPARATOR + ".." + FILESEPARATOR))
			{
				String temp = ret.substring(0, ret.indexOf(FILESEPARATOR + ".."));
				temp = temp.substring(0, temp.lastIndexOf(FILESEPARATOR));
				ret = temp + ret.substring(ret.indexOf(FILESEPARATOR + "..") + 3);
			}
			else
			{
				ret = replaceAll(ret, FILESEPARATOR + "." + FILESEPARATOR, FILESEPARATOR);
			}
		}
		return ret;
	}


	/**
	 * a relaceAll Method which doesn't interprets the toreplace String as a
	 * regex and so you can also replace \ and such special things
	 * 
	 * @param text
	 *            the text who has to be modified
	 * @param toreplace
	 *            the text which has to be replaced
	 * @param replacement
	 *            the text which has to be inserted instead of toreplace
	 * @return the modified text with all toreplace parts replaced with
	 *         replacement
	 */
	public String replaceAll(String text, String toreplace, String replacement)
	{
		String ret = "";
		while (text.contains(toreplace))
		{
			ret += text.substring(0, text.indexOf(toreplace)) + replacement;
			text = text.substring(text.indexOf(toreplace) + toreplace.length());
		}
		ret += text;
		return ret;
	}

	/**
	 * removes recursively all files on the path and his folders and at the end
	 * himself
	 * 
	 * @param path
	 *            the path to the folder which has to be deleted
	 */
	public void deleteTempFiles(File path)
	{
		if (path != null && path.isDirectory())
		{
			for (File file : path.listFiles())
			{
				if (file.isDirectory())
					deleteTempFiles(file);
				file.delete();
			}
		}
		if (path != null)
			path.delete();
	}

	public String getFrancaVersion()
	{
		return Platform.getBundle("org.franca.core").getVersion().toString();
	}

	public void setNoProxyCode() {
		pref.setPreference(PreferenceConstants.P_GENERATEPROXY, "false");
		ConsoleLogger.printLog("No proxy code will be generated");
	}

	public void setNoStubCode() {
		pref.setPreference(PreferenceConstants.P_GENERATESTUB, "false");
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

	public void setCommonDirectory(String optionValue) {
		ConsoleLogger.printLog("Common output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_COMMON, optionValue);
	}

	public void setProxyDirectory(String optionValue) {
		ConsoleLogger.printLog("Proxy output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_PROXIES, optionValue);
	}

	public void setStubtDirectory(String optionValue) {
		ConsoleLogger.printLog("Stub output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_STUBS, optionValue);
	}

	public void setSkeletonDirectory(String optionValue) {
		ConsoleLogger.printLog("Skeleton output directory: " + optionValue);
		pref.setPreference(PreferenceConstants.P_OUTPUT_SKELETON, optionValue);
	}

	public void setLogLevel(String optionValue) {
		if(PreferenceConstants.LOGLEVEL_QUIET.equals(optionValue)) {
			pref.setPreference(PreferenceConstants.P_LOGOUTPUT, "false");
			ConsoleLogger.enableLogging(false);
			ConsoleLogger.enableErrorLogging(false);
		}
		if(PreferenceConstants.LOGLEVEL_VERBOSE.equals(optionValue)) {
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
		pref.setPreference(PreferenceConstants.P_GENERATESKELETON, "true");
		ConsoleLogger.printLog("Skeleton code will be created");
	}

	public void setSkeletonPostfix(String optionValue) {
		String postfix = (optionValue == null ? "Default" : optionValue);
		pref.setPreference(PreferenceConstants.P_SKELETONPOSTFIX, postfix);
		ConsoleLogger.printLog("Skeleton postfix: " + postfix);
	}
	
	public void enableValidation(String optionValue) {
		if(optionValue.equals("no") || optionValue.equals("off")) {
			ConsoleLogger.printLog("Validation is off");
			isValidation = false;
		}
 	}
}
