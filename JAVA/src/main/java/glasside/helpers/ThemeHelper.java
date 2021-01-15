package glasside.helpers;

import com.intellij.ide.ui.LafManager;
import com.intellij.ide.ui.UITheme;
import com.intellij.ide.ui.laf.UIThemeBasedLookAndFeelInfo;
import com.intellij.ide.ui.laf.darcula.DarculaLaf;
import com.intellij.openapi.editor.colors.EditorColorsManager;
import com.intellij.openapi.editor.colors.EditorColorsScheme;
import com.intellij.ui.AppUIUtil;
import com.intellij.ui.JBColor;

import javax.swing.*;

public class ThemeHelper {
    private static EditorColorsScheme highContrastScheme = null;
    private static UIDefaults uiDefaults = null;
    private static boolean isBright = false;
    private static UIManager.LookAndFeelInfo currentLookAndFeel = null;
    private static UIThemeBasedLookAndFeelInfo highContrastUiTheme = null;
    private static boolean isHighContrastEnabled = false;

    private static void setTempMode(boolean enabled) {
        highContrastScheme.getMetaProperties().setProperty("TEMP_SCHEME_KEY", enabled ? "true" : "false");
    }

    /**
     * This will enable High Contrast theme temporarily so if you restart IDE, the original theme will still be set
     */
    public static void enableHighContrastMode() {

        if (uiDefaults == null) {
            try {
                UIManager.getDefaults().clear();
                UIManager.setLookAndFeel(new DarculaLaf());
            } catch (UnsupportedLookAndFeelException e) {
                throw new RuntimeException("Failed to setup uiDefaults");
            }
            uiDefaults = (UIDefaults) UIManager.getDefaults().clone();
        }
        LafManager lafManager = LafManager.getInstance();
        currentLookAndFeel = lafManager.getCurrentLookAndFeel();
        if (currentLookAndFeel == null)
            throw new RuntimeException("Failed to get currentLookAndFeel object");

        isBright = JBColor.isBright();

        UIManager.LookAndFeelInfo[] lookAndFeelInfos = lafManager.getInstalledLookAndFeels();
        final String HIGH_CONTRAST_THEME_NAME = "High contrast";
        UIManager.LookAndFeelInfo highContrastLaf = null;
        for (UIManager.LookAndFeelInfo lookAndFeelInfo : lookAndFeelInfos) {
            if (!HIGH_CONTRAST_THEME_NAME.equals(lookAndFeelInfo.getName())) continue;
            highContrastLaf = lookAndFeelInfo;
            break;
        }
        if (highContrastLaf == null)
            throw new RuntimeException("Theme \"" + HIGH_CONTRAST_THEME_NAME + "\" not found");
        UITheme uiTheme = ((UIThemeBasedLookAndFeelInfo) highContrastLaf).getTheme();
        highContrastScheme = EditorColorsManager.getInstance().getScheme(uiTheme.getEditorSchemeName());
        setTempMode(true);
        UIManager.getDefaults().clear();
        UIManager.getDefaults().putAll(uiDefaults);
        try {
            UIManager.setLookAndFeel(new DarculaLaf());
        } catch (UnsupportedLookAndFeelException e) {
            setTempMode(false);
            throw new RuntimeException("Failed in UIManager.setLookAndFeel(new DarculaLaf());", e);
        }
        highContrastUiTheme = new UIThemeBasedLookAndFeelInfo(uiTheme);
        highContrastUiTheme.installTheme(UIManager.getLookAndFeelDefaults(), false);
        AppUIUtil.updateForDarcula(true);

        isHighContrastEnabled = true;
    }

    /**
     * Disable the high contrast mode and restore to the original mode
     */
    public static void disableHighContrastMode() {
        if (currentLookAndFeel == null)
            return;

        LafManager lafManager = LafManager.getInstance();

        lafManager.setCurrentLookAndFeel(highContrastUiTheme, false);
        lafManager.setCurrentLookAndFeel(currentLookAndFeel, false);
        AppUIUtil.updateForDarcula(!isBright);
        lafManager.updateUI();

        setTempMode(false);

        isHighContrastEnabled = false;
    }

    public static boolean isIsHighContrastEnabled() {
        return isHighContrastEnabled;
    }
}