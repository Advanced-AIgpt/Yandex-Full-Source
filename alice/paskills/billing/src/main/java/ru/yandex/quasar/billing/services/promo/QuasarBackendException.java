package ru.yandex.quasar.billing.services.promo;

import org.springframework.http.HttpStatus;

public class QuasarBackendException extends RuntimeException {
    private final HttpStatus status;

    public QuasarBackendException(HttpStatus status) {
        this.status = status;
    }

    public QuasarBackendException(HttpStatus status, String message) {
        super(message);
        this.status = status;
    }

    public QuasarBackendException(HttpStatus status, Throwable cause) {
        super(cause);
        this.status = status;
    }

    public QuasarBackendException(String message, Throwable cause) {
        super(message, cause);
        this.status = null;
    }

    public HttpStatus getStatus() {
        return status;
    }
}
