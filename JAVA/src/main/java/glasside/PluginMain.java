package glasside;

import com.intellij.openapi.project.Project;
import com.intellij.util.concurrency.AppExecutorUtil;
import glasside.helpers.WindowsHelpers;

import java.util.concurrent.ScheduledFuture;

import static java.util.concurrent.TimeUnit.SECONDS;

public class PluginMain {


    private final Project project;
    private String initErrorMsg = null;
    private Renderer renderer = null;
    private static final int RENDERER_SCHEDULER_RUN_EVERY_SECONDS = 5;
    private ScheduledFuture<?> rendererMaintainerSF = null;
    private final Storage storage;

    private boolean isGlassEnabled = false;
    private int opacityLevel = 30;
    private int brightnessLevel = 30;
    private int blurType = 0;


    public PluginMain(Project project) {
        this.project = project;
        this.storage = Storage.getInstance();
    }

    // region init methods
    public void init() {

        this.opacityLevel = storage.opacityLevel;
        this.brightnessLevel = storage.brightnessLevel;
        this.blurType = storage.blurType;

        if (storage.isEnabled)
            enableGlassMode(opacityLevel, brightnessLevel, blurType);

    }

    public void dispose() {
        disableGlassMode(); // This will disable the glass mode if needed
    }

    private Renderer getRenderer() {
        if (renderer == null) {
            // Get the window handle for Windows
            long ideWindowId = WindowsHelpers.getIdeWindowOfProject(project);
            if (ideWindowId == 0) {
                initErrorMsg = "Failed to detect the IDE window handle";
                throw new RuntimeException(initErrorMsg);
            }
            renderer = new Renderer(ideWindowId);
        }

        return renderer;
    }

    // endregion


    // region control methods
    private void abortIfInitError() {
        if (initErrorMsg != null)
            throw new RuntimeException(initErrorMsg);
    }

    public void enableGlassMode(int opacityLevel, int brightnessLevel, int blurType) {
        if (isGlassEnabled)
            return;

        abortIfInitError();
        getRenderer().enableGlassEffect(storage.isCudaEnabled, opacityLevel, brightnessLevel, blurType);

        rendererMaintainerSF = AppExecutorUtil.getAppScheduledExecutorService().
                scheduleWithFixedDelay(new RendererMaintainer(this, renderer),
                        RENDERER_SCHEDULER_RUN_EVERY_SECONDS, RENDERER_SCHEDULER_RUN_EVERY_SECONDS,
                        SECONDS);

        this.opacityLevel = opacityLevel;
        this.brightnessLevel = brightnessLevel;
        this.blurType = blurType;
        this.isGlassEnabled = true;
    }

    public void disableGlassMode() {
        if (renderer != null)
            getRenderer().disableGlassEffect();

        if (rendererMaintainerSF != null) {
            rendererMaintainerSF.cancel(true);
            rendererMaintainerSF = null;
        }

        isGlassEnabled = false;
    }

    public void setBlurType(int blurType) {
        abortIfInitError();
        this.blurType = blurType;
        if (isGlassEnabled)
            getRenderer().setBlurType(blurType);
    }

    public void setOpacityLevel(int opacityLevel) {
        abortIfInitError();
        this.opacityLevel = opacityLevel;
        if (isGlassEnabled)
            getRenderer().setOpacityLevel(opacityLevel);
    }

    public void setBrightnessLevel(int brightnessLevel) {
        abortIfInitError();
        this.brightnessLevel = brightnessLevel;
        if (isGlassEnabled)
            getRenderer().setBrightnessLevel(brightnessLevel);
    }

    // endregion

    // region getters & setters

    public int getBlurType() {
        return blurType;
    }

    public int getBrightnessLevel() {
        return brightnessLevel;
    }

    public int getOpacityLevel() {
        return opacityLevel;
    }

    public boolean isGlassEffectEnabled() {
        return isGlassEnabled;
    }

    // endregion

}
