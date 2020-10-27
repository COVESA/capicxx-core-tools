/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.ui.preferences;

import org.eclipse.jface.preference.FieldEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

public class MultiLineStringFieldEditor extends FieldEditor
{
    private Text   textControl;
    private String oldTextValue;

    public MultiLineStringFieldEditor(String name, String labelText, int width, Composite parent)
    {
        init(name, labelText);
        createControl(parent);
    }

    @Override
    protected void adjustForNumColumns(int numColumns)
    {
        GridData gdLabel = (GridData) getLabelControl().getLayoutData();
        gdLabel.horizontalSpan = numColumns;

        GridData gdText = (GridData) textControl.getLayoutData();
        gdText.horizontalSpan = numColumns;
    }

    @Override
    public int getNumberOfControls()
    {
        return 1;
    }

    @Override
    protected void doFillIntoGrid(Composite parent, int numColumns)
    {
        GridData gdParent = new GridData(GridData.FILL_HORIZONTAL);
        parent.setLayoutData(gdParent);

        Label labelControl = getLabelControl(parent);
        GridData gdLabel = new GridData();
        labelControl.setLayoutData(gdLabel);

        textControl = getTextControl(parent);
        GridData gdText = new GridData(GridData.FILL_HORIZONTAL);
        gdText.widthHint = 100;
        gdText.heightHint = 60;
        textControl.setLayoutData(gdText);
    }

    protected Text getTextControl(Composite parent)
    {
        if (textControl == null)
        {
            textControl = new Text(parent, SWT.MULTI | SWT.V_SCROLL | SWT.BORDER | SWT.WRAP);
            textControl.setFont(parent.getFont());
            textControl.addKeyListener(new KeyAdapter()
            {
                @Override
                public void keyPressed(KeyEvent e)
                {
                    valueChanged();
                }
            });

            textControl.addDisposeListener(new DisposeListener()
            {
                @Override
                public void widgetDisposed(DisposeEvent event)
                {
                    textControl = null;
                }
            });
        }
        else
            checkParent(textControl, parent);

        return textControl;
    }

    @Override
    protected void doLoad()
    {
        if (textControl != null)
        {
            String value = getPreferenceStore().getString(getPreferenceName());
            textControl.setText(value);
            oldTextValue = value;
        }
    }

    @Override
    protected void doLoadDefault()
    {
        if (textControl != null)
        {
            String value = getPreferenceStore().getDefaultString(getPreferenceName());
            textControl.setText(value);
        }
        valueChanged();
    }

    @Override
    protected void doStore()
    {
        getPreferenceStore().setValue(getPreferenceName(), textControl.getText());
    }

    @Override
    public void setFocus()
    {
        if (textControl != null)
            textControl.setFocus();
    }

    protected void valueChanged()
    {
        setPresentsDefaultValue(false);

        String newValue = textControl.getText();
        if (!newValue.equals(oldTextValue))
        {
            fireValueChanged(VALUE, oldTextValue, newValue);
            oldTextValue = newValue;
        }
    }

    @Override
    public void setEnabled(boolean enabled, Composite parent)
    {
        getTextControl(parent).setEnabled(enabled);
    }
}
