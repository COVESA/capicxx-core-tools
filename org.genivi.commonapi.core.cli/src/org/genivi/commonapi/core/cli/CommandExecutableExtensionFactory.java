/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.cli;

import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;
import org.eclipse.xtext.generator.IGenerator;
import org.eclipse.xtext.ui.ecore.ExecutableExtensionFactory;
import org.genivi.commonapi.core.generator.FrancaGenerator;
import org.osgi.framework.Bundle;

import com.google.inject.Binder;
import com.google.inject.Injector;
import com.google.inject.Module;

public class CommandExecutableExtensionFactory extends ExecutableExtensionFactory
{
    private Injector injector;

    @Override
    protected Bundle getBundle()
    {
        return Activator.getDefault().getBundle();
    }

    @Override
    protected Injector getInjector()
    {
        if (injector == null)
            injector = super.getInjector().createChildInjector(new Module()
            {
                @Override
                public void configure(final Binder binder)
                {
                    binder.bind(ResourceSet.class).to(ResourceSetImpl.class);
                    bindGeneratorClass(binder);
                }
            });

        return injector;
    }

    protected void bindGeneratorClass(final Binder binder)
    {
        binder.bind(IGenerator.class).to(FrancaGenerator.class);
    }
}
