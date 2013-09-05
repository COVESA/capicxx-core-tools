/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.ui.handler;

import java.util.Iterator;

import javax.inject.Inject;

import org.eclipse.core.commands.AbstractHandler;
import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IMarker;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.handlers.HandlerUtil;
import org.eclipse.xtext.builder.EclipseResourceFileSystemAccess2;
import org.eclipse.xtext.generator.IFileSystemAccess;
import org.eclipse.xtext.generator.IGenerator;
import org.eclipse.xtext.resource.IResourceDescriptions;
import org.eclipse.xtext.resource.XtextResource;
import org.eclipse.xtext.ui.editor.XtextEditor;
import org.eclipse.xtext.ui.resource.IResourceSetProvider;
import org.eclipse.xtext.util.concurrent.IUnitOfWork;
import org.genivi.commonapi.core.preferences.FPreferences;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;
import org.genivi.commonapi.core.ui.preferences.CommonAPIPreferencePage;
import org.genivi.commonapi.core.ui.preferences.FieldEditorOverlayPage;

import com.google.inject.Provider;

public class GenerationCommand extends AbstractHandler {

    @Inject
    private Provider<EclipseResourceFileSystemAccess2> fileAccessProvider;
    @Inject
    private IResourceDescriptions resourceDescriptions;
    @Inject
    private IResourceSetProvider resourceSetProvider;
    @Inject
    private IGenerator francaGenerator;

    @Override
    public Object execute(final ExecutionEvent event) throws ExecutionException {
        final ISelection selection = HandlerUtil.getCurrentSelection(event);
        if (selection instanceof IStructuredSelection) {
            if (!selection.isEmpty())
                executeGeneratorForSelection((IStructuredSelection) selection);
        } else {
            final IEditorPart activeEditor = HandlerUtil.getActiveEditor(event);
            if (activeEditor instanceof XtextEditor)
                executeGeneratorForXtextEditor((XtextEditor) activeEditor);
            else
                throw new ExecutionException("Cannot handle ExecutionEvent: "
                        + event);
        }

        return null;
    }

    private void executeGeneratorForSelection(
            final IStructuredSelection structuredSelection) {
        IProject project = null;
        for (Iterator<?> iterator = structuredSelection.iterator(); iterator
                .hasNext();) {
            final Object selectiobObject = iterator.next();
            if (selectiobObject instanceof IFile) {
                final IFile file = (IFile) selectiobObject;
                final URI uri = URI.createPlatformResourceURI(file
                        .getFullPath().toString(), true);
                final ResourceSet rs = resourceSetProvider.get(file
                        .getProject());
                final Resource r = rs.getResource(uri, true);

                project = file.getProject();
                FieldEditorOverlayPage page = new CommonAPIPreferencePage();
                page.setElement(project);
                page.createControl(null);
                FPreferences.init(page.getPreferenceStore(), CommonApiUiPlugin
                        .getDefault().getPreferenceStore(), project);
                final EclipseResourceFileSystemAccess2 fileSystemAccess = createFileSystemAccess(project);
                fileSystemAccess.setProject(project);
                Job job = new Job("validation and generation") {

                    @Override
                    protected IStatus run(IProgressMonitor monitor) {
                        monitor.beginTask("handle " + file.getName(),
                                IProgressMonitor.UNKNOWN);
                        monitor.subTask("validation");
                        int i = 0;
                        try {
                            for (IMarker m : ((IResource) file).findMarkers(
                                    IMarker.PROBLEM, true, 2)) {
                                if ((Integer) m.getAttribute(IMarker.SEVERITY) == IMarker.SEVERITY_ERROR) {
                                    i++;
                                    break;
                                }
                            }

                        } catch (CoreException ce) {
                        }
                        if (r.getErrors().size() == 0 && i == 0) {
                            monitor.subTask("Generate");
                            try {
                                francaGenerator.doGenerate(r, fileSystemAccess);
                            } catch (Exception e) {
                                exceptionPopUp(e, file);
                                return Status.CANCEL_STATUS;
                            } catch (Error e) {
                                errorPopUp(e, file);
                                return Status.CANCEL_STATUS;
                            }
                            return Status.OK_STATUS;
                        } else {
                            markerPopUp(file);
                            return Status.CANCEL_STATUS;
                        }

                    }

                };
                job.schedule();
            }
        }
    }

