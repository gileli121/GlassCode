package glasside.ui.settings;

import org.jetbrains.annotations.NotNull;

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

//    @NotNull
//    public String getUserNameText() {
//        return "";
//    }
//
//    public void setUserNameText(@NotNull String newText) {
//    }
//
//    public boolean getIdeaUserStatus() {
//        return true;
//    }
//
//    public void setIdeaUserStatus(boolean newStatus) {
//
//    }


}
