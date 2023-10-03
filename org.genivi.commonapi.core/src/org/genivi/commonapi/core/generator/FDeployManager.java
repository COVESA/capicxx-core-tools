/* Copyright (C) 2015-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;
import org.franca.core.dsl.FrancaImportsProvider;
import org.franca.core.franca.FModel;
import org.franca.core.utils.ImportsProvider;
import org.franca.deploymodel.dsl.FDeployImportsProvider;
import org.franca.deploymodel.dsl.fDeploy.FDModel;
import org.franca.deploymodel.dsl.fDeploy.FDSpecification;
import org.franca.deploymodel.dsl.fDeploy.Import;
import org.genivi.commonapi.core.verification.ValidateElements;

/**
 * The FDeployManager loads models from fdepl files and from fidl files that are
 * imported in a fdepl file. It continues to import files even it could not find
 * all of them (e.g an unknown deployment specification).
 *
 * @author gutknecht
 *
 */
public class FDeployManager {

	private final static String fdeplExtension = "fdepl";

	private ResourceSet resourceSet;

	/**
	 * Map used to handle generically different model files.
	 */
	private static Map<String, ImportsProvider> fileHandlerRegistry = new HashMap<String, ImportsProvider>();

	private Map<String, FDModel> deploymentModels = new HashMap<String, FDModel>();
	private Map<String, FModel> fidlModels = new HashMap<String, FModel>();
	private Map<String, FDSpecification> specifications = new HashMap<String, FDSpecification>();
	private List<String> importsLoadedList = new ArrayList<String>();
	private ValidateElements validateElements = new ValidateElements();

	public FDeployManager() {

		resourceSet = new ResourceSetImpl();

		// we expect fdepl files
		fileHandlerRegistry.put(fdeplExtension, new FDeployImportsProvider());
		// allow for fidl file imports
		fileHandlerRegistry.put("fidl", new FrancaImportsProvider());
	}

	/**
	 *
	 * Load the model found in the fileName. Its dependencies (imports) can be
	 * loaded subsequently.
	 *
	 * @param uri
	 *            the URI to be loaded
	 * @param root
	 *            the root of the model (needed for loading multiple file
	 *            models) This has to be an absolute, hierarchical URI.
	 * @return the root model or null in case of an error.
	 */
	public EObject loadModel(URI uri, URI root) {

		// Add the URI to be loaded, to check the cyclic imports
		importsLoadedList.add(uri.toString());

		// Check if this file is already loaded
		if(deploymentModels.keySet().contains(uri.toString())
				|| fidlModels.keySet().contains(uri.toString())) {
			return null;
		}

		URI absURI = uri.resolve(root);

        // Do not put relative paths into that 'URIMap'. See also: https://github.com/franca/franca/issues/180
		//
//		if (!uri.equals(absURI)) {
//			// add this pair to URI converter so that others can get the URI by
//			// its relative path
//			resourceSet.getURIConverter().getURIMap().put(uri, absURI);
//		}

		// load root model
		Resource resource = null;
		try {
			resource = resourceSet.getResource(absURI, true);
			// Set the isLoaded flag to false in order to force reloading of
			// fdepl/fidl files
			resource.unload();
			resource.load(Collections.EMPTY_MAP);
		} catch (Exception e) {
			// Don't show an error message here, because code may be generated
			// from an included fidl file.
			// System.err.println("Failed to load model from : " + absURI + "(" + e.getMessage() +")");
			return null;
		}

		EObject model = resource.getContents().get(0);

		// load all its imports recursively
		for (Iterator<String> it = fileHandlerRegistry.get(
				absURI.fileExtension()).importsIterator(model); it.hasNext();) {
			String importURIStr = it.next();
			if (importURIStr != null) {
				if(!importURIStr.startsWith("platform")) {
					String[] splitted = importURIStr.split(":");
					// there may be a drive letter "C:" under Windows,
					// remove it, to get it resolved
					importURIStr = splitted[splitted.length -1];
				}
				URI importURI = URI.createURI(importURIStr);
				URI resolvedURI = importURI.resolve(absURI);
		        // Do not put relative paths into that 'URIMap'. See also: https://github.com/franca/franca/issues/180
		        //
//				// add this pair to URI converter so that others can get the URI
//				// by its relative path
//				resourceSet.getURIConverter().getURIMap()
//						.put(importURI, resolvedURI);
				String uriName = resolvedURI.toString();
				EObject importModel = null;

                if (!importsLoadedList.contains(uriName))
                {
                    importModel = loadModel(resolvedURI, root);
                }

				if (importModel != null) {
					if (importModel instanceof FDModel) {
						// CommonAPI models do _NOT_ contain specifications 
						List<FDSpecification> itsSpecifications = ((FDModel)importModel).getSpecifications();
						if (itsSpecifications.size() == 0) {
							deploymentModels.put(uriName, (FDModel) importModel);
						} else {
							for (FDSpecification s : itsSpecifications) {
								if (!specifications.containsKey(s.getName())) {
									specifications.put(s.getName(), s);
								}
							}
						}
					}
					else if(importModel instanceof FModel) {
						fidlModels.put(uriName, (FModel) importModel);
					}
				}
			}
		}

		if (model instanceof FDModel) {
			validateElements.validateFDModelElements((FDModel)model);
		}
		else if(model instanceof FModel) {
			String rootUriString = root.toString();
			if (rootUriString.endsWith("fidl")) {
				validateElements.validateFModelElements((FModel)model);
			}
		}
		importsLoadedList.clear();
		return model;
	}

	/**
	 * @return the file extension this class will deal with (.fdepl)
	 */
	public static String fileExtension() {
		return fdeplExtension;
	}

	/**
	 * Load the model from fdepl that has an fidl file import
	 *
	 * @param fdmodel
	 *            - the deployment model from fdepl
	 * @param input
	 *            - the fdepl uri
	 * @return the model defined in the fidl file import
	 */
	public FModel getModelFromFdepl(FDModel fdmodel, URI input) {
		for (Import fdimport : fdmodel.getImports()) {
			String uriString = fdimport.getImportURI();
			if (uriString.endsWith("fidl")) {
				URI newUri = URI.createURI(uriString);
				URI fidlUri = newUri.resolve(input);
				// load model
				Resource resource = null;
				try {
					resource = resourceSet.getResource(fidlUri, true);
					resource.load(Collections.EMPTY_MAP);
				} catch (Exception e) {
					// failed to load model from fidl
					return null;
				}
				return (FModel) resource.getContents().get(0);
			}
		}
		return null;
	}

	public Map<String, FDModel> getDeploymentModels() {
		return deploymentModels;
	}

	public void clearDeploymentModels() {
		deploymentModels.clear();
	}

	public Map<String, FModel> getFidlModels() {
		return fidlModels;
	}

	public void clearFidlModels() {
		fidlModels.clear();
	}
	
	public FDSpecification getDeploymentSpecification(String name) {
		return specifications.get(name);
	}
}
