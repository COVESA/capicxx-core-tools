/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.ui.preferences;

import java.util.ResourceBundle;

public class Messages {

    private final static String RESOURCE_BUNDLE = "org.genivi.commonapi.core.ui.preferences.Messages";//$NON-NLS-1$

    private static ResourceBundle fgResourceBundle = null;

    private static boolean notRead = true;

    public Messages() {
    }

    public static ResourceBundle getResourceBundle() {
        if (notRead) {
            notRead = false;
            try {
                fgResourceBundle = ResourceBundle.getBundle(RESOURCE_BUNDLE);
            } catch (Exception e) {
            }
        }

        return fgResourceBundle;
    }

    public static String getString(String key) {
        try {
            return getResourceBundle().getString(key);
        } catch (Exception e) {
            return "!" + key + "!";//$NON-NLS-2$ //$NON-NLS-1$
        }
    }
}
