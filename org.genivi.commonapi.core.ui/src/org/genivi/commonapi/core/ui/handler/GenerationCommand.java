/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.ui.handler;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;

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
import org.eclipse.emf.common.util.Diagnostic;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.console.MessageConsoleStream;
import org.eclipse.ui.handlers.HandlerUtil;
import org.eclipse.xtext.builder.EclipseResourceFileSystemAccess2;
import org.eclipse.xtext.diagnostics.Severity;
import org.eclipse.xtext.generator.IGenerator;
import org.eclipse.xtext.resource.IResourceServiceProvider;
import org.eclipse.xtext.resource.XtextResource;
import org.eclipse.xtext.ui.editor.XtextEditor;
import org.eclipse.xtext.ui.resource.IResourceSetProvider;
import org.eclipse.xtext.util.IAcceptor;
import org.eclipse.xtext.util.concurrent.IUnitOfWork;
import org.eclipse.xtext.validation.CheckMode;
import org.eclipse.xtext.validation.DiagnosticConverterImpl;
import org.eclipse.xtext.validation.IResourceValidator;
import org.eclipse.xtext.validation.Issue;
import org.franca.core.dsl.FrancaIDLRuntimeModule;
import org.franca.core.dsl.FrancaImportsProvider;
import org.franca.core.dsl.ui.util.SpecificConsole;
import org.franca.core.utils.ModelPersistenceHandler;
import org.franca.deploymodel.dsl.FDeployImportsProvider;
import org.franca.deploymodel.dsl.fDeploy.FDModel;
import org.genivi.commonapi.core.preferences.FPreferences;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;

