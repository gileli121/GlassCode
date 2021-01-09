// Copyright 2000-2020 JetBrains s.r.o. and other contributors. Use of this source code is governed by the Apache 2.0 license that can be found in the LICENSE file.

package glasside.ui.toolwindow;

import com.intellij.openapi.components.ServiceManager;
import com.intellij.openapi.project.Project;
import glasside.PluginMain;
import glasside.helpers.PluginUiHelpers;

import javax.swing.*;
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
    private final PluginMain pluginMain;

    public GlassIdeToolWindow(Project project) {

        // Get the context
        pluginMain = ServiceManager.getService(project, PluginMain.class);
        pluginMain.loadGlassIdeToolWindow(this);

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
            pluginMain.setOpacityLevel(level);
        } catch (RuntimeException e) {
            PluginUiHelpers.showErrorNotificationAndAbort(e.getMessage());
        }

        setOpacityLabelText(level);
    }

    private void onBrightnessSliderChange(int level) {

        try {
            pluginMain.setBrightnessLevel(level);
        } catch (RuntimeException e) {
            PluginUiHelpers.showErrorNotificationAndAbort(e.getMessage());
        }

        setBrightnessLabelText(level);
    }

    private void onBlurTypeSliderChange(int level) {

        try {
            pluginMain.setBlurType(level);
        } catch (RuntimeException e) {
            PluginUiHelpers.showErrorNotificationAndAbort(e.getMessage());
        }

        setBlurTypeLabelText(level);
    }

    private void onEnableCheckBoxChange(boolean enabled) {
        try {
            if (enabled)
                pluginMain.enableGlassMode(opacitySlider.getValue(), brightnessSlider.getValue(),
                        blurTypeSlider.getValue());
            else
                pluginMain.disableGlassMode();

        } catch (RuntimeException e) {
            enableCheckBox.setEnabled(false);
            PluginUiHelpers.showErrorNotificationAndAbort(e.getMessage());
        }
    }



    // endregion


    // region utility methods

    public void updateUi() {

        isUiUpdating = true;
        opacitySlider.setValue(pluginMain.getOpacityLevel());
        brightnessSlider.setValue(pluginMain.getBrightnessLevel());
        blurTypeSlider.setValue(pluginMain.getBlurType());
        enableCheckBox.setSelected(pluginMain.isGlassEffectEnabled());
        isUiUpdating = false;

        setOpacityLabelText(pluginMain.getOpacityLevel());
        setBrightnessLabelText(pluginMain.getBrightnessLevel());
        setBlurTypeLabelText(pluginMain.getBlurType());
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
