package ru.yandex.quasar.billing.services.processing.yapay;

public class YaPayClientException extends RuntimeException {
    public YaPayClientException(String message) {
        super(message);
    }

    public YaPayClientException(String message, Throwable cause) {
        super(message, cause);
    }
}
