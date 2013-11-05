/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.preferences;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.jface.preference.IPreferenceStore;
import org.franca.core.franca.FModel;

public class FPreferences {

    private static FPreferences instance = null;
    private Map<IResource, Map<String, String>> preferences = null;

    private FPreferences() {
        preferences = new HashMap<IResource, Map<String, String>>();
    }
    
    public void resetPreferences(){
        preferences.clear();
    }

    public static FPreferences getInstance() {
        if (instance == null) {
            return instance = new FPreferences();
        }
        return instance;
    }

    public void addPreferences(IResource res) {
        Map<String, String> map = new HashMap<String, String>();
        
        if (res != null) {
            try {
                QualifiedName useProjectSettingsIdentifier = new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.USEPROJECTSETTINGS);
                map.put(PreferenceConstants.USEPROJECTSETTINGS, res.getPersistentProperty(useProjectSettingsIdentifier));

                QualifiedName outputPathIdentifier = new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT);
                map.put(PreferenceConstants.P_OUTPUT, res.getPersistentProperty(outputPathIdentifier));

                QualifiedName licenseIdentifier = new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_LICENSE);
                map.put(PreferenceConstants.P_LICENSE, res.getPersistentProperty(licenseIdentifier));

                QualifiedName generateStubsIdentifier = new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATESTUB);
                map.put(PreferenceConstants.P_GENERATESTUB, res.getPersistentProperty(generateStubsIdentifier));

                QualifiedName generateProxiesIdentifier = new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATEPROXY);
                map.put(PreferenceConstants.P_GENERATEPROXY, res.getPersistentProperty(generateProxiesIdentifier));

            } catch (CoreException e) {
                e.printStackTrace();
            }

        } else {
            if(!preferences.get(null).containsKey(PreferenceConstants.USEPROJECTSETTINGS))
                map.put(PreferenceConstants.USEPROJECTSETTINGS, Boolean.FALSE.toString());
            if(!preferences.get(null).containsKey(PreferenceConstants.P_OUTPUT))
                map.put(PreferenceConstants.P_OUTPUT, PreferenceConstants.DEFAULT_OUTPUT);
            if(!preferences.get(null).containsKey(PreferenceConstants.P_LICENSE))
                map.put(PreferenceConstants.P_LICENSE, PreferenceConstants.DEFAULT_LICENSE);
            if(!preferences.get(null).containsKey(PreferenceConstants.P_GENERATESTUB))
                map.put(PreferenceConstants.P_GENERATESTUB, Boolean.TRUE.toString());
            if(!preferences.get(null).containsKey(PreferenceConstants.P_GENERATEPROXY))
                map.put(PreferenceConstants.P_GENERATEPROXY, Boolean.TRUE.toString());
            map.putAll(preferences.get(null));
        }
        preferences.put(res, map);
    }

    public String getPreference(IResource res, String preferencename, String defaultValue) {
        Map<String, String> map = getPreferencesForResource(res);
        if (map != null
                        && map.get(preferencename) != null
                        && map.get(PreferenceConstants.USEPROJECTSETTINGS).equals(Boolean.TRUE.toString())) {
            return map.get(preferencename);
        }
        if (res instanceof IFile) {
            return getPreference(res.getProject(), preferencename, defaultValue);
        }
        return defaultValue;
    }

    public boolean useModelSpecific(IResource res) {
        Map<String, String> map = getPreferencesForResource(res);
        return map.get(PreferenceConstants.USEPROJECTSETTINGS) != null
               && map.get(PreferenceConstants.USEPROJECTSETTINGS).equals(Boolean.TRUE.toString());
    }

    private Map<String, String> getPreferencesForResource(IResource res) {
        return preferences.get(res);
    }

    public void setPreferences(String name, File file) throws IOException {
        Reader r = null;
        try {
            r = new FileReader(file);
            setPreference(name, r, "");
        } finally {
            try {
                r.close();
            } catch (IOException e) {
            }
        }
    }

    public void setPreference(String name, String preference) {
        if(preferences.get(null) == null) {
            preferences.put(null, new HashMap<String, String>());
        }
        preferences.get(null).put(name, preference);
    }

    public void setPreference(String name, Reader inreader, String path) throws IOException {
        BufferedReader reader = new BufferedReader(inreader);
        StringBuilder builder = new StringBuilder();
        String line = "";
        while ((line = reader.readLine()) != null) {
            builder.append(line + "\n");
        }
        if (preferences.get(null) == null)
            preferences.put(null, new HashMap<String, String>());
        preferences.get(null).put(name, builder.toString());
    }

    public String getModelPath(FModel model) {
        String ret = model.eResource().getURI().toString();
        return ret;
    }

    public static void init(IPreferenceStore store, IPreferenceStore defaultstore, IProject project) {
        if (instance == null) {
            instance = new FPreferences();
        }
        String useProjectSettingsString = store.getString(PreferenceConstants.USEPROJECTSETTINGS);
        boolean useProjectSettings = useProjectSettingsString.equals(Boolean.toString(true));
        if (useProjectSettings) {
            if (instance.preferences.get(project) == null) {
                instance.preferences.put(project, new HashMap<String, String>());
            }
            instance.preferences.get(project).put(PreferenceConstants.USEPROJECTSETTINGS, Boolean.toString(true));

            String outputFolder = store.getString(PreferenceConstants.P_OUTPUT);
            instance.preferences.get(project).put(PreferenceConstants.P_OUTPUT, outputFolder);

            String licenseHeader = store.getString(PreferenceConstants.P_LICENSE);
            instance.preferences.get(project).put(PreferenceConstants.P_LICENSE, licenseHeader);

            String generateProxy = store.getString(PreferenceConstants.P_GENERATEPROXY);
            instance.preferences.get(project).put(PreferenceConstants.P_GENERATEPROXY, generateProxy);

            String generatStub = store.getString(PreferenceConstants.P_GENERATESTUB);
            instance.preferences.get(project).put(PreferenceConstants.P_GENERATESTUB, generatStub);
        } else {
            if (instance.preferences.get(project) == null) {
                instance.preferences.put(project, new HashMap<String, String>());
            }
            instance.preferences.get(project).put(PreferenceConstants.P_OUTPUT,
                    defaultstore.getString(PreferenceConstants.P_OUTPUT));
            instance.preferences.get(project).put(PreferenceConstants.P_LICENSE,
                    defaultstore.getString(PreferenceConstants.P_LICENSE));
            instance.preferences.get(project).put(PreferenceConstants.P_GENERATEPROXY,
                    defaultstore.getString(PreferenceConstants.P_GENERATEPROXY));
            instance.preferences.get(project).put(PreferenceConstants.P_GENERATESTUB,
                    defaultstore.getString(PreferenceConstants.P_GENERATESTUB));
        }
    }

}
