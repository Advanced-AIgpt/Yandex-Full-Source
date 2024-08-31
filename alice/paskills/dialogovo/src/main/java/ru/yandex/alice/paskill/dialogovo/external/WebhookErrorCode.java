package ru.yandex.alice.paskill.dialogovo.external;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

public enum WebhookErrorCode {
    INVALID_RESPONSE("INVALID_RESPONSE", "Недопустимый ответ", true),
    INVALID_VALUE("INVALID_VALUE", "Недопустимое значение поля", false),
    TYPE_MISMATCH("TYPE_MISMATCH", "Недопустимый тип поля", false),
    MISSING_INTENT("MISSING_INTENT", "Отсутствует интент", false),
    EMPTY_RESPONSE("EMPTY_RESPONSE", "Пустой ответ", true),
    RESPONSE_TO_LONG("RESPONSE_TO_LONG", "Размер ответа превышает допустимую длину", true),
    TIME_OUT("TIME_OUT", "Webhook не ответил за отведенное время", true),
    HTTP_ERROR_300("HTTP_ERROR", "HTTP ошибка в ответе webhook: 300", true),
    HTTP_ERROR_400("HTTP_ERROR", "HTTP ошибка в ответе webhook: 400", true),
    HTTP_ERROR_500("HTTP_ERROR", "HTTP ошибка в ответе webhook: 500", true),
    INVALID_SSL("SSL_ERROR", "Некорректный SSL-сертификат", true),
    UNKNOWN("UNKNOWN", "Неизвестная ошибка", true);

    @JsonProperty
    private final String code;
    private final String description;
    @JsonIgnore
    private final boolean fatal;

    WebhookErrorCode(String code, String description, boolean fatal) {
        this.code = code;
        this.description = description;
        this.fatal = fatal;
    }

    public WebhookError error(String path) {
        return WebhookError.create(path, this);
    }

    public WebhookError error() {
        return WebhookError.create(this);
    }

    public String getCode() {
        return this.code;
    }

    public String getDescription() {
        return this.description;
    }

    public boolean isFatal() {
        return this.fatal;
    }
}
