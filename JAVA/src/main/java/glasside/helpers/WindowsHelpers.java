package glasside.helpers;

import com.intellij.openapi.project.Project;
import com.intellij.openapi.wm.ToolWindowManager;
import com.sun.jna.Native;

import java.awt.*;

public class WindowsHelpers {

    public static long getIdeWindowOfProject(Project project) {
        Window ideWindow = ToolWindowManager.getInstance(project).getFocusManager().getLastFocusedIdeWindow();
        if (ideWindow == null)
            return 0;
        return Native.getWindowID(ideWindow);
    }

}
