/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.ui.handler;

import java.util.Iterator;

import javax.inject.Inject;

import org.eclipse.core.commands.AbstractHandler;
import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
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

import com.google.inject.Provider;

public class GenerationCommand extends AbstractHandler {
	public static final String OUTPUT_DIRECTORY = "src-gen"; 

	@Inject private Provider<EclipseResourceFileSystemAccess2> fileAccessProvider;
	@Inject private IResourceDescriptions resourceDescriptions;
	@Inject private IResourceSetProvider resourceSetProvider;
	@Inject private IGenerator francaGenerator;

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
				throw new ExecutionException("Cannot handle ExecutionEvent: " + event);
		}

		return null;
	}

	private void executeGeneratorForSelection(final IStructuredSelection structuredSelection) {
		final EclipseResourceFileSystemAccess2 fileSystemAccess = createFileSystemAccess();

		for (Iterator<?> iterator = structuredSelection.iterator(); iterator.hasNext();) {
			final Object selectiobObject = iterator.next();
			if (selectiobObject instanceof IFile) {
				final IFile file = (IFile) selectiobObject;
				final URI uri = URI.createPlatformResourceURI(file.getFullPath().toString(), true);
				final ResourceSet rs = resourceSetProvider.get(file.getProject());
				final Resource r = rs.getResource(uri, true);

				fileSystemAccess.setProject(file.getProject());
				francaGenerator.doGenerate(r, fileSystemAccess);
			}
		}
	}

	private void executeGeneratorForXtextEditor(final XtextEditor xtextEditor) {
		final Object fileObject = xtextEditor.getEditorInput().getAdapter(IFile.class);
		if (fileObject instanceof IFile) {
			final EclipseResourceFileSystemAccess2 fileSystemAccess = createFileSystemAccess();
			fileSystemAccess.setProject(((IResource) fileObject).getProject());

			xtextEditor.getDocument().readOnly(
					new IUnitOfWork<Boolean, XtextResource>() {
						@Override
						public Boolean exec(XtextResource xtextResource) throws Exception {
							francaGenerator.doGenerate(xtextResource, fileSystemAccess);
							return Boolean.TRUE;
						}
					});
		}
	}
	
	private EclipseResourceFileSystemAccess2 createFileSystemAccess() {
		final EclipseResourceFileSystemAccess2 fsa = fileAccessProvider.get();

		fsa.setOutputPath(OUTPUT_DIRECTORY);
		fsa.getOutputConfigurations().get(IFileSystemAccess.DEFAULT_OUTPUT).setCreateOutputDirectory(true);
		fsa.setMonitor(new NullProgressMonitor());

		return fsa;
	}
}
