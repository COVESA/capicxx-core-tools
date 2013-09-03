/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.genivi.commonapi.core.validator;

public class Triple<Package, TypeCollectionList, InterfaceList> {

    public final Package packageName;
    public final TypeCollectionList typeCollectionList;
    public final InterfaceList interfaceList;

    public Triple(Package packageName, TypeCollectionList typeCollectionList,
            InterfaceList interfaceList) {
        this.packageName = packageName;
        this.interfaceList = interfaceList;
        this.typeCollectionList = typeCollectionList;
    }

}