package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.domain.SkillInfo;

public class ActivateSkillAction {

    private ActivateSkillAction() {
        throw new UnsupportedOperationException();
    }

    public static AnalyticsInfoAction create(SkillInfo skill) {
        return new AnalyticsInfoAction(
                "external_skill.activate",
                "external_skill.activate",
                String.format("Запуск навыка «%s»", skill.getName())
        );
    }
}
