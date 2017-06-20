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
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;

public class ValidatorCorePreferencesPage extends FieldEditorPreferencePage
        implements IWorkbenchPreferencePage {


    @Override
    public void checkState() {
        super.checkState();
    }

    @Override
    public void createFieldEditors() {
        addField(new BooleanFieldEditor(PreferenceConstants.P_ENABLE_CORE_VALIDATOR,
                "Enable CommonAPI-Core specific validation of Franca IDL files", getFieldEditorParent()));
    }

    @Override
    public void init(IWorkbench workbench) {
        IPreferenceStore prefStore = CommonApiUiPlugin.getValidatorPreferences();
        setPreferenceStore(prefStore);
    }
}
