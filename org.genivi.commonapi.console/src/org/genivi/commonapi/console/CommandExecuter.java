/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.console;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.cli.AlreadySelectedException;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.MissingArgumentException;
import org.apache.commons.cli.MissingOptionException;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.OptionGroup;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.Parser;
import org.apache.commons.cli.PosixParser;
import org.apache.commons.cli.UnrecognizedOptionException;
import org.apache.commons.lang.WordUtils;
import org.eclipse.core.runtime.Assert;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionPoint;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.ISafeRunnable;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.SafeRunner;
import org.eclipse.core.runtime.Status;
import org.genivi.commonapi.console.internal.Application;

public enum CommandExecuter
{
    INSTANCE;

    private final String       COMMANDS_EXTENSION_POINT_ID                 = Application.PLUGIN_ID + ".commands";
    private final String       OPTIONS_EXTENSION_POINT_ID                  = Application.PLUGIN_ID + ".options";
    private final String       OPTION_GROUPS_EXTENSION_POINT_ID            = Application.PLUGIN_ID + ".optionGroups";

    private final String       COMMAND_ID_ATTRIBUTE_NAME                   = "id";
    private final String       COMMAND_NAME_ATTRIBUTE_NAME                 = "name";
    private final String       COMMAND_HANDLER_ATTRIBUTE_NAME              = "class";
    private final String       COMMAND_SYNTAX_ATTRIBUTE_NAME               = "syntax";

    private final String       OPTION_ID_ATTRIBUTE_NAME                    = "id";
    private final String       OPTION_SHORT_NAME_ATTRIBUTE_NAME            = "shortName";
    private final String       OPTION_LONG_NAME_ATTRIBUTE_NAME             = "longName";
    private final String       OPTION_DESCRIPTION_ATTRIBUTE_NAME           = "description";
    private final String       OPTION_REQUIRED_ATTRIBUTE_NAME              = "required";
    private final String       OPTION_ARG_COUNT_ATTRIBUTE_NAME             = "argCount";
    private final String       OPTION_ARG_NAME_ATTRIBUTE_NAME              = "argName";
    private final String       OPTION_HAS_OPTIONAL_ARG_ATTRIBUTE_NAME      = "hasOptionalArg";
    private final String       OPTION_VALUE_SEPARATOR_ATTRIBUTE_NAME       = "valueSeparator";

    private final String       OPTION_ID_OPTION_ID_ATTRIBUTE_NAME          = "optionId";

    private final String       OPTION_GROUP_ID_ATTRIBUTE_NAME              = "id";
    private final String       OPTION_GROUP_REQUIRED_ATTRIBUTE_NAME        = "required";

    private final String       OPTION_GROUP_OPTION_GROUP_ID_ATTRIBUTE_NAME = "optionGroupId";

    private final String       OPTIONS_CONFIGURATION_NAME                  = "options";
    private final String       HEADER_CONFIGURATION_NAME                   = "header";
    private final String       FOOTER_CONFIGURATION_NAME                   = "footer";
    private final String       OPTION_CONFIGURATION_NAME                   = "option";
    private final String       OPTION_ID_CONFIGURATION_NAME                = "optionId";
    private final String       OPTION_GROUP_CONFIGURATION_NAME             = "optionGroup";
    private final String       OPTION_GROUP_ID_CONFIGURATION_NAME          = "optionGroupId";

    private final String       ECLIPSE_LAUNCHER_PROPERTY_NAME              = "eclipse.launcher";

    private final String       ID_OPTION_DESCRIPTION                       = "%s of the desired console command";
    private final String       EXTENSION_POINT_PARSING_ERROR_MESSAGE       = "An error occured while parsing console command \"%s\"!";
    private final String       OPTION_ID_NOT_FOUND_MESSAGE                 = "The option ID \"%s\" could not be found!";
    private final String       OPTION_GROUP_ID_NOT_FOUND_MESSAGE           = "The option group ID \"%s\" could not be found!";
    private final String       HELP_NO_COMMANDS_TEXT_MESSAGE               = "No registered console commands available!";
    private final String       HELP_NAME_TEXT_MESSAGE                      = "Command: %s";
    private final String       HELP_ID_TEXT_MESSAGE                        = "%s: %s";
    private final String       HELP_WRONG_ID_MESSAGE                       = HELP_ID_TEXT_MESSAGE + "%nID does not match: %s";
    private final String       HELP_SEVERAL_COMMANDS_MESSAGE               = "Several console commands are compatible to this set of parameters. Add ID option to select the desired one.%n";
    private final String       LAUNCHER_NAME;
    private final Option       ID_OPTION;

