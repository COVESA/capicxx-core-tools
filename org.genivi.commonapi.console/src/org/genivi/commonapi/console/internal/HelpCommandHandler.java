/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.console.internal;

import org.apache.commons.cli.CommandLine;
import org.genivi.commonapi.console.AbstractCommandLineHandler;
import org.genivi.commonapi.console.CommandExecuter;

public class HelpCommandHandler extends AbstractCommandLineHandler
{
    @Override
    public int excute(CommandLine parsedArguments)
    {
        // Command executer will print help on empty command.
        CommandExecuter.INSTANCE.executeCommand("");

        return 0;
    }
}
