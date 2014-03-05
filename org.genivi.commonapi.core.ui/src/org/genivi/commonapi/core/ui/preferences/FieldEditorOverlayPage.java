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

import org.eclipse.core.resources.IFile;
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
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IWorkbenchPropertyPage;
import org.genivi.commonapi.core.preferences.PreferenceConstants;

public abstract class FieldEditorOverlayPage extends FieldEditorPreferencePage implements IWorkbenchPropertyPage
{

    /***
     * Name of resource property for the selection of workbench or project
     * settings
     ***/
    //	public static final String USEPROJECTSETTINGS = "useProjectSettings"; //$NON-NLS-1$

    private static final String FALSE         = "false";                     //$NON-NLS-1$
    private static final String TRUE          = "true";                      //$NON-NLS-1$

    // Stores all created field editors
    private List<FieldEditor>   editors       = new ArrayList<FieldEditor>();

    private List<Button>        buttons       = new ArrayList<Button>();

    private Button              checkboxproxy = null;
    private Button              checkboxstub  = null;

    // Stores owning element of properties
    private IAdaptable          element;

    // Additional buttons for property pages
    private Button              useWorkspaceSettingsButton, useProjectSettingsButton, configureButton;

    // Overlay preference store for property pages
    private IPreferenceStore    overlayStore;

    // The image descriptor of this pages title image
    private ImageDescriptor     image;

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
        this.image = image;
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

    protected void addButton(Button button)
    {
        buttons.add(button);
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
                updateFieldEditors();
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
        if (isPropertyPage())
        {
            createSelectionGroup(parent);
            updateFieldEditors();
        }
        else
            createButtons(parent);
        return super.createContents(parent);
    }

