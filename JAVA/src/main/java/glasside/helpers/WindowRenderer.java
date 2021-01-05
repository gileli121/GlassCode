package glasside.helpers;

public class WindowRenderer {

    private final long windowId;
    private int rendererPid = 0;
    private boolean isRendererRunning = false;

    public WindowRenderer(long windowId) {
        this.windowId = windowId;
    }

    // initialize / un-initialize methods
    public void dispose() {
        // TODO: Logic to close the renderer process
    }


    // endregion

    // region API/Public methods
    public void enableEffect(int opacityLevel,int brightnessLevel, int blurType) {
        startupRendererProcessIfNeeded();
        // TODO: Logic to sent command to the renderer process
    }

    public void disableEffect() {
        // TODO: Logic to send command to disable the effect and exit
    }

    public boolean isEnabled() {
        return rendererPid != 0;
    }

    public void setBlurType(int blurType) {

    }

    public void setOpacityLevel(int opacityLevel) {

    }

    public void setBrightnessLevel(int brightnessLevel) {

    }

    // endregion

    // region helpers
    private void startupRendererProcessIfNeeded() {

    }

    private void shutdownRendererProcess() {

    }

    // endregion
}
