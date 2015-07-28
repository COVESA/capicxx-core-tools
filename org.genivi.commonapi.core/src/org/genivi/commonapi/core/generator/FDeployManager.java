package org.genivi.commonapi.core.generator;

import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
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
import org.franca.deploymodel.dsl.fDeploy.Import;

/**
 * The FDeployManager loads models from fdepl files and from fidl files that are imported in a fdepl file.
 * It continues to import files even it could not find all of them (e.g an unknown deployment specification).
 * 
 * @author gutknecht
 *
 */
public class FDeployManager {

	private final static String fileExtension = "fdepl";

	private ResourceSet resourceSet;
	
	/**
	 * Map used to handle generically different model files. 
	 */
	private static Map<String, ImportsProvider> fileHandlerRegistry = new HashMap<String, ImportsProvider>();

	public FDeployManager() {
		
		resourceSet = new ResourceSetImpl();
		
		// we expect fdepl files
		fileHandlerRegistry.put(fileExtension, new FDeployImportsProvider());
		// allow for fidl file imports
		fileHandlerRegistry.put("fidl", new FrancaImportsProvider());
	}	
	
	/**
	 * 
	 * Load the model found in the fileName. Its dependencies (imports) can be loaded subsequently.
	 * @param uri       the URI to be loaded
	 * @param root      the root of the model (needed for loading multiple file models)
	 *                  This has to be an absolute, hierarchical URI.
	 * @return the root model or null in case of an error.
	 */
	public EObject loadModel(URI uri, URI root) {
		// resolve the input uri, in case it is a relative path
		URI absURI = uri.resolve(root);
		if (! uri.equals(absURI)) {
			// add this pair to URI converter so that others can get the URI by its relative path
			resourceSet.getURIConverter().getURIMap().put(uri, absURI);
		}
		// load root model
		Resource resource = null;
		try {
			resource = resourceSet.getResource(absURI, true);
			// Set the isLoaded flag to false in order to force reloading of fdepl or imported fidl files
			resource.unload();
			resource.load(Collections.EMPTY_MAP);
		} catch (Exception e) {
			// Don't show an error message here, because code may be generated from an included fidl file.
			//System.err.println("Failed to load model from : " + absURI);
			return null;
		}
		EObject model = resource.getContents().get(0);
		
		// load all its imports recursively
		for (Iterator<String> it = fileHandlerRegistry.get(absURI.fileExtension()).importsIterator(model); it.hasNext();) {
			String importURIStr = it.next();
			if(importURIStr != null) {
				URI importURI = URI.createURI(importURIStr);
				URI resolvedURI = importURI.resolve(absURI);
				// add this pair to URI converter so that others can get the URI by its relative path
				resourceSet.getURIConverter().getURIMap().put(importURI, resolvedURI);
				//System.out.println("trying to load model " + resolvedURI);
				EObject localModel = loadModel(resolvedURI, root);
				if(localModel == null) {
					// something went wrong with this import, go on with the next one
					continue;
				}
			}
		}
		return  model;
	}

	/**
	 * @return the file extension this class will deal with (.fdepl)
	 */
	public static String fileExtension() {
		return fileExtension;
	}	
	
	/**
	 * Load the model from fdepl that has an fidl file import
	 * @param fdmodel - the deployment model from fdepl 
	 * @param input - the fdepl uri
	 * @return the model defined in the fidl file import
	 */
	public FModel getModelFromFdepl(FDModel fdmodel, URI input) {
		for(Import fdimport : fdmodel.getImports()) {
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
					// failed to  load model from fidl
					return null;
				}
				return (FModel) resource.getContents().get(0);
			}
		}
		return null;
	}	
	
	
}
