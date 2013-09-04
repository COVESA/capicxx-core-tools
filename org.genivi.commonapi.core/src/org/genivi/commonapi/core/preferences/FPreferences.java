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
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.preference.IPreferenceStore;
import org.franca.core.franca.FModel;

public class FPreferences {

    private static FPreferences instance = null;
    private Map<String, Map<String, String>> preferences = null;

    private FPreferences() {
        preferences = new HashMap<String, Map<String, String>>();
        preferences.put("default", new HashMap<String, String>());
        preferences.get("default").put(PreferenceConstants.USEPROJECTSETTINGS, Boolean.toString(false));
    }

    public static FPreferences getInstance() {
        if (instance == null)
            return instance = new FPreferences();
        return instance;
    }

    public void setPreferences(String name, File file, String path) throws IOException {
        Reader r = null;
        try {
            r = new FileReader(file);
            setPreference(name, r, path);
        } finally {
            try {
                r.close();
            } catch (IOException e) {
                ;
            }
        }
    }

    public void setPreference(String name, String preference, FModel model) {
        getPreferencesForPathString(getModelPath(model), true).put(name, preference);
    }

    public void setPreference(String name, String preference, String path) {
        getPreferencesForPathString(path, true).put(name, preference);
    }

    private Map<String, String> getPreferencesForPathString(String path, boolean setter) {
        Map<String, String> ret = null;
        for (String key : preferences.keySet()) {
            if (path.contains(key))
                return preferences.get(key);
        }
        if (setter) {
            preferences.put(path, new HashMap<String, String>());
            return preferences.get(path);
        }
        return ret;
    }

    public void setPreference(String name, InputStream in, String path) throws IOException {
        Reader r = null;
        try {
            r = new InputStreamReader(in);
            setPreference(name, r, path);
        } finally {
            try {
                r.close();
            } catch (IOException e) {
                ;
            }
        }
    }

    public void setPreference(String name, Reader inreader, String path) throws IOException {
        BufferedReader reader = new BufferedReader(inreader);
        StringBuilder builder = new StringBuilder();
        String line = "";
        while ((line = reader.readLine()) != null) {
            builder.append(line + "\n");
        }
        getPreferencesForPathString(path, true).put(name, builder.toString());
    }

    private String getModelPath(FModel model) {
        String ret = model.eResource().getURI().toString();
        return ret;
    }

    public String getPreference(String name, String defaultret, FModel model) {
        if (getPreferencesForPathString(getModelPath(model), false) != null
                && getPreferencesForPathString(getModelPath(model), false).get(PreferenceConstants.USEPROJECTSETTINGS)
                        .equals(Boolean.toString(true)))
            return getPreferencesForPathString(getModelPath(model), false).get(name);
        return defaultret;
    }

    public String getPreference(String name, String defaultret, String path) {
        if (getPreferencesForPathString(path, false) != null
                && getPreferencesForPathString(path, false).get(PreferenceConstants.USEPROJECTSETTINGS).equals(
                        Boolean.toString(true)))
            return getPreferencesForPathString(path, false).get(name);
        return defaultret;
    }

    public static void init(IPreferenceStore store, IPreferenceStore defaultstore, IProject project) {
        String propath = project.getFullPath().toPortableString();
        if (instance == null)
            instance = new FPreferences();
        if (store.getString(PreferenceConstants.USEPROJECTSETTINGS).equals(Boolean.toString(true))) {
            if (instance.preferences.get(propath) == null)
                instance.preferences.put(propath, new HashMap<String, String>());
            instance.preferences.get(propath).put(PreferenceConstants.USEPROJECTSETTINGS, Boolean.toString(true));
            instance.preferences.get(propath).put(PreferenceConstants.P_OUTPUT,
                    store.getString(PreferenceConstants.P_OUTPUT));
            instance.preferences.get(propath).put(PreferenceConstants.P_LICENSE,
                    store.getString(PreferenceConstants.P_LICENSE));
        } else {
            if (instance.preferences.get(propath) == null)
                instance.preferences.put(propath, new HashMap<String, String>());
            instance.preferences.get(propath).put(PreferenceConstants.P_OUTPUT,
                    defaultstore.getString(PreferenceConstants.P_OUTPUT));
            instance.preferences.get(propath).put(PreferenceConstants.P_LICENSE,
                    defaultstore.getString(PreferenceConstants.P_LICENSE));
        }
    }

}
