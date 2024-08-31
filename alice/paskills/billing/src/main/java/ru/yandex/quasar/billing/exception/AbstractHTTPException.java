package ru.yandex.quasar.billing.exception;


import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Optional;
import java.util.UUID;

import org.springframework.core.annotation.AnnotatedElementUtils;
import org.springframework.http.HttpStatus;
import org.springframework.util.StringUtils;
import org.springframework.web.bind.annotation.ResponseStatus;

/**
 * Base class for our custom HTTP exceptions.
 * <p>
 * These are intercepted and properly logged / etc.
 * <p>
 * Children are encouraged to annotate with {@link org.springframework.web.bind.annotation.ResponseStatus}
 * and override {@link #getExternalMessage()} / {@link #getInternalMessage()} as needed.
 */
public abstract class AbstractHTTPException extends RuntimeException {
    private final UUID exceptionId;
    private final ZonedDateTime exceptionTime;

    public AbstractHTTPException(String message, Throwable cause) {
        super(message, cause);

        this.exceptionId = UUID.randomUUID();
        this.exceptionTime = ZonedDateTime.now();
    }

    public AbstractHTTPException(String message) {
        super(message);

        this.exceptionId = UUID.randomUUID();
        this.exceptionTime = ZonedDateTime.now();
    }

    public AbstractHTTPException() {
        super();
        this.exceptionId = UUID.randomUUID();
        this.exceptionTime = ZonedDateTime.now();
    }

    /**
     * @return a message to be returned to the caller / user. If nothing is given in annotation return own message.
     */
    public String getExternalMessage() {
        return getResponseStatusAnnotation().map(ResponseStatus::reason).filter(StringUtils::hasText)
                .orElse(getMessage());
    }

    /**
     * @return a possibly different message to be logged / viewed internally. May contain extra info.
     */
    public String getInternalMessage() {
        return getMessage();
    }

    public String getExceptionId() {
        return exceptionId.toString();
    }

    public String getExceptionTime() {
        return exceptionTime.format(DateTimeFormatter.ISO_DATE_TIME);
    }

    public HttpStatus getStatus() {
        return getResponseStatusAnnotation().map(ResponseStatus::value).orElse(HttpStatus.INTERNAL_SERVER_ERROR);
    }

    private Optional<ResponseStatus> getResponseStatusAnnotation() {
        return Optional.ofNullable(AnnotatedElementUtils.findMergedAnnotation(this.getClass(), ResponseStatus.class));
    }
}
