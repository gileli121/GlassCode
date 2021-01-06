package glasside.helpers;

import com.intellij.ide.plugins.IdeaPluginDescriptor;
import com.intellij.ide.plugins.PluginManager;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.platform.win32.BaseTSD;
import com.sun.jna.platform.win32.User32;
import com.sun.jna.platform.win32.WinDef;
import com.sun.jna.platform.win32.WinUser;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.List;

public class WindowRenderer {

    private static Path rendererPath = null;
    private final long windowId;
    private Process rendererProcess = null;
    private long rendererMsgWindow = 0;
    private WinDef.HWND rendererMsgHwnd = null;
    private String rendererLastLogs = "";
    private boolean isRendererRunning = false;
    private boolean isCudaEnabled = false;

    public WindowRenderer(long windowId) {
        this.windowId = windowId;
    }

    public static class CommandId {
        public static int EXIT = 0;
        public static int SET_OPACITY = 1;
        public static int SET_BRIGHTNESS = 2;
        public static int SET_BLUR_TYPE = 3;
    }

    // region initialize / un-initialize methods
    public void dispose() {
        disable();
    }


    // endregion

    // region API/Public methods
    public void enableEffect(boolean isCudaEnabled, int opacityLevel, int brightnessLevel, int blurType) {

        if (isEnabled())
            return;

        rendererMsgWindow = 0;
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

        // Get the hwnd communication window from the renderer process
        {
            BufferedReader reader =
                    new BufferedReader(new InputStreamReader(rendererProcess.getInputStream()));
            StringBuilder builder = new StringBuilder();
            String line;
            try {
                while ((line = reader.readLine()) != null) {
                    if (line.startsWith("MSG_WINDOW=")) {
                        rendererMsgWindow = Long.parseLong(line.replace("MSG_WINDOW=", ""), 16);
                        break;
                    }
                    builder.append(line);
                    builder.append(System.lineSeparator());
                }
                rendererLastLogs += System.lineSeparator() + builder.toString();


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

    }


    public void disable() {
        if (!isEnabled()) return;
        sendMessage(CommandId.EXIT, 0);
    }

    public boolean isEnabled() {
        return rendererProcess != null && rendererProcess.isAlive();
    }

    private void abortIfNotEnabled() {
        if (!isEnabled())
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
        BufferedReader reader =
                new BufferedReader(new InputStreamReader(rendererProcess.getInputStream()));
        StringBuilder builder = new StringBuilder();
        String line = null;
        boolean foundText = false;
        try {
            while ((line = reader.readLine()) != null) {
                builder.append(line);
                builder.append(System.getProperty("line.separator"));
                foundText = true;
            }
            if (foundText)
                rendererLastLogs += builder.toString();

            return rendererLastLogs;
        } catch (IOException e) {
            rendererLastLogs = null;
            return "Failed to get Renderer.exe logs";
        }
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
