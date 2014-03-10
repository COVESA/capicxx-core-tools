/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.cli;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import javax.inject.Inject;

import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.preferences.DefaultScope;
import org.eclipse.core.runtime.preferences.InstanceScope;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.generator.IFileSystemAccess;
import org.eclipse.xtext.generator.JavaIoFileSystemAccess;
import org.eclipse.xtext.resource.XtextResourceSet;
import org.genivi.commonapi.core.generator.FrancaGenerator;
import org.genivi.commonapi.core.generator.FrancaGeneratorExtensions;
import org.genivi.commonapi.core.preferences.FPreferences;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.dbus.generator.FrancaDBusGenerator;

import com.google.inject.Injector;
import com.google.inject.Provider;

/**
 * This is a little Tool to generate C++ files from Franca files over the
 * commandline without Eclipse. The arguments of this tool can be the Franca
 * files which have to be generated. Additional you can set an argument -dbus to
 * generate for dbus, -dest and a path to a folder in the arguments will set the
 * output-destination to the given path and finally with -pref and a path to a
 * Textile will set the comment in the head of each file to the text from the
 * file given after -pref
 * 
 * @author Patrick Sattler
 */
public class CommandlineToolMain
{

    public static final String                      FILESEPARATOR    = System.getProperty("file.separator");

    public static final String                      FILESEPARATORIMP = "/";
    public static final String                      TEMP_PFAD        = System.getProperty("user.dir") + FILESEPARATOR + "temp/";
    public static final String                      CORE_DEPL        = TEMP_PFAD + FILESEPARATOR + "org.genivi.commonapi.core"
                                                                             + FILESEPARATOR + "deployment" + FILESEPARATOR
                                                                             + "CommonAPI_deployment.fdepl";
    public static final String                      CORE_PFAD        = TEMP_PFAD + FILESEPARATOR + "org.genivi.commonapi.core"
                                                                             + FILESEPARATOR + "deployment";
    public static final String                      DBUS_DEPL        = TEMP_PFAD + FILESEPARATOR + "org.genivi.commonapi.dbus"
                                                                             + FILESEPARATOR + "deployment" + FILESEPARATOR
                                                                             + "CommonAPI-DBus_deployment.fdepl";
    public static final String                      DBUS_PFAD        = TEMP_PFAD + FILESEPARATOR + "org.genivi.commonapi.dbus"
                                                                             + FILESEPARATOR + "deployment";
    public static final String                      TEMP_FDEPL_PFAD  = TEMP_PFAD + FILESEPARATOR + "fdepl";
    public static List<String>                      files            = new ArrayList<String>();
    // All given files were saved in this list with an absolute path
    private static Set<String>                     filelist         = new LinkedHashSet<String>();
    // true if for all interfaces have to be generated the stubs
    private static boolean                          allstubs         = false;

    @Inject
    private static Provider<JavaIoFileSystemAccess> fileAccessProvider;

