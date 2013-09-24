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
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.genivi.commonapi.core.generator.FTypeCycleDetector;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.ResourcesPlugin;

import org.eclipse.ui.IWorkbench;
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
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;

import com.google.inject.Guice;

public class ResourceValidator implements IFrancaExternalValidator {

    private FTypeCycleDetector cycleDetector;
    private ResourceSet resourceSet;
    private HashMap<String, HashSet<String>> importList = new HashMap<String, HashSet<String>>();
    private Boolean hasChanged = false;
    private AllInfoMapsBuilder aimBuilder = new AllInfoMapsBuilder();
    private Map<String, HashMap<String, HashSet<String>>> fastAllInfo = new HashMap<String, HashMap<String, HashSet<String>>>();
    private CppKeywords cppKeyWords = new CppKeywords();

    @Override
    public void validateModel(FModel model,
            ValidationMessageAcceptor messageAcceptor) {
        cycleDetector = Guice.createInjector().getInstance(
                FTypeCycleDetector.class);
        resourceSet = new ResourceSetImpl();
        Resource res = model.eResource();
        URI uri = res.getURI();
        String projectName = uri.segment(1);

        final Path platformPath = new Path(res.getURI().toPlatformString(true));
        final IFile file = ResourcesPlugin.getWorkspace().getRoot()
                .getFile(platformPath);
        IPath filePath = file.getLocation();
        String cwd = filePath.removeLastSegments(1).toString();
        String cutCwd = cwd.substring(0, cwd.lastIndexOf(projectName)
                + projectName.length());

        if (aimBuilder.buildAllInfos(cutCwd)) {
            fastAllInfo = aimBuilder.fastAllInfo;
        } else {
            if (!uri.segment(2).toString().equals("bin"))
                aimBuilder.updateAllInfo((EObject) model, filePath.toString());
            fastAllInfo = aimBuilder.fastAllInfo;
        }

        HashSet<String> importedFiles = new HashSet<String>();
        ArrayList<String> importUriAndNamesspace = new ArrayList<String>();
        for (Import fImport : model.getImports()) {

            if (importUriAndNamesspace.contains(fImport.getImportURI() + ","
                    + fImport.getImportedNamespace()))
                messageAcceptor.acceptWarning("Multiple times imported!",
                        fImport, FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
                        null);
            if (fImport.getImportURI().equals(file.getName())) {
                messageAcceptor.acceptError("Class may not import itself!",
                        fImport, FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
                        null);
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
        List<String> interfaceTypecollectionNames = new ArrayList<String>();
        for (FTypeCollection fTypeCollection : model.getTypeCollections()) {
            for(Entry<String, Triple<String, ArrayList<String>, ArrayList<String>>> entry :aimBuilder.allInfo.entrySet()){
                if(!entry.getKey().equals(cwd+"/"+file.getName())){
                    if(entry.getValue().packageName.startsWith(model.getName()+"."+fTypeCollection.getName())){
                        if(importList.get(cwd+"/"+file.getName()).contains(entry.getKey())){
                            messageAcceptor.acceptError(
                                    "Imported file's package "
                                            + entry.getValue().packageName
                                            + " may not start with package "
                                            + model.getName() + " + TypeCollectionName "
                                            + fTypeCollection.getName(), fTypeCollection,
                                    null, -1, null);
                        }else{
                            messageAcceptor.acceptWarning(
                                    entry.getKey()+". File's package "
                                            + entry.getValue().packageName
                                            + " starts with package "
                                            + model.getName() + " + TypeCollectionName "
                                            + fTypeCollection.getName(), fTypeCollection,
                                    null, -1, null);
                        }
                    }
                }
            }
        }
        HashMap<FInterface,EList<FInterface>> managedInterfaces = new HashMap<FInterface,EList<FInterface>>();
        for (FInterface fInterface : model.getInterfaces()) {
            managedInterfaces.put(fInterface, fInterface.getManagedInterfaces());
            for(Entry<String, Triple<String, ArrayList<String>, ArrayList<String>>> entry :aimBuilder.allInfo.entrySet()){
                if(!entry.getKey().equals(cwd+"/"+file.getName())){
                    if(entry.getValue().packageName.startsWith(model.getName()+"."+fInterface.getName())){
                        if(importList.get(cwd+"/"+file.getName()).contains(entry.getKey())){
                            messageAcceptor.acceptError(
                                    "Imported file's package "
                                            + entry.getValue().packageName
                                            + " may not start with package "
                                            + model.getName() + " + InterfaceName "
                                            + fInterface.getName(), fInterface,
                                    null, -1, null);
                        }else{
                            messageAcceptor.acceptWarning(
                                    entry.getKey()+". File's package "
                                            + entry.getValue().packageName
                                            + " starts with package "
                                            + model.getName() + " + InterfaceName "
                                            + fInterface.getName(), fInterface,
                                    null, -1, null);
                        }
                    }
                }
            }
            
        }
        importUriAndNamesspace.clear();
        for (FTypeCollection fTypeCollection : model.getTypeCollections()) {
            validateName(fTypeCollection.getName(), messageAcceptor,
                    fTypeCollection);
            if (interfaceTypecollectionNames.indexOf(fTypeCollection.getName()) != interfaceTypecollectionNames
                    .lastIndexOf(fTypeCollection.getName())) {
                messageAcceptor.acceptError("Name " + fTypeCollection.getName()
                        + " isn't unique in this file!", fTypeCollection, null,
                        -1, null);
            }
            if (fastAllInfo.get(fTypeCollection.getName()).get(model.getName())
                    .size() > 1) {
                for (String s : fastAllInfo.get(fTypeCollection.getName()).get(
                        model.getName())) {
                    if (!s.equals(filePath.toString())) {
                        if (importList.containsKey(s)) {
                            messageAcceptor
                                    .acceptError(
                                            "Imported file "
                                                    + s
                                                    + " has interface or typeCollection with the same name and same package!",
                                            fTypeCollection, null, -1, null);
                        } else {
                            messageAcceptor
                                    .acceptWarning(
                                            "Interface or typeCollection in file "
                                                    + s
                                                    + " has the same name and same package!",
                                            fTypeCollection, null, -1, null);
                        }
                    }
                }

            }
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
                        if (fEnumerator.getValue() != null) {
                            String enumeratorValue = fEnumerator.getValue()
                                    .toLowerCase();
                            validateEnumerationValue(enumeratorValue,
                                    messageAcceptor, fEnumerator);
                        }
                    }
                }
            }
        }
        
        
      
