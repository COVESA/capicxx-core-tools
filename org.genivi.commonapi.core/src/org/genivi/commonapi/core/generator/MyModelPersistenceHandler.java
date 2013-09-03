/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.genivi.commonapi.core.generator;

import java.io.IOException;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.franca.core.utils.ImportsProvider;
import org.franca.core.utils.ModelPersistenceHandler;

public class MyModelPersistenceHandler extends ModelPersistenceHandler {
 
    private ResourceSet resourceSet;

    private static Map<String, ImportsProvider> fileHandlerRegistry = new HashMap<String, ImportsProvider>();

    public MyModelPersistenceHandler(ResourceSet newResourceSet) {
        super(newResourceSet);
    }

    public static void registerFileExtensionHandler(String extension,
            ImportsProvider importsProvider) {
        fileHandlerRegistry.put(extension, importsProvider);
    }

    @Override
    public EObject loadModel(String filename, String cwd) {
        resourceSet = super.getResourceSet();
        Path p = new Path(filename);
        URI fileURI;
        URI cwdURI;
        if (!p.isAbsolute()) {
            fileURI = normalizeURI(URI.createURI(filename));
            cwdURI = normalizeURI(URI.createURI(cwd));
        } else {
            URI absolutURI = normalizeURI(URI.createURI(filename));
            fileURI = normalizeURI(URI.createURI(absolutURI.lastSegment()));
            cwdURI = normalizeURI(URI.createURI(absolutURI.toString()
                    .substring(0, absolutURI.toString().lastIndexOf("/") + 1)));
        }
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
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
        EObject model = resource.getContents().get(0);

        for (Iterator<String> it = fileHandlerRegistry.get(
                fileURI.fileExtension()).importsIterator(model); it.hasNext();) {
            String importURI = it.next();
            this.loadModel(importURI, getCWDForImport(fileURI, cwdURI)
                    .toString());
        }
        return model;
    }
}
