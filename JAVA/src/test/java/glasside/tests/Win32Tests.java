package glasside.tests;

import com.intellij.testFramework.fixtures.BasePlatformTestCase;
import com.sun.jna.Pointer;
import com.sun.jna.platform.win32.WinDef;
import glasside.helpers.WindowsHelpers;

public class Win32Tests extends BasePlatformTestCase {

    private static final long winId = 0x00000000001005E6;

    public void testGetWindowTransparency() {
        int alpha = WindowsHelpers.getWindowTransparency(new WinDef.HWND(Pointer.createConstant(winId)));
        System.out.println(alpha);
    }

    public void testSetWindowTransparency() {
        WindowsHelpers.setWindowTransparency(new WinDef.HWND(Pointer.createConstant(winId)),127);
        System.out.println(1);
    }

}
