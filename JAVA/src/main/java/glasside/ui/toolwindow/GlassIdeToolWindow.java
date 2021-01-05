// Copyright 2000-2020 JetBrains s.r.o. and other contributors. Use of this source code is governed by the Apache 2.0 license that can be found in the LICENSE file.

package glasside.ui.toolwindow;

import com.intellij.openapi.components.PersistentStateComponent;
import com.intellij.openapi.components.ServiceManager;
import com.intellij.openapi.options.Configurable;
import com.intellij.openapi.project.Project;
import glasside.GlassIdeContext;
import glasside.GlassIdeStorage;
import glasside.helpers.PluginUiHelpers;
import glasside.ui.settings.SettingsScreenManager;

import javax.swing.*;
import javax.swing.event.ChangeEvent;
import java.awt.event.ItemEvent;

public class GlassIdeToolWindow {

    private JPanel mainContent;
    private JSlider opacitySlider;
    private JSlider brightnessSlider;
    private JSlider blurTypeSlider;
    private JCheckBox enableCheckBox;
    private JLabel opacityLabel;
    private JLabel brightnessLabel;
    private JLabel blurTypeLabel;

    private boolean isUiUpdating = false;
    private GlassIdeContext context = null;

    public GlassIdeToolWindow(Project project) {

        // Get the context
        context = ServiceManager.getService(project, GlassIdeContext.class);

        updateUi();

        // Set event listeners
        opacitySlider.addChangeListener(e -> {
            if (!isUiUpdating) onOpacitySliderChange(opacitySlider.getValue());
        });

        brightnessSlider.addChangeListener(e -> {
            if (!isUiUpdating) onBrightnessSliderChange(brightnessSlider.getValue());
        });

        blurTypeSlider.addChangeListener(e -> {
            if (!isUiUpdating) onBlurTypeSliderChange(blurTypeSlider.getValue());
        });

        enableCheckBox.addItemListener(e -> {
            if (!isUiUpdating)
                onEnableCheckBoxChange(e.getStateChange() == ItemEvent.SELECTED);
        });

    }


    // region On event methods
    private void onOpacitySliderChange(int level) {

        try {
            context.setOpacityLevel(level);
        } catch (RuntimeException e) {
            PluginUiHelpers.showErrorNotificationAndAbort(e.getMessage());
        }

        setOpacityLabelText(level);
    }

    private void onBrightnessSliderChange(int level) {

        try {
            context.setBrightnessLevel(level);
        } catch (RuntimeException e) {
            PluginUiHelpers.showErrorNotificationAndAbort(e.getMessage());
        }

        setBrightnessLabelText(level);
    }

    private void onBlurTypeSliderChange(int level) {

        try {
            context.setBlurType(level);
        } catch (RuntimeException e) {
            PluginUiHelpers.showErrorNotificationAndAbort(e.getMessage());
        }

        setBlurTypeLabelText(level);
    }

    private void onEnableCheckBoxChange(boolean enabled) {
        try {
            if (enabled)
                context.enableGlassMode(opacitySlider.getValue(), brightnessSlider.getValue(),
                        blurTypeSlider.getValue());
            else
                context.disableGlassMode();

        } catch (RuntimeException e) {
            enableCheckBox.setEnabled(false);
            PluginUiHelpers.showErrorNotificationAndAbort(e.getMessage());
        }
    }


    private void onSaveAsDefaultEvent() {
        GlassIdeStorage storage = GlassIdeStorage.getInstance();
        storage.opacityLevel = opacitySlider.getValue();
        storage.brightnessLevel = brightnessSlider.getValue();
        storage.blurType = blurTypeSlider.getValue();
        storage.isEnabled = enableCheckBox.isSelected();



    }


    // endregion


    // region utility methods

    public void updateUi() {

        isUiUpdating = true;
        opacitySlider.setValue(context.getOpacityLevel());
        brightnessSlider.setValue(context.getBrightnessLevel());
        blurTypeSlider.setValue(context.getBlurType());
        enableCheckBox.setSelected(context.isEffectEnabled());
        isUiUpdating = false;

        setOpacityLabelText(context.getOpacityLevel());
        setBrightnessLabelText(context.getBrightnessLevel());
        setBlurTypeLabelText(context.getBlurType());
    }


    private void setOpacityLabelText(int level) {
        opacityLabel.setText(level + "%");
    }

    private void setBrightnessLabelText(int level) {
        brightnessLabel.setText(level + "%");
    }

    private void setBlurTypeLabelText(int type) {
        switch (type) {
            case 1:
                blurTypeLabel.setText("Medium");
                break;
            case 2:
                blurTypeLabel.setText("High");
                break;
            default:
                blurTypeLabel.setText("None");
        }
    }

    // endregion

    public JPanel getContent() {
        return mainContent;
    }

}
