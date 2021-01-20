package glasside;

import com.intellij.ide.plugins.IdeaPluginDescriptor;
import com.intellij.ide.plugins.PluginManager;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.platform.win32.BaseTSD;
import com.sun.jna.platform.win32.User32;
import com.sun.jna.platform.win32.WinDef;
import com.sun.jna.platform.win32.WinUser;
import glasside.helpers.WindowsHelpers;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.List;

public class Renderer {

    private static final int MINIMUM_REQUIRED_OS_BUILD_NUMBER = 19041;

    private static Path rendererPath = null;
    private final long windowId;
    private final WinDef.HWND windowHwnd;
    private Process rendererProcess = null;
    private WinDef.HWND rendererMsgHwnd = null;
    private int osBuildNumber = 0;

    public Renderer(long windowId) {
        this.windowId = windowId;
        this.windowHwnd = new WinDef.HWND(Pointer.createConstant(windowId));
        this.osBuildNumber = WindowsHelpers.getWindowsBuildNumber();
    }


    public static class CommandId {
        public static int EXIT = 0;
        public static int SET_OPACITY = 1;
        public static int SET_BRIGHTNESS = 2;
        public static int SET_TEXT_BRIGHTNESS = 3;
        public static int SET_BLUR_TYPE = 4;
    }


    // endregion

    // region API/Public methods
    public void enableGlassEffect(boolean isCudaEnabled, int opacityLevel, int brightnessLevel, int textExtraBrightnessLevel,
                                  int blurType) {

        if (isGlassEffectRunning())
            return;

        if (osBuildNumber < MINIMUM_REQUIRED_OS_BUILD_NUMBER)
            throw new RuntimeException("The build number (" + osBuildNumber + ") of Windows 10 is too old of this effect. " +
                    "in order to enable it, you need to update Windows 10 to the latest update."
                    + System.lineSeparator() + System.lineSeparator() +
                    "Minimum required OS build number: " + MINIMUM_REQUIRED_OS_BUILD_NUMBER);

        try {

            rendererMsgHwnd = null;

            Path rendererPath = getRendererPath();

            // Start the renderer process with the defined settings
            {
                try {
                    rendererProcess = new ProcessBuilder
                            (
                                    rendererPath.toString(),
                                    isCudaEnabled ? "1" : "0",
                                    String.valueOf(windowId),
                                    String.valueOf(opacityLevel),
                                    String.valueOf(brightnessLevel),
                                    String.valueOf(textExtraBrightnessLevel),
                                    String.valueOf(blurType)
                            ).start();
                } catch (IOException e) {
                    String rendererLogs = getRendererLogs();
                    rendererProcess = null;
                    throw new RuntimeException("Failed to start Renderer.exe." +
                            "Renderer Logs: " + rendererLogs +
                            "Exception: " + e.getMessage());
                }


            }

            long rendererMsgWindow = 0;
//         Get the hwnd communication window from the renderer process
            {
                BufferedReader reader =
                        new BufferedReader(new InputStreamReader(rendererProcess.getInputStream()));
                String line;
                try {
                    while ((line = reader.readLine()) != null) {
                        if (line.startsWith("MSG_WINDOW=")) {
                            rendererMsgWindow = Long.parseLong(line.replace("MSG_WINDOW=", ""), 16);
                            break;
                        }
                    }
                } catch (Exception e) {
                    if (rendererProcess.isAlive())
                        rendererProcess.destroy();
                    throw new RuntimeException("Failed to communicate with the native renderer process. " +
                            "Exception: " + e.getMessage());
                }


                if (rendererMsgWindow == 0) {
                    if (rendererProcess.isAlive())
                        rendererProcess.destroy();

                    throw new RuntimeException("Failed to communicate with the native renderer process. " +
                            "Could not get communication window handle." +
                            "Renderer Logs: " + getRendererLogs());
                } else {
                    rendererMsgHwnd = new WinDef.HWND(Pointer.createConstant(rendererMsgWindow));
                }
            }


            // Need it to fix bug that sometimes the IDE process if frozen for some reason. This fix the issue
            try {
                rendererProcess.getOutputStream().close();
                rendererProcess.getInputStream().close();
                rendererProcess.getErrorStream().close();
            } catch (IOException e) {
                rendererProcess.destroy();
                throw new RuntimeException("Failed to detach from output stream of the Renderer process");
            }

        } catch (Exception e) {
            if (isGlassEffectRunning()) {
                try {
                    disableGlassEffect();
                } catch (Exception ignored) {
                }
            }
            throw e;
        }


    }


