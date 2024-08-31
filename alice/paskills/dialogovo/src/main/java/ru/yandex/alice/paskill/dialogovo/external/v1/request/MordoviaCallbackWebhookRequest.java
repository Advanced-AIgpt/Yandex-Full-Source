package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

@Getter
public class MordoviaCallbackWebhookRequest extends WebhookRequestBase {

    @JsonProperty("canvas_command")
    private final CanvasCallback canvasCallback;

    public MordoviaCallbackWebhookRequest(Meta meta, WebhookSession session, WebhookState state,
                                          CanvasCallback canvasCallback) {
        super(meta, session, state);
        this.canvasCallback = canvasCallback;
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEventConstants.INSTANCE.getCanvasCallbackMetricaEvent();
    }
}
