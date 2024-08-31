package ru.yandex.quasar.billing.exception;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ResponseStatus;


@ResponseStatus(HttpStatus.NOT_FOUND)
public class NotFoundException extends AbstractHTTPException {

    public NotFoundException(String message) {
        super(message);
    }

    public NotFoundException(String message, Throwable cause) {
        super(message, cause);
    }

    public static NotFoundException unknownProvider(String provider) {
        return new NotFoundException(String.format("Unknown provider '%s'", provider));
    }
}
