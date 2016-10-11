package org.genivi.commonapi.console.internal;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.MissingResourceException;
import java.util.PropertyResourceBundle;

import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Plugin;
import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;

/**
 * The activator class controls the plug-in life cycle
 */
public class Activator extends Plugin
{
    // The plug-in ID
    public static final String  PLUGIN_ID            = "org.genivi.commonapi.console"; //$NON-NLS-1$

    // The shared instance
    private static Activator    plugin;

    private static final String ABOUT_MAPPINGS       = "$nl$/about.mappings";         //$NON-NLS-1$

    public static final String  VERSION_MAPPING      = "0";                           //$NON-NLS-1$
    public static final String  BUILD_ID_MAPPING     = "1";                           //$NON-NLS-1$
    public static final String  ARCHITECTURE_MAPPING = "2";                           //$NON-NLS-1$
    public static final String  VERSION              = "VERSION";                     //$NON-NLS-1$
    public static final String  BUILD_ID             = "BUILD_ID";                    //$NON-NLS-1$

    public Map<String, String>  mappings             = null;

    /**
     * The constructor
     */
    public Activator()
    {
    }

    /*
     * (non-Javadoc)
     *
     * @see org.eclipse.ui.plugin.AbstractUIPlugin#start(org.osgi.framework.BundleContext)
     */
    @Override
    public void start(BundleContext context) throws Exception
    {
        super.start(context);
        plugin = this;

        mappings = loadMappings(Platform.getBundle(PLUGIN_ID));
    }

    /*
     * (non-Javadoc)
     *
     * @see org.eclipse.ui.plugin.AbstractUIPlugin#stop(org.osgi.framework.BundleContext)
     */
    @Override
    public void stop(BundleContext context) throws Exception
    {
        plugin = null;
        super.stop(context);
    }

    public String getMapping(String key)
    {
        return mappings.get(key);
    }

    /**
     * Returns the shared instance
     *
     * @return the shared instance
     */
    public static Activator getDefault()
    {
        return plugin;
    }

    private HashMap<String, String> loadMappings(Bundle definingBundle)
    {
        URL location = FileLocator.find(definingBundle, new Path(ABOUT_MAPPINGS), Collections.<String,String>emptyMap());
        PropertyResourceBundle bundle = null;
        InputStream inputStream = null;

        if (null != location)
        {
            try
            {
                inputStream = location.openStream();
                bundle = new PropertyResourceBundle(inputStream);
            }
            catch (IOException e)
            {
                bundle = null;
            }
            finally
            {
                try
                {
                    if (null != inputStream)
                    {
                        inputStream.close();
                    }
                }
                catch (IOException e)
                {
                    // do nothing if we fail to close
                }
            }
        }

        HashMap<String, String> mappings = new HashMap<String, String>();

        if (null != bundle)
        {
            boolean found = true;

            for (int i = 0; true == found; i++)
            {
                try
                {
                    String key = Integer.toString(i);
                    String value = bundle.getString(key);
                    int valueLength = value.length();

                    if (valueLength > 2 && value.charAt(0) == '$' && value.charAt(valueLength - 1) == '$')
                    {
                        String systemPropertyKey = value.substring(1, valueLength - 1);
                        value = System.getProperty(systemPropertyKey); //$NON-NLS-1$;
                    }

                    mappings.put(key, value);
                }
                catch (MissingResourceException e)
                {
                    found = false;
                }
            }
        }

        return mappings;
    }
}
