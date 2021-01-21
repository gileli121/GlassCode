package glasscode.helpers;

import com.intellij.notification.Notification;
import com.intellij.notification.NotificationType;
import com.intellij.notification.Notifications;
import glasscode.PluginConstants;

public class PluginUiHelpers {
    private static final String NOTIFICATION_GROUP = PluginConstants.PLUGIN_NAME;

    public static void showErrorNotification(String errorMessage) {
        Notifications.Bus.notify(new Notification(NOTIFICATION_GROUP, PluginConstants.PLUGIN_NAME + ": Can't perform operation",
                "Error: " + errorMessage, NotificationType.ERROR));

    }

    public static void showErrorNotificationAndAbort(String errorMessage) {
        showErrorNotification(errorMessage);
        throw new RuntimeException();
    }


    public static void showInfoNotification(String infoMessage) {
        Notifications.Bus.notify(new Notification(NOTIFICATION_GROUP, PluginConstants.PLUGIN_NAME,
                infoMessage, NotificationType.INFORMATION));
    }

}
