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
import org.eclipse.xtext.Grammar;
import org.eclipse.xtext.generator.IGenerator;
import org.eclipse.xtext.impl.GrammarImpl;
import org.franca.core.dsl.FrancaIDLRuntimeModule;
import org.franca.core.dsl.FrancaIDLStandaloneSetup;
import org.genivi.commonapi.core.generator.FrancaGenerator;

import com.google.inject.Binder;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

public class StandaloneSetup extends FrancaIDLStandaloneSetup {
    @Override
    public Injector createInjector() {
        return Guice.createInjector(Modules.combine(new FrancaIDLRuntimeModule(), new Module() {

            public void configure(Binder binder) {
                binder.bind(IGenerator.class).to(FrancaGenerator.class);
                binder.bind(Grammar.class).to(GrammarImpl.class);
            }
        }));
    }

    @Override
    public Injector createInjectorAndDoEMFRegistration() {
        EPackage.Registry.INSTANCE.put(GenModelPackage.eNS_URI, GenModelPackage.eINSTANCE);
        return super.createInjectorAndDoEMFRegistration();
    }
}