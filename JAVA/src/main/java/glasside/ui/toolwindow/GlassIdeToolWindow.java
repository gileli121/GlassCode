// Copyright 2000-2020 JetBrains s.r.o. and other contributors. Use of this source code is governed by the Apache 2.0 license that can be found in the LICENSE file.

package glasside.ui.toolwindow;

import com.intellij.openapi.components.ServiceManager;
import com.intellij.openapi.project.Project;
import glasside.GlassIdeStorage;

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
    private final Project project;

    public GlassIdeToolWindow(Project project) {
        // TODO: Set the sliders from the settings

        System.out.println(1);

        this.project = project;

        updateUi();

        // Set event listeners
        opacitySlider.addChangeListener(e -> {
            if (!isUiUpdating) onOpacitySliderChange(opacitySlider.getValue(),e);
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
    private void onOpacitySliderChange(int level, ChangeEvent e) {
        GlassIdeStorage glassIdeStorage = ServiceManager.getService(GlassIdeStorage.class);

        setOpacityLabelText(level);
    }

    private void onBrightnessSliderChange(int level) {
        setBrightnessLabelText(level);
    }

    private void onBlurTypeSliderChange(int level) {
        setBlurTypeLabelText(level);
    }

    private void onEnableCheckBoxChange(boolean enabled) {

    }
    // endregion


    // region utility methods

    public void updateUi() {

        // TODO: Later it should load it from the UI state object...
        int opacityLevel = 50;
        int brightnessLevel = 50;
        int blurType = 1;
        boolean isEnabled = true;

        isUiUpdating = true;
        opacitySlider.setValue(opacityLevel);
        brightnessSlider.setValue(brightnessLevel);
        blurTypeSlider.setValue(blurType);
        enableCheckBox.setEnabled(isEnabled);
        isUiUpdating = false;

        setOpacityLabelText(opacityLevel);
        setBrightnessLabelText(brightnessLevel);
        setBlurTypeLabelText(blurType);
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
