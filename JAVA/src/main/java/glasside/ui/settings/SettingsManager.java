package glasside.ui.settings;

import com.intellij.openapi.options.Configurable;
import glasside.GlassIdeStorage;
import org.jetbrains.annotations.Nls;
import org.jetbrains.annotations.Nullable;

import javax.swing.*;


public class SettingsManager implements Configurable {

    private SettingsScreen settingsScreen;
    private final GlassIdeStorage glassIdeStorage;

    public SettingsManager() {
        this.glassIdeStorage = GlassIdeStorage.getInstance();
    }

    @Nls(capitalization = Nls.Capitalization.Title)
    @Override
    public String getDisplayName() {
        return "GlassIDE Settings";
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

        if (glassIdeStorage.isCudaEnabled() != settingsScreen.isCudaEnabled())
            modified = true;

        return modified;
    }

    @Override
    public void apply() {
        glassIdeStorage.setCudaEnabled(settingsScreen.isCudaEnabled());
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