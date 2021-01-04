package glasside;

import com.intellij.openapi.project.Project;

public class GlassIdeContext {
    private Project project = null;

    public GlassIdeContext(Project project) {
        this.project = project;
    }

    public void initialize() {

    }

    public void dispose() {

    }
}
