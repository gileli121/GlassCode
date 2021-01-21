package glasscode;

import com.intellij.openapi.project.Project;
import com.intellij.util.concurrency.AppExecutorUtil;
import glasscode.exceptions.GetIdeWindowException;
import glasscode.helpers.ThemeHelper;
import glasscode.helpers.WindowsHelpers;
import glasscode.ui.toolwindow.GlassCodeToolWindow;

import java.util.concurrent.ScheduledFuture;

import static java.util.concurrent.TimeUnit.SECONDS;

public class PluginMain {


    private final Project project;
    private GlassCodeToolWindow glassCodeToolWindow = null;
    private String initErrorMsg = null;
    private Renderer renderer = null;
    private static final int RENDERER_SCHEDULER_RUN_EVERY_SECONDS = 5;
    private ScheduledFuture<?> rendererMaintainerSF = null;
    private final GlassCodeStorage glassCodeStorage;

    private boolean isGlassEnabled;
    private int opacityLevel;
    private int brightnessLevel;
    private int textExtraBrightnessLevel;
    private int blurType;
    private boolean enableHighContrast;


    public PluginMain(Project project) {
        this.project = project;

        this.glassCodeStorage = GlassCodeStorage.getInstance();
        this.opacityLevel = glassCodeStorage.getOpacityLevel();
        this.brightnessLevel = glassCodeStorage.getBrightnessLevel();
        this.textExtraBrightnessLevel = glassCodeStorage.getTextExtraBrightnessLevel();
        this.blurType = glassCodeStorage.getBlurType();
        this.enableHighContrast = glassCodeStorage.isUseHighContrastTheme();
        this.isGlassEnabled = glassCodeStorage.isEnabled();
    }

    // region init methods
    public void init() {

        if (isGlassEnabled) {
            rendererMaintainerSF = AppExecutorUtil.getAppScheduledExecutorService().
                    scheduleWithFixedDelay(new PluginThread(this),
                            RENDERER_SCHEDULER_RUN_EVERY_SECONDS, RENDERER_SCHEDULER_RUN_EVERY_SECONDS,
                            SECONDS);
        }

    }

    public void loadToolWindow(GlassCodeToolWindow glassCodeToolWindow) {
        this.glassCodeToolWindow = glassCodeToolWindow;
    }

    public void dispose() {
        disableGlassMode(); // This will disable the glass mode if needed
    }

    public Renderer getRenderer() {
        if (renderer == null) {
            // Get the window handle for Windows
            long ideWindowId = WindowsHelpers.getIdeWindowOfProject(project);
            if (ideWindowId == 0) {
                throw new GetIdeWindowException("Failed to detect the IDE window handle");
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

    public void enableGlassMode(int opacityLevel, int brightnessLevel, int textExtraBrightnessLevel, int blurType,
                                boolean enableHighContrast) {
        if (isGlassEnabled)
            return;

        abortIfInitError();

        getRenderer().enableGlassEffect(glassCodeStorage.isCudaEnabled(), opacityLevel, brightnessLevel,
                textExtraBrightnessLevel, blurType);

        if (enableHighContrast && PluginInitializer.getOpenedProjectsCount() <= 1)
            ThemeHelper.enableHighContrastMode();

        this.opacityLevel = opacityLevel;
        this.brightnessLevel = brightnessLevel;
        this.textExtraBrightnessLevel = textExtraBrightnessLevel;
        this.blurType = blurType;
        this.enableHighContrast = enableHighContrast;
        this.isGlassEnabled = true;

        rendererMaintainerSF = AppExecutorUtil.getAppScheduledExecutorService().
                scheduleWithFixedDelay(new PluginThread(this),
                        RENDERER_SCHEDULER_RUN_EVERY_SECONDS, RENDERER_SCHEDULER_RUN_EVERY_SECONDS,
                        SECONDS);


    }

    public void disableGlassMode() {

        if (rendererMaintainerSF != null) {
            rendererMaintainerSF.cancel(true);
            rendererMaintainerSF = null;
        }

        if (ThemeHelper.isTemporaryThemeEnabled())
            ThemeHelper.disableHighContrastMode();

        if (renderer != null)
            getRenderer().disableGlassEffect();


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

    public void enableHighContrastMode(boolean enableHighContrast) {
        abortIfInitError();
        this.enableHighContrast = enableHighContrast;
        if (!isGlassEnabled) return;

        if (enableHighContrast) {
            if (!ThemeHelper.isTemporaryThemeEnabled() && PluginInitializer.getOpenedProjectsCount() <= 1)
                ThemeHelper.enableHighContrastMode();
        } else {
            if (ThemeHelper.isTemporaryThemeEnabled())
                ThemeHelper.disableHighContrastMode();
        }

    }

    // endregion

    // region getters & setters

    public GlassCodeToolWindow getToolWindow() {
        return glassCodeToolWindow;
    }

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

    public boolean isEnableHighContrast() {
        return enableHighContrast;
    }

    public int getTextExtraBrightnessLevel() {
        return textExtraBrightnessLevel;
    }

    public void setTextExtraBrightnessLevel(int level) {
        abortIfInitError();
        this.textExtraBrightnessLevel = level;
        if (isGlassEnabled)
            getRenderer().setTextExtraBrightnessLevel(textExtraBrightnessLevel);
    }


    // endregion

}
