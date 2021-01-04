// Copyright 2000-2020 JetBrains s.r.o. and other contributors. Use of this source code is governed by the Apache 2.0 license that can be found in the LICENSE file.

package glasside.ui.toolwindow;

import com.intellij.openapi.wm.ToolWindow;

import javax.swing.*;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.util.Calendar;

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

    public GlassIdeToolWindow(ToolWindow toolWindow) {
        // TODO: Set the sliders from the settings

        System.out.println(1);


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
            case 1 -> blurTypeLabel.setText("Medium");
            case 2 -> blurTypeLabel.setText("High");
            default -> blurTypeLabel.setText("None");
        }
    }

    // endregion


//    private JButton refreshToolWindowButton;
//    private JButton hideToolWindowButton;
//    private JLabel currentDate;
//    private JLabel currentTime;
//    private JLabel timeZone;
//    private JPanel myToolWindowContent;
//
//    public GlassIdeToolWindow(ToolWindow toolWindow) {
//        hideToolWindowButton.addActionListener(e -> toolWindow.hide(null));
//        refreshToolWindowButton.addActionListener(e -> currentDateTime());
//
//        this.currentDateTime();
//    }
//
//    public void currentDateTime() {
//        // Get current date and time
//        Calendar instance = Calendar.getInstance();
//        currentDate.setText(
//                instance.get(Calendar.DAY_OF_MONTH) + "/"
//                        + (instance.get(Calendar.MONTH) + 1) + "/"
//                        + instance.get(Calendar.YEAR)
//        );
//        currentDate.setIcon(new ImageIcon(getClass().getResource("/toolWindow/Calendar-icon.png")));
//        int min = instance.get(Calendar.MINUTE);
//        String strMin = min < 10 ? "0" + min : String.valueOf(min);
//        currentTime.setText(instance.get(Calendar.HOUR_OF_DAY) + ":" + strMin);
//        currentTime.setIcon(new ImageIcon(getClass().getResource("/toolWindow/Time-icon.png")));
//        // Get time zone
//        long gmt_Offset = instance.get(Calendar.ZONE_OFFSET); // offset from GMT in milliseconds
//        String str_gmt_Offset = String.valueOf(gmt_Offset / 3600000);
//        str_gmt_Offset = (gmt_Offset > 0) ? "GMT + " + str_gmt_Offset : "GMT - " + str_gmt_Offset;
//        timeZone.setText(str_gmt_Offset);
//        timeZone.setIcon(new ImageIcon(getClass().getResource("/toolWindow/Time-zone-icon.png")));
//    }

    public JPanel getContent() {
        return mainContent;
    }

}
