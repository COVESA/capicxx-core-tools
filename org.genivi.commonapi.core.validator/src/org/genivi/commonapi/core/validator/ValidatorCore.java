/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.genivi.commonapi.core.validator;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map.Entry;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;
import org.eclipse.xtext.validation.ValidationMessageAcceptor;
import org.franca.core.dsl.validation.IFrancaExternalValidator;
import org.franca.core.franca.FArgument;
import org.franca.core.franca.FAttribute;
import org.franca.core.franca.FBroadcast;
import org.franca.core.franca.FEnumerationType;
import org.franca.core.franca.FEnumerator;
import org.franca.core.franca.FField;
import org.franca.core.franca.FInterface;
import org.franca.core.franca.FMethod;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FStructType;
import org.franca.core.franca.FType;
import org.franca.core.franca.FTypeCollection;
import org.franca.core.franca.FrancaPackage;
import org.franca.core.franca.Import;

public class ValidatorCore implements IFrancaExternalValidator {

    private CppKeywords cppKeywords = new CppKeywords();
    private HashMap<String, HashSet<String>> importList = new HashMap<String, HashSet<String>>();
    private Boolean hasChanged = false;
    private ResourceSet resourceSet;

    @Override
    public void validateModel(FModel model,
            ValidationMessageAcceptor messageAcceptor) {
        resourceSet = new ResourceSetImpl();
        Resource res = model.eResource();
        final Path platformPath = new Path(res.getURI().toPlatformString(true));
        final IFile file = ResourcesPlugin.getWorkspace().getRoot()
                .getFile(platformPath);
        IPath filePath = file.getLocation();
        String cwd = filePath.removeLastSegments(1).toString();
        validateImport(model, messageAcceptor, file, filePath, cwd);
        List<String> interfaceTypecollectionNames = new ArrayList<String>();
        for (FTypeCollection fTypeCollection : model.getTypeCollections()) {
            interfaceTypecollectionNames.add(fTypeCollection.getName());
        }
        HashMap<FInterface, EList<FInterface>> managedInterfaces = new HashMap<FInterface, EList<FInterface>>();
        for (FInterface fInterface : model.getInterfaces()) {
            interfaceTypecollectionNames.add(fInterface.getName());
            managedInterfaces
                    .put(fInterface, fInterface.getManagedInterfaces());
        }

        for (FTypeCollection fTypeCollection : model.getTypeCollections()) {
            validateTypeCollectionName(messageAcceptor,
                    interfaceTypecollectionNames, fTypeCollection);
            validateTypeCollectionElements(messageAcceptor, fTypeCollection);
        }

        for (FInterface fInterface : model.getInterfaces()) {
            validateTypeCollectionName(messageAcceptor,
                    interfaceTypecollectionNames, fInterface);
            validateManagedInterfaces(messageAcceptor, fInterface);
            validateFInterfaceElements(messageAcceptor, fInterface);
        }

        interfaceTypecollectionNames.clear();
        importList.clear();
    }

    private void validateManagedInterfaces(
            ValidationMessageAcceptor messageAcceptor, FInterface fInterface) {
        ArrayList<FInterface> startI = new ArrayList<FInterface>();
        int index = 0;
        startI.add(fInterface);
        ArrayList<FInterface> managedList = new ArrayList<FInterface>();
        for (FInterface managedInterface : fInterface.getManagedInterfaces()) {
            findCyclicManagedInterfaces(managedInterface, startI, fInterface,
                    messageAcceptor, index, false);
            if (managedList.contains(managedInterface))
                acceptError("Interface " + managedInterface.getName()
                        + " is already managed! Delete this equivalent!",
                        fInterface,
                        FrancaPackage.Literals.FINTERFACE__MANAGED_INTERFACES,
                        index, messageAcceptor);
            managedList.add(managedInterface);
            index++;
        }
    }

    private void findCyclicManagedInterfaces(FInterface rekInterface,
            ArrayList<FInterface> interfaceList, FInterface fInterface,
            ValidationMessageAcceptor messageAcceptor, int index,
            boolean managed) {
        if (interfaceList.contains(rekInterface)) {
            String errorString = "";
            for (FInterface a : interfaceList) {
                errorString = errorString + a.getName() + " -> ";
            }
            errorString = errorString + rekInterface.getName();
            if (rekInterface.equals(fInterface)) {
                acceptError("Interface " + fInterface.getName()
                        + " manages itself: " + errorString, fInterface,
                        FrancaPackage.Literals.FINTERFACE__MANAGED_INTERFACES,
                        index, messageAcceptor);
            } else {
                // if(!managed){
                acceptError("Cycle detected: " + errorString, fInterface,
                        FrancaPackage.Literals.FINTERFACE__MANAGED_INTERFACES,
                        index, messageAcceptor);
                // }
            }
            // acceptError("Cycle detected: " +errorString, fInterface,
            // FrancaPackage.Literals.FINTERFACE__MANAGED_INTERFACES, index,
            // messageAcceptor);
        } else {
            interfaceList.add(rekInterface);
            for (FInterface nextInterface : rekInterface.getManagedInterfaces()) {
                findCyclicManagedInterfaces(nextInterface, interfaceList,
                        fInterface, messageAcceptor, index, managed);
            }
            if (rekInterface.getBase() != null)
                findCyclicManagedInterfaces(rekInterface.getBase(),
                        interfaceList, fInterface, messageAcceptor, index, true);
            interfaceList.remove(rekInterface);
        }
    }

