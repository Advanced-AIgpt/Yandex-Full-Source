package ru.yandex.quasar.billing.providers;

public class WrongContentTypeException extends RuntimeException {
    public WrongContentTypeException() {
    }

    public WrongContentTypeException(String message) {
        super(message);
    }

    public WrongContentTypeException(String message, Throwable cause) {
        super(message, cause);
    }

    public WrongContentTypeException(Throwable cause) {
        super(cause);
    }

    public WrongContentTypeException(String message, Throwable cause, boolean enableSuppression,
                                     boolean writableStackTrace) {
        super(message, cause, enableSuppression, writableStackTrace);
    }
}
