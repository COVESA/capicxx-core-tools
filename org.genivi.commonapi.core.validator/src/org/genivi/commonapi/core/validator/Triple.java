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