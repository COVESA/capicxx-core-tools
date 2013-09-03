/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.cli;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.linking.lazy.LazyLinker;
import org.eclipse.xtext.linking.lazy.LazyLinkingResource;
import org.eclipse.xtext.linking.lazy.LazyURIEncoder;
import org.eclipse.xtext.resource.IResourceFactory;
import org.franca.deploymodel.dsl.parser.antlr.FDeployParser;
import org.franca.deploymodel.dsl.services.FDeployGrammarAccess;

import com.google.inject.Inject;
import com.google.inject.Injector;

public class FrancaResourceFactory implements IResourceFactory {

    @Inject
    Injector in;

    /**
     * creates from uris to files with the fdepl fileending resources
     */
    public Resource createResource(URI uri) {

        LazyLinkingResource rs = in.getInstance(LazyLinkingResource.class);
        rs.setURI(uri);
        FDeployParser parser = in.getInstance(FDeployParser.class);
        FDeployGrammarAccess access = in.getInstance(FDeployGrammarAccess.class);

        // For here the FDeploy.xmi is loaded
        parser.setGrammarAccess(access);
        rs.setParser(parser);

        rs.setLinker(in.getInstance(LazyLinker.class));
        rs.setEncoder(in.getInstance(LazyURIEncoder.class));

        return rs;
    }

}
