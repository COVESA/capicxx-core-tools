/* Copyright (C) 2015-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator;

import java.util.HashSet;
import java.util.Set;

import org.eclipse.xtext.generator.JavaIoFileSystemAccess;


/**
 * This class adds the feature to printout the generated files to the JavaIoFileSystemAccess implementation.
 */
public class GeneratorFileSystemAccess extends JavaIoFileSystemAccess {

	private Set<String> fileList = new HashSet<String>();
	
	public GeneratorFileSystemAccess() {
		super();
	}
	
	public void clearFileList() {
		fileList.clear();
	}
	
	/**
	 * Call the base class method and store the filename in the list
	 * @param fileName using '/' as file separator
	 * @param contents the to-be-written contents.
	 */	
	public void generateFile(String fileName, CharSequence contents) {
		// do not generate a file if the contents is empty (feature: suppress code generation)
		if(contents.length() > 0) {
			super.generateFile(fileName, contents);
		}
		addFilePath(fileName, "");
	}
	
	/**
	 * Call the base class method and store the filename in the list
	 * @param fileName using '/' as file separator
	 * @param outputConfigurationName the name of the output configuration
	 * @param contents the to-be-written contents.
	 */
	public void generateFile(String fileName, String outputConfigurationName, CharSequence contents) {
		if(contents.length() > 0) {
			super.generateFile(fileName, outputConfigurationName, contents);
		}
		addFilePath(fileName, outputConfigurationName);
	}
	
	public void dumpGeneratedFiles() {
		for(String name : fileList) {
			System.out.println(name);
		}
	}
	
	public void addFilePath(String fileName, String outputConfigurationName) {
		String dirName = "";
		if(!outputConfigurationName.isEmpty()) {
			dirName = super.getOutputConfig(outputConfigurationName).getOutputDirectory();
		}
		if(!dirName.endsWith("/")) {
			dirName += "/";
		}
		fileList.add(dirName + fileName);		
	}
}
