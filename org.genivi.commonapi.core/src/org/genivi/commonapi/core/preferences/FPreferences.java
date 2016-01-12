/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.preferences;


import java.util.HashMap;
import java.util.Map;
import java.io.File;

import org.eclipse.xtext.generator.IFileSystemAccess;
import org.eclipse.xtext.generator.OutputConfiguration;
import org.franca.core.franca.FModel;


public class FPreferences {

    private static FPreferences instance = null;
    private Map<String, String> preferences = null;

    public Map<String, String> getPreferences() {
		return preferences;
	}

	private FPreferences() {
        preferences = new HashMap<String, String>();
        clidefPreferences();
    }

    public void resetPreferences(){
        preferences.clear();
    }

    public static FPreferences getInstance() {
        if (instance == null) {
            instance = new FPreferences();
        }
        return instance;
    }

    /**
     * Apply defaults
     */
    private void clidefPreferences(){

        if (!preferences.containsKey(PreferenceConstants.P_OUTPUT_COMMON)) {
            preferences.put(PreferenceConstants.P_OUTPUT_COMMON, PreferenceConstants.DEFAULT_OUTPUT);
        }    	
        if (!preferences.containsKey(PreferenceConstants.P_OUTPUT_PROXIES)) {
            preferences.put(PreferenceConstants.P_OUTPUT_PROXIES, PreferenceConstants.DEFAULT_OUTPUT);
        }
        if (!preferences.containsKey(PreferenceConstants.P_OUTPUT_STUBS)) {
            preferences.put(PreferenceConstants.P_OUTPUT_STUBS, PreferenceConstants.DEFAULT_OUTPUT);
        }
        if (!preferences.containsKey(PreferenceConstants.P_OUTPUT_DEFAULT)) {
            preferences.put(PreferenceConstants.P_OUTPUT_DEFAULT, PreferenceConstants.DEFAULT_OUTPUT);
        }    
        if (!preferences.containsKey(PreferenceConstants.P_OUTPUT_SUBDIRS)) {
            preferences.put(PreferenceConstants.P_OUTPUT_SUBDIRS, "false");
        }
        if (!preferences.containsKey(PreferenceConstants.P_OUTPUT_SKELETON)) {
            preferences.put(PreferenceConstants.P_OUTPUT_SKELETON, PreferenceConstants.DEFAULT_OUTPUT);
        }    
        if (!preferences.containsKey(PreferenceConstants.P_LICENSE)) {
            preferences.put(PreferenceConstants.P_LICENSE, PreferenceConstants.DEFAULT_LICENSE);
        }
        if (!preferences.containsKey(PreferenceConstants.P_GENERATE_COMMON)) {
        	preferences.put(PreferenceConstants.P_GENERATE_COMMON, "true");
        }
        if (!preferences.containsKey(PreferenceConstants.P_GENERATE_STUB)) {
            preferences.put(PreferenceConstants.P_GENERATE_STUB, "true");
        }
        if (!preferences.containsKey(PreferenceConstants.P_GENERATE_PROXY)) {
            preferences.put(PreferenceConstants.P_GENERATE_PROXY, "true");
        }
        if (!preferences.containsKey(PreferenceConstants.P_SKELETONPOSTFIX)) {
            preferences.put(PreferenceConstants.P_SKELETONPOSTFIX, "Default");
        }
        if (!preferences.containsKey(PreferenceConstants.P_GENERATE_SKELETON)) {
            preferences.put(PreferenceConstants.P_GENERATE_SKELETON, "false");
        }
        if (!preferences.containsKey(PreferenceConstants.P_LOGOUTPUT)) {
            preferences.put(PreferenceConstants.P_LOGOUTPUT, "true");
        }
        if (!preferences.containsKey(PreferenceConstants.P_ENUMPREFIX)) {
            preferences.put(PreferenceConstants.P_ENUMPREFIX, "");
        }
        if (!preferences.containsKey(PreferenceConstants.P_GENERATE_CODE)) {
            preferences.put(PreferenceConstants.P_GENERATE_CODE, "true");    
        }
        if (!preferences.containsKey(PreferenceConstants.P_GENERATE_DEPENDENCIES)) {
            preferences.put(PreferenceConstants.P_GENERATE_DEPENDENCIES, "true");    
        }
        if (!preferences.containsKey(PreferenceConstants.P_GENERATE_SYNC_CALLS)) {
            preferences.put(PreferenceConstants.P_GENERATE_SYNC_CALLS, "true");    
        }
    }

