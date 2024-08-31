package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

@Getter
public class UtteranceWebhookRequest extends WebhookRequest<SimpleUtteranceRequest> {

    @JsonCreator
    public UtteranceWebhookRequest(@JsonProperty("meta") Meta meta,
                                   @JsonProperty("session") WebhookSession session,
                                   @JsonProperty("request") SimpleUtteranceRequest request,
                                   @Nullable @JsonProperty("state") WebhookState state) {

        super(meta, session, request, state);
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEventConstants.INSTANCE.getUtteranceMetricaEvent();
    }
}
