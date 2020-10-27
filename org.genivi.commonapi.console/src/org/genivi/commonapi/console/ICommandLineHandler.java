/* Copyright (C) 2015-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.console;

import org.apache.commons.cli.CommandLine;

public interface ICommandLineHandler
{
    /**
     * This method has to be implemented. It will be called by the console plug-in if the application arguments match the specification.
     *
     * @param parsedArguments
     *            The parsed arguments with their values
     * @return result of the excution
     */
    public int excute(CommandLine parsedArguments);
}
