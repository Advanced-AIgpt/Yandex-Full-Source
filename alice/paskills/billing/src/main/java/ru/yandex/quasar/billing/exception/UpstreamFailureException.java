package ru.yandex.quasar.billing.exception;


import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;

@ResponseStatus(value = HttpStatus.BAD_GATEWAY, reason = "Upstream request failed")
public class UpstreamFailureException extends InternalErrorException {
    public UpstreamFailureException(String message, Throwable cause) {
        super(message, cause);
    }

    public UpstreamFailureException(String message) {
        super(message);
    }
}
