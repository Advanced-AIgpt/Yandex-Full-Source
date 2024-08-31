package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;

public class DeactivateSkillAction {

    public static final AnalyticsInfoAction INSTANCE = new AnalyticsInfoAction(
            "external_skill.deactivate",
            "external_skill.deactivate",
            "Завершение навыка");

    private DeactivateSkillAction() {
        throw new UnsupportedOperationException();
    }

}
