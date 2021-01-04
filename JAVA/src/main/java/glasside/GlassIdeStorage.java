package glasside;

import com.intellij.openapi.components.PersistentStateComponent;
import com.intellij.openapi.components.ServiceManager;
import com.intellij.openapi.components.State;
import com.intellij.openapi.components.Storage;
import com.intellij.util.xmlb.XmlSerializerUtil;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

/**
 * Supports storing the application settings in a persistent way.
 * The {@link State} and {@link Storage} annotations define the name of the data and the file name where
 * these persistent application settings are stored.
 */
@State(
        name = "glasside.AppSettingsState",
        storages = {@Storage("SdkSettingsPlugin.xml")}
)
public class GlassIdeStorage implements PersistentStateComponent<GlassIdeStorage> {

  public String userId = "John Q. Public";
  public boolean ideaStatus = false;

  public static GlassIdeStorage getInstance() {
    return ServiceManager.getService(GlassIdeStorage.class);
  }

  @Nullable
  @Override
  public GlassIdeStorage getState() {
    return this;
  }

  @Override
  public void loadState(@NotNull GlassIdeStorage state) {
    XmlSerializerUtil.copyBean(state, this);
  }

}