    /**
     * Creates and initializes a selection group with two choice buttons and one
     * push button.
     * 
     * @param parent
     *            - the parent composite
     */
    private void createSelectionGroup(Composite parent)
    {
        Composite comp = new Composite(parent, SWT.NONE);
        GridLayout layout = new GridLayout(2, false);
        layout.marginHeight = 0;
        layout.marginWidth = 0;
        comp.setLayout(layout);
        comp.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        Composite radioGroup = new Composite(comp, SWT.NONE);
        radioGroup.setLayout(new GridLayout());
        radioGroup.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

        String msg = Messages.getString("OverlayPage.Use_Workspace_Settings");
        if (element instanceof IFile)
            msg = Messages.getString("OverlayPage.Use_Project_Settings");
        useWorkspaceSettingsButton = createRadioButton(radioGroup, msg); //$NON-NLS-1$
        msg = Messages.getString("OverlayPage.Use_Project_Settings");
        if (element instanceof IFile)
            msg = Messages.getString("OverlayPage.Use_File_Settings");
        useProjectSettingsButton = createRadioButton(radioGroup, msg); //$NON-NLS-1$
        configureButton = new Button(comp, SWT.PUSH);
        msg = Messages.getString("OverlayPage.Configure_Workspace_Settings");
        if (element instanceof IFile)
            msg = Messages.getString("OverlayPage.Configure_Project_Settings");
        configureButton.setText(msg); //$NON-NLS-1$
        configureButton.addSelectionListener(new SelectionAdapter()
        {
            public void widgetSelected(SelectionEvent e)
            {
                configureWorkspaceSettings();
            }
        });
        createButtons(parent);
        // Set workspace/project radio buttons
        try
        {
            String use = ((IResource) element).getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.USEPROJECTSETTINGS));
            if (use == null)
            {
                ((IResource) element).setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.USEPROJECTSETTINGS),
                        Boolean.FALSE.toString());
                use = FALSE;
            }
            if (TRUE.equals(use))
            {
                useProjectSettingsButton.setSelection(true);
                configureButton.setEnabled(false);
            }
            else
                useWorkspaceSettingsButton.setSelection(true);
        }
        catch (CoreException e)
        {
            useWorkspaceSettingsButton.setSelection(true);
        }
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
        checkboxproxy = new Button(radioGroup, SWT.CHECK);
        checkboxproxy.setText(Messages.getString("OverlayPage.Generate_Proxy"));
        checkboxstub = new Button(radioGroup, SWT.CHECK);
        checkboxstub.setText(Messages.getString("OverlayPage.Generate_Stub"));
        if (isPropertyPage())
        {
            try
            {
                String use = ((IResource) getElement())
                        .getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATEPROXY));
                if (TRUE.equals(use))
                    checkboxproxy.setSelection(true);
            }
            catch (CoreException e)
            {
                checkboxproxy.setSelection(true);
            }
            try
            {
                String use = ((IResource) getElement())
                        .getPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATESTUB));
                if (TRUE.equals(use))
                    checkboxstub.setSelection(true);
                else if (!checkboxproxy.getSelection())
                {
                    checkboxproxy.setSelection(true);
                    checkboxstub.setSelection(true);
                }
            }
            catch (CoreException e)
            {
                checkboxstub.setSelection(true);
            }
        }
        else
        {
            String use = DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATEPROXY, "");
            use = InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATEPROXY, use);
            if (TRUE.equals(use))
                checkboxproxy.setSelection(true);
            use = DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATESTUB, "");
            use = InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_GENERATESTUB, use);
            if (TRUE.equals(use))
                checkboxstub.setSelection(true);
            if (!checkboxproxy.getSelection() && !checkboxstub.getSelection())
            {
                checkboxproxy.setSelection(true);
                checkboxstub.setSelection(true);
            }
        }
        checkboxproxy.addSelectionListener(new SelectionAdapter()
        {

            @Override
            public void widgetSelected(SelectionEvent e)
            {
                if (!checkboxproxy.getSelection() && !checkboxstub.getSelection())
                    checkboxstub.setSelection(true);
            }

        });
        checkboxstub.addSelectionListener(new SelectionAdapter()
        {

            @Override
            public void widgetSelected(SelectionEvent e)
            {
                if (!checkboxproxy.getSelection() && !checkboxstub.getSelection())
                    checkboxproxy.setSelection(true);
            }
        });
        addButton(checkboxproxy);
        addButton(checkboxstub);
    }

    /**
     * Convenience method creating a radio button
     * 
     * @param parent
     *            - the parent composite
     * @param label
     *            - the button label
     * @return - the new button
     */
    private Button createRadioButton(Composite parent, String label)
    {
        final Button button = new Button(parent, SWT.RADIO);
        button.setText(label);
        button.addSelectionListener(new SelectionAdapter()
        {
            public void widgetSelected(SelectionEvent e)
            {
                configureButton.setEnabled(button == useWorkspaceSettingsButton);
                updateFieldEditors();
            }
        });
        return button;
    }

    /**
     * Returns in case of property pages the overlay store, in case of
     * preference pages the standard preference store
     * 
     * @see org.eclipse.jface.preference.PreferencePage#getPreferenceStore()
     */
    public IPreferenceStore getPreferenceStore()
    {
        if (isPropertyPage())
            return overlayStore;
        return super.getPreferenceStore();
    }

    /*
     * Enables or disables the field editors and buttons of this page
     */
    private void updateFieldEditors()
    {
        // We iterate through all field editors
        boolean enabled = useProjectSettingsButton.getSelection();
        updateFieldEditors(enabled);
    }

    /**
     * Enables or disables the field editors and buttons of this page Subclasses
     * may override.
     * 
     * @param enabled
     *            - true if enabled
     */
    protected void updateFieldEditors(boolean enabled)
    {
        Composite parent = getFieldEditorParent();
        Iterator<FieldEditor> it = editors.iterator();
        while (it.hasNext())
        {
            FieldEditor editor = it.next();
            editor.setEnabled(enabled, parent);
        }
        for (Button button : buttons)
        {
            button.setEnabled(enabled);
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
        if (result && isPropertyPage())
        {
            // Save state of radio buttons in project properties
            IResource resource = (IResource) getElement();
            try
            {
                String value = (useProjectSettingsButton.getSelection()) ? TRUE : FALSE;
                resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.USEPROJECTSETTINGS), value);
                value = (checkboxproxy.getSelection()) ? TRUE : FALSE;
                resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATEPROXY), value);
                value = (checkboxstub.getSelection()) ? TRUE : FALSE;
                resource.setPersistentProperty(new QualifiedName(pageId, PreferenceConstants.P_GENERATESTUB), value);
            }
            catch (CoreException e)
            {
            }
        }
        else if (result)
        {
            String value = (checkboxproxy.getSelection()) ? TRUE : FALSE;
            InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATEPROXY, value);
            value = (checkboxstub.getSelection()) ? TRUE : FALSE;
            InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_GENERATESTUB, value);
        }
        return result;
    }

    /**
     * We override the performDefaults method. In case of property pages we
     * switch back to the workspace settings and disable the field editors.
     * 
     * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
     */
    protected void performDefaults()
    {
        if (isPropertyPage())
        {
            useWorkspaceSettingsButton.setSelection(true);
            useProjectSettingsButton.setSelection(false);
            configureButton.setEnabled(true);
            checkboxproxy.setSelection(true);
            checkboxstub.setSelection(true);
            updateFieldEditors();
        }
        super.performDefaults();
    }

    /**
     * Creates a new preferences page and opens it
     * 
     * @see com.bdaum.SpellChecker.preferences.SpellCheckerPreferencePage#configureWorkspaceSettings()
     */
    protected void configureWorkspaceSettings()
    {
        try
        {
            // create a new instance of the current class
            FieldEditorOverlayPage page = (FieldEditorOverlayPage) this.getClass().newInstance();
            page.setTitle(getTitle());
            if (element instanceof IFile)
                page.setElement(((IFile) element).getProject());
            page.setImageDescriptor(image);
            // and show it
            showPreferencePage(pageId, page);
        }
        catch (InstantiationException e)
        {
            e.printStackTrace();
        }
        catch (IllegalAccessException e)
        {
            e.printStackTrace();
        }
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
