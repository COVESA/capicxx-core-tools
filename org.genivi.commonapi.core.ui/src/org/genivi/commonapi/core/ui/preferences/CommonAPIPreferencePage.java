package org.genivi.commonapi.core.ui.preferences;

import org.eclipse.jface.preference.*;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.eclipse.ui.IWorkbench;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;
import org.genivi.commonapi.core.preferences.PreferenceConstants;

/**
 * This class represents a preference page that
 * is contributed to the Preferences dialog. By 
 * subclassing <samp>FieldEditorPreferencePage</samp>, we
 * can use the field support built into JFace that allows
 * us to create a page that is small and knows how to 
 * save, restore and apply itself.
 * <p>
 * This page is used to modify preferences only. They
 * are stored in the preference store that belongs to
 * the main plug-in class. That way, preferences can
 * be accessed directly via the preference store.
 */

public class CommonAPIPreferencePage
	extends FieldEditorPreferencePage
	implements IWorkbenchPreferencePage {
	
	public CommonAPIPreferencePage() {
		super(GRID);
		setDescription("Preferences for CommonAPI");
	}
	
	/**
	 * Creates the field editors. Field editors are abstractions of
	 * the common GUI blocks needed to manipulate various types
	 * of preferences. Each field editor knows how to save and
	 * restore itself.
	 */
	public void createFieldEditors() {
		addField(
				new StringFieldEditor(PreferenceConstants.P_LICENSE, "The header to insert for all generated files", 60, getFieldEditorParent()));
		addField(
				new StringFieldEditor(PreferenceConstants.P_OUTPUT, "Output directory inside project", 30, getFieldEditorParent()));
	}
	
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
	 */
	public void init(IWorkbench workbench) {
		//PreferenceInitializer init = new PreferenceInitializer();
		//init.initializeDefaultPreferences();
		setPreferenceStore(CommonApiUiPlugin.getDefault().getPreferenceStore());
	}
	
}