/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.ui.preferences;

import org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer;
import org.eclipse.jface.preference.IPreferenceStore;

import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;

/**
 * Class used to initialize default preference values.
 */
public class PreferenceInitializer extends AbstractPreferenceInitializer {

    /*
     * (non-Javadoc)
     * @see org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer#
     * initializeDefaultPreferences()
     */
    @Override
    public void initializeDefaultPreferences() {
        IPreferenceStore store = CommonApiUiPlugin.getDefault().getPreferenceStore();
        store.setDefault(PreferenceConstants.P_LICENSE, PreferenceConstants.DEFAULT_LICENSE);
        store.setDefault(PreferenceConstants.P_OUTPUT_PROXIES, PreferenceConstants.DEFAULT_OUTPUT);
        store.setDefault(PreferenceConstants.P_OUTPUT_STUBS, PreferenceConstants.DEFAULT_OUTPUT);
        store.setDefault(PreferenceConstants.P_OUTPUT_COMMON, PreferenceConstants.DEFAULT_OUTPUT);
        store.setDefault(PreferenceConstants.P_OUTPUT_SKELETON, PreferenceConstants.DEFAULT_OUTPUT);
        store.setDefault(PreferenceConstants.P_GENERATE_COMMON, true);
        store.setDefault(PreferenceConstants.P_GENERATE_PROXY, true);
        store.setDefault(PreferenceConstants.P_GENERATE_STUB, true);
        store.setDefault(PreferenceConstants.P_GENERATE_SKELETON, false);
        store.setDefault(PreferenceConstants.P_SKELETONPOSTFIX, PreferenceConstants.DEFAULT_SKELETONPOSTFIX);
        store.setDefault(PreferenceConstants.P_USEPROJECTSETTINGS, false);
        store.setDefault(PreferenceConstants.P_GENERATE_DEPENDENCIES, true);
        store.setDefault(PreferenceConstants.P_ENABLE_CORE_VALIDATOR, true);
        store.setDefault(PreferenceConstants.P_ENABLE_CORE_DEPLOYMENT_VALIDATOR, true);
        store.setDefault(PreferenceConstants.P_GENERATE_SYNC_CALLS, true);
    }

}
