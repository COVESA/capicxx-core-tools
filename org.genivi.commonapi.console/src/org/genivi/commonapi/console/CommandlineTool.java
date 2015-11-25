package org.genivi.commonapi.console;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.core.runtime.Platform;


public class CommandlineTool {

	public static final String       FILESEPARATOR    = System.getProperty("file.separator");
	protected boolean isCodeGeneration = true;
	protected static final int ERROR_STATE = 1;
	protected static final int NO_ERROR_STATE = 0;
	protected boolean dumpGeneratedFiles;
	protected List<String>       files            = new ArrayList<String>();
	protected boolean hasValidationError;
	protected boolean isValidation = true;
	protected static final String FDEPL_EXTENSION = ".fdepl";
	protected static final String FIDL_EXTENSION = ".fidl";
	
	/**
	 * Get the text from a file which will be inserted as a comment in each generated file (for example your license)
	 * 
	 * @param fileWithText
	 * @return
	 */
	public String getLicenseText(String fileWithText) {
		String licenseText = "";
		if (fileWithText != null && !fileWithText.isEmpty())
		{
			File file = new File(createAbsolutPath(fileWithText));
			if (!file.exists() || file.isDirectory())
			{
				ConsoleLogger.printErrorLog("Please specify a path to an existing file after option -l");
			}
			BufferedReader inReader = null;

			try
			{
				inReader = new BufferedReader(new FileReader(file));
				String thisLine;
				while ((thisLine = inReader.readLine()) != null)
				{
					licenseText = licenseText + thisLine + "\n";
				}
			}
			catch (IOException e)
			{
				ConsoleLogger.printLog("Failed to get the text from the given file: " + e.getLocalizedMessage());
			}
			finally
			{
				try
				{
					if(inReader != null) {
						inReader.close();
					}
				}
				catch (Exception e)
				{
					;
				}
			}
		}
		else
		{
			ConsoleLogger.printErrorLog("Please write a path to an existing file after -l");
		}
		return licenseText;
	}	

	/**
	 * creates a absolute path from a relative path which starts on the current
	 * user directory
	 * 
	 * @param path
	 *            the relative path which start on the current user-directory
	 * @return the created absolute path
	 */
	public String createAbsolutPath(String path)
	{
		return createAbsolutPath(path, System.getProperty("user.dir") + FILESEPARATOR);
	}

	/**
	 * Here we create an absolute path from a relativ path and a rootpath from
	 * which the relative path begins
	 * 
	 * @param path
	 *            the relative path which begins on rootpath
	 * @param rootpath
	 *            an absolute path to a folder
	 * @return the merded absolute path without points
	 */
	private String createAbsolutPath(String path, String rootpath)
	{
		if (System.getProperty("os.name").contains("Windows"))
		{
			if (path.startsWith(":", 1))
				return path;
		}
		else
		{
			if (path.startsWith(FILESEPARATOR))
				return path;
		}

		String ret = (rootpath.endsWith(FILESEPARATOR) ? rootpath : (rootpath + FILESEPARATOR)) + path;
		while (ret.contains(FILESEPARATOR + "." + FILESEPARATOR) || ret.contains(FILESEPARATOR + ".." + FILESEPARATOR))
		{
			if (ret.contains(FILESEPARATOR + ".." + FILESEPARATOR))
			{
				String temp = ret.substring(0, ret.indexOf(FILESEPARATOR + ".."));
				temp = temp.substring(0, temp.lastIndexOf(FILESEPARATOR));
				ret = temp + ret.substring(ret.indexOf(FILESEPARATOR + "..") + 3);
			}
			else
			{
				ret = replaceAll(ret, FILESEPARATOR + "." + FILESEPARATOR, FILESEPARATOR);
			}
		}
		return ret;
	}


	/**
	 * a relaceAll Method which doesn't interprets the toreplace String as a
	 * regex and so you can also replace \ and such special things
	 * 
	 * @param text
	 *            the text who has to be modified
	 * @param toreplace
	 *            the text which has to be replaced
	 * @param replacement
	 *            the text which has to be inserted instead of toreplace
	 * @return the modified text with all toreplace parts replaced with
	 *         replacement
	 */
	public String replaceAll(String text, String toreplace, String replacement)
	{
		String ret = "";
		while (text.contains(toreplace))
		{
			ret += text.substring(0, text.indexOf(toreplace)) + replacement;
			text = text.substring(text.indexOf(toreplace) + toreplace.length());
		}
		ret += text;
		return ret;
	}

	public String getFrancaVersion()
	{
		return Platform.getBundle("org.franca.core").getVersion().toString();
	}
	
	protected static FilenameFilter fidlFilter = new FilenameFilter() {
		public boolean accept(File dir, String name) {
			if(name.endsWith(FIDL_EXTENSION)) {
				return true;
			} 
		return false;
		}
	};	
	
	protected static FilenameFilter fdeplFilter = new FilenameFilter() {
		public boolean accept(File dir, String name) {
			if (name.endsWith(FDEPL_EXTENSION)) {
				return true;
			}
			return false;
		}
	};	

	private static FilenameFilter fidlFdeplFilter = new FilenameFilter() {
		public boolean accept(File dir, String name) {
			if((name.endsWith(FIDL_EXTENSION) || name.endsWith(FDEPL_EXTENSION))) {
				return true;
			} 
		return false;
		}
	};
	
	
	public List<String> searchFidlFiles(String searchPath) {
		return searchFiles(searchPath, fidlFilter); 
	}
	
	public List<String> searchFdeplFiles(String searchPath) {
		return searchFiles(searchPath, fdeplFilter); 
	}

	public List<String> searchFidlandFdeplFiles(String searchPath) {
		return searchFiles(searchPath, fidlFdeplFilter); 
	}	
	
	/**
	 * Search files with the given filter in the given path. the directories in a linux path are
	 * separated by ":".
	 * 
	 * @param searchPath
	 * @return the list of fdepl files.
	 */
	private List<String> searchFiles(String searchPath, FilenameFilter filter) {
		List<String> fileList = new ArrayList<String>();
		String[] searchDirs;
		// if the search path contains several directories...
		if (System.getProperty("os.name").contains("Windows")) {
			searchDirs = searchPath.split(",");			
		}
		else {
			searchDirs = searchPath.split(":");
		}
		for (String dir : searchDirs) {
			try {
				File searchDir = new File(dir);
				if (searchDir.isDirectory()) {
					for (File file : searchDir.listFiles(filter)) {
						fileList.add(file.getPath());
					}
				}
			} catch (Exception e) {
			}
		}
		return fileList;
	}	
	
}
