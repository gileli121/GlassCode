package glasscode;

import com.intellij.openapi.application.ApplicationManager;
import com.intellij.openapi.components.ServiceManager;
import com.intellij.openapi.project.Project;
import com.intellij.openapi.project.ProjectManagerListener;
import org.jetbrains.annotations.NotNull;


public class PluginInitializer implements ProjectManagerListener {

    private static int openedProjects = 0;

    /**
     * Invoked on project open.
     *
     * @param project opening project
     */
    @Override
    public void projectOpened(@NotNull Project project) {
        // Ensure this isn't part of testing
        if (ApplicationManager.getApplication().isUnitTestMode()) {
            return;
        }

        openedProjects++;

        PluginMain pluginMain = ServiceManager.getService(project, PluginMain.class);
        pluginMain.init();

    }

    /**
     * Invoked on project close.
     *
     * @param project closing project
     */
    @Override
    public void projectClosed(@NotNull Project project) {
        // Ensure this isn't part of testing
        if (ApplicationManager.getApplication().isUnitTestMode()) {
            return;
        }

        openedProjects--;

        PluginMain pluginMain = ServiceManager.getService(project, PluginMain.class);
        pluginMain.dispose();
    }

    public static int getOpenedProjectsCount() {
        return openedProjects;
    }

}