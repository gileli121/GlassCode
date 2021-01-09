package glasside;

import com.intellij.openapi.project.Project;
import glasside.helpers.WindowRenderer;
import glasside.helpers.WindowsHelpers;

public class GlassIdeContext {


    private final Project project;
    private String initErrorMsg = null;
    private WindowRenderer renderer = null;
    private GlassIdeStorage storage = null;

    private int opacityLevel = 30;
    private int brightnessLevel = 30;
    private int blurType = 0;


    public GlassIdeContext(Project project) {
        this.project = project;
        this.storage = GlassIdeStorage.getInstance();
    }

    // region init methods
    public void init() {

        this.opacityLevel = storage.opacityLevel;
        this.brightnessLevel = storage.brightnessLevel;
        this.blurType = storage.blurType;

        if (storage.isEnabled)
            enableGlassMode(opacityLevel,brightnessLevel,blurType);

    }

    public void dispose() {
        if (isEffectEnabled())
            renderer.disable();
    }

    private WindowRenderer getRenderer() {
        if (renderer == null) {
            // Get the window handle for Windows
            long ideWindowId = WindowsHelpers.getIdeWindowOfProject(project);
            if (ideWindowId == 0) {
                initErrorMsg = "Failed to detect the IDE window handle";
                throw new RuntimeException(initErrorMsg);
            }
            renderer = new WindowRenderer(ideWindowId);
        }

        return renderer;
    }

    // endregion


    // region control methods
    private void abortIfInitError() {
        if (initErrorMsg != null)
            throw new RuntimeException(initErrorMsg);
    }

    public void enableGlassMode(int opacityLevel,int brightnessLevel, int blurType) {
        abortIfInitError();
        getRenderer().enableEffect(storage.isCudaEnabled,opacityLevel, brightnessLevel, blurType);
        this.opacityLevel = opacityLevel;
        this.brightnessLevel = brightnessLevel;
        this.blurType = blurType;
    }

    public void disableGlassMode() {
        if (isEffectEnabled()) {
            getRenderer().disable();
        }
    }

    public void setBlurType(int blurType) {
        abortIfInitError();
        this.blurType = blurType;
        if (isEffectEnabled())
            getRenderer().setBlurType(blurType);
    }

    public void setOpacityLevel(int opacityLevel) {
        abortIfInitError();
        this.opacityLevel = opacityLevel;
        if (isEffectEnabled())
            getRenderer().setOpacityLevel(opacityLevel);
    }

    public void setBrightnessLevel(int brightnessLevel) {
        abortIfInitError();
        this.brightnessLevel = brightnessLevel;
        if (isEffectEnabled())
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

    public boolean isEffectEnabled() {
        return renderer != null && renderer.isEnabled();
    }

    // endregion

}
