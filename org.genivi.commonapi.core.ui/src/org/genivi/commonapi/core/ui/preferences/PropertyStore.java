/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.ui.preferences;

import java.io.IOException;
import java.io.OutputStream;

import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceStore;
import org.genivi.commonapi.core.preferences.FPreferences;
import org.genivi.commonapi.core.preferences.PreferenceConstants;

public class PropertyStore extends PreferenceStore {

    private IResource resource;
    private IPreferenceStore workbenchStore;
    private String pageId;
    private boolean inserting = false;

    public PropertyStore(IResource resource, IPreferenceStore workbenchStore, String pageId) throws IOException {

        this.resource = resource;
        this.workbenchStore = workbenchStore;
        this.pageId = pageId;
        FPreferences pref = FPreferences.getInstance();
        try {
            pref.setPreference(PreferenceConstants.P_LICENSE, getProperty(PreferenceConstants.P_LICENSE), resource
                    .getFullPath().toPortableString());
            pref.setPreference(PreferenceConstants.P_OUTPUT, getProperty(PreferenceConstants.P_OUTPUT), resource
                    .getFullPath().toPortableString());
            pref.setPreference(PreferenceConstants.USEPROJECTSETTINGS,
                    getProperty(PreferenceConstants.USEPROJECTSETTINGS), resource.getFullPath().toPortableString());
        } catch (CoreException e) {
            throw new IOException(Messages.getString("PropertyStore.Cannot_read_resource_property")
                    + PreferenceConstants.P_LICENSE);
        }
    }

    /*** Write modified values back to properties ***/

    /*
     * (non-Javadoc)
     * @see org.eclipse.jface.preference.IPersistentPreferenceStore#save()
     */
    public void save() throws IOException {
        try {
            if (getProperty(PreferenceConstants.USEPROJECTSETTINGS).equals(Boolean.TRUE.toString()))
                writeProperties();
        } catch (CoreException e) {
            throw new IOException(Messages.getString("PropertyStore.Cannot_read_resource_property")
                    + PreferenceConstants.USEPROJECTSETTINGS); //$NON-NLS-1$
        }
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.PreferenceStore#save(java.io.OutputStream,
     * java.lang.String)
     */
    public void save(OutputStream out, String header) throws IOException {
        writeProperties();
    }

    /**
     * Writes modified preferences into resource properties.
     */
    private void writeProperties() throws IOException {
        String[] preferences = super.preferenceNames();
        for (int i = 0; i < preferences.length; i++) {
            String name = preferences[i];
            try {
                setProperty(name, getString(name));
            } catch (CoreException e) {
                throw new IOException(Messages.getString("PropertyStore.Cannot_write_resource_property") + name); //$NON-NLS-1$
            }
        }
    }

    /**
     * Convenience method to set a property
     * @param name
     *            - the preference name
     * @param value
     *            - the property value or null to delete the property
     * @throws CoreException
     */
    private void setProperty(String name, String value) throws CoreException {
        resource.setPersistentProperty(new QualifiedName(pageId, name), value);
    }

    /*** Get default values (Delegate to workbench store) ***/

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getDefaultBoolean(java.
     * lang.String)
     */
    public boolean getDefaultBoolean(String name) {
        return workbenchStore.getDefaultBoolean(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getDefaultDouble(java.lang
     * .String)
     */
    public double getDefaultDouble(String name) {
        return workbenchStore.getDefaultDouble(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getDefaultFloat(java.lang
     * .String)
     */
    public float getDefaultFloat(String name) {
        return workbenchStore.getDefaultFloat(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getDefaultInt(java.lang
     * .String)
     */
    public int getDefaultInt(String name) {
        return workbenchStore.getDefaultInt(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getDefaultLong(java.lang
     * .String)
     */
    public long getDefaultLong(String name) {
        return workbenchStore.getDefaultLong(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getDefaultString(java.lang
     * .String)
     */
    public String getDefaultString(String name) {
        return workbenchStore.getDefaultString(name);
    }

    /*** Get property values ***/

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getBoolean(java.lang.String
     * )
     */
    public boolean getBoolean(String name) {
        insertValue(name);
        return super.getBoolean(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getDouble(java.lang.String)
     */
    public double getDouble(String name) {
        insertValue(name);
        return super.getDouble(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getFloat(java.lang.String)
     */
    public float getFloat(String name) {
        insertValue(name);
        return super.getFloat(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getInt(java.lang.String)
     */
    public int getInt(String name) {
        insertValue(name);
        return super.getInt(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getLong(java.lang.String)
     */
    public long getLong(String name) {
        insertValue(name);
        return super.getLong(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#getString(java.lang.String)
     */
    public String getString(String name) {
        insertValue(name);
        return super.getString(name);
    }

    /**
     * @param name
     */
    private synchronized void insertValue(String name) {
        if (inserting)
            return;
        if (super.contains(name))
            return;
        inserting = true;
        String prop = null;
        try {
            prop = getProperty(name);
        } catch (CoreException e) {
        }
        if (prop == null)
            prop = workbenchStore.getString(name);
        if (prop != null)
            setValue(name, prop);
        inserting = false;
    }

    /**
     * Convenience method to fetch a property
     * @param name
     *            - the preference name
     * @return - the property value
     * @throws CoreException
     */
    private String getProperty(String name) throws CoreException {
        return resource.getPersistentProperty(new QualifiedName(pageId, name));
    }

    /*** Misc ***/

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#contains(java.lang.String)
     */
    public boolean contains(String name) {
        return workbenchStore.contains(name);
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#setToDefault(java.lang.
     * String)
     */
    public void setToDefault(String name) {
        setValue(name, getDefaultString(name));
    }

    /*
     * (non-Javadoc)
     * @see
     * org.eclipse.jface.preference.IPreferenceStore#isDefault(java.lang.String)
     */
    public boolean isDefault(String name) {
        String defaultValue = getDefaultString(name);
        if (defaultValue == null)
            return false;
        return defaultValue.equals(getString(name));
    }

}
