package glasside;

import glasside.helpers.PluginUiHelpers;

public class RendererMaintainer implements Runnable {
    private final Renderer renderer;
    private final PluginMain pluginMain;
    private static final int MAX_CRASH_COUNT = 10;
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
            // TODO: ???? <---- Disable this task (have now idea how to do it from here
            return;

        if (renderer.isGlassEffectRunning())
            // If the glass effect is running then everything is great, we can stop here
            return;

        // Count this attempt
        attempts++;

        if (attempts > MAX_CRASH_COUNT) {
            // In this case we need to stop this scheduled task and abort the effect
            pluginMain.disableGlassMode();
            return;
        }

        try {

            // Start the effect again
            renderer.enableGlassEffect(
                    Storage.getInstance().isCudaEnabled,
                    pluginMain.getOpacityLevel(),
                    pluginMain.getBrightnessLevel(),
                    pluginMain.getBlurType());

            // Reset the attempts counter
            attempts = 0;
        } catch (Exception e) {
            PluginUiHelpers.showErrorNotification("Failed to re-enable the effect " +
                    "(attempt "+ attempts +")");
        }

    }
}
