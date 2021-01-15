package glasside.helpers;

import com.intellij.notification.Notification;
import com.intellij.notification.NotificationType;
import com.intellij.notification.Notifications;

public class PluginUiHelpers {
    private static final String GLASSIDE_NOTIFICATION_GROUP = "GlassIDE";

    public static void showErrorNotification(String errorMessage) {
        Notifications.Bus.notify(new Notification(GLASSIDE_NOTIFICATION_GROUP, "GlassIDE: Can't perform operation",
                "Error: " + errorMessage, NotificationType.ERROR));

    }

    public static void showErrorNotificationAndAbort(String errorMessage) {
        showErrorNotification(errorMessage);
        throw new RuntimeException();
    }


    public static void showInfoNotification(String infoMessage) {
        Notifications.Bus.notify(new Notification(GLASSIDE_NOTIFICATION_GROUP, "GlassIDE",
                infoMessage, NotificationType.INFORMATION));
    }

}
