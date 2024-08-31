package ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics;

import ru.yandex.alice.kronstadt.core.analytics.AnalyticsInfoAction;

public class PurchaseCompleteAction {
    public static final AnalyticsInfoAction INSTANCE = new AnalyticsInfoAction(
            "external_skill.purchase_complete",
            "external_skill.purchase_complete",
            "Возврат в навык после покупки"
    );

    private PurchaseCompleteAction() {
        throw new UnsupportedOperationException();
    }
}
