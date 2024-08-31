package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;

public class SkillProductActivationActions {
    public static final AnalyticsInfoAction ACTIVATION_BY_MUSIC_START_ACTION =
            new AnalyticsInfoAction(
                    "skill_product_activation_start_by_music",
                    "skill_product_activation_start_by_music",
                    "Команда распознания музыки для активации продукта"
            );

    public static final AnalyticsInfoAction ACTIVATED_BY_MUSIC_ACTION =
            new AnalyticsInfoAction(
                    "skill_product_activation_complete_by_music",
                    "skill_product_activation_complete_by_music",
                    "Активация продукта с помощью музыки завершена"
            );

    public static final AnalyticsInfoAction ACTIVATION_FAILED_MUSIC_NOT_RECOGNIZED_ACTION =
            new AnalyticsInfoAction(
                    "skill_product_activation_failed_music_not_recognized",
                    "skill_product_activation_failed_music_not_recognized",
                    "Активация продукта не выполнена. Для музыки не найден продукт"
            );

    public static final AnalyticsInfoAction ACTIVATION_FAILED_MUSIC_NOT_PLAYING_ACTION =
            new AnalyticsInfoAction(
                    "skill_product_activation_failed_music_not_playing",
                    "skill_product_activation_failed_music_not_playing",
                    "Активация продукта не выполнена. Не играет музыка"
            );

    private SkillProductActivationActions() {
        throw new UnsupportedOperationException("utility class");
    }
}
