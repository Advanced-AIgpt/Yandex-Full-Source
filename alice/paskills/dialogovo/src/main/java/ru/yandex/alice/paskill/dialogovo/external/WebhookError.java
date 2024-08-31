package ru.yandex.alice.paskill.dialogovo.external;

import javax.annotation.Nullable;

public record WebhookError(@Nullable String path, @Nullable String message, WebhookErrorCode code) {

    public static WebhookError create(String path, WebhookErrorCode code, String message) {
        return new WebhookError(path, message, code);
    }

    public static WebhookError create(String path, WebhookErrorCode code) {
        return new WebhookError(path, null, code);
    }

    public static WebhookError create(WebhookErrorCode code) {
        return new WebhookError(null, null, code);
    }

}
