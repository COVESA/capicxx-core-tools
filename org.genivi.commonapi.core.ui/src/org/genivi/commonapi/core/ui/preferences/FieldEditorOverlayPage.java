/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.ui.preferences;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.core.runtime.preferences.DefaultScope;
import org.eclipse.core.runtime.preferences.InstanceScope;
import org.eclipse.jface.preference.FieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.IPreferenceNode;
import org.eclipse.jface.preference.IPreferencePage;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.BusyIndicator;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.IWorkbenchPropertyPage;
import org.genivi.commonapi.core.preferences.PreferenceConstants;

public abstract class FieldEditorOverlayPage extends FieldEditorPreferencePage implements IWorkbenchPropertyPage
{
    private static final String FALSE         = "false";                     //$NON-NLS-1$
    private static final String TRUE          = "true";                      //$NON-NLS-1$

    // Stores all created field editors
    private List<FieldEditor>   editors       = new ArrayList<FieldEditor>();

    private List<Button>        buttons       = new ArrayList<Button>();

    private Button				checkboxcommon = null;
    private Button              checkboxproxy = null;
    private Button              checkboxstub  = null;
    private Button				checkboxProject = null;
    private Button				checkboxIncludes = null;
    private Button				checkboxSyncCalls = null;
    
    // Stores owning element of properties
    private IAdaptable          element;

    // Overlay preference store for property pages
    private IPreferenceStore    overlayStore;

    // Cache for page id
    private String              pageId;

    /**
     * Constructor
     * 
     * @param style
     *            - layout style
     */
    public FieldEditorOverlayPage(int style)
    {
        super(style);
    }

    /**
     * Constructor
     * 
     * @param title
     *            - title string
     * @param style
     *            - layout style
     */
    public FieldEditorOverlayPage(String title, int style)
    {
        super(title, style);
    }

    /**
     * Constructor
     * 
     * @param title
     *            - title string
     * @param image
     *            - title image
     * @param style
     *            - layout style
     */
    public FieldEditorOverlayPage(String title, ImageDescriptor image, int style)
    {
        super(title, image, style);
    }

    /**
     * Returns the id of the current preference page as defined in plugin.xml
     * Subclasses must implement.
     * 
     * @return - the qualifier
     */
    protected abstract String getPageId();

    /**
     * Receives the object that owns the properties shown in this property page.
     * 
     * @see org.eclipse.ui.IWorkbenchPropertyPage#setElement(org.eclipse.core.runtime.IAdaptable)
     */
    public void setElement(IAdaptable element)
    {
        this.element = element;
    }

    /**
     * Delivers the object that owns the properties shown in this property page.
     * 
     * @see org.eclipse.ui.IWorkbenchPropertyPage#getElement()
     */
    public IAdaptable getElement()
    {
        return element;
    }

    /**
     * Returns true if this instance represents a property page
     * 
     * @return - true for property pages, false for preference pages
     */
    public boolean isPropertyPage()
    {
        return getElement() != null;
    }

    /**
     * We override the addField method. This allows us to store each field
     * editor added by subclasses in a list for later processing.
     * 
     * @see org.eclipse.jface.preference.FieldEditorPreferencePage#addField(org.eclipse.jface.preference.FieldEditor)
     */
    protected void addField(FieldEditor editor)
    {
        editors.add(editor);
        super.addField(editor);
    }

