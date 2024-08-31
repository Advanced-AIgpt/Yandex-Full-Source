package ru.yandex.quasar.billing.exception;

public class RetryPurchaseException extends AbstractHTTPException {
    public RetryPurchaseException(String message) {
        super(message);
    }

    public RetryPurchaseException(String message, Throwable cause) {
        super(message, cause);
    }
}
