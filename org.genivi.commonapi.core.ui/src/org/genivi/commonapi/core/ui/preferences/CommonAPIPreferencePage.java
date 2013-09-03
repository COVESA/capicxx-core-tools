/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.ui.preferences;

import org.eclipse.core.runtime.preferences.DefaultScope;
import org.eclipse.jface.preference.FieldEditor;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.StringFieldEditor;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;

/**
 * This class represents a preference page that is contributed to the
 * Preferences dialog. By subclassing <samp>FieldEditorOverlayPage</samp>, we
 * can use the field support built into JFace that allows us to create a page
 * that is small and knows how to save, restore and apply itself.
 * <p>
 * This page is used to modify preferences only. They are stored in the
 * preference store that belongs to the main plug-in class. That way,
 * preferences can be accessed directly via the preference store.
 */

public class CommonAPIPreferencePage extends FieldEditorOverlayPage implements IWorkbenchPreferencePage {

    private FieldEditor lizense = null;
    private FieldEditor output = null;

    public CommonAPIPreferencePage() {
        super(GRID);
        setDescription("Preferences for CommonAPI");
    }

    /**
     * Creates the field editors. Field editors are abstractions of the common
     * GUI blocks needed to manipulate various types of preferences. Each field
     * editor knows how to save and restore itself.
     */
    public void createFieldEditors() {
        lizense = new MultiLineTextField(PreferenceConstants.P_LICENSE, "The header to insert for all generated files",
                60, getFieldEditorParent());
        addField(lizense);
        output = new StringFieldEditor(PreferenceConstants.P_OUTPUT, "Output directory inside project", 30,
                getFieldEditorParent());
        addField(output);
    }

    @Override
    protected void performDefaults() {
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_OUTPUT,
                PreferenceConstants.DEFAULT_OUTPUT);
        super.performDefaults();
    }

    public void init(IWorkbench workbench) {
        if (!isPropertyPage())
            setPreferenceStore(CommonApiUiPlugin.getDefault().getPreferenceStore());
    }

    @Override
    protected String getPageId() {
        return PreferenceConstants.PROJECT_PAGEID;
    }
    @Override
    protected IPreferenceStore doGetPreferenceStore() {
        return CommonApiUiPlugin.getDefault().getPreferenceStore();
    }

}