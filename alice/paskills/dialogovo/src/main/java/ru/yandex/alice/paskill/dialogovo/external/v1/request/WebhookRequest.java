package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import javax.annotation.Nullable;

import lombok.Getter;
import lombok.ToString;

@Getter
@ToString
public abstract class WebhookRequest<TRequest extends RequestBase> extends WebhookRequestBase {
    private final TRequest request;

    public WebhookRequest(Meta meta, WebhookSession session, TRequest request, @Nullable WebhookState state) {
        super(meta, session, state);
        this.request = request;
    }
}
