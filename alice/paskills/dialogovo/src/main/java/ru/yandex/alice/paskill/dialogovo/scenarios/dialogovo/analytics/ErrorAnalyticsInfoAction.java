package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;

public class ErrorAnalyticsInfoAction {
    public static final AnalyticsInfoAction REQUEST_FAILURE = new AnalyticsInfoAction(
            "external_skill.request_failure",
            "external_skill.request_failure",
            "Диалог не отвечает"
    );

    public static final AnalyticsInfoAction UNSUPPORTED_SURFACE_FAILURE = new AnalyticsInfoAction(
            "external_skill.unsupported_surface_failure",
            "external_skill.unsupported_surface_failure",
            "Данная поверхность не поддерживается"
    );

    public static final AnalyticsInfoAction ACCOUNT_LINKING_FAILURE = new AnalyticsInfoAction(
            "external_skill.account_linking_failure",
            "external_skill.account_linking_failure",
            "Ошибка при связке аккаунтов"
    );

    public static final AnalyticsInfoAction USER_AGREEMENT_LIST_FAILURE = new AnalyticsInfoAction(
        "external_skill.user_agreement_list_failure",
            "external_skill.user_agreement_list_failure",
            "Ошибка при отображении списка пользовательских соглашений"
    );

    public static final AnalyticsInfoAction USER_AGREEMENT_CALLBACK_FAILURE = new AnalyticsInfoAction(
            "external_skill.user_agreement_callback_failure",
            "external_skill.user_agreement_callback_failure",
            "Ошибка при обработке колбека принятия/отказа от пользовательского соглашения в навыке"
    );

    public static final AnalyticsInfoAction START_PURCHASE_FAILURE = new AnalyticsInfoAction(
            "external_skill.start_purchase_failure",
            "external_skill.start_purchase_failure",
            "Ошибка при оплате в навыке");

    public static final AnalyticsInfoAction PURCHASE_NOT_FOUND_FAILURE = new AnalyticsInfoAction(
            "external_skill.purchase_not_found_failure",
            "external_skill.purchase_not_found_failure",
            "Покупка не найдена");

    public static final AnalyticsInfoAction MUSIC_SKILL_PRODUCT_ACTIVATION_FAILURE = new AnalyticsInfoAction(
            "external_skill.music_skill_product_activation_failure",
            "external_skill.music_skill_product_activation_failure",
            "Ошибка при активации продукта в навыке");

    public static final AnalyticsInfoAction ACTIVATE_SKILL_PRODUCT = new AnalyticsInfoAction(
            "external_skill.activate_skill_product_failure",
            "external_skill.activate_skill_product_failure",
            "Ошибка при активации продукта навыка");

    public static final AnalyticsInfoAction CALLBACK_PURCHASE_FAILURE = new AnalyticsInfoAction(
            "external_skill.callback_purchase_failure",
            "external_skill.callback_purchase_failure",
            "Ошибка во время подтверждения покупки в навыке");

    private ErrorAnalyticsInfoAction() {
        throw new UnsupportedOperationException();
    }
}
