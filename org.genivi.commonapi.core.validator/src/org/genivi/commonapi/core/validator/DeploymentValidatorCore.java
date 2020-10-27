/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.validator;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.common.util.BasicDiagnostic;
import org.eclipse.emf.common.util.Diagnostic;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.xtext.validation.FeatureBasedDiagnostic;
import org.eclipse.xtext.validation.ValidationMessageAcceptor;
import org.franca.deploymodel.dsl.fDeploy.FDModel;
import org.franca.deploymodel.dsl.validation.IFDeployExternalValidator;
import org.genivi.commonapi.core.preferences.PreferenceConstants;
import org.genivi.commonapi.core.ui.CommonApiUiPlugin;
import org.genivi.commonapi.core.verification.DeploymentValidator;

public class DeploymentValidatorCore implements IFDeployExternalValidator {

	@Override
	public void validateModel(FDModel fdepl, ValidationMessageAcceptor messageAcceptor) {
		try {
			if (!isDeploymentValidatorEnabled())
			{
				return;
			}
			List<FDModel> modelList = new ArrayList<FDModel>();
			modelList.add(fdepl);
			DeploymentValidator validator = new DeploymentValidator();
			BasicDiagnostic diagnostics = new BasicDiagnostic();
			validator.validate(modelList, diagnostics);
			// copy the diagnostics to the message acceptor
			for (Diagnostic diagnostic: diagnostics.getChildren() ) {
				if (diagnostic instanceof FeatureBasedDiagnostic) {
					FeatureBasedDiagnostic fd = (FeatureBasedDiagnostic) diagnostic;
					int severity = fd.getSeverity();
					if (severity == Diagnostic.WARNING) {
						messageAcceptor.acceptWarning(fd.getMessage(), fd.getSourceEObject(), fd.getFeature(), -1, fd.getIssueCode(), fd.getIssueData());
					}
					if (severity == Diagnostic.ERROR) {
						messageAcceptor.acceptError(fd.getMessage(), fd.getSourceEObject(), fd.getFeature(), -1, fd.getIssueCode(), fd.getIssueData());
					}
					if (severity == Diagnostic.INFO) {
						messageAcceptor.acceptInfo(fd.getMessage(), fd.getSourceEObject(), fd.getFeature(), -1, fd.getIssueCode(), fd.getIssueData());
					}
				}
			}
		}
		catch (Exception ex) {
			ex.printStackTrace();
			throw ex;
		}
	}
	public boolean isDeploymentValidatorEnabled()
	{
		IPreferenceStore prefs = CommonApiUiPlugin.getValidatorPreferences();
		return prefs != null && prefs.getBoolean(PreferenceConstants.P_ENABLE_CORE_DEPLOYMENT_VALIDATOR);
	}
}
