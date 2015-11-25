/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */
package org.genivi.commonapi.core.cli;

import java.util.List;

import org.apache.commons.cli.CommandLine;
import org.genivi.commonapi.console.AbstractCommandLineHandler;
import org.genivi.commonapi.console.ConsoleLogger;
import org.genivi.commonapi.console.ICommandLineHandler;


/**
 * Handle command line options 
 */
public class CommandLineHandler extends AbstractCommandLineHandler implements
		ICommandLineHandler {

	
	public static final String FILE_EXTENSION_FDEPL = "fdepl";
	public static final String FILE_EXTENSION_FIDL = "fidl";
	private CommandlineToolMain cliTool;
	
	public CommandLineHandler() {
		cliTool = new CommandlineToolMain();
	}

	@Override
	public int excute(CommandLine parsedArguments) {

		@SuppressWarnings("unchecked")
		List<String> files = parsedArguments.getArgList();

		// Disable log outputs if "quiet" as the very first action.
		// -ll --loglevel quiet or verbose
		if(parsedArguments.hasOption("ll")) {
			cliTool.setLogLevel(parsedArguments.getOptionValue("ll"));
		}		
		ConsoleLogger.printLog("Executing CommonAPI Core Code Generation...\n");

		if(parsedArguments.hasOption("sk")) {
			// Switch on generation of skeletons (if this option has a parameter,
			// the parameter is the postfix for the file names
			cliTool.setCreateSkeletonCode();
			String skArgument = parsedArguments.getOptionValue("sk");
			if(skArgument == null) {
				// no -sk argument was given, use "Default"
				skArgument = "Default";
			} 
			// The fidl/fdepl file will be interpreted as sk argument, 
			// if "-sk" is placed just before the fidl/fdepl file in the command line !
			if(skArgument.endsWith(FILE_EXTENSION_FDEPL) || skArgument.endsWith(FILE_EXTENSION_FIDL)) {
				// it is not an -sk argument but it is the file to generate code from !
				files.add(skArgument);
			}
			else {
				cliTool.setSkeletonPostfix(skArgument);
			}
		}			
		// a search path may be specified, collect all fidl/fdepl files
		if(parsedArguments.hasOption("sp")) {
			String searchPath = parsedArguments.getOptionValue("sp");
			files.addAll(cliTool.searchFidlandFdeplFiles(searchPath));
		}	
		
		// we expect at least the fidel/fdepl file as command line argument
		if(files.size() > 0 && files.get(0) != null) {
			String file = files.get(0); 
			if(file.endsWith(FILE_EXTENSION_FDEPL) || file.endsWith(FILE_EXTENSION_FIDL)) {
				// handle command line options

				// Switch off generation of common code
				// -nc --no-common do not generate common code
				if(parsedArguments.hasOption("nc")) {
					cliTool.setNoCommonCode();
				}
				
				// Switch off generation of proxy code
				// -np --no-proxy do not generate proxy code
				if(parsedArguments.hasOption("np")) {
					cliTool.setNoProxyCode();
				}

				// Switch off generation of stub code
				// -ns --no-stub do not generate stub code				
				if(parsedArguments.hasOption("ns")) {
					cliTool.setNoStubCode();
				}

				// destination: -d --dest overwrite default directory
				if(parsedArguments.hasOption("d")) {
					cliTool.setDefaultDirectory(parsedArguments.getOptionValue("d"));
				}

				// destination: -dc --dest-common overwrite target directory for common part
				if(parsedArguments.hasOption("dc")) {
					cliTool.setCommonDirectory(parsedArguments.getOptionValue("dc"));
				}

				// destination: -dp --dest-proxy overwrite target directory for proxy code
				if(parsedArguments.hasOption("dp")) {
					cliTool.setProxyDirectory(parsedArguments.getOptionValue("dp"));
				}

				// destination: -ds --dest-stub overwrite target directory for stub code
				if(parsedArguments.hasOption("ds")) {
					cliTool.setStubDirectory(parsedArguments.getOptionValue("ds"));
				}

				// destination: -dsk --dest-skel overwrite target directory for skeleton code
				if(parsedArguments.hasOption("dsk")) {
					cliTool.setSkeletonDirectory(parsedArguments.getOptionValue("dsk"));
				}

				// A file path, that points to a file, that contains the license text.
				// -l --license license text in generated files
				if(parsedArguments.hasOption("l")) {
					cliTool.setLicenseText(parsedArguments.getOptionValue("l"));
				}

				// Add prefixes to enumeration literals
				// -pre --prefix-enum-literal additional prefix to all generated enumeration literals
				if(parsedArguments.hasOption("pre")) {
					cliTool.setEnumPrefix(parsedArguments.getOptionValue("pre"));
				}
				// Switch off validation
				if(parsedArguments.hasOption("nv")) {
					cliTool.disableValidation();
				}
				// Switch off code generation
				if(parsedArguments.hasOption("ng")) {
					cliTool.disableCodeGeneration();
				}
				// Don't generate code for included types and interfaces
				if(parsedArguments.hasOption("wod")) {
					cliTool.noCodeforDependencies();
				}
				// Don't generate synchronous calls
				if(parsedArguments.hasOption("nsc")) {
					cliTool.disableSyncCalls();
				}
				// print out generated files
				if(parsedArguments.hasOption("pf")) {
					cliTool.listGeneratedFiles();
				}
				// finally invoke the generator.
				// the remaining arguments are assumed to be files !
				cliTool.generateCore(files);
			}
			else {
				System.out.println("The file extension should be ." + FILE_EXTENSION_FIDL + " or ." + FILE_EXTENSION_FDEPL);
			}
		}
		else {
			System.out.println("A *.fidl or *.fdepl file was not specified !");
		}
		return 0;
	}
}
