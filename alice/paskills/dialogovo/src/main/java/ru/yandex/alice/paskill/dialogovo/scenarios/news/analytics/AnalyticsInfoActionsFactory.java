package ru.yandex.alice.paskill.dialogovo.scenarios.news.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;
import ru.yandex.alice.paskill.dialogovo.scenarios.news.domain.NewsSkillInfo;

public class AnalyticsInfoActionsFactory {
    private AnalyticsInfoActionsFactory() {
        throw new UnsupportedOperationException();
    }


    public static AnalyticsInfoAction createActivateAction(NewsSkillInfo skill) {
        return new AnalyticsInfoAction("external_skill.news.activate", "external_skill.news.activate",
                String.format("Запуск новостей от «%s»", skill.getName()));
    }

    public static AnalyticsInfoAction createContinueAction(NewsSkillInfo skill) {
        return new AnalyticsInfoAction("external_skill.news.continue", "external_skill.news.continue",
                String.format("Продолжение новостей от «%s»", skill.getName()));
    }

    public static AnalyticsInfoAction createPostrollAction(NewsSkillInfo skill) {
        return new AnalyticsInfoAction("external_skill.news.postroll", "external_skill.news.postroll",
                String.format("Постролл новостей от «%s»", skill.getName()));
    }
}