    public String getPreference(String preferencename, String defaultValue) {
    	
    	if (preferences.containsKey(preferencename)) {
    		return preferences.get(preferencename);
    	}
    	System.err.println("Unknown preference " + preferencename);
        return "";
    }

    public void setPreference(String name, String value) {
        if(preferences != null) {
        	preferences.put(name, value);
        }
    }

    public String getModelPath(FModel model) {
        String ret = model.eResource().getURI().toString();
        return ret;
    }
    
    /**
     * Set the output path configurations (based on stored preference values) for file system access used in the generator.
     * @return the map of output configurations
     */
    public HashMap<String, OutputConfiguration> getOutputpathConfiguration() {
        return getOutputpathConfiguration(null);
    }

    /**
     * Set the output path configurations (based on stored preference values) for file system access used in the generator.
     * @subdir the subdir to use, can be null
     * @return the map of output configurations
     */
    public HashMap<String, OutputConfiguration> getOutputpathConfiguration(String subdir) {

        String defaultDir = getPreference(PreferenceConstants.P_OUTPUT_DEFAULT, PreferenceConstants.DEFAULT_OUTPUT);
        String outputCommonDir = getPreference(PreferenceConstants.P_OUTPUT_COMMON, defaultDir);
        String outputProxyDir = getPreference(PreferenceConstants.P_OUTPUT_PROXIES, defaultDir);
        String outputStubDir = getPreference(PreferenceConstants.P_OUTPUT_STUBS, defaultDir);
        String outputSkelDir = getPreference(PreferenceConstants.P_OUTPUT_SKELETON, defaultDir);

        if (null != subdir && getPreference(PreferenceConstants.P_OUTPUT_SUBDIRS, "false").equals("true")) {
            defaultDir = new File(defaultDir, subdir).getPath();
            outputCommonDir = new File(outputCommonDir, subdir).getPath();
            outputProxyDir = new File(outputProxyDir, subdir).getPath();
            outputStubDir = new File(outputStubDir, subdir).getPath();
            outputSkelDir = new File(outputSkelDir, subdir).getPath();
        }

        // the map of output directory configurations
        HashMap<String, OutputConfiguration>  outputs = new HashMap<String, OutputConfiguration> ();

        OutputConfiguration commonOutput = new OutputConfiguration(PreferenceConstants.P_OUTPUT_COMMON);
        commonOutput.setDescription("Common Output Folder");
        commonOutput.setOutputDirectory(outputCommonDir);
        commonOutput.setCreateOutputDirectory(true);
        outputs.put(IFileSystemAccess.DEFAULT_OUTPUT, commonOutput);
        
        OutputConfiguration proxyOutput = new OutputConfiguration(PreferenceConstants.P_OUTPUT_PROXIES);
        proxyOutput.setDescription("Proxy Output Folder");
        proxyOutput.setOutputDirectory(outputProxyDir);
        proxyOutput.setCreateOutputDirectory(true);
        outputs.put(PreferenceConstants.P_OUTPUT_PROXIES, proxyOutput);
        
        OutputConfiguration stubOutput = new OutputConfiguration(PreferenceConstants.P_OUTPUT_STUBS);
        stubOutput.setDescription("Stub Output Folder");
        stubOutput.setOutputDirectory(outputStubDir);
        stubOutput.setCreateOutputDirectory(true);
        outputs.put(PreferenceConstants.P_OUTPUT_STUBS, stubOutput);
        
        OutputConfiguration skelOutput = new OutputConfiguration(PreferenceConstants.P_OUTPUT_SKELETON);
        skelOutput.setDescription("Skeleton Output Folder");
        skelOutput.setOutputDirectory(outputSkelDir);
        skelOutput.setCreateOutputDirectory(true);
        outputs.put(PreferenceConstants.P_OUTPUT_SKELETON, skelOutput);
        
        return outputs;
    }
}
