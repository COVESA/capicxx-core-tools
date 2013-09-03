/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.cli;

import org.eclipse.emf.codegen.ecore.genmodel.GenModelPackage;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;
import org.franca.deploymodel.dsl.FDeployRuntimeModule;
import org.franca.deploymodel.dsl.FDeployStandaloneSetup;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.util.Modules;

public class DeployStandaloneSetup extends FDeployStandaloneSetup {

    @Override
    public Injector createInjector() {
        // The Module has to replace some standard bindings in the
        // FDeployRuntimeModule
        return Guice.createInjector(Modules.override(new FDeployRuntimeModule()).with(new TestModule()));
    }

    @Override
    public Injector createInjectorAndDoEMFRegistration() {
        // Here the Resourcefactory is set for the fileending .fdepl so that the
        // resourceset knows where to search for a factory for files with these
        // ending
        Resource.Factory.Registry.INSTANCE.getExtensionToFactoryMap().put("fdepl", new FrancaResourceFactory());
        EPackage.Registry.INSTANCE.put(GenModelPackage.eNS_URI, GenModelPackage.eINSTANCE);
        return super.createInjectorAndDoEMFRegistration();
    }

}