    public static final String SHORT_ID_OPTION                             = "ID";
    public static final String LONG_ID_OPTION                              = "COMMAND_ID";

    public static final int    DEFAULT_RETURN_VALUE                        = -1000;
    public static final int    CONSOLE_WIDTH                               = 80;

    private int                returnValue                                 = DEFAULT_RETURN_VALUE;

    private CommandExecuter()
    {
        String launcherName = System.getProperty(ECLIPSE_LAUNCHER_PROPERTY_NAME);
        LAUNCHER_NAME = (null != launcherName) ? new Path(launcherName).lastSegment() : Platform.getProduct().getName();

        ID_OPTION = new Option(SHORT_ID_OPTION, LONG_ID_OPTION, true, String.format(ID_OPTION_DESCRIPTION, SHORT_ID_OPTION));
        ID_OPTION.setRequired(true);
        ID_OPTION.setArgs(1);
        ID_OPTION.setOptionalArg(true);
    }

    public int executeCommand(String command)
    {
        Assert.isNotNull(command);

        String[] arguments = (command.length() > 0) ? (command.trim().split("\\s")) : (new String[0]);

        return executeCommand(arguments);
    }

    public int executeCommand(String[] arguments)
    {
        Assert.isNotNull(arguments);

        final List<ConsoleConfiguration> configurations = getParsedExtensionPointConfigurations();

        // Print message if there are no registered console commands.
        if (configurations.size() == 0)
        {
            println(HELP_NO_COMMANDS_TEXT_MESSAGE);

            return DEFAULT_RETURN_VALUE;
        }

        // Print help text if no option is available.
        if (arguments.length == 0)
        {
            printHelp(configurations);

            return DEFAULT_RETURN_VALUE;
        }

        // Find a suitable console configuration.
        final List<ConsoleConfiguration> perfectMatchingConfigurations = new ArrayList<ConsoleConfiguration>();
        final List<ConsoleConfiguration> partialMatchingConfigurations = new ArrayList<ConsoleConfiguration>();
        final List<ConsoleConfiguration> notMatchingConfigurations = new ArrayList<ConsoleConfiguration>();

        Parser parser = new PosixParser();

        for (ConsoleConfiguration configuration : configurations)
        {
            try
            {
                CommandLine commandLine = parser.parse(configuration.options, arguments);

                // Add ID option and ID information to configuration. Thus if there are several
                // perfect matches the printHelp method can print this information.
                configuration.options.addOption(ID_OPTION);
                configuration.message = String.format(HELP_ID_TEXT_MESSAGE, SHORT_ID_OPTION, configuration.id);
                configuration.commandLine = commandLine;
                perfectMatchingConfigurations.add(configuration);
            }
            catch (MissingOptionException | MissingArgumentException | AlreadySelectedException exception)
            {
                configuration.message = exception.getMessage();
                partialMatchingConfigurations.add(configuration);
            }
            catch (UnrecognizedOptionException exception)
            {
                // Check if the unrecognized option is the ID option.
                String unrecognizedOption = exception.getOption();

                if (unrecognizedOption.equals("-" + SHORT_ID_OPTION) || unrecognizedOption.equals("--" + LONG_ID_OPTION))
                {
                    try
                    {
                        // Add ID option to options object an parse arguments again.
                        CommandLine commandLine = parser.parse(configuration.options.addOption(ID_OPTION), arguments);
                        String idValue = commandLine.getOptionValue(SHORT_ID_OPTION);

                        // The correct console configuration is found if parsing did not throw an
                        // exception and the ID option equals extension point ID.
                        if (configuration.id.equals(idValue))
                        {
                            configuration.commandLine = commandLine;
                            perfectMatchingConfigurations.add(configuration);
                        }
                        else
                        {
                            configuration.message = String.format(HELP_WRONG_ID_MESSAGE, SHORT_ID_OPTION, configuration.id, idValue);
                            partialMatchingConfigurations.add(configuration);
                        }
                    }
                    catch (ParseException e)
                    {
                        configuration.message = e.getMessage();
                        notMatchingConfigurations.add(configuration);
                    }
                }
                else
                {
                    configuration.message = exception.getMessage();
                    notMatchingConfigurations.add(configuration);
                }
            }
            catch (ParseException exception)
            {
                configuration.message = exception.getMessage();
                notMatchingConfigurations.add(configuration);
            }
        }

        // Execute compatible command or print help text
        if (perfectMatchingConfigurations.size() == 1)
        {
            SafeRunner.run(new ISafeRunnable()
            {
                @Override
                public void run() throws Exception
                {
                    ConsoleConfiguration configuration = perfectMatchingConfigurations.get(0);

                    Object executable = configuration.commandConfiguration.createExecutableExtension(COMMAND_HANDLER_ATTRIBUTE_NAME);
                    ICommandLineHandler handler = (ICommandLineHandler) executable;

                    returnValue = handler.excute(configuration.commandLine);
                }

                @Override
                public void handleException(Throwable throwable)
                {
                    log(throwable);

                    // Explicitly show the error to the user, otherwise we risk to just quietly log it without
                    // giving the user a chance to know that something went wrong.
                    throwable.printStackTrace();
                }
            });
        }
        else if (perfectMatchingConfigurations.size() > 1)
        {
            println(HELP_SEVERAL_COMMANDS_MESSAGE);
            printHelp(perfectMatchingConfigurations);
        }
        else if (partialMatchingConfigurations.size() > 0)
        {
            printHelp(partialMatchingConfigurations);
        }
        else
        {
            printHelp(notMatchingConfigurations);
        }

        return returnValue;
    }

