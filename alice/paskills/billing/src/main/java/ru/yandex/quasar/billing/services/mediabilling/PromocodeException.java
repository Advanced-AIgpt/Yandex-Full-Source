package ru.yandex.quasar.billing.services.mediabilling;

import ru.yandex.quasar.billing.services.mediabilling.MediaBillingClient.MusicPromoActivationResult;

public class PromocodeException extends RuntimeException {
    private final MusicPromoActivationResult error;

    public PromocodeException(MusicPromoActivationResult error) {
        this.error = error;
    }
}
