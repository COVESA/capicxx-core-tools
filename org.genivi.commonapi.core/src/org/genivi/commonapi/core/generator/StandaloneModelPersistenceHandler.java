/* Copyright (C) 2015-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator;

import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.common.util.WrappedException;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.xtext.validation.ValidationMessageAcceptor;
import org.franca.core.utils.ImportsProvider;

/**
 * This is a fixed version of {@link org.franca.core.utils.ModelPersistenceHandler}.
 * <p></p>
 * Usage of {@link StandaloneModelPersistenceHandler} is at least needed when Franca plug-ins are initialized for 'standalone' usage, hence the class name.
 * <p></p>
 * See also descriptions and discussions about the issue in the original code here: https://github.com/franca/franca/issues/180
 * <p></p>
 * Basically the problem is that {@link org.franca.core.utils.ModelPersistenceHandler} is using a 'URIMap'
 * to map relative import paths to absolute URIs. This may lead to wrong results in case the mapping is
 * not unique within a given file set.
 */
public class StandaloneModelPersistenceHandler {

    /**
     * All models that have cross-references must exist in the same ResourceSet
     */
    private ResourceSet resourceSet;

    /**
     * Map used to handle generically different model files.
     */
    private static Map<String, ImportsProvider> fileHandlerRegistry = new HashMap<String, ImportsProvider>();

    /**
     * Error handling.
     */
    private ValidationMessageAcceptor messageAcceptor;

    /**
     * Special handling for missing deployment specification files.
     *
     * Missing deployment specification files shall not create an error. e.g. if Some/IP code generator plug-in
     * is performing its Some/IP validation, it shall not create an error about a missing D-Bus deployment
     * specification file and vice versa.
     */
    private static final String DEPLOYMENT_SPEC = "deployment_spec.fdepl";
    private boolean ignoreMissingDeploymentSpecs = false;

    /**
     * Creating an object used to save or to load a set of related models from files.
     *
     * @param newResourceSet
     *            the resource set to save all the loaded files/ where all the models to be saved exist
     * @param newPrependPath
     *            a relative path to work in
     */
    public StandaloneModelPersistenceHandler(ResourceSet newResourceSet) {
        resourceSet = newResourceSet;
    }

    public static void registerFileExtensionHandler(String extension, ImportsProvider importsProvider)
    {
        fileHandlerRegistry.put(extension, importsProvider);
    }

    /**
     *
     * Load the model found in the fileName. Its dependencies can be loaded subsequently.
     *
     * @param uri       the URI to be loaded
     * @param root      the root of the model (needed for loading multiple file models)
     *                  This has to be an absolute, hierarchical URI.
     * @return the root model
     */
    public EObject loadModel(URI uri, URI root) {
        // resolve the input uri, in case it is a relative path
        URI absURI = uri.resolve(root);

        // load root model
        Resource resource = null;
        try {
            resource = resourceSet.getResource(absURI, true);
            resource.load(Collections.EMPTY_MAP);
        } catch (IOException e) {
            if (ignoreMissingDeploymentSpecs && absURI.toString().endsWith(DEPLOYMENT_SPEC))
                return null;
            if (messageAcceptor != null)
                messageAcceptor.acceptError(e.getMessage(), null, null, 0, null, (String[])null);
            else
                e.printStackTrace();
            return null;
        } catch (WrappedException e) {
            if (e.exception() instanceof IOException) {
                if (ignoreMissingDeploymentSpecs && absURI.toString().endsWith(DEPLOYMENT_SPEC))
                    return null;
            }
            if (messageAcceptor != null)
                messageAcceptor.acceptError(e.getMessage(), null, null, 0, null, (String[])null);
            else
                e.printStackTrace();
            return null;
        }
        if (resource.getContents().size() == 0)
            return null;
        EObject model = resource.getContents().get(0);

        // load all its imports recursively
        for (Iterator<String> it = fileHandlerRegistry.get(absURI.fileExtension()).importsIterator(model); it.hasNext();) {
            String importURIStr = it.next();
            URI importURI = URI.createURI(importURIStr);
            URI resolvedURI = importURI.resolve(absURI);
            loadModel(resolvedURI, root);
        }
        return model;
    }

    public ResourceSet getResourceSet() {
        return resourceSet;
    }

    /**
     * Calculates the new relative working directory for an import.
     *
     * @param filename
     * @param cwd
     * @return
     */
    public static URI getCWDForImport(URI filename, URI cwd) {
        URI relativeCWD = cwd;

        if (filename.isRelative()) {
            if (cwd.segmentCount() > 0 && filename.segmentCount() > 1) {
                relativeCWD = URI.createURI(cwd.toString() + "/" + filename.trimSegments(1).toString()) ;
            } else if (filename.segmentCount() > 1) {
                relativeCWD = filename.trimSegments(1);
            }
        }
        return relativeCWD;
    }

    /**
     * Convert Windows path separator to Unix one used in URIs.
     *
     * @param path
     * @return
     */
    public static URI normalizeURI(URI path)
    {
        if (path.isFile())
        {
            return URI.createURI(path.toString().replaceAll("\\\\", "/"));
        }
        return path;
    }

    /**
     * Specify that file IO errors due to missing deployment specification files are ignored.
     *
     * @param ignoreMissingDeploymentSpecs
     */
    public void setIgnoreMissingDeploymentSpecs(boolean ignoreMissingDeploymentSpecs)
    {
        this.ignoreMissingDeploymentSpecs = ignoreMissingDeploymentSpecs;
    }

    public boolean isIgnoreMissingDeploymentSpecs()
    {
        return ignoreMissingDeploymentSpecs;
    }

    /**
     * Set a message acceptor for {@link #loadModel(URI, URI)}
     *
     * @param messageAcceptor
     */
    public void setMessageAcceptor(ValidationMessageAcceptor messageAcceptor)
    {
        this.messageAcceptor = messageAcceptor;
    }

    public ValidationMessageAcceptor getMessageAcceptor()
    {
        return messageAcceptor;
    }
}
