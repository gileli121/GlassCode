package glasscode;

import com.intellij.openapi.components.PersistentStateComponent;
import com.intellij.openapi.components.ServiceManager;
import com.intellij.openapi.components.State;
import com.intellij.openapi.components.Storage;
import com.intellij.util.xmlb.XmlSerializerUtil;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;


@State(
        name = "glasscode.GlassCodeStorage",
        storages = {@Storage("glassCodePlugin.xml")}
)
public class GlassCodeStorage implements PersistentStateComponent<GlassCodeStorage> {

    private static final int DEFAULT_OPACITY = 80;
    private static final int DEFAULT_BRIGHTNESS = 60;
    private static final int DEFAULT_BLUR_TYPE = 0;
    private static final int DEFAULT_TEXT_EXTRA_BRIGHTNESS = 0;
    private static final boolean DEFAULT_CUDA_ENABLED = false;
    private static final boolean DEFAULT_IS_GLASS_ENABLED = false;
    private static final boolean DEFAULT_USE_CONTRAST_THEME = true;

    private boolean wasSavedBefore = false;
    private int opacityLevel = DEFAULT_OPACITY;
    private int brightnessLevel = DEFAULT_BRIGHTNESS;
    private int textExtraBrightnessLevel = DEFAULT_TEXT_EXTRA_BRIGHTNESS;
    private int blurType = DEFAULT_BLUR_TYPE;
    private boolean isEnabled = DEFAULT_IS_GLASS_ENABLED;
    private boolean isCudaEnabled = DEFAULT_CUDA_ENABLED;
    private boolean useHighContrastTheme = DEFAULT_USE_CONTRAST_THEME;


    public static GlassCodeStorage getInstance() {
        return ServiceManager.getService(GlassCodeStorage.class);
    }


    @Override
    public @Nullable GlassCodeStorage getState() {
        return this;
    }

    @Override
    public void loadState(@NotNull GlassCodeStorage state) {
        XmlSerializerUtil.copyBean(state, this);
    }

    @Override
    public void noStateLoaded() {
    }

    @Override
    public void initializeComponent() {

    }

    public void setDefaults() {
        opacityLevel = DEFAULT_OPACITY;
        brightnessLevel = DEFAULT_BRIGHTNESS;
        blurType = DEFAULT_BLUR_TYPE;
        isEnabled = DEFAULT_IS_GLASS_ENABLED;
        isCudaEnabled = DEFAULT_CUDA_ENABLED;
        useHighContrastTheme = DEFAULT_USE_CONTRAST_THEME;
    }


    public int getBlurType() {
        return wasSavedBefore ? blurType : DEFAULT_BLUR_TYPE;
    }

    public int getBrightnessLevel() {
        return wasSavedBefore ? brightnessLevel : DEFAULT_BRIGHTNESS;
    }

    public int getOpacityLevel() {
        return wasSavedBefore ? opacityLevel : DEFAULT_OPACITY;
    }

    public int getTextExtraBrightnessLevel() {
        return wasSavedBefore ? textExtraBrightnessLevel : DEFAULT_TEXT_EXTRA_BRIGHTNESS;
    }

    public boolean isCudaEnabled() {
        return wasSavedBefore ? isCudaEnabled : DEFAULT_CUDA_ENABLED;
    }

    public boolean isEnabled() {
        return wasSavedBefore ? isEnabled : DEFAULT_IS_GLASS_ENABLED;
    }

    public void setCudaEnabled(boolean cudaEnabled) {
        isCudaEnabled = cudaEnabled;
        wasSavedBefore = true;
    }

    public boolean isUseHighContrastTheme() {
        return useHighContrastTheme;
    }

    public void setEnabled(boolean enabled) {
        isEnabled = enabled;
        wasSavedBefore = true;
    }

    public void setOpacityLevel(int opacityLevel) {
        this.opacityLevel = opacityLevel;
        wasSavedBefore = true;
    }

    public void setBlurType(int blurType) {
        this.blurType = blurType;
        wasSavedBefore = true;
    }

    public void setBrightnessLevel(int brightnessLevel) {
        this.brightnessLevel = brightnessLevel;
        wasSavedBefore = true;
    }

    public void setUseHighContrastTheme(boolean useHighContrastTheme) {
        this.useHighContrastTheme = useHighContrastTheme;
        wasSavedBefore = true;
    }

    public void setTextExtraBrightnessLevel(int textExtraBrightnessLevel) {
        this.textExtraBrightnessLevel = textExtraBrightnessLevel;
    }
}