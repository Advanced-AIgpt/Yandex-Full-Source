package ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct;

import javax.annotation.Nullable;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.Meta;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookState;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

public class SkillPurchaseConfirmationWebhookRequest extends WebhookRequest<SkillPurchaseConfirmationRequest> {

    public SkillPurchaseConfirmationWebhookRequest(
            Meta meta,
            WebhookSession session,
            SkillPurchaseConfirmationRequest skillPurchaseConfirmationRequest,
            @Nullable WebhookState state
    ) {
        super(meta, session, skillPurchaseConfirmationRequest, state);
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEventConstants.INSTANCE.getSkillPurchaseConfirmationMetricaEvent();
    }
}
