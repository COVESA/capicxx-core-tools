/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.ui;

import org.eclipse.jface.preference.PreferenceStore;
import org.eclipse.ui.plugin.AbstractUIPlugin;
import org.eclipse.xtext.ui.ecore.Activator;
import org.osgi.framework.BundleContext;

public class CommonApiUiPlugin extends AbstractUIPlugin {
	public static final String PLUGIN_ID = "org.genivi.commonapi.core.ui"; //$NON-NLS-1$

	private static CommonApiUiPlugin INSTANCE;

	public CommonApiUiPlugin() {
	}

	@Override
	public void start(final BundleContext context) throws Exception {
		super.start(context);
		INSTANCE = this;
	}

	@Override
	public void stop(final BundleContext context) throws Exception {
		INSTANCE = null;
		super.stop(context);
	}

	public static CommonApiUiPlugin getInstance() {
		return INSTANCE;
	}
	
	public static CommonApiUiPlugin getDefault() {
		return INSTANCE;
	}
}
