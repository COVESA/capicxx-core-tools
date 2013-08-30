package org.genivi.commonapi.core.validator;

import org.franca.core.franca.impl.FModelImpl;
import java.io.File;
import java.io.IOException;
import java.util.*;
import java.util.Map.Entry;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;
import org.franca.core.franca.FInterface;
import org.franca.core.franca.FTypeCollection;

public class AllInfoMapsBuilder {

    private String path = "";
    public Map<String, Triple<String, ArrayList<String>, ArrayList<String>>> allInfo = new HashMap<String, Triple<String, ArrayList<String>, ArrayList<String>>>();
    private Triple<String, ArrayList<String>, ArrayList<String>> infoTriple;
    private ResourceSet resourceSet = null;
    public Map<String, HashMap<String, HashSet<String>>> fastAllInfo = new HashMap<String, HashMap<String, HashSet<String>>>();

    public boolean buildAllInfos(String path) {
        if (this.path.equals(path)) {
            return false;
        }
        resourceSet = new ResourceSetImpl();
        this.path = path;
        allInfo.clear();
        buildAllInfo(path);
        fastAllInfo.clear();
        buildFastAllInfo();
        return true;

    }

    private void buildFastAllInfo() {

        for (Entry<String, Triple<String, ArrayList<String>, ArrayList<String>>> entry : allInfo
                .entrySet()) {
            addNames(entry, entry.getValue().interfaceList);
            addNames(entry, entry.getValue().typeCollectionList);
        }
    }

    private void addNames(
            Entry<String, Triple<String, ArrayList<String>, ArrayList<String>>> entry,
            ArrayList<String> arrayList) {
        for (String name : arrayList) {
            HashMap<String, HashSet<String>> hilfMap = new HashMap<String, HashSet<String>>();
            HashSet<String> hilfSet = new HashSet<String>();
            if (fastAllInfo.containsKey(name)) {
                hilfMap = fastAllInfo.get(name);
                if (hilfMap.containsKey(entry.getValue().packageName)) {
                    hilfSet = hilfMap.get(entry.getValue().packageName);
                    hilfSet.add(entry.getKey());
                    hilfMap.put(entry.getValue().packageName, hilfSet);
                    fastAllInfo.put(name, hilfMap);
                }
            }
            hilfSet.add(entry.getKey());
            hilfMap.put(entry.getValue().packageName, hilfSet);
            fastAllInfo.put(name, hilfMap);
        }
    }

    public void updateAllInfo(EObject model, String absolutPath) {
        ArrayList<String> typeCollectionList = new ArrayList<String>();
        ArrayList<String> interfaceList = new ArrayList<String>();
        if (model != null) {
            for (EObject e : model.eContents()) {
                if (e instanceof FTypeCollection && !(e instanceof FInterface)) {
                    typeCollectionList.add(((FTypeCollection) e).getName());
                }
                if (e instanceof FInterface) {
                    interfaceList.add(((FInterface) e).getName());
                }
            }
            infoTriple = new Triple<String, ArrayList<String>, ArrayList<String>>(
                    ((FModelImpl) model).getName(), typeCollectionList,
                    interfaceList);
            allInfo.put(absolutPath, infoTriple);
        }
        fastAllInfo.clear();
        buildFastAllInfo();
    }

    private void buildAllInfo(String path) {

        File folder = new File(path);
        for (File file : folder.listFiles()) {
            if (file.isDirectory()) {
                if (!(file.getName().equals("bin") || file.equals(".settings")))
                    buildAllInfo(path + "/" + file.getName());
            }
            if (file.isFile()) {
                if (file.getName().endsWith(".fidl")) {
                    String cwd = "file:/" + path;
                    EObject model = buildResource(file.getName(), cwd);
                    ArrayList<String> typeCollectionList = new ArrayList<String>();
                    ArrayList<String> interfaceList = new ArrayList<String>();
                    if (model != null) {
                        for (EObject e : model.eContents()) {
                            if (e instanceof FTypeCollection
                                    && !(e instanceof FInterface)) {
                                typeCollectionList.add(((FTypeCollection) e)
                                        .getName());
                            }
                            if (e instanceof FInterface) {
                                interfaceList.add(((FInterface) e).getName());
                            }
                        }
                        infoTriple = new Triple<String, ArrayList<String>, ArrayList<String>>(
                                ((FModelImpl) model).getName(),
                                typeCollectionList, interfaceList);
                        allInfo.put(file.getAbsolutePath().replace("\\", "/"),
                                infoTriple);
                    }
                }
            }
        }
    }

    private EObject buildResource(String filename, String cwd) {

        URI fileURI = normalizeURI(URI.createURI(filename));
        URI cwdURI = normalizeURI(URI.createURI(cwd));
        Resource resource = null;

        if (cwd != null && cwd.length() > 0) {
            resourceSet
                    .getURIConverter()
                    .getURIMap()
                    .put(fileURI,
                            URI.createURI((cwdURI.toString() + "/" + fileURI
                                    .toString()).replaceAll("/+", "/")));
        }

        try {

            resource = resourceSet.getResource(fileURI, true);
            resource.load(Collections.EMPTY_MAP);
        } catch (IOException e) {
            return null;
        }
        try {
            return resource.getContents().get(0);
        } catch (Exception e) {
            return null;
        }
    }

    private static URI normalizeURI(URI path) {
        if (path.isFile()) {
            return URI.createURI(path.toString().replaceAll("\\\\", "/"));
        }
        return path;
    }

}
