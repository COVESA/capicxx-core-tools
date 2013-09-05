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
public class PreferenceConstants {
    public static final String SCOPE = "org.genivi.commonapi.core.ui";
    public static final String P_LICENSE = "licenseHeader";
    public static final String P_OUTPUT = "outputDir";
    public static final String USEPROJECTSETTINGS = "useProjectSettings";
    public static final String DEFAULT_OUTPUT = "./src-gen/";
    public static final String DEFAULT_LICENSE = "This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.\n" +
            "If a copy of the MPL was not distributed with this file, You can obtain one at\n" +
            "http://mozilla.org/MPL/2.0/.";
}
