/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.cli;

import org.eclipse.xtext.resource.IResourceFactory;
import org.eclipse.xtext.resource.XtextResourceSet;
import org.franca.deploymodel.dsl.FDeployRuntimeModule;

import com.google.inject.Binder;

public class TestModule extends FDeployRuntimeModule {

    /**
     * Here the Injector gets the bindings which we need for correct generating.
     * For example there we set the binding for the XtextResourceSet that these
     * classes can only be created by the injector by a Provider which i have
     * implemented
     */
    public void configure(Binder binder) {
        binder.bind(IResourceFactory.class).to(FrancaResourceFactory.class);
        binder.bind(XtextResourceSet.class).toProvider(MyProviderXtext.class);
    }

}