import com.google.inject.Guice;
import com.google.inject.Injector;
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
		final EclipseResourceFileSystemAccess2 fileSystemAccess = createFileSystemAccess();

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
				fileSystemAccess.setProject(project);

				setupPreferences(file);
				// define output directories from the preferences just set before
				setupOutputDirectories(fileSystemAccess);
				
				// Clear any already existing output from a previous command execution (e.g. errors) from the Franca console
				// by creating a new one or initializing an already existing one.
				//
                final SpecificConsole francaConsole = new SpecificConsole("Franca");
                francaConsole.getOut().println("Loading " + file.getFullPath().toPortableString());

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

						// Validation of FIDL and FDEPL and all imported FIDL/FDEPL files.
						try {
						    if (!validate(uri, francaConsole)) {
						        validatorErrorPopUp(file);
						        outputCancelResult(francaConsole);
						        return Status.CANCEL_STATUS;
						    }
						}
                        catch (Exception ex) {
                            validatorExceptionPopUp(ex, file);
                            outputCancelResult(francaConsole);
                            return Status.CANCEL_STATUS;
                        }

						if (r.getErrors().size() == 0 && i == 0) {
							monitor.subTask("Generate");
							try {
								francaGenerator.doGenerate(r, fileSystemAccess);
							} catch (Exception e) {
 								exceptionPopUp(e, file);
 	                            outputCancelResult(francaConsole);
								return Status.CANCEL_STATUS;
							} catch (Error e) {
								errorPopUp(e, file);
	                            outputCancelResult(francaConsole);
								return Status.CANCEL_STATUS;
							}
							outputSuccessResult(francaConsole);
							return Status.OK_STATUS;
						}
						markerPopUp(file, r);
						outputCancelResult(francaConsole);
						return Status.CANCEL_STATUS;
					}

				};
				job.schedule();
				// wait for this job to end
				try {
					job.join();
				} catch (InterruptedException e) {
					exceptionPopUp(e, file);
				}
			}
		}
	}

	protected void setupOutputDirectories(EclipseResourceFileSystemAccess2 fileSystemAccess) {
		fileSystemAccess.setOutputConfigurations(FPreferences.getInstance().getOutputpathConfiguration());
	}

	protected boolean validate(URI resourcePathUri, SpecificConsole francaConsole)
	{
        // Load the FIDL and all imported FIDL files, or the FDEPL and all imported FDEPL/FIDL files.
	    //
        ModelPersistenceHandler.registerFileExtensionHandler("fidl", new FrancaImportsProvider());
        ModelPersistenceHandler.registerFileExtensionHandler("fdepl", new FDeployImportsProvider());
        Injector injector = Guice.createInjector(new FrancaIDLRuntimeModule());
        ModelPersistenceHandler modelPersistenceHandler = new ModelPersistenceHandler(injector.getInstance(ResourceSet.class));
        URI rootPathUri = URI.createPlatformResourceURI("/", true);
        modelPersistenceHandler.loadModel(resourcePathUri, rootPathUri);

        if ("fdepl".equals(resourcePathUri.fileExtension()))
        {
            if (!validateDeployment(modelPersistenceHandler.getResourceSet(), francaConsole))
                return false;
        }

        // Validate all loaded FDEPL and FIDL files.
        francaConsole.getOut().println("Validating " + getDisplayPath(resourcePathUri));
        return validate(modelPersistenceHandler.getResourceSet(), francaConsole);
	}

	protected boolean validate(ResourceSet resourceSet, SpecificConsole francaConsole)
    {
        boolean hasValidationError = false;
        for (Resource resource : resourceSet.getResources())
        {
            //francaConsole.getOut().println("Validating " + getDisplayPath(resource.getURI()));
            if (!validate(resource, francaConsole))
                hasValidationError = true;
        }
        return !hasValidationError;
    }

    protected boolean validate(Resource resource, SpecificConsole francaConsole)
    {
        boolean hasValidationError = false;
        IResourceServiceProvider resourceServiceProvider = IResourceServiceProvider.Registry.INSTANCE.getResourceServiceProvider(resource.getURI());
        if (resourceServiceProvider != null)
        {
            IResourceValidator resourceValidator = resourceServiceProvider.getResourceValidator();
            Collection<Issue> issues = resourceValidator.validate(resource, CheckMode.ALL, null);
            if (!outputIssues(issues, francaConsole))
                hasValidationError = true;
        }
        return !hasValidationError;
    }

    private boolean outputIssues(Collection<Issue> issues, SpecificConsole francaConsole)
    {
        boolean hasValidationError = false;
        for (Issue issue : issues)
        {
            if (issue.getSeverity() == Severity.ERROR) {
                hasValidationError = true;
                francaConsole.getErr().println(issue.toString());
            }
            else if (issue.getSeverity() == Severity.WARNING)
                francaConsole.getOut().println(issue.toString());
        }
        return !hasValidationError;
    }

    String getDisplayPath(URI uri)
    {
        String displayPath = null;
        if (uri.isPlatformResource())
            displayPath = uri.toPlatformString(true);
        if (displayPath == null)
            displayPath = uri.toFileString();
        if (displayPath == null)
            displayPath = uri.toString();
        return displayPath;
    }

	private void outputSuccessResult(SpecificConsole console)
	{
        console.getOut().println("Code generation finished successfully.");
	}

    private void outputCancelResult(SpecificConsole console)
    {
        console.getOut().println("Code generation aborted.");
    }

    protected boolean validateDeployment(ResourceSet resourceSet, SpecificConsole francaConsole)
    {
        boolean hasValidationError = false;

        List<FDModel> fdeplModels = new ArrayList<FDModel>();
        for (Resource resource : resourceSet.getResources())
        {
            for (EObject eObject : resource.getContents())
            {
                if (eObject instanceof FDModel)
                {
                    fdeplModels.add((FDModel)eObject);
                }
            }
        }

        //francaConsole.getOut().println("Validating deployment");

        List<Diagnostic> diags = validateDeployment(fdeplModels);
        if (diags != null)
        {
            int numErrors = 0, numWarnings = 0;
            final List<Issue> issues = new ArrayList<Issue>();
            IAcceptor<Issue> acceptor = new IAcceptor<Issue>()
            {
                @Override
                public void accept(Issue issue)
                {
                    issues.add(issue);
                }
            };
            DiagnosticConverterImpl converter = new DiagnosticConverterImpl();
            for (Diagnostic diag : diags)
            {
                converter.convertValidatorDiagnostic(diag, acceptor);
                if (diag.getSeverity() == Diagnostic.ERROR)
                    numErrors++;
                else if (diag.getSeverity() == Diagnostic.WARNING)
                    numWarnings++;
            }
            if (!outputIssues(issues, francaConsole))
                hasValidationError = true;
            francaConsole.getOut().println("Validaton of deployment finished with: " + numErrors + " errors, " + numWarnings + " warnings.");
        }

        return !hasValidationError;
    }

    protected List<Diagnostic> validateDeployment(List<FDModel> fdepls)
    {
        return null;
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
			@Override
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
	    markerPopUp(f, null);
	}

	private void markerPopUp(final IFile file, final Resource resource)
    {
        Display.getDefault().asyncExec(new Runnable() {
            @Override
            public void run() {
                if (resource != null && resource.getErrors().size() > 0)
                {
                    SpecificConsole francaConsole = new SpecificConsole("Franca");
                    MessageConsoleStream out = francaConsole.getOut();
                    for (org.eclipse.emf.ecore.resource.Resource.Diagnostic diag : resource.getErrors())
                    {
                        StringBuffer msg = new StringBuffer();
                        msg.append(diag.getLocation() != null ? diag.getLocation() : file.getFullPath());
                        msg.append(":");
                        msg.append(diag.getLine());
                        msg.append(" ");
                        msg.append(diag.getMessage());
                        out.println(msg.toString());
                    }
                }
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
			@Override
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

	private void validatorErrorPopUp(final IFile file)
    {
        Display.getDefault().asyncExec(new Runnable() {
            @Override
            public void run() {
                MessageDialog.openError(
                        null,
                        "Error by validating file " + file.getName(),
                        "Couldn't validate file.\n\nSee console view for details.");
            }
        });
    }

    private void validatorExceptionPopUp(final Exception ex, final IFile file) {
        Display.getDefault().asyncExec(new Runnable() {
            @Override
            public void run() {
                ex.printStackTrace();
                String exMessage = null;
                if (ex.getCause() != null)
                    exMessage = ex.getCause().getMessage();
                if (exMessage == null)
                    exMessage = ex.getMessage();

                SpecificConsole francaConsole = new SpecificConsole("Franca");
                MessageConsoleStream out = francaConsole.getOut();
                out.println("ERROR: " + exMessage);

                MessageDialog.openError(
                        null,
                        "Error by validating file " + file.getName(),
                        "Couldn't validate file. Exception occured:\n"
                                + exMessage
                                + "\n\nSee console view for details.");
            }
        });
    }

	protected EclipseResourceFileSystemAccess2 createFileSystemAccess() {

		final EclipseResourceFileSystemAccess2 fsa = fileAccessProvider.get();

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
		String generateCommon = null;
		String generateProxy = null;
		String generateStub = null;
		String generateSkeleton = null;
		String generateDependencies = null;
		String skeletonPostfix = null;
		String enumPrefix = null;
		String generateSyncCalls = null;

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
			generateCommon = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_COMMON));
			generateProxy = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_PROXY));
			generateStub = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_STUB));
			generateSkeleton = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_SKELETON));
			generateDependencies = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_DEPENDENCIES));
			generateSyncCalls = resource.getPersistentProperty(new QualifiedName(PreferenceConstants.PROJECT_PAGEID, PreferenceConstants.P_GENERATE_SYNC_CALLS));
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
		if (generateCommon == null) {
			generateCommon = store.getString(PreferenceConstants.P_GENERATE_COMMON);
		}
		if(generateProxy == null) {
			generateProxy = store.getString(PreferenceConstants.P_GENERATE_PROXY);
		}
		if(generateStub == null) {
			generateStub = store.getString(PreferenceConstants.P_GENERATE_STUB);
		}
		if(generateSkeleton == null) {
			generateSkeleton = store.getString(PreferenceConstants.P_GENERATE_SKELETON);
		}
		if(generateDependencies == null) {
			generateDependencies = store.getString(PreferenceConstants.P_GENERATE_DEPENDENCIES);
		}
		if(generateSyncCalls == null) {
			generateSyncCalls = store.getString(PreferenceConstants.P_GENERATE_SYNC_CALLS);
		}
		// finally, store the properties for the code generator
		instance.setPreference(PreferenceConstants.P_OUTPUT_COMMON, outputFolderCommon);
		instance.setPreference(PreferenceConstants.P_OUTPUT_PROXIES, outputFolderProxies);
		instance.setPreference(PreferenceConstants.P_OUTPUT_STUBS, outputFolderStubs);
		instance.setPreference(PreferenceConstants.P_OUTPUT_SKELETON, outputFolderSkeleton);
		instance.setPreference(PreferenceConstants.P_LICENSE, licenseHeader);
		instance.setPreference(PreferenceConstants.P_GENERATE_COMMON, generateCommon);
		instance.setPreference(PreferenceConstants.P_GENERATE_PROXY, generateProxy);
		instance.setPreference(PreferenceConstants.P_GENERATE_STUB, generateStub);
		instance.setPreference(PreferenceConstants.P_GENERATE_SKELETON, generateSkeleton);
		instance.setPreference(PreferenceConstants.P_SKELETONPOSTFIX, skeletonPostfix);
		instance.setPreference(PreferenceConstants.P_ENUMPREFIX, enumPrefix);
		instance.setPreference(PreferenceConstants.P_GENERATE_DEPENDENCIES, generateDependencies);
		instance.setPreference(PreferenceConstants.P_GENERATE_SYNC_CALLS, generateSyncCalls);
	}

}
