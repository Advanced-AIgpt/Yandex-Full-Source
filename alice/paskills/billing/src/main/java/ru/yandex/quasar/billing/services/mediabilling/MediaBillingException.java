package ru.yandex.quasar.billing.services.mediabilling;

class MediaBillingException extends RuntimeException {

    MediaBillingException(String message, Throwable cause) {
        super(message, cause);
    }

    MediaBillingException(String message) {
        super(message);
    }

    MediaBillingException(Throwable cause) {
        super(cause);
    }
}
