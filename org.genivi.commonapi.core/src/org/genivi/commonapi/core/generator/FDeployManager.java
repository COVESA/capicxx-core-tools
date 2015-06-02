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
import org.franca.core.utils.ImportsProvider;
import org.franca.deploymodel.dsl.FDeployImportsProvider;

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
			URI importURI = URI.createURI(importURIStr);
			URI resolvedURI = importURI.resolve(absURI);
			
			// add this pair to URI converter so that others can get the URI by its relative path
			resourceSet.getURIConverter().getURIMap().put(importURI, resolvedURI);
			//System.out.println("trying to load model " + resolvedURI);
			model = loadModel(resolvedURI, root);
			if(model == null) {
				// something went wrong with this import, go on with the next one
				continue;
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
	
	
}
