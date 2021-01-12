package glasside.helpers;

import com.intellij.openapi.project.Project;
import com.intellij.openapi.wm.ToolWindowManager;
import com.sun.jna.Native;
import com.sun.jna.platform.win32.*;
import com.sun.jna.ptr.ByteByReference;
import com.sun.jna.ptr.IntByReference;

import java.awt.*;

public class WindowsHelpers {

    public static long getIdeWindowOfProject(Project project) {
        Window ideWindow = ToolWindowManager.getInstance(project).getFocusManager().getLastFocusedIdeWindow();
        if (ideWindow == null)
            return 0;
        return Native.getWindowID(ideWindow);
    }


    public static int getWindowTransparency(WinDef.HWND windowHwnd) {
        ByteByReference alpha = new ByteByReference();
        // According to my tests, this API call will return false when the transparency is 255 (This is unlike when using it from C)
        // so we just assume it as 255 and return 255
        if (!User32.INSTANCE.GetLayeredWindowAttributes(windowHwnd, null, alpha, new IntByReference((byte) 0x00000002)))
            return 255;

        int transparency = alpha.getValue();

        // Here we fix some strange behavior that the value may be negative.
        // This will fix it and make sure it between 0 and 255
        if (transparency < 0)
            transparency = 128 + (128 - Math.abs(transparency));

        return transparency;
    }


    public static void setWindowTransparency(WinDef.HWND windowHwnd, int transparency) {

        if (User32.INSTANCE.SetWindowLong(windowHwnd, User32.GWL_EXSTYLE, User32.WS_EX_LAYERED) == 0) {
            throw new RuntimeException("Failed to set transparency of " + transparency + " to window. Failed in SetWindowLong" +
                    ". Kernel32 Error: " + Kernel32.INSTANCE.GetLastError());
        }

        if (!User32.INSTANCE.SetLayeredWindowAttributes(
                windowHwnd, 0, (byte) transparency, User32.LWA_ALPHA)) {
            throw new RuntimeException("Failed to set transparency of " + transparency + " to window. Failed in SetLayeredWindowAttributes." +
                    " Kernel32 Error: " + Kernel32.INSTANCE.GetLastError());
        }
    }


    public static int getWindowsBuildNumber() {
        try {
            return Integer.parseInt(Advapi32Util.registryGetStringValue(
                    WinReg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CurrentBuild"));
        } catch (Exception e) {
            return 0;
        }
    }

}

