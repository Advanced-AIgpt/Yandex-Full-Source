package ru.yandex.alice.paskill.dialogovo.webhook.client;

import java.time.Duration;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.stream.Collectors;

import lombok.Data;

import ru.yandex.alice.paskill.dialogovo.external.WebhookError;
import ru.yandex.alice.paskill.dialogovo.external.v1.response.WebhookResponse;
import ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.analytics.ProxyType;

@Data
public class WebhookRequestResult {
    private final Optional<String> rawResponse;
    private final Optional<WebhookResponse> response;
    private final List<WebhookError> errors;
    private final Optional<Exception> cause;
    private final Duration callDuration;
    private final ProxyType proxyType;

    public Optional<String> getRawResponse() {
        return rawResponse;
    }

    public Optional<WebhookResponse> getResponse() {
        return response;
    }

    public Optional<Exception> getCause() {
        return cause;
    }

    public Duration getCallDuration() {
        return callDuration;
    }

    public ProxyType getProxyType() {
        return proxyType;
    }

    public boolean hasErrors() {
        return errors.size() > 0;
    }

    public String formatErrors() {
        if (!hasErrors()) {
            return "";
        }

        return errors.stream()
                .map(error -> String.format("[%s]: %s", error.code(), Objects.requireNonNullElse(error.path(), "")))
                .collect(Collectors.joining(", "));
    }

    public List<WebhookError> getErrors() {
        return errors;
    }
}
