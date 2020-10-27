/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
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
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.ISchedulingRule;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.emf.common.util.Diagnostic;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.jface.dialogs.MessageDialog;
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

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Provider;

public abstract class GenerationCommand extends AbstractHandler {

	@Inject
	protected Provider<EclipseResourceFileSystemAccess2> fileAccessProvider;
	@Inject
	private IResourceSetProvider resourceSetProvider;
	@Inject
	private IGenerator francaGenerator;

    private static String VALIDATION_MARKER = "org.genivi.commonapi.core.ui.VALIDATION_MARKER";

    /**
	 * Init core preferences
	 * @param file
	 * @param page
	 * @param project
	 */
	protected abstract void setupPreferences(IFile file);

	protected abstract void setupOutputDirectories(EclipseResourceFileSystemAccess2 fileSystemAccess);
    protected abstract List<Diagnostic> validateDeployment(List<FDModel> fdeplModels);

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
        final EclipseResourceFileSystemAccess2 fileSystemAccess = createFileSystemAccess();

        // Clear any already existing output from a previous command execution (e.g. errors) from the Franca console
        // by creating a new one or initializing an already existing one.
        //
        final SpecificConsole francaConsole = new SpecificConsole("Franca");
        Job job = new Job("Validation and generation") {

            @Override
            protected IStatus run(IProgressMonitor monitor)
            {
                boolean validationError = false;
                for (Iterator<?> iterator = structuredSelection.iterator(); iterator.hasNext(); )
                {
                    if (monitor.isCanceled())
                        return Status.CANCEL_STATUS;

                    final Object selectiobObject = iterator.next();
                    if (selectiobObject instanceof IFile)
                    {
                        final IFile file = (IFile) selectiobObject;
                        final URI uri = URI.createPlatformResourceURI(file.getFullPath().toString(), true);

                        monitor.beginTask(file.getName(), IProgressMonitor.UNKNOWN);
                        monitor.subTask("Validating");
                        deleteValidationMarkers(file);

                        // Validation of FIDL and FDEPL and all imported FIDL/FDEPL files.
                        try {
                            francaConsole.getOut().println("Validating " + getDisplayPath(uri));
                            if (!validate(uri, francaConsole))
                                validationError = true;
                        }
                        catch (Exception ex) {
                            validatorExceptionPopUp(ex, file);
                            outputCancelResult(francaConsole);
                            return Status.CANCEL_STATUS;
                        }
                    }
                }
                if (validationError)
                {
                    validatorErrorPopUp();
                    outputCancelResult(francaConsole);
                    return Status.CANCEL_STATUS;
                }

                for (Iterator<?> iterator = structuredSelection.iterator(); iterator.hasNext(); )
                {
                    if (monitor.isCanceled())
                        return Status.CANCEL_STATUS;

                    final Object selectiobObject = iterator.next();
                    if (selectiobObject instanceof IFile)
                    {
                        final IFile file = (IFile) selectiobObject;
                        final URI uri = URI.createPlatformResourceURI(file.getFullPath().toString(), true);
                        final ResourceSet rs = resourceSetProvider.get(file.getProject());
                        final Resource r = rs.getResource(uri, true);

                        IProject project = file.getProject();
                        fileSystemAccess.setProject(project);

                        setupPreferences(file);
                        // define output directories from the preferences just set before
                        setupOutputDirectories(fileSystemAccess);

                        monitor.beginTask(file.getName(), IProgressMonitor.UNKNOWN);
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
                            monitor.subTask("Generating");
                            try {
                                francaConsole.getOut().println("Generating " + getDisplayPath(uri));
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
                            continue;
                        }
                        markerPopUp(file, r);
                        outputCancelResult(francaConsole);
                        return Status.CANCEL_STATUS;
                    }
                }

                outputSuccessResult(francaConsole);
                return Status.OK_STATUS;
            }
        };
        job.setRule(new GeneratorRule());
        job.schedule();
    }

    class GeneratorRule implements ISchedulingRule
    {
        @Override
        public boolean contains(ISchedulingRule rule)
        {
            return rule == this || rule instanceof IResource;
        }

        @Override
        public boolean isConflicting(ISchedulingRule rule)
        {
            return rule instanceof GeneratorRule || rule instanceof IResource;
        }
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

    void deleteValidationMarkers(IResource resource)
    {
        try
        {
            // Validation markers may get generated for all FDEPL within the project, depending
            // on the "imports" used. Therefore, we can't know which validation markers really
            // relate to one particular resource. Therefore we have to delete all validation markers
            // within the project that is associated with that resource. That's reasonable because
            // the nature of the validation is "sort of" a project scope anyway.
            //
            // This does not effect the file based validation which is performed by XText/Franca validators.
            //
            IMarker[] markers = resource.getProject().findMarkers(IMarker.PROBLEM, false, 2);
            for (IMarker marker : markers)
            {
                // Only delete deployment validation markers, not any other problem markers.
                if (marker.getAttribute(VALIDATION_MARKER, false))
                    marker.delete();
            }
        }
        catch (CoreException ex)
        {
            ex.printStackTrace();
        }
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
						"Couldn't generate file(s) for \"" + file.getFullPath().toPortableString() + "\". Exception occured:\n"
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
                        "Couldn't generate file(s). File \"" + file.getFullPath().toPortableString() + "\" still holds errors!\n\nSee Problems view for details.");
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
						"Couldn't generate file(s) for \"" + file.getFullPath().toPortableString() + "\". Error occured:\n"
								+ er.toString()
								+ "\n\nSee console for stack trace.");
			}
		});
	}

	private void validatorErrorPopUp()
    {
        Display.getDefault().asyncExec(new Runnable() {
            @Override
            public void run() {
                MessageDialog.openError(
                        null,
                        "Error",
                        "Validation failed.\n\nSee console view for details.");
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
                        "Couldn't validate file \"" + file.getFullPath().toPortableString() + "\" Exception occured:\n"
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

            addValidationMarkers(issues);

            if (diags.size() > 0)
                francaConsole.getOut().println("Validation of deployment finished with: " + numErrors + " errors, " + numWarnings + " warnings.");
        }

        return !hasValidationError;
    }

	void addValidationMarkers(List<Issue> issues)
    {
        IWorkspaceRoot workspaceRoot = ResourcesPlugin.getWorkspace().getRoot();
        for (Issue issue : issues)
        {
            try
            {
                URI uri = issue.getUriToProblem();
                if (uri.isPlatform())
                {
                    IResource resource = workspaceRoot.findMember(uri.toPlatformString(true));
                    if (resource != null)
                    {
                        IMarker marker = resource.createMarker(IMarker.PROBLEM);
                        if (marker != null)
                        {
                            // Tag our markers, so we can find them again.
                            marker.setAttribute(VALIDATION_MARKER, true);

                            if (issue.getSeverity() == Severity.ERROR)
                                marker.setAttribute(IMarker.SEVERITY, IMarker.SEVERITY_ERROR);
                            else if (issue.getSeverity() == Severity.WARNING)
                                marker.setAttribute(IMarker.SEVERITY, IMarker.SEVERITY_WARNING);
                            else
                                marker.setAttribute(IMarker.SEVERITY, IMarker.SEVERITY_INFO);

                            marker.setAttribute(IMarker.MESSAGE, issue.getMessage());
                            marker.setAttribute(IMarker.PRIORITY, IMarker.PRIORITY_HIGH);
                            marker.setAttribute(IMarker.LINE_NUMBER, issue.getLineNumber());
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                ex.printStackTrace();
            }
        }
    }

}

