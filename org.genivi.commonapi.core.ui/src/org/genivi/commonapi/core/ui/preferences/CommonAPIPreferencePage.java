/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.ui.preferences;

import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.core.runtime.preferences.DefaultScope;
import org.eclipse.jface.preference.BooleanFieldEditor;
import org.eclipse.jface.preference.FieldEditor;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.StringFieldEditor;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;

/**
 * This class represents a preference page that is contributed to the
 * Preferences dialog. By subclassing <samp>FieldEditorOverlayPage</samp>.
 * <p>
 * This page is used to modify preferences. They are stored in the preference store that
 * belongs to the main plug-in class.
 */

public class CommonAPIPreferencePage extends FieldEditorOverlayPage implements IWorkbenchPreferencePage
{

    private MultiLineStringFieldEditor license = null;
    private FieldEditor proxyOutput = null;
    private FieldEditor stubOutput  = null;
    private FieldEditor commonOutput  = null;
    private FieldEditor skeletonOutput  = null;
    private StringFieldEditor postFix = null;
    private BooleanFieldEditor generatSkeleton = null;
	private StringFieldEditor enumPrefix = null;

    public CommonAPIPreferencePage()
    {
        super(GRID);
    }

    /**
     * Creates the field editors. Field editors are abstractions of the common
     * GUI blocks needed to manipulate various types of preferences. Each field
     * editor knows how to save and restore itself.
     */
    @Override
    public void createFieldEditors()
    {
    	generatSkeleton = new BooleanFieldEditor(PreferenceConstants.P_GENERATE_SKELETON, "Generate skeleton code", getFieldEditorParent());
    	addField(generatSkeleton);

        license = new MultiLineStringFieldEditor(PreferenceConstants.P_LICENSE, "The license header to be inserted in all generated files", 30,
                getFieldEditorParent());
        license.setLabelText(""); // need to set this parameter (seems to be a bug)
        addField(license);

        // output directory definitions
        commonOutput = new StringFieldEditor(PreferenceConstants.P_OUTPUT_COMMON, "Output directory for the common code", 30,
                getFieldEditorParent());
        addField(commonOutput);
        proxyOutput = new StringFieldEditor(PreferenceConstants.P_OUTPUT_PROXIES, "Output directory for proxy code", 30,
                getFieldEditorParent());
        addField(proxyOutput);
        stubOutput = new StringFieldEditor(PreferenceConstants.P_OUTPUT_STUBS, "Output directory for stub code", 30,
                getFieldEditorParent());
        addField(stubOutput);
        skeletonOutput = new StringFieldEditor(PreferenceConstants.P_OUTPUT_SKELETON, "Output directory for the skeleton code", 30,
                getFieldEditorParent());
        addField(skeletonOutput);

        postFix = new StringFieldEditor(PreferenceConstants.P_SKELETONPOSTFIX, "Postfix for skeleton filenames", 30, getFieldEditorParent());
        addField(postFix);
        postFix.setValidateStrategy(StringFieldEditor.VALIDATE_ON_KEY_STROKE);

        enumPrefix  = new StringFieldEditor(PreferenceConstants.P_ENUMPREFIX, "Prefix for enumeration literals", 30, getFieldEditorParent());
        addField(enumPrefix);
    }

    @Override
    protected void performDefaults()
    {
    	DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_COMMON, "true");
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_PROXY, "true");
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_STUB, "true");
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_SKELETON, "false");
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_SKELETONPOSTFIX, "Default");
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_ENUMPREFIX, "");

    	DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_OUTPUT_PROXIES,
                PreferenceConstants.DEFAULT_OUTPUT);
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE)
                .put(PreferenceConstants.P_OUTPUT_STUBS, PreferenceConstants.DEFAULT_OUTPUT);
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE)
        		.put(PreferenceConstants.P_OUTPUT_COMMON, PreferenceConstants.DEFAULT_OUTPUT);
        DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE)
        		.put(PreferenceConstants.P_OUTPUT_SKELETON, PreferenceConstants.DEFAULT_OUTPUT);

        super.performDefaults();
    }

    @Override
    public void init(IWorkbench workbench)
    {
        if (!isPropertyPage())
            setPreferenceStore(CommonApiUiPlugin.getDefault().getPreferenceStore());
    }

    @Override
    protected String getPageId()
    {
        return PreferenceConstants.PROJECT_PAGEID;
    }

    @Override
    protected IPreferenceStore doGetPreferenceStore()
    {
        return CommonApiUiPlugin.getDefault().getPreferenceStore();
    }

    @Override
    public void propertyChange(PropertyChangeEvent event) {
    	super.propertyChange(event);

    	String preferenceName = ((FieldEditor) event.getSource()).getPreferenceName();
    	// was the skeletonOutput field editor changed ?
    	if(preferenceName != null && preferenceName.equals(PreferenceConstants.P_SKELETONPOSTFIX)) {
    		boolean enableSkeletonCode = !event.getNewValue().toString().isEmpty();
    		generatSkeleton.setEnabled(enableSkeletonCode, getFieldEditorParent());

    		if(!enableSkeletonCode) {
    			// disable skeleton code generation
    			generatSkeleton.loadDefault();

    			IResource resource = (IResource) getElement();
    			try
    			{
    				resource.setPersistentProperty(new QualifiedName(getPageId(), PreferenceConstants.P_GENERATE_SKELETON), "false");
    			}
    			catch (CoreException e)
    			{
    			}
    		}
    	}
    	// will be disposed from FieldEditorPreferencePage !
    }


}
