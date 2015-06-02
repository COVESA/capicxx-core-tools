package org.genivi.commonapi.console.internal;

import java.util.Map;
import java.util.TreeMap;

import org.apache.commons.cli.CommandLine;
import org.eclipse.core.runtime.Platform;
import org.genivi.commonapi.console.AbstractCommandLineHandler;
import org.genivi.commonapi.console.CommandExecuter;
import org.osgi.framework.Bundle;

public class VersionCommandHandler extends AbstractCommandLineHandler
{
    public static final String SHORT_ALL_OPTION       = "a";
    public static final String SHORT_PLUGINS_OPTION   = "p";

    private final char         HEADER_SEPARATOR_CHAR  = '-';
    private final String       LIST_SEPARATOR_TEXT    = " ";

    private final String       SYMBOLIC_NAME_TEXT     = "Symbolic Name";
    private final String       VERSION_TEXT           = "Version";
    private final String       BIT_TEXT               = "Bit";

	private final String       CODE_GENERATOR_MESSAGE = "Code Generator:";
    private final String       VERSION_MESSAGE        = VERSION_TEXT + ":";
    private final String       BUILD_ID_MESSAGE       = "Build id:";
    private final String       ARCHITECTURE_MESSAGE   = "Architecture:";
    private final String       PLUGINS_MESSAGE        = "Plug-ins:";

    final int                  DEFAULT_NAME_WIDTH     = 50;

    @Override
    public int excute(final CommandLine parsedArguments)
    {
        final boolean printAllVersions = parsedArguments.hasOption(SHORT_ALL_OPTION);
        final boolean printPluginVersions = parsedArguments.hasOption(SHORT_PLUGINS_OPTION);

        if (printAllVersions)
        {
            println(CODE_GENERATOR_MESSAGE);
        }

        if (printAllVersions || !printPluginVersions)
        {
            final String formatString = "%-13s %s";

            println(formatString, VERSION_MESSAGE, Activator.getDefault().getMapping(Activator.VERSION_MAPPING));
            println(formatString, BUILD_ID_MESSAGE, Activator.getDefault().getMapping(Activator.BUILD_ID_MAPPING));
            println(formatString + " %s", ARCHITECTURE_MESSAGE, Activator.getDefault().getMapping(Activator.ARCHITECTURE_MAPPING), BIT_TEXT);
            println("");
        }

        if (printAllVersions)
        {
            println(PLUGINS_MESSAGE);
            println("%-" + DEFAULT_NAME_WIDTH + "s%s%s", SYMBOLIC_NAME_TEXT, LIST_SEPARATOR_TEXT, VERSION_TEXT);
            println(String.format("%-" + CommandExecuter.CONSOLE_WIDTH + "s", "").replace(' ', HEADER_SEPARATOR_CHAR));
        }

        if (printAllVersions || printPluginVersions)
        {
            final TreeMap<String, String> sortedbundleInfos = new TreeMap<String, String>();

            for (final Bundle bundle : Platform.getBundle(Activator.PLUGIN_ID).getBundleContext().getBundles())
            {
                sortedbundleInfos.put(bundle.getSymbolicName(), bundle.getVersion().toString());
            }

            final int width = CommandExecuter.CONSOLE_WIDTH - LIST_SEPARATOR_TEXT.length();

            for (final Map.Entry<String, String> bundleInfo : sortedbundleInfos.entrySet())
            {
                final String name = bundleInfo.getKey();
                final String version = bundleInfo.getValue();
                int nameWidth = DEFAULT_NAME_WIDTH;
                int versionWidth = version.length();

                if (versionWidth > width - nameWidth)
                {
                    nameWidth = Math.max(width - versionWidth, name.length());
                }

                println("%-" + nameWidth + "s%s%s", name, LIST_SEPARATOR_TEXT, version);
            }

            println("");
        }

        return 0;
    }
}
