package glasside;

import com.intellij.openapi.components.PersistentStateComponent;
import com.intellij.openapi.components.ServiceManager;
import com.intellij.openapi.components.State;
import com.intellij.openapi.components.Storage;
import com.intellij.util.xmlb.XmlSerializerUtil;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.util.LinkedHashMap;


@State(
        name = "glasside.GlassIdeStorage",
        storages = {@Storage("glassIdePlugin.xml")}
)
public class GlassIdeStorage implements PersistentStateComponent<GlassIdeStorage> {

    public static final int DEFAULT_OPACITY = 65;
    public static final int DEFAULT_BRIGHTNESS = 100;
    public static final int DEFAULT_BLUR_TYPE = 1;
    public static final boolean DEFAULT_CUDA_ENABLED = true;
    public static final boolean DEFAULT_IS_GLASS_ENABLED = false;


    private int opacityLevel = DEFAULT_OPACITY;
    private int brightnessLevel = DEFAULT_BRIGHTNESS;
    private int blurType = DEFAULT_BLUR_TYPE;
    private boolean isEnabled = DEFAULT_IS_GLASS_ENABLED;
    private boolean isCudaEnabled = DEFAULT_CUDA_ENABLED;

    public static GlassIdeStorage getInstance() {
        return ServiceManager.getService(GlassIdeStorage.class);
    }


    @Override
    public @Nullable GlassIdeStorage getState() {
        return this;
    }

    @Override
    public void loadState(@NotNull GlassIdeStorage state) {
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

    public boolean isCudaEnabled() {
        return isCudaEnabled;
    }

    public boolean isEnabled() {
        return isEnabled;
    }

    public void setCudaEnabled(boolean cudaEnabled) {
        isCudaEnabled = cudaEnabled;
    }

    public void setEnabled(boolean enabled) {
        isEnabled = enabled;
    }

    public void setOpacityLevel(int opacityLevel) {
        this.opacityLevel = opacityLevel;
    }

    public void setBlurType(int blurType) {
        this.blurType = blurType;
    }

    public void setBrightnessLevel(int brightnessLevel) {
        this.brightnessLevel = brightnessLevel;
    }

}