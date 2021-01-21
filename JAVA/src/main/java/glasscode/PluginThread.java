package glasscode;

import glasscode.exceptions.GetIdeWindowException;
import glasscode.helpers.PluginUiHelpers;
import glasscode.helpers.ThemeHelper;
import glasscode.ui.toolwindow.GlassCodeToolWindow;

import javax.swing.*;

public class PluginThread implements Runnable {
    private final PluginMain pluginMain;
    private static final int MAX_CRASH_COUNT = 5;
    private int attempts = 0;

    public PluginThread(PluginMain pluginMain) {
        this.pluginMain = pluginMain;
    }

    @Override
    public void run() {

        if (!pluginMain.isGlassEffectEnabled())
            // Should never happen because when the effect is disabled this task will not run
            // Did it only to be more safe
            // TODO: ???? <---- Disable this task (have no idea how to do it from here
            return;

        if (pluginMain.isEnableHighContrast()) {
            if (PluginInitializer.getOpenedProjectsCount() <= 1) {
                if (!ThemeHelper.isTemporaryThemeEnabled())
                    SwingUtilities.invokeLater(ThemeHelper::enableHighContrastMode);
            } else {
                if (ThemeHelper.isTemporaryThemeEnabled())
                    SwingUtilities.invokeLater(ThemeHelper::disableHighContrastMode);
            }
        }

        Renderer renderer = null;
        try {
            renderer = pluginMain.getRenderer();
        } catch (GetIdeWindowException ignored) {
            // Ignored - Try again next time. Maybe next time the IDE window will loaded and show
        }

        if (renderer != null && !renderer.isGlassEffectRunning()) {

            // Count this attempt
            attempts++;

            if (attempts > MAX_CRASH_COUNT) {
                // In this case we need to stop this scheduled task and abort the effect
                pluginMain.disableGlassMode();

                // Update the UI if available
                GlassCodeToolWindow glassCodeToolWindow = pluginMain.getToolWindow();
                if (glassCodeToolWindow != null)
                    glassCodeToolWindow.updateUi();

                return;
            }

            try {

                GlassCodeStorage glassCodeStorage = GlassCodeStorage.getInstance();

                // Start the effect again
                renderer.enableGlassEffect(
                        glassCodeStorage.isCudaEnabled(),
                        pluginMain.getOpacityLevel(),
                        pluginMain.getBrightnessLevel(),
                        pluginMain.getTextExtraBrightnessLevel(),
                        pluginMain.getBlurType());

            } catch (Exception e) {
                PluginUiHelpers.showErrorNotification("Failed to re-enable the effect " +
                        "(attempt " + attempts + "), Exception: " + e.getMessage());
            }
        }

    }
}
