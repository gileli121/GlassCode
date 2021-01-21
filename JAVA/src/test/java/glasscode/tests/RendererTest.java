package glasscode.tests;


import com.intellij.testFramework.fixtures.BasePlatformTestCase;
import glasscode.Renderer;

public class RendererTest extends BasePlatformTestCase {

    private Renderer getWindowRenderer() {
        return null;//new WindowRenderer(0x0000000000080DEE);
    }

    public void testEnableRenderer() {
        Renderer renderer = getWindowRenderer();
        renderer.enableGlassEffect(true,0,50,0);
    }

    public void testDisableRenderer() throws InterruptedException {
        Renderer renderer = getWindowRenderer();
        renderer.enableGlassEffect(true,0,50,0);
        renderer.disableGlassEffect();
    }

}
