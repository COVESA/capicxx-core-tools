/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.verification;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

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
import org.franca.core.franca.FModelElement;
import org.franca.core.franca.FStructType;
import org.franca.core.franca.FType;
import org.franca.core.franca.FTypeCollection;
import org.franca.core.franca.FrancaPackage;
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions;
import org.osgi.framework.Version;

/**
 * This validator is used with the command line environment (it is not automatically triggered from the XText editor).
 * It is meant to be called before the actual code generation. The code generation may not be executed if this
 * validator detects errors.
 *
 */
public class ValidatorCore implements IFrancaExternalValidator {

	private CppKeywords cppKeywords = new CppKeywords();
    private HashMap<String, HashSet<String>> importList = new HashMap<String, HashSet<String>>();
    protected ResourceSet resourceSet = new ResourceSetImpl();

    @Override
    public void validateModel(FModel model,
            ValidationMessageAcceptor messageAcceptor) {

		// don't log the good case
    	//acceptInfo("model " + model.getName(), messageAcceptor);

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

    protected void validateManagedInterfaces(ValidationMessageAcceptor messageAcceptor, FInterface fInterface) {
        ArrayList<FInterface> startI = new ArrayList<FInterface>();
        int index = 0;
        startI.add(fInterface);
        ArrayList<FInterface> managedList = new ArrayList<FInterface>();
        for (FInterface managedInterface : fInterface.getManagedInterfaces()) {
        	// check that the interface name is not null
        	if(managedInterface.getName() == null) {
        		acceptError("unknown managed interface: " + managedInterface.getName(), fInterface,
                FrancaPackage.Literals.FINTERFACE__MANAGED_INTERFACES, index, messageAcceptor);
        	} else {
        		// don't log the good case
        	 	//acceptInfo("managed interface: " + managedInterface.getName(), messageAcceptor);
        	}
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

    /**
     * Find interfaces that manage itself
     *
     */
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

    // TODO: use it without workspace context
//    private void validateImport(FModel model,
//            ValidationMessageAcceptor messageAcceptor, final IFile file,
//            IPath filePath, String cwd) {
//        HashSet<String> importedFiles = new HashSet<String>();
//        ArrayList<String> importUriAndNamesspace = new ArrayList<String>();
//        for (Import fImport : model.getImports()) {
//        	acceptInfo("validateImport: " + fImport.getImportedNamespace());
//            if (importUriAndNamesspace.contains(fImport.getImportURI() + ","
//                    + fImport.getImportedNamespace()))
//                acceptWarning("Multiple times imported!", fImport,
//                        FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
//                        messageAcceptor);
//            if (fImport.getImportURI().equals(file.getName())) {
//                acceptError("Class may not import itself!", fImport,
//                        FrancaPackage.Literals.IMPORT__IMPORT_URI, -1,
//                        messageAcceptor);
//            } else {
//                Path absoluteImportPath = new Path(fImport.getImportURI());
//                if (!absoluteImportPath.isAbsolute()) {
//                    absoluteImportPath = new Path(cwd + "/"
//                            + fImport.getImportURI());
//                    importedFiles.add(absoluteImportPath.toString());
//                } else {
//                    importedFiles.add(absoluteImportPath.toString()
//                            .replaceFirst(absoluteImportPath.getDevice() + "/",
//                                    ""));
//                }
//
//            }
//            importUriAndNamesspace.add(fImport.getImportURI() + ","
//                    + fImport.getImportedNamespace());
//        }
//        importUriAndNamesspace.clear();
//        importList.put(filePath.toString(), importedFiles);
//
//        ArrayList<String> start = new ArrayList<String>();
//        try {
//            importList = buildImportList(importList);
//        } catch (NullPointerException e) {
//        }
//        start.add(filePath.toString());
//        for (Import fImport : model.getImports()) {
//            Path importPath = new Path(fImport.getImportURI());
//            if (importPath.isAbsolute()) {
//                findCyclicImports(
//                        importPath.toString().replaceFirst(
//                                importPath.getDevice() + "/", ""),
//                        filePath.toString(), start, fImport, messageAcceptor);
//            } else {
//                importPath = new Path(cwd + "/" + fImport.getImportURI());
//                findCyclicImports(importPath.toString(), filePath.toString(),
//                        start, fImport, messageAcceptor);
//            }
//        }
//        start.clear();
//    }



    protected EObject buildResource(String filename, String cwd) {
        URI fileURI = normalizeURI(URI.createURI(filename));
        URI cwdURI = normalizeURI(URI.createURI(cwd));
        Resource resource = null;

        if (cwd != null && cwd.length() > 0) {
            fileURI = URI.createURI((cwdURI.toString() + "/" + fileURI
                    .toString()).replaceAll("/+", "/"));
        }

        try {
            resource = resourceSet.getResource(fileURI, true);
            resource.load(Collections.EMPTY_MAP);
        } catch (RuntimeException e) {
            return null;
        } catch (IOException io) {
            return null;
        }

        try {
            return resource.getContents().get(0);
        } catch (Exception e) {
            return null;
        }
    }

    protected static URI normalizeURI(URI path) {
        if (path.isFile()) {
            return URI.createURI(path.toString().replaceAll("\\\\", "/"));
        }
        return path;
    }

    protected void validateTypeCollectionName(
            ValidationMessageAcceptor messageAcceptor,
            List<String> interfaceTypecollectionNames,
            FTypeCollection fTypeCollection) {
		// don't log the good case
    	//acceptInfo("type collection: " + fTypeCollection.getName(), messageAcceptor);

        validateName(messageAcceptor,
                fTypeCollection);
        if (!isFrancaVersionGreaterThan(0, 8, 9)) {
            if (interfaceTypecollectionNames.indexOf(fTypeCollection.getName()) != interfaceTypecollectionNames
                    .lastIndexOf(fTypeCollection.getName())) {
                acceptError("Name " + fTypeCollection.getName()
                        + " isn't unique in this file!", fTypeCollection,
                        FrancaPackage.Literals.FMODEL_ELEMENT__NAME, -1,
                        messageAcceptor);
            }
        }
    }

    protected void validateTypeCollectionElements(
            ValidationMessageAcceptor messageAcceptor,
            FTypeCollection fTypeCollection) {
        for (FType fType : fTypeCollection.getTypes()) {
            validateName(messageAcceptor, fType);
            if (fType instanceof FStructType) {
                for (FField fField : ((FStructType) fType).getElements()) {
                    validateName(messageAcceptor, fField);
                }
            }

            if (fType instanceof FEnumerationType) {
                for (FEnumerator fEnumerator : ((FEnumerationType) fType)
                        .getEnumerators()) {
                    validateName(messageAcceptor,
                            fEnumerator);
                }
            }
        }
    }

    protected void validateFInterfaceElements(
            ValidationMessageAcceptor messageAcceptor, FInterface fInterface) {
		// don't log the good case
    	//acceptInfo("interface elements: " + fInterface.getName(), messageAcceptor);
        if (fInterface.getVersion() == null)
            acceptError("Missing version! Add: version(major int minor int)",
                    fInterface, FrancaPackage.Literals.FMODEL_ELEMENT__NAME,
                    -1, messageAcceptor);

        for (FAttribute att : fInterface.getAttributes()) {
            validateName(messageAcceptor, att);
        }
        for (FBroadcast fBroadcast : fInterface.getBroadcasts()) {
            validateName(messageAcceptor, fBroadcast);
            for (FArgument out : fBroadcast.getOutArgs()) {
                validateName(messageAcceptor, out);
            }
        }
        for (FMethod fMethod : fInterface.getMethods()) {
            validateName(messageAcceptor, fMethod);
            for (FArgument out : fMethod.getOutArgs()) {
                validateName(messageAcceptor, out);
            }
            for (FArgument in : fMethod.getInArgs()) {
                validateName(messageAcceptor, in);
            }
        }
    }

    private void validateName(ValidationMessageAcceptor messageAcceptor,
            EObject eObject) {
        String name = ((FModelElement) eObject).getName();
        if (cppKeywords.keyWords.contains(name)) {
            acceptError("Name " + name
                    + " is a keyword in c++", eObject,
                    FrancaPackage.Literals.FMODEL_ELEMENT__NAME, -1,
                    messageAcceptor);
            return;
        }
        if (cppKeywords.reservedWords.contains(name)) {
            acceptError("Name " + name
                    + " is a reserved identifier", eObject,
                    FrancaPackage.Literals.FMODEL_ELEMENT__NAME, -1,
                    messageAcceptor);
            return;
        }
    }

    private boolean isFrancaVersionGreaterThan(int major, int minor, int micro) {
    	Version francaVersion = Version.parseVersion(FrancaGeneratorExtensions.getFrancaVersion());
        if (francaVersion.getMajor() > major) {
            return true;
        }
        if (francaVersion.getMajor() < major){
            return false;
        }
        if (francaVersion.getMinor() > minor) {
            return true;
        }
        if (francaVersion.getMinor() < minor){
            return false;
        }
        if (francaVersion.getMicro() > micro) {
            return true;
        }
        if (francaVersion.getMicro() < micro) {
            return false;
        }
        return false;
    }

    protected void acceptError(String message, EObject object,
            EStructuralFeature feature, int index,
            ValidationMessageAcceptor messageAcceptor) {
        messageAcceptor.acceptError(message, object,
                feature, index, null);
    }

    protected void acceptWarning(String message, EObject object,
            EStructuralFeature feature, int index,
            ValidationMessageAcceptor messageAcceptor) {
        messageAcceptor.acceptWarning(message, object,
                feature, index, null);
    }

    protected void acceptInfo(String message, ValidationMessageAcceptor messageAcceptor) {
        messageAcceptor.acceptInfo(message, null, null, 0, null);
    }

}
