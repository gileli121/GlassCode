package glasside;

import glasside.helpers.PluginUiHelpers;
import glasside.ui.toolwindow.GlassIdeToolWindow;

public class RendererMaintainer implements Runnable {
    private final Renderer renderer;
    private final PluginMain pluginMain;
    private static final int MAX_CRASH_COUNT = 5;
    private int attempts = 0;

    public RendererMaintainer(PluginMain pluginMain, Renderer renderer) {
        this.pluginMain = pluginMain;
        this.renderer = renderer;
    }

    @Override
    public void run() {

        if (!pluginMain.isGlassEffectEnabled())
            // Should never happen because when the effect is disabled this task will not run
            // Did it only to be more safe
            // TODO: ???? <---- Disable this task (have no idea how to do it from here
            return;

        if (renderer.isGlassEffectRunning())
            // If the glass effect is running then everything is great, we can stop here
            return;

        // Count this attempt
        attempts++;

        if (attempts > MAX_CRASH_COUNT) {
            // In this case we need to stop this scheduled task and abort the effect
            pluginMain.disableGlassMode();

            // Update the UI if available
            GlassIdeToolWindow glassIdeToolWindow = pluginMain.getGlassIdeToolWindow();
            if (glassIdeToolWindow != null)
                glassIdeToolWindow.updateUi();

            return;
        }

        try {

            GlassIdeStorage glassIdeStorage = GlassIdeStorage.getInstance();

            // Start the effect again
            renderer.enableGlassEffect(
                    glassIdeStorage.isCudaEnabled(),
                    pluginMain.getOpacityLevel(),
                    pluginMain.getBrightnessLevel(),
                    pluginMain.getBlurType());

        } catch (Exception e) {
            PluginUiHelpers.showErrorNotification("Failed to re-enable the effect " +
                    "(attempt " + attempts + "), Exception: " + e.getMessage());
        }

    }
}