    private void printHelp(List<ConsoleConfiguration> configurations)
    {
        for (ConsoleConfiguration configuration : configurations)
        {
            printHelp(configuration);
        }
    }

    private void printHelp(ConsoleConfiguration configuration)
    {
        println(HELP_NAME_TEXT_MESSAGE, configuration.name);

        if (null != configuration.message)
        {
            println(configuration.message);
        }

        boolean generateSyntax = true;
        String syntax = LAUNCHER_NAME;

        if (null != configuration.syntax && configuration.syntax.length() > 0)
        {
            syntax = syntax + " " + configuration.syntax;
            generateSyntax = false;
        }

        new HelpFormatter().printHelp(CONSOLE_WIDTH, syntax, configuration.header, configuration.options, configuration.footer,
                generateSyntax);

        System.out.println();
    }

    private void log(Throwable throwable)
    {
        IStatus status = new Status(Status.ERROR, Application.PLUGIN_ID, throwable.getMessage(), throwable);

        Platform.getLog(Platform.getBundle(Application.PLUGIN_ID)).log(status);
    }

    private List<ConsoleConfiguration> getParsedExtensionPointConfigurations()
    {
        List<ConsoleConfiguration> configurations = new ArrayList<ConsoleConfiguration>();

        // Parse extension point information
        IExtensionRegistry extensionRegistry = Platform.getExtensionRegistry();
        IExtensionPoint commandsExtensionPoint = extensionRegistry.getExtensionPoint(COMMANDS_EXTENSION_POINT_ID);

        if (null != commandsExtensionPoint)
        {
            Map<String, IConfigurationElement> referenceableOptionConfigurations = parseExtensionPoint(
                    extensionRegistry.getExtensionPoint(OPTIONS_EXTENSION_POINT_ID), OPTION_ID_ATTRIBUTE_NAME);
            Map<String, IConfigurationElement> referenceableOptionGroupConfigurations = parseExtensionPoint(
                    extensionRegistry.getExtensionPoint(OPTION_GROUPS_EXTENSION_POINT_ID), OPTION_GROUP_ID_ATTRIBUTE_NAME);

            for (IConfigurationElement commandConfiguration : commandsExtensionPoint.getConfigurationElements())
            {
                String id = commandConfiguration.getAttribute(COMMAND_ID_ATTRIBUTE_NAME);

                try
                {
                    Options options = new Options();

                    String name = commandConfiguration.getAttribute(COMMAND_NAME_ATTRIBUTE_NAME);
                    String syntax = commandConfiguration.getAttribute(COMMAND_SYNTAX_ATTRIBUTE_NAME);
                    String header = null;
                    String footer = null;

                    for (IConfigurationElement childConfiguration : commandConfiguration.getChildren())
                    {
                        switch (childConfiguration.getName())
                        {
                            case OPTIONS_CONFIGURATION_NAME:
                                for (IConfigurationElement optionConfiguration : childConfiguration.getChildren())
                                {
                                    Option option = getOptionByConfiguration(optionConfiguration, referenceableOptionConfigurations);
                                    OptionGroup optionGroup = getOptionGroupByConfiguration(optionConfiguration,
                                            referenceableOptionGroupConfigurations, referenceableOptionConfigurations);

                                    if (null != option)
                                    {
                                        options.addOption(option);
                                    }

                                    if (null != optionGroup)
                                    {
                                        options.addOptionGroup(optionGroup);
                                    }
                                }
                                break;

                            case HEADER_CONFIGURATION_NAME:
                                header = childConfiguration.getValue();
                                break;

                            case FOOTER_CONFIGURATION_NAME:
                                footer = childConfiguration.getValue();
                                break;
                        }
                    }

                    configurations.add(new ConsoleConfiguration(commandConfiguration, options, id, name, syntax, header, footer));
                }
                catch (Exception exception)
                {
                    String message = String.format(EXTENSION_POINT_PARSING_ERROR_MESSAGE + " " + exception.getMessage(), id);

                    log(new IllegalArgumentException(message, exception));
                }
            }
        }

        return configurations;
    }

