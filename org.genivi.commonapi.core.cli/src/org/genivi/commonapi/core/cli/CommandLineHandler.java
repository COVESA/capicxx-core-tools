package org.genivi.commonapi.core.cli;

import java.util.List;

import org.apache.commons.cli.CommandLine;
import org.genivi.commonapi.console.AbstractCommandLineHandler;
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
		// we expect at least the fidel/fdepl file as command line argument
		if(files.size() > 0 && files.get(0) != null) {
			String file = files.get(0); 
			if(file.endsWith(FILE_EXTENSION_FDEPL) || file.endsWith(FILE_EXTENSION_FIDL)) {
				// handle command line options

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
					cliTool.setStubtDirectory(parsedArguments.getOptionValue("ds"));
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

				// Switch on/off validation
				if(parsedArguments.hasOption("val")) {
					cliTool.enableValidation(parsedArguments.getOptionValue("val"));
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
