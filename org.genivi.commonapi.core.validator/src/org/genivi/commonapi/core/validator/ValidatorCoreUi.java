/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.validator;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map.Entry;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.xtext.validation.ValidationMessageAcceptor;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FrancaPackage;
import org.franca.core.franca.Import;
import org.genivi.commonapi.core.verification.ValidateElements;
import org.genivi.commonapi.core.verification.ValidatorCore;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;

/**
 * This validator is automatically triggered from the XText editor.
 */
public class ValidatorCoreUi extends ValidatorCore {

    private HashMap<String, HashSet<String>> importList = new HashMap<String, HashSet<String>>();
    private Boolean hasChanged = false;
    private ValidateElements validateElements = new ValidateElements();

    @Override
    public void validateModel(FModel model,
            ValidationMessageAcceptor messageAcceptor) {
        try {
            if (!isValidatorEnabled()) {
                return;
            }

            validateElements.verifyEqualInOutAndAddSuffix(model);

            // call the super validation method
            super.validateModel(model, messageAcceptor);

            Resource res = model.eResource();
            IPath filePath;
            if (res.getURI().isPlatform())
            {
                final Path platformPath = new Path(res.getURI().toPlatformString(true));
                IFile file = ResourcesPlugin.getWorkspace().getRoot()
                        .getFile(platformPath);
                filePath = file.getLocation();
            }
            else
            {
                filePath = new Path(res.getURI().toFileString());
            }
            String cwd = filePath.removeLastSegments(1).toString();
            validateImport(model, messageAcceptor, filePath, cwd);
        }
        catch (Exception ex) {
            ex.printStackTrace();
            throw ex;
        }
        finally {
            importList.clear();
            hasChanged = false;
        }
    }

    protected void validateImport(FModel model,
            ValidationMessageAcceptor messageAcceptor,
            IPath filePath, String cwd) {
        String fileName = filePath.lastSegment();
        HashSet<String> importedFiles = new HashSet<String>();
        ArrayList<String> importUriAndNamesspace = new ArrayList<String>();
        for (Import fImport : model.getImports()) {

            if (importUriAndNamesspace.contains(fImport.getImportURI() + ","
                    + fImport.getImportedNamespace()))
                acceptWarning("Multiple times imported!", fImport,
                        FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
                        messageAcceptor);
            if (fImport.getImportURI().equals(fileName)) {
                acceptError("Class may not import itself!", fImport,
                        FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
                        messageAcceptor);
            } else {
                Path absoluteImportPath = new Path(fImport.getImportURI());
                if (!absoluteImportPath.isAbsolute()) {
                    absoluteImportPath = new Path(cwd + "/"
                            + fImport.getImportURI());
                    importedFiles.add(absoluteImportPath.toString());
                } else {
                    importedFiles.add(absoluteImportPath.toString()
                            .replaceFirst(absoluteImportPath.getDevice() + "/",
                                    ""));
                }

            }
            importUriAndNamesspace.add(fImport.getImportURI() + ","
                    + fImport.getImportedNamespace());
        }
        importUriAndNamesspace.clear();
        importList.put(filePath.toString(), importedFiles);

        ArrayList<String> start = new ArrayList<String>();
        try {
            importList = buildImportList(importList);
        } catch (NullPointerException e) {
        }
        start.add(filePath.toString());
        for (Import fImport : model.getImports()) {
            Path importPath = new Path(fImport.getImportURI());
            if (importPath.isAbsolute()) {
                findCyclicImports(
                        importPath.toString().replaceFirst(
                                importPath.getDevice() + "/", ""),
                        filePath.toString(), start, fImport, messageAcceptor);
            } else {
                importPath = new Path(cwd + "/" + fImport.getImportURI());
                findCyclicImports(importPath.toString(), filePath.toString(),
                        start, fImport, messageAcceptor);
            }
        }
        start.clear();
    }

    private HashMap<String, HashSet<String>> buildImportList(
            HashMap<String, HashSet<String>> rekImportList) {
        HashMap<String, HashSet<String>> helpMap = new HashMap<String, HashSet<String>>();
        for (Entry<String, HashSet<String>> entry : rekImportList.entrySet()) {
            for (String importedPath : entry.getValue()) {
                if (!rekImportList.containsKey(importedPath)) {
                    hasChanged = true;
                    HashSet<String> importedFIDL = new HashSet<String>();
                    EObject resource = null;
                    resource = buildResource(
                            importedPath.substring(
                                    importedPath.lastIndexOf("/") + 1,
                                    importedPath.length()),
                            "file:/"
                                    + importedPath.substring(0,
                                            importedPath.lastIndexOf("/") + 1));
                    if (resource != null)
                    {
                        for (EObject imp : resource.eContents()) {
                            if (imp instanceof Import) {
                                Path importImportedPath = new Path(
                                        ((Import) imp).getImportURI());
                                if (importImportedPath.isAbsolute()) {
                                    importedFIDL.add(importImportedPath.toString()
                                            .replaceFirst(
                                                    importImportedPath.getDevice()
                                                            + "/", ""));
                                } else {
                                    importImportedPath = new Path(
                                            importedPath.substring(0,
                                                    importedPath.lastIndexOf("/"))
                                                    + "/"
                                                    + ((Import) imp).getImportURI());
                                    importedFIDL.add(importImportedPath.toString());
                                }
                            }
                        }
                    }
                    helpMap.put(importedPath, importedFIDL);
                }
            }
        }
        if (hasChanged) {
            hasChanged = false;
            helpMap.putAll(rekImportList);
            return buildImportList(helpMap);
        } else {
            return rekImportList;
        }
    }


    protected void findCyclicImports(String filePath, String prevFilePath,
            ArrayList<String> cyclicList, Import imp,
            ValidationMessageAcceptor messageAcceptor) {
        if (cyclicList.contains(filePath)) {
            String errorString = "";
            for (String impString : cyclicList) {
                errorString = errorString + impString + "->";
            }
            if (prevFilePath.equals(filePath)) {
                if (cyclicList.size() > 1)
                    acceptError("Last file imports itself!: " + errorString
                            + filePath, imp,
                            FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
                            messageAcceptor);
                return;
            }
            //Commented out because the new catalogue allows cyclic imports, error disabled
            /*
            else {
                acceptError("Cyclic Imports: " + errorString + filePath, imp,
                        FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
                        messageAcceptor);
                return;
            }*/
        } else {
            cyclicList.add(filePath);
            if (importList.containsKey(filePath)) {
                for (String importPath : importList.get(filePath)) {
                    findCyclicImports(importPath, filePath, cyclicList, imp,
                            messageAcceptor);
                }
            }
            cyclicList.remove(cyclicList.size() - 1);
        }
    }


    /**
     * Check whether the validation is enabled in the eclipse preferences
     * @return
     */
    public boolean isValidatorEnabled() {

    	IPreferenceStore prefs = CommonApiUiPlugin.getValidatorPreferences();
    	return prefs != null && prefs.getBoolean(PreferenceConstants.P_ENABLE_CORE_VALIDATOR);
    }
}
