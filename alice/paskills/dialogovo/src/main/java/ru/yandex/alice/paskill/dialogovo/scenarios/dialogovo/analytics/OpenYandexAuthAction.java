package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;

public class OpenYandexAuthAction {
    public static final AnalyticsInfoAction INSTANCE = new AnalyticsInfoAction(
            "external_skill.open_yandex_auth",
            "external_skill.open_yandex_auth",
            "Переход на страницу с авторизацией в Яндексе");

    private OpenYandexAuthAction() {
        throw new UnsupportedOperationException();
    }

}
