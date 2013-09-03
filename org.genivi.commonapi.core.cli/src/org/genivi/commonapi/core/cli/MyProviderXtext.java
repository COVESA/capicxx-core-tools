/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.cli;

import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.resource.XtextResourceSet;

import com.google.inject.Provider;

public class MyProviderXtext implements Provider<XtextResourceSet> {

    /**
     * These Provider creates a XtextResourceSet because i had to modify the
     * URIConverter so that the FDeploy.xmi will be loaded correctly
     */
    public XtextResourceSet get() {
        XtextResourceSet rsset = new XtextResourceSet();
        rsset.setURIConverter(new MyURIConverter());
        rsset.setResourceFactoryRegistry(Resource.Factory.Registry.INSTANCE);
        return rsset;
    }

}