        for (FInterface fInterface : model.getInterfaces()) {
            ArrayList<FInterface> startI = new ArrayList<FInterface>();
            int index = 0 ;
            startI.add(fInterface);
            ArrayList<FInterface> managedList = new ArrayList<FInterface>();
            for(FInterface managedInterface : fInterface.getManagedInterfaces()){
                findCyclicManagedInterfaces(managedInterface, startI , fInterface, messageAcceptor, index);
                if(managedList.contains(managedInterface))
                    messageAcceptor.acceptError("Interface "+ managedInterface.getName() +" is already managed! Delete this equivalent!", fInterface, FrancaPackage.Literals.FINTERFACE__MANAGED_INTERFACES, index, null);
                managedList.add(managedInterface);
                index++;
            }
            
            
            validateName(fInterface.getName(), messageAcceptor, fInterface);
            if (interfaceTypecollectionNames.indexOf(fInterface.getName()) != interfaceTypecollectionNames
                    .lastIndexOf(fInterface.getName())) {
                messageAcceptor.acceptError("Name " + fInterface.getName()
                        + " isn't unique in this file!", fInterface, null, -1,
                        null);
            }
            try {
                if (fastAllInfo.get(fInterface.getName()).get(model.getName())
                        .size() > 1) {
                    for (String s : fastAllInfo.get(fInterface.getName()).get(
                            model.getName())) {
                        if (!s.equals(filePath.toString()))
                            if (importList.containsKey(s)) {
                                messageAcceptor
                                        .acceptError(
                                                "Imported file "
                                                        + s
                                                        + " has interface or typeCollection with the same name and same package!",
                                                fInterface, null, -1, null);
                            } else {
                                messageAcceptor
                                        .acceptWarning(
                                                "Interface or typeCollection in file "
                                                        + s
                                                        + " has the same name and same package!",
                                                fInterface, null, -1, null);
                            }
                    }
                }
            } catch (Exception e) {
            }

            if (fInterface.getVersion() == null)
                messageAcceptor.acceptError(
                        "Missing version! Add: version(major int minor int)",
                        fInterface, FrancaPackage.Literals.FINTERFACE__BASE,
                        -1, null);

            for (FAttribute att : fInterface.getAttributes()) {
                validateName(att.getName(), messageAcceptor, att);
                if (att.getType().getPredefined().toString() == "undefined") {
                    try {
                        if (cycleDetector.hasCycle(att.getType().getDerived())) {

                            messageAcceptor
                                    .acceptError(
                                            "Cyclic dependencie: "
                                                    + cycleDetector.outErrorString,
                                            att,
                                            FrancaPackage.Literals.FATTRIBUTE__NO_SUBSCRIPTIONS,
                                            -1, null);
                        }
                    } catch (NullPointerException npe) {
                        messageAcceptor
                                .acceptError(
                                        "Derives from an undefined Type!",
                                        att,
                                        FrancaPackage.Literals.FATTRIBUTE__NO_SUBSCRIPTIONS,
                                        -1, null);
                    }
                }
            }
            int count = 0;
            for (FBroadcast fBroadcast : fInterface.getBroadcasts()) {
                validateName(fBroadcast.getName(), messageAcceptor, fBroadcast);
                for (FArgument out : fBroadcast.getOutArgs()) {
                    validateName(out.getName(), messageAcceptor, out);
                    if (out.getType().getPredefined().toString() == "undefined") {
                        try {
                            if (cycleDetector.hasCycle(out.getType()
                                    .getDerived())) {
                                messageAcceptor
                                        .acceptError(
                                                "Cyclic dependencie: "
                                                        + cycleDetector.outErrorString,
                                                fBroadcast,
                                                FrancaPackage.Literals.FBROADCAST__OUT_ARGS,
                                                count, null);
                            }
                        } catch (NullPointerException npe) {
                            messageAcceptor
                                    .acceptError(
                                            "Derives from an undefined Type!",
                                            fBroadcast,
                                            FrancaPackage.Literals.FBROADCAST__OUT_ARGS,
                                            count, null);
                        }
                    }
                    count += 1;
                }
                count = 0;
            }
            count = 0;
            for (FMethod fMethod : fInterface.getMethods()) {
                validateName(fMethod.getName(), messageAcceptor, fMethod);
                for (FArgument out : fMethod.getOutArgs()) {
                    validateName(out.getName(), messageAcceptor, out);
                    if (out.getType().getPredefined().toString() == "undefined") {
                        try {
                            if (cycleDetector.hasCycle(out.getType()
                                    .getDerived())) {
                                messageAcceptor
                                        .acceptError(
                                                "Cyclic dependencie: "
                                                        + cycleDetector.outErrorString,
                                                fMethod,
                                                FrancaPackage.Literals.FMETHOD__OUT_ARGS,
                                                count, null);
                            }
                        } catch (NullPointerException npe) {
                            messageAcceptor.acceptError(
                                    "Derives from an undefined Type!", fMethod,
                                    FrancaPackage.Literals.FMETHOD__OUT_ARGS,
                                    count, null);
                        }
                    }
                    count += 1;
                }
                count = 0;
                for (FArgument in : fMethod.getInArgs()) {
                    validateName(in.getName(), messageAcceptor, in);
                    if (in.getType().getPredefined().toString() == "undefined") {
                        try {
                            if (cycleDetector.hasCycle(in.getType()
                                    .getDerived())) {
                                messageAcceptor
                                        .acceptError(
                                                "Cyclic dependencie: "
                                                        + cycleDetector.outErrorString,
                                                fMethod,
                                                FrancaPackage.Literals.FMETHOD__IN_ARGS,
                                                count, null);
                            }
                        } catch (NullPointerException npc) {
                            messageAcceptor.acceptError(
                                    "Derives from an undefined Type!", fMethod,
                                    FrancaPackage.Literals.FMETHOD__IN_ARGS,
                                    count, null);
                        }
                    }
                    count += 1;
                }
                count = 0;
            }

        }

