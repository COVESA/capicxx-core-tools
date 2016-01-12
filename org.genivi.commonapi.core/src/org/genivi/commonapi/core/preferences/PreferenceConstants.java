/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.preferences;

/**
 * Constant definitions for plug-in preferences
 */
public interface PreferenceConstants
{
    public static final String SCOPE              = "org.genivi.commonapi.core.ui";
    public static final String PROJECT_PAGEID     = "org.genivi.commonapi.core.ui.preferences.CommonAPIPreferencePage";

    // preference keys
    public static final String P_LICENSE          		= "licenseHeader";
    public static final String P_OUTPUT_PROXIES   		= "outputDirProxies";
    public static final String P_OUTPUT_STUBS     		= "outputDirStubs";
	public static final String P_OUTPUT_COMMON    		= "outputDirCommon";
	public static final String P_OUTPUT_SKELETON  		= "outputDirSkeleton";
	public static final String P_OUTPUT_DEFAULT   		= "outputDirDefault";
	public static final String P_OUTPUT_SUBDIRS   		= "outputSubdirs";
	public static final String P_GENERATE_COMMON  		= "generatecommon";
    public static final String P_GENERATE_PROXY    		= "generateproxy";
    public static final String P_GENERATE_STUB     		= "generatestub";
    public static final String P_GENERATE_SKELETON 		= "generateskeleton";
	public static final String P_LOGOUTPUT        		= "logoutput";
	public static final String P_ENUMPREFIX       		= "enumprefix";
	public static final String P_SKELETONPOSTFIX  		= "skeletonpostfix";
	public static final String P_USEPROJECTSETTINGS 	= "useProjectSettings";
	public static final String P_GENERATE_CODE    		= "generateCode";
	public static final String P_GENERATE_DEPENDENCIES 	= "generateDependencies";
	public static final String P_GENERATE_SYNC_CALLS 	= "generateSyncCalls";
    public static final String P_ENABLE_CORE_VALIDATOR 	= "enableCoreValidator";
    
	// preference values
    public static final String DEFAULT_OUTPUT     = "./src-gen/";
	public static final String LOGLEVEL_QUIET     = "quiet";
	public static final String LOGLEVEL_VERBOSE   = "verbose";
    public static final String DEFAULT_LICENSE    = "This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.\n"
                                                   + "If a copy of the MPL was not distributed with this file, You can obtain one at\n"
                                                   + "http://mozilla.org/MPL/2.0/.";
	public static final String DEFAULT_SKELETONPOSTFIX = "Default";
	public static final String NO_CODE            = "";
}
