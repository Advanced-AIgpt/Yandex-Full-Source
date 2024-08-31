package ru.yandex.alice.paskill.dialogovo.external.v1.request.show;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.Meta;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookState;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;


public class ShowPullWebhookRequest extends WebhookRequest<ShowPullRequest> {

    @JsonCreator
    public ShowPullWebhookRequest(@JsonProperty("meta") Meta meta,
                                  @JsonProperty("session") WebhookSession session,
                                  @JsonProperty("request") ShowPullRequest request,
                                  @JsonProperty("state") WebhookState state) {

        super(meta, session, request, state);
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEventConstants.INSTANCE.getShowPullMetricaEvent();
    }
}
