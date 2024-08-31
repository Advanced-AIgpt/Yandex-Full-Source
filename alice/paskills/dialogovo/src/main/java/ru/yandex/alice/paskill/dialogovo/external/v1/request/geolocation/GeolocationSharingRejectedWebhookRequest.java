package ru.yandex.alice.paskill.dialogovo.external.v1.request.geolocation;

import javax.annotation.Nullable;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.Meta;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookState;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

public class GeolocationSharingRejectedWebhookRequest extends WebhookRequest<GeolocationSharingRejectedRequest> {

    public GeolocationSharingRejectedWebhookRequest(
            Meta meta,
            WebhookSession session,
            GeolocationSharingRejectedRequest geolocationSharingRejectedRequest,
            @Nullable WebhookState state
    ) {
        super(meta, session, geolocationSharingRejectedRequest, state);
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEventConstants.INSTANCE.getSkillGeolocationRejectedMetricaEvent();
    }
}
