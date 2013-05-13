package org.genivi.commonapi.core.ui.preferences;

import org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer;
import org.eclipse.core.runtime.preferences.DefaultScope;
import org.eclipse.core.runtime.preferences.IEclipsePreferences;

/**
 * Class used to initialize default preference values.
 */
public class PreferenceInitializer extends AbstractPreferenceInitializer {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer#initializeDefaultPreferences()
	 */
	public void initializeDefaultPreferences() {
		IEclipsePreferences store = DefaultScope.INSTANCE.getNode("org.genivi.commonapi.core"); 
		store.put(PreferenceConstants.P_LICENSE,
				"* This Source Code Form is subject to the terms of the Mozilla Public\n" + 
				"* License, v. 2.0. If a copy of the MPL was not distributed with this\n" +
				"* file, You can obtain one at http://mozilla.org/MPL/2.0/.");
	}

}
