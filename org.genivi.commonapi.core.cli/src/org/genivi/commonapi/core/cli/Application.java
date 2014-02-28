/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */
package org.genivi.commonapi.core.cli;

import org.eclipse.core.runtime.Platform;
import org.eclipse.equinox.app.IApplication;
import org.eclipse.equinox.app.IApplicationContext;

public class Application implements IApplication
{
    /*
     * (non-Javadoc)
     *
     * @see org.eclipse.equinox.app.IApplication#start(org.eclipse.equinox.app.
     * IApplicationContext)
     */
    @Override
    public Object start(IApplicationContext context) throws Exception
    {
        /*
         * sleep 100 ms to guarantee that the gogo.shell.Activator is not
         * interrupted in start-up, sometimes causing a InterruptException
         */
        Thread.sleep(100);
        String[] args = Platform.getApplicationArgs();

        try
        {
            CommandlineToolMain.generate(args);
        }
        catch (IllegalArgumentException e)
        {
            System.err.println(e.getMessage());
        }

        return IApplication.EXIT_OK;
    }

    /*
     * (non-Javadoc)
     *
     * @see org.eclipse.equinox.app.IApplication#stop()
     */
    @Override
    public void stop()
    {

    }
}
