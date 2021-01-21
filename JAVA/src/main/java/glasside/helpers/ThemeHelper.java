package glasside.helpers;

import com.intellij.ide.ui.LafManager;
import com.intellij.ide.ui.UITheme;
import com.intellij.ide.ui.laf.UIThemeBasedLookAndFeelInfo;
import com.intellij.openapi.editor.colors.EditorColorsManager;
import com.intellij.openapi.editor.colors.EditorColorsScheme;
import com.intellij.openapi.editor.colors.impl.EditorColorsSchemeImpl;
import com.intellij.openapi.editor.markup.TextAttributes;
import com.intellij.ui.AppUIUtil;
import com.intellij.ui.JBColor;

import javax.swing.*;
import java.awt.*;
import java.util.HashMap;
import java.util.Map;

public class ThemeHelper {
    private static EditorColorsSchemeImpl activeThemeEditorScheme = null;
    private static Map<String, TextAttributes> themeEditorBackupTextAttributes = null;
    private static UIDefaults uiDefaults = null;
    private static boolean isBright = false;
    private static UIManager.LookAndFeelInfo oldTheme = null;
    private static UIThemeBasedLookAndFeelInfo activeTheme = null;

    private static boolean isEnabled = false;

    private static void setEditorSchemeTempMode(EditorColorsSchemeImpl editorColorsScheme, boolean enabled) {
        editorColorsScheme.getMetaProperties().setProperty("TEMP_SCHEME_KEY", enabled ? "true" : "false");
    }

    public static void enableTemporaryTheme(String themeName) {

        // Get current look and feel and save it
        LafManager lafManager = LafManager.getInstance();
        oldTheme = lafManager.getCurrentLookAndFeel();
        if (oldTheme == null)
            throw new RuntimeException("Failed to get currentLookAndFeel (old theme) object");

        // Save isBright state (We use it later when disabling the theme)
        isBright = JBColor.isBright();


        // Find the theme we need (also called "look and feel"
        UIManager.LookAndFeelInfo[] lookAndFeelInfos = lafManager.getInstalledLookAndFeels();
        UIManager.LookAndFeelInfo highContrastLaf = null;
        for (UIManager.LookAndFeelInfo lookAndFeelInfo : lookAndFeelInfos) {
            if (!themeName.equals(lookAndFeelInfo.getName())) continue;
            highContrastLaf = lookAndFeelInfo;
            break;
        }
        if (highContrastLaf == null)
            throw new RuntimeException("Theme \"" + themeName + "\" not found");


        UITheme uiTheme = ((UIThemeBasedLookAndFeelInfo) highContrastLaf).getTheme();


        activeTheme = new UIThemeBasedLookAndFeelInfo(uiTheme); // This is the theme we need


        EditorColorsManager editorColorsManager = EditorColorsManager.getInstance();


        activeThemeEditorScheme = (EditorColorsSchemeImpl) editorColorsManager.getScheme(uiTheme.getEditorSchemeName());
        if (activeThemeEditorScheme == null)
            throw new RuntimeException("Failed to find editor scheme with name " + uiTheme.getEditorSchemeName());


        // Enable temp mode for the editor scheme of the active theme
        setEditorSchemeTempMode(activeThemeEditorScheme, true);

        // Reset colors in UIManager to avoid getting colors from the old theme in the new theme
        if (uiDefaults == null) {
            try {
                UIManager.getDefaults().clear();
                UIManager.setLookAndFeel(highContrastLaf.getClassName());
            } catch (Exception e) {
                throw new RuntimeException("Failed to setup uiDefaults", e);
            }
            uiDefaults = (UIDefaults) UIManager.getDefaults().clone();
        }
        UIManager.getDefaults().clear();
        UIManager.getDefaults().putAll(uiDefaults);

        // Set look and feel in UIManager
        try {
            UIManager.setLookAndFeel(highContrastLaf.getClassName());
        } catch (Exception e) {
            setEditorSchemeTempMode(activeThemeEditorScheme, false);
            throw new RuntimeException("Failed in UIManager.setLookAndFeel(new DarculaLaf());", e);
        }

        // Install the new theme
        activeTheme.installTheme(UIManager.getLookAndFeelDefaults(), false);

        isEnabled = true;

    }

    public static void disableTemporaryTheme() {

        if (themeEditorBackupTextAttributes != null && activeThemeEditorScheme != null) {

            EditorColorsSchemeImpl editorColorsSchemeImpl = (EditorColorsSchemeImpl) activeThemeEditorScheme;
            Map<String, TextAttributes> attributesMap = editorColorsSchemeImpl.getDirectlyDefinedAttributes();

            for (Map.Entry<String, TextAttributes> entry : attributesMap.entrySet()) {
                TextAttributes textAttributes = entry.getValue();
                TextAttributes backupTextAttributes = themeEditorBackupTextAttributes.get(entry.getKey());
                textAttributes.copyFrom(backupTextAttributes);

            }

            themeEditorBackupTextAttributes = null;

        }

        if (oldTheme != null) {
            LafManager lafManager = LafManager.getInstance();
            lafManager.setCurrentLookAndFeel(activeTheme, false);
            lafManager.setCurrentLookAndFeel(oldTheme, false);
            AppUIUtil.updateForDarcula(!isBright);
            lafManager.updateUI();
            setEditorSchemeTempMode(activeThemeEditorScheme, false);

        }

        isEnabled = false;
    }


    private static void removeWhileBackgroundsInEditorScheme(EditorColorsScheme editorColorsScheme) {
//        editorColorsScheme.setAttributes(TextAttributesKey.find("INLINE_PARAMETER_HINT"), new TextAttributes());

        EditorColorsSchemeImpl editorColorsSchemeImpl = (EditorColorsSchemeImpl) editorColorsScheme;

        Map<String, TextAttributes> attributesMap = editorColorsSchemeImpl.getDirectlyDefinedAttributes();

        themeEditorBackupTextAttributes = new HashMap<>();

        for (Map.Entry<String, TextAttributes> entry : attributesMap.entrySet()) {
            themeEditorBackupTextAttributes.put(entry.getKey(),
                    entry.getValue() != null ? entry.getValue().clone() : null);
        }


        for (Map.Entry<String, TextAttributes> entry : attributesMap.entrySet()) {

            TextAttributes textAttributes = entry.getValue();

            Color bkColor = textAttributes.getBackgroundColor();
            if (bkColor == null) continue;

            Color fbColor = textAttributes.getForegroundColor();
            if (fbColor == null) continue;

            int brBackgroundColor = (int) ((bkColor.getRed() + bkColor.getGreen() + bkColor.getBlue()) * (bkColor.getAlpha() / 255.0)) / 3;
            int brForegroundColor = (int) ((fbColor.getRed() + fbColor.getGreen() + fbColor.getBlue()) * (fbColor.getAlpha() / 255.0)) / 3;

            if (brForegroundColor < brBackgroundColor) {
                textAttributes.setBackgroundColor(fbColor);
                textAttributes.setForegroundColor(bkColor);
            }

        }

    }

    /**
     * This will enable High Contrast theme temporarily so if you restart IDE, the original theme will still be set
     */
    public static void enableHighContrastMode() {
        enableTemporaryTheme("High contrast");
        AppUIUtil.updateForDarcula(true);
        removeWhileBackgroundsInEditorScheme(activeThemeEditorScheme);
    }

    /**
     * Disable the high contrast mode and restore to the original mode
     */
    public static void disableHighContrastMode() {
        disableTemporaryTheme();

    }

    public static boolean isTemporaryThemeEnabled() {
        return isEnabled;
    }
}