    public void disableGlassEffect() {

        if (isGlassEffectRunning()) {
            sendMessage(CommandId.EXIT, 0);

            if (rendererMsgHwnd != null) {
                sendMessage(CommandId.EXIT, 0);
                long timer = System.currentTimeMillis();
                while (rendererProcess.isAlive()) {
                    if (System.currentTimeMillis() - timer > 5000) {
                        rendererProcess.destroy();
                        break;
                    }
                }
            } else {
                rendererProcess.destroy();
            }
        }

        // Get the current transparency of the window. if it is not 100%, recover it now using JNA
        int transparency = WindowsHelpers.getWindowTransparency(windowHwnd);
        if (transparency < 255)
            WindowsHelpers.setWindowTransparency(windowHwnd, 255);

        rendererProcess = null;
        rendererMsgHwnd = null;
    }

    public boolean isGlassEffectRunning() {
        return rendererProcess != null && rendererProcess.isAlive();
    }

    private void abortIfNotEnabled() {
        if (!isGlassEffectRunning())
            throw new RuntimeException("Renderer process is not running. Can't processed");
    }

    public void setBlurType(int blurType) {
        abortIfNotEnabled();
        sendMessage(CommandId.SET_BLUR_TYPE, blurType);
    }

    public void setOpacityLevel(int opacityLevel) {
        abortIfNotEnabled();
        sendMessage(CommandId.SET_OPACITY, opacityLevel);
    }

    public void setBrightnessLevel(int brightnessLevel) {
        abortIfNotEnabled();
        sendMessage(CommandId.SET_BRIGHTNESS, brightnessLevel);
    }

    public void setTextExtraBrightnessLevel(int textExtraBrightnessLevel) {
        abortIfNotEnabled();
        sendMessage(CommandId.SET_TEXT_BRIGHTNESS, textExtraBrightnessLevel);
    }

    // endregion

    // region utility methods


    public static class MsgStruct extends Structure {
        public int commandId;
        public int commandValue;

        public MsgStruct(int commandId, int commandValue) {
            this.commandId = commandId;
            this.commandValue = commandValue;
            write();
        }

        protected List<String> getFieldOrder() {
            return Arrays.asList("commandId", "commandValue");
        }
    }

    private void sendMessage(int commandId, int commandValue) {

        MsgStruct myData = new MsgStruct(commandId, commandValue);

        WinUser.COPYDATASTRUCT copyDataStruct = new WinUser.COPYDATASTRUCT();
        copyDataStruct.dwData = new BaseTSD.ULONG_PTR(0);
        copyDataStruct.cbData = myData.size();
        copyDataStruct.lpData = myData.getPointer();
        copyDataStruct.write();

        WinDef.LRESULT result = User32.INSTANCE.SendMessage(rendererMsgHwnd, WinUser.WM_COPYDATA,
                new WinDef.WPARAM(windowId), new WinDef.LPARAM(Pointer.nativeValue(copyDataStruct.getPointer())));

        if (result.intValue() != 0) {
            throw new RuntimeException("Failed to send message with commandId=" + commandId + ", commandValue="
                    + commandValue);
        }
    }

    private String getRendererLogs() {
        return "Error: Getting renderer logs was not implemented yet";
    }

    private static Path getRendererPath() {
        if (rendererPath == null) {
            Path tmpPath = null;
            IdeaPluginDescriptor[] plugins = PluginManager.getPlugins();
            for (IdeaPluginDescriptor plugin : plugins) {
                if ("gileli121.glasside.windows".equals(plugin.getPluginId().toString())) {
                    tmpPath = Paths.get(plugin.getPluginPath().toString(), "bin", "Renderer.exe");
                    break;
                }
            }

            if (tmpPath == null)
                throw new RuntimeException("Failed to get the plugin path");

            if (!tmpPath.toFile().exists())
                throw new RuntimeException("File Renderer.exe does not exists");

            rendererPath = tmpPath;
        }
        return rendererPath;
    }

    // endregion
}
