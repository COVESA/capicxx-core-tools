/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.genivi.commonapi.core.generator;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.franca.core.dsl.FrancaImportsProvider;
import org.franca.core.dsl.FrancaPersistenceManager;
import org.franca.core.franca.FModel;
import org.franca.core.utils.FileHelper;

import com.google.inject.Inject;
import com.google.inject.Provider;

public class MyFrancaPersistenceManager extends FrancaPersistenceManager {

    private final String fileExtension = "fidl";

    @Inject
    private Provider<ResourceSet> resourceSetProvider;

    @Override
    public FModel loadModel(String filename) {
        try {
            URI uri = FileHelper.createURI(filename);
            URI fileURI = MyModelPersistenceHandler.normalizeURI(uri);

            if (fileURI.segmentCount() > 1) {
                return loadModel(fileURI.lastSegment(), fileURI.trimSegments(1)
                        .toString() + "/");
            } else {
                return loadModel(filename, "");
            }
        } catch (Exception ex) {
            System.err.println("Error: " + ex.getMessage());
            return null;
        }
    }

    private FModel loadModel(String filename, String cwd) {
        String fn = filename;

        if (fn == null)
            return null;
        if (!fn.endsWith("." + fileExtension)) {
            fn += "." + fileExtension;
        }

        MyModelPersistenceHandler persistenceHandler = createMyModelPersistenceHandler(resourceSetProvider
                .get());
        return (FModel) persistenceHandler.loadModel(fn, cwd);
    }

    private MyModelPersistenceHandler createMyModelPersistenceHandler(
            ResourceSet resourceSet) {
        MyModelPersistenceHandler.registerFileExtensionHandler(fileExtension,
                new FrancaImportsProvider());

        MyModelPersistenceHandler persistenceHandler = new MyModelPersistenceHandler(
                resourceSet);
        return persistenceHandler;
    }
}
