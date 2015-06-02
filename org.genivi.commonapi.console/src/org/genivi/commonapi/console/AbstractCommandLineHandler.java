/* Copyright (C) 2013-2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.console;

import org.apache.commons.cli.CommandLine;

public abstract class AbstractCommandLineHandler implements ICommandLineHandler
{
    public AbstractCommandLineHandler()
    {
    }

    /**
     * This method has to be implemented. It will be called by the console plug-in if the application arguments match the specification.
     *
     * @param parsedArguments
     *            The parsed arguments with their values
     * @return result of the excution
     */
    @Override
    public abstract int excute(CommandLine parsedArguments);

    /**
     * Converts an instance the CommandLine Object into an array of strings.
     *
     * @param commandLine
     *            instance
     * @return Arguments split into string array
     */
    public static String[] getArguments(CommandLine commandLine)
    {
        return CommandExecuter.getArguments(commandLine);
    }

    /**
     * Converts an instance the CommandLine Object into an array of strings.
     *
     * @param commandLine
     *            instance
     * @param keepIdOption
     *            if true the returned array will contain the ID option if available
     * @return Arguments split into string array
     */
    public static String[] getArguments(CommandLine commandLine, boolean keepIdOption)
    {
        return CommandExecuter.getArguments(commandLine, keepIdOption);
    }

    /**
     * Prints a message to the console and wraps the string to the same width as the outputs done by the command executer.
     *
     * @param message
     *            the message to print
     * @param arguments
     *            Arguments referenced by the format specifiers in the message string. If there are more arguments than format specifiers,
     *            the extra arguments are ignored. The number of arguments is variable and may be zero.
     */
    public static void println(String message, Object... arguments)
    {
        CommandExecuter.println(message, arguments);
    }
}
