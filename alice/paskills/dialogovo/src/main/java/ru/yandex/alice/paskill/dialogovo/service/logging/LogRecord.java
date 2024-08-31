package ru.yandex.alice.paskill.dialogovo.service.logging;

import java.time.Instant;
import java.util.Arrays;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nullable;

import lombok.Data;
import lombok.Getter;
import lombok.ToString;

import ru.yandex.alice.paskill.dialogovo.external.WebhookErrorCode;

import static java.util.stream.Collectors.toMap;

@Data
@ToString(exclude = {"requestBody", "responseBody"})
class LogRecord {
    private final String skillId;
    private final String sessionId;
    private final long messageId;
    @Nullable
    private final String eventId;
    private final Instant timestamp;
    @Nullable
    private final String requestBody;
    @Nullable
    private final String responseBody;
    private final Status status;
    private final List<String> validationErrors;
    private final long callDuration;
    @Nullable
    private final String requestId;
    @Nullable
    private final String deviceId;
    @Nullable
    private final String uuid;

    @Getter
    enum Status {
        OK("OK", "OK", null),
        INVALID_RESPONSE("INVALID_RESPONSE", "Недопустимый ответ", WebhookErrorCode.INVALID_RESPONSE),
        EMPTY_RESPONSE("EMPTY_RESPONSE", "Пустой ответ", WebhookErrorCode.EMPTY_RESPONSE),
        RESPONSE_TO_LONG("RESPONSE_TO_LONG", "Размер ответа превышает допустимую длину",
                WebhookErrorCode.RESPONSE_TO_LONG),
        TIME_OUT("TIME_OUT", "Webhook не ответил за отведенное время", WebhookErrorCode.TIME_OUT),
        HTTP_ERROR_300("HTTP_3XX", "HTTP ошибка в ответе webhook: 300", WebhookErrorCode.HTTP_ERROR_300),
        HTTP_ERROR_400("HTTP_4XX", "HTTP ошибка в ответе webhook: 400", WebhookErrorCode.HTTP_ERROR_400),
        HTTP_ERROR_500("HTTP_5XX", "HTTP ошибка в ответе webhook: 500", WebhookErrorCode.HTTP_ERROR_500),
        UNKNOWN("UNKNOWN", "Неизвестная ошибка", WebhookErrorCode.UNKNOWN),
        VALIDATION_ERROR("VALIDATION_ERROR", "Ответ не соответствует требованиям API", null);
        private final String statusCode;
        private final String description;
        @Nullable
        private final WebhookErrorCode errorCode;
        private static final Map<WebhookErrorCode, Status> BY_ERROR_CODE = Arrays.stream(Status.values())
                .filter(it -> it.errorCode != null)
                .collect(toMap(Status::getErrorCode,
                        x -> x,
                        (u, v) -> u,
                        () -> new EnumMap<>(WebhookErrorCode.class)));

        Status(String statusCode, String description, @Nullable WebhookErrorCode errorCode) {
            this.statusCode = statusCode;
            this.description = description;
            this.errorCode = errorCode;
        }

        static Optional<Status> fromWebhookErrorCode(WebhookErrorCode errorCode) {
            return Optional.ofNullable(BY_ERROR_CODE.get(errorCode));
        }
    }
}
