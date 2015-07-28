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
import org.eclipse.core.runtime.QualifiedName;
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
import org.eclipse.xtext.generator.IGenerator;
import org.eclipse.xtext.resource.XtextResource;
import org.eclipse.xtext.ui.editor.XtextEditor;
import org.eclipse.xtext.ui.resource.IResourceSetProvider;
import org.eclipse.xtext.util.concurrent.IUnitOfWork;
import org.genivi.commonapi.core.preferences.FPreferences;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;

import com.google.inject.Provider;

public class GenerationCommand extends AbstractHandler {

	@Inject
	protected Provider<EclipseResourceFileSystemAccess2> fileAccessProvider;
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

				setupPreferences(file);               

				final EclipseResourceFileSystemAccess2 fileSystemAccess = createFileSystemAccess();
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

	/**
	 * Init core preferences
	 * @param file 
	 * @param page
	 * @param project
	 */
	protected void setupPreferences(IFile file) {

		initPreferences(file, CommonApiUiPlugin.getDefault().getPreferenceStore());
	}

	private void executeGeneratorForXtextEditor(final XtextEditor xtextEditor) {
		final Object fileObject = xtextEditor.getEditorInput().getAdapter(
				IFile.class);
		if (fileObject instanceof IFile) {
			IProject project = ((IResource) fileObject).getProject();
			final EclipseResourceFileSystemAccess2 fileSystemAccess = createFileSystemAccess();
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

	protected EclipseResourceFileSystemAccess2 createFileSystemAccess() {

		final EclipseResourceFileSystemAccess2 fsa = fileAccessProvider.get();

		fsa.setOutputConfigurations(FPreferences.getInstance().getOutputpathConfiguration());

		fsa.setMonitor(new NullProgressMonitor());

		return fsa;
	}

	/**
	 * Set the properties for the code generation from the resource properties (set with the property page, via the context menu).
	 * Take default values from the eclipse preference page.
	 * @param file 
	 * @param store - the eclipse preference store
	 */
	private void initPreferences(IFile file, IPreferenceStore store) {
		FPreferences instance = FPreferences.getInstance();
		
		String outputFolderCommon = null;
		String outputFolderProxies = null;
		String outputFolderStubs = null;
		String outputFolderSkeleton = null;
		String licenseHeader = null;
		String generateProxy = null;
		String generatStub = null;
		String generatSkeleton = null;
		String skeletonPostfix = null;
		String enumPrefix = null;
		
		IProject project = file.getProject();
		IResource resource = file;

		try {
			// Should project or file specific properties be used ?
			String useProject1 = project.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_USEPROJECTSETTINGS));
			String useProject2 = file.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_USEPROJECTSETTINGS));
			if("true".equals(useProject1) || "true".equals(useProject2)) {
				resource = project;
			} 
			outputFolderCommon = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT_COMMON));
			outputFolderSkeleton = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT_SKELETON));
			outputFolderProxies = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT_PROXIES));
			outputFolderStubs = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_OUTPUT_STUBS));
			licenseHeader = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_LICENSE));
			generateProxy = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATEPROXY));
			generatStub = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATESTUB));
			generatSkeleton = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATESKELETON));
			skeletonPostfix = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_SKELETONPOSTFIX));
			enumPrefix = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_ENUMPREFIX));
		} catch (CoreException ce) {
			System.err.println("Failed to get property for " + resource.getName());
		}  
		
		// Set defaults from the preference store in the case, where the value was not specified in the properties.
		if(outputFolderCommon == null) {
			outputFolderCommon = store.getString(PreferenceConstants.P_OUTPUT_COMMON);			
		}
		if(outputFolderProxies == null) {
			outputFolderProxies = store.getString(PreferenceConstants.P_OUTPUT_PROXIES);			
		}
		if(outputFolderStubs == null) {
			outputFolderStubs = store.getString(PreferenceConstants.P_OUTPUT_STUBS);	
		}
		if(outputFolderSkeleton == null) {
			outputFolderSkeleton = store.getString(PreferenceConstants.P_OUTPUT_SKELETON);	
		}
		if(skeletonPostfix == null) {
			skeletonPostfix = store.getString(PreferenceConstants.P_SKELETONPOSTFIX);	
		}
		if(enumPrefix == null) {
			enumPrefix = store.getString(PreferenceConstants.P_ENUMPREFIX);	
		}
		if(licenseHeader == null) {
			licenseHeader = store.getString(PreferenceConstants.P_LICENSE);			
		}
		if(generateProxy == null) {
			generateProxy = store.getString(PreferenceConstants.P_GENERATEPROXY);
		}
		if(generatStub == null) {
			generatStub = store.getString(PreferenceConstants.P_GENERATESTUB);
		}
		if(generatSkeleton == null) {
			generatSkeleton = store.getString(PreferenceConstants.P_GENERATESKELETON);
		}
		
		// finally, store the properties for the code generator
		instance.setPreference(PreferenceConstants.P_OUTPUT_COMMON, outputFolderCommon);
		instance.setPreference(PreferenceConstants.P_OUTPUT_PROXIES, outputFolderProxies);
		instance.setPreference(PreferenceConstants.P_OUTPUT_STUBS, outputFolderStubs);
		instance.setPreference(PreferenceConstants.P_OUTPUT_SKELETON, outputFolderSkeleton);
		instance.setPreference(PreferenceConstants.P_LICENSE, licenseHeader);
		instance.setPreference(PreferenceConstants.P_GENERATEPROXY, generateProxy);
		instance.setPreference(PreferenceConstants.P_GENERATESTUB, generatStub);
		instance.setPreference(PreferenceConstants.P_GENERATESKELETON, generatSkeleton);
		instance.setPreference(PreferenceConstants.P_SKELETONPOSTFIX, skeletonPostfix);
		instance.setPreference(PreferenceConstants.P_ENUMPREFIX, enumPrefix);
	}    

}