    private Map<String, IConfigurationElement> parseExtensionPoint(IExtensionPoint extensionPoint, String keyAttributeName)
    {
        Map<String, IConfigurationElement> map = new HashMap<String, IConfigurationElement>();

        if (null != extensionPoint && null != keyAttributeName)
        {
            for (IConfigurationElement configurationElement : extensionPoint.getConfigurationElements())
            {
                map.put(configurationElement.getAttribute(keyAttributeName), configurationElement);
            }
        }

        return map;
    }

    private Option getOptionByConfiguration(IConfigurationElement optionConfiguration,
            Map<String, IConfigurationElement> referenceableOptionConfigurations)
    {
        switch (optionConfiguration.getName())
        {
            case OPTION_CONFIGURATION_NAME:
                return createOption(optionConfiguration);

            case OPTION_ID_CONFIGURATION_NAME:
                String referencedOptionId = optionConfiguration.getAttribute(OPTION_ID_OPTION_ID_ATTRIBUTE_NAME);
                IConfigurationElement referencedOptionConfiguration = referenceableOptionConfigurations.get(referencedOptionId);

                if (null == referencedOptionConfiguration)
                {
                    throw new IllegalArgumentException(String.format(OPTION_ID_NOT_FOUND_MESSAGE, referencedOptionId));
                }

                return createOption(referencedOptionConfiguration);

            default:
                return null;
        }
    }

    private OptionGroup getOptionGroupByConfiguration(IConfigurationElement optionConfiguration,
            Map<String, IConfigurationElement> referenceableOptionGroupConfigurations,
            Map<String, IConfigurationElement> referenceableOptionConfigurations)
    {
        switch (optionConfiguration.getName())
        {
            case OPTION_GROUP_CONFIGURATION_NAME:
                return createOptionGroup(optionConfiguration, referenceableOptionConfigurations);

            case OPTION_GROUP_ID_CONFIGURATION_NAME:
                String referencedOptionGroupId = optionConfiguration.getAttribute(OPTION_GROUP_OPTION_GROUP_ID_ATTRIBUTE_NAME);
                IConfigurationElement referencedOptionGroupConfiguration = referenceableOptionGroupConfigurations
                        .get(referencedOptionGroupId);

                if (null == referencedOptionGroupConfiguration)
                {
                    throw new IllegalArgumentException(String.format(OPTION_GROUP_ID_NOT_FOUND_MESSAGE, referencedOptionGroupId));
                }

                return createOptionGroup(referencedOptionGroupConfiguration, referenceableOptionConfigurations);

            default:
                return null;
        }
    }

