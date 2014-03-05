/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.genivi.commonapi.core.validator.preferencepage;

import org.eclipse.jface.preference.BooleanFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;

public class ValidatorCorePreferencesPage extends FieldEditorPreferencePage
        implements IWorkbenchPreferencePage {

    public final static String ENABLED_CORE_VALIDATOR = "ENABLED_CORE_VALIDATOR";

    @Override
    public void checkState() {
        super.checkState();
    }

    @Override
    public void createFieldEditors() {
        addField(new BooleanFieldEditor(ENABLED_CORE_VALIDATOR,
                "validator enabled", getFieldEditorParent()));
    }

    @Override
    public void init(IWorkbench workbench) {
        IPreferenceStore prefStore = CommonApiUiPlugin.getDefault()
                .getPreferenceStore();
        setPreferenceStore(prefStore);
        setDescription("Disable or enable the core validator!");
        prefStore.setDefault(
                ValidatorCorePreferencesPage.ENABLED_CORE_VALIDATOR, true);
    }
}