    private void validateImport(FModel model,
            ValidationMessageAcceptor messageAcceptor, final IFile file,
            IPath filePath, String cwd) {
        HashSet<String> importedFiles = new HashSet<String>();
        ArrayList<String> importUriAndNamesspace = new ArrayList<String>();
        for (Import fImport : model.getImports()) {

            if (importUriAndNamesspace.contains(fImport.getImportURI() + ","
                    + fImport.getImportedNamespace()))
                acceptWarning("Multiple times imported!", fImport,
                        FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
                        messageAcceptor);
            if (fImport.getImportURI().equals(file.getName())) {
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

    private EObject buildResource(String filename, String cwd) {
        URI fileURI = normalizeURI(URI.createURI(filename));
        URI cwdURI = normalizeURI(URI.createURI(cwd));
        Resource resource = null;

        if (cwd != null && cwd.length() > 0) {
            resourceSet
                    .getURIConverter()
                    .getURIMap()
                    .put(fileURI,
                            URI.createURI((cwdURI.toString() + "/" + fileURI
                                    .toString()).replaceAll("/+", "/")));
        }

        try {
            resource = resourceSet.getResource(fileURI, true);
            resource.load(Collections.EMPTY_MAP);
        } catch (RuntimeException e) {
            return null;
        } catch (IOException io) {
            return null;
        }

        return resource.getContents().get(0);
    }

    private static URI normalizeURI(URI path) {
        if (path.isFile()) {
            return URI.createURI(path.toString().replaceAll("\\\\", "/"));
        }
        return path;
    }

    private void validateTypeCollectionName(
            ValidationMessageAcceptor messageAcceptor,
            List<String> interfaceTypecollectionNames,
            FTypeCollection fTypeCollection) {
        validateName(fTypeCollection.getName(), messageAcceptor,
                fTypeCollection);
        if (interfaceTypecollectionNames.indexOf(fTypeCollection.getName()) != interfaceTypecollectionNames
                .lastIndexOf(fTypeCollection.getName())) {
            acceptError("Name " + fTypeCollection.getName()
                    + " isn't unique in this file!", fTypeCollection,
                    FrancaPackage.Literals.FMODEL_ELEMENT__NAME, -1,
                    messageAcceptor);
        }
    }

    private void findCyclicImports(String filePath, String prevFilePath,
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
            } else {
                acceptError("Cyclic Imports: " + errorString + filePath, imp,
                        FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
                        messageAcceptor);
                return;
            }
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

    private void validateTypeCollectionElements(
            ValidationMessageAcceptor messageAcceptor,
            FTypeCollection fTypeCollection) {
        for (FType fType : fTypeCollection.getTypes()) {
            validateName(fType.getName(), messageAcceptor, fType);
            if (fType instanceof FStructType) {
                for (FField fField : ((FStructType) fType).getElements()) {
                    validateName(fField.getName(), messageAcceptor, fField);
                }
            }

            if (fType instanceof FEnumerationType) {
                for (FEnumerator fEnumerator : ((FEnumerationType) fType)
                        .getEnumerators()) {
                    validateName(fEnumerator.getName(), messageAcceptor,
                            fEnumerator);
                }
            }
        }
    }

    private void validateFInterfaceElements(
            ValidationMessageAcceptor messageAcceptor, FInterface fInterface) {
        if (fInterface.getVersion() == null)
            acceptError("Missing version! Add: version(major int minor int)",
                    fInterface, FrancaPackage.Literals.FMODEL_ELEMENT__NAME,
                    -1, messageAcceptor);

        for (FAttribute att : fInterface.getAttributes()) {
            validateName(att.getName(), messageAcceptor, att);
        }
        for (FBroadcast fBroadcast : fInterface.getBroadcasts()) {
            validateName(fBroadcast.getName(), messageAcceptor, fBroadcast);
            for (FArgument out : fBroadcast.getOutArgs()) {
                validateName(out.getName(), messageAcceptor, out);
            }
        }
        for (FMethod fMethod : fInterface.getMethods()) {
            validateName(fMethod.getName(), messageAcceptor, fMethod);
            for (FArgument out : fMethod.getOutArgs()) {
                validateName(out.getName(), messageAcceptor, out);
            }
            for (FArgument in : fMethod.getInArgs()) {
                validateName(in.getName(), messageAcceptor, in);
            }
        }
    }

    private void validateName(String name,
            ValidationMessageAcceptor messageAcceptor, EObject eObject) {
        if (cppKeywords.keyWords.contains(name)) {
            acceptError("Name " + name + " is a keyword in c++", eObject,
                    FrancaPackage.Literals.FMODEL_ELEMENT__NAME, -1,
                    messageAcceptor);
            return;
        }
    }

    private void acceptError(String message, EObject object,
            EStructuralFeature feature, int index,
            ValidationMessageAcceptor messageAcceptor) {
        messageAcceptor.acceptError("Core validation: " + message, object,
                feature, index, null);
    }

    private void acceptWarning(String message, EObject object,
            EStructuralFeature feature, int index,
            ValidationMessageAcceptor messageAcceptor) {
        messageAcceptor.acceptWarning("Core validation: " + message, object,
                feature, index, null);
    }
}
