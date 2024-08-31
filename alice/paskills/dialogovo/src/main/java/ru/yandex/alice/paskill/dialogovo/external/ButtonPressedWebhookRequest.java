package ru.yandex.alice.paskill.dialogovo.external;

import lombok.Data;
import lombok.EqualsAndHashCode;

import ru.yandex.alice.paskill.dialogovo.external.v1.request.ButtonPressedRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.Meta;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookRequest;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookSession;
import ru.yandex.alice.paskill.dialogovo.external.v1.request.WebhookState;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

@Data
@EqualsAndHashCode(callSuper = true)
public class ButtonPressedWebhookRequest extends WebhookRequest<ButtonPressedRequest> {

    public ButtonPressedWebhookRequest(Meta meta, WebhookSession session, ButtonPressedRequest request,
                                       WebhookState state) {
        super(meta, session, request, state);
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEventConstants.INSTANCE.getButtonPressedMetricaEvent();
    }
}
