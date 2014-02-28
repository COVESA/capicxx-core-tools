/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.cli;

import org.eclipse.xtext.generator.IGenerator;
import org.genivi.commonapi.dbus.generator.FrancaDBusGenerator;
import org.osgi.framework.Bundle;

import com.google.inject.Binder;

public class DBusCommandExecutableExtensionFactory extends CommandExecutableExtensionFactory
{
    @Override
    protected Bundle getBundle()
    {
        return Activator.getDefault().getBundle();
    }

    @Override
    protected void bindGeneratorClass(Binder binder)
    {
        binder.bind(IGenerator.class).to(FrancaDBusGenerator.class);
    }
}