    private Option createOption(IConfigurationElement optionConfiguration)
    {
        String shortName = optionConfiguration.getAttribute(OPTION_SHORT_NAME_ATTRIBUTE_NAME);
        String longName = optionConfiguration.getAttribute(OPTION_LONG_NAME_ATTRIBUTE_NAME);
        String description = optionConfiguration.getAttribute(OPTION_DESCRIPTION_ATTRIBUTE_NAME);
        String required = optionConfiguration.getAttribute(OPTION_REQUIRED_ATTRIBUTE_NAME);
        String argCount = optionConfiguration.getAttribute(OPTION_ARG_COUNT_ATTRIBUTE_NAME);
        String argName = optionConfiguration.getAttribute(OPTION_ARG_NAME_ATTRIBUTE_NAME);
        String hasOptionalArg = optionConfiguration.getAttribute(OPTION_HAS_OPTIONAL_ARG_ATTRIBUTE_NAME);
        String valueSeparator = optionConfiguration.getAttribute(OPTION_VALUE_SEPARATOR_ATTRIBUTE_NAME);

        Option option = new Option(shortName, description);
        option.setRequired(Boolean.parseBoolean(required));
        option.setArgs(Integer.parseInt(argCount));
        option.setOptionalArg(Boolean.parseBoolean(hasOptionalArg));

        if (null != longName)
        {
            option.setLongOpt(longName);
        }

        if (null != argName)
        {
            option.setArgName(argName);
        }

        if (null != valueSeparator)
        {
            option.setValueSeparator(valueSeparator.charAt(0));
        }

        return option;
    }

    private OptionGroup createOptionGroup(IConfigurationElement optionGroupConfiguration,
            Map<String, IConfigurationElement> referenceableOptionConfigurations)
    {
        String required = optionGroupConfiguration.getAttribute(OPTION_GROUP_REQUIRED_ATTRIBUTE_NAME);

        OptionGroup optionGroup = new OptionGroup();
        optionGroup.setRequired(Boolean.parseBoolean(required));

        for (IConfigurationElement optionConfiguration : optionGroupConfiguration.getChildren())
        {
            Option option = getOptionByConfiguration(optionConfiguration, referenceableOptionConfigurations);

            if (null != option)
            {
                optionGroup.addOption(option);
            }
        }

        return optionGroup;
    }

    private class ConsoleConfiguration
    {
        public final IConfigurationElement commandConfiguration;
        public final Options               options;
        public final String                id;
        public final String                name;
        public final String                syntax;
        public final String                header;
        public final String                footer;

        public CommandLine                 commandLine;
        public String                      message;

        public ConsoleConfiguration(IConfigurationElement commandConfiguration, Options options, String id, String name, String syntax,
                String header, String footer, CommandLine commandLine, String message)
        {
            this.commandConfiguration = commandConfiguration;
            this.options = options;
            this.id = id;
            this.name = name;
            this.syntax = syntax;
            this.header = header;
            this.footer = footer;
            this.commandLine = commandLine;
            this.message = message;
        }

        public ConsoleConfiguration(IConfigurationElement commandConfiguration, Options options, String id, String name, String syntax,
                String header, String footer)
        {
            this(commandConfiguration, options, id, name, syntax, header, footer, null, null);
        }
    }

    /**
     * Converts an instance the CommandLine Object into an array of strings.
     *
     * @param commandLine
     *            instance
     * @return Arguments split into string array
     */
    public static String[] getArguments(CommandLine commandLine)
    {
        return getArguments(commandLine, false);
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
        Assert.isNotNull(commandLine);

        List<String> argumentsList = new ArrayList<String>();

        Option[] options = commandLine.getOptions();

        if (null != options)
        {
            for (Option option : options)
            {
                String optionString = option.getOpt();

                if (keepIdOption == false && SHORT_ID_OPTION.equals(optionString))
                {
                    continue;
                }

                argumentsList.add("-" + optionString);

                String[] values = option.getValues();

                if (null != values)
                {
                    for (String value : values)
                    {
                        argumentsList.add(value);
                    }
                }
            }
        }

        return argumentsList.toArray(new String[argumentsList.size()]);
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
        Assert.isNotNull(message);

        String formattedString = String.format(message, arguments);
        String newLineString = String.format("%n");

        // Split message into single line strings because the wrap method cannot handle multi-line messages.
        for (String line : formattedString.split(newLineString))
        {
            System.out.println(WordUtils.wrap(line, CONSOLE_WIDTH));
        }

        // A new-line character at the end gets lost because of the split method.
        if (formattedString.endsWith(newLineString))
        {
            System.out.println();
        }
    }
}
