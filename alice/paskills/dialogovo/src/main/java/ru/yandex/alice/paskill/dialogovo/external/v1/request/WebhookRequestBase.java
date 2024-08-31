package ru.yandex.alice.paskill.dialogovo.external.v1.request;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.external.ApiVersion;
import ru.yandex.alice.paskill.dialogovo.service.appmetrica.MetricaEvent;

import static com.fasterxml.jackson.annotation.JsonInclude.Include.NON_ABSENT;

@Data
@JsonInclude(NON_ABSENT)
public abstract class WebhookRequestBase {
    private final Meta meta;
    private final WebhookSession session;
    private final ApiVersion version;
    @Nullable
    private final WebhookState state;

    protected WebhookRequestBase(Meta meta, WebhookSession session, @Nullable WebhookState state) {
        this.meta = meta;
        this.session = session;
        this.version = ApiVersion.V1_0;
        this.state = state != null && !state.equals(WebhookState.EMPTY) ? state : null;
    }

    @JsonIgnore
    public abstract MetricaEvent getMetricaEvent();
}
