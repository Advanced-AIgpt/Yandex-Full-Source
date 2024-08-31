package ru.yandex.alice.paskill.dialogovo.external.v1.request.audio;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.Meta;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookState;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;

@Getter
public class AudioPlayerEventWebhookRequest extends WebhookRequest<AudioPlayerEventRequest> {

    @JsonIgnore
    private final String metricaTypeValue;
    @JsonIgnore
    private final ObjectMapper mapper;

    public AudioPlayerEventWebhookRequest(Meta meta, WebhookSession session, WebhookState state,
                                          AudioPlayerEventRequest audioPlayerEventRequest, ObjectMapper objectMapper) {
        super(meta, session, audioPlayerEventRequest, state);
        this.metricaTypeValue = audioPlayerEventRequest.getType().getMetricaValue();
        this.mapper = objectMapper;
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEvent.TypedMetricaEvent.create("audio_player_event", metricaTypeValue, mapper);
    }
}
