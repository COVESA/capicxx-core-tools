/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.cli;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.impl.ExtensibleURIConverterImpl;
import org.eclipse.xtext.resource.FileNotFoundOnClasspathException;

public class MyURIConverter extends ExtensibleURIConverterImpl {

    /**
     * If these URIConverter gets as URI the file FDeploy.xmi sothe path will be
     * new created to the temporary loaded file. the path to the temporary file
     * we get from the Main class which knows the temporary path. If the given
     * URI is FDeploy.xtextbin we throw a FileNotFoundOnClasspathException
     * because we don't have these file and the calling method expects this
     * error if the file is not available
     */
    @Override
    public URI normalize(URI uri) {
        if (uri.lastSegment() != null && uri.lastSegment().contains("FDeploy.xmi")) {
            URI ret = URI.createFileURI(CommandlineToolMain.createAbsolutPath("." + CommandlineToolMain.FILESEPARATOR
                    + "temp" + CommandlineToolMain.FILESEPARATOR + "FDeploy.xmi"));
            return ret;
        }
        if (uri.lastSegment() != null && uri.lastSegment().contains("FDeploy.xtextbin"))
            throw new FileNotFoundOnClasspathException("");
        return super.normalize(uri);
    }
}
