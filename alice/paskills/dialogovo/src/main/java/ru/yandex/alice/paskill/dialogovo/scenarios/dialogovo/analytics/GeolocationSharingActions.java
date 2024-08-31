package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;

public class GeolocationSharingActions {
    public static final AnalyticsInfoAction GEOLOCATION_SHARING_ALLOWED = new AnalyticsInfoAction(
            "geolocation_sharing_allowed",
            "geolocation_sharing_allowed",
            "Пользователь разрешил шаринг геолокации"
    );

    public static final AnalyticsInfoAction GEOLOCATION_SHARING_REJECTED = new AnalyticsInfoAction(
            "geolocation_sharing_rejected",
            "geolocation_sharing_rejected",
            "Пользователь запретил шаринг геолокации"
    );

    private GeolocationSharingActions() {
        throw new UnsupportedOperationException("utility class");
    }
}
