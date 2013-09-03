/*
 * Copyright (C) 2013 BMW Group Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de) This Source Code Form is
 * subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the
 * MPL was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 */

package org.genivi.commonapi.core.ui.preferences;

import org.eclipse.cdt.ui.newui.MultiLineTextFieldEditor;
import org.eclipse.swt.widgets.Composite;

public class MultiLineTextField extends MultiLineTextFieldEditor {

    public MultiLineTextField() {
        super();
    }

    public MultiLineTextField(String name, String labelText, Composite parent) {
        super(name, labelText, parent);
    }

    public MultiLineTextField(String name, String labelText, int width, Composite parent) {
        super(name, labelText, width, parent);
    }

    public MultiLineTextField(String name, String labelText, int width, int strategy, Composite parent) {
        super(name, labelText, width, strategy, parent);
    }

    @Override
    public void setEnabled(boolean enabled, Composite parent) {
        // super.setEnabled(enabled, parent);
        getTextControl(parent).setEnabled(enabled);
    }
}