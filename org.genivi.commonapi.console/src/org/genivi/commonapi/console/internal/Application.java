/* Copyright (C) 2015-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.console.internal;

import org.eclipse.core.runtime.Platform;
import org.eclipse.equinox.app.IApplication;
import org.eclipse.equinox.app.IApplicationContext;
import org.franca.core.dsl.FrancaIDLStandaloneSetup;
import org.franca.deploymodel.dsl.FDeployStandaloneSetup;
import org.genivi.commonapi.console.CommandExecuter;

public class Application implements IApplication
{
    public static final String PLUGIN_ID          = "org.genivi.commonapi.console";

    private final long         MINIMUM_RUNTIME_MS = 100;

    @Override
    public Object start(IApplicationContext context) throws Exception
    {
        long startTime = System.currentTimeMillis();

        FrancaIDLStandaloneSetup.doSetup();
        FDeployStandaloneSetup.doSetup();

        int returnValue = CommandExecuter.INSTANCE.executeCommand(Platform.getApplicationArgs());

        // Prevent launcher messages if exit code is not 0.
        System.setProperty(IApplicationContext.EXIT_DATA_PROPERTY, "");

        // The application has to run at least 100 ms. Otherwise the plug-in "org.apache.felix.gogo.shell"
        // will throw an interrupt exception because its startup process takes about 100 ms.
        long waitTime = MINIMUM_RUNTIME_MS - (System.currentTimeMillis() - startTime);

        if (waitTime > 0)
        {
            Thread.sleep(waitTime);
        }

        return new Integer(returnValue);
    }

    @Override
    public void stop()
    {
    }
}
