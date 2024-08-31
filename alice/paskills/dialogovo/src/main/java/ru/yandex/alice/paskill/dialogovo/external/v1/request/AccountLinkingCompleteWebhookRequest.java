package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Getter;

import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEventConstants;

@Getter
public class AccountLinkingCompleteWebhookRequest extends WebhookRequestBase {

    @JsonProperty("account_linking_complete_event")
    private final AccountLinkingCompleteEvent accountLinkingCompleteEvent = AccountLinkingCompleteEvent.INSTANCE;

    public AccountLinkingCompleteWebhookRequest(Meta meta, WebhookSession session, WebhookState state) {
        super(meta, session, state);
    }

    @Override
    public MetricaEvent getMetricaEvent() {
        return MetricaEventConstants.INSTANCE.getAccountLinkingCompleteMetricaEvent();
    }

    @JsonSerialize
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    private static class AccountLinkingCompleteEvent {
        public static final AccountLinkingCompleteEvent INSTANCE = new AccountLinkingCompleteEvent();
    }
}
