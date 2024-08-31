package ru.yandex.quasar.billing.services.mediabilling;

import lombok.Data;

@Data
public class PromoCodeActivationResult {
    private final MediaBillingClient.MusicPromoActivationResult status;

    // activated product info to be added further here
}
