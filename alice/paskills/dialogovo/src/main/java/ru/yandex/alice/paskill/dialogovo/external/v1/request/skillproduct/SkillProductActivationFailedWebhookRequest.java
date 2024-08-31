package ru.yandex.alice.paskill.dialogovo.external.v1.request.skillproduct;

import javax.annotation.Nullable;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.Meta;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookState;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

public class SkillProductActivationFailedWebhookRequest extends WebhookRequest<SkillProductActivationFailedRequest> {

    public SkillProductActivationFailedWebhookRequest(
            Meta meta,
            WebhookSession session,
            SkillProductActivationFailedRequest skillProductActivatedRequest,
            @Nullable WebhookState state
    ) {
        super(meta, session, skillProductActivatedRequest, state);
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEventConstants.INSTANCE.getSkillProductActivationFailedMetricaEvent();
    }
}