    public static void generate(String[] args)
    {

        // Initialization with all options
        File tempfolder = null;
        boolean dbus = false;
        String dest = createAbsolutPath("." + FILESEPARATOR + "src-gen" + FILESEPARATOR);
        if (args.length < 1)
        {
            String errorMessage = "Usage: commonapi_generator [options] file...\n"
                    + "\n"
                    + "Options:\n"
                    + "  -dbus                    Additionally generate gluecode for the CommonAPI-D-Bus middleware binding\n"
                    + "  -dest <path/to/folder>   Relative to current location, the generated files will be saved there\n"
                    + "  -pref <path/to/file>     The text in this file which will be inserted as a comment in each generated file (for example your license)\n"
                    + "  -version                 Used versions from the CommonAPI-Generator and Franca plugin\n"
                    + "  -genallincl              Generates all included fidls too (all Proxys and Stubs will be generated)\n";
            throw new IllegalArgumentException(errorMessage);
        }

        List<String> tempfilelist = new ArrayList<String>();
        FPreferences pref = FPreferences.getInstance();
        pref.clidefPreferences();
        /*
         * Reading the options and the files given in the arguments and they
         * will be saved in the predefined attributes
         */
        String francaversion = getFrancaVersion();
        String coreversion = FrancaGeneratorExtensions.getCoreVersion();
        pref.setPreference(PreferenceConstants.FRANCA_VERSION, francaversion);
        pref.setPreference(PreferenceConstants.USEPROJECTSETTINGS, Boolean.TRUE.toString());
        for (int i = 0; i < args.length; i++)
        {
            String arg = args[i];
            if (arg.equals("-dbus"))
                dbus = true;
            else if (arg.equals("-dest"))
            {
                if (i + 1 == args.length)
                {
                    System.err.println("Please write a destination folder after -dest");
                    System.exit(0);
                }
                File file = new File(args[i + 1]);
                if (!file.exists() || !file.isDirectory())
                {
                    if (!file.mkdirs())
                    {
                        System.err.println("Could not create dest path");
                        System.exit(0);
                    }
                }
                dest = createAbsolutPath(args[i + 1]);
                i++;
            }
            else if (arg.equals("-pref"))
            {
                if (i + 1 == args.length)
                {
                    System.err.println("Please write a path to an existing file after -pref");
                    System.exit(0);
                }
                File file = new File(createAbsolutPath(args[i + 1]));
                i++;
                if (!file.exists() || file.isDirectory())
                {
                    System.err.println("Please write a path to an existing file after -pref");
                    System.exit(0);
                }
                System.out.println("The following file was set as header:\n" + file.getAbsolutePath());
                try
                {
                    pref.setPreference(PreferenceConstants.USEPROJECTSETTINGS, Boolean.toString(true));
                    pref.setPreferences(PreferenceConstants.P_LICENSE, file);
                }
                catch (IOException e)
                {
                    e.printStackTrace();
                }
            }
            else if (arg.equals("-version"))
            {
                System.out.println("Franca Version: " + francaversion);
                System.out.println("CommonAPI Version: " + coreversion);
                System.exit(0);
            }
            else if (arg.equals("-genallincl"))
            {
                allstubs = true;
            }
            else
            {
                File file = new File(createAbsolutPath(args[i]));
                if (!file.exists() || file.isDirectory())
                {
                    System.err.println("The following path won't be generated because it doesn't exists:\n" + args[i] + "\n");
                }
                else
                    tempfilelist.add(createAbsolutPath(arg));
            }
        }
        if (tempfilelist.size() == 0)
        {
            System.err.println("There are no valid files to generate!");
            System.exit(0);
        }
        System.out.println("The following path was set as the Outputfolder: \n" + dest);
        System.out.println("Using Franca Version " + francaversion);
        System.out.println("and CommonAPI Version " + coreversion);
        try
        {
            /*
             * The FDeploy.xmi will be loaded from the jar in a temporary folder
             * because the Generator expects a hierarchical URI an with a
             * Hierarchical URI you can't refer to a file in a jar
             */
            File xmifile = null;
            InputStream in = null;
            OutputStream out = null;
            try
            {
                in = CommandlineToolMain.class.getResourceAsStream("/org/franca/deploymodel/dsl/FDeploy.xmi");
                xmifile = new File(TEMP_PFAD + FILESEPARATOR + "FDeploy.xmi");
                tempfolder = new File(TEMP_PFAD);
                tempfolder.mkdir();
                xmifile.createNewFile();
                out = new DataOutputStream(new FileOutputStream(xmifile));
                int l = 0;
                while ((l = in.read()) != -1)
                    out.write(l);
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
            finally
            {
                try
                {
                    out.close();
                }
                catch (Exception e)
                {
                }
                try
                {
                    in.close();
                }
                catch (Exception e)
                {
                }
            }
            /*
             * Here we look recursively at all files and their imports because
             * all fdepls must have overwritten imports so that the user can use
             * the same files in eclipse and the Commandline to generate. In the
             * same time the temporary path to the modified file will be saved
             * instead of the original one
             */

            for (int i = 0; i < tempfilelist.size(); i++)
            {
                filelist.add(rewriteImports(tempfilelist.get(i)));
            }

            DBusCommandExecutableExtensionFactory dbusCommandFactory = new DBusCommandExecutableExtensionFactory();
            Injector injectorDBus = dbusCommandFactory.getInjector();
            CommandExecutableExtensionFactory commandFactory = new CommandExecutableExtensionFactory();
            Injector injectorCore = commandFactory.getInjector();
            URI uri = null;
            // we initialize both generators with the Injectors
            FrancaDBusGenerator dbusgenerator = injectorDBus.getInstance(FrancaDBusGenerator.class);
            FrancaGenerator generator = injectorCore.getInstance(FrancaGenerator.class);

            XtextResourceSet rsset = injectorCore.getProvider(XtextResourceSet.class).get();
            fileAccessProvider = injectorCore.getProvider(JavaIoFileSystemAccess.class);

            final JavaIoFileSystemAccess fsa = fileAccessProvider.get();

            DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_OUTPUT_PROXIES, dest);
            DefaultScope.INSTANCE.getNode(PreferenceConstants.SCOPE).put(PreferenceConstants.P_OUTPUT_STUBS, dest);
            InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_OUTPUT_PROXIES, dest);
            InstanceScope.INSTANCE.getNode(PreferenceConstants.SCOPE).get(PreferenceConstants.P_OUTPUT_STUBS, dest);
            pref.setPreference(PreferenceConstants.P_OUTPUT_PROXIES, dest);
            pref.setPreference(PreferenceConstants.P_OUTPUT_STUBS, dest);
            fsa.setOutputPath(createAbsolutPath(dest));
            fsa.getOutputConfigurations().get(IFileSystemAccess.DEFAULT_OUTPUT).setCreateOutputDirectory(true);
            for (String file : filelist)
            {
                uri = URI.createFileURI(file);
                Resource rs = rsset.createResource(uri);
                if (dbus)
                {
                    // Attention!!! some Methods from the generator are
                    // deprecated because of this it could be in the near future
                    // that URI's will be used
                    dbusgenerator.doGenerate(rs, fsa);
                }
                else
                {
                    generator.doGenerate(rs, fsa);
                }
            }

        }
        finally
        {
            deleteTempFiles(tempfolder);
        }
    }

    /**
     * gets the last segment from a path which fileseperators are
     * System.getProperty("file.separator")
     * 
     * @param pfad
     *            the path from were the filename is going to be extracted
     * @return the last segment of the path
     */
    private static String getFileName(String pfad)
    {
        if (pfad.lastIndexOf(FILESEPARATOR) >= 0)
            return pfad.substring(pfad.lastIndexOf(FILESEPARATOR) + 1).trim();
        return pfad.trim();
    }

    /**
     * the file on the path will be loaded, if the file has imports then the
     * imports will be overwritten with the right path. if the imported files
     * are also *.fdepl then thex will be also loaded
     * 
     * @param path
     *            the path to the file which has to be loaded and analyzed
     * @return the new path to the file
     */
    private static String rewriteImports(String path)
    {
        files.add(path);
        String rootpath = path.substring(0, path.lastIndexOf(FILESEPARATOR));
        String ret = path;
        if (path.endsWith(".fdepl"))
        {
            File filein = new File(createAbsolutPath(path));
            String uristr = "";
            File fileout = null;
            uristr = TEMP_FDEPL_PFAD + FILESEPARATOR + getFileName(path);
            fileout = new File(uristr);
            ret = uristr;
            File folder = new File(TEMP_FDEPL_PFAD);
            folder.mkdirs();
            BufferedReader instr = null;
            BufferedWriter outstr = null;
            try
            {
                fileout.createNewFile();
                instr = new BufferedReader(new FileReader(filein));
                outstr = new BufferedWriter(new FileWriter(fileout));
                String line = "";
                while ((line = instr.readLine()) != null)
                {
                    // if a line contains the string "import" it will be
                    // analyzed if there is something imported that i dont know
                    // how to handle it will be passed through

                    // here could be problems with relative paths on things that
                    // get imported but not handled

                    // the imported files will only be handled if they weren't
                    // handled before
                    if (line.trim().startsWith("import"))
                    {
                        if (line.contains("dbus/deployment/CommonAPI-DBus_deployment.fdepl"))
                        {
                            line = "import \"file:" + FILESEPARATORIMP + replaceAll(DBUS_DEPL, "\\", "/") + "\"";
                            if (!files.contains(DBUS_DEPL))
                            {
                                handleDeployment(false);
                            }
                        }
                        else if (line.contains("core/deployment/CommonAPI_deployment.fdepl"))
                        {
                            line = "import \"file:" + FILESEPARATORIMP + replaceAll(CORE_DEPL, "\\", "/") + "\"";
                            if (!files.contains(CORE_DEPL))
                            {
                                handleDeployment(true);
                            }
                        }
                        else if (line.contains(".fdepl"))
                        {
                            String cp = line;
                            line = "import \"file:" + FILESEPARATORIMP + replaceAll(TEMP_FDEPL_PFAD, "\\", "/") + FILESEPARATORIMP
                                    + getImportsName(line) + "\"";
                            if (!files.contains(createAbsolutPath(getImportPath(cp), path)))
                                rewriteImports(createAbsolutPath(getImportPath(cp), path));
                        }
                        else if (line.contains(".fidl"))
                        {
                            String fidlpath = createAbsolutPath(getImportPath(line), path.substring(0, path.lastIndexOf(FILESEPARATOR)));
                            if (allstubs)
                                filelist.add(fidlpath);
                            line = "import \"file:" + replaceAll(fidlpath, "\\", "/") + "\"";

                        }
                    }
                    outstr.write(line + "\n");
                }

            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
            finally
            {
                try
                {
                    instr.close();
                }
                catch (Exception e)
                {
                    ;
                }
                try
                {
                    outstr.close();
                }
                catch (Exception e)
                {
                    ;
                }
            }
        }
        else if (allstubs)
        {
            File file = new File(createAbsolutPath(path));
            BufferedReader str = null;
            try
            {
                str = new BufferedReader(new FileReader(file));
                String line = "";
                while ((line = str.readLine()) != null)
                {
                    if (line.contains("import"))
                    {
                        String importfile = line.substring(line.indexOf('"') + 1);
                        importfile = importfile.substring(0, importfile.indexOf('"'));
                        filelist.add(createAbsolutPath(importfile,rootpath));
                    }
                }
            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
            finally
            {
                try
                {
                    str.close();
                }
                catch (Exception e)
                {
                }
            }
        }
        return ret;
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
    private static String createAbsolutPath(String path, String rootpath)
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
     * reads from a line with import "path/to/file" the path to file
     * 
     * @param line
     *            the line with the import instruction
     * @return the path alone without import and ""
     */
    private static String getImportPath(String line)
    {
        line = line.substring(line.indexOf("import") + 8).trim();
        line = line.substring(0, line.length() - 1);
        return line;
    }

    /**
     * creates a absolute path from a relative path which starts on the current
     * user directory
     * 
     * @param path
     *            the relative path which start on the current user-directory
     * @return the created absolute path
     */
    public static String createAbsolutPath(String path)
    {
        return createAbsolutPath(path, System.getProperty("user.dir") + FILESEPARATOR);
    }

    /**
     * reads from a line with import "path/to/import/file" the filename
     * 
     * @param line
     *            the line with the import instruction
     * @return The name of the imported file
     */
    private static String getImportsName(String line)
    {
        return getFileName(getImportPath(line));
    }

    /**
     * handles the import instructions which refer to a deployment.fdepl (dbus
     * or core). These deployment files are in the commandline jar and because
     * of this they have to be loaded on a different way you can choose if you
     * would like only to load the core or also the dbus depl (because the dbus
     * deployment.fdepl imports the core deployment.fdepl)
     * 
     * @param core
     *            if true only the core file will be loaded otherwise also the
     *            dbus deployment.fdepl will be loaded
     */
    private static void handleDeployment(boolean core)
    {
        BufferedReader reader = null;
        BufferedWriter writer = null;
        if (!core)
        {
            File file = new File(DBUS_DEPL);
            File folder = new File(DBUS_PFAD);
            folder.mkdirs();
            try
            {
                file.createNewFile();
                reader = new BufferedReader(new InputStreamReader(
                        CommandlineToolMain.class.getResourceAsStream("/CommonAPI-DBus_deployment.fdepl")));
                writer = new BufferedWriter(new FileWriter(file));
                String line = "";
                while ((line = reader.readLine()) != null)
                {
                    if (line.contains("import"))
                    {
                        line = "import \"file:" + FILESEPARATORIMP + replaceAll(CORE_DEPL, "\\", "/") + "\"";
                    }

                    writer.write(line + "\n");
                }
                files.add(DBUS_DEPL);
            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
            finally
            {
                try
                {
                    reader.close();
                }
                catch (Exception e)
                {
                    ;
                }
                try
                {
                    writer.close();
                }
                catch (Exception e)
                {
                    ;
                }
            }

        }
        File file = new File(CORE_DEPL);
        File folder = new File(CORE_PFAD);
        folder.mkdirs();
        try
        {
            file.createNewFile();
            reader = new BufferedReader(new InputStreamReader(CommandlineToolMain.class.getResourceAsStream("/CommonAPI_deployment.fdepl")));
            writer = new BufferedWriter(new FileWriter(file));
            int i = 0;
            while ((i = reader.read()) != -1)
                writer.write(i);
            files.add(CORE_DEPL);
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }
        finally
        {
            try
            {
                reader.close();
            }
            catch (Exception e)
            {
                ;
            }
            try
            {
                writer.close();
            }
            catch (Exception e)
            {
                ;
            }
        }
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
    public static String replaceAll(String text, String toreplace, String replacement)
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

    /**
     * removes recursively all files on the path and his folders and at the end
     * himself
     * 
     * @param path
     *            the path to the folder which has to be deleted
     */
    public static void deleteTempFiles(File path)
    {
        if (path != null && path.isDirectory())
        {
            for (File file : path.listFiles())
            {
                if (file.isDirectory())
                    deleteTempFiles(file);
                file.delete();
            }
        }
        if (path != null)
            path.delete();
    }

    public static String getFrancaVersion()
    {
        return Platform.getBundle("org.franca.core").getVersion().toString();
    }

}
