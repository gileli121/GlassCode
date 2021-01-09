package glasside.ui.settings;

import glasside.GlassIdeStorage;

import javax.swing.*;

public class SettingsScreen {
    private JPanel mainPanel;
    private JCheckBox enableGPUAccelerationCheckBox;


    public JPanel getPanel() {
        return mainPanel;
    }

    public JComponent getPreferredFocusedComponent() {
        return enableGPUAccelerationCheckBox;
    }

    public boolean isCudaEnabled() {
        return enableGPUAccelerationCheckBox.isSelected();
    }

    public void setCudaEnabled(boolean enabled) {
        enableGPUAccelerationCheckBox.setSelected(enabled);
    }

    public void updateUi() {
        GlassIdeStorage glassIdeStorage = GlassIdeStorage.getInstance();
        setCudaEnabled(glassIdeStorage.isCudaEnabled());
    }

}
