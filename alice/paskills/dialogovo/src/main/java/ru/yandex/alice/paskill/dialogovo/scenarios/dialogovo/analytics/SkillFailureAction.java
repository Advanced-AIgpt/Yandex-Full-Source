package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;

public class SkillFailureAction {
    private static final AnalyticsInfoAction INSTANCE = new AnalyticsInfoAction(
            "external_skill.request_failure",
            "external_skill.request_failure",
            "Диалог не отвечает");
}