        interfaceTypecollectionNames.clear();
        importList.clear();

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
                    messageAcceptor
                            .acceptError("Last file imports itself!: "
                                    + errorString + filePath, imp,
                                    FrancaPackage.Literals.IMPORT__IMPORT_URI,
                                    -1, null);
                return;
            } else {
                messageAcceptor.acceptError("Cyclic Imports: " + errorString
                        + filePath, imp,
                        FrancaPackage.Literals.IMPORT__IMPORT_URI, -1, null);
                return;
            }
        } else {
            cyclicList.add(filePath);
            for (String importPath : importList.get(filePath)) {
                findCyclicImports(importPath, filePath, cyclicList, imp,
                        messageAcceptor);
            }
            cyclicList.remove(cyclicList.size() - 1);
        }
    }
    private void findCyclicManagedInterfaces(FInterface rekInterface, ArrayList<FInterface> interfaceList, FInterface fInterface, ValidationMessageAcceptor messageAcceptor, int index){
        if(interfaceList.contains(rekInterface)){
            String errorString="";
            for(FInterface a : interfaceList ){
                errorString=errorString + a.getName()+" -> ";
            }
            errorString = errorString + rekInterface.getName();
            messageAcceptor.acceptError("Cycle detected: " +errorString, fInterface, FrancaPackage.Literals.FINTERFACE__MANAGED_INTERFACES, index, null);
        }else{
            interfaceList.add(rekInterface);
            for(FInterface nextINterface :rekInterface.getManagedInterfaces()){
                findCyclicManagedInterfaces(nextINterface, interfaceList, fInterface, messageAcceptor, index);
            }
            interfaceList.remove(rekInterface);
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

    private void validateEnumerationValue(String enumeratorValue,
            ValidationMessageAcceptor messageAcceptor, FEnumerator fEnumerator) {
        String value = enumeratorValue;
        if (value.length() == 0) {
            messageAcceptor.acceptWarning("Missing value!", fEnumerator,
                    FrancaPackage.Literals.FENUMERATOR__VALUE, -1, null);
            return;
        }

        if (value.length() == 1) {
            if (48 > (int) value.charAt(0) || (int) value.charAt(0) > 57) {
                messageAcceptor.acceptWarning("Not a valid number!",
                        fEnumerator, FrancaPackage.Literals.FENUMERATOR__VALUE,
                        -1, null);
                return;
            }
        }

        if (value.length() > 2) {
            if (value.charAt(0) == '0') {
                // binary
                if (value.charAt(1) == 'b') {
                    for (int i = 2; i < value.length(); i++) {
                        if (value.charAt(i) != '0' && value.charAt(i) != '1') {
                            messageAcceptor.acceptWarning(
                                    "Not a valid number! Should be binary",
                                    fEnumerator,
                                    FrancaPackage.Literals.FENUMERATOR__VALUE,
                                    -1, null);
                            return;
                        }
                    }
                    return;
                }
                // hex
                if (value.charAt(1) == 'x') {
                    for (int i = 2; i < value.length(); i++) {
                        if ((48 > (int) value.charAt(i) || (int) value
                                .charAt(i) > 57)
                                && (97 > (int) value.charAt(i) || (int) value
                                        .charAt(i) > 102)) {
                            messageAcceptor
                                    .acceptWarning(
                                            "Not a valid number! Should be hexadecimal",
                                            fEnumerator,
                                            FrancaPackage.Literals.FENUMERATOR__VALUE,
                                            -1, null);
                            return;
                        }
                    }
                    return;
                }
            }
        }
        if (value.charAt(0) == '0') {
            // oct
            for (int i = 1; i < value.length(); i++) {
                if (48 > (int) value.charAt(i) || (int) value.charAt(i) > 55) {
                    messageAcceptor
                            .acceptWarning(
                                    "Not a valid number! Should be octal",
                                    fEnumerator,
                                    FrancaPackage.Literals.FENUMERATOR__VALUE,
                                    -1, null);
                    return;
                }
            }
            return;
        }
        // dec
        for (int i = 0; i < value.length(); i++) {
            if (48 > (int) value.charAt(i) || (int) value.charAt(i) > 57) {
                messageAcceptor.acceptWarning(
                        "Not a valid number! Should be decimal", fEnumerator,
                        FrancaPackage.Literals.FENUMERATOR__VALUE, -1, null);
                return;
            }
        }
    }

    private void validateName(String name,
            ValidationMessageAcceptor messageAcceptor, EObject eObject) {
        if (cppKeyWords.keyWords.contains(name)) {
            messageAcceptor.acceptError(
                    "Name " + name + " is a keyword in c++", eObject, null, -1,
                    null);
            return;
        }
        if (eObject instanceof FTypeCollection) {
            if (name.indexOf(".") != -1) {
                messageAcceptor.acceptError("Name may not contain '.'",
                        eObject, null, -1, null);
            }
        }
    }
}
