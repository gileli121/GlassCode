package glasside;

import com.intellij.openapi.components.PersistentStateComponent;
import com.intellij.openapi.components.ServiceManager;
import com.intellij.openapi.components.State;
import com.intellij.util.xmlb.XmlSerializerUtil;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

/**
 * Supports storing the application settings in a persistent way.
 * The {@link State} and {@link com.intellij.openapi.components.Storage} annotations define the name of the data and the file name where
 * these persistent application settings are stored.
 */
@State(
        name = "glasside.AppSettingsState",
        storages = {@com.intellij.openapi.components.Storage("GradleIdeSettings.xml")}
)
public class Storage implements PersistentStateComponent<Storage> {

    public int opacityLevel = 65;
    public int brightnessLevel = 100;
    public int blurType = 1;
    public boolean isEnabled = false;
    public boolean isCudaEnabled = true;


    public static Storage getInstance() {
        return ServiceManager.getService(Storage.class);
    }

    @Nullable
    @Override
    public Storage getState() {
        return this;
    }

    @Override
    public void loadState(@NotNull Storage state) {
        XmlSerializerUtil.copyBean(state, this);
    }


}