    private void executeGeneratorForXtextEditor(final XtextEditor xtextEditor) {
        final Object fileObject = xtextEditor.getEditorInput().getAdapter(
                IFile.class);
        if (fileObject instanceof IFile) {
            IProject project = ((IResource) fileObject).getProject();
            final EclipseResourceFileSystemAccess2 fileSystemAccess = createFileSystemAccess(project);
            fileSystemAccess.setProject(project);

            xtextEditor.getDocument().readOnly(
                    new IUnitOfWork<Boolean, XtextResource>() {
                        @Override
                        public Boolean exec(XtextResource xtextResource)
                                throws Exception {
                            final XtextResource xtextRes = xtextResource;
                            Job job = new Job("validation and generation") {

                                @Override
                                protected IStatus run(IProgressMonitor monitor) {

                                    monitor.beginTask(
                                            "handle "
                                                    + ((IResource) fileObject)
                                                            .getName(),
                                            IProgressMonitor.UNKNOWN);
                                    monitor.subTask("validation");
                                    int i = 0;
                                    try {
                                        for (IMarker m : ((IResource) fileObject)
                                                .findMarkers(IMarker.PROBLEM,
                                                        true, 2)) {
                                            if ((Integer) m
                                                    .getAttribute(IMarker.SEVERITY) == IMarker.SEVERITY_ERROR) {
                                                i++;
                                                break;
                                            }
                                        }
                                    } catch (CoreException ce) {
                                    }
                                    if (xtextRes.getErrors().size() == 0
                                            && i == 0) {
                                        monitor.subTask("Generate");
                                        try {
                                            francaGenerator.doGenerate(
                                                    xtextRes, fileSystemAccess);
                                        } catch (Exception e) {
                                            exceptionPopUp(e,
                                                    (IFile) fileObject);
                                            return Status.CANCEL_STATUS;
                                        } catch (Error e) {
                                            errorPopUp(e, (IFile) fileObject);
                                            return Status.CANCEL_STATUS;
                                        }
                                        return Status.OK_STATUS;
                                    } else {
                                        markerPopUp((IFile) fileObject);
                                        return Status.CANCEL_STATUS;
                                    }
                                }

                            };
                            job.schedule();
                            return Boolean.TRUE;
                        }
                    });
        }
    }

    private void exceptionPopUp(Exception e, IFile f) {
        final Exception ex = e;
        final IFile file = f;
        Display.getDefault().asyncExec(new Runnable() {
            public void run() {
                ex.printStackTrace();
                MessageDialog.openError(
                        null,
                        "Error by generating file " + file.getName(),
                        "Couldn't generate file. Exception occured:\n"
                                + ex.toString()
                                + "\n\nSee console for stack trace.");
            }
        });
    }

    private void markerPopUp(IFile f) {
        final IFile file = f;
        Display.getDefault().asyncExec(new Runnable() {
            public void run() {
                MessageDialog.openError(
                        null,
                        "Error in file " + file.getName(),
                        "Couldn't generate file. File still holds errors!\n\nSee Problems view for details.");
            }
        });
    }

    private void errorPopUp(Error e, IFile f) {
        final Error er = e;
        final IFile file = f;
        Display.getDefault().asyncExec(new Runnable() {
            public void run() {
                er.printStackTrace();
                MessageDialog.openError(
                        null,
                        "Error by generating file " + file.getName(),
                        "Couldn't generate file. Error occured:\n"
                                + er.toString()
                                + "\n\nSee console for stack trace.");
            }
        });
    }

    private EclipseResourceFileSystemAccess2 createFileSystemAccess(
            IProject project) {
        IPreferenceStore store = CommonApiUiPlugin.getDefault()
                .getPreferenceStore();
        String outputDir = store.getString(PreferenceConstants.P_OUTPUT);
        if (FPreferences.getInstance().getPreference(
                PreferenceConstants.P_OUTPUT, null,
                project.getFullPath().toPortableString()) != null)
            outputDir = FPreferences.getInstance().getPreference(
                    PreferenceConstants.P_OUTPUT, null,
                    project.getFullPath().toPortableString());
        final EclipseResourceFileSystemAccess2 fsa = fileAccessProvider.get();

        fsa.setOutputPath(outputDir);
        fsa.getOutputConfigurations().get(IFileSystemAccess.DEFAULT_OUTPUT)
                .setCreateOutputDirectory(true);
        fsa.setMonitor(new NullProgressMonitor());

        return fsa;
    }
}
