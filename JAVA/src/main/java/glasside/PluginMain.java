package glasside;

import com.intellij.openapi.project.Project;
import com.intellij.util.concurrency.AppExecutorUtil;
import glasside.exceptions.GetIdeWindowException;
import glasside.helpers.ThemeHelper;
import glasside.helpers.WindowsHelpers;
import glasside.ui.toolwindow.GlassIdeToolWindow;

import java.util.concurrent.ScheduledFuture;

import static java.util.concurrent.TimeUnit.SECONDS;

public class PluginMain {


    private final Project project;
    private GlassIdeToolWindow glassIdeToolWindow = null;
    private String initErrorMsg = null;
    private Renderer renderer = null;
    private static final int RENDERER_SCHEDULER_RUN_EVERY_SECONDS = 5;
    private ScheduledFuture<?> rendererMaintainerSF = null;
    private final GlassIdeStorage glassIdeStorage;

    private boolean isGlassEnabled;
    private int opacityLevel;
    private int brightnessLevel;
    private int blurType;
    private boolean enableHighContrast;


    public PluginMain(Project project) {
        this.project = project;

        this.glassIdeStorage = GlassIdeStorage.getInstance();
        this.opacityLevel = glassIdeStorage.getOpacityLevel();
        this.brightnessLevel = glassIdeStorage.getBrightnessLevel();
        this.blurType = glassIdeStorage.getBlurType();
        this.enableHighContrast = glassIdeStorage.isUseHighContrastTheme();
        this.isGlassEnabled = glassIdeStorage.isEnabled();
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

    public void loadGlassIdeToolWindow(GlassIdeToolWindow glassIdeToolWindow) {
        this.glassIdeToolWindow = glassIdeToolWindow;
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

    public void enableGlassMode(int opacityLevel, int brightnessLevel, int blurType, boolean enableHighContrast) {
        if (isGlassEnabled)
            return;

        abortIfInitError();

        getRenderer().enableGlassEffect(glassIdeStorage.isCudaEnabled(), opacityLevel, brightnessLevel, blurType);

        if (enableHighContrast && PluginInitializer.getOpenedProjectsCount() <= 1)
            ThemeHelper.enableHighContrastMode();

        this.opacityLevel = opacityLevel;
        this.brightnessLevel = brightnessLevel;
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

        if (ThemeHelper.isIsHighContrastEnabled())
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
            if (!ThemeHelper.isIsHighContrastEnabled() && PluginInitializer.getOpenedProjectsCount() <= 1)
                ThemeHelper.enableHighContrastMode();
        } else {
            if (ThemeHelper.isIsHighContrastEnabled())
                ThemeHelper.disableHighContrastMode();
        }

    }

    // endregion

    // region getters & setters

    public GlassIdeToolWindow getGlassIdeToolWindow() {
        return glassIdeToolWindow;
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


    // endregion

}