    /**
     * We override the createControl method. In case of property pages we create
     * a new PropertyStore as local preference store. After all control have
     * been create, we enable/disable these controls.
     * 
     * @see org.eclipse.jface.preference.PreferencePage#createControl()
     */
    public void createControl(Composite parent)
    {
        // Special treatment for property pages
        if (isPropertyPage())
        {
            // Cache the page id
            pageId = getPageId();
            // Create an overlay preference store and fill it with properties
            try
            {
                IResource res = (IResource) element;
                if (res instanceof IFolder)
                {
                    res = res.getProject();
                    setElement(res);
                }
                overlayStore = new PropertyStore(res, super.getPreferenceStore(), pageId);
            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
            // Set overlay store as current preference store
            setPreferenceStore(overlayStore);
        }
        if (parent != null)
        {
            super.createControl(parent);
            // Update state of all subclass controls
            if (isPropertyPage())
                enableControls();
        }
    }

    /**
     * We override the createContents method. In case of property pages we
     * insert two radio buttons at the top of the page.
     * 
     * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
     */
    protected Control createContents(Composite parent)
    {
        createButtons(parent);
        enableControls();
        return super.createContents(parent);
    }

    public void createButtons(Composite parent)
    {
        Composite comp = new Composite(parent, SWT.NONE);
        GridLayout layout = new GridLayout(2, false);
        layout.marginHeight = 0;
        layout.marginWidth = 0;
        comp.setLayout(layout);
        comp.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        Composite radioGroup = new Composite(comp, SWT.BOTTOM);
        radioGroup.setLayout(new GridLayout());
        radioGroup.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        
        Group settingsGroup = new Group(radioGroup, SWT.SHADOW_IN);
        //settingsGroup.setText("Scope of properties: ");
        settingsGroup.setLayout(new GridLayout());
        settingsGroup.setLayoutData(new GridData());        
        
        checkboxProject = new Button(settingsGroup, SWT.CHECK);
        checkboxProject.setText("Enable project specific settings");

        checkboxcommon = new Button(radioGroup, SWT.CHECK);
        checkboxcommon.setText(Messages.getString("OverlayPage.Generate_Common"));
        checkboxproxy = new Button(radioGroup, SWT.CHECK);
        checkboxproxy.setText(Messages.getString("OverlayPage.Generate_Proxy"));
        checkboxstub = new Button(radioGroup, SWT.CHECK);
        checkboxstub.setText(Messages.getString("OverlayPage.Generate_Stub"));
        checkboxIncludes = new Button(radioGroup, SWT.CHECK);
        checkboxIncludes.setText(Messages.getString("OverlayPage.Generate_Includes"));
        checkboxSyncCalls = new Button(radioGroup, SWT.CHECK);
        checkboxSyncCalls.setText(Messages.getString("OverlayPage.Generate_SyncCalls"));
        
        buttons.add(checkboxcommon);
        buttons.add(checkboxproxy);
        buttons.add(checkboxstub);
        buttons.add(checkboxProject);
        buttons.add(checkboxIncludes);
        buttons.add(checkboxSyncCalls);
        
        String genCommon = TRUE;
        String genProxy = TRUE;
        String genStub = TRUE;
        String project = FALSE;
        String dependencies = TRUE;
        String syncCalls = TRUE;
        
        if (isPropertyPage())
        {
        	// get values from the persistent properties of theses resources and set the button states
            try
            {
            	genCommon = ((IResource) getElement())
            			.getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_COMMON));
                genProxy = ((IResource) getElement())
                        .getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_PROXY));
                genStub = ((IResource) getElement())
                        .getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_STUB));
                project = ((IResource) getElement())
                        .getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_USEPROJECTSETTINGS));
                dependencies = ((IResource) getElement())
                        .getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_DEPENDENCIES));
                syncCalls = ((IResource) getElement())
                        .getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_SYNC_CALLS));
            }
            catch (CoreException e)
            {
            	// failed to access this resource...
            }
        	// Not all properties are set for this resource
            if (genCommon == null) {
            	genCommon = TRUE;
            }
            if(genProxy == null) {
            	genProxy = TRUE;
            }
            if(genStub == null) {
            	genStub = TRUE;
            }
            if(project == null) {
            	project = FALSE;
            }
            if(dependencies == null) {
            	dependencies = TRUE;
            }
            if(syncCalls == null) {
            	syncCalls = TRUE;
            }
        }
        else // is a preference page
        {
            genCommon = DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_COMMON, TRUE);
            genCommon = InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_COMMON, genCommon);

            genProxy = DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_PROXY, TRUE);
            genProxy = InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_PROXY, genProxy);

            genStub = DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_STUB, TRUE);
            genStub = InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_STUB, genStub);

            project = DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_USEPROJECTSETTINGS, FALSE);
            project = InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_USEPROJECTSETTINGS, project);

            dependencies = DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_DEPENDENCIES, TRUE);
            dependencies = InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_DEPENDENCIES, dependencies);

            syncCalls = DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_SYNC_CALLS, TRUE);
            syncCalls = InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATE_SYNC_CALLS, syncCalls);
        }
        // set the selection state of the buttons
        checkboxcommon.setSelection(TRUE.equals(genCommon));
        checkboxproxy.setSelection(TRUE.equals(genProxy));
        checkboxstub.setSelection(TRUE.equals(genStub));
        checkboxProject.setSelection(TRUE.equals(project));
        checkboxIncludes.setSelection(TRUE.equals(dependencies));
        checkboxSyncCalls.setSelection(TRUE.equals(syncCalls));
    }

    /**
     * Returns in case of property pages the overlay store, in case of
     * preference pages the standard preference store
     * 
     * @see org.eclipse.jface.preference.PreferencePage#getPreferenceStore()
     */
    public IPreferenceStore getPreferenceStore()
    {
        if (isPropertyPage()) {
            return overlayStore;
        }
        return super.getPreferenceStore();
    }

    /**
     * Enables the field editors and buttons of this page 
     */
    protected void enableControls()
    {
        Composite parent = getFieldEditorParent();
        Iterator<FieldEditor> it = editors.iterator();
        while (it.hasNext())
        {
            FieldEditor editor = it.next();
            editor.setEnabled(true, parent);
        }
        for (Button button : buttons)
        {
            button.setEnabled(true);
        }
    }

    /**
     * We override the performOk method. In case of property pages we copy the
     * values in the overlay store into the property values of the selected
     * project. We also save the state of the radio buttons.
     * 
     * @see org.eclipse.jface.preference.IPreferencePage#performOk()
     */
    public boolean performOk()
    {
    	boolean result = super.performOk();
    	String genCommon = (checkboxcommon.getSelection()) ? TRUE : FALSE;
    	String genProxy = (checkboxproxy.getSelection()) ? TRUE : FALSE;
    	String genStub = (checkboxstub.getSelection()) ? TRUE : FALSE;
    	String project = (checkboxProject.getSelection()) ? TRUE : FALSE;
    	String dependencies = (checkboxIncludes.getSelection()) ? TRUE : FALSE;
    	String syncCalls = (checkboxSyncCalls.getSelection()) ? TRUE : FALSE;

    	if (result) {
    		if(isPropertyPage()) {
    			IResource resource = (IResource) getElement();
    			try
    			{
    				resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_USEPROJECTSETTINGS), project);
    				resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_COMMON), genCommon);
    				resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_PROXY), genProxy);
    				resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_STUB), genStub);
    				resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_DEPENDENCIES), dependencies);
    				resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATE_SYNC_CALLS), syncCalls);
    			}
    			catch (CoreException e)
    			{
    				result = false;
    			}
    		} else { // preference page
    			InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_COMMON, genCommon);
    			InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_PROXY, genProxy);
    			InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_STUB, genStub);
    			InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_DEPENDENCIES, dependencies);
    			InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_SYNC_CALLS, syncCalls);
    		}
    	}
    	return result;
    }

    /**
     * In case of property page set checkboxes to true
     */ 
    protected void performDefaults()
    {
    	enableControls();
    	checkboxcommon.setSelection(true);
    	checkboxproxy.setSelection(true);
    	checkboxstub.setSelection(true);
    	checkboxProject.setSelection(false);
    	checkboxIncludes.setSelection(true);
    	InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_COMMON, TRUE);
        InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_PROXY, TRUE);
        InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_STUB, TRUE);    	
        InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_USEPROJECTSETTINGS, FALSE);    	
        InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_DEPENDENCIES, TRUE);    	
        InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATE_SYNC_CALLS, TRUE);    	

    	super.performDefaults();
    }

    /**
     * Show a single preference pages
     * 
     * @param id
     *            - the preference page identification
     * @param page
     *            - the preference page
     */
    protected void showPreferencePage(String id, IPreferencePage page)
    {
        final IPreferenceNode targetNode = new PreferenceNode(id, page);
        PreferenceManager manager = new PreferenceManager();
        manager.addToRoot(targetNode);
        final PreferenceDialog dialog = new PreferenceDialog(getControl().getShell(), manager);
        BusyIndicator.showWhile(getControl().getDisplay(), new Runnable()
        {
            public void run()
            {
                dialog.create();
                dialog.setMessage(targetNode.getLabelText());
                dialog.open();
            }
        });
    }

}
