package glasscode.ui.settings;

import com.intellij.openapi.options.Configurable;
import glasscode.GlassCodeStorage;
import glasscode.PluginConstants;
import org.jetbrains.annotations.Nls;
import org.jetbrains.annotations.Nullable;

import javax.swing.*;


public class SettingsManager implements Configurable {

    private SettingsScreen settingsScreen;
    private final GlassCodeStorage glassCodeStorage;

    public SettingsManager() {
        this.glassCodeStorage = GlassCodeStorage.getInstance();
    }

    @Nls(capitalization = Nls.Capitalization.Title)
    @Override
    public String getDisplayName() {
        return PluginConstants.PLUGIN_NAME +  " Settings";
    }

    @Override
    public JComponent getPreferredFocusedComponent() {
        return settingsScreen.getPreferredFocusedComponent();
    }

    @Nullable
    @Override
    public JComponent createComponent() {
        settingsScreen = new SettingsScreen();
        settingsScreen.updateUi();
        return settingsScreen.getPanel();
    }

    @Override
    public boolean isModified() {
        boolean modified = false;

        if (glassCodeStorage.isCudaEnabled() != settingsScreen.isCudaEnabled())
            modified = true;

        return modified;
    }

    @Override
    public void apply() {
        glassCodeStorage.setCudaEnabled(settingsScreen.isCudaEnabled());
    }

    @Override
    public void reset() {
        settingsScreen.updateUi();
    }

    @Override
    public void disposeUIResources() {
        settingsScreen = null;
    }

}