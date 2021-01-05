package glasside.ui.settings;

import com.intellij.openapi.options.Configurable;
import glasside.GlassIdeStorage;
import org.jetbrains.annotations.Nls;
import org.jetbrains.annotations.Nullable;

import javax.swing.*;

/**
 * Provides controller functionality for plugin settings.
 */
public class SettingsScreenManager implements Configurable {

  private SettingsScreen mySettingsComponent;

  // A default constructor with no arguments is required because this implementation
  // is registered as an applicationConfigurable EP

  @Nls(capitalization = Nls.Capitalization.Title)
  @Override
  public String getDisplayName() {
    return "GlassIDE Settings";
  }

  @Override
  public JComponent getPreferredFocusedComponent() {
    return mySettingsComponent.getPreferredFocusedComponent();
  }

  @Nullable
  @Override
  public JComponent createComponent() {
    mySettingsComponent = new SettingsScreen();
    return mySettingsComponent.getPanel();
  }

  @Override
  public boolean isModified() {
    // TODO: Build logic here
//    GlassIdeStorage settings = GlassIdeStorage.getInstance();
//    boolean modified = !mySettingsComponent.getUserNameText().equals(settings.userId);
//    modified |= mySettingsComponent.getIdeaUserStatus() != settings.ideaStatus;
//    return modified;
    return false;
  }

  @Override
  public void apply() {
    // TODO: Build logic here
//    GlassIdeStorage settings = GlassIdeStorage.getInstance();
//    settings.userId = mySettingsComponent.getUserNameText();
//    settings.ideaStatus = mySettingsComponent.getIdeaUserStatus();
  }

  @Override
  public void reset() {
    // TODO: Build logic here
//    GlassIdeStorage settings = GlassIdeStorage.getInstance();
//    mySettingsComponent.setUserNameText(settings.userId);
//    mySettingsComponent.setIdeaUserStatus(settings.ideaStatus);
  }

  @Override
  public void disposeUIResources() {
    mySettingsComponent = null;
  }

}