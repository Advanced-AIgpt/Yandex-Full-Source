package ru.yandex.quasar.billing.services.processing.trust;

public class RefundException extends ProcessingException {
    public RefundException(String message) {
        super(message);
    }

    public RefundException(String message, Throwable cause) {
        super(message, cause);
    }
}
